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


//
// UI_Scroll Constructor
//
UI_Scroll::UI_Scroll(int X, int Y, int W, int H) :
		Fl_Widget(X, Y, W, H, NULL)
{
	scrollbar = new Fl_Scrollbar(X, Y, 16, H, NULL);

	scrollbar->align(FL_ALIGN_LEFT);
	scrollbar->color(FL_DARK2, FL_DARK2);
	scrollbar->selection_color(FL_GRAY0);

	pack = new Fl_Group(X + scrollbar->w(), Y, W - scrollbar->w(), H);
	pack->end();
}


//
// UI_Scroll Destructor
//
UI_Scroll::~UI_Scroll()
{
}


void UI_Scroll::resize(int X, int Y, int W, int H)
{
	// resize ourself 
	Fl_Widget::resize(X, Y, W, H);

}


void UI_Scroll::draw()
{
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
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
