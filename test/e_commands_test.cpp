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

class SelectNeighborFixture : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override
    {
        // Cleanup
        delete inst.edit.Selected;
    }

    Instance inst;
};

void SelectNeighborFixture::SetUp()
{
    inst.edit.Selected = new selection_c(inst.edit.mode, true);

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

    Document &doc = inst.level;

    auto addVertex = [&doc](int x, int y)
    {
        Vertex *vertex = new Vertex;
        vertex->SetRawXY(MapFormat::doom, v2double_t{(double)x, (double)y});
        doc.vertices.emplace_back(vertex);
    };
    auto addSector = [&doc](int floorh, int ceilh)
    {
        Sector *sector = new Sector;
        sector->floorh = floorh;
        sector->ceilh = ceilh;
        sector->floor_tex = BA_InternaliseString("FLOOR");
        sector->ceil_tex = BA_InternaliseString("CEIL");
        sector->light = 160;
        sector->type = sector->tag = 0;
        doc.sectors.emplace_back(sector);
    };
    auto addSide = [&doc](const SString &upper, const SString &middle, const SString &lower,
                          int sector)
    {
        SideDef *side = new SideDef{};
        side->upper_tex = BA_InternaliseString(upper);
        side->mid_tex = BA_InternaliseString(middle);
        side->lower_tex = BA_InternaliseString(lower);
        side->sector = sector;
        doc.sidedefs.emplace_back(side);
    };
    auto addLine = [&doc](int v1, int v2, int s1, int s2)
    {
        LineDef *line = new LineDef{};
        line->start = v1;
        line->end = v2;
        line->right = s1;
        line->left = s2;
        doc.linedefs.emplace_back(line);
    };

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

TEST_F(SelectNeighborFixture, SelectFromMidWall)
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

TEST_F(SelectNeighborFixture, SelectFromBottomThenAddThenClearAll)
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

TEST_F(SelectNeighborFixture, SelectCage)
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

TEST_F(SelectNeighborFixture, WallsDoNotPropagateToRails)
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

TEST_F(SelectNeighborFixture, RailsDoNotPropagateToWalls)
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
     |OD |   |SD |SL |   |OL |   |OM |
 x---x-->x---x<--x-->x---x<--x---x-->x---x
 |                                       |
 |       x<------x------>x<------x       |
 |       ^ step  | st+he | head  v       |
 |       x---x-->x<------x-->x---x       |
 |       v s+^               ^ h+v       |
 |       x<--x               x-->x       |
 |                                       |
 x---------------------------------------x

 */

// TODO: masked mid-line height equivalence
// What to consider: taller textures will check opening size
//                   shorter textures will check their equality and position
//                   need to combine them
