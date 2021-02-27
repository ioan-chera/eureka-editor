//------------------------------------------------------------------------
//  WAD Reading / Writing
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
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

#ifndef __EUREKA_W_WAD_H__
#define __EUREKA_W_WAD_H__

#include "Errors.h"
#include "Lump.h"

#include "m_strings.h"

//
// Wad namespace
//
enum class WadNamespace
{
	Global,
	Flats,
	Sprites,
	TextureLumps
};

enum class WadOpenMode
{
	read,
	write,
	append
};

enum class WadKind
{
	PWAD,
	IWAD
};

const char *WadNamespaceString(WadNamespace ns);

//------------------------------------------------------------------------

// load the lump into memory, returning the size
void W_FreeLumpData(byte ** buf_ptr);

void W_StoreString(char *buf, const SString &str, size_t buflen);

namespace global
{
	extern bool udmf_testing;
}

//=============================================================================
//
// Wad and lump fully loaded in memory
//

//
// Holds info of failed resource to read
//
struct FailedWadReadEntry
{
	int dirIndex;	// index in the directory
	// content of directory
	char name[9];
	int position;
	int length;
};

//
// Lump with namespace
//
struct NamespacedLump : Lump
{
	WadNamespace ns = WadNamespace::Global;
};

bool WadFileValidate(const SString &filename);

//
// Wad of lumps
//
class Wad
{
public:
	size_t totalSize() const;

	bool readFromPath(const SString& path);
	bool writeToPath(const SString& path) const;

	int levelFind(const SString &name) const;
	int levelFindByNumber(int number) const;
	int levelCount() const
	{
		return (int)mLevels.size();
	}
	int levelLastLump(int lev_num) const;
	int levelLookupLump(int lev_num, const char *name) const;
	void removeGLNodes(int lev_num);
	MapFormat levelFormat(int lev_num) const;
	Lump &addLevel(const SString &name, int max_size, int *lev_num);
	void sortLevels();

	void removeLevel(int lev_num);
	void removeZNodes(int lev_num);

	const Lump *findLump(const SString &name) const;
	int findLumpNum(const SString &name) const;
	Lump *findLump(const SString &name);
	const Lump *findLumpInNamespace(const SString &name, WadNamespace group) const;
	Lump *findLumpInNamespace(const SString &name, WadNamespace group);
	void renameLump(int index, const SString &new_name);

	Lump &addNewLump();
	Lump &insertNewLump(int position);
	const Lump &getLump(int n) const
	{
		SYS_ASSERT(0 <= n && n < (int)mLumps.size());
		return mLumps[n];
	}
	Lump &getLump(int n)
	{
		SYS_ASSERT(0 <= n && n < (int)mLumps.size());
		return mLumps[n];
	}
	void removeLumps(int index, int count);
	void destroy();

	//
	// Returns the lump ID of the level header entry
	//
	int levelHeader(int levelNum) const
	{
		SYS_ASSERT(0 <= levelNum && levelNum < (int)mLevels.size());
		return mLevels[levelNum];
	}

	//
	// Return the first level, if available. Otherwise -1.
	//
	int levelFindFirst() const
	{
		return !mLevels.empty() ? 0 : -1;
	}

	//
	// Check if wad is loaded (has path)
	//
	bool isLoaded() const
	{
		return mPath.good();
	}

	//
	// Get the lumps
	//
	const std::vector<NamespacedLump> &getLumps() const
	{
		return mLumps;
	}

	//
	// Get path
	//
	const SString &path() const
	{
		return mPath;
	}

	//
	// Set the insertion point for subsequent wad-add commands
	//
	void setInsertionPoint(int point)
	{
		mInsertionPoint = point;
	}

	//
	// Resets the insertion point. Call this when you consider a writing command finished
	//
	void resetInsertionPoint()
	{
		mInsertionPoint = -1;
	}
private:
	void detectLevels();
	void processNamespaces();
	void fixLevelGroup(int index, int num_added, int num_removed);

	SString mPath;

	WadKind mKind = WadKind::PWAD;  // 'P' for PWAD, 'I' for IWAD
	std::vector<NamespacedLump> mLumps;

	std::vector<FailedWadReadEntry> mFailedReadEntries;

	// these are lump indices (into 'directory' vector)
	std::vector<int> mLevels;

	int mInsertionPoint = -1;
};

#endif  /* __EUREKA_W_WAD_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
