//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022 Ioan Chera
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

#include "Vertex.h"
#include "gtest/gtest.h"

TEST(Vertex, ZeroInit)
{
	Vertex vertex;
	ASSERT_FALSE(vertex.raw_x);
	ASSERT_FALSE(vertex.raw_y);
}

TEST(Vertex, RawToDouble)
{
	Vertex vertex;

	vertex.raw_x = FFixedPoint(123.75);
	ASSERT_EQ(vertex.x(), 123.75);

	vertex.raw_y = FFixedPoint(-234.875);
	ASSERT_EQ(vertex.y(), -234.875);

	ASSERT_EQ(vertex.xy(), v2double_t(123.75, -234.875));
}

TEST(Vertex, SetCoordinateClassicFormat)
{
	Vertex vertex;

	vertex.SetRawX(MapFormat::doom, 12.75);
	ASSERT_EQ(vertex.x(), 13);

	vertex.SetRawY(MapFormat::hexen, -24.23);
	ASSERT_EQ(vertex.y(), -24);

	vertex.SetRawX(MapFormat::hexen, -24.73);

	// UDMF keeps decimals though
	vertex.SetRawY(MapFormat::udmf, -24.75);
	ASSERT_EQ(vertex.y(), -24.75);
	vertex.SetRawX(MapFormat::udmf, 12.75);
	ASSERT_EQ(vertex.x(), 12.75);

}

TEST(Vertex, SetRawXY)
{
	Vertex vertex;

	vertex.SetRawXY(MapFormat::doom, {12.75, -24.73});
	ASSERT_EQ(vertex.x(), 13);
	ASSERT_EQ(vertex.y(), -25);

	vertex.SetRawXY(MapFormat::udmf, {12.75, -24.75});
	ASSERT_EQ(vertex.x(), 12.75);
	ASSERT_EQ(vertex.y(), -24.75);
}

TEST(Vertex, Equality)
{
	Vertex vertex;
	vertex.raw_x = FFixedPoint(1.5);
	vertex.raw_y = FFixedPoint(-2.75);

	ASSERT_TRUE(vertex.Matches(FFixedPoint(1.5), FFixedPoint(-2.75)));
	ASSERT_FALSE(vertex.Matches(FFixedPoint(1.5), FFixedPoint(-2.74)));

	Vertex vertex2;
	vertex2.raw_x = FFixedPoint(1.5);
	vertex2.raw_y = FFixedPoint(-2.75);
	Vertex vertex3;
	vertex3.raw_x = FFixedPoint(2);
	vertex3.raw_y = FFixedPoint(-2.75);

	ASSERT_EQ(vertex, vertex2);
	ASSERT_NE(vertex, vertex3);
	ASSERT_NE(vertex2, vertex3);
}
