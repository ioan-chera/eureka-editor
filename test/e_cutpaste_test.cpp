//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2024 Ioan Chera
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
#include "w_rawdef.h"

class ECutPasteFixture : public ::testing::Test
{
protected:
	~ECutPasteFixture()
	{
		inst.level.clear();
	}

	void addArea();
	void addSecondArea();

	Instance inst;
};

void ECutPasteFixture::addArea()
{
	Vertex *vertex;

	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(0);
	vertex->raw_y = FFixedPoint(0);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(0);
	vertex->raw_y = FFixedPoint(256);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(256);
	vertex->raw_y = FFixedPoint(256);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(256);
	vertex->raw_y = FFixedPoint(0);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));

	Sector *sector;
	sector = new Sector;
	sector->floorh = 0;
	sector->ceilh = 128;
	inst.level.sectors.push_back(std::shared_ptr<Sector>(sector));

	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(new SideDef));
	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(new SideDef));
	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(new SideDef));
	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(new SideDef));

	LineDef *line;
	line = new LineDef;
	line->start = 0;
	line->end = 1;
	line->right = 0;
	line->left = -1;
	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
	line = new LineDef;
	line->start = 1;
	line->end = 2;
	line->right = 1;
	line->left = -1;
	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
	line = new LineDef;
	line->start = 2;
	line->end = 3;
	line->right = 2;
	line->left = -1;
	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
	line = new LineDef;
	line->start = 3;
	line->end = 0;
	line->right = 3;
	line->left = -1;
	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));

	Thing *thing;
	thing = new Thing;
	thing->raw_x = FFixedPoint(192);
	thing->raw_y = FFixedPoint(128);
	thing->type = 1;
	thing->angle = 0;
	inst.level.things.push_back(std::shared_ptr<Thing>(thing));
	thing = new Thing;
	thing->raw_x = FFixedPoint(128);
	thing->raw_y = FFixedPoint(192);
	thing->type = 2;
	thing->angle = 90;
	inst.level.things.push_back(std::shared_ptr<Thing>(thing));
	thing = new Thing;
	thing->raw_x = FFixedPoint(64);
	thing->raw_y = FFixedPoint(128);
	thing->type = 3;
	thing->angle = 180;
	inst.level.things.push_back(std::shared_ptr<Thing>(thing));
	thing = new Thing;
	thing->raw_x = FFixedPoint(128);
	thing->raw_y = FFixedPoint(64);
	thing->type = 4;
	thing->angle = 270;
	inst.level.things.push_back(std::shared_ptr<Thing>(thing));
}

void ECutPasteFixture::addSecondArea()
{
	Vertex *vertex;

	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(320);
	vertex->raw_y = FFixedPoint(192);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(320);
	vertex->raw_y = FFixedPoint(256);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(384);
	vertex->raw_y = FFixedPoint(256);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(384);
	vertex->raw_y = FFixedPoint(192);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));

	Sector *sector;
	sector = new Sector;
	sector->floorh = 0;
	sector->ceilh = 128;
	inst.level.sectors.push_back(std::shared_ptr<Sector>(sector));

	SideDef *side;
	side = new SideDef;
	side->sector = 1;
	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
	side = new SideDef;
	side->sector = 1;
	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
	side = new SideDef;
	side->sector = 1;
	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
	side = new SideDef;
	side->sector = 1;
	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));

	LineDef *line;
	line = new LineDef;
	line->start = 4;
	line->end = 5;
	line->right = 4;
	line->left = -1;
	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
	line = new LineDef;
	line->start = 5;
	line->end = 6;
	line->right = 5;
	line->left = -1;
	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
	line = new LineDef;
	line->start = 6;
	line->end = 7;
	line->right = 6;
	line->left = -1;
	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
	line = new LineDef;
	line->start = 7;
	line->end = 4;
	line->right = 7;
	line->left = -1;
	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
}

TEST_F(ECutPasteFixture, DeletingAllPlayersWillNotCrash)
{
	addArea();

	inst.edit.Selected = selection_c();	// things
	inst.edit.Selected->set(0);
	inst.edit.Selected->set(1);
	inst.edit.Selected->set(2);
	inst.edit.Selected->set(3);

	inst.CMD_Delete();

	ASSERT_TRUE(inst.level.things.empty());
}


TEST_F(ECutPasteFixture, DeletingAllLinesFromAnotherSectorWillNotCrash)
{
	addArea();
	addSecondArea();

	ASSERT_EQ(inst.level.linedefs.size(), 8);
	ASSERT_EQ(inst.level.vertices.size(), 8);
	ASSERT_EQ(inst.level.sidedefs.size(), 8);
	ASSERT_EQ(inst.level.sectors.size(), 2);
	ASSERT_EQ(inst.level.things.size(), 4);

	inst.edit.mode = ObjType::linedefs;
	inst.edit.Selected = selection_c(ObjType::linedefs);
	inst.edit.Selected->set(4);
	inst.edit.Selected->set(5);
	inst.edit.Selected->set(6);
	inst.edit.Selected->set(7);

	inst.CMD_Delete();

	ASSERT_EQ(inst.level.linedefs.size(), 4);
	ASSERT_EQ(inst.level.vertices.size(), 4);
	ASSERT_EQ(inst.level.sidedefs.size(), 4);
	ASSERT_EQ(inst.level.sectors.size(), 1);
	ASSERT_EQ(inst.level.things.size(), 4);
}


TEST_F(ECutPasteFixture, DeletingAllLinesWillNotCrash)
{
	addArea();

	inst.edit.mode = ObjType::linedefs;
	inst.edit.Selected = selection_c(ObjType::linedefs);
	inst.edit.Selected->set(0);
	inst.edit.Selected->set(1);
	inst.edit.Selected->set(2);
	inst.edit.Selected->set(3);

	inst.CMD_Delete();

	ASSERT_TRUE(inst.level.linedefs.empty());
	ASSERT_TRUE(inst.level.vertices.empty());
	ASSERT_TRUE(inst.level.sidedefs.empty());
	ASSERT_TRUE(inst.level.sectors.empty());
	ASSERT_TRUE(inst.level.things.empty());
}

//TEST_F(ECutPasteFixture, CopyPastingSectorWithNeighbor)
//{
//
//
//	// Set up a scenario where we have multiple sectors and copy only some of them
//	// This will test the case where SD.sector = INVALID_SECTOR in CopyGroupOfObjects
//
//	addArea();      // Creates sector 0 with 4 sidedefs
//	addSecondArea(); // Creates sector 1 with 4 sidedefs
//
//	// Add a third sector that references both sector 0 and sector 1
//	Vertex *vertex;
//	vertex = new Vertex;
//	vertex->raw_x = FFixedPoint(400);
//	vertex->raw_y = FFixedPoint(0);
//	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex)); // vertex 8
//
//	vertex = new Vertex;
//	vertex->raw_x = FFixedPoint(400);
//	vertex->raw_y = FFixedPoint(128);
//	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex)); // vertex 9
//
//	// Create sector 2
//	Sector *sector = new Sector;
//	sector->floorh = 0;
//	sector->ceilh = 128;
//	inst.level.sectors.push_back(std::shared_ptr<Sector>(sector)); // sector 2
//
//	// Create sidedefs that reference different sectors
//	SideDef *side1 = new SideDef;
//	side1->sector = 0;  // References sector 0 (will NOT be copied)
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side1)); // sidedef 8
//
//	SideDef *side2 = new SideDef;
//	side2->sector = 1;  // References sector 1 (will be copied)
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side2)); // sidedef 9
//
//	// Create a linedef using these sidedefs
//	LineDef *line = new LineDef;
//	line->start = 8;
//	line->end = 9;
//	line->right = 8;  // References sidedef that points to sector 0 (not copied)
//	line->left = 9;   // References sidedef that points to sector 1 (copied)
//	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line)); // linedef 8
//
//	// Verify initial setup
//	ASSERT_EQ(inst.level.sectors.size(), 3);
//	ASSERT_EQ(inst.level.sidedefs.size(), 10);
//	ASSERT_EQ(inst.level.linedefs.size(), 9);
//
//	// Set up to copy ONLY sector 1 (not sector 0 or 2)
//	inst.edit.mode = ObjType::sectors;
//	inst.edit.Selected = selection_c(ObjType::sectors);
//	inst.edit.Selected->set(1);  // Select only sector 1
//
//	// Perform the copy operation - this should trigger the INVALID_SECTOR assignment
//	inst.CMD_Clipboard_Copy();
//
//	// Now paste to verify the clipboard was created correctly
//	// The sidedef that referenced sector 0 (which wasn't copied) should have
//	// its sector reference set to INVALID_SECTOR in the clipboard
//	inst.edit.map.xy = {500, 200}; // Position for paste
//	inst.CMD_Clipboard_Paste();
//
//	// After pasting, we should have new geometry
//	// The test verifies that the copy operation handled the INVALID_SECTOR case
//	// without crashing and that paste operation completed successfully
//	ASSERT_GT(inst.level.sectors.size(), 3);  // Should have more sectors after paste
//}
//
//TEST_F(ECutPasteFixture, LCDSevenSegmentEightLayout)
//{
//	Vertex *vertex;
//
//	// ...
//	// 0..
//	vertex = new Vertex;
//	vertex->raw_x = FFixedPoint(0);
//	vertex->raw_y = FFixedPoint(0);
//	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
//
//	// 1..
//	// 0..
//	vertex = new Vertex;
//	vertex->raw_x = FFixedPoint(0);
//	vertex->raw_y = FFixedPoint(128);
//	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
//
//	// 12.
//	// 0..
//	vertex = new Vertex;
//	vertex->raw_x = FFixedPoint(128);
//	vertex->raw_y = FFixedPoint(128);
//	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
//
//	// 123
//	// 0..
//	vertex = new Vertex;
//	vertex->raw_x = FFixedPoint(256);
//	vertex->raw_y = FFixedPoint(128);
//	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
//
//	// 123
//	// 0.4
//	vertex = new Vertex;
//	vertex->raw_x = FFixedPoint(256);
//	vertex->raw_y = FFixedPoint(0);
//	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
//
//	// 123
//	// 054
//	vertex = new Vertex;
//	vertex->raw_x = FFixedPoint(128);
//	vertex->raw_y = FFixedPoint(0);
//	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
//
//	// West sector (LOWER)
//	Sector *sector;
//	sector = new Sector;
//	sector->floorh = 0;
//	sector->ceilh = 128;
//	inst.level.sectors.push_back(std::shared_ptr<Sector>(sector));
//
//	// East sector (HIGHER)
//	sector = new Sector;
//	sector->floorh = 64;
//	sector->ceilh = 192;
//	inst.level.sectors.push_back(std::shared_ptr<Sector>(sector));
//
//	// Sidedefs for western sector (0-3)
//	SideDef *side;
//
//	//
//	//
//	// x-x
//	side = new SideDef;
//	side->sector = 0;
//	side->mid_tex = BA_InternaliseString("WALL1");
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
//
//	// x
//	// |
//	// x-x
//	side = new SideDef;
//	side->sector = 0;
//	side->mid_tex = BA_InternaliseString("WALL1");
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
//
//	// x-x
//	// |
//	// x-x
//	side = new SideDef;
//	side->sector = 0;
//	side->mid_tex = BA_InternaliseString("WALL1");
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
//
//	// x-x
//	// | |
//	// x-x
//	side = new SideDef;
//	side->sector = 0;
//	side->lower_tex = BA_InternaliseString("STEP");
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
//
//	// Sidedefs for eastern sector (4-7)
//
//	// x-x
//	//
//	//
//	side = new SideDef;
//	side->sector = 1;
//	side->mid_tex = BA_InternaliseString("WALL2");
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
//
//	// x-x
//	//   |
//	//   x
//	side = new SideDef;
//	side->sector = 1;
//	side->mid_tex = BA_InternaliseString("WALL2");
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
//
//	// x-x
//	//   |
//	// x-x
//	side = new SideDef;
//	side->sector = 1;
//	side->mid_tex = BA_InternaliseString("WALL2");
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
//
//	// x-x
//	// | |
//	// x-x
//	side = new SideDef;
//	side->sector = 1;
//	side->lower_tex = BA_InternaliseString("HEAD");
//	inst.level.sidedefs.push_back(std::shared_ptr<SideDef>(side));
//
//	// Linedefs
//	LineDef *line;
//
//	// First three linedefs: impassable, one-sided, for western sector
//	line = new LineDef;
//	line->start = 5;
//	line->end = 0;
//	line->right = 0;
//	line->left = -1;
//	line->flags = MLF_Blocking;
//	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
//
//	line = new LineDef;
//	line->start = 0;
//	line->end = 1;
//	line->right = 1;
//	line->left = -1;
//	line->flags = MLF_Blocking;
//	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
//
//	line = new LineDef;
//	line->start = 1;
//	line->end = 2;
//	line->right = 2;
//	line->left = -1;
//	line->flags = MLF_Blocking;
//	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
//
//	// Next three linedefs: impassable, one-sided, for eastern sector
//	line = new LineDef;
//	line->start = 2;
//	line->end = 3;
//	line->right = 4;
//	line->left = -1;
//	line->flags = MLF_Blocking;
//	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
//
//	line = new LineDef;
//	line->start = 3;
//	line->end = 4;
//	line->right = 5;
//	line->left = -1;
//	line->flags = MLF_Blocking;
//	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
//
//	line = new LineDef;
//	line->start = 4;
//	line->end = 5;
//	line->right = 6;
//	line->left = -1;
//	line->flags = MLF_Blocking;
//	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
//
//	// Last linedef: passable, two-sided, connecting the sectors
//	line = new LineDef;
//	line->start = 5;
//	line->end = 2;
//	line->right = 7;
//	line->left = 3;
//	line->flags = MLF_TwoSided;  // passable
//	inst.level.linedefs.push_back(std::shared_ptr<LineDef>(line));
//}

TEST_F(ECutPasteFixture, LCDSevenSegmentEightLayoutUsingCommands)
{
	// LCD 8-shaped layout using editor commands to simulate user drawing
	inst.edit.mode = ObjType::vertices;
	inst.edit.action = EditorAction::nothing;
	inst.edit.pointer_in_window = true;

	// Draw bottom-left square: (0,0) -> (0,128) -> (128,128) -> (128,0) -> close
	const v2double_t coords[] =
	{
		{0, 0}, {0, 128}, {128, 128}, {256, 128}, {256, 0}, {128, 0}, {0, 0}, {128, 0}, {128, 128}
	};

	for (v2double_t xy : coords)
	{
		inst.edit.map.xy = xy;
		inst.UpdateHighlight();
		inst.CMD_ObjectInsert();
		// TODO
	}
}
