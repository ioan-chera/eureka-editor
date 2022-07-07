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

#ifndef FIXEDPOINT_H_
#define FIXEDPOINT_H_

#include "sys_macro.h"

// The fraction unit
static constexpr int kFracUnit = 4096;
static constexpr double kFracUnitD = static_cast<double>(kFracUnit);

//
// Type safe fixed point number
//
class FFixedPoint
{
public:
	FFixedPoint() = default;
	explicit FFixedPoint(double value) : value(iround(value * kFracUnitD))
	{
	}
	explicit FFixedPoint(int value) : value(value * kFracUnit)
	{
	}

	int raw() const
	{
		return value;
	}

	bool operator == (FFixedPoint other) const
	{
		return value == other.value;
	}

	bool operator != (FFixedPoint other) const
	{
		return value != other.value;
	}

	bool operator < (FFixedPoint other) const
	{
		return value < other.value;
	}
	bool operator > (FFixedPoint other) const
	{
		return value > other.value;
	}

	FFixedPoint operator + (FFixedPoint other) const
	{
		FFixedPoint result;
		result.value = value + other.value;
		return result;
	}
	FFixedPoint operator - (FFixedPoint other) const
	{
		FFixedPoint result;
		result.value = value - other.value;
		return result;
	}
	FFixedPoint &operator += (FFixedPoint other)
	{
		value += other.value;
		return *this;
	}
	FFixedPoint &operator -= (FFixedPoint other)
	{
		value -= other.value;
		return *this;
	}
	FFixedPoint &operator /= (int factor)
	{
		value /= factor;
		return *this;
	}

	FFixedPoint operator * (int factor) const
	{
		FFixedPoint result;
		result.value = value * factor;
		return result;
	}

	explicit operator double() const
	{
		return value / kFracUnitD;
	}
	explicit operator int() const
	{
		return value / kFracUnit;
	}

	bool operator ! () const
	{
		return !value;
	}

	FFixedPoint abs() const
	{
		FFixedPoint result;
		result.value = ::abs(value);
		return result;
	}
private:
	int value;
};

static_assert(sizeof(FFixedPoint) == sizeof(int), "FixedPoint must be same size as int");


#endif
