//------------------------------------------------------------------------
//  LEVEL CUT 'N' PASTE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2009-2012 Andrew Apted
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

#ifndef __EUREKA_E_CUTPASTE_H__
#define __EUREKA_E_CUTPASTE_H__

void Clipboard_Clear();
void Clipboard_ClearLocals();

bool Clipboard_HasStuff();

void Clipboard_NotifyBegin();
void Clipboard_NotifyInsert(obj_type_e type, int objnum);
void Clipboard_NotifyDelete(obj_type_e type, int objnum);
void Clipboard_NotifyChange(obj_type_e type, int objnum, int field);
void Clipboard_NotifyEnd();

void UnusedVertices(selection_c *lines, selection_c *result);
void UnusedSideDefs(selection_c *lines, selection_c *result);

void CMD_Delete(void);

bool CMD_Copy();
bool CMD_Paste();

#endif  /* __EUREKA_E_CUTPASTE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
