//------------------------------------------------------------------------
//  BUILDING NODES / PLAY THE MAP
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
#include "levels.h"
#include "e_loadsave.h"
#include "m_dialog.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_nodes.h"


#include "glbsp.h"


// config items
bool glbsp_fast    = false;
bool glbsp_verbose = false;
bool glbsp_warn    = false;


static glbsp::nodebuildinfo_t nb_info;
static volatile glbsp::nodebuildcomms_t nb_comms;

static int display_mode = glbsp::DIS_INVALID;
static int progress_limit;

static char message_buf[MSG_BUF_LEN];

static UI_NodeDialog * dialog;


static const char *glbsp_ErrorString(glbsp::glbsp_ret_e ret)
{
	switch (ret)
	{
		case glbsp::GLBSP_E_OK: return "OK";

		 // the arguments were bad/inconsistent.
		case glbsp::GLBSP_E_BadArgs: return "Bad Arguments";

		// the info was bad/inconsistent, but has been fixed
		case glbsp::GLBSP_E_BadInfoFixed: return "Bad Args (fixed)";

		// file errors
		case glbsp::GLBSP_E_ReadError:  return "Read Error";
		case glbsp::GLBSP_E_WriteError: return "Write Error";

		// building was cancelled
		case glbsp::GLBSP_E_Cancelled: return "Cancelled by User";

		// an unknown error occurred (this is the catch-all value)
		case glbsp::GLBSP_E_Unknown:

		default: return "Unknown Error";
	}
}


static void GB_PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsnprintf(message_buf, MSG_BUF_LEN, str, args);
	va_end(args);

	message_buf[MSG_BUF_LEN-1] = 0;

	dialog->Print(message_buf);

	LogPrintf("GLBSP: %s", message_buf);
}

static void GB_FatalError(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsnprintf(message_buf, MSG_BUF_LEN, str, args);
	va_end(args);

	message_buf[MSG_BUF_LEN-1] = 0;

	FatalError("glBSP Failure:\n%s", message_buf);
	/* NOT REACHED */
}

static void GB_Ticker(void)
{
	if (dialog->WantCancel())
	{
		nb_comms.cancelled = TRUE;
	}
}

static glbsp::boolean_g GB_DisplayOpen(glbsp::displaytype_e type)
{
	display_mode = type;
	return TRUE;
}

static void GB_DisplaySetTitle(const char *str)
{
	/* does nothing */
}

static void GB_DisplaySetBarText(int barnum, const char *str)
{
	if (display_mode == glbsp::DIS_BUILDPROGRESS && barnum == 1)
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

static void GB_DisplaySetBarLimit(int barnum, int limit)
{
	if (display_mode == glbsp::DIS_BUILDPROGRESS && barnum == 2)
	{
		progress_limit = MAX(1, limit);
	}
}

static void GB_DisplaySetBar(int barnum, int count)
{
	if (display_mode == glbsp::DIS_BUILDPROGRESS && barnum == 2)
	{
		int perc = count * 100.0 / progress_limit;

		dialog->SetProg(perc);
	}
}

static void GB_DisplayClose(void)
{
	/* does nothing */
}

static const glbsp::nodebuildfuncs_t  build_funcs =
{
	GB_FatalError,
	GB_PrintMsg,
	GB_Ticker,

	GB_DisplayOpen,
	GB_DisplaySetTitle,
	GB_DisplaySetBar,
	GB_DisplaySetBarLimit,
	GB_DisplaySetBarText,
	GB_DisplayClose
};


static bool DM_BuildNodes(const char *in_name, const char *out_name)
{
	LogPrintf("\n");

	display_mode = glbsp::DIS_INVALID;

	memcpy(&nb_info,  &glbsp::default_buildinfo,  sizeof(glbsp::default_buildinfo));
	memcpy((void*)&nb_comms, &glbsp::default_buildcomms, sizeof(glbsp::nodebuildcomms_t));

	nb_info.input_file  = glbsp::GlbspStrDup(in_name);
	nb_info.output_file = glbsp::GlbspStrDup(out_name);

	nb_info.fast          = glbsp_fast ? TRUE : FALSE;
	nb_info.quiet         = glbsp_verbose ? FALSE : TRUE;
	nb_info.mini_warnings = glbsp_warn ? TRUE : FALSE;

	nb_info.pack_sides = FALSE;
	nb_info.force_normal = TRUE;

	glbsp::glbsp_ret_e  ret;

	ret = glbsp::CheckInfo(&nb_info, &nb_comms);

	if (ret != glbsp::GLBSP_E_OK)
	{
		// check info failure (unlikely to happen)
		GB_PrintMsg("\n");
		GB_PrintMsg("Param Check FAILED: %s\n", glbsp_ErrorString(ret));
		GB_PrintMsg("Reason: %s\n\n", nb_comms.message);
		return false;
	}

	ret = glbsp::BuildNodes(&nb_info, &build_funcs, &nb_comms);

	if (ret == glbsp::GLBSP_E_Cancelled)
	{
		GB_PrintMsg("\n");
		GB_PrintMsg("Building CANCELLED.\n\n");
		return false;
	}

	if (ret != glbsp::GLBSP_E_OK)
	{
		// build nodes failed
		GB_PrintMsg("\n");
		GB_PrintMsg("Building FAILED: %s\n", glbsp_ErrorString(ret));
		GB_PrintMsg("Reason: %s\n\n", nb_comms.message);
		return false;
	}

	return true;
}


void CMD_BuildNodes()
{
	if (! edit_wad)
	{
		Notify(-1, -1, "Cannot build nodes unless you are editing a PWAD.", NULL);
		return;
	}

	if (MadeChanges)
	{
		// TODO: ideally ask the question "save changes now?"
		//       HOWEVER we need to know if that was successful (ouch)

		Notify(-1, -1, "You have unsaved changes, please save them first.", NULL);
		return;
	}


	const char *old_name = StringDup(edit_wad->PathName());
	const char *new_name = ReplaceExtension(old_name, "new");

	if (MatchExtension(old_name, "new"))
	{
		Notify(-1, -1, "Cannot build nodes on a pwad with .NEW extension.", NULL);
		return;
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

		Replacer = false;
		MadeChanges = 0;
	}
}


//------------------------------------------------------------------------


void CMD_TestMap()
{
	// TODO: remove this restriction
	if (! edit_wad)
	{
		Notify(-1, -1, "Cannot test the map unless you are editing a PWAD.", NULL);
		return;
	}

	if (MadeChanges)
	{
		// TODO: ideally ask the question "save changes now?"
		//       HOWEVER we need to know if that was successful (ouch)

		Notify(-1, -1, "You have unsaved changes, please save them first.", NULL);
		return;
	}

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
