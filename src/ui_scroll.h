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


#define SBAR_W  16


class UI_Scroll : public Fl_Group
{
private:
	Fl_Scrollbar * scrollbar;

	bool resize_horiz_;

	int top_y, bottom_y;

public:
	UI_Scroll(int X, int Y, int W, int H);

	virtual ~UI_Scroll();

	/* FLTK methods */
	void resize(int X, int Y, int W, int H);
	int  handle(int event);

public:
	void resize_horiz(bool r) { resize_horiz_ = r; }

	void Add(Fl_Widget *w);
	void Remove(Fl_Widget *w);
	void Remove_first();
	void Remove_all();

	int Children() const;
	Fl_Widget * Child(int i) const;

	void Init_sizes();

	void Line_size(int pixels);

	// delta is positive to move further down the list, negative to
	// move further up.  Its absolute value can be:
	//   1 : scroll by a small amount
	//   2 : scroll by one "line"
	//   3 : scroll to next/previous page
	//   4 : scroll to end
	void Scroll(int delta);

private:
	void do_scroll();
	void calc_extents();
	void reposition_all(int start_y);

	static void bar_callback(Fl_Widget *, void *);
};


#endif  /* __EUREKA_UI_SCROLL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
