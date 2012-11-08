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

#ifndef __EUREKA_UI_SCROLL_H__
#define __EUREKA_UI_SCROLL_H__


class UI_Scroll : public Fl_Widget
{
private:
	Fl_Scrollbar * scrollbar;

	Fl_Group * pack;

	int top_y, bottom_y;

public:
	UI_Scroll(int X, int Y, int W, int H);

	virtual ~UI_Scroll();

	/* FLTK methods */
	void resize(int X, int Y, int W, int H);
	int  handle(int event);
	void draw();

public:
	void add(Fl_Widget *w);
	void remove(Fl_Widget *w);
	void remove_first();
	void clear();

	int children() const;
	Fl_Widget * child(int i) const;

	void init_sizes();

private:
	void resize_horiz(int X, int W);
	void resize_vert (int Y, int H);

	void draw_child(Fl_Widget *w);

	void do_scroll();
	void calc_extents();
	void reposition_all(int start_y);

	static void bar_callback(Fl_Widget *, void *);
};


#endif  /* __EUREKA_UI_SCROLL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
