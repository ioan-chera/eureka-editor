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

void GoToObject(Instance &inst, const Objid& objid);
void GoToSelection(Instance &inst);
void GoToErrors(Instance &inst);


enum propagate_level_e
{
	PGL_Never = 0,	// can never be heared
	PGL_Maybe,		// a possibility of being heared
	PGL_Level_1,	// reduced by a single level
	PGL_Level_2		// no blocking at all

};

#endif  /* __EUREKA_E_PATH_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
