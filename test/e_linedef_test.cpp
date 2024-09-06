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

#include "e_linedef.h"
#include "Instance.h"
#include "gtest/gtest.h"

TEST(ELinedef, MoveCoordOntoLinedef)
{
	Instance inst;
	Vertex* vertex;
	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(0);
	vertex->raw_y = FFixedPoint(0);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));
	vertex = new Vertex;
	vertex->raw_x = FFixedPoint(64);
	vertex->raw_y = FFixedPoint(64);
	inst.level.vertices.push_back(std::shared_ptr<Vertex>(vertex));


	inst.level.linedefs.push_back(std::make_shared<LineDef>());
	auto& L = inst.level.linedefs.back();
	L->start = 0;
	L->end = 1;

	v2double_t v = { 32, 16 };

	linemod::moveCoordOntoLinedef(inst.level, 0, v);
	
	ASSERT_DOUBLE_EQ(v.x, 24.0);
	ASSERT_DOUBLE_EQ(v.y, 24.0);
}