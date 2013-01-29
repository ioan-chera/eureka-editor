//------------------------------------------------------------------------
//  Recent Files / Known Iwads / Backups
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

class Wad_file;

void M_LoadRecent();
void M_SaveRecent();

void M_AddRecent(const char *filename, const char *map_name);
void M_RecentDialog(const char ** file_v, const char ** map_v);

void M_LookForIWADs();
void M_AddKnownIWAD(const char *path);
const char * M_QueryKnownIWAD(const char *game);
const char * M_KnownIWADsForMenu(int *exist_val, const char *exist_name);
const char * M_PickDefaultIWAD();

bool M_ParseEurekaLump(Wad_file *wad);
void M_WriteEurekaLump(Wad_file *wad);

void M_BackupWad(Wad_file *wad);

#endif  /* __EUREKA_M_FILES_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
