//------------------------------------------------------------------------
//  Node Building Window
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2012 Andrew Apted
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


static void node_dialog_Close(Fl_Widget *w, void *data)
{
}

static void node_dialog_Cancel(Fl_Widget *w, void *data)
{
}

static void node_dialog_OK(Fl_Widget *w, void *data)
{
}


//
//  Constructor
//
UI_NodeDialog::UI_NodeDialog() :
	    Fl_Double_Window(500, 300, EUREKA_TITLE)
{
	end(); // cancel begin() in Fl_Group constructor

	size_range(w(), h());

	callback((Fl_Callback *) node_dialog_Close);

	color(FL_DARK3, FL_DARK3);
//  color(WINDOW_BG, WINDOW_BG);

	int cy = 0;

	// FIXME !!!
}


//
//  Destructor
//
UI_NodeDialog::~UI_NodeDialog()
{ }


void UI_NodeDialog::SetStatus(const char *str)
{
	// FIXME
}


void UI_NodeDialog::SetProg(int perc)
{
	// FIXME
}


void UI_NodeDialog::Print(const char *str)
{
	// FIXME
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
