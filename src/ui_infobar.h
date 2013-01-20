//------------------------------------------------------------------------
//  Information Bar (bottom of window)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2012 Andrew Apted
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

#ifndef __EUREKA_UI_INFOBAR_H__
#define __EUREKA_UI_INFOBAR_H__


class UI_InfoBar : public Fl_Group
{
public:
	UI_InfoBar(int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_InfoBar();

public:
	Fl_Choice *mode;
	Fl_Choice *scale;

	Fl_Choice *grid_size;
	Fl_Toggle_Button *grid_snap;

	Fl_Output *mouse_x;
	Fl_Output *mouse_y;

	Fl_Box *status;


public:
	int handle(int event);
	// FLTK virtual method for handling input events.

public:
	void SetStatus(const char *str);

	void NewEditMode(char _mode);

	void SetMouse(double mx, double my);

	void SetScale(int i);  // called from Grid_State_c ONLY!
	void SetGrid(int i);   // called from Grid_State_c

	void UpdateSnap();

private:
	void UpdateModeColor();
	void UpdateSnapText();

	static void  mode_callback(Fl_Widget *, void *);
	static void scale_callback(Fl_Widget *, void *);
	static void  grid_callback(Fl_Widget *, void *);
	static void  snap_callback(Fl_Widget *, void *);
};

#endif  /* __EUREKA_UI_INFOBAR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
