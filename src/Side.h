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

#ifndef SIDE_H_
#define SIDE_H_

enum class Side
{
	right,
	left,
	neither
};
static const Side kSides[] = { Side::right, Side::left };
inline static Side operator - (Side side)
{
	if(side == Side::right)
		return Side::left;
	if(side == Side::left)
		return Side::right;
	return side;
}
inline static Side operator * (Side side1, Side side2)
{
	if(side1 == Side::neither || side2 == Side::neither)
		return Side::neither;
	if(side1 != side2)
		return Side::left;
	return Side::right;
}

#endif
