//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2023 Ioan Chera
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
#include "e_basis.h"

// these handle rounding to integer in non-UDMF mode
void Vertex::SetRawX(MapFormat format, double x)
{
    raw_x = MakeValidCoord(format, x);
}
void Vertex::SetRawY(MapFormat format, double y)
{
    raw_y = MakeValidCoord(format, y);
}