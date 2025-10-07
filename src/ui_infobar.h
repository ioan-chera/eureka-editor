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

#include <FL/Fl_Group.H>

class Fl_Menu_Button;
class Fl_Toggle_Button;

class UI_InfoBar : public Fl_Group
{
public:
	UI_InfoBar(Instance &inst, int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_InfoBar();

private:
	Fl_Menu_Button *mode;
	Fl_Menu_Button *scale;
	Fl_Menu_Button *grid_size;
	Fl_Menu_Button *sec_rend;
	Fl_Menu_Button *ratio_lock;

	Fl_Toggle_Button *grid_snap;

public:
	// FLTK virtual method for handling input events.
	int handle(int event);

public:
	void NewEditMode(ObjType new_mode);

	void SetMouse();

	void SetScale(double new_scale);
	void SetGrid(int new_step);

	void UpdateSnap();
	void UpdateSecRend();
	void UpdateRatio();

private:
	static const char  *scale_options_str;
	static const double scale_amounts[9];

	void UpdateModeColor();
	void UpdateSnapText();

	static void mode_callback(Fl_Widget *, void *);
	static void rend_callback(Fl_Widget *, void *);
	static void scale_callback(Fl_Widget *, void *);
	static void sc_minus_callback(Fl_Widget *, void *);
	static void sc_plus_callback(Fl_Widget *, void *);
	static void grid_callback(Fl_Widget *, void *);
	static void snap_callback(Fl_Widget *, void *);
	static void ratio_callback(Fl_Widget *, void *);

	Instance &inst;
};


//------------------------------------------------------------------------

class UI_StatusBar : public Fl_Widget
{
private:
	SString status;

	Instance &inst;

public:
	UI_StatusBar(Instance &inst, int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_StatusBar();

public:
	// FLTK methods
	void draw();
	int handle(int event);

	// this only used by Status_Set() and Status_Clear()
	void SetStatus(const char *str);

private:
	void IB_ShowDrag(int cx, int cy);
	void IB_ShowTransform(int cx, int cy);
	void IB_ShowOffsets(int cx, int cy);
	void IB_ShowDrawLine(int cx, int cy);

	void IB_String(int& cx, int& cy, const char *str);
	void IB_Number(int& cx, int& cy, const char *label, int value, int size);
	void IB_Coord (int& cx, int& cy, const char *label, float value);
	void IB_Flag  (int& cx, int& cy, bool value, const char *label_on, const char *label_off);
};

#endif  /* __EUREKA_UI_INFOBAR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
