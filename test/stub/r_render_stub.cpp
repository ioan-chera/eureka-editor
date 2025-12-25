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

class Instance;
struct Document;

void Render3D_NotifyBegin()
{
}

void Render3D_NotifyChange(ObjType type, int objnum, Basis::EditField efield)
{
}

void Render3D_NotifyDelete(const Document &doc, ObjType type, int objnum)
{
}

void Render3D_NotifyEnd(Instance &inst)
{
}

void Render3D_NotifyInsert(ObjType type, int objnum)
{
}
