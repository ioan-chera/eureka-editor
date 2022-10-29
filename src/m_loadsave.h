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

#include "w_wad.h"
#include <unordered_map>

//
// Background loading data for Main_LoadResources
//
struct LoadingData
{
	void prepareConfigVariables();
	bool parseEurekaLump(const Wad_file *wad, bool keep_cmd_line_args = false);
	void writeEurekaLump(Wad_file *wad) const;

	SString gameName;	// Name of game "doom", "doom2", "heretic", ...
	SString portName;	// Name of source port "vanilla", "boom", ...
	SString iwadName;	// Filename of the iwad
	SString levelName;	// Name of map lump we are editing
	SString udmfNamespace;	// for UDMF, the current namespace
	std::vector<fs::path> resourceList;
	MapFormat levelFormat = {};	// format of current map

	std::unordered_map<SString, SString> parse_vars;
};

void OpenFileMap(const SString &filename, const SString &map_name = "");

#endif  /* __EUREKA_E_LOADSAVE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
