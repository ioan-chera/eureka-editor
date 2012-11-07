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

	// assume right widget is as narrow as possible

	limiter = new Fl_Box(X + 32, Y, W - right->w() - 32, H);  // FL_NO_BOX

	add(limiter);

	resizable(limiter);
}


//
// UI_Tile Destructor
//
UI_Tile::~UI_Tile()
{
}


void UI_Tile::resize(int X, int Y, int W, int H)
{
	// FIXME !!!!

	// resize ourself (skip the Fl_Group resize)
	Fl_Widget::resize(X,Y,W,H);

	// update limiter
fprintf(stderr, "limiter x=%d w=%d\n", limiter->x(), limiter->w());

	if (find(right) >= children())
	{
		left->resize(X, Y, W, H);
		return;
	}

	// FIXME
}


void UI_Tile::ShowRight()
{
	if (find(right) < children())
		return;

	int right_W = 300; //!!!!

	add(right);

	right->resize(x() + w() - right_W, y(), right_W, h());
	right->show();
	right->redraw();

	left->resize(x(), y(), w() - right_W, h());
	left->redraw();

	init_sizes();
}


void UI_Tile::HideRight()
{
	if (find(right) >= children())
		return;

	right->hide();

	remove(right);

	left->size(w(), h());
	left->redraw();

	// widgets in our group (the window) got rearranged, tell FLTK
	init_sizes();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
