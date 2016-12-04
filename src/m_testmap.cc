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
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
