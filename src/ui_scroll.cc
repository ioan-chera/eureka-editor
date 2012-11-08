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
#include "ui_scroll.h"


#define SBAR_W  16


//
// UI_Scroll Constructor
//
UI_Scroll::UI_Scroll(int X, int Y, int W, int H) :
		Fl_Widget(X, Y, W, H, NULL),
		total_h(0)
{
	scrollbar = new Fl_Scrollbar(X, Y, SBAR_W, H, NULL);

	scrollbar->align(FL_ALIGN_LEFT);
	scrollbar->color(FL_DARK2, FL_DARK2);
	scrollbar->selection_color(FL_GRAY0);
	scrollbar->callback(bar_callback, this);

	pack = new Fl_Group(X + SBAR_W, Y, W - SBAR_W, H);
	pack->end();

	pack->resizable(NULL);
}


//
// UI_Scroll Destructor
//
UI_Scroll::~UI_Scroll()
{
}


void UI_Scroll::resize(int X, int Y, int W, int H)
{
	int ox = x();
	int oy = y();
	int ow = w();
	int oh = h();

	// resize ourself first
	Fl_Widget::resize(X, Y, W, H);

	scrollbar->resize(X, Y, SBAR_W, H);

	pack->resize(X + SBAR_W, Y, W - SBAR_W, H);

/*
	// horizontal change?
	if (W != ow)
	{
		pack->resize(ox, Y, ow, H);
	}

	// vertical change?
	if (H != oh)
		resize_vert(H);
	
	if (Y != oy)
	{
	}
	if (! (Y == oy && H == oh))
	{
		resize_vert(
	}
*/
}


void UI_Scroll::draw()
{
	draw_box();
	draw_child(scrollbar);
	draw_child(pack);
}


void UI_Scroll::draw_child(Fl_Widget *w)
{
	w->clear_damage(FL_DAMAGE_ALL);
	w->draw();
	w->clear_damage();
}


int UI_Scroll::handle(int event)
{
	return scrollbar->handle(event) || pack->handle(event);
}


void UI_Scroll::bar_callback(Fl_Widget *w, void *data)
{
	UI_Scroll * that = (UI_Scroll *)data;

	that->reposition_all(that->y() - that->scrollbar->value());
	that->redraw();
}


void UI_Scroll::calc_bottom()
{
	total_h = 0;

	for (int i = 0 ; i < children() ; i++)
	{
		Fl_Widget * w = child(i);

		if (w->visible())
			total_h += w->h();
	}
}


void UI_Scroll::reposition_all(int top_y)
{
	for (int i = 0 ; i < children() ; i++)
	{
		Fl_Widget * w = child(i);

		w->position(w->x(), top_y);

		if (w->visible())
			top_y += w->h();
	}
}


//------------------------------------------------------------------------
//
//  PASS-THROUGHS
//

void UI_Scroll::add(Fl_Widget *w)
{
	pack->add(w);
}

void UI_Scroll::remove(Fl_Widget *w)
{
	pack->remove(w);
}

void UI_Scroll::remove_first()
{
	pack->remove(0);
}

void UI_Scroll::clear()
{
	pack->clear();
	pack->resizable(NULL);
}


int UI_Scroll::children() const
{
	return pack->children();
}

Fl_Widget * UI_Scroll::child(int i) const
{
	return pack->child(i);
}


void UI_Scroll::init_sizes()
{
	pack->init_sizes();

	calc_bottom();

	scrollbar->value(0, h(), 0, MAX(h(), total_h));
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
