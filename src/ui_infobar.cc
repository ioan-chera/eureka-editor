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
	mode->add("Things|Linedefs|Sectors|Vertices");
	mode->value(0);
	mode->callback(mode_callback, this);
	mode->labelsize(KF_fonth);
	mode->textsize(KF_fonth - 2);

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

	if (!str || !str[0])
		status->tooltip(NULL);
	else
		status->copy_tooltip(str);
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
		case 0: /* Things   */ mode->color(THING_MODE_COL);  break;
		case 1: /* Linedefs */ mode->color(LINE_MODE_COL);   break;
		case 2: /* Sectors  */ mode->color(SECTOR_MODE_COL); break;
		case 3: /* Vertices */ mode->color(VERTEX_MODE_COL); break;
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


//------------------------------------------------------------------------


#define INFO_TEXT_COL	fl_rgb_color(192, 192, 192)
#define INFO_DIM_COL	fl_rgb_color(128, 128, 128)


UI_3DInfoBar::UI_3DInfoBar(int X, int Y, int W, int H, const char *label) :
    Fl_Widget(X, Y, W, H, label)
{
	box(FL_NO_BOX);
}

UI_3DInfoBar::~UI_3DInfoBar()
{
}


int UI_3DInfoBar::handle(int event)
{
	// this never handles any events
	return 0;
}

void UI_3DInfoBar::draw()
{
	if (r_view.SelectEmpty())
		fl_color(FL_BLACK);
	else
		fl_color(fl_rgb_color(96,48,0));

	fl_rectf(x(), y(), w(), h());

	fl_push_clip(x(), y(), w(), h());

	fl_font(FL_COURIER, 16);

	int cx = x() + 10;
	int cy = y() + 20;

	int ang = I_ROUND(r_view.angle * 180 / M_PI);
	if (ang < 0) ang += 360;

	IB_Number(cx, cy, "angle", ang, 3);
	cx += 8;

	IB_Number(cx, cy, "z", I_ROUND(r_view.z) - game_info.view_height, 4);

	IB_Number(cx, cy, "gamma", usegamma, 1);
	cx += 10;

	IB_Flag(cx, cy, r_view.gravity, "GRAVITY", "gravity");

	IB_Flag(cx, cy, true, "|", "|");

	IB_Highlight(cx, cy);

	fl_pop_clip();
}


void UI_3DInfoBar::IB_Number(int& cx, int& cy, const char *label, int value, int size)
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


void UI_3DInfoBar::IB_Flag(int& cx, int& cy, bool value, const char *label_on, const char *label_off)
{
	const char *label = value ? label_on : label_off;

	fl_color(value ? INFO_TEXT_COL : INFO_DIM_COL);

	fl_draw(label, cx, cy);

	cx = cx + fl_width(label) + 20;
}


void UI_3DInfoBar::IB_Highlight(int& cx, int& cy)
{
	char buffer[256];

	if (! r_view.hl.valid())
	{
		fl_color(INFO_DIM_COL);

		strcpy(buffer, "no highlight");
	}
	else
	{
		fl_color(INFO_TEXT_COL);

		if (r_view.hl.isThing())
		{
			const Thing *th = Things[r_view.hl.num];
			const thingtype_t *info = M_GetThingType(th->type);

			snprintf(buffer, sizeof(buffer), "thing #%d  %s",
					 r_view.hl.num, info->desc);

		}
		else if (r_view.hl.isSector())
		{
			int tex = r_view.GrabTextureFromObject(r_view.hl);

			snprintf(buffer, sizeof(buffer), " sect #%d  %-8s",
					 r_view.hl.num,
					 (tex < 0) ? "??????" : BA_GetString(tex));
		}
		else
		{
			int tex = r_view.GrabTextureFromObject(r_view.hl);

			snprintf(buffer, sizeof(buffer), " line #%d  %-8s",
					 r_view.hl.num,
					 (tex < 0) ? "??????" : BA_GetString(tex));
		}
	}

	fl_draw(buffer, cx, cy);

	cx = cx + fl_width(buffer);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
