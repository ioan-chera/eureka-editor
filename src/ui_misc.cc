//------------------------------------------------------------------------
//  Miscellaneous UI Dialogs
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

#include "main.h"
#include "ui_window.h"
#include "ui_misc.h"
#include "x_mirror.h"


UI_MoveDialog::UI_MoveDialog() :
	Fl_Double_Window(360, 180, "Move Objects"),
	want_close(false)
{
    Fl_Box *title = new Fl_Box(10, 11, w() - 20, 32, "Enter the offset to move objects:");
	title->labelsize(KF_fonth);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	delta_x = new Fl_Int_Input(95, 55, 65, 25,  "delta x:");
	delta_y = new Fl_Int_Input(240, 55, 65, 25, "delta y:");

	delta_x->value("0");
	delta_y->value("0");

	Fl_Group * grp = new Fl_Group(0, 110, w(), h() - 110);
	grp->box(FL_FLAT_BOX);
	grp->color(FL_DARK3, FL_DARK3);
	{
		cancel_but = new Fl_Button(30, 130, 95, 30, "Cancel");
		cancel_but->callback(close_callback, this);
	
		ok_but = new Fl_Button(245, 130, 95, 30, "Move");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		grp->end();
	}

	end();

	resizable(NULL);

	callback(close_callback, this);
}


UI_MoveDialog::~UI_MoveDialog()
{
}


void UI_MoveDialog::Run()
{
	set_modal();

	show();

	while (! WantClose())
	{
		Fl::wait(0.2);
	}
}


void UI_MoveDialog::close_callback(Fl_Widget *w, void *data)
{
	UI_MoveDialog * that = (UI_MoveDialog *)data;

	that->want_close = true;
}


void UI_MoveDialog::ok_callback(Fl_Widget *w, void *data)
{
	UI_MoveDialog * that = (UI_MoveDialog *)data;

	int delta_x = atoi(that->delta_x->value());
	int delta_y = atoi(that->delta_y->value());

	CMD_MoveObjects(delta_x, delta_y);

	that->want_close = true;
}


//------------------------------------------------------------------------


UI_ScaleDialog::UI_ScaleDialog() :
	Fl_Double_Window(360, 240, "Scale Objects"),
	want_close(false)
{
    Fl_Box *title = new Fl_Box(10, 11, w() - 20, 32, "Enter the scale amount:");
	title->labelsize(KF_fonth);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	scale_x = new Fl_Input(95, 55, 85, 25,  "scale x:");
	scale_y = new Fl_Input(240, 55, 85, 25, "scale y:");

	scale_x->value("1");
	scale_y->value("1");
	
	origin = new Fl_Choice(95, 90, 140, 25, "origin:");
	origin->add("Bottom Left|Bottom|Bottom Right|"
	            "Left|Centre|Right|"
	            "Top Left|Top|Top Right");
	origin->value(4);

	Fl_Group * grp = new Fl_Group(0, 130, w(), h() - 130);
	grp->box(FL_FLAT_BOX);
	grp->color(FL_DARK3, FL_DARK3);
	{
		Fl_Box * help = new Fl_Box(10, 145, w() - 20, 40);
		help->label("Scale Values:\n"
		            "    can be real: 0.25 or 3.7\n"
					"    or percentages: 25%\n"
					"    or fractions: 3 / 4");
		help->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);

		cancel_but = new Fl_Button(245, 150, 95, 30, "Cancel");
		cancel_but->callback(close_callback, this);
	
		ok_but = new Fl_Button(245, 195, 95, 30, "Scale");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		grp->end();
	}

	end();

	resizable(NULL);

	callback(close_callback, this);
}


UI_ScaleDialog::~UI_ScaleDialog()
{
}


void UI_ScaleDialog::Run()
{
	set_modal();

	show();

	while (! WantClose())
	{
		Fl::wait(0.2);
	}
}


static double ParseScaleStr(const char * s)
{
	// handle percentages
	if (strchr(s, '%'))
	{
		double val;

		if (sscanf(s, " %lf %% ", &val) != 1)
			return -1;

		return val;
	}

	// handle fractions
	if (strchr(s, '/'))
	{
		double num, denom;

		if (sscanf(s, " %lf / %lf ", &num, &denom) != 2)
			return -1;

		if (denom <= 0)
			return -1;
		
		return num / denom;
	}

	return atof(s);
}


void UI_ScaleDialog::close_callback(Fl_Widget *w, void *data)
{
	UI_ScaleDialog * that = (UI_ScaleDialog *)data;

	that->want_close = true;
}


void UI_ScaleDialog::ok_callback(Fl_Widget *w, void *data)
{
	UI_ScaleDialog * that = (UI_ScaleDialog *)data;

	double scale_x = ParseScaleStr(that->scale_x->value());
	double scale_y = ParseScaleStr(that->scale_y->value());

	if (scale_x <= 0 || scale_y <= 0)
	{
		Beep();
		return;
	}

	int pos_x = (that->origin->value() % 3) - 1;
	int pos_y = (that->origin->value() / 3) - 1;

	CMD_ScaleObjects3(scale_x, scale_y, pos_x, pos_y);

	that->want_close = true;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
