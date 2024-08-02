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
#include "Instance.h"

#include <algorithm>

#include "Errors.h"
#include "lib_adler.h"
#include "m_files.h"
#include "SafeOutFile.h"
#include "w_rawdef.h"
#include "w_wad.h"

#include <assert.h>

// UDMF support is unfinished and hence disabled by default.
bool global::udmf_testing = false;


#define MAX_LUMPS_IN_A_LEVEL	21

//
// Wad namespace string
//
static const char *WadNamespaceString(WadNamespace ns) noexcept
{
	switch(ns)
	{
		case WadNamespace::Flats:
			return "F";
		case WadNamespace::Global:
			return "(global)";
		case WadNamespace::Sprites:
			return "S";
		case WadNamespace::TextureLumps:
			return "TX";
		default:
			return "(invalid)";
	}
}


//------------------------------------------------------------------------
//  LUMP Handling
//------------------------------------------------------------------------

Lump_c::Lump_c(const SString &_nam)
{
	// ensure lump name is uppercase
	name = _nam.asUpper();
	if(name.length() > 8)
		name.erase(8, std::string::npos);
}

void Lump_c::Rename(const char *new_name)
{
	name = SString(new_name).asUpper();
	if(name.length() > 8)
		name.erase(8, std::string::npos);
}

bool LumpInputStream::read(void *data, int len) noexcept
{
	bool result = true;
	if(pos + len > lump.Length())
	{
		result = false;
		len = lump.Length() - pos;
	}
	memcpy(data, lump.getData().data() + pos, len);
	pos += len;
	return result;
}

bool LumpInputStream::readLine(SString &string) noexcept
{
	if(pos >= lump.Length())
		return false;	// EOF

	string.clear();
	for(; pos < lump.Length(); ++pos)
	{
		string.push_back(static_cast<char>(lump.getData()[pos]));
		if(string.back() == '\n')
		{
			++pos;
			break;
		}
	}
	return true;	// OK
}

void Lump_c::Write(const void *vdata, int len)
{
	auto data = static_cast<const byte *>(vdata);
	mData.insert(mData.begin() + mPos, data, data + len);
	mPos += len;
}


void Lump_c::Printf(EUR_FORMAT_STRING(const char *msg), ...)
{
	va_list args;

	va_start(args, msg);
	SString buffer = SString::vprintf(msg, args);
	va_end(args);

	Write(buffer.c_str(), (int)buffer.length());
}

//
// Writes the data by freading from FILE. Returns the result of the involved
// fread call, as number of bytes read. Be sure to check feof and ferror if not
// as expected.
//
size_t Lump_c::writeData(FILE *f, int len)
{
	mData.insert(mData.begin() + mPos, len, 0);
	size_t actualRead = fread(mData.data() + mPos, 1, len, f);
	if((int)actualRead < len)
	{
		mData.erase(mData.begin() + mPos + actualRead,
					mData.begin() + mPos + len);
	}
	mPos += (int)actualRead;
	return actualRead;
}

//
// Returns the name coded into 8 bytes
//
int64_t Lump_c::getName8() const noexcept
{
	union
	{
		char cbuf[8];
		int64_t cint;
	} buffer;
	memcpy(buffer.cbuf, Name().c_str(), std::min(Name().length() + 1, sizeof(buffer)));
	return buffer.cint;
}

//------------------------------------------------------------------------
//  WAD Reading Interface
//------------------------------------------------------------------------

Wad_file::~Wad_file()
{
	gLog.printf("Closing WAD file: %s\n", filename.u8string().c_str());
}


std::shared_ptr<Wad_file> Wad_file::Open(const fs::path &filename,
										 WadOpenMode mode)
{
	SYS_ASSERT(mode == WadOpenMode::read || mode == WadOpenMode::write || mode == WadOpenMode::append);

	if (mode == WadOpenMode::write)
		return Create(filename, mode);

	gLog.printf("Opening WAD file: %s\n", filename.u8string().c_str());

	FILE *fp = NULL;

retry:
	// TODO: #55 unicode
	fp = fopen(filename.u8string().c_str(), (mode == WadOpenMode::read ? "rb" : "r+b"));

	if (! fp)
	{
		// mimic the fopen() semantics
		if (mode == WadOpenMode::append && errno == ENOENT)
			return Create(filename, mode);

		// if file is read-only, open in 'r' mode instead
		if (mode == WadOpenMode::append && (errno == EACCES || errno == EROFS))
		{
			gLog.printf("Open r/w failed, trying again in read mode...\n");
			mode = WadOpenMode::read;
			goto retry;
		}

		int what = errno;
		gLog.printf("Open failed: %s\n", GetErrorMessage(what).c_str());
		return NULL;
	}

	return createAndReadDirectory(filename, mode, fp);
}

std::shared_ptr<Wad_file> Wad_file::loadFromFile(const fs::path &filename)
{
	gLog.printf("Opening WAD file: %s\n", filename.u8string().c_str());
	// TODO: #55 unicode
	FILE *fp = fopen(filename.u8string().c_str(), "rb");
	if(!fp)
	{
		gLog.printf("Open failed: %s\n", GetErrorMessage(errno).c_str());
		return nullptr;
	}
	return createAndReadDirectory(filename, WadOpenMode::append, fp);
}

std::shared_ptr<Wad_file> Wad_file::readFromDir(const fs::path &path)
{
	gLog.printf("Opening WAD folder: %s\n", path.u8string().c_str());

	// Reading from folder follows this rule (which should be a cross-port standard):
	// https://eternity.youfailit.net/wiki/ZIP

	// Currently no editing is allowed in folder paths, which are only for resources anyway
	auto wraw = new Wad_file(path, WadOpenMode::read);
	auto w = std::shared_ptr<Wad_file>(wraw);

	fs::directory_iterator iterator;
	try
	{
		iterator = fs::directory_iterator(path);
	}
	catch(const fs::filesystem_error &e)
	{
		gLog.printf("Open failed: %s\n", e.what());
		return nullptr;
	}

	auto tryAddNewLump = [](std::shared_ptr<Wad_file> &w, const fs::path &path, WadNamespace nameSpace)
	{
		SString lumpname = SString(path.filename().replace_extension().u8string()).asUpper().substr(0, 8);

		for(char &c : lumpname)
			if(c == '^')
				c = '\\';	// de-escape backslashes, due to vile precedent

		Lump_c *lump = new Lump_c(lumpname);
		std::vector<uint8_t> data;
		if(!FileLoad(path, data))
		{
			gLog.printf("Failed reading %s\n", path.u8string().c_str());
			delete lump;
			return;
		}

		lump->setData(std::move(data));

		LumpRef ref = {};
		ref.lump.reset(lump);
		ref.ns = nameSpace;
		w->directory.push_back(std::move(ref));
	};

	for(const auto &entry : iterator)
	{
		if(fs::is_regular_file(entry.path()))
		{
			// Promptly add the lump now
			tryAddNewLump(w, entry.path(), WadNamespace::Global);
		}
		else if(fs::is_directory(entry.path()))
		{
			fs::directory_iterator subiterator;
			try
			{
				subiterator = fs::directory_iterator(entry.path());
			}
			catch(const fs::filesystem_error &e)
			{
				gLog.printf("Error opening subfolder %s: %s\n", entry.path().u8string().c_str(), e.what());
			}
			for(const auto &subentry : subiterator)
			{
				if(!fs::is_regular_file(subentry.path()))
					continue;	// only allow one sub level
				// Check namespaces by the way
				SString folderName = entry.path().filename().u8string();
				WadNamespace nameSpace;
				if(folderName.noCaseEqual("flats"))
					nameSpace = WadNamespace::Flats;
				else if(folderName.noCaseEqual("sprites"))
					nameSpace = WadNamespace::Sprites;
				else if(folderName.noCaseEqual("textures"))
					nameSpace = WadNamespace::TextureLumps;
				else
					nameSpace = WadNamespace::Global;

				tryAddNewLump(w, subentry.path(), nameSpace);
			}
		}
		else
		{
			gLog.printf("Ignoring irregular file path %s\n", entry.path().u8string().c_str());
		}
	}

	// No DetectLevels allowed either.

	return w;
}

std::shared_ptr<Wad_file> Wad_file::Create(const fs::path &filename,
										   WadOpenMode mode)
{
	gLog.printf("Creating new WAD file: %s\n", filename.u8string().c_str());

	return std::shared_ptr<Wad_file>(new Wad_file(filename, mode));
}

std::shared_ptr<Wad_file> Wad_file::createAndReadDirectory(const fs::path &filename,
														   WadOpenMode mode, FILE *fp)
{
	auto wraw = new Wad_file(filename, mode);
	auto w = std::shared_ptr<Wad_file>(wraw);

	// determine total size (seek to end)
	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		ThrowException("Error determining WAD size.\n");
	}

	int total_size = (int)ftell(fp);

	gLog.debugPrintf("total_size = %d\n", total_size);

	if (total_size < 0)
	{
		fclose(fp);
		ThrowException("Error determining WAD size.\n");
	}

	if (! w->ReadDirectory(fp, total_size))
	{
		gLog.printf("Open wad failed (reading directory)\n");
		fclose(fp);
		return NULL;
	}

	w->DetectLevels();
	w->ProcessNamespaces();

	fclose(fp);

	return w;
}

bool Wad_file::Validate(const fs::path &filename)
{
	std::ifstream stream(filename, std::ios::binary);

	if (! stream.is_open())
		return false;

	raw_wad_header_t header;

	if(!stream.read(reinterpret_cast<char *>(&header), sizeof(header)) ||
	   stream.gcount() != sizeof(header))
	{
		return false;
	}

	stream.close();

	if (! ( header.ident[1] == 'W' &&
			header.ident[2] == 'A' &&
			header.ident[3] == 'D'))
	{
		return false;
	}

	return true;  // OK
}


static int WhatLevelPart(const SString &name) noexcept
{
	if (name.noCaseEqual("THINGS")) return 1;
	if (name.noCaseEqual("LINEDEFS")) return 2;
	if (name.noCaseEqual("SIDEDEFS")) return 3;
	if (name.noCaseEqual("VERTEXES")) return 4;
	if (name.noCaseEqual("SECTORS")) return 5;

	return 0;
}

static bool IsLevelLump(const SString &name) noexcept
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

inline static bool IsGLNodeLump(const SString &name) noexcept
{
	return name.noCaseStartsWith("GL_");
}

//
// Wad total size
//
int Wad_file::TotalSize() const noexcept
{
	int size = 12;
	for(const LumpRef &ref : directory)
	{
		assert(ref.lump.get() != nullptr);
		size += ref.lump->Length();
	}
	size += (int)directory.size() * 16;
	return size;
}

Lump_c * Wad_file::GetLump(int index) const noexcept
{
	SYS_ASSERT(0 <= index && index < NumLumps());
	SYS_ASSERT(directory[index].lump);

	return directory[index].lump.get();
}


Lump_c * Wad_file::FindLump(const SString &name) const noexcept
{
	for (auto it = directory.rbegin(); it != directory.rend(); ++it)
		if (it->lump->name.noCaseEqual(name))
			return it->lump.get();

	return nullptr;  // not found
}

int Wad_file::FindLumpNum(const SString &name) const noexcept
{
	for (int k = NumLumps() - 1 ; k >= 0 ; k--)
		if (directory[k].lump->name.noCaseEqual(name))
			return k;

	return -1;  // not found
}


int Wad_file::LevelLookupLump(int lev_num, const char *name) const noexcept
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


int Wad_file::LevelFind(const SString &name) const noexcept
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


int Wad_file::LevelLastLump(int lev_num) const noexcept
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


int Wad_file::LevelFindByNumber(int number) const noexcept
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
	snprintf(buffer, sizeof(buffer), "E%dM%d", std::max(1, number / 10), number % 10);

	index = LevelFind(buffer);
	if (index >= 0)
		return index;

	return -1;  // not found
}


int Wad_file::LevelFindFirst() const noexcept
{
	if (levels.size() > 0)
		return 0;
	else
		return -1;  // none
}


int Wad_file::LevelHeader(int lev_num) const noexcept
{
	SYS_ASSERT(0 <= lev_num && lev_num < LevelCount());

	return levels[lev_num];
}


MapFormat Wad_file::LevelFormat(int lev_num) const noexcept
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


const Lump_c * Wad_file::FindLumpInNamespace(const SString &name, WadNamespace group) const noexcept
{
	for(const LumpRef &lumpRef : directory)
	{
		if(lumpRef.ns != group || !lumpRef.lump->name.noCaseEqual(name))
			continue;
		return lumpRef.lump.get();
	}

	return nullptr; // not found!
}

//
// Searches for the
//
std::vector<SpriteLumpRef> Wad_file::findFirstSpriteLump(const SString &stem) const
{
	auto isSprite = [](const LumpRef &ref)
	{
		if(ref.ns != WadNamespace::Sprites)
			return false;
		const SString &name = ref.lump->name;
		if(name.length() != 6 && name.length() != 8)
			return false;
		if(name[5] < '0' || name[5] > '8')
			return false;
		if(name.length() == 8 && (name[7] < '0' || name[7] > '8'))
			return false;
		return true;
	};

	std::vector<SpriteLumpRef> result;
	SString substem = stem.length() > 4 ? stem.substr(0, 4) : stem;
	std::vector<const Lump_c *> candidates;

	SString foundName;
	const Lump_c *foundLump = nullptr;
	// 1. Find the first ordered stem
	// 2. Find the first ordered frame
	for(const LumpRef &ref : directory)
	{
		if(!isSprite(ref))
			continue;
		const SString &name = ref.lump->name;
		if(name.startsWith(substem.c_str()))
		{
			candidates.push_back(ref.lump.get());
		}
		if(!name.startsWith(stem.c_str()))
			continue;
		if(foundName.empty() || foundName.get() > name.get())
		{
			foundName = name;
			foundLump = ref.lump.get();
		}
	}
	if(foundName.empty())
		return {};

	char letter = 0;
	char rot = 0;

	// 3. Find all the other rotations with that frame
	letter = foundName[4];
	rot = foundName[5];
	if(rot == '0')
		return {{foundLump, false}};	// got one see-all-around sprite already
	// we got some rotation
	result.resize(8);
	// Now look for all rotations
	for(const Lump_c *lump : candidates)
	{
		const SString &name = lump->Name();
		if(stem.length() < 4 && name.substr(0, 4) != foundName.substr(0, 4))
			continue;
		if(name[4] == letter)
			result[name[5] - '1'] = {lump, false};
		if(name.length() == 8 && name[6] == letter)
		{
			if(name[7] == '0')
				return {{lump, true}};
			result[name[7] - '1'] = {lump, true};
		}
	}

	return result;
}

bool Wad_file::ReadDirectory(FILE *fp, int total_size)
{
	rewind(fp);

	raw_wad_header_t header;

	if (fread(&header, sizeof(header), 1, fp) != 1)
	{
		gLog.printf("Error reading WAD header.\n");
		return false;
	}

	kind = header.ident[0] == 'I' ? WadKind::IWAD : WadKind::PWAD;

	int dir_start = LE_S32(header.dir_start);
	int dir_count = LE_S32(header.num_entries);

	if (dir_count < 0)
	{
		gLog.printf("Bad WAD header, invalid number of entries (%d)\n", dir_count);
		return false;
	}

	if (fseek(fp, dir_start, SEEK_SET) != 0)
	{
		gLog.printf("Error seeking to WAD directory.\n");
		return false;
	}

	directory.reserve(dir_count);

	for (int _ = 0 ; _ < dir_count ; _++)
	{
		raw_wad_entry_t entry;

		if (fread(&entry, sizeof(entry), 1, fp) != 1)
		{
			gLog.printf("Error reading entry in WAD directory.\n");
			return false;
		}

		Lump_c *lump = new Lump_c(SString(entry.name, 8));
		int l_length = LE_U32(entry.size);
		int l_start = LE_U32(entry.pos);
		if(l_length == 0)
			l_start = 0;

		// check if entry is valid
		// [ the total_size value was computed in parent function ]
		if (l_length != 0)
		{
			const int max_size = 99999999;

			if (l_length < 0 || l_start < 0 || l_length >= max_size ||
				l_start > total_size || l_start + l_length > total_size)
			{
				gLog.printf("WARNING: clearing lump '%s' with invalid position (%d+%d > %d)\n",
						  lump->name.c_str(), l_start, l_length, total_size);

				l_start = 0;
				l_length = 0;
			}

			if(l_length > 0)
			{
				long curpos = ftell(fp);
				if(curpos < 0)
				{
					gLog.printf("%s: ftell failed with error %d\n", __func__,
								errno);
					return false;
				}
				if(fseek(fp, l_start, SEEK_SET) < 0)
				{
					gLog.printf("%s: fseek failed with error %d\n", __func__,
								errno);
					return false;
				}
				if((int)lump->writeData(fp, l_length) < l_length)
				{
					gLog.printf("%s: failed reading %d bytes for lump '%s'\n",
								__func__, l_length, lump->name.c_str());
					return false;
				}
				if(fseek(fp, curpos, SEEK_SET) < 0)
				{
					gLog.printf("%s: fseek back failed with error %d\n",
								__func__, errno);
					return false;
				}
			}
		}

		LumpRef lumpRef = {};	// Currently not set, will set in ResolveNamespace
		lumpRef.lump.reset(lump);
		directory.push_back(std::move(lumpRef));
	}
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
			gLog.debugPrintf("Detected level : %s (UDMF)\n", directory[k].lump->name.c_str());
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

			gLog.debugPrintf("Detected level : %s\n", directory[k].lump->name.c_str());
		}
	}

	// sort levels into alphabetical order
	// (mainly for the 'N' next map and 'P' prev map commands)

	SortLevels();
}


void Wad_file::SortLevels() noexcept
{
	std::sort(levels.begin(), levels.end(), level_name_CMP_pred(this));
}


static bool IsDummyMarker(const SString &name) noexcept
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
	WadNamespace active = WadNamespace::Global;

	for (LumpRef &lumpRef : directory)
	{
		const SString &name = lumpRef.lump->name;

		lumpRef.ns = WadNamespace::Global;	// default it to global

		// skip the sub-namespace markers
		if (IsDummyMarker(name))
			continue;

		if (name.noCaseEqual("S_START") || name.noCaseEqual("SS_START"))
		{
			if (active != WadNamespace::Global && active != WadNamespace::Sprites)
				gLog.printf("WARNING: missing %s_END marker.\n", WadNamespaceString(active));

			active = WadNamespace::Sprites;
			continue;
		}
		if (name.noCaseEqual("S_END") || name.noCaseEqual("SS_END"))
		{
			if (active != WadNamespace::Sprites)
				gLog.printf("WARNING: stray S_END marker found.\n");

			active = WadNamespace::Global;
			continue;
		}

		if (name.noCaseEqual("F_START") || name.noCaseEqual("FF_START"))
		{
			if (active != WadNamespace::Global && active != WadNamespace::Flats)
				gLog.printf("WARNING: missing %s_END marker.\n", WadNamespaceString(active));

			active = WadNamespace::Flats;
			continue;
		}
		if (name.noCaseEqual("F_END") || name.noCaseEqual("FF_END"))
		{
			if (active != WadNamespace::Flats)
				gLog.printf("WARNING: stray F_END marker found.\n");

			active = WadNamespace::Global;
			continue;
		}

		if (name.noCaseEqual("TX_START"))
		{
			if (active != WadNamespace::Global && active != WadNamespace::TextureLumps)
				gLog.printf("WARNING: missing %s_END marker.\n", WadNamespaceString(active));

			active = WadNamespace::TextureLumps;
			continue;
		}
		if (name.noCaseEqual("TX_END"))
		{
			if (active != WadNamespace::TextureLumps)
				gLog.printf("WARNING: stray TX_END marker found.\n");

			active = WadNamespace::Global;
			continue;
		}

		if (active != WadNamespace::Global)
		{
			if (lumpRef.lump->Length() == 0)
			{
				gLog.printf("WARNING: skipping empty lump %s in %s_START\n",
						  name.c_str(), WadNamespaceString(active));
				continue;
			}

			lumpRef.ns = active;
		}
	}

	if (active != WadNamespace::Global)
		gLog.printf("WARNING: Missing %s_END marker (at EOF)\n", WadNamespaceString(active));
}


//------------------------------------------------------------------------
//  WAD Writing Interface
//------------------------------------------------------------------------

//
// Writes the content to disk now
//
void Wad_file::writeToDisk() noexcept(false)
{
	if(IsReadOnly())
	{
		ThrowException("Cannot overwrite a read-only file (%s)!",
					   filename.u8string().c_str());
	}

	// Write to our path now
	writeToPath(filename);

	// reset the insertion point
	insert_point = -1;
}


void Wad_file::RenameLump(int index, const char *new_name)
{
	SYS_ASSERT(0 <= index && index < NumLumps());

	Lump_c *lump = directory[index].lump.get();
	SYS_ASSERT(lump);

	lump->Rename(new_name);
}


void Wad_file::RemoveLumps(int index, int count)
{
	SYS_ASSERT(0 <= index && index < NumLumps());
	SYS_ASSERT(directory[index].lump);

	directory.erase(directory.begin() + index,
					directory.begin() + index + count);

	FixLevelGroup(index, 0, count);

	// reset the insertion point
	insert_point = -1;

	ProcessNamespaces();
}


std::vector<Lump_c> Wad_file::RemoveLevel(int lev_num)
{
	SYS_ASSERT(0 <= lev_num && lev_num < LevelCount());

	int start  = LevelHeader(lev_num);
	int finish = LevelLastLump(lev_num);

	// NOTE: FixGroup() will remove the entry in levels[]

	std::vector<Lump_c> result;
	for (int i = start; i <= finish; ++i)
		result.push_back(std::move(*directory[i].lump));

	RemoveLumps(start, finish - start + 1);
	return result;
}


void Wad_file::RemoveGLNodes(int lev_num)
{
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

//
// Writes to the given path
//
void Wad_file::writeToPath(const fs::path &path) const noexcept(false)
{
	BufferedOutFile sof(path);
	// Write the header
	if(kind == WadKind::PWAD)
		sof.write("PWAD", 4);
	else
		sof.write("IWAD", 4);

	int32_t numlumps = (int32_t)directory.size();
	sof.write(&numlumps, 4);

	int32_t infotableofs = 12;
	for(const LumpRef &ref : directory)
		infotableofs += (int32_t)ref.lump->Length();

	sof.write(&infotableofs, 4);
	for(const LumpRef &ref : directory)
	{
		assert(ref.lump.get() != nullptr);
		const Lump_c &lump = *ref.lump;
		sof.write(lump.getData().data(), lump.Length());
	}
	infotableofs = 12;
	for(const LumpRef &ref : directory)
	{
		sof.write(&infotableofs, 4);
		const Lump_c &lump = *ref.lump;
		numlumps = lump.Length();
		sof.write(&numlumps, 4);
		infotableofs += numlumps;
		int64_t nm = lump.getName8();
		sof.write(&nm, 8);
	}
	sof.commit();
}


Lump_c & Wad_file::AddLump(const SString &name)
{
	Lump_c *lump = new Lump_c(name);

	// check if the insert_point is still valid
	if (insert_point >= NumLumps())
		insert_point = -1;

	if (insert_point >= 0)
	{
		FixLevelGroup(insert_point, 1, 0);

		LumpRef lumpRef = {};
		lumpRef.lump.reset(lump);
		directory.insert(directory.begin() + insert_point, std::move(lumpRef));

		insert_point++;
	}
	else  // add to end
	{
		LumpRef lumpRef = {};
		lumpRef.lump.reset(lump);
		directory.push_back(std::move(lumpRef));
	}

	ProcessNamespaces();

	return *lump;
}

Lump_c * Wad_file::AddLevel(const SString &name, int *lev_num)
{
	int actual_point = insert_point;

	if (actual_point < 0 || actual_point > NumLumps())
		actual_point = NumLumps();

	Lump_c & lump = AddLump(name);

	if (lev_num)
	{
		*lev_num = (int)levels.size();
	}

	levels.push_back(actual_point);

	return &lump;
}


void Wad_file::InsertPoint(int index) noexcept
{
	// this is validated on usage
	insert_point = index;
}

//
// This one merely saves it as a new filename
//
bool Wad_file::Backup(const fs::path &new_filename) const
{
	try
	{
		writeToPath(new_filename);
	}
	catch(const std::exception &e)
	{
		gLog.printf("Failed backing up %s to %s: %s\n", PathName().u8string().c_str(),
			new_filename.u8string().c_str(), e.what());
		return false;
	}
	return true;
}


//------------------------------------------------------------------------
//  GLOBAL API
//------------------------------------------------------------------------

//
// find a lump in any loaded wad (later ones tried first),
// returning NULL if not found.
//
const Lump_c *MasterDir::findGlobalLump(const SString &name) const
{
	std::vector<std::shared_ptr<Wad_file>> wads = getAll();
	for (auto it = wads.rbegin(); it != wads.rend(); ++it)
	{
		const Lump_c *L = (*it)->FindLumpInNamespace(name, WadNamespace::Global);
		if (L)
			return L;
	}

	return NULL;  // not found
}

//------------------------------------------------------------------------

void MasterDir::MasterDir_CloseAll()
{
	resource_wads.clear();
	edit_wad.reset();
	game_wad.reset();
}


static bool W_FilenameAbsEqual(const SString &A, const SString &B)
{
	const SString &A_path = GetAbsolutePath(A.get()).u8string();
	const SString &B_path = GetAbsolutePath(B.get()).u8string();
	return A_path.noCaseEqual(B_path);
}


void W_StoreString(char *buf, const SString &str, size_t buflen)
{
	memset(buf, 0, buflen);

	for (size_t i = 0 ; i < buflen && str[i] ; i++)
		buf[i] = str[i];
}

bool MasterDir::MasterDir_HaveFilename(const SString &chk_path) const
{
	std::vector<std::shared_ptr<Wad_file>> wads = getAll();
	for (const std::shared_ptr<Wad_file> &wad : wads)
	{
		SString wad_path = wad->PathName().u8string();

		if (W_FilenameAbsEqual(wad_path, chk_path))
			return true;
	}

	return false;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
