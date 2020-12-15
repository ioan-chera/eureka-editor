//------------------------------------------------------------------------
//  LEVEL CUT 'N' PASTE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2009-2019 Andrew Apted
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

//
// Edit menu command
//
enum class EditCommand
{
	cut,
	copy,
	paste,
	del
};

void Clipboard_Clear();
void Clipboard_ClearLocals();

void Clipboard_NotifyBegin();
void Clipboard_NotifyInsert(obj_type_e type, int objnum);
void Clipboard_NotifyDelete(obj_type_e type, int objnum);
void Clipboard_NotifyChange(obj_type_e type, int objnum, int field);
void Clipboard_NotifyEnd();

void UnusedVertices(selection_c *lines, selection_c *result);
void UnusedSideDefs(selection_c *lines, selection_c *secs, selection_c *result);

void DeleteObjects_WithUnused(selection_c *list,
			bool keep_things = false,
			bool keep_verts  = false,
			bool keep_lines  = false);

void CMD_Delete();
void CMD_CopyAndPaste();

void CMD_Clipboard_Cut();
void CMD_Clipboard_Copy();
void CMD_Clipboard_Paste();


//----------------------------------------------------------------------
//  Texture Clipboard
//----------------------------------------------------------------------

void Texboard_Clear();

int Texboard_GetTexNum();
int Texboard_GetFlatNum();
int Texboard_GetThing();

void Texboard_SetTex(const SString &new_tex);
void Texboard_SetFlat(const SString &new_flat);
void Texboard_SetThing(int new_id);


#endif  /* __EUREKA_E_CUTPASTE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
