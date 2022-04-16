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

#include "Vertex.h"
#include "Instance.h"

//
// Set raw x/y/height
//
void Vertex::SetRawX(const Instance &inst, double x)
{
	raw_x = MakeValidCoord(inst.loaded.levelFormat, x);
}
void Vertex::SetRawY(const Instance &inst, double y)
{
	raw_y = MakeValidCoord(inst.loaded.levelFormat, y);
}
