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

#include "FixedPoint.h"
#include "gtest/gtest.h"

TEST(FixedPoint, FracUnitConsistency)
{
	ASSERT_EQ(kFracUnit, static_cast<int>(kFracUnitD));
}

TEST(FixedPoint, FracUnitIsPowerOf2)
{
	ASSERT_GT(kFracUnit, 0);
	ASSERT_FALSE(kFracUnit & (kFracUnit - 1));
}

TEST(FixedPoint, FromCoord)
{
	auto number = FFixedPoint(123.25);
	ASSERT_EQ(static_cast<double>(number), 123.25);

	// now try negative

	number = FFixedPoint(-67.75);
	ASSERT_EQ(static_cast<double>(number), -67.75);
}

TEST(FixedPoint, ToCoordRaw)
{
	double number = 123.25;
	ASSERT_EQ(FFixedPoint(number).raw(), static_cast<int>(123.25 * kFracUnit));

	number = -67.75;
	ASSERT_EQ(FFixedPoint(number).raw(), static_cast<int>(-67.75 * kFracUnit));
}

TEST(FixedPoint, IntToCoord)
{
	int number = 123;
	ASSERT_EQ(FFixedPoint(number).raw(), 123 * kFracUnit);

	number = -67;
	ASSERT_EQ(FFixedPoint(number).raw(), -67 * kFracUnit);
}

TEST(FixedPoint, CoordToInt)
{
	auto number = FFixedPoint(123);
	ASSERT_EQ(static_cast<int>(number), 123);

	// now try negative

	number = FFixedPoint(-67);
	ASSERT_EQ(static_cast<int>(number), -67);
}

TEST(FixedPoint, ZeroInit)
{
	FFixedPoint point = {};
	ASSERT_EQ(point.raw(), 0);

	ASSERT_EQ(FFixedPoint{}.raw(), 0);
}
