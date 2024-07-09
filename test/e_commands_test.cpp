//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2023 Ioan Chera
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

#include "Instance.h"
#include "LineDef.h"
#include "Vertex.h"
#include "w_wad.h"
#include "gtest/gtest.h"

class SelectNeighbor : public ::testing::Test
{
protected:
    SelectNeighbor()
    {
        inst.edit.Selected.emplace(inst.edit.mode, true);
    }
    ~SelectNeighbor()
    {
        inst.edit.Selected.reset();
    }

    void addVertex(int x, int y);
    void addSector(int floorh, int ceilh);
	void addSide(const SString &upper, const SString &middle, const SString &lower, int sector,
				 int yoffset = 0);
    void addLine(int v1, int v2, int s1, int s2);

    Instance inst;
    Document &doc = inst.level;
};

void SelectNeighbor::addVertex(int x, int y)
{
    Vertex *vertex = new Vertex;
    vertex->SetRawXY(MapFormat::doom, v2double_t{ (double)x, (double)y });
    doc.vertices.emplace_back(vertex);
}

void SelectNeighbor::addSector(int floorh, int ceilh)
{
    Sector *sector = new Sector;
    sector->floorh = floorh;
    sector->ceilh = ceilh;
    sector->floor_tex = BA_InternaliseString("FLOOR");
    sector->ceil_tex = BA_InternaliseString("CEIL");
    sector->light = 160;
    sector->type = sector->tag = 0;
    doc.sectors.emplace_back(sector);
}

void SelectNeighbor::addSide(const SString &upper, const SString &middle, const SString &lower,
    int sector, int yoffset)
{
    SideDef *side = new SideDef{};
    side->upper_tex = BA_InternaliseString(upper);
    side->mid_tex = BA_InternaliseString(middle);
    side->lower_tex = BA_InternaliseString(lower);
    side->sector = sector;
	side->y_offset = yoffset;
    doc.sidedefs.emplace_back(side);
}

void SelectNeighbor::addLine(int v1, int v2, int s1, int s2)
{
    LineDef *line = new LineDef{};
    line->start = v1;
    line->end = v2;
    line->right = s1;
    line->left = s2;
    doc.linedefs.emplace_back(line);
}

class SelectNeighborTexture : public SelectNeighbor
{
protected:
    void SetUp() override;
};

void SelectNeighborTexture::SetUp()
{
    // Use case: highlight a wall and see selection spread to:
    // - other walls
    // - upper and lower textures
    /*

    Wall aspect

    *---*---*---*
    |           |
    |       *---*
    |L0  L1 |L2
    |       *---*
    |           |
    *---*---*---*

    Highlight L1, expect to select L0 and L2 due to same texture being shown

    Top view vertices and sectors
              *
             / \
    *---*---*---*
    |           |
    |   *---*   |
    |   |mid|   |
    |   *---*   |
    |        \._|
    *-----------*

    */

    addVertex(0, 0);
    addVertex(64, 0);
    addVertex(128, 0);
    addVertex(192, 0);
    addVertex(0, -192); // bottom vertices
    addVertex(192, -192); // bottom vertices
    addVertex(160, 32); // top V
    // Middle cage
    addVertex(64, -64);
    addVertex(128, -64);
    addVertex(128, -128);
    addVertex(64, -128);

    addSector(0, 128);
    addSector(32, 96);

    addSide("-", "WALL", "-", 0);
    addSide("-", "WALL", "-", 0);
    addSide("WALL", "-", "WALL", 0);
    // bottom walls
    addSide("-", "OTHER", "-", 0);
    addSide("-", "OTHER", "-", 0);
    addSide("-", "OTHER", "-", 0);
    // top walls (differently textured)
    addSide("-", "-", "-", 1);
    addSide("-", "NICHE", "-", 1);
    addSide("-", "NICHE", "-", 1);
    // middle cage: all are same textured, except for one to show which get picked
    addSide("-", "CAGE", "-", 0);
    addSide("-", "CAGE2", "-", 0);
    addSide("-", "CAGE", "-", 0);
    addSide("-", "CAGE", "-", 0);
    addSide("-", "CAGE", "-", 0);
    addSide("-", "CAGE", "-", 0);
    addSide("-", "CAGE", "-", 0);
    addSide("-", "CAGE", "-", 0);
    // Bottom right beam (midtexture though)
    addSide("-", "OTHER", "-", 0);
    addSide("-", "-", "-", 0);

    addLine(0, 1, 0, -1);
    addLine(1, 2, 1, -1);
    addLine(2, 3, 2, 6);

    addLine(3, 5, 3, -1);
    addLine(5, 4, 4, -1);
    addLine(4, 0, 5, -1);

    addLine(2, 6, 7, -1);
    addLine(6, 3, 8, -1);

    // Cage (8-11)
    addLine(7, 8, 9, 10);   // ----> (top)
    addLine(9, 8, 11, 12);  // up (right)
    addLine(9, 10, 13, 14); // <---- (bottom)
    addLine(7, 10, 15, 16); // down (left)

    // Bottom right beam (midtexture)
    addLine(5, 9, 18, 17);
}

TEST_F(SelectNeighborTexture, SelectFromMidWall)
{
    inst.EXEC_Param[0] = "texture";
    inst.edit.mode = ObjType::linedefs;
    inst.edit.highlight.num = 1;
    inst.edit.highlight.parts = PART_RT_LOWER;

    inst.CMD_SelectNeighbors();
    ASSERT_EQ(inst.edit.Selected->count_obj(), 3);
    ASSERT_EQ(inst.edit.Selected->get_ext(0), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(1), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(2), PART_RT_LOWER | PART_RT_UPPER);
}

TEST_F(SelectNeighborTexture, SelectFromBottomThenAddThenClearAll)
{
    inst.EXEC_Param[0] = "texture";
    inst.edit.mode = ObjType::linedefs;
    inst.edit.highlight.num = 2;
    inst.edit.highlight.parts = PART_RT_LOWER;

    inst.CMD_SelectNeighbors();
    ASSERT_EQ(inst.edit.Selected->count_obj(), 3);
    ASSERT_EQ(inst.edit.Selected->get_ext(0), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(1), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(2), PART_RT_LOWER | PART_RT_UPPER);

    // Now select the two textures from north to show they get added
    inst.edit.highlight.num = 7;
    inst.edit.highlight.parts = PART_RT_LOWER;
    inst.CMD_SelectNeighbors();
    ASSERT_EQ(inst.edit.Selected->count_obj(), 5);
    ASSERT_EQ(inst.edit.Selected->get_ext(0), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(1), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(2), PART_RT_LOWER | PART_RT_UPPER);
    ASSERT_EQ(inst.edit.Selected->get_ext(6), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(7), PART_RT_LOWER);

    // Now manually select a bottom one
    inst.edit.Selected->set_ext(4, PART_RT_LOWER);

    // And apply the command on it
    inst.edit.highlight.num = 4;
	inst.edit.highlight.parts = PART_RT_LOWER;
    inst.CMD_SelectNeighbors();
    ASSERT_TRUE(inst.edit.Selected->empty());
}

TEST_F(SelectNeighborTexture, SelectCage)
{
    // Select interior
    inst.EXEC_Param[0] = "texture";
    inst.edit.mode = ObjType::linedefs;
    inst.edit.highlight.num = 8;
    inst.edit.highlight.parts = PART_RT_RAIL;

    inst.CMD_SelectNeighbors();
    ASSERT_EQ(inst.edit.Selected->count_obj(), 4);
    ASSERT_EQ(inst.edit.Selected->get_ext(8), PART_RT_RAIL);
    ASSERT_EQ(inst.edit.Selected->get_ext(9), PART_LF_RAIL);
    ASSERT_EQ(inst.edit.Selected->get_ext(10), PART_RT_RAIL);
    ASSERT_EQ(inst.edit.Selected->get_ext(11), PART_LF_RAIL);

    // Now select exterior
    inst.edit.highlight.num = 9;
    inst.edit.highlight.parts = PART_RT_RAIL;
    inst.CMD_SelectNeighbors();
    ASSERT_EQ(inst.edit.Selected->count_obj(), 4);
    ASSERT_EQ(inst.edit.Selected->get_ext(8), PART_RT_RAIL);
    ASSERT_EQ(inst.edit.Selected->get_ext(9), PART_LF_RAIL | PART_RT_RAIL);
    ASSERT_EQ(inst.edit.Selected->get_ext(10), PART_RT_RAIL | PART_LF_RAIL);
    ASSERT_EQ(inst.edit.Selected->get_ext(11), PART_LF_RAIL | PART_RT_RAIL);
}

TEST_F(SelectNeighborTexture, WallsDoNotPropagateToRails)
{
    inst.EXEC_Param[0] = "texture";
    inst.edit.mode = ObjType::linedefs;
    inst.edit.highlight.num = 3;
    inst.edit.highlight.parts = PART_RT_LOWER;
    inst.CMD_SelectNeighbors();

    ASSERT_EQ(inst.edit.Selected->count_obj(), 3);
    ASSERT_EQ(inst.edit.Selected->get_ext(3), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(4), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(5), PART_RT_LOWER);
}

TEST_F(SelectNeighborTexture, RailsDoNotPropagateToWalls)
{
    inst.EXEC_Param[0] = "texture";
    inst.edit.mode = ObjType::linedefs;
    inst.edit.highlight.num = 12;
    inst.edit.highlight.parts = PART_LF_RAIL;
    inst.CMD_SelectNeighbors();

    ASSERT_EQ(inst.edit.Selected->count_obj(), 1);
    ASSERT_EQ(inst.edit.Selected->get_ext(12), PART_LF_RAIL);
}

/*
 Same height selection

 Alcoves: open door, shut door, shut lift, shut mid, open all
     x---x   x---x---x   x---x   x---x
     |OD |   |SD vSL |   |SM |   |OA |
 x---x-->x---x<--x-->x---x<--x---x-->x---x
 |                                       |
 |       x<------x------>x<------x       |
 |       ^ step  ^ st+he v head  v       |
 |       x<--x-->x<------x-->x-->x       |
 |       v s+^               ^ h+v       |
 |       x<--x               x-->x       |
 |                                       |
 x---------------------------------------x

 */

// What to consider: taller textures will check opening size
//                   shorter textures will check their equality and position
//                   need to combine them

class SelectNeighborHeight : public SelectNeighbor
{
protected:
    void SetUp() override;
};

void SelectNeighborHeight::SetUp()
{
    addVertex(0, 0);    // 0
    addVertex(640, 0);

    addVertex(128, 64); // 2
    addVertex(192, 64);
    addVertex(448, 64);
    addVertex(512, 64);

    addVertex(128, 128);    // 6
    addVertex(192, 128);
    addVertex(256, 128);
    addVertex(384, 128);
    addVertex(448, 128);
    addVertex(512, 128);

    addVertex(128, 192);    // 12
    addVertex(256, 192);
    addVertex(384, 192);
    addVertex(512, 192);

    addVertex(0, 256);  // 16
    addVertex(64, 256);
    addVertex(128, 256);
    addVertex(192, 256);
    addVertex(256, 256);
    addVertex(320, 256);
    addVertex(384, 256);
    addVertex(448, 256);
    addVertex(512, 256);
    addVertex(576, 256);
    addVertex(640, 256);

    addVertex(64, 320); // 27
    addVertex(128, 320);
    addVertex(192, 320);
    addVertex(256, 320);
    addVertex(320, 320);
    addVertex(384, 320);
    addVertex(448, 320);
    addVertex(512, 320);
    addVertex(576, 320);
    // 36

    addSector(0, 128);  // 0

    addSector(16, 128); // 1
    addSector(0, 112);

    addSector(8, 128);  // 3
    addSector(8, 120);
    addSector(0, 120);

    addSector(0, 48);   // 6
    addSector(0, 0);
    addSector(128, 128);
    addSector(48, 48);
    addSector(0, 128);
    // 11

    addSide("-", "wall", "-", 0);   // 0

    addSide("-", "wall", "-", 0);   // 1
    addSide("-", "wall", "-", 0);

    addSide("-", "-", "-", 1);  // 3
    addSide("-", "-", "step", 0);
    addSide("top", "-", "-", 0);
    addSide("-", "-", "-", 2);

    addSide("-", "-", "step", 0);   // 7
    addSide("-", "-", "-", 1);
    addSide("-", "-", "step", 0);
    addSide("-", "-", "-", 1);
    addSide("-", "-", "-", 2);
    addSide("top", "-", "-", 0);
    addSide("-", "-", "-", 2);
    addSide("top", "-", "-", 0);

    addSide("-", "-", "step", 3);   // 15
    addSide("-", "-", "-", 1);
    addSide("-", "-", "step", 0);
    addSide("-", "-", "-", 3);
    addSide("-", "-", "-", 4);
    addSide("top", "-", "step", 0);
    addSide("top", "-", "-", 0);
    addSide("-", "-", "-", 5);
    addSide("-", "-", "-", 2);
    addSide("top", "-", "-", 5);

    addSide("-", "-", "-", 3);  // 25
    addSide("-", "-", "step", 0);
    addSide("-", "-", "-", 4);
    addSide("top", "-", "-", 3);
    addSide("-", "-", "-", 4);
    addSide("-", "-", "step", 5);
    addSide("-", "-", "-", 5);
    addSide("top", "-", "-", 0);

    addSide("-", "-", "step", 0);   // 33
    addSide("-", "-", "-", 3);
    addSide("-", "-", "-", 4);
    addSide("top", "-", "step", 0);
    addSide("top", "-", "-", 0);
    addSide("-", "-", "-", 5);

    addSide("-", "wall", "-", 0);   // 39
    addSide("door", "-", "-", 0);
    addSide("-", "-", "-", 6);
    addSide("-", "wall", "-", 0);
    addSide("-", "-", "-", 7);
    addSide("door", "-", "-", 0);
    addSide("-", "-", "lift", 0);
    addSide("-", "-", "-", 8);
    addSide("-", "wall", "-", 0);
    addSide("-", "-", "-", 9);
    addSide("door", "-", "lift", 0);
    addSide("-", "wall", "-", 0);
    addSide("door", "-", "lift", 0);
    addSide("-", "-", "-", 10);
    addSide("-", "wall", "-", 0);

    addSide("-", "wall", "-", 6);   // 54
    addSide("-", "wall", "-", 6);
    addSide("-", "wall", "-", 7);
    addSide("-", "-", "low", 7);
    addSide("high", "-", "-", 8);
    addSide("-", "wall", "-", 8);
    addSide("-", "wall", "-", 9);
    addSide("-", "wall", "-", 9);
    addSide("-", "wall", "-", 10);
    addSide("-", "wall", "-", 10);

    addSide("-", "wall", "-", 6);   // 64
    addSide("-", "wall", "-", 7);
    addSide("-", "wall", "-", 8);
    addSide("-", "wall", "-", 9);
    addSide("-", "wall", "-", 10);
    // 69

    addLine(1, 0, 0, -1);	// 0

    addLine(0, 16, 1, -1);	// 1
    addLine(26, 1, 2, -1);

    addLine(3, 2, 3, 4);	// 3
    addLine(4, 5, 5, 6);

    addLine(6, 2, 7, 8);	// 5
    addLine(3, 7, 9, 10);
    addLine(4, 10, 11, 12);
    addLine(11, 5, 13, 14);

    addLine(7, 6, 15, 16);	// 9
    addLine(7, 8, 17, 18);
    addLine(9, 8, 19, 20);
    addLine(9, 10, 21, 22);
    addLine(10, 11, 23, 24);

    addLine(6, 12, 25, 26);	// 14
    addLine(8, 13, 27, 28);
    addLine(14, 9, 29, 30);
    addLine(15, 11, 31, 32);

    addLine(13, 12, 33, 34),    // 18
    addLine(13, 14, 35, 36),
    addLine(15, 14, 37, 38),

    addLine(16, 17, 39, -1);	// 21
    addLine(17, 18, 40, 41);
    addLine(18, 19, 42, -1);
    addLine(20, 19, 43, 44);
    addLine(20, 21, 45, 46);
    addLine(21, 22, 47, -1);
    addLine(23, 22, 48, 49);
    addLine(23, 24, 50, -1);
    addLine(24, 25, 51, 52);
    addLine(25, 26, 53, -1);

    addLine(17, 27, 54, -1);	// 31
    addLine(28, 18, 55, -1);
    addLine(19, 29, 56, -1);
    addLine(30, 20, 57, 58);
    addLine(31, 21, 59, -1);
    addLine(22, 32, 60, -1);
    addLine(33, 23, 61, -1);
    addLine(24, 34, 62, -1);
    addLine(35, 25, 63, -1);

    addLine(27, 28, 64, -1);	// 40
    addLine(29, 30, 65, -1);
    addLine(30, 31, 66, -1);
    addLine(32, 33, 67, -1);
    addLine(34, 35, 68, -1);
	// 45
}

TEST_F(SelectNeighborHeight, WallGetsClosedDoorsButNotMids)
{
	inst.EXEC_Param[0] = "height";
	inst.edit.mode = ObjType::linedefs;
	inst.edit.highlight.num = 23;
	inst.edit.highlight.parts = PART_RT_LOWER;

	inst.CMD_SelectNeighbors();

	ASSERT_EQ(inst.edit.Selected->count_obj(), 4);
	ASSERT_EQ(inst.edit.Selected->get_ext(23), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(24), PART_LF_UPPER);
	ASSERT_EQ(inst.edit.Selected->get_ext(25), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(26), PART_RT_LOWER);
}

TEST_F(SelectNeighborHeight, WallGoesAcrossSectorsThenAddAnotherThenClear)
{
	inst.EXEC_Param[0] = "height";
	inst.edit.mode = ObjType::linedefs;
	inst.edit.highlight.num = 0;
	inst.edit.highlight.parts = PART_RT_LOWER;

	inst.CMD_SelectNeighbors();

	ASSERT_EQ(inst.edit.Selected->count_obj(), 9);
	ASSERT_EQ(inst.edit.Selected->get_ext(0), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(1), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(2), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(21), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(28), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(30), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(38), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(39), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(44), PART_RT_LOWER);

	// Now select the open sector
	inst.edit.highlight.num = 31;
	inst.edit.highlight.parts = PART_RT_LOWER;
	inst.CMD_SelectNeighbors();

	ASSERT_EQ(inst.edit.Selected->count_obj(), 12);
	ASSERT_EQ(inst.edit.Selected->get_ext(31), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(32), PART_RT_LOWER);
	ASSERT_EQ(inst.edit.Selected->get_ext(40), PART_RT_LOWER);

	// Now apply the same command on a selected line and see how all gets deselected.
	inst.edit.highlight.num = 30;
	inst.edit.highlight.parts = PART_RT_LOWER;
	inst.CMD_SelectNeighbors();
	ASSERT_TRUE(inst.edit.Selected->empty());
}

TEST_F(SelectNeighborHeight, SelectSteps)
{
	inst.EXEC_Param[0] = "height";
	inst.edit.mode = ObjType::linedefs;
	inst.edit.highlight.num = 14;
	inst.edit.highlight.parts = PART_LF_LOWER;

    // Select smaller steps
    inst.CMD_SelectNeighbors();

    ASSERT_EQ(inst.edit.Selected->count_obj(), 6);
    ASSERT_EQ(inst.edit.Selected->get_ext(10), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(11), PART_LF_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(14), PART_LF_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(16), PART_LF_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(18), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(19), PART_LF_LOWER);

    // Now select a top
    inst.edit.highlight.num = 15;
    inst.edit.highlight.parts = PART_LF_UPPER;
    inst.CMD_SelectNeighbors();

    ASSERT_EQ(inst.edit.Selected->count_obj(), 10);
    ASSERT_EQ(inst.edit.Selected->get_ext(10), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(11), PART_LF_LOWER | PART_LF_UPPER);
    ASSERT_EQ(inst.edit.Selected->get_ext(12), PART_RT_UPPER);
    ASSERT_EQ(inst.edit.Selected->get_ext(14), PART_LF_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(15), PART_LF_UPPER);
    ASSERT_EQ(inst.edit.Selected->get_ext(16), PART_LF_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(17), PART_LF_UPPER);
    ASSERT_EQ(inst.edit.Selected->get_ext(18), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(19), PART_LF_LOWER | PART_LF_UPPER);
    ASSERT_EQ(inst.edit.Selected->get_ext(20), PART_RT_UPPER);

    // Select an inner top+
    inst.edit.highlight.num = 13;
    inst.edit.highlight.parts = PART_LF_UPPER;
    inst.CMD_SelectNeighbors();

    ASSERT_EQ(inst.edit.Selected->count_obj(), 11);
    ASSERT_EQ(inst.edit.Selected->get_ext(13), PART_LF_UPPER);
    // Nothing else

    // Select an outer step+
    inst.edit.highlight.num = 6;
    inst.edit.highlight.parts = PART_RT_LOWER;
    inst.CMD_SelectNeighbors();

    ASSERT_EQ(inst.edit.Selected->count_obj(), 14);
    ASSERT_EQ(inst.edit.Selected->get_ext(3), PART_LF_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(5), PART_RT_LOWER);
    ASSERT_EQ(inst.edit.Selected->get_ext(6), PART_RT_LOWER);
}

/*
Mid-line same-height selection


*/

static const byte texHeight4[] =
{
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x01, 0x03, 0x00, 0x00, 0x00, 0x75, 0x16, 0xC7,
	0x79, 0x00, 0x00, 0x00, 0x03, 0x50, 0x4C, 0x54, 0x45, 0x00, 0x00, 0x00, 0xA7, 0x7A, 0x3D, 0xDA,
	0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x01, 0x63, 0x80, 0x02, 0x00, 0x00, 0x08,
	0x00, 0x01, 0x30, 0x8A, 0x83, 0xD2, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42,
	0x60, 0x82
};

static const byte texHeight6[] =
{
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x38, 0xDE, 0x66,
	0x72, 0x00, 0x00, 0x00, 0x03, 0x50, 0x4C, 0x54, 0x45, 0x00, 0x00, 0x00, 0xA7, 0x7A, 0x3D, 0xDA,
	0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x01, 0x63, 0x40, 0x02, 0x00, 0x00, 0x0C,
	0x00, 0x01, 0xFB, 0x9E, 0xB4, 0x85, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42,
	0x60, 0x82
};

static const byte texHeight8[] =
{
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xD4, 0x07,
	0x02, 0x00, 0x00, 0x00, 0x03, 0x50, 0x4C, 0x54, 0x45, 0x00, 0x00, 0x00, 0xA7, 0x7A, 0x3D, 0xDA,
	0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x01, 0x63, 0x40, 0x03, 0x00, 0x00, 0x10,
	0x00, 0x01, 0x25, 0xFE, 0x3D, 0x34, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42,
	0x60, 0x82
};

class SelectNeighborMidLines : public SelectNeighbor
{
protected:
	void SetUp() override;

private:
	void initTextures();
};

/*

 1. Same sector height, different texture heights

 */

void SelectNeighborMidLines::SetUp()
{
	initTextures();

	// Now we have the texes
   
   // corners
   addVertex(0, 0);
   addVertex(0, 256);
   addVertex(256, 256);
   addVertex(256, 0);
      
   addSector(0, 128);
   
   // walls
   addSide("-", "WALL", "-", 0);
   addSide("-", "WALL", "-", 0);
   addSide("-", "WALL", "-", 0);
   addSide("-", "WALL", "-", 0);
      
   // walls
   addLine(0, 1, 0, -1);
   addLine(1, 2, 1, -1);
   addLine(2, 3, 2, -1);
   addLine(3, 0, 3, -1);
}

void SelectNeighborMidLines::initTextures()
{
	auto wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	wad->AddLump("TX_START");
	Lump_c *lump;
	lump = &wad->AddLump("PIC4");
	lump->Write(texHeight4, sizeof(texHeight4));
	lump = &wad->AddLump("PIC6");
	lump->Write(texHeight6, sizeof(texHeight6));
	lump = &wad->AddLump("PIC8");
	lump->Write(texHeight8, sizeof(texHeight8));
	wad->AddLump("TX_END");

	inst.wad.master.setGameWad(wad);

	inst.conf.features.tx_start = 1;
	inst.wad.W_LoadTextures(inst.conf);
}

TEST_F(SelectNeighborMidLines, CheckDifferentTextureHeights)
{
	addVertex(64, 128);
	addVertex(80, 128);
	addVertex(96, 128);
	addVertex(128, 128);
	addVertex(144, 128);
	addVertex(160, 128);
	addVertex(176, 128);
	addVertex(192, 128);
	addVertex(208, 128);
	addVertex(224, 128);
	
	addSide("-", "PIC4", "-", 0);
	addSide("-", "PIC4", "-", 0);
	
	addSide("-", "PIC4", "-", 0);
	addSide("-", "PIC6", "-", 0);
	
	addSide("-", "PIC6", "-", 0);
	addSide("-", "PIC6", "-", 0);
	
	addSide("-", "PIC6", "-", 0);
	addSide("-", "PIC6", "-", 0);
	
	addSide("-", "PIC8", "-", 0);
	addSide("-", "PIC8", "-", 0);
	
	addSide("-", "PIC8", "-", 0);
	addSide("-", "PIC8", "-", 0);
	
	addSide("-", "PIC8", "-", 0);
	addSide("-", "PIC8", "-", 0);
	
	addSide("-", "PIC8", "-", 0, 1);
	addSide("-", "PIC8", "-", 0);
	
	addLine(4, 5, 4, 5);
	addLine(5, 6, 6, 7);
	addLine(6, 7, 8, 9);
	addLine(7, 8, 10, 11);
	addLine(8, 9, 12, 13);
	addLine(9, 10, 14, 15);
	addLine(10, 11, 16, 17);
	addLine(11, 12, 18, 19);
	
	inst.EXEC_Param[0] = "height";
	inst.edit.mode = ObjType::linedefs;
	inst.edit.highlight.num = 5;
	inst.edit.highlight.parts = PART_RT_RAIL;

	inst.CMD_SelectNeighbors();
	
	ASSERT_EQ(inst.edit.Selected->count_obj(), 2);
	ASSERT_EQ(inst.edit.Selected->get_ext(4), PART_RT_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(5), PART_RT_RAIL);
	
	inst.edit.highlight.num = 6;
	inst.edit.highlight.parts = PART_LF_RAIL;
	inst.CMD_SelectNeighbors();
	
	ASSERT_EQ(inst.edit.Selected->count_obj(), 4);
	ASSERT_EQ(inst.edit.Selected->get_ext(4), PART_RT_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(5), PART_RT_RAIL | PART_LF_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(6), PART_LF_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(7), PART_LF_RAIL);
	
	inst.edit.highlight.num = 8;
	inst.edit.highlight.parts = PART_RT_RAIL;
	inst.CMD_SelectNeighbors();
	
	ASSERT_EQ(inst.edit.Selected->count_obj(), 7);
	ASSERT_EQ(inst.edit.Selected->get_ext(4), PART_RT_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(5), PART_RT_RAIL | PART_LF_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(6), PART_LF_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(7), PART_LF_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(8), PART_RT_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(9), PART_RT_RAIL);
	ASSERT_EQ(inst.edit.Selected->get_ext(10), PART_RT_RAIL);
}
