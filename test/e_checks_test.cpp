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

#include "e_checks.h"

#include "e_basis.h"
#include "e_hover.h"
#include "Instance.h"
#include "m_select.h"
#include "ui_window.h"

//==============================================================================
//
// Mock-ups
//
//==============================================================================

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

void ConvertSelection(const Document &doc, const selection_c & src, selection_c & dest)
{
}

void DeleteObjects_WithUnused(EditOperation &op, const Document &doc, const selection_c &list, bool keep_things,
							  bool keep_verts, bool keep_lines)
{
}

int DLG_Confirm(const std::vector<SString> &buttons, EUR_FORMAT_STRING(const char *msg), ...)
{
	return 0;
}

void DLG_Notify(const char *msg, ...)
{
}

DocumentModule::DocumentModule(Document &doc) : inst(doc.inst), doc(doc)
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

const thingtype_t &M_GetThingType(const ConfigData &config, int type)
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

void Instance::Selection_Clear(bool no_save)
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

SelectHighlight Editor_State_t::SelectionOrHighlight()
{
	return SelectHighlight::ok;
}

void Instance::Status_Set(const char *fmt, ...) const
{
}

bool ImageSet::W_FlatIsKnown(const ConfigData &config, const SString &name) const
{
	return false;
}

Img_c * ImageSet::getTexture(const ConfigData &config, const SString &name, bool try_uppercase) const
{
	return nullptr;
}

bool ImageSet::W_TextureCausesMedusa(const SString &name) const
{
	return false;
}

bool ImageSet::W_TextureIsKnown(const ConfigData &config, const SString &name) const
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

void LogViewer_Open()
{
}

void ObjectsModule::del(EditOperation &op, const selection_c &list) const
{
}

bool ObjectsModule::lineTouchesBox(int ld, double x0, double y0, double x1, double y1) const
{
	return false;
}

void Recently_used::insert(const SString &name)
{
}

void Recently_used::insert_number(int val)
{
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

void Selection_NotifyChange(ObjType type, int objnum, int field)
{
}

int UI_Escapable_Window::handle(int event)
{
	return 0;
}

void UnusedVertices(const Document &doc, const selection_c &lines, selection_c &result)
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
	inst.conf.features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, false), 666);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.conf.features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 666);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.conf.features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 666);
	ASSERT_EQ(findFreeTag(inst, true), 666);
	// Add one more and re-test
	lines[666].tag = 666;
	inst.conf.features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, false), 667);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.conf.features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 667);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.conf.features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 667);
	ASSERT_EQ(findFreeTag(inst, true), 667);
	sectors[667].tag = 667;
	inst.conf.features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, false), 668);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.conf.features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 668);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.conf.features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 668);
	ASSERT_EQ(findFreeTag(inst, true), 668);
}

//
// Test tagsApplyNewValue
//
TEST(EChecks, TagsApplyNewValue)
{
	Instance inst;

	std::vector<LineDef> lines;
	std::vector<Sector> sectors;

	lines.resize(7);
	sectors.resize(5);

	for(LineDef &line : lines)
		inst.level.linedefs.push_back(&line);
	for(Sector &sector : sectors)
		inst.level.sectors.push_back(&sector);

	// Prepare the selection lists
	auto linesel = std::make_unique<selection_c>(ObjType::linedefs);
	auto secsel = std::make_unique<selection_c>(ObjType::sectors);

	// Start with linedefs
	inst.edit.mode = ObjType::linedefs;
	inst.edit.Selected = linesel.get();

	// Nothing selected: check that nothing happens
	inst.level.checks.tagsApplyNewValue(1);
	for(const LineDef &line : lines)
		ASSERT_EQ(line.tag, 0);
	for(const Sector &sector : sectors)
		ASSERT_EQ(sector.tag, 0);
	ASSERT_EQ(inst.level.checks.mLastTag, 0);	// didn't change

	// Select a couple of lines
	inst.edit.Selected->set(1);
	inst.edit.Selected->set(2);
	inst.level.checks.tagsApplyNewValue(1);
	for(const LineDef &line : lines)
		if(&line == &lines[1] || &line == &lines[2])
			ASSERT_EQ(line.tag, 1);
		else
			ASSERT_EQ(line.tag, 0);
	for(const Sector &sector : sectors)
		ASSERT_EQ(sector.tag, 0);
	ASSERT_EQ(inst.level.checks.mLastTag, 1);	// changed

	// Now select a couple of sectors
	inst.edit.mode = ObjType::sectors;
	inst.edit.Selected = secsel.get();
	inst.edit.Selected->set(2);
	inst.edit.Selected->set(4);
	inst.level.checks.tagsApplyNewValue(2);
	for(const LineDef &line : lines)
		if(&line == &lines[1] || &line == &lines[2])
			ASSERT_EQ(line.tag, 1);
		else
			ASSERT_EQ(line.tag, 0);
	for(const Sector &sector : sectors)
		if(&sector == &sectors[2] || &sector == &sectors[4])
			ASSERT_EQ(sector.tag, 2);
		else
			ASSERT_EQ(sector.tag, 0);
	ASSERT_EQ(inst.level.checks.mLastTag, 2);	// changed

	inst.edit.Selected->clear(4);
	inst.level.checks.tagsApplyNewValue(1);
	for(const LineDef &line : lines)
		if(&line == &lines[1] || &line == &lines[2])
			ASSERT_EQ(line.tag, 1);
		else
			ASSERT_EQ(line.tag, 0);
	for(const Sector &sector : sectors)
		if(&sector == &sectors[2])
			ASSERT_EQ(sector.tag, 1);
		else if(&sector == &sectors[4])
			ASSERT_EQ(sector.tag, 2);
		else
			ASSERT_EQ(sector.tag, 0);

	ASSERT_EQ(inst.level.checks.mLastTag, 1);	// changed again
}
