
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

#ifndef LINEDEF_H_
#define LINEDEF_H_

#include "Side.h"

class LineDef
{
public:
	int start = 0;
	int end = 0;
	int right = -1;
	int left = -1;

	int flags = 0;
	int type = 0;
	int tag = 0;

	int arg1 = 0;
	int arg2 = 0;
	int arg3 = 0;
	int arg4 = 0;
	int arg5 = 0;

	enum { F_START, F_END, F_RIGHT, F_LEFT,
		   F_FLAGS, F_TYPE, F_TAG, F_ARG1,
		   F_ARG2, F_ARG3, F_ARG4, F_ARG5 };

public:
	bool TouchesVertex(int v_num) const
	{
		return (start == v_num) || (end == v_num);
	}

	//
	// Assuming TouchesVertex(v_num), return the other one. Undefined otherwise.
	//
	int OtherVertex(int v_num) const
	{
		return start == v_num ? end : start;
	}

	bool NoSided() const
	{
		return (right < 0) && (left < 0);
	}

	bool OneSided() const
	{
		return (right >= 0) && (left < 0);
	}

	bool TwoSided() const
	{
		return (right >= 0) && (left >= 0);
	}

	// side is either SIDE_LEFT or SIDE_RIGHT
	int WhatSideDef(Side side) const;

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
