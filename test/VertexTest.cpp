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
	ASSERT_FALSE(vertex.xf);
	ASSERT_FALSE(vertex.yf);
}

TEST(Vertex, RawToDouble)
{
	Vertex vertex;

	vertex.xf = 123.75;
	ASSERT_EQ(vertex.x(), 123.75);

	vertex.yf = -234.875;
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
	vertex.xf = 1.5;
	vertex.yf = -2.75;

	ASSERT_TRUE(vertex.Matches(1.5, -2.75));
	ASSERT_FALSE(vertex.Matches(1.5, -2.74));

	Vertex vertex2;
	vertex2.xf = 1.5;
	vertex2.yf = -2.75;
	Vertex vertex3;
	vertex3.xf = 2;
	vertex3.yf = -2.75;

	ASSERT_EQ(vertex, vertex2);
	ASSERT_NE(vertex, vertex3);
	ASSERT_NE(vertex2, vertex3);
}
