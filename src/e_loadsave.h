//------------------------------------------------------------------------
//  LEVEL LOAD / SAVE / NEW
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
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

void RemoveEditWad();

void CMD_NewMap();
bool CMD_OpenMap();
void CMD_OpenRecentMap();
void CMD_SaveMap();
void CMD_ExportMap();

class crc32_c;
void BA_LevelChecksum(crc32_c& crc);

#endif  /* __EUREKA_E_LOADSAVE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
