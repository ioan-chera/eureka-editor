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

class Wad_file;

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

class Lump_c
{
friend class Wad_file;

private:
	Wad_file *parent;

	SString name;

	int l_start;
	int l_length;

	// constructor is private
	Lump_c(Wad_file *_par, const SString &_nam, int _start, int _len);
	Lump_c(Wad_file *_par, const struct raw_wad_entry_s *entry);

	void MakeEntry(struct raw_wad_entry_s *entry);

public:
	const SString &Name() const
	{
		return name;
	}
	int Length() const
	{
		return l_length;
	}

	// do not call this directly, use Wad_file::RenameLump()
	void Rename(const char *new_name);

	// attempt to seek to a position within the lump (default is
	// the beginning).  Returns true if OK, false on error.
	bool Seek(int offset = 0);

	// read some data from the lump, returning true if OK.
	bool Read(void *data, int len);

	// read a line of text, returns true if OK, false on EOF
	bool GetLine(SString &string);

	// write some data to the lump.  Only the lump which had just
	// been created with Wad_file::AddLump() or RecreateLump() can be
	// written to.
	bool Write(const void *data, int len);

	// write some text to the lump
	void Printf(EUR_FORMAT_STRING(const char *msg), ...) EUR_PRINTF(2, 3);

	// mark the lump as finished (after writing data to it).
	bool Finish();

	// predicate for std::sort()
	struct offset_CMP_pred
	{
		inline bool operator() (const Lump_c * A, const Lump_c * B) const
		{
			return A->l_start < B->l_start;
		}
	};

private:
	// deliberately don't implement these
	Lump_c(const Lump_c& other);
	Lump_c& operator= (const Lump_c& other);
};


//------------------------------------------------------------------------

struct LumpRef
{
	Lump_c *lump;
	WadNamespace ns;
};

class Wad_file
{
friend class Instance;
friend class Lump_c;

private:
	SString filename;

	WadOpenMode mode;  // mode value passed to ::Open()

	FILE * fp;

	WadKind kind = WadKind::PWAD;  // 'P' for PWAD, 'I' for IWAD

	// zero means "currently unknown", which only occurs after a
	// call to BeginWrite() and before any call to AddLump() or
	// the finalizing EndWrite().
	int total_size = 0;

	std::vector<LumpRef> directory;

	int dir_start = 0;
	int dir_count = 0;
	u32_t dir_crc = 0;

	// these are lump indices (into 'directory' vector)
	std::vector<int> levels;

	bool begun_write = false;
	int  begun_max_size;

	// when >= 0, the next added lump is placed _before_ this
	int insert_point = -1;

	// constructor is private
	Wad_file(const SString &_name, WadOpenMode _mode, FILE * _fp) :
	   filename(_name), mode(_mode), fp(_fp)
	{
	}

public:
	~Wad_file();

	// open a wad file.
	//
	// mode is similar to the fopen() function:
	//   'r' opens the wad for reading ONLY
	//   'a' opens the wad for appending (read and write)
	//   'w' opens the wad for writing (i.e. create it)
	//
	// Note: if 'a' is used and the file is read-only, it will be
	//       silently opened in 'r' mode instead.
	//
	static Wad_file * Open(const SString &filename, WadOpenMode mode = WadOpenMode::append);

	// check the given wad file exists and is a WAD file
	static bool Validate(const SString &filename);

	const SString &PathName() const
	{
		return filename;
	}
	bool IsReadOnly() const
	{
		return mode == WadOpenMode::read;
	}
	bool IsIWAD() const
	{
		return kind == WadKind::IWAD;
	}

	int TotalSize() const
	{
		return total_size;
	}

	int NumLumps() const
	{
		return static_cast<int>(directory.size());
	}
	Lump_c * GetLump(int index) const;
	Lump_c * FindLump(const SString &name);
	int FindLumpNum(const SString &name);

	Lump_c * FindLumpInNamespace(const SString &name, WadNamespace group);

	int LevelCount() const
	{
		return (int)levels.size();
	}
	int LevelHeader(int lev_num) const;
	int LevelLastLump(int lev_num);

	// these return a level number (0 .. count-1)
	int LevelFind(const SString &name);
	int LevelFindByNumber(int number);
	int LevelFindFirst();

	// returns a lump index, -1 if not found
	int LevelLookupLump(int lev_num, const char *name);

	MapFormat LevelFormat(int lev_num);

	void  SortLevels();

	// check whether another program has modified this WAD, and return
	// either true or false.  We test for change in file size, change
	// in directory size or location, and directory contents (CRC).
	// [ NOT USED YET.... ]
	bool WasExternallyModified();

	// backup the current wad into the given filename.
	// returns true if successful, false on error.
	bool Backup(const char *new_filename);

	// all changes to the wad must occur between calls to BeginWrite()
	// and EndWrite() methods.  the on-disk wad directory may be trashed
	// during this period, it will be re-written by EndWrite().
	void BeginWrite();
	void EndWrite();

	// change name of a lump (can be a level marker too)
	void RenameLump(int index, const char *new_name);

	// remove the given lump(s)
	// this will change index numbers on existing lumps
	// (previous results of FindLumpNum or LevelHeader are invalidated).
	void RemoveLumps(int index, int count = 1);

	// this removes the level marker PLUS all associated level lumps
	// which follow it.
	void RemoveLevel(int lev_num);

	// removes any GL-Nodes lumps that are associated with the given
	// level.
	void RemoveGLNodes(int lev_num);

	// removes any ZNODES lump from a UDMF level.
	void RemoveZNodes(int lev_num);

	// insert a new lump.
	// The second form is for a level marker.
	// The 'max_size' parameter (if >= 0) specifies the most data
	// you will write into the lump -- writing more will corrupt
	// something else in the WAD.
	Lump_c * AddLump (const SString &name, int max_size = -1);
	Lump_c * AddLevel(const SString &name, int max_size = -1, int *lev_num = nullptr);

	// setup lump to write new data to it.
	// the old contents are lost.
	void RecreateLump(Lump_c *lump, int max_size = -1);

	// set the insertion point -- the next lump will be added _before_
	// this index, and it will be incremented so that a sequence of
	// AddLump() calls produces lumps in the same order.
	//
	// passing a negative value or invalid index will reset the
	// insertion point -- future lumps get added at the END.
	// RemoveLumps(), RemoveLevel() and EndWrite() also reset it.
	void InsertPoint(int index = -1);

private:
	static Wad_file * Create(const SString &filename, WadOpenMode mode);

	// read the existing directory.
	bool ReadDirectory();

	void DetectLevels();
	void ProcessNamespaces();

	// look at all the lumps and determine the lowest offset from
	// start of file where we can write new data.  The directory itself
	// is ignored for this.
	int HighWaterMark();

	// look at all lumps in directory and determine the lowest offset
	// where a lump of the given length will fit.  Returns same as
	// HighWaterMark() when no largest gaps exist.  The directory itself
	// is ignored since it will be re-written at EndWrite().
	int FindFreeSpace(int length);

	// find a place (possibly at end of WAD) where we can write some
	// data of max_size (-1 means unlimited), and seek to that spot
	// (possibly writing some padding zeros -- the difference should
	// be no more than a few bytes).  Returns new position.
	int PositionForWrite(int max_size = -1);

	bool FinishLump(int final_size);
	int  WritePadding(int count);

	// write the new directory, updating the dir_xxx variables
	// (including the CRC).
	void WriteDirectory();

	void FixLevelGroup(int index, int num_added, int num_removed);

private:
	// deliberately don't implement these
	Wad_file(const Wad_file& other);
	Wad_file& operator= (const Wad_file& other);

private:
	// predicate for sorting the levels[] vector
	struct level_name_CMP_pred
	{
	private:
		Wad_file *wad;

	public:
		level_name_CMP_pred(Wad_file * _w) : wad(_w)
		{ }

		inline bool operator() (const int A, const int B) const
		{
			const Lump_c *L1 = wad->directory[A].lump;
			const Lump_c *L2 = wad->directory[B].lump;

			return L1->Name() < L2->Name();
		}
	};
};

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

	void removeLevel(int lev_num);
	void removeZNodes(int lev_num);

	const Lump *findLump(const SString &name) const;
	Lump *findLump(const SString &name);
	const Lump *findLumpInNamespace(const SString &name, WadNamespace group) const;
	Lump *findLumpInNamespace(const SString &name, WadNamespace group);

	Lump &appendNewLump();
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
private:
	void detectLevels();
	void sortLevels();
	void processNamespaces();
	void fixLevelGroup(int index, int num_added, int num_removed);

	SString mPath;

	WadKind mKind = WadKind::PWAD;  // 'P' for PWAD, 'I' for IWAD
	std::vector<NamespacedLump> mLumps;

	std::vector<FailedWadReadEntry> mFailedReadEntries;

	// these are lump indices (into 'directory' vector)
	std::vector<int> mLevels;
};

#endif  /* __EUREKA_W_WAD_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
