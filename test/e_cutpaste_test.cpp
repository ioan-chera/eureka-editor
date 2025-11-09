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

	void SetUp() override
	{
		inst.Editor_Init();
	}

	void addArea();
	void addSecondArea();

	void moveMouse(const v2double_t &pos);

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

void ECutPasteFixture::moveMouse(const v2double_t &pos)
{
	inst.edit.map.xy = pos;
	inst.UpdateHighlight();
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
		moveMouse(xy);
		inst.CMD_ObjectInsert();
	}

	// Make right sector taller
	inst.edit.mode = ObjType::sectors;
	inst.Selection_Clear();
	moveMouse({192, 64});
	ASSERT_TRUE(inst.edit.highlight.valid());
	ASSERT_EQ(inst.edit.highlight.type, ObjType::sectors);
	const auto &sector = inst.level.sectors[inst.edit.highlight.num];
	sector->floorh = 64;
	sector->ceilh = 192;

	// Put different textures on the middle line
	inst.edit.mode = ObjType::linedefs;
	inst.Selection_Clear();
	moveMouse({128, 64});
	ASSERT_TRUE(inst.edit.highlight.valid());
	ASSERT_EQ(inst.edit.highlight.type, ObjType::linedefs);
	auto linedef = inst.level.linedefs[inst.edit.highlight.num];
	auto sidedef = inst.level.getRight(*linedef);
	ASSERT_TRUE(sidedef);
	sidedef->upper_tex = BA_InternaliseString("UPPER");
	sidedef = inst.level.getLeft(*linedef);
	ASSERT_TRUE(sidedef);
	sidedef->lower_tex = BA_InternaliseString("LOWER");

	// Do copy
	inst.edit.mode = ObjType::sectors;
	inst.Selection_Clear();
	moveMouse({192, 64});
	inst.CMD_Clipboard_Copy();

	size_t linedefCountBeforePaste = inst.level.linedefs.size();
	size_t sidedefCountBeforePaste = inst.level.sidedefs.size();

	// Do paste
	moveMouse({384, 64});
	inst.CMD_Clipboard_Paste();

	// We have a new sector
	ASSERT_EQ(inst.level.sectors.size(), 3);

	// Only four lines get added
	ASSERT_EQ(inst.level.linedefs.size(), linedefCountBeforePaste + 4);

	// More important: only four sidedefs
	ASSERT_EQ(inst.level.sidedefs.size(), sidedefCountBeforePaste + 4);

	// Now check the linedef
	inst.edit.mode = ObjType::linedefs;
	inst.Selection_Clear();
	moveMouse({320, 64});
	ASSERT_TRUE(inst.edit.highlight.valid());
	ASSERT_EQ(inst.edit.highlight.type, ObjType::linedefs);
	linedef = inst.level.linedefs[inst.edit.highlight.num];
	ASSERT_TRUE(linedef->OneSided());
	ASSERT_TRUE(linedef->flags & MLF_Blocking);
	ASSERT_FALSE(linedef->flags & MLF_TwoSided);
	sidedef = inst.level.getRight(*linedef);
	ASSERT_TRUE(sidedef);
	ASSERT_EQ(sidedef->MidTex(), "UPPER");

	// Now paste the left sector
	inst.edit.mode = ObjType::sectors;
	inst.Selection_Clear();
	moveMouse({64, 64});
	inst.CMD_Clipboard_Copy();

	linedefCountBeforePaste = inst.level.linedefs.size();
	sidedefCountBeforePaste = inst.level.sidedefs.size();

	// Paste to a clear area
	moveMouse({384, 256});
	inst.CMD_Clipboard_Paste();

	// Only four lines get added
	ASSERT_EQ(inst.level.linedefs.size(), linedefCountBeforePaste + 4);

	// More important: only four sidedefs
	ASSERT_EQ(inst.level.sidedefs.size(), sidedefCountBeforePaste + 4);

	// Check the east linedef of the pasted sector
	inst.edit.mode = ObjType::linedefs;
	inst.Selection_Clear();
	moveMouse({448, 256});
	ASSERT_TRUE(inst.edit.highlight.valid());
	ASSERT_EQ(inst.edit.highlight.type, ObjType::linedefs);
	linedef = inst.level.linedefs[inst.edit.highlight.num];
	ASSERT_TRUE(linedef->OneSided());
	ASSERT_TRUE(linedef->flags & MLF_Blocking);
	ASSERT_FALSE(linedef->flags & MLF_TwoSided);
	sidedef = inst.level.getRight(*linedef);
	ASSERT_TRUE(sidedef);
	ASSERT_EQ(sidedef->MidTex(), "LOWER");
}
