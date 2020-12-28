//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
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

#ifndef INSTANCE_H_
#define INSTANCE_H_

#include "Document.h"
#include "e_main.h"
#include "main.h"

//
// An instance with a document, holding all other associated data, such as the window reference, the
// wad list.
//
class Instance
{
public:
	// E_BASIS
	fixcoord_t MakeValidCoord(double x) const;

	// E_MAIN
	void Editor_DefaultState();
	void Editor_Init();
	void Editor_WriteUser(std::ostream &os) const;
	void Selection_Add(Objid &obj) const;
	void Selection_Push() const;
	void Selection_Toggle(Objid &obj) const;
	SelectHighlight SelectionOrHighlight();

	// M_EVENTS
	void Editor_ClearNav();
	void Nav_Clear();

	// M_FILES
	bool M_ParseEurekaLump(Wad_file *wad, bool keep_cmd_line_args = false);
	SString M_PickDefaultIWAD() const;
	bool M_TryOpenMostRecent();
	void M_WriteEurekaLump(Wad_file *wad) const;

	// M_GAME
	SString M_GetBaseGame(const SString &game);
	void M_PrepareConfigVariables();

	// M_KEYS
	void Beep(EUR_FORMAT_STRING(const char *fmt), ...) const EUR_PRINTF(2, 3);

	// M_LOADSAVE
	void RemoveEditWad();

	// MAIN
	void Main_LoadResources();

	// UI_INFOBAR
	void Status_Set(EUR_FORMAT_STRING(const char *fmt), ...) const EUR_PRINTF(2, 3);
	void Status_Clear() const;

public:	// will be private when we encapsulate everything
	Document level{*this};	// level data proper

	UI_MainWindow *main_win = nullptr;
	Editor_State_t edit = {};

	//
	// Wad settings
	//

	// the current PWAD, or NULL for none.
	// when present it is also at master_dir.back()
	Wad_file *edit_wad = nullptr;
	Wad_file *game_wad = nullptr;
	MapFormat Level_format = {};	// format of current map
	SString Level_name;	// Name of map lump we are editing
	SString Port_name;	// Name of source port "vanilla", "boom", ...

	//
	// Game-dependent (thus instance dependent) defaults
	//
	int default_thing = 2001;
	SString default_wall_tex = "GRAY1";
	SString default_floor_tex = "FLAT1";
	SString default_ceil_tex = "FLAT1";
};

extern Instance gInstance;	// for now we run with one instance, will have more for the MDI.

#endif
