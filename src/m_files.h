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

class Wad_file;

void M_LoadRecent();
void M_SaveRecent();

void M_AddRecent(const char *filename, const char *map_name);
void M_OpenRecentFromMenu(void *priv_data);
bool M_TryOpenMostRecent();

// these three only for menu code
int    M_RecentCount();
void   M_RecentShortName(int index, char *name_buf, int name_size);
void * M_RecentData(int index);


void M_LookForIWADs();
void M_AddKnownIWAD(const char *path);
std::string M_QueryKnownIWAD(const char *game);
const char * M_CollectGamesForMenu(int *exist_val, const char *exist_name);
std::string M_PickDefaultIWAD();

void M_ValidateGivenFiles();
int  M_FindGivenFile(const char *filename);

bool M_ParseEurekaLump(Wad_file *wad, bool keep_cmd_line_args = false);
void M_WriteEurekaLump(Wad_file *wad);

void M_BackupWad(Wad_file *wad);


typedef struct port_path_info_t
{
	char exe_filename[FL_PATH_MAX];

} port_path_info_t;

port_path_info_t * M_QueryPortPath(const char *name, bool create_it = false);

bool M_IsPortPathValid(const port_path_info_t *info);


#endif  /* __EUREKA_M_FILES_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
