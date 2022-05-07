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

#ifndef M_VECTOR_H_
#define M_VECTOR_H_

#include "sys_macro.h"
#include <math.h>

struct v2int_t;

struct v2double_t
{
	v2double_t() = default;
	v2double_t(double x, double y) : x(x), y(y)
	{
	}
	explicit v2double_t(double block) : x(block), y(block)
	{
	}
	explicit inline v2double_t(v2int_t v2);


	bool nonzero() const
	{
		return x || y;
	}

	bool inbounds(const v2double_t &bottomleft, const v2double_t &topright) const
	{
		return x >= bottomleft.x && x <= topright.x && y >= bottomleft.y && y <= topright.y;
	}

	bool inboundsStrict(const v2double_t &bottomleft, const v2double_t &topright) const
	{
		return x > bottomleft.x && x < topright.x && y > bottomleft.y && y < topright.y;
	}

	bool operator == (const v2double_t &other) const
	{
		return x == other.x && y == other.y;
	}

	v2double_t &operator += (const v2double_t &other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}
	v2double_t &operator -= (const v2double_t &other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	v2double_t &operator /= (double val)
	{
		x /= val;
		y /= val;
		return *this;
	}

	v2double_t operator + (const v2double_t &other) const
	{
		return { x + other.x, y + other.y };
	}
	v2double_t operator - (const v2double_t &other) const
	{
		return { x - other.x, y - other.y };
	}
	double operator * (const v2double_t &other) const
	{
		return x * other.x + y * other.y;
	}
	v2double_t operator * (double val) const
	{
		return { x * val, y * val };
	}
	v2double_t operator / (double val) const
	{
		return { x / val, y / val };
	}

	double atan2() const
	{
		return ::atan2(y, x);
	}
	double hypot() const
	{
		return ::hypot(x, y);
	}
	inline v2int_t iround() const;

	double chebyshev() const
	{
		return fmax(fabs(x), fabs(y));
	}

	double x, y;
};

struct v2int_t
{
	v2int_t() = default;
	v2int_t(int x, int y) : x(x), y(y)
	{
	}
	explicit v2int_t(const v2double_t &v2) : x(static_cast<int>(v2.x)), y(static_cast<int>(v2.y))
	{
	}

	int x, y;
};

inline v2double_t::v2double_t(v2int_t v2) : x(v2.x), y(v2.y)
{
}

inline v2int_t v2double_t::iround() const
{
	return { ::iround(x), ::iround(y) };
}

struct v3double_t
{
	v3double_t() = default;
	v3double_t(double x, double y, double z) : x(x), y(y), z(z)
	{
	}
	v3double_t(int x, int y, int z) : x(x), y(y), z(z)
	{
	}
	explicit v3double_t(const v2double_t &v2) : x(v2.x), y(v2.y), z(0)
	{
	}

	union
	{
		struct
		{
			double x, y;
		};
		v2double_t xy;
	};
	double z;
};

#endif
