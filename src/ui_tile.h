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

#ifndef __EUREKA_UI_TILE_H__
#define __EUREKA_UI_TILE_H__


class UI_Tile : public Fl_Tile
{
private:
	Fl_Widget * left;
	Fl_Widget * right;

	Fl_Box * limiter;

	// when the right widget (the browser) is hidden, this remembers
	// how much of the available width it was using, so can restore it
	// where the user expects.
	//
	// NOTE: not set or used while right widget is visible.
	int right_W;

public:
	UI_Tile(int X, int Y, int W, int H, const char *what,
	        Fl_Widget *_left, Fl_Widget *_right);

	virtual ~UI_Tile();

	/* FLTK method */
	void resize(int, int, int, int);

public:
	void ShowRight();
	void HideRight();
};


#endif  /* __EUREKA_UI_TILE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
