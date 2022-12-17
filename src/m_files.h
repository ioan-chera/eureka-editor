//------------------------------------------------------------------------
//  RECENT FILES / KNOWN IWADS / BACKUPS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2013 Andrew Apted
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

#ifndef __EUREKA_M_FILES_H__
#define __EUREKA_M_FILES_H__

#include "lib_util.h"
#include "m_strings.h"
#include "sys_type.h"

#include <deque>
#include <ostream>

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

class Wad_file;



//------------------------------------------------------------------------
//  RECENT FILE HANDLING
//------------------------------------------------------------------------

#define MAX_RECENT  24


// this is for the "File/Recent" menu callbacks
class recent_file_data_c
{
public:
	fs::path file;
	SString map;

public:
	recent_file_data_c(const fs::path &_file, const SString &_map) :
		file(_file), map(_map)
	{ }

	recent_file_data_c(const recent_file_data_c &other) = default;

	recent_file_data_c() = default;
};


class RecentFiles_c
{
private:
	typedef std::deque<recent_file_data_c> Deque;

	void push_front(const fs::path &file, const SString &map);
	Deque::iterator find(const fs::path &file);

	Deque list;

public:

	int getSize() const
	{
		return (int)list.size();
	}

	recent_file_data_c *getData(int index) const;
	void clear()
	{
		list.clear();
	}
	
	void insert(const fs::path &file, const SString &map);
	void Write(std::ostream &stream) const;
	SString Format(int index) const;
	void Lookup(int index, fs::path *file_v, SString *map_v) const;
};

//
// Holds recently collected knowledge
//
struct RecentKnowledge
{
	void load(const fs::path &home_dir);
	void save(const fs::path &home_dir) const;
	void addRecent(const fs::path &filename, const SString &map_name, const fs::path &home_dir);

	const fs::path *queryIWAD(const SString &game) const
	{
		return get(known_iwads, game);
	}

	void addIWAD(const fs::path &path);

	fs::path *queryPortPath(const SString &name, bool create_it);

	RecentFiles_c files;
	std::map<SString, fs::path> known_iwads;
	std::map<SString, fs::path> port_paths;

private:
	void parseMiscConfig(std::istream &is);
	void parsePortPath(const SString &name, const SString &cpath);
};

void M_OpenRecentFromMenu(void *priv_data);

void M_LookForIWADs();
SString M_CollectGamesForMenu(int *exist_val, const char *exist_name, const std::map<SString, fs::path> &known_iwads);

void M_ValidateGivenFiles();
int  M_FindGivenFile(const fs::path &filename);

void M_BackupWad(Wad_file *wad);

namespace global
{
	extern RecentKnowledge recent;
}

bool M_IsPortPathValid(const fs::path *info);

bool readBuffer(FILE* f, size_t size, std::vector<byte>& target);

#endif  /* __EUREKA_M_FILES_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
