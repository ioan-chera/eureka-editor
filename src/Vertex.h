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

#ifndef VERTEX_H_
#define VERTEX_H_

#include "FixedPoint.h"
#include "m_vector.h"

class Instance;

class Vertex
{
public:
	FFixedPoint raw_x = {};
	FFixedPoint raw_y = {};

	enum { F_X, F_Y };

public:
	inline double x() const
	{
		return static_cast<double>(raw_x);
	}
	inline double y() const
	{
		return static_cast<double>(raw_y);
	}
	inline v2double_t xy() const
	{
		return { x(), y() };
	}

	// these handle rounding to integer in non-UDMF mode
	void SetRawX(const Instance &inst, double x);
	void SetRawY(const Instance &inst, double y);

	void SetRawXY(const Instance &inst, const v2double_t &pos)
	{
		SetRawX(inst, pos.x);
		SetRawY(inst, pos.y);
	}

	bool Matches(FFixedPoint ox, FFixedPoint oy) const
	{
		return (raw_x == ox) && (raw_y == oy);
	}

	bool operator == (const Vertex &other) const
	{
		return raw_x == other.raw_x && raw_y == other.raw_y;
	}
	bool operator != (const Vertex &other) const
	{
		return raw_x != other.raw_x || raw_y != other.raw_y;
	}
};

#endif
