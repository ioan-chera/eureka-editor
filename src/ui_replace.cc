//------------------------------------------------------------------------
//  Find and Replace
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2015 Andrew Apted
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
#include "ui_window.h"


UI_FindAndReplace::UI_FindAndReplace(int X, int Y, int W, int H) :
	Fl_Group(X, Y, W, H, NULL)
{
	box(FL_FLAT_BOX);


	Fl_Box *title = new Fl_Box(X + 50, Y + 10, W - 60, 30, "Find and Replace");
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	title->labelsize(18+KF*4);


	Y += 50;
	H -= 50;

	X += 6;
	W -= 12;

	int MX = X + W/2;


	before = new Fl_Input(X + 80, Y, W - 100, 25, "Find: ");

	Y += 30;


	after  = new Fl_Input(X + 80, Y, W - 100, 25, "Replace: ");

	Y += 30;


	resizable(NULL);

	end();
}


UI_FindAndReplace::~UI_FindAndReplace()
{ }


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
