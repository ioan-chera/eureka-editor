//------------------------------------------------------------------------
//  BUILDING NODES / PLAY THE MAP
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2019 Andrew Apted
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

#include "Errors.h"
#include "Instance.h"
#include "main.h"
#include "m_config.h"
#include "m_loadsave.h"
#include "e_main.h"
#include "w_wad.h"

#include "ui_window.h"

#include "bsp.h"


// config items
bool config::bsp_on_save	= true;
bool config::bsp_fast		= false;
bool config::bsp_warnings	= false;

int  config::bsp_split_factor	= DEFAULT_FACTOR;

bool config::bsp_gl_nodes		= true;
bool config::bsp_force_v5		= false;
bool config::bsp_force_zdoom	= false;
bool config::bsp_compressed		= false;


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
	    Fl_Double_Window(400, 400, "Building Nodes")
{
	size_range(w(), h());

	callback((Fl_Callback *) close_callback, this);

	color(WINDOW_BG, WINDOW_BG);


	browser = new Fl_Browser(0, 0, w(), h() - 100);


	Fl_Box * ptext = new Fl_Box(FL_NO_BOX, 10, h() - 80, 80, 20, "Progress:");
	(void) ptext;


	progress = new Fl_Progress(100, h() - 80, w() - 120, 20);
	progress->align(FL_ALIGN_INSIDE);
	progress->box(FL_FLAT_BOX);
	progress->color(FL_LIGHT2, NODE_PROGRESS_COLOR);

	progress->minimum(0.0);
	progress->maximum(100.0);
	progress->value(0.0);


	button = new Fl_Button(w() - 100, h() - 46, 80, 30, "Cancel");
	button->callback(button_callback, this);


	end();

	resizable(browser);
}


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


void UI_NodeDialog::SetProg(int perc)
{
	if (perc == cur_prog)
		return;

	cur_prog = perc;

	snprintf(prog_label, sizeof(prog_label), "%d%%", perc);

	progress->value(static_cast<float>(perc));
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


//------------------------------------------------------------------------

static const char *build_ErrorString(build_result_e ret)
{
	switch (ret)
	{
		case BUILD_OK: return "OK";

		// building was cancelled
		case BUILD_Cancelled: return "Cancelled by User";

		// the WAD file was corrupt / empty / bad filename
		case BUILD_BadFile: return "Bad File";

		default: return "Unknown Error";
	}
}


void Instance::GB_PrintMsg(EUR_FORMAT_STRING(const char *str), ...)
{
	va_list args;

	va_start(args, str);
	SString message_buf = SString::vprintf(str, args);
	va_end(args);

	if (nodeialog)
		nodeialog->Print(message_buf.c_str());

	gLog.printf("BSP: %s\n", message_buf.c_str());
}


static void PrepareInfo(nodebuildinfo_t *info)
{
	info->factor	= clamp(1, config::bsp_split_factor, 31);

	info->gl_nodes	= config::bsp_gl_nodes;
	info->fast		= config::bsp_fast;
	info->warnings	= config::bsp_warnings;

	info->force_v5			= config::bsp_force_v5;
	info->force_xnod		= config::bsp_force_zdoom;
	info->force_compress	= config::bsp_compressed;

	info->total_failed_maps		= 0;
	info->total_warnings		= 0;

	// clear cancelled flag
	info->cancelled = false;
}


build_result_e Instance::BuildAllNodes(nodebuildinfo_t *info)
{
	gLog.printf("\n");

	// sanity check

	SYS_ASSERT(1 <= info->factor && info->factor <= 32);

	int num_levels = wad.master.editWad()->LevelCount();
	SYS_ASSERT(num_levels > 0);

	GB_PrintMsg("\n");

	nodeialog->SetProg(0);

	build_result_e ret;

	// loop over each level in the wad
	for (int n = 0 ; n < num_levels ; n++)
	{
		// load level
		try
		{
			NewDocument newdoc = openDocument(loaded, *wad.master.editWad(), n);
			
			ret = AJBSP_BuildLevel(info, n, *this, newdoc.doc, newdoc.loading, *wad.master.editWad());
		}
		catch(const std::runtime_error &e)
		{
			GB_PrintMsg("Failed building nodes for level %d: %s\n", n, e.what());
			continue;
		}

		// don't fail on maps with overflows
		// [ Note that 'total_failed_maps' keeps a tally of these ]
		if (ret == BUILD_LumpOverflow)
			ret = BUILD_OK;

		if (ret != BUILD_OK)
			break;

		nodeialog->SetProg(100 * (n + 1) / num_levels);

		Fl::check();

		if (nodeialog->WantCancel())
		{
			nb_info->cancelled = true;
		}
	}
	
	try
	{
		wad.master.editWad()->writeToDisk();
	}
	catch(const std::runtime_error &e)
	{
		GB_PrintMsg("ERROR: could not save %s: %s\n", wad.master.editWad()->PathName().u8string().c_str(), e.what());
		ret = BUILD_BadFile;
	}

	if (ret == BUILD_OK)
	{
		GB_PrintMsg("\n");

		if (info->total_failed_maps == 0)
			GB_PrintMsg("All maps built successfully, %d warnings\n",
						info->total_warnings);
		else
			GB_PrintMsg("%d failed maps, %d warnings\n",
						info->total_failed_maps,
						info->total_warnings);
	}
	else if (ret == BUILD_Cancelled)
	{
		GB_PrintMsg("\n");
		GB_PrintMsg("Building CANCELLED.\n\n");
	}
	else
	{
		// build nodes failed
		GB_PrintMsg("\n");
		GB_PrintMsg("Building FAILED: %s\n", build_ErrorString(ret));
	}

	return ret;
}


void Instance::BuildNodesAfterSave(int lev_idx, const LoadingData& loading, Wad_file &wad)
{
	nodeialog.reset();

	nodebuildinfo_t nb_info;

	PrepareInfo(&nb_info);

	build_result_e ret = AJBSP_BuildLevel(&nb_info, lev_idx, *this, level, loading, wad);

	// TODO : maybe print # of serious/minor warnings

	if (ret != BUILD_OK)
		gLog.printf("NODES FAILED TO FAILED.\n");
}


void Instance::CMD_BuildAllNodes()
{

	if (!wad.master.editWad())
	{
		DLG_Notify("Cannot build nodes unless you are editing a PWAD.");
		return;
	}

	if (wad.master.editWad()->IsReadOnly())
	{
		DLG_Notify("Cannot build nodes on a read-only file.");
		return;
	}

	if (level.MadeChanges)
	{
		if (DLG_Confirm({ "Cancel", "&Save" },
			"You have unsaved changes, do you want to save them now "
			"and then build all the nodes?") <= 0)
		{
			return;
		}

		bool save_result = M_SaveMap(true);

		// user cancelled the save?
		if (!save_result)
			return;
	}


	// this probably cannot happen, but check anyway
	if (wad.master.editWad()->LevelCount() == 0)
	{
		DLG_Notify("Cannot build nodes: no levels found!");
		return;
	}


	// remember current level
	SString CurLevel(loaded.levelName);

	// reset various editor state
	Editor_ClearAction();
	Selection_InvalidateLast();

	edit.Selected->clear_all();
	edit.highlight.clear();


	nodeialog.emplace();

	nodeialog->set_modal();
	nodeialog->show();

	Fl::check();


	nodebuildinfo_t nb_info;

	PrepareInfo(&nb_info);

	build_result_e ret = BuildAllNodes(&nb_info);

	if (ret == BUILD_OK)
	{
		nodeialog->Finish_OK();
		Status_Set("Built nodes OK");
	}
	else if (nb_info.cancelled)
	{
		nodeialog->Finish_Cancel();
		Status_Set("Cancelled building nodes");
	}
	else
	{
		nodeialog->Finish_Error();
		Status_Set("Error building nodes");
	}

	while (!nodeialog->WantClose())
	{
		Fl::wait(0.2);
	}

	nodeialog.reset();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
