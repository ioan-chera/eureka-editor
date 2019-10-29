//------------------------------------------------------------------------
//  Information Bar (bottom of window)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2019 Andrew Apted
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

private:
	Fl_Menu_Button *mode;
	Fl_Menu_Button *scale;
	Fl_Menu_Button *grid_size;

	Fl_Toggle_Button *grid_snap;

public:
	// FLTK virtual method for handling input events.
	int handle(int event);

public:
	void NewEditMode(obj_type_e new_mode);

	void SetMouse(double mx, double my);

	void SetScale(double new_scale);
	void SetGrid(int new_step);

	void UpdateSnap();

private:
	static const char  *scale_options_str;
	static const double scale_amounts[9];

	static const char *grid_options_str;
	static const int   grid_amounts[12];

	void UpdateModeColor();
	void UpdateSnapText();

	static void mode_callback(Fl_Widget *, void *);
	static void scale_callback(Fl_Widget *, void *);
	static void sc_minus_callback(Fl_Widget *, void *);
	static void sc_plus_callback(Fl_Widget *, void *);
	static void grid_callback(Fl_Widget *, void *);
	static void snap_callback(Fl_Widget *, void *);
};


//------------------------------------------------------------------------

class UI_StatusBar : public Fl_Widget
{
private:
	std::string status;

public:
	UI_StatusBar(int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_StatusBar();

public:
	// FLTK methods
	void draw();
	int handle(int event);

	// this only used by Status_Set() and Status_Clear()
	void SetStatus(const char *str);

private:
	void IB_DragDelta(int cx, int cy);

	void IB_Number(int& cx, int& cy, const char *label, int value, int size);
	void IB_Coord (int& cx, int& cy, const char *label, float value);
	void IB_Flag  (int& cx, int& cy, bool value, const char *label_on, const char *label_off);
};

#endif  /* __EUREKA_UI_INFOBAR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
