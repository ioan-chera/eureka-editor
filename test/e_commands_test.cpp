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
#include "gtest/gtest.h"

class SelectNeighbor : public ::testing::Test
{
protected:
    SelectNeighbor()
    {
        inst.edit.Selected = new selection_c(inst.edit.mode, true);
    }
    ~SelectNeighbor()
    {
        delete inst.edit.Selected;
    }

    void addVertex(int x, int y);
    void addSector(int floorh, int ceilh);
    void addSide(const SString &upper, const SString &middle, const SString &lower, int sector);
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
    int sector)
{
    SideDef *side = new SideDef{};
    side->upper_tex = BA_InternaliseString(upper);
    side->mid_tex = BA_InternaliseString(middle);
    side->lower_tex = BA_InternaliseString(lower);
    side->sector = sector;
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
 Alcoves: shut door, shut lift, open door, open lift, open mid
     x---x   x---x---x   x---x   x---x
     |OD |   |SD vSL |   |OL |   |OM |
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

// TODO: masked mid-line height equivalence
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
    addSector(48, 128);
    addSector(32, 64);
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
    addSide("-", "-", "-", 2);
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

    addSide("-", "wall", "-", 0);   // 33
    addSide("door", "-", "-", 0);
    addSide("-", "-", "-", 6);
    addSide("-", "wall", "-", 0);
    addSide("-", "-", "-", 7);
    addSide("door", "-", "-", 0);
    addSide("-", "-", "lift", 0);
    addSide("-", "-", "-", 8);
    addSide("-", "wall", "-", 0);
    addSide("-", "-", "-", 9);
    addSide("-", "-", "lift", 0);
    addSide("-", "wall", "-", 0);
    addSide("door", "-", "lift", 0);
    addSide("-", "-", "-", 10);
    addSide("-", "wall", "-", 0);

    addSide("-", "wall", "-", 6);   // 48
    addSide("-", "wall", "-", 6);
    addSide("-", "wall", "-", 7);
    addSide("-", "-", "low", 7);
    addSide("high", "-", "-", 8);
    addSide("-", "wall", "-", 8);
    addSide("-", "wall", "-", 9);
    addSide("-", "wall", "-", 9);
    addSide("-", "wall", "-", 10);
    addSide("-", "wall", "-", 10);

    addSide("-", "wall", "-", 6);   // 58
    addSide("-", "wall", "-", 7);
    addSide("-", "wall", "-", 8);
    addSide("-", "wall", "-", 9);
    addSide("-", "wall", "-", 10);
    // 63

    addLine(1, 0, 0, -1);

    addLine(0, 16, 1, -1);
    addLine(26, 1, 2, -1);

    addLine(3, 2, 3, 4);
    addLine(4, 5, 5, 6);

    addLine(6, 2, 7, 8);
    addLine(3, 7, 9, 10);
    addLine(4, 10, 11, 12);
    addLine(11, 5, 13, 14);

    addLine(7, 6, 15, 16);
    addLine(7, 8, 17, 18);
    addLine(9, 8, 19, 20);
    addLine(9, 10, 21, 22);
    addLine(10, 11, 23, 24);

    addLine(6, 12, 25, 26);
    addLine(8, 13, 27, 28);
    addLine(14, 9, 29, 30);
    addLine(15, 11, 31, 32);

    addLine(16, 17, 33, -1);
    addLine(17, 18, 34, 35);
    addLine(18, 19, 36, -1);
    addLine(20, 19, 37, 38);
    addLine(20, 21, 39, 40);
    addLine(21, 22, 41, -1);
    addLine(23, 22, 42, 43);
    addLine(23, 24, 44, -1);
    addLine(24, 25, 45, 46);
    addLine(25, 26, 47, -1);
    
    addLine(17, 27, 48, -1);
    addLine(28, 18, 49, -1);
    addLine(19, 29, 50, -1);
    addLine(30, 20, 51, 52);
    addLine(31, 21, 53, -1);
    addLine(22, 32, 54, -1);
    addLine(33, 23, 55, -1);
    addLine(24, 34, 56, -1);
    addLine(35, 25, 57, -1);

    addLine(27, 28, 58, -1);
    addLine(29, 30, 59, -1);
    addLine(30, 31, 60, -1);
    addLine(32, 33, 61, -1);
    addLine(34, 35, 62, -1);
}
