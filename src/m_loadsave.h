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

void LoadLevel(Wad_file *wad, const char *level);

void GetLevelFormat(Wad_file *wad, const char *level);

void RemoveEditWad();

bool MissingIWAD_Dialog();

void OpenFileMap(const char *filename, const char *map_name = NULL);

extern int last_given_file;

// these return false if user cancelled
bool M_SaveMap();
bool M_ExportMap();

/* commands */

void CMD_NewProject();
void CMD_ManageProject();

void CMD_OpenMap();
void CMD_GivenFile();
void CMD_FlipMap();

void CMD_SaveMap();
void CMD_ExportMap();

void CMD_FreshMap();
void CMD_CopyMap();
void CMD_RenameMap();
void CMD_DeleteMap();

// this one in m_testmap.cc
void CMD_TestMap();

#endif  /* __EUREKA_E_LOADSAVE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
