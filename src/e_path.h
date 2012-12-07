//------------------------------------------------------------------------
//  LINEDEF PATHS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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

#ifndef __EUREKA_E_PATH_H__
#define __EUREKA_E_PATH_H__

typedef enum
{
	SLP_Normal = 0,

	SLP_ClearSel = (1 << 0),  // clear the selection first
	SLP_SameTex  = (1 << 1),  // require lines have same textures
	SLP_OneSided = (1 << 2),  // only handle one-sided lines
}
select_lines_in_path_flag_e;


void CMD_SelectLinesInPath(int flags);


typedef enum
{
	SCS_Normal = 0,

	SCS_ClearSel = (1 << 0),  // clear the selection first
	SCS_Floor_H  = (1 << 1),  // sectors have same floor height
	SCS_FloorTex = (1 << 2),  // sectors have same floor texture
	SCS_Ceil_H   = (1 << 3),  // sectors have same ceiling height
	SCS_CeilTex  = (1 << 4),  // sectors have same ceiling texture
}
select_contiguous_sectors_flag_e;


void CMD_SelectContiguousSectors(int flags);


/* find/next/prev stuff */

void GoToSelection();
void GoToObject(const Objid& objid);

void CMD_JumpToObject();
void CMD_NextObject();
void CMD_PrevObject();


#endif  /* __EUREKA_E_PATH_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
