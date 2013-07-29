//------------------------------------------------------------------------
//  LEVEL LOADING ETC
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

#ifndef __EUREKA_LEVELS_H__
#define __EUREKA_LEVELS_H__

#include <string>

#include "e_things.h"


extern int Map_bound_x1;   /* minimum X value of map */
extern int Map_bound_y1;   /* minimum Y value of map */
extern int Map_bound_x2;   /* maximum X value of map */
extern int Map_bound_y2;   /* maximum Y value of map */

extern int MadeChanges;

void MarkChanges();

void CalculateLevelBounds();


void MapStuff_NotifyBegin();
void MapStuff_NotifyInsert(obj_type_e type, int objnum);
void MapStuff_NotifyDelete(obj_type_e type, int objnum);
void MapStuff_NotifyChange(obj_type_e type, int objnum, int field);
void MapStuff_NotifyEnd();


#endif  /* __EUREKA_LEVELS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
