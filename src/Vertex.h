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

#include "e_basis.h"
#include "FixedPoint.h"
#include "m_vector.h"

class Instance;

struct Vertex
{
	double xf = 0.0, yf = 0.0;

	inline double x() const noexcept
	{
		return xf;
	}
	inline double y() const noexcept
	{
		return yf;
	}
	inline v2double_t xy() const noexcept
	{
		return { x(), y() };
	}

	// these handle rounding to integer in non-UDMF mode
	void SetRawX(MapFormat format, double x);
	void SetRawY(MapFormat format, double y);

	void SetRawXY(MapFormat format, const v2double_t &pos)
	{
		SetRawX(format, pos.x);
		SetRawY(format, pos.y);
	}

	bool Matches(double ox, double oy) const
	{
		return (xf == ox) && (yf == oy);
	}

	bool operator == (const Vertex &other) const
	{
		return xf == other.xf && yf == other.yf;
	}
	bool operator != (const Vertex &other) const
	{
		return xf != other.xf || yf != other.yf;
	}
};

#endif
