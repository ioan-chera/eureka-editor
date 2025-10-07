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
