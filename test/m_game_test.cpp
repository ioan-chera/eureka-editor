//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2021 Ioan Chera
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

#include "gtest/gtest.h"

#include "Instance.h"
#include "m_game.h"

//=============================================================================
//
// MOCKUPS
//
//=============================================================================
namespace global
{
    SString install_dir;
    SString home_dir;
}

void Clipboard_ClearLocals()
{
}

void Clipboard_NotifyBegin()
{
}

void Clipboard_NotifyChange(ObjType type, int objnum, int field)
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

DocumentModule::DocumentModule(Document &doc) : inst(doc.inst), doc(doc)
{
}

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

rgb_color_t ParseColor(const SString &cstr)
{
    // Use some independent example
    return (rgb_color_t)strtol(cstr.c_str(), nullptr, 16) << 8;
}

void Render3D_NotifyBegin()
{
}

void Render3D_NotifyChange(ObjType type, int objnum, int field)
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


void Instance::Selection_Clear(bool no_save)
{
}

void Instance::Selection_NotifyBegin()
{
}

void Selection_NotifyChange(ObjType type, int objnum, int field)
{
    // field changes never affect the current selection
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

void Instance::Status_Set(const char *fmt, ...) const
{
}

//=============================================================================
//
// TESTS
//
//=============================================================================

TEST(MGame, MClearAllDefinitions)
{
    Instance instance;
}
