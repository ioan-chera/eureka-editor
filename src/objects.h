//------------------------------------------------------------------------
//  OBJECT STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2015 Andrew Apted
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

#ifndef __EUREKA_OBJECTS_H__
#define __EUREKA_OBJECTS_H__


int GetMaxObjectNum (int objtype);
void  SelectObjectsInBox (selection_c *list, int, int, int, int, int);
void  HighlightObject (int, int, int);

void  DeleteObjects (selection_c * list);


bool LineTouchesBox (int, int, int, int, int);

void GetDragFocus(int *x, int *y, int map_x, int map_y);

bool Texture_MatchPattern(const char *tex, const char *pattern);


/* commands */

void CMD_Insert(void);

void CMD_MoveObjects(int delta_x, int delta_y, int delta_z = 0);

void CMD_CopyProperties(void);


#endif  /* __EUREKA_OBJECTS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
