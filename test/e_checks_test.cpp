//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2021-2026 Ioan Chera
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
#include "LineDef.h"
#include "m_select.h"
#include "Sector.h"
#include "ui_window.h"
#include "w_rawdef.h"

#include "FL/Fl.H"

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
	inst.loaded.levelFormat = MapFormat::doom;

	// Check with empty level
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 0);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 0);

	// Check with level having nothing tagged
	std::vector<LineDef> lines;
	auto assignLines = [&inst, &lines]()
	{
		inst.level.linedefs.clear();
		for(LineDef &line : lines)
		{
			auto addedLine = std::make_shared<LineDef>();
			*addedLine = line;
			inst.level.linedefs.push_back(std::move(addedLine));
		}
	};
	std::vector<Sector> sectors;
	auto assignSectors = [&inst, &sectors]()
	{
		inst.level.sectors.clear();
		for(Sector &sector : sectors)
		{
			auto newSector = std::make_shared<Sector>();
			*newSector = sector;
			inst.level.sectors.push_back(std::move(newSector));
		}
	};

	// Check a level just with lines
	lines.push_back(LineDef());
	lines.push_back(LineDef());
	assignLines();
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 1);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 1);

	// Also add the sectors
	sectors.push_back(Sector());
	sectors.push_back(Sector());
	assignSectors();
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 1);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 1);

	// Remove all lines
	lines.clear();
	assignLines();
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 1);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 1);

	// Remove all
	sectors.clear();
	assignSectors();
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 0);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 0);

	// Add them back and tag one line 1
	lines.push_back(LineDef());
	lines.push_back(LineDef());
	lines.push_back(LineDef());
	sectors.push_back(Sector());
	sectors.push_back(Sector());
	sectors.push_back(Sector());
	assignLines();
	assignSectors();
	inst.level.linedefs[1]->arg1 = 1;
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 2);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 2);

	// Also tag one sector 1
	inst.level.sectors[2]->tag = 1;
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 2);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 2);

	// Tag all of them 1: result should be 0 by now
	inst.level.linedefs[0]->arg1 = inst.level.linedefs[2]->arg1 = 1;
	inst.level.sectors[0]->tag = inst.level.sectors[1]->tag = 1;
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 0);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 0);

	// Restore their tags but tag one by a bigger amount
	inst.level.linedefs[0]->arg1 = inst.level.linedefs[2]->arg1 = 0;
	inst.level.linedefs[2]->arg1 = 4;
	inst.level.sectors[0]->tag = inst.level.sectors[1]->tag = 0;
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 2);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 2);

	// Tag one sector by the remaining gap
	inst.level.sectors[1]->tag = 2;
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 3);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 3);

	// Finally no more space
	inst.level.linedefs[0]->arg1 = 3;
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 5);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 5);

	// Test some other combos
	sectors.clear();
	lines.resize(5);
	assignSectors();
	assignLines();
	lines[0].arg1 = 0;
	lines[1].arg1 = 2;
	lines[2].arg1 = 3;
	lines[3].arg1 = 4;
	lines[4].arg1 = 5;
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 1);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 1);

	// Test the beastmark
	lines.resize(669);
	sectors.resize(669);
	assignLines();
	assignSectors();
	for(int i = 0; i < 666; ++i)
	{
		lines[i].arg1 = i;
		inst.level.sectors[i]->tag = i;
	}
	inst.conf.features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 666);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 668);
	inst.conf.features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 666);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 668);
	inst.conf.features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 666);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 666);
	// Add one more and re-test
	inst.level.linedefs[666]->arg1 = 666;
	inst.conf.features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 667);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 668);
	inst.conf.features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 667);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 668);
	inst.conf.features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 667);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 667);
	inst.level.sectors[667]->tag = 667;
	inst.conf.features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 668);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 668);
	inst.conf.features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 668);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 668);
	inst.conf.features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, ObjType::linedefs), 668);
	ASSERT_EQ(findFreeTag(inst, ObjType::sectors), 668);
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
	{
		auto newLine = std::make_shared<LineDef>();
		*newLine = line;
		inst.level.linedefs.push_back(std::move(newLine));
	}
	for(Sector &sector : sectors)
	{
		auto newSector = std::make_shared<Sector>();
		*newSector = sector;
		inst.level.sectors.push_back(std::move(newSector));
	}

	// Start with linedefs
	inst.edit.mode = ObjType::linedefs;
	inst.edit.Selected.emplace(ObjType::linedefs);

	// Nothing selected: check that nothing happens
	inst.level.checks.tagsApplyNewValue(1);
	for(const auto &line : inst.level.linedefs)
		ASSERT_EQ(line->arg1, 0);
	for(const auto &sector : inst.level.sectors)
		ASSERT_EQ(sector->tag, 0);
	ASSERT_EQ(inst.tagInMemory, 0);	// didn't change

	// Select a couple of lines
	inst.edit.Selected->set(1);
	inst.edit.Selected->set(2);
	inst.level.checks.tagsApplyNewValue(1);
	for(const auto &line : inst.level.linedefs)
		if(line == inst.level.linedefs[1] || line == inst.level.linedefs[2])
			ASSERT_EQ(line->arg1, 1);
		else
			ASSERT_EQ(line->arg1, 0);
	for(const auto &sector : inst.level.sectors)
		ASSERT_EQ(sector->tag, 0);
	ASSERT_EQ(inst.tagInMemory, 1);	// changed

	// Now select a couple of sectors
	inst.edit.mode = ObjType::sectors;
	inst.edit.Selected.emplace(ObjType::sectors);
	inst.edit.Selected->set(2);
	inst.edit.Selected->set(4);
	inst.level.checks.tagsApplyNewValue(2);
	for(const auto &line : inst.level.linedefs)
		if(line == inst.level.linedefs[1] || line == inst.level.linedefs[2])
			ASSERT_EQ(line->arg1, 1);
		else
			ASSERT_EQ(line->arg1, 0);
	for(const auto &sector : inst.level.sectors)
		if(sector.get() == inst.level.sectors[2].get() || sector.get() == inst.level.sectors[4].get())
			ASSERT_EQ(sector->tag, 2);
		else
			ASSERT_EQ(sector->tag, 0);
	ASSERT_EQ(inst.tagInMemory, 2);	// changed

	inst.edit.Selected->clear(4);
	inst.level.checks.tagsApplyNewValue(1);
	for(const auto &line : inst.level.linedefs)
		if(line == inst.level.linedefs[1] || line == inst.level.linedefs[2])
			ASSERT_EQ(line->arg1, 1);
		else
			ASSERT_EQ(line->arg1, 0);
	for(const auto &sector : inst.level.sectors)
		if(sector.get() == inst.level.sectors[2].get())
			ASSERT_EQ(sector->tag, 1);
		else if(sector == inst.level.sectors[4])
			ASSERT_EQ(sector->tag, 2);
		else
			ASSERT_EQ(sector->tag, 0);

	ASSERT_EQ(inst.tagInMemory, 1);	// changed again

	// Now work under UDMF
	inst.loaded.levelFormat = MapFormat::udmf;
	inst.edit.mode = ObjType::linedefs;
	inst.edit.Selected.emplace(ObjType::linedefs);
	inst.edit.Selected->set(1);
	inst.edit.Selected->set(2);
	inst.level.checks.tagsApplyNewValue(42);
	for(const auto &line : inst.level.linedefs)
		if(line == inst.level.linedefs[1] || line == inst.level.linedefs[2])
		{
			ASSERT_EQ(line->lineid, 42);
			ASSERT_EQ(line->arg1, 1);	// arg1 left alone
		}
		else
		{
			ASSERT_EQ(line->lineid, 0);
			ASSERT_EQ(line->arg1, 0);
		}
	ASSERT_EQ(inst.lineIDInMemory, 42);
}

static Fl_Window *findThingWindow()
{
	Fl_Window *foundWindow = nullptr;
	for(Fl_Window *window = Fl::first_window(); window; window = Fl::next_window(window))
	{
		if(!strcmp(window->label(), "Check : Things"))
		{
			foundWindow = window;
			break;
		}
	}

	return foundWindow;
}

struct WidgetLookup
{
	const Fl_Group *parent;
	int labelIndex;
};

static WidgetLookup checkDialogAndAssertMessage(Fl_Window *foundWindow, const char *message)
{
	EXPECT_GE(foundWindow->children(), 2);
	const Fl_Group *group = static_cast<const Fl_Group *>(foundWindow->child(1));	// HARD ASSUMPTION, WILL CRASH OTHERWISE
	EXPECT_TRUE(group);

	int foundItem = -1;
	for(int i = 0; i < group->children(); ++i)
	{
		const Fl_Widget *widget = group->child(i);
		EXPECT_TRUE(widget);

		if(widget->label() && strstr(widget->label(), message))
		{
			foundItem = i;
			break;
		}
	}
	EXPECT_GE(foundItem, 0);
	return {.parent = group, .labelIndex = foundItem};
}

class EThingChecks : public ::testing::Test
{
protected:
	void SetUp() override;
	void TearDown() override;

	Instance inst;
	int numUnspawnableThings = 0;
};

void EThingChecks::SetUp()
{
}

void EThingChecks::TearDown()
{
}

static void initForThings(Instance &inst)
{
	inst.edit.mode = ObjType::things;
	inst.edit.Selected.emplace(ObjType::things);
}

static constexpr double uiTimeout = 0.3;

TEST(EChecks, ThingsFindDudsUDMFUsesNewSkills)
{
	Instance inst;
	initForThings(inst);
	inst.loaded.levelFormat = MapFormat::udmf;

	// Only test with one thing, because we can't determine which are, so make it binary
	inst.level.things.push_back(std::make_shared<Thing>());

	// Put some valid non-player type to ensure skills
	auto &thing = inst.level.things[0];
	thing->type = 3004;
	thing->udmfflags = MTF_UDMF_skill5;
	thing->options = MTF_Hexen_SP | MTF_Hexen_Fighter;

	Fl::add_timeout(uiTimeout, [](void *data){
		Fl_Window *window = findThingWindow();
		checkDialogAndAssertMessage(window, "No unspawnable things");
		window->do_callback();
	}, this);

	inst.EXEC_Param[0] = "things";
	inst.CMD_MapCheck();
}

TEST(EChecks, ThingsFindDudsDoomDoesNotUseNewSkillsAndFixAddsThem)
{
	Instance inst;
	initForThings(inst);
	inst.loaded.levelFormat = MapFormat::hexen;

	// Only test with one thing, because we can't determine which are, so make it binary
	inst.level.things.push_back(std::make_shared<Thing>());

	// Put some valid non-player type to ensure skills
	auto &thing = inst.level.things[0];
	thing->type = 3004;
	thing->udmfflags = MTF_UDMF_skill5;
	thing->options = MTF_Hexen_SP | MTF_Hexen_Fighter;

	Fl::add_timeout(uiTimeout, [](void *data){
		Fl_Window *window = findThingWindow();
		WidgetLookup lookup = checkDialogAndAssertMessage(window, "1 unspawnable things");
		for(int i = lookup.labelIndex; i < lookup.parent->children(); ++i)
		{
			Fl_Widget *button = lookup.parent->child(i);
			if(button && button->label() && !strcmp(button->label(), "Fix"))
			{
				// Run the fix duds command
				static_cast<Fl_Button *>(button)->do_callback();
				auto inst = static_cast<Instance *>(data);
				EXPECT_EQ(inst->level.things[0]->options, MTF_Easy | MTF_Medium | MTF_Hard | MTF_Hexen_SP | MTF_Hexen_Fighter);
				// UDMF left alone
				EXPECT_EQ(inst->level.things[0]->udmfflags, MTF_UDMF_skill5);
				break;
			}
		}
		Fl::add_timeout(uiTimeout, [](void *data) {
			static_cast<Fl_Window *>(data)->do_callback();
		}, window);
	}, &inst);

	inst.EXEC_Param[0] = "things";
	inst.CMD_MapCheck();
}

TEST(EChecks, ThingsFindDudsUDMFForNoSkillsAndFixAddsThemAll)
{
	Instance inst;
	initForThings(inst);
	inst.loaded.levelFormat = MapFormat::udmf;

	inst.level.things.push_back(std::make_shared<Thing>());
	auto &thing = inst.level.things[0];
	thing->type = 3004;
	thing->options = MTF_Hexen_SP | MTF_Hexen_Fighter;
	Fl::add_timeout(uiTimeout, [](void *data){
		Fl_Window *window = findThingWindow();
		WidgetLookup lookup = checkDialogAndAssertMessage(window, "1 unspawnable things");
		for(int i = lookup.labelIndex; i < lookup.parent->children(); ++i)
		{
			Fl_Widget *button = lookup.parent->child(i);
			if(button && button->label() && !strcmp(button->label(), "Fix"))
			{
				// Run the fix duds command
				static_cast<Fl_Button *>(button)->do_callback();
				auto inst = static_cast<Instance *>(data);
				EXPECT_EQ(inst->level.things[0]->options, MTF_Easy | MTF_Medium | MTF_Hard | MTF_Hexen_SP | MTF_Hexen_Fighter);
				EXPECT_EQ(inst->level.things[0]->udmfflags, MTF_UDMF_skill1 | MTF_UDMF_skill5);
				break;
			}
		}
		Fl::add_timeout(uiTimeout, [](void *data) {
			static_cast<Fl_Window *>(data)->do_callback();
		}, window);
	}, &inst);

	inst.EXEC_Param[0] = "things";
	inst.CMD_MapCheck();

}

TEST(EChecks, ThingStuckInThing)
{
	Instance inst;
	initForThings(inst);
	inst.loaded.levelFormat = MapFormat::udmf;

	inst.level.things.push_back(std::make_shared<Thing>());
	inst.level.things.push_back(std::make_shared<Thing>());
	inst.level.things.push_back(std::make_shared<Thing>());

	for(const auto &T : inst.level.things)
	{
		T->type = 3004;
		T->udmfflags = MTF_UDMF_skill1;
		T->options = MTF_Hexen_Fighter | MTF_Hexen_SP;
	}

	inst.level.things[0]->SetRawXY(MapFormat::udmf, {10, 20});
	inst.level.things[1]->SetRawXY(MapFormat::udmf, {14, 25});
	inst.level.things[2]->SetRawXY(MapFormat::udmf, {50, 20});

	inst.conf.thing_types[3004] = {
		.group = 'm',
		.radius = (short)10,
		.scale = 1.0f,
	};

	Fl::add_timeout(uiTimeout, [](void *data){
		Fl_Window *window = findThingWindow();
		checkDialogAndAssertMessage(window, "1 stuck actors");
		window->do_callback();
	}, nullptr);

	inst.EXEC_Param[0] = "things";
	inst.CMD_MapCheck();

	// Now test Doom: should not be stuck now
	inst.loaded.levelFormat = MapFormat::hexen;
	Fl::add_timeout(uiTimeout, [](void *data){
		Fl_Window *window = findThingWindow();
		checkDialogAndAssertMessage(window, "No stuck actors");
		window->do_callback();
	}, nullptr);
	inst.EXEC_Param[0] = "things";
	inst.CMD_MapCheck();
}
