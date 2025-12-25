//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022-2025 Ioan Chera
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

#include "objid.h"

class EditOperation;
class selection_c;
struct Document;

void Clipboard_ClearLocals()
{
}

void Clipboard_NotifyBegin()
{
}

void Clipboard_NotifyDelete(ObjType type, int objnum)
{
}

void Clipboard_NotifyEnd()
{
}

void Clipboard_NotifyInsert(const Document &doc, ObjType type, int objnum)
{
}

void DeleteObjects_WithUnused(EditOperation &op, const Document &doc, const selection_c &list, bool keep_things,
                       bool keep_verts, bool keep_lines)
{
}
