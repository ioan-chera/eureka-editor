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
#include "LineDef.h"
#include "m_select.h"
#include "Sector.h"
#include "ui_window.h"

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
		inst.level.deleteAllLinedefs();
		for(const auto &line : lines)
		{
			inst.level.addLinedef(line);
		}
	};
	std::vector<Sector> sectors;
	auto assignSectors = [&inst, &sectors]()
	{
		inst.level.sectors = sectors;
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
	inst.level.getMutableLinedef(1).tag = 1;
	ASSERT_EQ(findFreeTag(inst, false), 2);
	ASSERT_EQ(findFreeTag(inst, true), 2);

	// Also tag one sector 1
	inst.level.sectors[2].tag = 1;
	ASSERT_EQ(findFreeTag(inst, false), 2);
	ASSERT_EQ(findFreeTag(inst, true), 2);

	// Tag all of them 1: result should be 0 by now
	inst.level.getMutableLinedef(0).tag = inst.level.getMutableLinedef(2).tag = 1;
	inst.level.sectors[0].tag = inst.level.sectors[1].tag = 1;
	ASSERT_EQ(findFreeTag(inst, false), 0);
	ASSERT_EQ(findFreeTag(inst, true), 0);

	// Restore their tags but tag one by a bigger amount
	inst.level.getMutableLinedef(0).tag = inst.level.getMutableLinedef(2).tag = 0;
	inst.level.getMutableLinedef(2).tag = 4;
	inst.level.sectors[0].tag = inst.level.sectors[1].tag = 0;
	ASSERT_EQ(findFreeTag(inst, false), 2);
	ASSERT_EQ(findFreeTag(inst, true), 2);

	// Tag one sector by the remaining gap
	inst.level.sectors[1].tag = 2;
	ASSERT_EQ(findFreeTag(inst, false), 3);
	ASSERT_EQ(findFreeTag(inst, true), 3);

	// Finally no more space
	inst.level.getMutableLinedef(0).tag = 3;
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
		inst.level.sectors[i].tag = i;
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
	inst.level.getMutableLinedef(666).tag = 666;
	inst.conf.features.tag_666 = Tag666Rules::doom;	// enable it
	ASSERT_EQ(findFreeTag(inst, false), 667);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.conf.features.tag_666 = Tag666Rules::heretic;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 667);
	ASSERT_EQ(findFreeTag(inst, true), 668);
	inst.conf.features.tag_666 = Tag666Rules::disabled;	// essentially the same
	ASSERT_EQ(findFreeTag(inst, false), 667);
	ASSERT_EQ(findFreeTag(inst, true), 667);
	inst.level.sectors[667].tag = 667;
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
	{
		inst.level.addLinedef(line);
	}
	for(Sector &sector : sectors)
	{
		inst.level.sectors.push_back(sector);
	}

	// Prepare the selection lists
	auto linesel = std::make_unique<selection_c>(ObjType::linedefs);
	auto secsel = std::make_unique<selection_c>(ObjType::sectors);

	// Start with linedefs
	inst.edit.mode = ObjType::linedefs;
	inst.edit.Selected = linesel.get();

	// Nothing selected: check that nothing happens
	inst.level.checks.tagsApplyNewValue(1);
	for(const auto &line : inst.level.getLinedefs())
		ASSERT_EQ(line.tag, 0);
	for(const auto &sector : inst.level.sectors)
		ASSERT_EQ(sector.tag, 0);
	ASSERT_EQ(inst.level.checks.mLastTag, 0);	// didn't change

	// Select a couple of lines
	inst.edit.Selected->set(1);
	inst.edit.Selected->set(2);
	inst.level.checks.tagsApplyNewValue(1);
	for(const auto &line : inst.level.getLinedefs())
		if(&line == &inst.level.getLinedef(1) || &line == &inst.level.getLinedef(2))
			ASSERT_EQ(line.tag, 1);
		else
			ASSERT_EQ(line.tag, 0);
	for(const auto &sector : inst.level.sectors)
		ASSERT_EQ(sector.tag, 0);
	ASSERT_EQ(inst.level.checks.mLastTag, 1);	// changed

	// Now select a couple of sectors
	inst.edit.mode = ObjType::sectors;
	inst.edit.Selected = secsel.get();
	inst.edit.Selected->set(2);
	inst.edit.Selected->set(4);
	inst.level.checks.tagsApplyNewValue(2);
	for(const auto &line : inst.level.getLinedefs())
		if(&line == &inst.level.getLinedef(1) || &line == &inst.level.getLinedef(2))
			ASSERT_EQ(line.tag, 1);
		else
			ASSERT_EQ(line.tag, 0);
	for(const auto &sector : inst.level.sectors)
		if(&sector == &inst.level.sectors[2] || &sector == &inst.level.sectors[4])
			ASSERT_EQ(sector.tag, 2);
		else
			ASSERT_EQ(sector.tag, 0);
	ASSERT_EQ(inst.level.checks.mLastTag, 2);	// changed

	inst.edit.Selected->clear(4);
	inst.level.checks.tagsApplyNewValue(1);
	for(const auto &line : inst.level.getLinedefs())
		if(&line == &inst.level.getLinedef(1) || &line == &inst.level.getLinedef(2))
			ASSERT_EQ(line.tag, 1);
		else
			ASSERT_EQ(line.tag, 0);
	for(const auto &sector : inst.level.sectors)
		if(&sector == &inst.level.sectors[2])
			ASSERT_EQ(sector.tag, 1);
		else if(&sector == &inst.level.sectors[4])
			ASSERT_EQ(sector.tag, 2);
		else
			ASSERT_EQ(sector.tag, 0);

	ASSERT_EQ(inst.level.checks.mLastTag, 1);	// changed again
}
