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

#include "e_checks.h"

#include "e_basis.h"
#include "e_hover.h"
#include "Instance.h"
#include "m_select.h"
#include "ui_window.h"

#ifdef None
#undef None
#endif
#include "gtest/gtest.h"

//==============================================================================
//
// Mock-ups
//
//==============================================================================

int BA_InternaliseString(const SString &str)
{
	return 0;
}

void Basis::abort(bool keepChanges)
{
}

int Basis::addNew(ObjType type)
{
	return 0;
}

void Basis::begin()
{
}

bool Basis::changeLinedef(int line, byte field, int value)
{
	return false;
}

bool Basis::changeSector(int sec, byte field, int value)
{
	return false;
}

bool Basis::changeSidedef(int side, byte field, int value)
{
	return false;
}

bool Basis::changeThing(int thing, byte field, int value)
{
	return false;
}

void Basis::EditOperation::destroy()
{
}

void Basis::end()
{
}

void Basis::setMessage(const char *format, ...)
{
}

void Basis::setMessageForSelection(const char *verb, const selection_c &list, const char *suffix)
{
}

void ConvertSelection(const Document &doc, const selection_c * src, selection_c * dest)
{
}

void DeleteObjects_WithUnused(const Document &doc, selection_c *list, bool keep_things,
							  bool keep_verts, bool keep_lines)
{
}

void DLG_Notify(const char *msg, ...)
{
}

DocumentModule::DocumentModule(Document &doc) : inst(doc.inst), doc(doc)
{
}

extrafloor_c::~extrafloor_c()
{
}

Grid_State_c::Grid_State_c(Instance &inst) : inst(inst)
{
}

Grid_State_c::~Grid_State_c()
{
}

void Hover::fastOpposite_begin()
{
}

void Hover::fastOpposite_finish()
{
}

Objid Hover::getNearbyObject(ObjType type, double x, double y) const
{
	return Objid();
}

int Hover::getOppositeSector(int ld, Side ld_side) const
{
	return 0;
}

bool Img_c::has_transparent() const
{
	return false;
}

void Instance::Beep(const char *fmt, ...)
{
}

void Instance::Editor_ChangeMode(char mode_char)
{
}

const linetype_t &Instance::M_GetLineType(int type) const
{
	static linetype_t linetype;
	return linetype;
}

const sectortype_t &Instance::M_GetSectorType(int type) const
{
	static sectortype_t sectortype;
	return sectortype;
}

const thingtype_t &Instance::M_GetThingType(int type) const
{
	static thingtype_t thingtype;
	return thingtype;
}

void Instance::GoToErrors()
{
}

bool Instance::is_sky(const SString &flat) const
{
	return false;
}

void Instance::RedrawMap()
{
}

void Instance::Selection_Clear(bool no_save)
{
}

SelectHighlight Instance::SelectionOrHighlight()
{
	return SelectHighlight::ok;
}

bool Instance::W_FlatIsKnown(const SString &name) const
{
	return false;
}

Img_c * Instance::W_GetTexture(const SString &name, bool try_uppercase) const
{
	return nullptr;
}

bool Instance::W_TextureCausesMedusa(const SString &name) const
{
	return false;
}

bool Instance::W_TextureIsKnown(const SString &name) const
{
	return false;
}

bool is_null_tex(const SString &tex)
{
	return false;
}

bool is_special_tex(const SString &tex)
{
	return false;
}

double LineDef::CalcLength(const Document &doc) const
{
	return 0;
}

SideDef * LineDef::Left(const Document &doc) const
{
	return nullptr;
}

SideDef * LineDef::Right(const Document &doc) const
{
	return nullptr;
}

Vertex * LineDef::End(const Document &doc) const
{
	return nullptr;
}

Vertex * LineDef::Start(const Document &doc) const
{
	return nullptr;
}

bool LineDef::TouchesSector(int sec_num, const Document &doc) const
{
	return false;
}

int LineDef::WhatSector(Side side, const Document &doc) const
{
	return 0;
}

void LogViewer_Open()
{
}

void ObjectsModule::del(selection_c *list) const
{
}

bool ObjectsModule::lineTouchesBox(int ld, double x0, double y0, double x1, double y1) const
{
	return false;
}

Recently_used::Recently_used(Instance &inst) : inst(inst)
{
}

Recently_used::~Recently_used()
{
}

Render_View_t::Render_View_t(Instance &inst) : inst(inst)
{
}

Render_View_t::~Render_View_t()
{
}

SString Sector::CeilTex() const
{
	return SString();
}

SString Sector::FloorTex() const
{
	return SString();
}

sector_3dfloors_c::sector_3dfloors_c()
{
}

sector_3dfloors_c::~sector_3dfloors_c()
{
}

sector_subdivision_c::~sector_subdivision_c()
{
}

bool sel_iter_c::done() const
{
	return false;
}

void sel_iter_c::next()
{
}

int sel_iter_c::operator* () const
{
	return 0;
}

sel_iter_c::sel_iter_c(const selection_c *_sel)
{
}

sel_iter_c::sel_iter_c(const selection_c& _sel)
{
}

void selection_c::change_type(ObjType new_type)
{
}

int selection_c::count_obj() const
{
	return 0;
}

bool selection_c::empty() const
{
	return false;
}

void selection_c::frob_range(int n1, int n2, BitOp op)
{
}

bool selection_c::get(int n) const
{
	return false;
}

selection_c::selection_c(ObjType type, bool extended)
{
}

selection_c::~selection_c()
{
}

void selection_c::set(int n)
{
}

SString SideDef::LowerTex() const
{
	return SString();
}

SString SideDef::MidTex() const
{
	return SString();
}

Sector * SideDef::SecRef(const Document &doc) const
{
	return nullptr;
}

SString SideDef::UpperTex() const
{
	return SString();
}

slope_plane_c::slope_plane_c()
{
}

slope_plane_c::~slope_plane_c()
{
}

int UI_Escapable_Window::handle(int event)
{
	return 0;
}

UI_Escapable_Window::UI_Escapable_Window(int W, int H, const char *L) :
	Fl_Double_Window(W, H, L)
{
}

UI_Escapable_Window::~UI_Escapable_Window()
{
}

void UnusedVertices(const Document &doc, selection_c *lines, selection_c *result)
{
}

//==============================================================================
//
// Tests
//
//==============================================================================

//
// Tests the findFreeTag function
//
TEST(EChecks, FindFreeTag)
{
	Instance inst;

	// Check with empty level
	ASSERT_EQ(findFreeTag(inst, false), 0);
	ASSERT_EQ(findFreeTag(inst, true), 0);

	// Check with level having nothing tagged
	std::vector<LineDef> lines;
	auto assignLines = [&inst, &lines]()
	{
		inst.level.linedefs.clear();
		for(LineDef &line : lines)
			inst.level.linedefs.push_back(&line);
	};
	std::vector<Sector> sectors;
	auto assignSectors = [&inst, &sectors]()
	{
		inst.level.sectors.clear();
		for(Sector &sector : sectors)
			inst.level.sectors.push_back(&sector);
	};

	// Check a level just with lines
	lines.push_back(LineDef());
	lines.push_back(LineDef());
	assignLines();
	ASSERT_EQ(findFreeTag(inst, false), 1);
	ASSERT_EQ(findFreeTag(inst, true), 1);

	// Also add the sectors
	sectors.push_back(Sector());
	sectors.push_back(Sector());
	assignSectors();
	ASSERT_EQ(findFreeTag(inst, false), 1);
	ASSERT_EQ(findFreeTag(inst, true), 1);

	// Remove all lines
	lines.clear();
	assignLines();
	ASSERT_EQ(findFreeTag(inst, false), 1);
	ASSERT_EQ(findFreeTag(inst, true), 1);

	// Remove all
	sectors.clear();
	assignSectors();
	ASSERT_EQ(findFreeTag(inst, false), 0);
	ASSERT_EQ(findFreeTag(inst, true), 0);

	// Add them back and tag one line 1
	lines.push_back(LineDef());
	lines.push_back(LineDef());
	lines.push_back(LineDef());
	sectors.push_back(Sector());
	sectors.push_back(Sector());
	sectors.push_back(Sector());
	assignLines();
	assignSectors();
	lines[1].tag = 1;
	ASSERT_EQ(findFreeTag(inst, false), 2);
	ASSERT_EQ(findFreeTag(inst, true), 2);

	// Also tag one sector 1
	sectors[2].tag = 1;
	ASSERT_EQ(findFreeTag(inst, false), 2);
	ASSERT_EQ(findFreeTag(inst, true), 2);

	// Tag all of them 1: result should be 0 by now
	lines[0].tag = lines[2].tag = sectors[0].tag = sectors[1].tag = 1;
	ASSERT_EQ(findFreeTag(inst, false), 0);
	ASSERT_EQ(findFreeTag(inst, true), 0);

	// Restore their tags but tag one by a bigger amount
	lines[0].tag = lines[2].tag = sectors[0].tag = sectors[1].tag = 0;
	lines[2].tag = 4;
	ASSERT_EQ(findFreeTag(inst, false), 2);
	ASSERT_EQ(findFreeTag(inst, true), 2);

	// Tag one sector by the remaining gap
	sectors[1].tag = 2;
	ASSERT_EQ(findFreeTag(inst, false), 3);
	ASSERT_EQ(findFreeTag(inst, true), 3);

	// Finally no more space
	lines[0].tag = 3;
	ASSERT_EQ(findFreeTag(inst, false), 5);
	ASSERT_EQ(findFreeTag(inst, true), 5);

	// Test some other combos
	sectors.clear();
	lines.resize(5);
	assignSectors();
	assignLines();
	lines[0].tag = 0;
	lines[1].tag = 2;
	lines[2].tag = 3;
	lines[3].tag = 4;
	lines[4].tag = 5;
	ASSERT_EQ(findFreeTag(inst, false), 1);
	ASSERT_EQ(findFreeTag(inst, true), 1);

	// Test the beastmark
	lines.resize(669);
	sectors.resize(669);
	assignLines();
	assignSectors();
	for(int i = 0; i < 666; ++i)
	{
		lines[i].tag = i;
		sectors[i].tag = i;
	}
	inst.Features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, false), 666);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.Features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 666);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.Features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 666);
	ASSERT_EQ(findFreeTag(inst, true), 666);
	// Add one more and re-test
	lines[666].tag = 666;
	inst.Features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, false), 667);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.Features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 667);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.Features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 667);
	ASSERT_EQ(findFreeTag(inst, true), 667);
	sectors[667].tag = 667;
	inst.Features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, false), 668);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.Features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 668);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.Features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 668);
	ASSERT_EQ(findFreeTag(inst, true), 668);
}
