//------------------------------------------------------------------------
//  BUILDING NODES / PLAY THE MAP
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2016 Andrew Apted
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
#include "levels.h"
#include "e_loadsave.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_nodes.h"

#include "bsp.h"


// config items
bool glbsp_fast    = false;
bool glbsp_verbose = false;
bool glbsp_warn    = false;


static ajbsp::nodebuildinfo_t nb_info;
static volatile ajbsp::nodebuildcomms_t nb_comms;

static int display_mode = ajbsp::DIS_INVALID;
static int progress_limit;

static char message_buf[MSG_BUF_LEN];

static UI_NodeDialog * dialog;


static const char *glbsp_ErrorString(ajbsp::build_result_e ret)
{
	switch (ret)
	{
		case ajbsp::BUILD_OK: return "OK";

		 // the arguments were bad/inconsistent.
		case ajbsp::BUILD_BadArgs: return "Bad Arguments";

		// the info was bad/inconsistent, but has been fixed
		case ajbsp::BUILD_BadInfoFixed: return "Bad Args (fixed)";

		// file errors
		case ajbsp::BUILD_ReadError:  return "Read Error";
		case ajbsp::BUILD_WriteError: return "Write Error";

		// building was cancelled
		case ajbsp::BUILD_Cancelled: return "Cancelled by User";

		// an unknown error occurred (this is the catch-all value)
		case ajbsp::BUILD_Unknown:

		default: return "Unknown Error";
	}
}


void GB_PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsnprintf(message_buf, MSG_BUF_LEN, str, args);
	va_end(args);

	message_buf[MSG_BUF_LEN-1] = 0;

	dialog->Print(message_buf);

	LogPrintf("BSP: %s", message_buf);
}

void GB_DisplayTicker(void)
{
	if (dialog->WantCancel())
	{
		nb_comms.cancelled = true;
	}
}

bool GB_DisplayOpen(ajbsp::displaytype_e type)
{
	display_mode = type;
	return true;
}

void GB_DisplaySetTitle(const char *str)
{
	/* does nothing */
}

void GB_DisplaySetBarText(int barnum, const char *str)
{
	if (display_mode == ajbsp::DIS_BUILDPROGRESS && barnum == 1)
	{
		dialog->SetStatus(str);

/* UNUSED:
		// extract map name
		const char * map_name = str + strlen(str);

		if (map_name > str)
			map_name--;

		// handle the (Hexen) suffix
		if (*map_name == ')')
		{
			while (map_name > str && map_name[1] != '(')
				map_name--;

			while (map_name > str && isspace(*map_name))
				map_name--;

			//  map_name[1] = 0;
		}

		while (map_name > str && !isspace(*map_name))
			map_name--;
*/
	}
}

void GB_DisplaySetBarLimit(int barnum, int limit)
{
	if (display_mode == ajbsp::DIS_BUILDPROGRESS && barnum == 2)
	{
		progress_limit = MAX(1, limit);
	}
}

void GB_DisplaySetBar(int barnum, int count)
{
	if (display_mode == ajbsp::DIS_BUILDPROGRESS && barnum == 2)
	{
		int perc = count * 100.0 / progress_limit;

		dialog->SetProg(perc);
	}
}


void GB_DisplayClose(void)
{
	/* does nothing */
}


static bool DM_BuildNodes(const char *in_name, const char *out_name)
{
	LogPrintf("\n");

	display_mode = ajbsp::DIS_INVALID;

	memcpy(&nb_info,  &ajbsp::default_buildinfo,  sizeof(ajbsp::default_buildinfo));
	memcpy((void*)&nb_comms, &ajbsp::default_buildcomms, sizeof(ajbsp::nodebuildcomms_t));

	nb_info.input_file  = StringDup(in_name);
	nb_info.output_file = StringDup(out_name);

	nb_info.fast          = glbsp_fast ? true : false;
	nb_info.quiet         = glbsp_verbose ? false : true;
	nb_info.mini_warnings = glbsp_warn ? true : false;

	nb_info.pack_sides = false;
	nb_info.force_normal = true;

	ajbsp::build_result_e  ret;

	ret = ajbsp::CheckInfo(&nb_info, &nb_comms);

	if (ret != ajbsp::BUILD_OK)
	{
		// check info failure (unlikely to happen)
		GB_PrintMsg("\n");
		GB_PrintMsg("Param Check FAILED: %s\n", glbsp_ErrorString(ret));
		GB_PrintMsg("Reason: %s\n\n", nb_comms.message);
		return false;
	}

	ret = ajbsp::BuildNodes(&nb_info, &nb_comms);

	if (ret == ajbsp::BUILD_Cancelled)
	{
		GB_PrintMsg("\n");
		GB_PrintMsg("Building CANCELLED.\n\n");
		return false;
	}

	if (ret != ajbsp::BUILD_OK)
	{
		// build nodes failed
		GB_PrintMsg("\n");
		GB_PrintMsg("Building FAILED: %s\n", glbsp_ErrorString(ret));
		GB_PrintMsg("Reason: %s\n\n", nb_comms.message);
		return false;
	}

	return true;
}


bool CMD_BuildNodes()
{
	if (MadeChanges)
	{
		if (DLG_Confirm("Cancel|&Save",
		                "You have unsaved changes, do you want to save them now "
						"and then build the nodes?") <= 0)
		{
			return false;
		}

		if (! CMD_SaveMap())
			return false;
	}

	if (! edit_wad)
	{
		DLG_Notify("Cannot build nodes unless you are editing a PWAD.");
		return false;
	}

	if (edit_wad->IsReadOnly())
	{
		DLG_Notify("Cannot build nodes on a read-only file.");
		return false;
	}

	SYS_ASSERT(edit_wad);


	const char *old_name = StringDup(edit_wad->PathName());
	const char *new_name = ReplaceExtension(old_name, "new");

	if (MatchExtension(old_name, "new"))
	{
		DLG_Notify("Cannot build nodes on a pwad with .NEW extension.");
		return false;
	}


	dialog = new UI_NodeDialog();

	dialog->set_modal();
	dialog->show();

	Fl::check();


	bool was_ok = DM_BuildNodes(old_name, new_name);

	if (was_ok)
	{
		MasterDir_Remove(edit_wad);

		delete edit_wad;
		edit_wad = NULL;
		Pwad_name = NULL;

		// delete the old file, rename the new file
		if (! FileDelete(old_name))
		{
#if 0
fprintf(stderr, "DELETE ERROR: %s\n", strerror(errno));
fprintf(stderr, "old_name : %s\n", old_name);
#endif
			FatalError("Unable to replace the pwad with the new version\n"
			           "containing the freshly built nodes, as the original\n"
					   "could not be deleted.\n");
		}

		if (! FileRename(new_name, old_name))
		{
#if 0
fprintf(stderr, "RENAME ERROR: %s\n", strerror(errno));
fprintf(stderr, "old_name : %s\n", old_name);
fprintf(stderr, "new_name : %s\n", new_name);
#endif
			FatalError("Unable to replace the pwad with the new version\n"
			           "containing the freshly built nodes, as a problem\n"
					   "occurred trying to rename the new file.\n"
					   "\n"
					   "Your wad has been left with the .NEW extension.\n");
		}

		GB_PrintMsg("\n");
		GB_PrintMsg("Replaced the old file with the new file.\n");
	}
	else
	{
		FileDelete(new_name);
	}


	if (was_ok)
	{
		dialog->Finish_OK();
	}
	else if (nb_comms.cancelled)
	{
		dialog->Finish_Cancel();

		Status_Set("Cancelled");
	}
	else
	{
		dialog->Finish_Error();

		Status_Set("Error building nodes");
	}

	while (! dialog->WantClose())
	{
		Fl::wait(0.2);
	}

	delete dialog;
	dialog = NULL;

	if (was_ok)
	{
		// re-open the PWAD

		LogPrintf("Re-opening the PWAD...\n");

		edit_wad = Wad_file::Open(old_name, 'a');
		Pwad_name = old_name;

		if (! edit_wad)
			FatalError("Unable to re-open the PWAD.\n");

		MasterDir_Add(edit_wad);

		LogPrintf("Re-opening the map (%s)\n", Level_name);

		LoadLevel(edit_wad, Level_name);

		Status_Set("Built nodes OK");
	}

	return was_ok;
}


//------------------------------------------------------------------------


void CMD_TestMap()
{
	if (MadeChanges)
	{
		if (DLG_Confirm("Cancel|&Save",
		                "You have unsaved changes, do you want to save them now "
						"and build the nodes?") <= 0)
		{
			return;
		}

		if (! CMD_BuildNodes())
			return;
	}

	if (! edit_wad)
	{
		DLG_Notify("Cannot test the map unless you are editing a PWAD.");
		return;
	}

	
	// FIXME:
	// if (missing nodes)
	//    DLG_Confirm(  "build the nodes now?")


	char cmd_buffer[FL_PATH_MAX * 2];

	// FIXME: use fl_filename_absolute() to get absolute paths

	snprintf(cmd_buffer, sizeof(cmd_buffer),
	         "cd /home/aapted/doom; ./edge135 -iwad %s -file %s -warp %s",
			 game_wad->PathName(),
			 edit_wad->PathName(),
			 Level_name);

	LogPrintf("Playing map using the following command:\n");
	LogPrintf("  %s\n", cmd_buffer);

	system(cmd_buffer);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
