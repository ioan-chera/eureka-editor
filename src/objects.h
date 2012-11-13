//------------------------------------------------------------------------
//  OBJECT STUFF
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

#ifndef __EUREKA_OBJECTS_H__
#define __EUREKA_OBJECTS_H__


void CMD_MoveObjects(int delta_x, int delta_y);

void CMD_InsertNewObject(keymod_e mod);


void ObjectBox_NotifyBegin();
void ObjectBox_NotifyInsert(obj_type_e type, int objnum);
void ObjectBox_NotifyDelete(obj_type_e type, int objnum);
void ObjectBox_NotifyChange(obj_type_e type, int objnum, int field);
void ObjectBox_NotifyEnd();


obj_no_t GetMaxObjectNum (int);
void  SelectObjectsInBox (selection_c *list, int, int, int, int, int);
void  HighlightObject (int, int, int);

void  DeleteObjects (selection_c * list);


bool  IsLineDefInside (int, int, int, int, int);
int GetOppositeSector (int, bool);
void  GetObjectCoords (int, int, int *, int *);
int FindFreeTag (void);
void GoToObject (const Objid& objid);

void GetDragFocus(int *x, int *y, int map_x, int map_y);


#endif  /* __EUREKA_OBJECTS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
