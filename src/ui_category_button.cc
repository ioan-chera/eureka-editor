//------------------------------------------------------------------------
//  COLLAPSIBLE CATEGORY BUTTON
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025 Ioan Chera
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

#include "ui_category_button.h"
#include <FL/fl_draw.H>

//
// UI_CategoryButton constructor
//
UI_CategoryButton::UI_CategoryButton(int X, int Y, int W, int H, const char *label) :
	Fl_Button(X, Y, W, H, label)
{
	box(FL_NO_BOX);
	align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	labelsize(12);
	labelfont(FL_HELVETICA_BOLD);
	clear_visible_focus();
}

//
// Set expanded state
//
void UI_CategoryButton::setExpanded(bool expanded)
{
	if(mExpanded != expanded)
	{
		mExpanded = expanded;
		redraw();
		do_callback();
	}
}

//
// Draw the category button
//
void UI_CategoryButton::draw()
{
	// Draw background
	draw_box();

	// Draw arrow indicator using Unicode character
	const char *arrow = mExpanded ? "\u25BC" : "\u25B6";  // ▼ (down) or ▶ (right)
	fl_color(FL_FOREGROUND_COLOR);
	fl_font(FL_HELVETICA, 10);
	fl_draw(arrow, x() + 4, y(), 12, h(), FL_ALIGN_LEFT | FL_ALIGN_CENTER);

	// Draw label with offset for arrow
	fl_color(labelcolor());
	fl_font(labelfont(), labelsize());
	fl_draw(label(), x() + 18, y(), w() - 18, h(), align());
}

//
// Handle mouse events
//
int UI_CategoryButton::handle(int event)
{
	if(event == FL_PUSH)
	{
		setExpanded(!mExpanded);
		return 1;
	}

	return Fl_Button::handle(event);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
