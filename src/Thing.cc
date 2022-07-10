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

#include "Thing.h"
#include "e_basis.h"

void Thing::SetRawX(MapFormat format, double x)
{
	raw_x = MakeValidCoord(format, x);
}
void Thing::SetRawY(MapFormat format, double y)
{
	raw_y = MakeValidCoord(format, y);
}
void Thing::SetRawH(MapFormat format, double h)
{
	raw_h = MakeValidCoord(format, h);
}