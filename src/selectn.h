//------------------------------------------------------------------------
//  SELECTION LISTS
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

#ifndef __EUREKA_SELECTN_H__
#define __EUREKA_SELECTN_H__


#define IsSelected(list, num)  ((list)->get(num))

#define ForgetSelection(list)  ((list)->clear_all())

#define SelectObject(list, num)  ((list)->set(num))
#define UnSelectObject(list, num)  ((list)->clear(num))
#define ToggleObject(list, num)  ((list)->toggle(num))

void DumpSelection (selection_c * list);

void ConvertSelection(selection_c * src, selection_c * dest);

void Selection_NotifyBegin();
void Selection_NotifyInsert(obj_type_e type, int objnum);
void Selection_NotifyDelete(obj_type_e type, int objnum);
void Selection_NotifyChange(obj_type_e type, int objnum, int field);
void Selection_NotifyEnd();

#endif  /* __EUREKA_SELECTN_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
