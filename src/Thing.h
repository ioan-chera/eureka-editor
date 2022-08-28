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

#ifndef THING_H_
#define THING_H_

#include "m_vector.h"
#include "FixedPoint.h"

//
// Must be here due to dependencies
//
enum class MapFormat
{
	invalid,
	doom,
	hexen,
	udmf
};

struct Thing
{
	FFixedPoint raw_x = {};
	FFixedPoint raw_y = {};

	int angle = 0;
	int type = 0;
	int options = 0;

	// Hexen stuff
	FFixedPoint raw_h = {};

	int tid = 0;
	int special = 0;
	int arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0;

	enum IntAddress
	{
		F_ANGLE = 2,
		F_TYPE,
		F_OPTIONS,
		F_TID = 6,
		F_SPECIAL,
		F_ARG1, F_ARG2, F_ARG3, F_ARG4, F_ARG5,
	};

	enum FixedPointAddress
	{
		F_X,
		F_Y,
		F_H = 5,
	};

	inline double x() const
	{
		return static_cast<double>(raw_x);
	}
	inline double y() const
	{
		return static_cast<double>(raw_y);
	}
	inline double h() const
	{
		return static_cast<double>(raw_h);
	}
	inline v2double_t xy() const
	{
		return { x(), y() };
	}

	// these handle rounding to integer in non-UDMF mode
	void SetRawX(MapFormat format, double x);
	void SetRawY(MapFormat format, double y);
	void SetRawH(MapFormat format, double h);

	void SetRawXY(MapFormat format, const v2double_t &pos)
	{
		SetRawX(format, pos.x);
		SetRawY(format, pos.y);
	}

	int Arg(int which /* 1..5 */) const
	{
		if (which == 1) return arg1;
		if (which == 2) return arg2;
		if (which == 3) return arg3;
		if (which == 4) return arg4;
		if (which == 5) return arg5;

		return 0;
	}
};

#endif
