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

#include "Side.h"
#include "gtest/gtest.h"
#include <vector>

TEST(Side, Enumeration)
{
	std::vector<Side> sides;
	for(Side side : kSides)
		sides.push_back(side);
	ASSERT_EQ(sides.size(), 2);
	ASSERT_EQ(sides[0], Side::right);
	ASSERT_EQ(sides[1], Side::left);
}

TEST(Side, Negation)
{
	ASSERT_EQ(-Side::right, Side::left);
	ASSERT_EQ(-Side::left, Side::right);
	ASSERT_EQ(-Side::neither, Side::neither);
}

TEST(Side, Multiplication)
{
	ASSERT_EQ(Side::right * Side::right, Side::right);
	ASSERT_EQ(Side::right * Side::left, Side::left);
	ASSERT_EQ(Side::right * Side::neither, Side::neither);
	ASSERT_EQ(Side::left * Side::right, Side::left);
	ASSERT_EQ(Side::left * Side::left, Side::right);
	ASSERT_EQ(Side::left * Side::neither, Side::neither);
	ASSERT_EQ(Side::neither * Side::right, Side::neither);
	ASSERT_EQ(Side::neither * Side::left, Side::neither);
	ASSERT_EQ(Side::neither * Side::neither, Side::neither);
}
