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

#include "Thing.h"
#include "gtest/gtest.h"

TEST(Thing, ZeroInit)
{
	Thing thing;
	ASSERT_FALSE(thing.raw_x);
	ASSERT_FALSE(thing.raw_y);
	ASSERT_FALSE(thing.angle);
	ASSERT_FALSE(thing.type);
	ASSERT_FALSE(thing.options);
	ASSERT_FALSE(thing.raw_h);
	ASSERT_FALSE(thing.tid);
	ASSERT_FALSE(thing.special);
	ASSERT_FALSE(thing.arg1);
	ASSERT_FALSE(thing.arg2);
	ASSERT_FALSE(thing.arg3);
	ASSERT_FALSE(thing.arg4);
	ASSERT_FALSE(thing.arg5);
}

TEST(Thing, RawToDouble)
{
	Thing thing;

	thing.raw_x = FFixedPoint(123.75);
	ASSERT_EQ(thing.x(), 123.75);

	thing.raw_y = FFixedPoint(-234.875);
	ASSERT_EQ(thing.y(), -234.875);

	ASSERT_EQ(thing.xy(), v2double_t(123.75, -234.875));

	thing.raw_h = FFixedPoint(-0.5);
	ASSERT_EQ(thing.h(), -0.5);
}

TEST(Thing, SetCoordinateClassicFormat)
{
	Thing thing;

	thing.SetRawX(MapFormat::doom, 12.75);
	ASSERT_EQ(thing.x(), 13);

	thing.SetRawY(MapFormat::hexen, -24.23);
	ASSERT_EQ(thing.y(), -24);

	thing.SetRawX(MapFormat::hexen, -24.73);
	ASSERT_EQ(thing.x(), -25);

	// UDMF keeps decimals though
	thing.SetRawY(MapFormat::udmf, -24.75);
	ASSERT_EQ(thing.y(), -24.75);
	thing.SetRawX(MapFormat::udmf, 12.75);
	ASSERT_EQ(thing.x(), 12.75);
}

TEST(Thing, Arg)
{
	Thing thing;

	thing.arg1 = 2;
	thing.arg2 = 5;
	thing.arg3 = -4;
	thing.arg4 = 22;
	thing.arg5 = 93;

	// Out of bounds values will just default to 0
	ASSERT_EQ(thing.Arg(0), 0);
	ASSERT_EQ(thing.Arg(1), 2);
	ASSERT_EQ(thing.Arg(2), 5);
	ASSERT_EQ(thing.Arg(3), -4);
	ASSERT_EQ(thing.Arg(4), 22);
	ASSERT_EQ(thing.Arg(5), 93);
	ASSERT_EQ(thing.Arg(6), 0);
	ASSERT_EQ(thing.Arg(7), 0);
	ASSERT_EQ(thing.Arg(8), 0);
}
