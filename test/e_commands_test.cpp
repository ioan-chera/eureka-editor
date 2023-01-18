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

TEST(Commands, SelectNeighborLinesByTexture)
{
    Instance inst;
    inst.EXEC_Param[0] = "texture";
    inst.edit.mode = ObjType::linedefs;
    inst.edit.highlight.num = 1;
    inst.edit.highlight.parts = PART_RT_RAIL;
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
      \_     _/
        \_ _/
          *

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
    addVertex(96, -64); // bottom V
    addVertex(160, 32); // top V

    addSector(0, 128);
    addSector(32, 96);

    addSide("-", "WALL", "-", 0);
    addSide("-", "WALL", "-", 0);
    addSide("WALL", "-", "WALL", 0);
    // bottom walls
    addSide("-", "OTHER", "-", 0);
    addSide("-", "OTHER", "-", 0);
    // top walls
    addSide("-", "-", "-", 1);
    addSide("-", "OTHER", "-", 1);
    addSide("-", "OTHER", "-", 1);

    addLine(0, 1, 0, -1);
    addLine(1, 2, 1, -1);
    addLine(2, 3, 2, 5);
    addLine(3, 4, 3, -1);
    addLine(4, 0, 4, -1);
    addLine(2, 5, 6, -1);
    addLine(5, 3, 7, -1);

    // Now we have it
    inst.CMD_SelectNeighbors();
    ASSERT_EQ(inst.edit.Selected->count_obj(), 3);
    ASSERT_EQ(inst.edit.Selected->get_ext(0), PART_RT_RAIL);
    ASSERT_EQ(inst.edit.Selected->get_ext(1), PART_RT_RAIL);
    ASSERT_EQ(inst.edit.Selected->get_ext(2), PART_RT_LOWER | PART_RT_UPPER);

    // Cleanup
    delete inst.edit.Selected;
}
