//------------------------------------------------------------------------
//  VECTORS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2021 Andrew Apted, 2021 Ioan Chera
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

#ifndef M_VECTOR_H_
#define M_VECTOR_H_

//
// Too bad the name "vector" got taken by STL to mean variable length array
//
struct Vec2d
{
	bool operator == (const Vec2d &other) const
	{
		return x == other.x && y == other.y;
	}

	double x, y;
};

#endif 
