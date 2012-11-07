//------------------------------------------------------------------------
//  Adjustable border (variation of Fl_Tile)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012 Andrew Apted
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

#include "main.h"
#include "ui_tile.h"


//
// UI_Tile Constructor
//
UI_Tile::UI_Tile(int X, int Y, int W, int H, const char *what,
                 Fl_Widget * _left, Fl_Widget * _right) :
		Fl_Tile(X, Y, W, H, what),
		left(_left), right(_right)
{
    end();

	add(left);
	add(right);

	// FIXME: limiter
}

//
// UI_Tile Destructor
//
UI_Tile::~UI_Tile()
{
}


void UI_Tile::resize(int X,int Y,int W,int H)
{
	// FIXME !!!!
}


void UI_Tile::ShowRight()
{
}


void UI_Tile::HideRight()
{
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
