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


#define NODE_PROGRESS_COLOR  fl_color_cube(2,6,2)


//
//  Callbacks
//

void UI_NodeDialog::close_callback(Fl_Widget *w, void *data)
{
	UI_NodeDialog * that = (UI_NodeDialog *)data;

	that->want_close = true;

	if (! that->finished)
		that->want_cancel = true;
}


void UI_NodeDialog::button_callback(Fl_Widget *w, void *data)
{
	UI_NodeDialog * that = (UI_NodeDialog *)data;

	if (that->finished)
		that->want_close = true;
	else
		that->want_cancel = true;
}


//
//  Constructor
//
UI_NodeDialog::UI_NodeDialog() :
	    Fl_Double_Window(400, 400, "Building Nodes"),
		cur_prog(-1),
		finished(false),
		want_cancel(false),
		want_close(false)
{
	end(); // cancel begin() in Fl_Group constructor

	size_range(w(), h());

	callback((Fl_Callback *) close_callback, this);

	color(WINDOW_BG, WINDOW_BG);


	browser = new Fl_Browser(0, 0, w(), h() - 100);

	add(browser);


	Fl_Box * ptext = new Fl_Box(FL_NO_BOX, 10, h() - 80, 80, 20, "Progress:");

	add(ptext);


	progress = new Fl_Progress(100, h() - 80, w() - 120, 20);
	progress->align(FL_ALIGN_INSIDE);
	progress->box(FL_FLAT_BOX);
	progress->color(FL_LIGHT2, NODE_PROGRESS_COLOR);

	progress->minimum(0.0);
	progress->maximum(100.0);
	progress->value(0.0);

	add(progress);


	button = new Fl_Button(w() - 100, h() - 46, 80, 30, "Cancel");
	button->callback(button_callback, this);

	add(button);


	resizable(browser);
}


//
//  Destructor
//
UI_NodeDialog::~UI_NodeDialog()
{ }


int UI_NodeDialog::handle(int event)
{
	if (event == FL_KEYDOWN && Fl::event_key() == FL_Escape)
	{
		if (finished)
			want_close = true;
		else
			want_cancel = true;
		return 1;
	}

	return Fl_Double_Window::handle(event);
}


void UI_NodeDialog::SetStatus(const char *str)
{
	/* not used */
}


void UI_NodeDialog::SetProg(int perc)
{
	if (perc == cur_prog)
		return;

	cur_prog = perc;

	sprintf(prog_label, "%d%%", perc);

	progress->value(perc);
	progress->label(prog_label);

	Fl::check();
}


void UI_NodeDialog::Print(const char *str)
{
	// split lines

	static char buffer[256];

	snprintf(buffer, sizeof(buffer), "%s", str);
	buffer[sizeof(buffer) - 1] = 0;

	char * pos = buffer;
	char * next;

	while (pos && *pos)
	{
		next = strchr(pos, '\n');

		if (next) *next++ = 0;

		browser->add(pos);

		pos = next;
	}

	browser->bottomline(browser->size());

	Fl::check();
}


void UI_NodeDialog::Finish_OK()
{
	finished = true;

	button->label("Close");

	progress->value(100);
	progress->label("Success");
	progress->color(FL_BLUE, FL_BLUE);
}

void UI_NodeDialog::Finish_Cancel()
{
	finished = true;

	button->label("Close");

	progress->value(0);
	progress->label("Cancelled");
	progress->color(FL_RED, FL_RED);
}

void UI_NodeDialog::Finish_Error()
{
	finished = true;

	button->label("Close");

	progress->value(100);
	progress->label("ERROR");
	progress->color(FL_RED, FL_RED);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
