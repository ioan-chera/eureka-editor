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
bool bsp_fast    = false;
bool bsp_verbose = false;
bool bsp_warn    = false;


static ajbsp::nodebuildinfo_t * nb_info;

static int display_mode = ajbsp::DIS_INVALID;
static int progress_limit;

static char message_buf[MSG_BUF_LEN];

static UI_NodeDialog * dialog;


static const char *build_ErrorString(ajbsp::build_result_e ret)
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
		nb_info->cancelled = true;
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


static ajbsp::build_result_e CheckInfo(ajbsp::nodebuildinfo_t *info)
{
	if (!info->input_file || info->input_file[0] == 0)
	{
		ajbsp::SetErrorMsg("Missing input filename !");
		return ajbsp::BUILD_BadArgs;
	}

	if (!info->output_file || info->output_file[0] == 0)
	{
		ajbsp::SetErrorMsg("Missing output filename !");
		return ajbsp::BUILD_BadArgs;
	}

	if (MatchExtension(info->input_file, "gwa"))
	{
		ajbsp::SetErrorMsg("Input file cannot be GWA (contains nothing to build)");
		return ajbsp::BUILD_BadArgs;
	}

	if (MatchExtension(info->output_file, "gwa"))
	{
		ajbsp::SetErrorMsg("Output file cannot be GWA");
		return ajbsp::BUILD_BadArgs;
	}

	if (y_stricmp(info->input_file, info->output_file) == 0)
	{
		ajbsp::SetErrorMsg("Input and Outfile file are the same!");
		return ajbsp::BUILD_BadArgs;
	}

	if (info->factor < 1 || info->factor > 32)
	{
		info->factor = DEFAULT_FACTOR;
//???		SetErrorMsg("Bad factor value !");
//???		return BUILD_BadInfoFixed;
	}

	return ajbsp::BUILD_OK;
}


static ajbsp::build_result_e BuildAllNodes(ajbsp::nodebuildinfo_t *info)
{
	char *file_msg;

	ajbsp::build_result_e ret;

	ret = CheckInfo(info);

	if (ret != ajbsp::BUILD_OK)
		return ret;

	info->total_big_warn = 0;
	info->total_small_warn = 0;

	// clear cancelled flag
	info->cancelled = false;

	// sanity check
	if (!info->input_file  || info->input_file[0] == 0 ||
		!info->output_file || info->output_file[0] == 0)
	{
		ajbsp::SetErrorMsg("INTERNAL ERROR: Missing in/out filename !");
		return ajbsp::BUILD_BadArgs;
	}

	int num_levels = edit_wad->NumLevels();

	if (num_levels <= 0)
	{
		ajbsp::SetErrorMsg("No levels found in wad !");
		return ajbsp::BUILD_Unknown;
	}

	GB_PrintMsg("\n");
//	PrintVerbose("Creating nodes using tunable factor of %d\n", info->factor);

	GB_DisplayOpen(ajbsp::DIS_BUILDPROGRESS);
	GB_DisplaySetTitle("glBSP Build Progress");

	file_msg = StringPrintf("File: %s", info->input_file);

	GB_DisplaySetBarText(2, file_msg);
	GB_DisplaySetBarLimit(2, num_levels * 10);
	GB_DisplaySetBar(2, 0);

	StringFree(file_msg);

	info->file_pos = 0;

	// loop over each level in the wad
	for (int n = 0 ; n < num_levels ; n++)
	{
		ret = ajbsp::BuildNodesForLevel(info, n);

		if (ret != ajbsp::BUILD_OK)
			break;

		info->file_pos += 10;

		GB_DisplaySetBar(2, info->file_pos);

		Fl::check();
	}

	GB_DisplayClose();

	// writes all the lumps to the output wad
	if (ret == ajbsp::BUILD_OK)
	{
		GB_PrintMsg("\n");
		GB_PrintMsg("Total serious warnings: %d\n", info->total_big_warn);
		GB_PrintMsg("Total minor warnings: %d\n", info->total_small_warn);

//!!!		ReportFailedLevels();
	}

	return ret;
}


static bool DM_BuildNodes(const char *in_name, const char *out_name)
{
	LogPrintf("\n");

	display_mode = ajbsp::DIS_INVALID;

	nb_info = new ajbsp::nodebuildinfo_t;

	nb_info->input_file  = StringDup(in_name);
	nb_info->output_file = StringDup(out_name);

	nb_info->fast          = bsp_fast ? true : false;
	nb_info->quiet         = bsp_verbose ? false : true;
	nb_info->mini_warnings = bsp_warn ? true : false;

	ajbsp::build_result_e  ret;

	ret = BuildAllNodes(nb_info);

	if (ret == ajbsp::BUILD_Cancelled)
	{
		GB_PrintMsg("\n");
		GB_PrintMsg("Building CANCELLED.\n\n");

		delete nb_info;

		return false;
	}

	if (ret != ajbsp::BUILD_OK)
	{
		// build nodes failed
		GB_PrintMsg("\n");
		GB_PrintMsg("Building FAILED: %s\n", build_ErrorString(ret));
		GB_PrintMsg("Reason: %s\n\n", nb_info->message);

		delete nb_info;

		return false;
	}

	delete nb_info;

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
	else if (nb_info->cancelled)
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

		if (! CMD_BuildNodes())
			return;
	}

	
	// FIXME:
	// if (missing nodes)
	//    DLG_Confirm(  "build the nodes now?")


	// FIXME: figure out the proper directory to cd into
	//        (and ensure that it exists)

	// TODO : remember current dir, reset afterwards

	// FIXME : check if this worked
	FileChangeDir("/home/aapted/oblige");


	char cmd_buffer[FL_PATH_MAX * 2];

	// FIXME: use fl_filename_absolute() to get absolute paths


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
