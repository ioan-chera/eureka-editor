//------------------------------------------------------------------------
//  File-related dialogs
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
#include "ui_file.h"


UI_ChooseMap::UI_ChooseMap() :
	Fl_Double_Window(360, 180, "Choose Map"),
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
	grp->color(WINDOW_BG, WINDOW_BG);
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


UI_ChooseMap::~UI_ChooseMap()
{
}


void UI_ChooseMap::Run()
{
	set_modal();

	show();

	while (! want_close)
	{
		Fl::wait(0.2);
	}
}


void UI_ChooseMap::close_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->want_close = true;
}


void UI_ChooseMap::ok_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->want_close = true;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
