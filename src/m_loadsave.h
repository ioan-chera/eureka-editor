//------------------------------------------------------------------------
//  LEVEL LOAD / SAVE / NEW
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

#ifndef __EUREKA_E_LOADSAVE_H__
#define __EUREKA_E_LOADSAVE_H__

#include "Document.h"
#include "m_game.h"
#include "w_wad.h"
#include "WadData.h"
#include <unordered_map>

class RecentKnowledge;

//
// Background loading data for Main_LoadResources
//
struct LoadingData
{
	std::unordered_map<SString, SString> prepareConfigVariables() const;
	bool parseEurekaLump(const fs::path &home_dir, const fs::path &install_dir, const RecentKnowledge &recent, const Wad_file *wad, bool keep_cmd_line_args = false);
	void writeEurekaLump(Wad_file &wad) const;

	SString gameName;	// Name of game "doom", "doom2", "heretic", ...
	SString portName;	// Name of source port "vanilla", "boom", ...
	fs::path iwadName;	// Filename of the iwad
	SString levelName;	// Name of map lump we are editing
	SString udmfNamespace;	// for UDMF, the current namespace
	std::vector<fs::path> resourceList;
	MapFormat levelFormat = {};	// format of current map
	SString testingCommandLine;	// command-line for testing map (stored in Eureka lump due to possible port and mod-specific features)
};


struct NewResources
{
	ConfigData config;
	LoadingData loading;
	WadData waddata;
};

struct BadCount
{
	bool exists() const
	{
		return linedef_count || sector_refs || sidedef_refs;
	}

	int linedef_count;
	int sector_refs;
	int sidedef_refs;
};


struct NewDocument
{
	Document doc;
	LoadingData loading;
	BadCount bad;
};

void OpenFileMap(const fs::path &filename, const SString &map_name = "") noexcept(false);

const Lump_c *Load_LookupAndSeek(int loading_level, const Wad_file *load_wad, const char *name);

#endif  /* __EUREKA_E_LOADSAVE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
