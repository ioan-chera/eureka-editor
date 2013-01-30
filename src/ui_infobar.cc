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

#include "main.h"
#include "ui_window.h"

#include "editloop.h"
#include "r_grid.h"


#define SNAP_COLOR  fl_rgb_color(255, 128, 128)
#define FREE_COLOR  fl_rgb_color(128, 255, 128)


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


	mode = new Fl_Choice(X+58, Y, 88, H, "Mode:");
	mode->align(FL_ALIGN_LEFT);
	mode->add("Things|Linedefs|Sectors|Vertices|RTS");
	mode->value(0);
	mode->callback(mode_callback, this);
	mode->labelsize(KF_fonth);
	mode->textsize(KF_fonth);

	X = mode->x() + mode->w() + 10;


	scale = new Fl_Choice(X+52, Y, 78, H, "Scale:");
	scale->align(FL_ALIGN_LEFT);
	scale->add(Grid_State_c::scale_options());
	scale->value(8);
	scale->callback(scale_callback, this);
	scale->labelsize(KF_fonth);
	scale->textsize(KF_fonth);

	X = scale->x() + scale->w() + 10;


	grid_size = new Fl_Choice(X+44, Y, 72, H, "Grid:");

	grid_size->align(FL_ALIGN_LEFT);
	grid_size->add(Grid_State_c::grid_options());
	grid_size->value(1);
	grid_size->callback(grid_callback, this);
	grid_size->labelsize(KF_fonth);
	grid_size->textsize(KF_fonth);

	X = grid_size->x() + grid_size->w() + 10;


	grid_snap = new Fl_Toggle_Button(X+4, Y, 72, H);
	grid_snap->value(grid.snap ? 1 : 0);
	grid_snap->color(FREE_COLOR);
	grid_snap->selection_color(SNAP_COLOR);
	grid_snap->callback(snap_callback, this);
	grid_snap->labelsize(KF_fonth);

	UpdateSnapText();

	X = grid_snap->x() + grid_snap->w() + 4;


	mouse_x = new Fl_Output(X+28,       Y, 64, H, "x");
	mouse_y = new Fl_Output(X+28+72+10, Y, 64, H, "y");

	mouse_x->align(FL_ALIGN_LEFT);
	mouse_y->align(FL_ALIGN_LEFT);

	mouse_x->labelsize(KF_fonth); mouse_y->labelsize(KF_fonth);
	mouse_x->textsize(KF_fonth);  mouse_y->textsize(KF_fonth);

	X = mouse_y->x() + mouse_y->w() + 10;


	Fl_Box *div = new Fl_Box(FL_FLAT_BOX, X, Y-4, 3, H+4, NULL);
	div->color(WINDOW_BG, WINDOW_BG);

	X += 6;


	status = new Fl_Box(FL_FLAT_BOX, X, Y-4, W - 4 - X, H+4, "Ready");
	status->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_CLIP);
	status->labelfont(FL_HELVETICA_BOLD);


	// ---- resizable ----
 
 	resizable(status);

	end();
}

//
// UI_InfoBar Destructor
//
UI_InfoBar::~UI_InfoBar()
{
}

int UI_InfoBar::handle(int event)
{
	return Fl_Group::handle(event);
}


void UI_InfoBar::mode_callback(Fl_Widget *w, void *data)
{
	Fl_Choice *mode = (Fl_Choice *)w;

	static const char *mode_keys = "tlsvr";

	Editor_ChangeMode(mode_keys[mode->value()]);
}


void UI_InfoBar::scale_callback(Fl_Widget *w, void *data)
{
	Fl_Choice *choice = (Fl_Choice *)w;

	grid.ScaleFromWidget(choice->value());
}


void UI_InfoBar::grid_callback(Fl_Widget *w, void *data)
{
	Fl_Choice *choice = (Fl_Choice *)w;

	grid.StepFromWidget(choice->value());
}


void UI_InfoBar::snap_callback(Fl_Widget *w, void *data)
{
	Fl_Toggle_Button *grid_snap = (Fl_Toggle_Button *)w;

	UI_InfoBar *bar = (UI_InfoBar *)data;

	// update editor state
	grid.snap = grid_snap->value() ? true : false;

	bar->UpdateSnapText();

	UpdateHighlight();
}



//------------------------------------------------------------------------

void UI_InfoBar::NewEditMode(char edit_mode)
{
	switch (edit_mode)
	{
		case 't': mode->value(0); break;
		case 'l': mode->value(1); break;
		case 's': mode->value(2); break;
		case 'v': mode->value(3); break;
		case 'r': mode->value(4); break;

		default: break;
	}

	UpdateModeColor();
}


void UI_InfoBar::SetMouse(double mx, double my)
{
	if (mx < -32767.0 || mx > 32767.0 ||
		my < -32767.0 || my > 32767.0)
	{
		mouse_x->value("off map");
		mouse_y->value("off map");

		return;
	}

	char x_buffer[60];
	char y_buffer[60];

	sprintf(x_buffer, "%d", I_ROUND(mx));
	sprintf(y_buffer, "%d", I_ROUND(my));

	mouse_x->value(x_buffer);
	mouse_y->value(y_buffer);
}


void UI_InfoBar::SetStatus(const char *str)
{
	status->copy_label(str);
}


void UI_InfoBar::SetScale(int i)
{
	scale->value(i);
}

void UI_InfoBar::SetGrid(int i)
{
	grid_size->value(i);
}


#if 0

void UI_InfoBar::SetZoom(float zoom_mul)
{
	char buffer[60];

	///  if (0.99 < zoom_mul && zoom_mul < 1.01)
	///  {
	///    grid_size->value("1:1");
	///    return;
	///  }

	if (zoom_mul < 0.99)
	{
		sprintf(buffer, "/ %1.3f", 1.0/zoom_mul);
	}
	else // zoom_mul > 1
	{
		sprintf(buffer, "x %1.3f", zoom_mul);
	}

	grid_size->value(buffer);
}


void UI_InfoBar::SetNodeIndex(int index)
{
#if 0
	char buffer[60];

	sprintf(buffer, "%d", index);

	ns_index->label("Node #");
	ns_index->value(buffer);

	redraw();
#endif
}


void UI_InfoBar::SetWhich(int index, int total)
{
	if (index < 0)
	{
		which->label(INDEX_NONE_STR);
	}
	else
	{
		char buffer[200];
		sprintf(buffer, "Index: #%d of %d", index, total);

		which->copy_label(buffer);
	}

	redraw();
}
#endif


void UI_InfoBar::UpdateSnap()
{
   grid_snap->value(grid.snap ? 1 : 0);

   UpdateSnapText();
}


void UI_InfoBar::UpdateModeColor()
{
	switch (mode->value())
	{
		case 0: /* Things   */ mode->color(FL_MAGENTA); break;
		case 1: /* Linedefs */ mode->color(fl_rgb_color(0,128,255)); break;
		case 2: /* Sectors  */ mode->color(FL_YELLOW); break;
		case 3: /* Vertices */ mode->color(fl_rgb_color(0,192,96));  break;
		case 4: /* RTS      */ mode->color(FL_RED); break;
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

	main_win->info_bar->SetStatus(buffer);
}


void Status_Clear()
{
	if (! main_win)
		return;

	main_win->info_bar->SetStatus("");
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
