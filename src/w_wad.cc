//------------------------------------------------------------------------
//  WAD Reading / Writing
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include <algorithm>

#include "Errors.h"
#include "lib_adler.h"
#include "w_rawdef.h"
#include "w_wad.h"


Wad_file * instance::game_wad;
Wad_file * instance::edit_wad;

std::vector<Wad_file *> master_dir;

// UDMF support is unfinished and hence disabled by default.
bool global::udmf_testing = false;


#define MAX_LUMPS_IN_A_LEVEL	21

//
// Wad namespace string
//
const char *WadNamespaceString(WadNamespace ns)
{
	switch(ns)
	{
		case WadNamespace_Flats:
			return "F";
		case WadNamespace_Global:
			return "(global)";
		case WadNamespace_Sprites:
			return "S";
		case WadNamespace_TextureLumps:
			return "TX";
		default:
			return "(invalid)";
	}
}


//------------------------------------------------------------------------
//  LUMP Handling
//------------------------------------------------------------------------

Lump_c::Lump_c(Wad_file *_par, const SString &_nam, int _start, int _len) :
	parent(_par), l_start(_start), l_length(_len)
{
	// ensure lump name is uppercase
	name = _nam.asUpper();
}


Lump_c::Lump_c(Wad_file *_par, const struct raw_wad_entry_s *entry) :
	parent(_par)
{
	// handle the entry name, which can lack a terminating NUL
	SString buffer(entry->name, 8);

	name = buffer;

	l_start  = LE_U32(entry->pos);
	l_length = LE_U32(entry->size);

//	DebugPrintf("new lump '%s' @ %d len:%d\n", name, l_start, l_length);

	if (l_length == 0)
		l_start = 0;
}

void Lump_c::MakeEntry(struct raw_wad_entry_s *entry)
{
	W_StoreString(entry->name, name, sizeof(entry->name));

	entry->pos  = LE_U32(l_start);
	entry->size = LE_U32(l_length);
}


void Lump_c::Rename(const char *new_name)
{
	name = SString(new_name).asUpper();
}


bool Lump_c::Seek(int offset)
{
	SYS_ASSERT(offset >= 0);

	return (fseek(parent->fp, l_start + offset, SEEK_SET) == 0);
}


bool Lump_c::Read(void *data, int len)
{
	SYS_ASSERT(data && len > 0);

	return (fread(data, len, 1, parent->fp) == 1);
}

//
// read a line of text, returns true if OK, false on EOF
//
bool Lump_c::GetLine(SString &string)
{
	SYS_ASSERT(!!parent->fp);
	long curPos = ftell(parent->fp);
	if(curPos < 0)
		return false;	// EOF

	string.clear();
	for(; curPos < l_length; ++curPos)
	{
		string.push_back(fgetc(parent->fp));
		if(string.back() == '\n')
			break;
		if(ferror(parent->fp))
			return false;
		if(feof(parent->fp))
			break;
	}
	return true;	// OK
}

bool Lump_c::Write(const void *data, int len)
{
	SYS_ASSERT(data && len > 0);

	l_length += len;

	return (fwrite(data, len, 1, parent->fp) == 1);
}


void Lump_c::Printf(EUR_FORMAT_STRING(const char *msg), ...)
{
	va_list args;

	va_start(args, msg);
	SString buffer = SString::vprintf(msg, args);
	va_end(args);

	Write(buffer.c_str(), (int)buffer.length());
}


bool Lump_c::Finish()
{
	if (l_length == 0)
		l_start = 0;

	return parent->FinishLump(l_length);
}


//------------------------------------------------------------------------
//  WAD Reading Interface
//------------------------------------------------------------------------

Wad_file::~Wad_file()
{
	LogPrintf("Closing WAD file: %s\n", filename.c_str());

	fclose(fp);

	// free the directory
	for (LumpRef &lumpRef : directory)
		delete lumpRef.lump;

	directory.clear();
}


Wad_file * Wad_file::Open(const SString &filename, WadOpenMode mode)
{
	SYS_ASSERT(mode == WadOpenMode_read || mode == WadOpenMode_write || mode == WadOpenMode_append);

	if (mode == WadOpenMode_write)
		return Create(filename, mode);

	LogPrintf("Opening WAD file: %s\n", filename.c_str());

	FILE *fp = NULL;

retry:
	// TODO: #55 unicode
	fp = fopen(filename.c_str(), (mode == WadOpenMode_read ? "rb" : "r+b"));

	if (! fp)
	{
		// mimic the fopen() semantics
		if (mode == WadOpenMode_append && errno == ENOENT)
			return Create(filename, mode);

		// if file is read-only, open in 'r' mode instead
		if (mode == WadOpenMode_append && (errno == EACCES || errno == EROFS))
		{
			LogPrintf("Open r/w failed, trying again in read mode...\n");
			mode = WadOpenMode_read;
			goto retry;
		}

		int what = errno;
		LogPrintf("Open failed: %s\n", GetErrorMessage(what).c_str());
		return NULL;
	}

	Wad_file *w = new Wad_file(filename, mode, fp);

	// determine total size (seek to end)
	if (fseek(fp, 0, SEEK_END) != 0)
		ThrowException("Error determining WAD size.\n");

	w->total_size = (int)ftell(fp);

	DebugPrintf("total_size = %d\n", w->total_size);

	if (w->total_size < 0)
		ThrowException("Error determining WAD size.\n");

	if (! w->ReadDirectory())
	{
		delete w;
		LogPrintf("Open wad failed (reading directory)\n");
		return NULL;
	}

	w->DetectLevels();
	w->ProcessNamespaces();

	return w;
}


Wad_file * Wad_file::Create(const SString &filename, WadOpenMode mode)
{
	LogPrintf("Creating new WAD file: %s\n", filename.c_str());

	// TODO: #55 unicode
	FILE *fp = fopen(filename.c_str(), "w+b");
	if (! fp)
		return NULL;

	Wad_file *w = new Wad_file(filename, mode, fp);

	// write out base header
	raw_wad_header_t header;

	memset(&header, 0, sizeof(header));
	memcpy(header.ident, "PWAD", 4);

	fwrite(&header, sizeof(header), 1, fp);
	fflush(fp);

	w->total_size = (int)sizeof(header);

	return w;
}


bool Wad_file::Validate(const SString &filename)
{
	FILE *fp = fopen(filename.c_str(), "rb");

	if (! fp)
		return false;

	raw_wad_header_t header;

	if (fread(&header, sizeof(header), 1, fp) != 1)
	{
		fclose(fp);
		return false;
	}

	if (! ( header.ident[1] == 'W' &&
			header.ident[2] == 'A' &&
			header.ident[3] == 'D'))
	{
		fclose(fp);
		return false;
	}

	fclose(fp);

	return true;  // OK
}


static int WhatLevelPart(const SString &name)
{
	if (name.noCaseEqual("THINGS")) return 1;
	if (name.noCaseEqual("LINEDEFS")) return 2;
	if (name.noCaseEqual("SIDEDEFS")) return 3;
	if (name.noCaseEqual("VERTEXES")) return 4;
	if (name.noCaseEqual("SECTORS")) return 5;

	return 0;
}

static bool IsLevelLump(const SString &name)
{
	if (name.noCaseEqual("SEGS")) return true;
	if (name.noCaseEqual("SSECTORS")) return true;
	if (name.noCaseEqual("NODES")) return true;
	if (name.noCaseEqual("REJECT")) return true;
	if (name.noCaseEqual("BLOCKMAP")) return true;
	if (name.noCaseEqual("BEHAVIOR")) return true;
	if (name.noCaseEqual("SCRIPTS")) return true;

	return WhatLevelPart(name) != 0;
}

inline static bool IsGLNodeLump(const SString &name)
{
	return name.noCaseStartsWith("GL_");
}


Lump_c * Wad_file::GetLump(int index) const
{
	SYS_ASSERT(0 <= index && index < NumLumps());
	SYS_ASSERT(directory[index].lump);

	return directory[index].lump;
}


Lump_c * Wad_file::FindLump(const SString &name)
{
	for (auto it = directory.rbegin(); it != directory.rend(); ++it)
		if (it->lump->name.noCaseEqual(name))
			return it->lump;

	return nullptr;  // not found
}

int Wad_file::FindLumpNum(const SString &name)
{
	for (int k = NumLumps() - 1 ; k >= 0 ; k--)
		if (directory[k].lump->name.noCaseEqual(name))
			return k;

	return -1;  // not found
}


int Wad_file::LevelLookupLump(int lev_num, const char *name)
{
	int start = LevelHeader(lev_num);

	// determine how far past the level marker (MAP01 etc) to search
	int finish = LevelLastLump(lev_num);

	for (int k = start+1 ; k <= finish ; k++)
	{
		SYS_ASSERT(0 <= k && k < NumLumps());

		if (directory[k].lump->name.noCaseEqual(name))
			return k;
	}

	return -1;  // not found
}


int Wad_file::LevelFind(const SString &name)
{
	for (int k = 0 ; k < (int)levels.size() ; k++)
	{
		int index = levels[k];

		SYS_ASSERT(0 <= index && index < NumLumps());
		SYS_ASSERT(directory[index].lump);

		if (directory[index].lump->name.noCaseEqual(name))
			return k;
	}

	return -1;  // not found
}


int Wad_file::LevelLastLump(int lev_num)
{
	int start = LevelHeader(lev_num);

	int count = 1;

	// UDMF level?
	if (directory[start + 1].lump->name.noCaseEqual("TEXTMAP"))
	{
		while (count < MAX_LUMPS_IN_A_LEVEL && start+count < NumLumps())
		{
			if (directory[start+count].lump->name.noCaseEqual("ENDMAP"))
			{
				count++;
				break;
			}

			count++;
		}

		return start + count - 1;
	}

	// standard DOOM or HEXEN format
	while (count < MAX_LUMPS_IN_A_LEVEL &&
		   start+count < NumLumps() &&
		   (IsLevelLump(directory[start+count].lump->name) ||
			IsGLNodeLump(directory[start+count].lump->name)) )
	{
		count++;
	}

	return start + count - 1;
}


int Wad_file::LevelFindByNumber(int number)
{
	// sanity check
	if (number <= 0 || number > 99)
		return -1;

	char buffer[16];
	int index;

	 // try MAP## first
	snprintf(buffer, sizeof(buffer), "MAP%02d", number);

	index = LevelFind(buffer);
	if (index >= 0)
		return index;

	// otherwise try E#M#
	snprintf(buffer, sizeof(buffer), "E%dM%d", MAX(1, number / 10), number % 10);

	index = LevelFind(buffer);
	if (index >= 0)
		return index;

	return -1;  // not found
}


int Wad_file::LevelFindFirst()
{
	if (levels.size() > 0)
		return 0;
	else
		return -1;  // none
}


int Wad_file::LevelHeader(int lev_num)
{
	SYS_ASSERT(0 <= lev_num && lev_num < LevelCount());

	return levels[lev_num];
}


MapFormat Wad_file::LevelFormat(int lev_num)
{
	int start = LevelHeader(lev_num);

	if (directory[start+1].lump->name.noCaseEqual("TEXTMAP"))
		return MapFormat::udmf;

	if (start + LL_BEHAVIOR < NumLumps())
	{
		const SString &name = GetLump(start + LL_BEHAVIOR)->Name();

		if (name.noCaseEqual("BEHAVIOR"))
			return MapFormat::hexen;
	}

	return MapFormat::doom;
}


Lump_c * Wad_file::FindLumpInNamespace(const SString &name, WadNamespace group)
{
	for(const LumpRef &lumpRef : directory)
	{
		if(lumpRef.ns != group || !lumpRef.lump->name.noCaseEqual(name))
			continue;
		return lumpRef.lump;
	}

	return nullptr; // not found!
}


bool Wad_file::ReadDirectory()
{
	rewind(fp);

	raw_wad_header_t header;

	if (fread(&header, sizeof(header), 1, fp) != 1)
	{
		LogPrintf("Error reading WAD header.\n");
		return false;
	}

	kind = header.ident[0] == 'I' ? WadKind_IWAD : WadKind_PWAD;

	dir_start = LE_S32(header.dir_start);
	dir_count = LE_S32(header.num_entries);

	if (dir_count < 0)
	{
		LogPrintf("Bad WAD header, invalid number of entries (%d)\n", dir_count);
		return false;
	}

	crc32_c checksum;

	if (fseek(fp, dir_start, SEEK_SET) != 0)
	{
		LogPrintf("Error seeking to WAD directory.\n");
		return false;
	}

	for (int _ = 0 ; _ < dir_count ; _++)
	{
		raw_wad_entry_t entry;

		if (fread(&entry, sizeof(entry), 1, fp) != 1)
		{
			LogPrintf("Error reading entry in WAD directory.\n");
			return false;
		}

		// update the checksum with each _RAW_ entry
		checksum.AddBlock((u8_t *) &entry, sizeof(entry));

		Lump_c *lump = new Lump_c(this, &entry);

		// check if entry is valid
		// [ the total_size value was computed in parent function ]
		if (lump->l_length != 0)
		{
			const int max_size = 99999999;

			if (lump->l_length < 0 || lump->l_start < 0 ||
				lump->l_length >= max_size ||
				lump->l_start > total_size ||
				lump->l_start + lump->l_length > total_size)
			{
				LogPrintf("WARNING: clearing lump '%s' with invalid position (%d+%d > %d)\n",
						  lump->name.c_str(), lump->l_start, lump->l_length, total_size);

				lump->l_start = 0;
				lump->l_length = 0;
			}
		}

		LumpRef lumpRef = {};	// Currently not set, will set in ResolveNamespace
		lumpRef.lump = lump;
		directory.push_back(lumpRef);
	}

	dir_crc = checksum.raw;

	DebugPrintf("Loaded directory. crc = %08x\n", dir_crc);
	return true;
}


void Wad_file::DetectLevels()
{
	// Determine what lumps in the wad are level markers, based on
	// the lumps which follow it.  Store the result in the 'levels'
	// vector.  The test here is rather lax, as I'm told certain
	// wads exist with a non-standard ordering of level lumps.

	for (int k = 0 ; k+1 < NumLumps() ; k++)
	{
		int part_mask  = 0;
		int part_count = 0;

		// check for UDMF levels
		if (global::udmf_testing && directory[k+1].lump->name.noCaseEqual("TEXTMAP"))
		{
			levels.push_back(k);
			DebugPrintf("Detected level : %s (UDMF)\n", directory[k].lump->name.c_str());
			continue;
		}

		// check whether the next four lumps are level lumps
		for (int i = 1 ; i <= 4 ; i++)
		{
			if (k + i >= NumLumps())
				break;

			int part = WhatLevelPart(directory[k+i].lump->name);

			if (part == 0)
				break;

			// do not allow duplicates
			if (part_mask & (1 << part))
				break;

			part_mask |= (1 << part);
			part_count++;
		}

		if (part_count == 4)
		{
			levels.push_back(k);

			DebugPrintf("Detected level : %s\n", directory[k].lump->name.c_str());
		}
	}

	// sort levels into alphabetical order
	// (mainly for the 'N' next map and 'P' prev map commands)

	SortLevels();
}


void Wad_file::SortLevels()
{
	std::sort(levels.begin(), levels.end(), level_name_CMP_pred(this));
}


static bool IsDummyMarker(const SString &name)
{
	// matches P1_START, F3_END etc...

	if (name.length() < 3)
		return false;

	if (! strchr("SF", toupper(name[0])))
		return false;

	if (! isdigit(name[1]))
		return false;

	if (y_stricmp(name.c_str()+2, "_START") == 0 ||
		y_stricmp(name.c_str()+2, "_END") == 0)
		return true;

	return false;
}


void Wad_file::ProcessNamespaces()
{
	WadNamespace active = WadNamespace_Global;

	for (LumpRef &lumpRef : directory)
	{
		const SString &name = lumpRef.lump->name;

		lumpRef.ns = WadNamespace_Global;	// default it to global

		// skip the sub-namespace markers
		if (IsDummyMarker(name))
			continue;

		if (name.noCaseEqual("S_START") || name.noCaseEqual("SS_START"))
		{
			if (active && active != WadNamespace_Sprites)
				LogPrintf("WARNING: missing %s_END marker.\n", WadNamespaceString(active));

			active = WadNamespace_Sprites;
			continue;
		}
		if (name.noCaseEqual("S_END") || name.noCaseEqual("SS_END"))
		{
			if (active != WadNamespace_Sprites)
				LogPrintf("WARNING: stray S_END marker found.\n");

			active = WadNamespace_Global;
			continue;
		}

		if (name.noCaseEqual("F_START") || name.noCaseEqual("FF_START"))
		{
			if (active && active != WadNamespace_Flats)
				LogPrintf("WARNING: missing %s_END marker.\n", WadNamespaceString(active));

			active = WadNamespace_Flats;
			continue;
		}
		if (name.noCaseEqual("F_END") || name.noCaseEqual("FF_END"))
		{
			if (active != WadNamespace_Flats)
				LogPrintf("WARNING: stray F_END marker found.\n");

			active = WadNamespace_Global;
			continue;
		}

		if (name.noCaseEqual("TX_START"))
		{
			if (active && active != WadNamespace_TextureLumps)
				LogPrintf("WARNING: missing %s_END marker.\n", WadNamespaceString(active));

			active = WadNamespace_TextureLumps;
			continue;
		}
		if (name.noCaseEqual("TX_END"))
		{
			if (active != WadNamespace_TextureLumps)
				LogPrintf("WARNING: stray TX_END marker found.\n");

			active = WadNamespace_Global;
			continue;
		}

		if (active)
		{
			if (lumpRef.lump->Length() == 0)
			{
				LogPrintf("WARNING: skipping empty lump %s in %s_START\n",
						  name.c_str(), WadNamespaceString(active));
				continue;
			}

			lumpRef.ns = active;
		}
	}

	if (active)
		LogPrintf("WARNING: Missing %s_END marker (at EOF)\n", WadNamespaceString(active));
}


bool Wad_file::WasExternallyModified()
{
	if (fseek(fp, 0, SEEK_END) != 0)
		FatalError("Error determining WAD size.\n");

	if (total_size != (int)ftell(fp))
		return true;

	rewind(fp);

	raw_wad_header_t header;

	if (fread(&header, sizeof(header), 1, fp) != 1)
		FatalError("Error reading WAD header.\n");

	if (dir_start != LE_S32(header.dir_start) ||
		dir_count != LE_S32(header.num_entries))
		return true;

	fseek(fp, dir_start, SEEK_SET);

	crc32_c checksum;

	for (int _ = 0 ; _ < dir_count ; _++)
	{
		raw_wad_entry_t entry;

		if (fread(&entry, sizeof(entry), 1, fp) != 1)
			FatalError("Error reading WAD directory.\n");

		checksum.AddBlock((u8_t *) &entry, sizeof(entry));

	}

	DebugPrintf("New CRC : %08x\n", checksum.raw);

	return (dir_crc != checksum.raw);
}


//------------------------------------------------------------------------
//  WAD Writing Interface
//------------------------------------------------------------------------

void Wad_file::BeginWrite()
{
	if (mode == WadOpenMode_read)
		BugError("Wad_file::BeginWrite() called on read-only file\n");

	if (begun_write)
		BugError("Wad_file::BeginWrite() called again without EndWrite()\n");

	// put the size into a quantum state
	total_size = 0;

	begun_write = true;
}


void Wad_file::EndWrite()
{
	if (! begun_write)
		BugError("Wad_file::EndWrite() called without BeginWrite()\n");

	begun_write = false;

	WriteDirectory();

	// reset the insertion point
	insert_point = -1;
}


void Wad_file::RenameLump(int index, const char *new_name)
{
	SYS_ASSERT(begun_write);
	SYS_ASSERT(0 <= index && index < NumLumps());

	Lump_c *lump = directory[index].lump;
	SYS_ASSERT(lump);

	lump->Rename(new_name);
}


void Wad_file::RemoveLumps(int index, int count)
{
	SYS_ASSERT(begun_write);
	SYS_ASSERT(0 <= index && index < NumLumps());
	SYS_ASSERT(directory[index].lump);

	int i;

	for (i = 0 ; i < count ; i++)
	{
		delete directory[index + i].lump;
	}

	for (i = index ; i+count < NumLumps() ; i++)
		directory[i] = directory[i+count];

	directory.resize(directory.size() - (size_t)count);

	FixLevelGroup(index, 0, count);

	// reset the insertion point
	insert_point = -1;

	ProcessNamespaces();
}


void Wad_file::RemoveLevel(int lev_num)
{
	SYS_ASSERT(begun_write);
	SYS_ASSERT(0 <= lev_num && lev_num < LevelCount());

	int start  = LevelHeader(lev_num);
	int finish = LevelLastLump(lev_num);

	// NOTE: FixGroup() will remove the entry in levels[]

	RemoveLumps(start, finish - start + 1);
}


void Wad_file::RemoveGLNodes(int lev_num)
{
	SYS_ASSERT(begun_write);
	SYS_ASSERT(0 <= lev_num && lev_num < LevelCount());

	int start  = LevelHeader(lev_num);
	int finish = LevelLastLump(lev_num);

	start++;

	while (start <= finish &&
		   IsLevelLump(directory[start].lump->name))
	{
		start++;
	}

	int count = 0;

	while (start+count <= finish &&
		   IsGLNodeLump(directory[start+count].lump->name))
	{
		count++;
	}

	if (count > 0)
		RemoveLumps(start, count);
}


void Wad_file::RemoveZNodes(int lev_num)
{
	SYS_ASSERT(begun_write);
	SYS_ASSERT(0 <= lev_num && lev_num < LevelCount());

	int start  = LevelHeader(lev_num);
	int finish = LevelLastLump(lev_num);

	for ( ; start <= finish ; start++)
	{
		if (directory[start].lump->name.noCaseEqual("ZNODES"))
		{
			RemoveLumps(start, 1);
			break;
		}
	}
}


void Wad_file::FixLevelGroup(int index, int num_added, int num_removed)
{
	bool did_remove = false;

	for (int k = 0 ; k < (int)levels.size() ; k++)
	{
		if (levels[k] < index)
			continue;

		if (levels[k] < index + num_removed)
		{
			levels[k] = -1;
			did_remove = true;
			continue;
		}

		levels[k] += num_added;
		levels[k] -= num_removed;
	}

	if (did_remove)
	{
		std::vector<int>::iterator ENDP;
		ENDP = std::remove(levels.begin(), levels.end(), -1);
		levels.erase(ENDP, levels.end());
	}
}


Lump_c * Wad_file::AddLump(const SString &name, int max_size)
{
	SYS_ASSERT(begun_write);

	begun_max_size = max_size;

	int start = PositionForWrite(max_size);

	Lump_c *lump = new Lump_c(this, name, start, 0);

	// check if the insert_point is still valid
	if (insert_point >= NumLumps())
		insert_point = -1;

	if (insert_point >= 0)
	{
		FixLevelGroup(insert_point, 1, 0);

		LumpRef lumpRef = {};
		lumpRef.lump = lump;
		directory.insert(directory.begin() + insert_point, lumpRef);

		insert_point++;
	}
	else  // add to end
	{
		LumpRef lumpRef = {};
		lumpRef.lump = lump;
		directory.push_back(lumpRef);
	}

	ProcessNamespaces();

	return lump;
}


void Wad_file::RecreateLump(Lump_c *lump, int max_size)
{
	SYS_ASSERT(begun_write);

	begun_max_size = max_size;

	int start = PositionForWrite(max_size);

	lump->l_start  = start;
	lump->l_length = 0;
}


Lump_c * Wad_file::AddLevel(const SString &name, int max_size, int *lev_num)
{
	int actual_point = insert_point;

	if (actual_point < 0 || actual_point > NumLumps())
		actual_point = NumLumps();

	Lump_c * lump = AddLump(name, max_size);

	if (lev_num)
	{
		*lev_num = (int)levels.size();
	}

	levels.push_back(actual_point);

	return lump;
}


void Wad_file::InsertPoint(int index)
{
	// this is validated on usage
	insert_point = index;
}


int Wad_file::HighWaterMark()
{
	int offset = (int)sizeof(raw_wad_header_t);

	for (const LumpRef &lumpRef : directory)
	{
		// ignore zero-length lumps (their offset could be anything)
		const Lump_c *lump = lumpRef.lump;
		if (lump->Length() <= 0)
			continue;

		int l_end = lump->l_start + lump->l_length;

		l_end = ((l_end + 3) / 4) * 4;

		if (offset < l_end)
			offset = l_end;
	}

	return offset;
}


int Wad_file::FindFreeSpace(int length)
{
	length = ((length + 3) / 4) * 4;

	// collect non-zero length lumps and sort by their offset
	std::vector<Lump_c *> sorted_dir;

	for (const LumpRef &lumpRef : directory)
	{
		Lump_c *lump = lumpRef.lump;
		if (lump->Length() > 0)
			sorted_dir.push_back(lump);
	}

	std::sort(sorted_dir.begin(), sorted_dir.end(), Lump_c::offset_CMP_pred());


	int offset = (int)sizeof(raw_wad_header_t);

	for (unsigned int k = 0 ; k < sorted_dir.size() ; k++)
	{
		Lump_c *lump = sorted_dir[k];

		int l_start = lump->l_start;
		int l_end   = lump->l_start + lump->l_length;

		l_end = ((l_end + 3) / 4) * 4;

		if (l_end <= offset)
			continue;

		if (l_start >= offset + length)
			continue;

		// the lump overlapped the current gap, so bump offset

		offset = l_end;
	}

	return offset;
}


int Wad_file::PositionForWrite(int max_size)
{
	int want_pos;

	if (max_size <= 0)
		want_pos = HighWaterMark();
	else
		want_pos = FindFreeSpace(max_size);

	// determine if position is past end of file
	// (difference should only be a few bytes)
	//
	// Note: doing this for every new lump may be a little expensive,
	//       but trying to optimise it away will just make the code
	//       needlessly complex and hard to follow.

	if (fseek(fp, 0, SEEK_END) < 0)
		FatalError("Error seeking to new write position.\n");

	total_size = (int)ftell(fp);

	if (total_size < 0)
		FatalError("Error seeking to new write position.\n");

	if (want_pos > total_size)
	{
		if (want_pos >= total_size + 8)
			FatalError("Internal Error: lump positions are beyond end of file\n(%d > %d)\n",
				want_pos, total_size);

		WritePadding(want_pos - total_size);
	}
	else if (want_pos == total_size)
	{
		/* ready to write */
	}
	else
	{
		if (fseek(fp, want_pos, SEEK_SET) < 0)
			FatalError("Error seeking to new write position.\n");
	}

	DebugPrintf("POSITION FOR WRITE: %d  (total_size %d)\n", want_pos, total_size);

	return want_pos;
}


bool Wad_file::FinishLump(int final_size)
{
	fflush(fp);

	// sanity check
	if (begun_max_size >= 0)
		if (final_size > begun_max_size)
			BugError("Internal Error: wrote too much in lump (%d > %d)\n",
					 final_size, begun_max_size);

	int pos = (int)ftell(fp);

	if (pos & 3)
	{
		WritePadding(4 - (pos & 3));
	}

	fflush(fp);
	return true;
}


int Wad_file::WritePadding(int count)
{
	static byte zeros[8] = { 0,0,0,0,0,0,0,0 };

	SYS_ASSERT(1 <= count && count <= 8);

	fwrite(zeros, count, 1, fp);

	return count;
}


//
// IDEA : Truncate file to "total_size" after writing the directory.
//
//        On Linux / MacOSX, this can be done as follows:
//                 - fflush(fp)   -- ensure STDIO has empty buffers
//                 - ftruncate(fileno(fp), total_size);
//                 - freopen(fp)
//
//        On Windows:
//                 - instead of ftruncate, use _chsize() or _chsize_s()
//                   [ investigate what the difference is.... ]
//


void Wad_file::WriteDirectory()
{
	dir_start = PositionForWrite();
	dir_count = NumLumps();

	DebugPrintf("WriteDirectory...\n");
	DebugPrintf("dir_start:%d  dir_count:%d\n", dir_start, dir_count);

	crc32_c checksum;

	for (const LumpRef &lumpRef : directory)
	{
		Lump_c *lump = lumpRef.lump;
		SYS_ASSERT(lump);

		raw_wad_entry_t entry;

		lump->MakeEntry(&entry);

		// update the CRC
		checksum.AddBlock((u8_t *) &entry, sizeof(entry));

		if (fwrite(&entry, sizeof(entry), 1, fp) != 1)
			ThrowException("Error writing WAD directory.\n");
	}

	dir_crc = checksum.raw;
	DebugPrintf("dir_crc: %08x\n", dir_crc);

	fflush(fp);

	total_size = (int)ftell(fp);
	DebugPrintf("total_size: %d\n", total_size);

	if (total_size < 0)
		ThrowException("Error determining WAD size.\n");

	// update header at start of file

	rewind(fp);

	raw_wad_header_t header;

	memcpy(header.ident, (kind == WadKind_IWAD) ? "IWAD" : "PWAD", 4);

	header.dir_start   = LE_U32(dir_start);
	header.num_entries = LE_U32(dir_count);

	if (fwrite(&header, sizeof(header), 1, fp) != 1)
		ThrowException("Error writing WAD header.\n");

	fflush(fp);
}


bool Wad_file::Backup(const char *new_filename)
{
	fflush(fp);

	return FileCopy(PathName(), new_filename);
}


//------------------------------------------------------------------------
//  GLOBAL API
//------------------------------------------------------------------------


Lump_c * W_FindGlobalLump(const SString &name)
{
	for (int i = (int)master_dir.size()-1 ; i >= 0 ; i--)
	{
		Lump_c *L = master_dir[i]->FindLumpInNamespace(name, WadNamespace_Global);
		if (L)
			return L;
	}

	return NULL;  // not found
}


Lump_c * W_FindSpriteLump(const SString &name)
{
	for (int i = (int)master_dir.size()-1 ; i >= 0 ; i--)
	{
		Lump_c *L = master_dir[i]->FindLumpInNamespace(name, WadNamespace_Sprites);
		if (L)
			return L;
	}

	return NULL;  // not found
}


int W_LoadLumpData(Lump_c *lump, byte ** buf_ptr)
{
	// include an extra byte, used to NUL-terminate a text buffer
	*buf_ptr = new byte[lump->Length() + 1];

	if (lump->Length() > 0)
	{
		if (! lump->Seek() ||
			! lump->Read(*buf_ptr, lump->Length()))
			FatalError("W_LoadLumpData: read error loading lump.\n");
	}

	(*buf_ptr)[lump->Length()] = 0;

	return lump->Length();
}


void W_FreeLumpData(byte ** buf_ptr)
{
	if (*buf_ptr)
	{
		delete[] *buf_ptr;
		*buf_ptr = NULL;
	}
}


//------------------------------------------------------------------------

void MasterDir_Add(Wad_file *wad)
{
	DebugPrintf("MasterDir: adding '%s'\n", wad->PathName().c_str());

	master_dir.push_back(wad);
}


void MasterDir_Remove(Wad_file *wad)
{
	DebugPrintf("MasterDir: removing '%s'\n", wad->PathName().c_str());

	std::vector<Wad_file *>::iterator ENDP;

	ENDP = std::remove(master_dir.begin(), master_dir.end(), wad);

	master_dir.erase(ENDP, master_dir.end());
}


void MasterDir_CloseAll()
{
	while (master_dir.size() > 0)
	{
		Wad_file *wad = master_dir.back();

		master_dir.pop_back();

		delete wad;
	}
}


static bool W_FilenameAbsEqual(const SString &A, const SString &B)
{
	const SString &A_path = GetAbsolutePath(A);
	const SString &B_path = GetAbsolutePath(B);
	return A_path.noCaseEqual(B_path);
}


void W_StoreString(char *buf, const SString &str, size_t buflen)
{
	memset(buf, 0, buflen);

	for (size_t i = 0 ; i < buflen && str[i] ; i++)
		buf[i] = str[i];
}


bool MasterDir_HaveFilename(const SString &chk_path)
{
	for (unsigned int k = 0 ; k < master_dir.size() ; k++)
	{
		const SString &wad_path = master_dir[k]->PathName();

		if (W_FilenameAbsEqual(wad_path, chk_path))
			return true;
	}

	return false;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
