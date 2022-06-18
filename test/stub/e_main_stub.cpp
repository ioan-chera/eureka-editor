//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022 Ioan Chera
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

#include "e_main.h"
#include "Instance.h"
#include "objid.h"

void Instance::MapStuff_NotifyBegin()
{
}

void Instance::MapStuff_NotifyChange(ObjType type, int objnum, int field)
{
}

void Instance::MapStuff_NotifyDelete(ObjType type, int objnum)
{
}

void Instance::MapStuff_NotifyEnd()
{
}

void Instance::MapStuff_NotifyInsert(ObjType type, int objnum)
{
}

void Instance::ObjectBox_NotifyBegin()
{
}

void Instance::ObjectBox_NotifyChange(ObjType type, int objnum, int field)
{
}

void Instance::ObjectBox_NotifyDelete(ObjType type, int objnum)
{
}

void Instance::ObjectBox_NotifyEnd() const
{
}

void Instance::ObjectBox_NotifyInsert(ObjType type, int objnum)
{
}

void Instance::RedrawMap()
{
}

void Instance::Selection_NotifyBegin()
{
}

void Instance::Selection_NotifyDelete(ObjType type, int objnum)
{
}

void Instance::Selection_NotifyEnd()
{
}

void Instance::Selection_NotifyInsert(ObjType type, int objnum)
{
}

void Recently_used::insert(const SString &name)
{
}

void Recently_used::insert_number(int val)
{
}

void Selection_NotifyChange(ObjType type, int objnum, int field)
{
}
