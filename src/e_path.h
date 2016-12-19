//------------------------------------------------------------------------
//  LINEDEF PATHS
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

#ifndef __EUREKA_E_PATH_H__
#define __EUREKA_E_PATH_H__

/* find/next/prev stuff */

void GoToObject(const Objid& objid);
void GoToSelection();
void GoToErrors();


typedef enum
{
	PGL_Never = 0,	// can never be heared
	PGL_Maybe,		// a possibility of being heared
	PGL_Level_1,	// reduced by a single level
	PGL_Level_2		// no blocking at all

} propagate_level_e;

byte * SoundPropagation(int start_sec);


/* commands */

void CMD_LIN_SelectPath(void);

void CMD_SEC_SelectGroup(void);

void CMD_JumpToObject(void);
void CMD_NextObject();
void CMD_PrevObject();

void CMD_PruneUnused(void);


#endif  /* __EUREKA_E_PATH_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
