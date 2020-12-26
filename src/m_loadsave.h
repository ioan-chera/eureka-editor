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

void LoadLevel(Wad_file *wad, const SString &level);
void LoadLevelNum(Wad_file *wad, int lev_num);

void GetLevelFormat(Wad_file *wad, const char *level);

void RemoveEditWad();

bool MissingIWAD_Dialog();

void OpenFileMap(const SString &filename, const SString &map_name = "");

extern int last_given_file;

// these return false if user cancelled
bool M_SaveMap();
bool M_ExportMap();

/* commands */

void CMD_NewProject(Instance &inst);
void CMD_ManageProject(Instance &inst);

void CMD_OpenMap(Instance &inst);
void CMD_GivenFile(Instance &inst);
void CMD_FlipMap(Instance &inst);

void CMD_SaveMap(Instance &inst);
void CMD_ExportMap(Instance &inst);

void CMD_FreshMap(Instance &inst);
void CMD_CopyMap(Instance &inst);
void CMD_RenameMap(Instance &inst);
void CMD_DeleteMap(Instance &inst);

// this one in m_testmap.cc
void CMD_TestMap(Instance &inst);

#endif  /* __EUREKA_E_LOADSAVE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
