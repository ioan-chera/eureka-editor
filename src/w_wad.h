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
#include "main.h"

#include <memory>

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

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

//
// Wad writing exception
//
class WadWriteException : public std::runtime_error
{
public:
	WadWriteException(const SString &msg) : std::runtime_error(msg.c_str())
	{
	}
};

const char *WadNamespaceString(WadNamespace ns);

class Lump_c
{
friend class Wad_file;

private:
	SString name;

	std::vector<byte> mData;
	int mPos = 0;	// insertion point for reading or writing

	// constructor is private
	explicit Lump_c(const SString &_nam);

public:
	const SString &Name() const noexcept
	{
		return name;
	}
	int Length() const
	{
		return (int)mData.size();
	}

	// do not call this directly, use Wad_file::RenameLump()
	void Rename(const char *new_name);

	// attempt to seek to a position within the lump (default is
	// the beginning).  Returns true if OK, false on error.
	void Seek(int offset = 0) noexcept;

	// read some data from the lump, returning true if OK.
	bool Read(void *data, int len) noexcept;

	// read a line of text, returns true if OK, false on EOF
	bool GetLine(SString &string) noexcept;

	// write some data to the lump.  Only the lump which had just
	// been created with Wad_file::AddLump() or RecreateLump() can be
	// written to.
	void Write(const void *data, int len);

	// write some text to the lump
	void Printf(EUR_FORMAT_STRING(const char *msg), ...) EUR_PRINTF(2, 3);

	// Memory buffer actions
	size_t writeData(FILE *f, int len);

    //
    // Clear the lump data
    //
    void clearData()
    {
        mData.clear();
        mPos = 0;
    }

	//
	// Gets the data from lump without moving the insertion point.
	//
	const void *getData() const noexcept
	{
		return mData.data();
	}

	int64_t getName8() const noexcept;

private:
	// deliberately don't implement these
	Lump_c(const Lump_c& other);
	Lump_c& operator= (const Lump_c& other);
};


//------------------------------------------------------------------------

struct LumpRef
{
	std::unique_ptr<Lump_c> lump;
	WadNamespace ns;
};


class Wad_file
{
private:
	const SString filename;

	WadOpenMode mode;  // mode value passed to ::Open()

	WadKind kind = WadKind::PWAD;  // 'P' for PWAD, 'I' for IWAD

	std::vector<LumpRef> directory;

	// these are lump indices (into 'directory' vector)
	std::vector<int> levels;

	// when >= 0, the next added lump is placed _before_ this
	int insert_point = -1;

	// constructor is private
	Wad_file(const SString &_name, WadOpenMode _mode) :
	   filename(_name), mode(_mode)
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
	static std::shared_ptr<Wad_file> Open(const fs::path &filename,
										  WadOpenMode mode
										  = WadOpenMode::append);

	// check the given wad file exists and is a WAD file
	static bool Validate(const fs::path &filename);

	fs::path PathName() const noexcept
	{
		return fs::u8path(filename.get());
	}
	bool IsReadOnly() const noexcept
	{
		return mode == WadOpenMode::read;
	}
	bool IsIWAD() const noexcept
	{
		return kind == WadKind::IWAD;
	}

	int TotalSize() const;

	int NumLumps() const noexcept
	{
		return static_cast<int>(directory.size());
	}
	Lump_c * GetLump(int index) const noexcept;
	Lump_c * FindLump(const SString &name) const noexcept;
	int FindLumpNum(const SString &name) const noexcept;

	Lump_c * FindLumpInNamespace(const SString &name, WadNamespace group)
			const noexcept;

	int LevelCount() const noexcept
	{
		return (int)levels.size();
	}
	int LevelHeader(int lev_num) const noexcept;
	int LevelLastLump(int lev_num) const noexcept;

	// these return a level number (0 .. count-1)
	int LevelFind(const SString &name) const noexcept;
	int LevelFindByNumber(int number) const noexcept;
	int LevelFindFirst() const noexcept;

	// returns a lump index, -1 if not found
	int LevelLookupLump(int lev_num, const char *name) const noexcept;

	MapFormat LevelFormat(int lev_num) const noexcept;

	void  SortLevels() noexcept;

	// backup the current wad into the given filename.
	// returns true if successful, false on error.
	bool Backup(const char *new_filename);

	void writeToDisk() noexcept(false);

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
	Lump_c * AddLump (const SString &name);
	Lump_c * AddLevel(const SString &name, int *lev_num = nullptr);

	// set the insertion point -- the next lump will be added _before_
	// this index, and it will be incremented so that a sequence of
	// AddLump() calls produces lumps in the same order.
	//
	// passing a negative value or invalid index will reset the
	// insertion point -- future lumps get added at the END.
	// RemoveLumps(), RemoveLevel() and EndWrite() also reset it.
	void InsertPoint(int index = -1);

	const std::vector<LumpRef> &getDir() const
	{
		return directory;
	}

private:
	static std::shared_ptr<Wad_file> Create(const SString &filename,
											WadOpenMode mode);

	// read the existing directory.
	bool ReadDirectory(FILE *fp, int totalSize);

	void DetectLevels();
	void ProcessNamespaces();

	void FixLevelGroup(int index, int num_added, int num_removed);

	void writeToPath(const fs::path &path) const noexcept(false);

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

		inline bool operator() (const int A, const int B) const noexcept
		{
			const Lump_c *L1 = wad->directory[A].lump.get();
			const Lump_c *L2 = wad->directory[B].lump.get();

			return L1->Name() < L2->Name();
		}
	};
};

// load the lump into memory, returning the size
int  W_LoadLumpData(Lump_c *lump, std::vector<byte> &buffer);

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

#endif  /* __EUREKA_W_WAD_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
