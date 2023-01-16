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

#include "LineDef.h"

#include "Errors.h"

int LineDef::WhatSideDef(Side side) const
{
	switch (side)
	{
		case Side::left:
			return left;
		case Side::right:
			return right;

		default:
			BugError("bad side : %d\n", (int)side);
			return -1;
	}
}
