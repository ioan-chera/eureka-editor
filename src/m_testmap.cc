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


static const char * QueryName(const char *port = Port_name, const char *game = Game_name)
{
	SYS_ASSERT(port);

	static char buffer[256];

	if (y_stricmp(port, "vanilla") == 0)
	{
		if (! game)
			game = "doom2";

		snprintf(buffer, sizeof(buffer), "vanilla_%s\n", game);
		return buffer;
	}

	return port;
}


class UI_PortPathDialog : public UI_Escapable_Window
{
public:
	Fl_Output *exe_display;

	Fl_Button *ok_but;
	Fl_Button *cancel_but;

	// the chosen EXE name, or NULL if cancelled
	char *exe_name;

	bool want_close;

private:
	void SetEXE(const char *newbie)
	{
		StringFree(exe_name);

		exe_name = StringDup(newbie);
	}

	static void ok_callback(Fl_Widget *w, void *data)
	{
		UI_PortPathDialog * that = (UI_PortPathDialog *)data;

		that->want_close = true;
	}

	static void close_callback(Fl_Widget *w, void *data)
	{
		UI_PortPathDialog * that = (UI_PortPathDialog *)data;

		that->SetEXE(NULL);

		that->want_close = true;
	}

	static void find_callback(Fl_Widget *w, void *data)
	{
		UI_PortPathDialog * that = (UI_PortPathDialog *)data;

		Fl_Native_File_Chooser chooser;

		chooser.title("Pick the executable file");
		chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
		chooser.filter("Executables\t*.exe");

		// FIXME : if we have an exe_filename already, and folder exists, go there
		chooser.directory(Main_FileOpFolder());

		switch (chooser.show())
		{
			case -1:  // error
				DLG_Notify("Unable to use that exe:\n\n%s", chooser.errmsg());
				return;

			case 1:  // cancelled
				return;

			default:
				break;  // OK
		}

		// we assume the chosen file exists

		that->SetEXE(chooser.filename());
	}

public:
	UI_PortPathDialog(const char *port_name) :
		UI_Escapable_Window(560, 250, "Port Settings"),
		exe_name(NULL), want_close(false)
	{
		char message_buf[256];

		sprintf(message_buf, "Setting up location of the executable (EXE) for %s.", port_name);

		Fl_Box *header = new Fl_Box(FL_NO_BOX, 20, 20, w() - 40, 30, "");
		header->copy_label(message_buf);
		header->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

		header = new Fl_Box(FL_NO_BOX, 20, 55, w() - 40, 30,
		           "This is only needed for the Test Map command.");
		header->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);


		exe_display = new Fl_Output(98, 100, w()-200, 26, "Exe path: ");

		Fl_Button *find_but = new Fl_Button(w()-80, 100, 60, 26, "Find");
		find_but->callback((Fl_Callback*)find_callback, this);

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

		return exe_name ? true : false;
	}
};


bool M_PortSetupDialog(const char *port, const char *game)
{
	char name_buf[256];

	if (y_stricmp(port, "vanilla") == 0)
	{
		snprintf(name_buf, sizeof(name_buf), "Vanilla %s\n", game);
		name_buf[8] = toupper(name_buf[8]);
	}
	else if (y_stricmp(port, "mbf") == 0)	// temp hack
	{
		strcpy(name_buf, "MBF");
	}
	else
	{
		snprintf(name_buf, sizeof(name_buf), "%s", port);
		name_buf[0] = toupper(name_buf[0]);
	}

	UI_PortPathDialog *dialog = new UI_PortPathDialog(name_buf);

	bool ok = dialog->Run();

	if (ok)
	{
		// persist the new port settings
		port_path_info_t *info = M_QueryPortPath(QueryName(port, game), true /* create_it */);

		snprintf(info->exe_filename, sizeof(info->exe_filename), "%s", dialog->exe_name);

		M_SaveRecent();
	}

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


	port_path_info_t *info = M_QueryPortPath(QueryName());

	if (! (info && M_IsPortPathValid(info)))
	{
		if (! M_PortSetupDialog(Port_name, Game_name))
			return;

		info = M_QueryPortPath(QueryName());
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
