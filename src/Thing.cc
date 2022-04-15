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
#include "Instance.h"

void Thing::SetRawX(const Instance &inst, double x)
{
	raw_x = inst.MakeValidCoord(x);
}
void Thing::SetRawY(const Instance &inst, double y)
{
	raw_y = inst.MakeValidCoord(y);
}
void Thing::SetRawH(const Instance &inst, double h)
{
	raw_h = inst.MakeValidCoord(h);
}

