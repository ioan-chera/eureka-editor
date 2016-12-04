//------------------------------------------------------------------------
//  TEST (PLAY) THE MAP
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2016 Andrew Apted
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
#include "m_loadsave.h"
#include "w_wad.h"

#include "ui_window.h"


class UI_TestMapDialog : public UI_Escapable_Window
{
private:
	Fl_Choice *port;

	Fl_Output *exe_path;

	Fl_Button *ok_but;
	Fl_Button *cancel_but;

	bool result;
	bool want_close;

private:
	static void ok_callback(Fl_Widget *w, void *data)
	{
		UI_TestMapDialog * that = (UI_TestMapDialog *)data;

		that->result = true;
		that->want_close = true;
	}

	static void close_callback(Fl_Widget *w, void *data)
	{
		UI_TestMapDialog * that = (UI_TestMapDialog *)data;

		that->result = false;
		that->want_close = true;
	}

public:
	UI_TestMapDialog() :
		UI_Escapable_Window(460, 425, "Test Map Settings"),
		result(false), want_close(false)
	{
		int X = 0;
		int Y = 0;

		port = new Fl_Choice(90, Y+30, 200, 30, "Port: ");
		port->textsize(17);
		port->add("Boom|Edge|XDoom|Vanilla Doom|Vanilla Heretic|Vanilla HacX|Vanilla Strife");
		port->value(0);

		exe_path = new Fl_Output(90, 80, 225, 26, "Exe path: ");

		Fl_Button *load_but = new Fl_Button(380, 80, 60, 26, "Load");

		/* bottom buttons */

		Fl_Group * grp = new Fl_Group(0, h() - 70, w(), 70);
		grp->box(FL_FLAT_BOX);
		grp->color(WINDOW_BG, WINDOW_BG);
		{
			cancel_but = new Fl_Button(30, grp->y() + 20, 95, 30, "Cancel");
			cancel_but->callback(close_callback, this);

			ok_but = new Fl_Button(245, grp->y() + 20, 95, 30, "Play");
			ok_but->labelfont(FL_HELVETICA_BOLD);
			ok_but->callback(ok_callback, this);
			ok_but->shortcut(FL_Enter);
		}
		grp->end();

		end();

		resizable(NULL);

		callback(close_callback, this);
	}

	virtual ~UI_TestMapDialog()
	{ }

	// returns true if user clicked OK
	bool Run()
	{
		set_modal();
		show();

		while (! want_close)
			Fl::wait(0.2);

		return result;
	}
};


//------------------------------------------------------------------------


void CMD_TestMap()
{
	// FIXME : remove this restriction  (simply don't have a -file parameter for the edit_wad)
	if (! edit_wad)
	{
		DLG_Notify("Cannot test the map unless you are editing a PWAD.");
		return;
	}

	if (MadeChanges)
	{
		if (DLG_Confirm("Cancel|&Save",
		                "You have unsaved changes, do you want to save them now "
						"and build the nodes?") <= 0)
		{
			return;
		}

		if (! M_SaveMap())
			return;
	}


	UI_TestMapDialog *dialog = new UI_TestMapDialog();

	bool ok = dialog->Run();

	if (! ok)
	{
		delete dialog;
		return;
	}



	// FIXME: figure out the proper directory to cd into
	//        (and ensure that it exists)

	// TODO : remember current dir, reset afterwards

	// FIXME : check if this worked
	FileChangeDir("/home/aapted/oblige");


	char cmd_buffer[FL_PATH_MAX * 2];

	// FIXME: use fl_filename_absolute() to get absolute paths

	// add each wad in the MASTER directory


	// FIXME : handle DOOM1/ULTDOOM style warp option

	snprintf(cmd_buffer, sizeof(cmd_buffer),
	         "./boomPR -iwad %s -file %s -warp %s",
			 game_wad->PathName(),
			 edit_wad->PathName(),
			 Level_name);

	LogPrintf("Playing map using the following command:\n");
	LogPrintf("  %s\n", cmd_buffer);

	Status_Set("TESTING MAP...");

	main_win->redraw();
	Fl::wait(0.1);
	Fl::wait(0.1);

	int status = system(cmd_buffer);

	if (status == 0)
		Status_Set("Result: OK");
	else
		Status_Set("Result code: %d\n", status);


	delete dialog;

	main_win->redraw();

	Fl::wait(0.1);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
