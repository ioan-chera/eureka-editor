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
	Fl_Double_Window(350, 200, "Move Dialog")
{
    Fl_Box *title = new Fl_Box(15, 11, 305, 32, "Offset to move objects by:");
	title->labeltype(FL_ENGRAVED_LABEL);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	delta_x = new Fl_Value_Input(65, 55, 65, 25, "delta x:");
	delta_y = new Fl_Value_Input(220, 55, 65, 25, "delta y:");

	ok_but = new Fl_Button(225, 130, 95, 40, "OK");
//	ok_but->callback(...);

	cancel_but = new Fl_Button(95, 130, 95, 35, "Cancel");
//	cancel_but->callback(...);

	end();

	resizable(NULL);
}


UI_MoveDialog::~UI_MoveDialog()
{
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
