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

#include "main.h"
#include "ui_window.h"

#include "e_main.h"
#include "m_config.h"
#include "m_game.h"
#include "r_grid.h"
#include "r_render.h"


#define SNAP_COLOR  (gui_scheme == 2 ? fl_rgb_color(255,96,0) : fl_rgb_color(255, 96, 0))
#define FREE_COLOR  (gui_scheme == 2 ? fl_rgb_color(0,192,0) : fl_rgb_color(128, 255, 128))


const char *UI_InfoBar::scale_options_str =
	"  6%| 12%| 25%| 33%| 50%|100%|200%|400%|800%";

const double UI_InfoBar::scale_amounts[9] =
{
	0.0625, 0.125, 0.25, 0.33333, 0.5, 1.0, 2.0, 4.0, 8.0
};

const char *UI_InfoBar::grid_options_str =
	"1024|512|256|192|128| 64| 32| 16|  8|  4|  2|OFF";

const int UI_InfoBar::grid_amounts[12] =
{
	1024, 512, 256, 192, 128, 64, 32, 16, 8, 4, 2,
	-1 /* OFF */
};


//
// UI_InfoBar Constructor
//
UI_InfoBar::UI_InfoBar(int X, int Y, int W, int H, const char *label) :
    Fl_Group(X, Y, W, H, label)
{
	box(FL_FLAT_BOX);


	// Fitts' law : keep buttons flush with bottom of window
	Y += 4;
	H -= 4;


	Fl_Box *mode_lab = new Fl_Box(FL_NO_BOX, X, Y, 56, H, "Mode:");
	mode_lab->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	mode_lab->labelsize(KF_fonth);


	mode = new Fl_Menu_Button(X+58, Y, 96, H, "Things");
	mode->align(FL_ALIGN_INSIDE);
	mode->add("Things|Linedefs|Sectors|Vertices");
	mode->callback(mode_callback, this);
	mode->labelsize(KF_fonth);

	X = mode->x() + mode->w() + 12;


	Fl_Box *scale_lab = new Fl_Box(FL_NO_BOX, X, Y, 50, H, "Scale:");
	scale_lab->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	scale_lab->labelsize(KF_fonth);

	scale = new Fl_Menu_Button(X+52+26, Y, 78, H, "100%");
	scale->align(FL_ALIGN_INSIDE);
	scale->add(scale_options_str);
	scale->callback(scale_callback, this);
	scale->labelsize(KF_fonth);

	Fl_Button *sc_minus, *sc_plus;

	sc_minus = new Fl_Button(X+52, Y+1, 24, H-2, "-");
	sc_minus->callback(sc_minus_callback, this);
	sc_minus->labelfont(FL_HELVETICA_BOLD);
	sc_minus->labelsize(KF_fonth);

	sc_plus = new Fl_Button(X+52+26+80, Y+1, 24, H-2, "+");
	sc_plus->callback(sc_plus_callback, this);
	sc_plus->labelfont(FL_HELVETICA_BOLD);
	sc_plus->labelsize(16);

	X = sc_plus->x() + sc_plus->w() + 12;


	Fl_Box *gs_lab = new Fl_Box(FL_NO_BOX, X, Y, 42, H, "Grid:");
	gs_lab->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	gs_lab->labelsize(KF_fonth);

	grid_size = new Fl_Menu_Button(X+44, Y, 72, H, "OFF");

	grid_size->align(FL_ALIGN_INSIDE);
	grid_size->add(grid_options_str);
	grid_size->callback(grid_callback, this);
	grid_size->labelsize(KF_fonth);
	grid_size->textsize(KF_fonth);

	X = grid_size->x() + grid_size->w() + 12;


	grid_snap = new Fl_Toggle_Button(X+4, Y, 72, H);
	grid_snap->value(grid.snap ? 1 : 0);
	grid_snap->color(FREE_COLOR);
	grid_snap->selection_color(SNAP_COLOR);
	grid_snap->callback(snap_callback, this);
	grid_snap->labelsize(KF_fonth);

	UpdateSnapText();

	X = grid_snap->x() + grid_snap->w() + 14;


	Fl_Box *div = new Fl_Box(FL_FLAT_BOX, X, Y-4, 3, H+4, NULL);
	div->color(WINDOW_BG, WINDOW_BG);

	X += 10;


	resizable(NULL);

	end();
}

//
// UI_InfoBar Destructor
//
UI_InfoBar::~UI_InfoBar()
{ }


int UI_InfoBar::handle(int event)
{
	return Fl_Group::handle(event);
}


void UI_InfoBar::mode_callback(Fl_Widget *w, void *data)
{
	Fl_Menu_Button *mode = (Fl_Menu_Button *)w;

	static const char *mode_keys = "tlsvr";

	Editor_ChangeMode(mode_keys[mode->value()]);
}


void UI_InfoBar::scale_callback(Fl_Widget *w, void *data)
{
	Fl_Menu_Button *scale = (Fl_Menu_Button *)w;

	double new_scale = scale_amounts[scale->value()];

	grid.NearestScale(new_scale);
}


void UI_InfoBar::sc_minus_callback(Fl_Widget *w, void *data)
{
	ExecuteCommand("Zoom", "-1", "/center");
}

void UI_InfoBar::sc_plus_callback(Fl_Widget *w, void *data)
{
	ExecuteCommand("Zoom", "+1", "/center");
}


void UI_InfoBar::grid_callback(Fl_Widget *w, void *data)
{
	Fl_Menu_Button *gsize = (Fl_Menu_Button *)w;

	int new_step = grid_amounts[gsize->value()];

	if (new_step < 0)
		grid.SetShown(false);
	else
		grid.ForceStep(new_step);
}


void UI_InfoBar::snap_callback(Fl_Widget *w, void *data)
{
	Fl_Toggle_Button *grid_snap = (Fl_Toggle_Button *)w;

	// update editor state
	grid.SetSnap(grid_snap->value() ? true : false);
}


//------------------------------------------------------------------------

void UI_InfoBar::NewEditMode(obj_type_e new_mode)
{
	switch (new_mode)
	{
		case OBJ_THINGS:   mode->value(0); break;
		case OBJ_LINEDEFS: mode->value(1); break;
		case OBJ_SECTORS:  mode->value(2); break;
		case OBJ_VERTICES: mode->value(3); break;

		default: break;
	}

	UpdateModeColor();
}


void UI_InfoBar::SetMouse(double mx, double my)
{
	// TODO this method should go away

	main_win->status_bar->redraw();
}


void UI_InfoBar::SetScale(double new_scale)
{
	double perc = new_scale * 100.0;

	char buffer[64];

	if (perc < 10.0)
		sprintf(buffer, "%1.1f%%", perc);
	else
		sprintf(buffer, "%3d%%", (int)perc);

	scale->copy_label(buffer);
}

void UI_InfoBar::SetGrid(int new_step)
{
	if (new_step < 0)
	{
		grid_size->label("OFF");
	}
	else
	{
		char buffer[64];
		sprintf(buffer, "%d", new_step);
		grid_size->copy_label(buffer);
	}
}


void UI_InfoBar::UpdateSnap()
{
   grid_snap->value(grid.snap ? 1 : 0);

   UpdateSnapText();
}


void UI_InfoBar::UpdateModeColor()
{
	switch (mode->value())
	{
		case 0: mode->label("Things");   mode->color(THING_MODE_COL);  break;
		case 1: mode->label("Linedefs"); mode->color(LINE_MODE_COL);   break;
		case 2: mode->label("Sectors");  mode->color(SECTOR_MODE_COL); break;
		case 3: mode->label("Vertices"); mode->color(VERTEX_MODE_COL); break;
	}
}


void UI_InfoBar::UpdateSnapText()
{
	if (grid_snap->value())
	{
		grid_snap->label("SNAP");
	}
	else
	{
		grid_snap->label("Free");
	}

	grid_snap->redraw();
}


//------------------------------------------------------------------------


#define INFO_TEXT_COL	fl_rgb_color(192, 192, 192)
#define INFO_DIM_COL	fl_rgb_color(128, 128, 128)


UI_StatusBar::UI_StatusBar(int X, int Y, int W, int H, const char *label) :
    Fl_Widget(X, Y, W, H, label),
	status()
{
	box(FL_NO_BOX);
}

UI_StatusBar::~UI_StatusBar()
{ }


int UI_StatusBar::handle(int event)
{
	// this never handles any events
	return 0;
}

void UI_StatusBar::draw()
{
	fl_color(fl_rgb_color(64, 64, 64));
	fl_rectf(x(), y(), w(), h());

	fl_color(fl_rgb_color(96, 96, 96));
	fl_rectf(x(), y() + h() - 1, w(), 1);

	fl_push_clip(x(), y(), w(), h());

	fl_font(FL_COURIER, 16);

	int cx = x() + 10;
	int cy = y() + 20;

	if (edit.render3d)
	{
		IB_Number(cx, cy, "x", I_ROUND(r_view.x), 5);
		IB_Number(cx, cy, "y", I_ROUND(r_view.y), 5);
		IB_Number(cx, cy, "z", I_ROUND(r_view.z) - Misc_info.view_height, 4);

		int ang = I_ROUND(r_view.angle * 180 / M_PI);
		if (ang < 0) ang += 360;

		IB_Number(cx, cy, "angle", ang, 3);
		cx += 8;

		IB_Flag(cx, cy, r_view.gravity, "GRAVITY", "gravity");

		IB_Number(cx, cy, "gamma", usegamma, 1);
		cx += 10;
	}
	else  // 2D view
	{
		int mx = I_ROUND(grid.SnapX(edit.map_x));
		int my = I_ROUND(grid.SnapX(edit.map_y));

		mx = CLAMP(-32767, mx, 32767);
		my = CLAMP(-32767, my, 32767);

		IB_Number(cx, cy, "x", mx, 5);
		IB_Number(cx, cy, "y", my, 5);
		cx += 10;

		IB_Number(cx, cy, "gamma", usegamma, 1);
		cx += 10;
	}

	/* status message */

	IB_Flag(cx, cy, true, "|", "|");

	fl_color(INFO_TEXT_COL);

	fl_draw(status.c_str(), cx, cy);

	fl_pop_clip();
}


void UI_StatusBar::IB_Number(int& cx, int& cy, const char *label, int value, int size)
{
	char buffer[256];

	// negative size means we require a sign
	if (size < 0)
		sprintf(buffer, "%s:%-+*d ", label, -size + 1, value);
	else
		sprintf(buffer, "%s:%-*d ", label, size, value);

	fl_color(INFO_TEXT_COL);

	fl_draw(buffer, cx, cy);

	cx = cx + fl_width(buffer);
}


void UI_StatusBar::IB_Flag(int& cx, int& cy, bool value, const char *label_on, const char *label_off)
{
	const char *label = value ? label_on : label_off;

	fl_color(value ? INFO_TEXT_COL : INFO_DIM_COL);

	fl_draw(label, cx, cy);

	cx = cx + fl_width(label) + 20;
}


void UI_StatusBar::SetStatus(const char *str)
{
	status = str;

	redraw();
}


void Status_Set(const char *fmt, ...)
{
	if (! main_win)
		return;

	va_list arg_ptr;

	static char buffer[MSG_BUF_LEN];

	va_start(arg_ptr, fmt);
	vsnprintf(buffer, MSG_BUF_LEN-1, fmt, arg_ptr);
	va_end(arg_ptr);

	buffer[MSG_BUF_LEN-1] = 0;

	main_win->status_bar->SetStatus(buffer);
}


void Status_Clear()
{
	if (! main_win)
		return;

	main_win->status_bar->SetStatus("");
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
