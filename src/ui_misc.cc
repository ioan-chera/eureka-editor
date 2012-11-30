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


//
//  Callbacks
//
/* TODO
void UI_NodeDialog::close_callback(Fl_Widget *w, void *data)
{
	UI_NodeDialog * that = (UI_NodeDialog *)data;

	that->want_close = true;

	if (! that->finished)
		that->want_cancel = true;
}


void UI_NodeDialog::ok_callback(Fl_Widget *w, void *data)
{
	UI_NodeDialog * that = (UI_NodeDialog *)data;

	if (that->finished)
		that->want_close = true;
	else
		that->want_cancel = true;
}
*/


UI_MoveDialog::UI_MoveDialog() :
	Fl_Double_Window(360, 180, "Move Dialog")
{
    Fl_Box *title = new Fl_Box(10, 11, w() - 20, 32, "Enter the offset to move objects:");
	title->labelsize(KF_fonth);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	delta_x = new Fl_Value_Input(95, 55, 65, 25,  "delta x:");
	delta_y = new Fl_Value_Input(240, 55, 65, 25, "delta y:");

	Fl_Group * grp = new Fl_Group(0, 110, w(), h() - 110);
	grp->box(FL_FLAT_BOX);
	grp->color(FL_DARK3, FL_DARK3);
	{
		cancel_but = new Fl_Button(30, 130, 95, 30, "Cancel");
	//	cancel_but->callback(...);
	
		ok_but = new Fl_Button(245, 130, 95, 30, "Move");
		ok_but->labelfont(FL_HELVETICA_BOLD);
	//	ok_but->callback(...);

		grp->end();
	}

	end();

	resizable(NULL);
}


UI_MoveDialog::~UI_MoveDialog()
{
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
