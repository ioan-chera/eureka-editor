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

// a fixed-point coordinate with 12 bits of fractional part.
typedef int fixcoord_t;

// The fraction unit
static constexpr int kFracUnit = 4096;
static constexpr double kFracUnitD = static_cast<double>(kFracUnit);

//
// Fixcoord to double
//
inline static double fromCoord(fixcoord_t fx)
{
	return fx / kFracUnitD;
}

//
// Double to fixcoord
//
inline static fixcoord_t toCoord(double db)
{
	return static_cast<fixcoord_t>(iround(db * kFracUnitD));
}

//
// Scales an integer to fixed-point coordinates.
//
inline static fixcoord_t intToCoord(int i)
{
	return static_cast<fixcoord_t>(i * kFracUnit);
}

//
// Truncates fixed-point coordinates to integer
//
inline static int coordToInt(fixcoord_t i)
{
	return static_cast<int>(i / kFracUnit);
}

#endif
