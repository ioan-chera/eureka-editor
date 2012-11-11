//------------------------------------------------------------------------
//  BUILDING NODES
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
#include "w_wad.h"

#include "glbsp.h"


static glbsp::nodebuildinfo_t nb_info;
static volatile glbsp::nodebuildcomms_t nb_comms;

static int display_mode = glbsp::DIS_INVALID;
static int progress_limit;

static char message_buf[MSG_BUF_LEN];


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
/*
	Main_Ticker();

	if (main_action >= MAIN_CANCEL)
	{
		nb_comms.cancelled = TRUE;
	}
*/
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
		// extract map name

		const char * map_name = str + strlen(str);

		if (map_name > str)
			map_name--;

		while (map_name > str && !isspace(*map_name))
			map_name--;

fprintf(stderr, "map name: '%s'\n", map_name);
	}
}

static void GB_DisplaySetBarLimit(int barnum, int limit)
{
	if (display_mode == glbsp::DIS_BUILDPROGRESS && barnum == 2)
	{
fprintf(stderr, "limit --> %d\n", limit);
/*
		progress_limit = limit;

		main_win->build_box->SetStatus("Building nodes");
		main_win->build_box->Prog_Nodes(0, limit);
*/
	}
}

static void GB_DisplaySetBar(int barnum, int count)
{
	if (display_mode == glbsp::DIS_BUILDPROGRESS && barnum == 2)
	{
// fprintf(stderr, "progress --> %d\n", count);
/*
		main_win->build_box->Prog_Nodes(count, progress_limit);
*/
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

	nb_info.quiet = TRUE;
	nb_info.pack_sides = FALSE;
	nb_info.force_normal = TRUE;
	nb_info.fast = TRUE;

	glbsp::glbsp_ret_e  ret;

	ret = glbsp::CheckInfo(&nb_info, &nb_comms);

	if (ret != glbsp::GLBSP_E_OK)
	{
		// check info failure (unlikely to happen)
		GB_PrintMsg("Param Check FAILED: %s\n", glbsp_ErrorString(ret));
		GB_PrintMsg("Reason: %s\n\n", nb_comms.message);

//??		Main_ProgStatus("glBSP Error");
		return false;
	}

	ret = glbsp::BuildNodes(&nb_info, &build_funcs, &nb_comms);

	if (ret == glbsp::GLBSP_E_Cancelled)
	{
		GB_PrintMsg("Building CANCELLED.\n\n");
//??		Main_ProgStatus("Cancelled");
		return false;
	}

	if (ret != glbsp::GLBSP_E_OK)
	{
		// build nodes failed
		GB_PrintMsg("Building FAILED: %s\n", glbsp_ErrorString(ret));
		GB_PrintMsg("Reason: %s\n\n", nb_comms.message);

//??		Main_ProgStatus("glBSP Error");
		return false;
	}

	return true;
}


void CMD_BuildNodes()
{
	if (MadeChanges)
	{
		// FIXME: dialog "your changes are not saved"
		Beep();
		return;
	}

	if (! edit_wad)
	{
		// FIXME: dialog "require a PWAD to build nodes for"
		Beep();
		return;
	}

	DM_BuildNodes(edit_wad->PathName(), "./foobie.wad");

	// TODO
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
