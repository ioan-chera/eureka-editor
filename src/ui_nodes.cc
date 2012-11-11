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
#include "ui_nodes.h"


void UI_NodeDialog::close_callback(Fl_Widget *w, void *data)
{
}

void UI_NodeDialog::cancel_callback(Fl_Widget *w, void *data)
{
}

void UI_NodeDialog::ok_callback(Fl_Widget *w, void *data)
{
}


//
//  Constructor
//
UI_NodeDialog::UI_NodeDialog() :
	    Fl_Double_Window(400, 400, "Building Nodes")
{
	end(); // cancel begin() in Fl_Group constructor

	size_range(w(), h());

	callback((Fl_Callback *) close_callback, this);

	color(FL_DARK3, FL_DARK3);


	int cy = 0;

	// FIXME !!!


	browser = new Fl_Browser(0, 0, w(), h() - 100);

	add(browser);


	resizable(browser);
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
	// FIXME : split lines

	browser->add(str);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
