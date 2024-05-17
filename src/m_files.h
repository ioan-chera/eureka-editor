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
struct RecentMap
{
	fs::path file;
	SString map;
};


class RecentFiles_c
{
private:
	typedef std::deque<RecentMap> Deque;

	void push_front(const fs::path &file, const SString &map);
	Deque::iterator find(const fs::path &file);

	Deque list;

public:

	int getSize() const
	{
		return (int)list.size();
	}

	void clear()
	{
		list.clear();
	}
	
	void insert(const fs::path &file, const SString &map);
	void Write(std::ostream &stream) const;
	SString Format(int index) const;
	const RecentMap &Lookup(int index) const;
};

//
// Holds recently collected knowledge
//
class RecentKnowledge
{
public:
	void load(const fs::path &home_dir);
	void save(const fs::path &home_dir) const;
	void addRecent(const fs::path &filename, const SString &map_name, const fs::path &home_dir);

	const fs::path *queryIWAD(const SString &game) const
	{
		return get(known_iwads, game);
	}

	void lookForIWADs(const fs::path &install_dir, const fs::path &home_dir);
	void addIWAD(const fs::path &path);
	SString collectGamesForMenu(int *exist_val, const char *exist_name) const;
	const fs::path *getFirstIWAD() const
	{
		return known_iwads.empty() ? nullptr : &known_iwads.begin()->second;
	}

	//
	// Query port path
	//
	const fs::path *queryPortPath(const SString &name) const
	{
		return get(port_paths, name);
	}

	//
	// Changes the port path from name
	//
	void setPortPath(const SString &name, const fs::path &path)
	{
		port_paths[name] = path;
	}

	//
	// Constant getter of recent files
	//
	const RecentFiles_c &getFiles() const
	{
		return files;
	}
	
	bool hasIwadByPath(const fs::path &path) const;

private:
	void parseMiscConfig(std::istream &is);
	void writeKnownIWADs(std::ostream &os) const;
	void parsePortPath(const SString &name, const SString &cpath);
	void writePortPaths(std::ostream &os) const;

	RecentFiles_c files;
	std::map<SString, fs::path> known_iwads;
	std::map<SString, fs::path> port_paths;
};

void M_OpenRecentFromMenu(void *priv_data);

void M_ValidateGivenFiles();
int  M_FindGivenFile(const fs::path &filename);

void M_BackupWad(const Wad_file *wad);

namespace global
{
	extern RecentKnowledge recent;
}

#endif  /* __EUREKA_M_FILES_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
