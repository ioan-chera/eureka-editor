//------------------------------------------------------------------------
//  A decent scrolling widget
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
#include "m_config.h"
#include "ui_scroll.h"


#define HUGE_DIST  (1 << 24)


#define SCRBAR_COL  (gui_scheme == 2 ? FL_GRAY0 : FL_LIGHT2)


//
// UI_Scroll Constructor
//
UI_Scroll::UI_Scroll(int X, int Y, int W, int H) :
		Fl_Group(X, Y, W, H, NULL),
		resize_horiz_(false),
		top_y(0), bottom_y(0)
{
	end();

	scrollbar = new Fl_Scrollbar(X, Y, SBAR_W, H, NULL);

	scrollbar->align(FL_ALIGN_LEFT);
	scrollbar->color(FL_DARK2, FL_DARK2);
	scrollbar->selection_color(SCRBAR_COL);
	scrollbar->callback(bar_callback, this);

	add(scrollbar);

	clip_children(1);

	resizable(NULL);
}


//
// UI_Scroll Destructor
//
UI_Scroll::~UI_Scroll()
{
}


void UI_Scroll::resize(int X, int Y, int W, int H)
{
//	int ox = x();
//	int oy = y();
//	int oh = h();
	int ow = w();


	Fl_Group::resize(X, Y, W, H);

	scrollbar->resize(X, Y, SBAR_W, H);


	int total_h = bottom_y - top_y;

	scrollbar->value(0, h(), 0, MAX(h(), total_h));


	if (ow != W && resize_horiz_)
	{
		for (int i = 0 ; i < Children() ; i++)
		{
			Fl_Widget * w = Child(i);

			w->resize(X + SBAR_W, w->y(), W - SBAR_W, w->h());
		}
	}
}


int UI_Scroll::handle(int event)
{
	return Fl_Group::handle(event);
}


void UI_Scroll::bar_callback(Fl_Widget *w, void *data)
{
	UI_Scroll * that = (UI_Scroll *)data;

	that->do_scroll();
}


void UI_Scroll::do_scroll()
{
	int pos = scrollbar->value();

	int total_h = bottom_y - top_y;

	scrollbar->value(pos, h(), 0, MAX(h(), total_h));

	reposition_all(y() - pos);

	redraw();
}


void UI_Scroll::calc_extents()
{
	if (Children() == 0)
	{
		top_y = bottom_y = 0;
		return;
	}

	   top_y =  HUGE_DIST;
	bottom_y = -HUGE_DIST;

	for (int i = 0 ; i < Children() ; i++)
	{
		Fl_Widget * w = Child(i);

		if (! w->visible())
			continue;

		   top_y = MIN(top_y, w->y());
		bottom_y = MAX(bottom_y, w->y() + w->h());
	}
}


void UI_Scroll::reposition_all(int start_y)
{
	for (int i = 0 ; i < Children() ; i++)
	{
		Fl_Widget * w = Child(i);

		int y = start_y + (w->y() - top_y);

		w->position(w->x(), y);
	}

	calc_extents();

	init_sizes();
}


void UI_Scroll::Scroll(int delta)
{
	int pixels;
	int line_size = scrollbar->linesize();

	if (abs(delta) <= 1)
		pixels = MAX(1, line_size / 4);
	else if (abs(delta) == 2)
		pixels = line_size;
	else if (abs(delta) == 3)
		pixels = MAX(h() - line_size / 2, h() * 2 / 3);
	else
		pixels = HUGE_DIST;

	if (delta < 0)
		pixels = -pixels;


	int pos = scrollbar->value() + pixels;

	int total_h = bottom_y - top_y;

	if (pos > total_h - h())
		pos = total_h - h();

	if (pos < 0)
		pos = 0;

	scrollbar->value(pos, h(), 0, MAX(h(), total_h));

	reposition_all(y() - pos);

	redraw();
}


//------------------------------------------------------------------------
//
//  PASS-THROUGHS
//

void UI_Scroll::Add(Fl_Widget *w)
{
	add(w);
}

void UI_Scroll::Remove(Fl_Widget *w)
{
	remove(w);
}

void UI_Scroll::Remove_first()
{
	// skip scrollbar
	remove(1);
}

void UI_Scroll::Remove_all()
{
	remove(scrollbar);

	clear();
	resizable(NULL);

	add(scrollbar);
}


int UI_Scroll::Children() const
{
	// ignore scrollbar
	return children() - 1;
}

Fl_Widget * UI_Scroll::Child(int i) const
{
	SYS_ASSERT(child(0) == scrollbar);

	// skip scrollbar
	return child(1 + i);
}


void UI_Scroll::Init_sizes()
{
	calc_extents();

	init_sizes();

	int total_h = bottom_y - top_y;

	scrollbar->value(0, h(), 0, MAX(h(), total_h));
}


void UI_Scroll::Line_size(int pixels)
{
	scrollbar->linesize(pixels);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
