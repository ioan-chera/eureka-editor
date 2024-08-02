//------------------------------------------------------------------------
//  A decent scrolling widget
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2019 Andrew Apted
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

#include "Errors.h"
#include "Instance.h"

#include "main.h"
#include "m_config.h"
#include "e_main.h"		// Map_bound_xxx

#include "r_render.h"
#include "ui_window.h"


#define HUGE_DIST  (1 << 24)

#define SCRBAR_BACK  (config::gui_scheme == 2 ? FL_DARK3 : FL_DARK2)
#define SCRBAR_COL   (config::gui_scheme == 2 ? FL_DARK1 : FL_BACKGROUND_COLOR)


//
// UI_Scroll Constructor
//
UI_Scroll::UI_Scroll(int X, int Y, int W, int H, int _bar_side) :
		Fl_Group(X, Y, W, H, NULL),
		bar_side(_bar_side),
		resize_horiz_(false),
		top_y(0), bottom_y(0)
{
	end();

	if (_bar_side < 0)
		scrollbar = new Fl_Scrollbar(X, Y, SBAR_W, H, NULL);
	else
		scrollbar = new Fl_Scrollbar(X + W - SBAR_W, Y, SBAR_W, H, NULL);

	scrollbar->align(FL_ALIGN_LEFT);
	scrollbar->color(SCRBAR_BACK, SCRBAR_BACK);
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

	if (bar_side < 0)
		scrollbar->resize(X, Y, SBAR_W, H);
	else
		scrollbar->resize(X+W-SBAR_W, Y, SBAR_W, H);


	int total_h = bottom_y - top_y;

	scrollbar->value(0, h(), 0, std::max(h(), total_h));


	if (ow != W && resize_horiz_)
	{
		for (int i = 0 ; i < Children() ; i++)
		{
			Fl_Widget * w = Child(i);

			w->resize(X + (bar_side < 0 ? SBAR_W : 0), w->y(), W - SBAR_W, w->h());
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

	scrollbar->value(pos, h(), 0, std::max(h(), total_h));

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

		   top_y = std::min(top_y, w->y());
		bottom_y = std::max(bottom_y, w->y() + w->h());
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
		pixels = std::max(1, line_size / 4);
	else if (abs(delta) == 2)
		pixels = line_size;
	else if (abs(delta) == 3)
		pixels = std::max(h() - line_size / 2, h() * 2 / 3);
	else
		pixels = HUGE_DIST;

	if (delta < 0)
		pixels = -pixels;

	ScrollByPixels(pixels);
}


void UI_Scroll::ScrollByPixels(int pixels)
{
	int pos = scrollbar->value() + pixels;

	int total_h = bottom_y - top_y;

	if (pos > total_h - h())
		pos = total_h - h();

	if (pos < 0)
		pos = 0;

	scrollbar->value(pos, h(), 0, std::max(h(), total_h));

	reposition_all(y() - pos);

	redraw();
}


void UI_Scroll::JumpToChild(int i)
{
	const Fl_Widget * w = Child(i);

	int diff = w->y() - top_y - scrollbar->value();

	ScrollByPixels(diff);
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

	scrollbar->value(0, h(), 0, std::max(h(), total_h));
}


void UI_Scroll::Line_size(int pixels)
{
	scrollbar->linesize(pixels);
}


//------------------------------------------------------------------------

#define INFO_BAR_H	30

UI_CanvasScroll::UI_CanvasScroll(Instance &inst, int X, int Y, int W, int H) :
	Fl_Group(X, Y, W, H),
	enable_bars(true),
	bound_x1(0), bound_x2(100),
	bound_y1(0), bound_y2(100), mInstance(inst)
{
	for (int i = 0 ; i < 4 ; i++)
		last_bounds[i] = 12345678;


	box(FL_NO_BOX);


	vert = new Fl_Scrollbar(X, Y + INFO_BAR_H, SBAR_W, H - SBAR_W - INFO_BAR_H, NULL);

	vert->type(FL_VERTICAL);
	vert->align(FL_ALIGN_LEFT);
	vert->color(SCRBAR_BACK, SCRBAR_BACK);
	vert->selection_color(SCRBAR_COL);
	vert->callback(bar_callback, this);


	horiz = new Fl_Scrollbar(X + SBAR_W, Y + H - SBAR_W, W - SBAR_W, SBAR_W, NULL);

	horiz->type(FL_HORIZONTAL);
	horiz->align(FL_ALIGN_LEFT);
	horiz->color(SCRBAR_BACK, SCRBAR_BACK);
	horiz->selection_color(SCRBAR_COL);
	horiz->callback(bar_callback, this);


	canvas = new UI_Canvas(mInstance, X + SBAR_W, Y, W - SBAR_W, H - SBAR_W);

	resizable(canvas);


	status = new UI_StatusBar(inst, X, Y, W, INFO_BAR_H);

	end();
}


UI_CanvasScroll::~UI_CanvasScroll()
{ }


void UI_CanvasScroll::UpdateRenderMode()
{
	int old_bars = enable_bars ? 1 : 0;
	int new_bars = config::map_scroll_bars && !mInstance.edit.render3d ? 1 : 0;

	int old_rend = status->visible() ? 1 : 0;
	int new_rend = mInstance.edit.render3d ? 1 : 0;

	// nothing changed?
	if (old_bars == new_bars && old_rend == new_rend)
		return;

	bool show_info = true;

	int I = show_info ? INFO_BAR_H : 0;
	int B = new_bars ? SBAR_W : 0;

	canvas->resize(x() + B, y() + I, w() - B, h() - B - I);

	enable_bars = new_bars;

	if (enable_bars)
	{
		  vert->show();
		 horiz->show();
	}
	else
	{
		  vert->hide();
		 horiz->hide();
	}

	if (show_info)
		status->show();
	else
		status->hide();

	init_sizes();
}


void UI_CanvasScroll::UpdateBounds()
{
	UpdateBounds_X();
	UpdateBounds_Y();
}


void UI_CanvasScroll::UpdateBounds_X()
{
	if (last_bounds[0] == mInstance.level.Map_bound1.x &&
		last_bounds[1] == mInstance.level.Map_bound2.x)
	{
		return;
	}

	last_bounds[0] = static_cast<int>(mInstance.level.Map_bound1.x);
	last_bounds[1] = static_cast<int>(mInstance.level.Map_bound2.x);

	int expand = static_cast<int>(512 + (mInstance.level.Map_bound2.x - mInstance.level.Map_bound1.x) / 8);

	bound_x1 = static_cast<int>(mInstance.level.Map_bound1.x - expand);
	bound_x2 = static_cast<int>(mInstance.level.Map_bound2.x + expand);

	Adjust_X();
}


void UI_CanvasScroll::UpdateBounds_Y()
{
	if (last_bounds[2] == mInstance.level.Map_bound1.y &&
		last_bounds[3] == mInstance.level.Map_bound2.y)
	{
		return;
	}

	last_bounds[2] = static_cast<int>(mInstance.level.Map_bound1.y);
	last_bounds[3] = static_cast<int>(mInstance.level.Map_bound2.y);

	int expand = static_cast<int>(512 + (mInstance.level.Map_bound2.y - mInstance.level.Map_bound1.y) / 8);

	bound_y1 = static_cast<int>(mInstance.level.Map_bound1.y - expand);
	bound_y2 = static_cast<int>(mInstance.level.Map_bound2.y + expand);

	Adjust_Y();
}


void UI_CanvasScroll::AdjustPos()
{
	Adjust_X();
	Adjust_Y();
}


void UI_CanvasScroll::Adjust_X()
{
	int cw = canvas->w();

	int map_w = iround(cw / mInstance.grid.getScale());
	int map_x = static_cast<int>(mInstance.grid.getOrig().x - map_w / 2);

	if (map_x > bound_x2 - map_w) map_x = bound_x2 - map_w;
	if (map_x < bound_x1) map_x = bound_x1;

	horiz->value(map_x, map_w, bound_x1, bound_x2 - bound_x1);
}


void UI_CanvasScroll::Adjust_Y()
{
	int ch = canvas->h();

	int map_h = iround(ch / mInstance.grid.getScale());
	int map_y = static_cast<int>(mInstance.grid.getOrig().y - map_h / 2);

	// invert, since screen coords are the reverse of map coords
	map_y = bound_y2 - map_h - (map_y - bound_y1);

	if (map_y > bound_y2 - map_h) map_y = bound_y2 - map_h;
	if (map_y < bound_y1) map_y = bound_y1;

	vert->value(map_y, map_h, bound_y1, bound_y2 - bound_y1);
}


void UI_CanvasScroll::Scroll_X()
{
	int pos = horiz->value();

	double map_w = canvas->w() / mInstance.grid.getScale();

	double new_x = pos + map_w / 2.0;

	mInstance.grid.MoveTo({ new_x, mInstance.grid.getOrig().y });
}


void UI_CanvasScroll::Scroll_Y()
{
	int pos = vert->value();

	double map_h = canvas->h() / mInstance.grid.getScale();

	double new_y = bound_y2 - map_h / 2.0 - (pos - bound_y1);

	mInstance.grid.MoveTo({ mInstance.grid.getOrig().x, new_y });
}


void UI_CanvasScroll::bar_callback(Fl_Widget *w, void *data)
{
	UI_CanvasScroll * that = (UI_CanvasScroll *)data;

	if (w == that->horiz)
		that->Scroll_X();

	if (w == that->vert)
		that->Scroll_Y();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
