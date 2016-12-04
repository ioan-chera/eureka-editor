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

#include "m_files.h"
#include "m_loadsave.h"
#include "w_wad.h"

#include "ui_window.h"


class UI_PortPathDialog : public UI_Escapable_Window
{
private:
	Fl_Output *exe_path;

	Fl_Button *ok_but;
	Fl_Button *cancel_but;

	bool result;
	bool want_close;

private:
	static void ok_callback(Fl_Widget *w, void *data)
	{
		UI_PortPathDialog * that = (UI_PortPathDialog *)data;

		that->result = true;
		that->want_close = true;
	}

	static void close_callback(Fl_Widget *w, void *data)
	{
		UI_PortPathDialog * that = (UI_PortPathDialog *)data;

		that->result = false;
		that->want_close = true;
	}

public:
	UI_PortPathDialog() :
		UI_Escapable_Window(560, 250, "Port Settings"),
		result(false), want_close(false)
	{
		// FIXME : name of port in this message

		Fl_Box *header = new Fl_Box(FL_NO_BOX, 20, 20, w() - 40, 30,
		           "Setting up location of the executable (EXE) for Vanilla Doom2.");
		header->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

		header = new Fl_Box(FL_NO_BOX, 20, 55, w() - 40, 30,
		           "This is only needed for the Test Map command.");
		header->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);


		exe_path = new Fl_Output(98, 100, w()-200, 26, "Exe path: ");

		Fl_Button *find_but = new Fl_Button(w()-80, 100, 60, 26, "Find");

		/* bottom buttons */

		Fl_Group * grp = new Fl_Group(0, h() - 60, w(), 70);
		grp->box(FL_FLAT_BOX);
		grp->color(WINDOW_BG, WINDOW_BG);
		{
			cancel_but = new Fl_Button(w() - 260, h() - 45, 95, 30, "Cancel");
			cancel_but->callback(close_callback, this);

			ok_but = new Fl_Button(w() - 120, h() - 45, 95, 30, "OK");
			ok_but->labelfont(FL_HELVETICA_BOLD);
			ok_but->callback(ok_callback, this);
			ok_but->shortcut(FL_Enter);
		}
		grp->end();

		end();

		resizable(NULL);

		callback(close_callback, this);
	}

	virtual ~UI_PortPathDialog()
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


bool M_PortSetupDialog(const char *port, const char *game)
{
	UI_PortPathDialog *dialog = new UI_PortPathDialog();

	bool ok = dialog->Run();

	delete dialog;
	return ok;
}


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


	port_path_info_t *info = M_QueryPortPath(Port_name);

	if (! (info && M_IsPortPathValid(info)))
	{
		if (! M_PortSetupDialog(Port_name, Game_name))
			return;

		info = M_QueryPortPath(Port_name);
	}

	// this generally cannot happen, but we check anyway...
	if (! (info && M_IsPortPathValid(info)))
	{
		fl_beep();
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

	main_win->redraw();
	Fl::wait(0.1);
	Fl::wait(0.1);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
