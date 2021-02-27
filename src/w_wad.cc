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
#include "w_rawdef.h"
#include "w_wad.h"

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
//  WAD Reading Interface
//------------------------------------------------------------------------

bool WadFileValidate(const SString &filename)
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

//------------------------------------------------------------------------
//  WAD Writing Interface
//------------------------------------------------------------------------


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

//------------------------------------------------------------------------
//  GLOBAL API
//------------------------------------------------------------------------

//
// find a lump in any loaded wad (later ones tried first),
// returning NULL if not found.
//
Lump *Instance::W_FindGlobalLump(const SString &name)
{
	for (int i = (int)masterDirSize() - 1 ; i >= 0 ; i--)
	{
		Lump *L = masterDir(i).findLumpInNamespace(name, WadNamespace::Global);
		if (L)
			return L;
	}

	return NULL;  // not found
}
const Lump *Instance::W_FindGlobalLump(const SString &name) const
{
	for (int i = (int)masterDirSize() - 1 ; i >= 0 ; i--)
	{
		const Lump *L = masterDir(i).findLumpInNamespace(name, WadNamespace::Global);
		if (L)
			return L;
	}

	return NULL;  // not found
}

//
// find a lump that only exists in a certain namespace (sprite,
// or patch) of a loaded wad (later ones tried first).
//
const Lump *Instance::W_FindSpriteLump(const SString &name) const
{
	for (int i = (int)masterDirSize() - 1 ; i >= 0 ; i--)
	{
		const Lump *L = masterDir(i).findLumpInNamespace(name, WadNamespace::Sprites);
		if (L)
			return L;
	}

	return NULL;  // not found
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

//
// Returns the total file size
//
size_t Wad::totalSize() const
{
	size_t s = 12;
	for(const Lump &lump : mLumps)
		s += 16 + lump.getSize();
	return s;
}

//
// Read a lump from path
//
bool Wad::readFromPath(const SString& path)
{
	FILE* f = fopen(path.c_str(), "rb");
	if (!f)
	{
		LogPrintf("Couldn't open '%s'.\n", path.c_str());
		return false;
	}
	
	raw_wad_header_t header = {};
	if (fread(&header, sizeof(header), 1, f) != 1)
	{
		LogPrintf("Error reading WAD header.\n");
		fclose(f);
		return false;
	}

	WadKind newKind = header.ident[0] == 'I' ? WadKind::IWAD : WadKind::PWAD;

	int dirStart = LE_S32(header.dir_start);
	int dirCount = LE_S32(header.num_entries);

	if (dirCount < 0)
	{
		LogPrintf("Bad WAD header, invalid number of entries (%d)\n", dirCount);
		fclose(f);
		return false;
	}

	if (dirStart < 0 || fseek(f, dirStart, SEEK_SET) != 0)
	{
		LogPrintf("Error seeking to WAD directory at %d.\n", dirStart);
		fclose(f);
		return false;
	}

	std::vector<FailedWadReadEntry> failed;
	auto addFailed = [&failed](int index, const char* name, int pos, int len)
	{
		FailedWadReadEntry fail = {};
		fail.dirIndex = index;
		strncpy(fail.name, name, 8);
		fail.name[8] = 0;
		fail.position = pos;
		fail.length = len;
		failed.push_back(fail);

		LogPrintf("Bad lump '%s' at index %d, file position %d and length %d\n", fail.name, index, pos, len);
	};

	std::vector<NamespacedLump> lumps;
	lumps.reserve(dirCount);
	for (int i = 0; i < dirCount; ++i)
	{
		raw_wad_entry_t entry = {};
		if (fread(&entry, sizeof(entry), 1, f) != 1)
		{
			LogPrintf("Error reading entry in WAD directory.\n");
			fclose(f);
			return false;
		}

		NamespacedLump lump;
		long curpos = ftell(f);
		int pos = LE_S32(entry.pos);
		int len = LE_S32(entry.size);
		if (pos < 0 || fseek(f, pos, SEEK_SET) != 0)
		{
			addFailed(i, entry.name, pos, len);
			continue;
		}

		std::vector<byte> content;
		if (len < 0 || !readBuffer(f, len, content))
		{
			addFailed(i, entry.name, pos, len);
			if (fseek(f, curpos, SEEK_SET) != 0)
			{
				LogPrintf("Error seeking back to WAD directory at %ld.\n", curpos);
				fclose(f);
				return false;
			}
			continue;
		}

		if (fseek(f, curpos, SEEK_SET) != 0)
		{
			LogPrintf("Error seeking back to WAD directory at %ld.\n", curpos);
			fclose(f);
			return false;
		}

		lump.setName(SString(entry.name, 8));
		lump.setData(std::move(content));
		lumps.push_back(std::move(lump));
	}
	fclose(f);
	// All good
	mPath = path;
	mKind = newKind;
	mLumps = std::move(lumps);
	mFailedReadEntries = std::move(failed);

	// Post processing
	detectLevels();

	return true;
}

//
// Write to path
//
bool Wad::writeToPath(const SString& path) const
{
	// TODO
	FILE *f = fopen(path.c_str(), "wb");
	if(!f)
	{
		LogPrintf("Couldn't open '%s' for writing.\n", path.c_str());
		return false;
	}

	if(mKind == WadKind::IWAD)
		fputs("IWAD", f);
	else
		fputs("PWAD", f);

	int32_t data = static_cast<int32_t>(mLumps.size());
	if(!fwrite(&data, 4, 1, f))
	{
		fclose(f);
		return false;
	}
	size_t totallumpsize = 0;
	for(const auto &lump : mLumps)
		totallumpsize += lump.getSize();

	data = static_cast<int32_t>(totallumpsize + 12);
	if(!fwrite(&data, 4, 1, f))
	{
		fclose(f);
		return false;
	}
	for(const auto &lump : mLumps)
		if(fwrite(lump.getData(), 1, lump.getSize(), f) < lump.getSize())
		{
			fclose(f);
			return false;
		}
	size_t filepos = 12;
	for(const auto &lump : mLumps)
	{
		data = static_cast<uint32_t>(filepos);
		if(!fwrite(&data, 4, 1, f))
		{
			fclose(f);
			return false;
		}
		data = static_cast<uint32_t>(lump.getSize());
		if(!fwrite(&data, 4, 1, f))
		{
			fclose(f);
			return false;
		}
		filepos += lump.getSize();
		if(fwrite(lump.getName(), 1, 8, f) < 8)
		{
			fclose(f);
			return false;
		}
	}

	fclose(f);
	return true;
}

//
// Returns the index of a level lump the given name. Returns -1 if not found.
//
int Wad::levelFind(const SString &name) const
{
	for(int k = 0; k < static_cast<int>(mLevels.size()); ++k)
	{
		int index = mLevels[k];
		SYS_ASSERT(0 <= index && index < static_cast<int>(mLumps.size()));

		if(!y_stricmp(mLumps[index].getName(), name.c_str()))
			return k;
	}
	return -1;	// not found
}

//
// Returns level by number
//
int Wad::levelFindByNumber(int number) const
{
	// sanity check
	if (number <= 0 || number > 99)
		return -1;

	int index;

	 // try MAP## first
	SString buffer = SString::printf("MAP%02d", number);

	index = levelFind(buffer);
	if (index >= 0)
		return index;

	// otherwise try E#M#
	buffer = SString::printf("E%dM%d", MAX(1, number / 10), number % 10);

	index = levelFind(buffer);
	if (index >= 0)
		return index;

	return -1;  // not found
}

//
// Last level lump
//
int Wad::levelLastLump(int lev_num) const
{
	int start = levelHeader(lev_num);
	int count = 1;

	// UDMF level?
	if(!y_stricmp(mLumps[start + 1].getName(), "TEXTMAP"))
	{
		while(count < MAX_LUMPS_IN_A_LEVEL && start + count < (int)mLumps.size())
		{
			if(!y_stricmp(mLumps[start + count].getName(), "ENDMAP"))
			{
				count++;
				break;
			}
			count++;
		}
		return start + count - 1;
	}

	// standard DOOM or HEXEN format
	while(count < MAX_LUMPS_IN_A_LEVEL && start + count < (int)mLumps.size() &&
		  (IsLevelLump(mLumps[start + count].getName()) ||
		   IsGLNodeLump(mLumps[start + count].getName())))
	{
		count++;
	}
	return start + count - 1;
}

//
// Lookup lump
//
int Wad::levelLookupLump(int lev_num, const char *name) const
{
	int start = levelHeader(lev_num);

	// determine how far past the level marker (MAP01 etc) to search
	int finish = levelLastLump(lev_num);

	for (int k = start + 1 ; k <= finish ; k++)
	{
		SYS_ASSERT(0 <= k && k < (int)mLumps.size());

		if (!y_stricmp(mLumps[k].getName(), name))
			return k;
	}

	return -1;  // not found
}

//
// Remove gl nodes
//
void Wad::removeGLNodes(int lev_num)
{
	SYS_ASSERT(0 <= lev_num && lev_num < levelCount());

	int start  = levelHeader(lev_num);
	int finish = levelLastLump(lev_num);

	start++;

	while (start <= finish &&
		   IsLevelLump(mLumps[start].getName()))
	{
		start++;
	}

	int count = 0;

	while (start + count <= finish &&
		   IsGLNodeLump(mLumps[start+count].getName()))
	{
		count++;
	}

	if (count > 0)
		removeLumps(start, count);
}

//
// Level format
//
MapFormat Wad::levelFormat(int lev_num) const
{
	int start = levelHeader(lev_num);

	if (!y_stricmp(mLumps[start + 1].getName(), "TEXTMAP"))
		return MapFormat::udmf;

	if (start + LL_BEHAVIOR < (int)mLumps.size())
	{
		const SString &name = getLump(start + LL_BEHAVIOR).getName();

		if (name.noCaseEqual("BEHAVIOR"))
			return MapFormat::hexen;
	}

	return MapFormat::doom;
}

//
// Add level
//
Lump &Wad::addLevel(const SString &name, int max_size, int *lev_num)
{
	int actualPoint = mInsertionPoint;
	if(actualPoint < 0 || actualPoint > (int)mLumps.size())
		actualPoint = (int)mLumps.size();

	Lump &lump = addNewLump();
	lump.setName(name);

	if (lev_num)
	{
		*lev_num = (int)mLevels.size();
	}

	mLevels.push_back(actualPoint);

	return lump;
}

//
// Remove level
//
void Wad::removeLevel(int lev_num)
{
	int start = levelHeader(lev_num);
	int finish = levelLastLump(lev_num);

	// NOTE: FixGroup() will remove the entry in levels[]

	removeLumps(start, finish - start + 1);
}

//
// Remove ZNodes
//
void Wad::removeZNodes(int lev_num)
{
	SYS_ASSERT(0 <= lev_num && lev_num < (int)mLevels.size());

	int start  = levelHeader(lev_num);
	int finish = levelLastLump(lev_num);

	for ( ; start <= finish ; start++)
	{
		if (!y_stricmp(mLumps[start].getName(), "ZNODES"))
		{
			removeLumps(start, 1);
			break;
		}
	}
}


//
// Remove the wad completely
//
void Wad::destroy()
{
	*this = Wad();	// assign the wad
}

//
// Gets the last lump with the given name. Returns nullptr if unfound
//
const Lump *Wad::findLump(const SString &name) const
{
	for(auto it = mLumps.rbegin(); it != mLumps.rend(); ++it)
		if(!y_stricmp(it->getName(), name.c_str()))
			return &*it;
	return nullptr;
}
Lump *Wad::findLump(const SString &name)
{
	for(auto it = mLumps.rbegin(); it != mLumps.rend(); ++it)
		if(!y_stricmp(it->getName(), name.c_str()))
			return &*it;
	return nullptr;
}

//
// Find lump number
//
int Wad::findLumpNum(const SString &name) const
{
	for (int k = (int)mLumps.size() - 1 ; k >= 0 ; k--)
		if (!y_stricmp(mLumps[k].getName(), name.c_str()))
			return k;

	return -1;  // not found
}

//
// Find in namespace
//
const Lump *Wad::findLumpInNamespace(const SString &name, WadNamespace group) const
{
	for(const NamespacedLump &lump : mLumps)
	{
		if(lump.ns != group || y_stricmp(lump.getName(), name.c_str()))
			continue;
		return &lump;
	}

	return nullptr; // not found!
}
Lump *Wad::findLumpInNamespace(const SString &name, WadNamespace group)
{
	for(NamespacedLump &lump : mLumps)
	{
		if(lump.ns != group || y_stricmp(lump.getName(), name.c_str()))
			continue;
		return &lump;
	}

	return nullptr; // not found!
}

//
// Rename lump
//
void Wad::renameLump(int index, const SString &new_name)
{
	mLumps[index].setName(new_name);
}

//
// Appends an empty lump for editing
//
Lump &Wad::addNewLump()
{
	if(mInsertionPoint >= (int)mLumps.size())
		mInsertionPoint = -1;
	if(mInsertionPoint >= 0)
	{
		fixLevelGroup(mInsertionPoint, 1, 0);
		mLumps.insert(mLumps.begin() + mInsertionPoint, NamespacedLump());
		++mInsertionPoint;
	}
	else	// add to end
		mLumps.push_back(NamespacedLump());

	processNamespaces();
	return mLumps.back();
}

//
// Insert new lump
//
Lump &Wad::insertNewLump(int position)
{
	mLumps.insert(mLumps.begin() + position, NamespacedLump());
	processNamespaces();
	return mLumps[position];
}

//
// Remove lumps
//
void Wad::removeLumps(int index, int count)
{
	SYS_ASSERT(0 <= index && index < (int)mLumps.size());
	mLumps.erase(mLumps.begin() + index, mLumps.begin() + index + count);

	fixLevelGroup(index, 0, count);

	mInsertionPoint = -1;

	processNamespaces();
}

//
// Detect the levels
//
void Wad::detectLevels()
{
	// Determine what lumps in the wad are level markers, based on
	// the lumps which follow it.  Store the result in the 'levels'
	// vector.  The test here is rather lax, as I'm told certain
	// wads exist with a non-standard ordering of level lumps.
	mLevels.clear();
	for(int k = 0; k + 1 < (int)mLumps.size(); ++k)
	{
		// check for UDMF levels
		if(global::udmf_testing && !y_stricmp(mLumps[k + 1].getName(), "TEXTMAP"))
		{
			mLevels.push_back(k);
			DebugPrintf("Detected level: %s (UDMF)\n", mLumps[k].getName());
			continue;
		}

		int part_mask  = 0;
		int part_count = 0;

		// check whether the next four lumps are level lumps
		for(int i = 1; i <= 4; ++i)
		{
			if(k + i >= (int)mLumps.size())
				break;
			int part = WhatLevelPart(mLumps[k + i].getName());
			if(!part)
				break;
			// do not allow duplicates
			if(part_mask & (1 << part))
				break;
			part_mask |= (1 << part);
			part_count++;
		}
		if(part_count == 4)
		{
			mLevels.push_back(k);
			DebugPrintf("Detected level: %s\n", mLumps[k].getName());
		}
	}

	// sort levels into alphabetical order
	// (mainly for the 'N' next map and 'P' prev map commands)

	sortLevels();
	processNamespaces();
}

//
// Sort levels by lump header name
//
void Wad::sortLevels()
{
	std::sort(mLevels.begin(), mLevels.end(), [this](int lev1, int lev2)
			  {
		SYS_ASSERT(0 <= lev1 && lev1 < (int)mLumps.size());
		SYS_ASSERT(0 <= lev2 && lev2 < (int)mLumps.size());
		return y_stricmp(mLumps[lev1].getName(), mLumps[lev2].getName()) < 0;
	});
}

//
// Process namespaces
//
void Wad::processNamespaces()
{
	WadNamespace active = WadNamespace::Global;

	for(NamespacedLump &lump : mLumps)
	{
		const SString &name = lump.getName();
		lump.ns = WadNamespace::Global;	// default it to global

		// skip the sub-namespace markers
		if(IsDummyMarker(name))
			continue;

		if(name.noCaseEqual("S_START") || name.noCaseEqual("SS_START"))
		{
			if(active != WadNamespace::Global && active != WadNamespace::Sprites)
				LogPrintf("WARNING: missing %s_END marker.\n", WadNamespaceString(active));
			active = WadNamespace::Sprites;
			continue;
		}
		if(name.noCaseEqual("S_END") || name.noCaseEqual("SS_END"))
		{
			if(active != WadNamespace::Sprites)
				LogPrintf("WARNING: stray S_END marker found.\n");
			active = WadNamespace::Global;
			continue;
		}
		if (name.noCaseEqual("F_START") || name.noCaseEqual("FF_START"))
		{
			if (active != WadNamespace::Global && active != WadNamespace::Flats)
				LogPrintf("WARNING: missing %s_END marker.\n", WadNamespaceString(active));

			active = WadNamespace::Flats;
			continue;
		}
		if (name.noCaseEqual("F_END") || name.noCaseEqual("FF_END"))
		{
			if (active != WadNamespace::Flats)
				LogPrintf("WARNING: stray F_END marker found.\n");

			active = WadNamespace::Global;
			continue;
		}
		if (name.noCaseEqual("TX_START"))
		{
			if (active != WadNamespace::Global && active != WadNamespace::TextureLumps)
				LogPrintf("WARNING: missing %s_END marker.\n", WadNamespaceString(active));

			active = WadNamespace::TextureLumps;
			continue;
		}
		if (name.noCaseEqual("TX_END"))
		{
			if (active != WadNamespace::TextureLumps)
				LogPrintf("WARNING: stray TX_END marker found.\n");

			active = WadNamespace::Global;
			continue;
		}

		if (active != WadNamespace::Global)
		{
			if (lump.getSize() == 0)
			{
				LogPrintf("WARNING: skipping empty lump %s in %s_START\n",
						  name.c_str(), WadNamespaceString(active));
				continue;
			}

			lump.ns = active;
		}
	}

	if (active != WadNamespace::Global)
		LogPrintf("WARNING: Missing %s_END marker (at EOF)\n", WadNamespaceString(active));
}

//
// Fix level group
//
void Wad::fixLevelGroup(int index, int num_added, int num_removed)
{
	bool did_remove = false;
	for(int k = 0; k < (int)mLevels.size(); ++k)
	{
		if(mLevels[k] < index)
			continue;
		if(mLevels[k] < index + num_removed)
		{
			mLevels[k] = -1;
			did_remove = true;
			continue;
		}

		mLevels[k] += num_added;
		mLevels[k] -= num_removed;
	}
	if(did_remove)
	{
		std::vector<int>::iterator ENDP;
		ENDP = std::remove(mLevels.begin(), mLevels.end(), -1);
		mLevels.erase(ENDP, mLevels.end());
	}
}

bool Instance::MasterDir_HaveFilename(const SString &chk_path) const
{
	for (unsigned int k = 0 ; k < masterDirSize() ; k++)
	{
		const SString &wad_path = masterDir(k).path();

		if (W_FilenameAbsEqual(wad_path, chk_path))
			return true;
	}

	return false;
}

//
// Return the wads in the masterdir
//
Wad &Instance::masterDir(int n)
{
	if(!n)
		return gameWad;
	--n;
	if(n >= 0 && n < (int)resourceWads.size())
		return resourceWads[n];
	return editWad;
}
const Wad &Instance::masterDir(int n) const
{
	if(!n)
		return gameWad;
	--n;
	if(n >= 0 && n < (int)resourceWads.size())
		return resourceWads[n];
	return editWad;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
