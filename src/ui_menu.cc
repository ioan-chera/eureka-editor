//------------------------------------------------------------------------
//  Menus
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2013 Andrew Apted
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
#include "ui_about.h"
#include "ui_misc.h"
#include "ui_prefs.h"

#include "editloop.h"
#include "e_basis.h"
#include "e_loadsave.h"
#include "e_nodes.h"
#include "e_cutpaste.h"
#include "e_path.h"
#include "levels.h"
#include "r_grid.h"
#include "x_mirror.h"


//------------------------------------------------------------------------
//  FILE MENU
//------------------------------------------------------------------------

static void file_do_quit(Fl_Widget *w, void * data)
{
	CMD_Quit();
}

static void file_do_new(Fl_Widget *w, void * data)
{
	CMD_NewMap();
}

static void file_do_open(Fl_Widget *w, void * data)
{
	CMD_OpenMap();
}

static void file_do_save(Fl_Widget *w, void * data)
{
	CMD_SaveMap();
}

static void file_do_export(Fl_Widget *w, void * data)
{
	CMD_ExportMap();
}

static void file_do_recent(Fl_Widget *w, void * data)
{
	CMD_OpenRecentMap();
}

static void file_do_manage_wads(Fl_Widget *w, void * data)
{
	Main_ProjectSetup();
}

static void file_do_prefs(Fl_Widget *w, void * data)
{
	CMD_Preferences();
}

static void file_do_build_nodes(Fl_Widget *w, void * data)
{
	CMD_BuildNodes();
}

static void file_do_test_map(Fl_Widget *w, void * data)
{
	CMD_TestMap();
}


//------------------------------------------------------------------------
//  EDIT MENU
//------------------------------------------------------------------------

static void edit_do_undo(Fl_Widget *w, void * data)
{
	if (BA_Undo())
		edit.RedrawMap = 1;
	else
		Beep("No operation to undo");
}

static void edit_do_redo(Fl_Widget *w, void * data)
{
	if (BA_Redo())
		edit.RedrawMap = 1;
	else
		Beep("No operation to redo");
}

static void edit_do_cut(Fl_Widget *w, void * data)
{
	if (! CMD_Copy())
	{
		Beep("Nothing to cut");
		return;
	}

	ExecuteCommand("Delete");
}

static void edit_do_copy(Fl_Widget *w, void * data)
{
	if (! CMD_Copy())
	{
		Beep("Nothing to copy");
		return;
	}
}

static void edit_do_paste(Fl_Widget *w, void * data)
{
	if (! CMD_Paste())
	{
		Beep("Clipboard is empty");
		return;
	}
}

static void edit_do_delete(Fl_Widget *w, void * data)
{
	ExecuteCommand("Delete");
}


static void edit_do_select_all(Fl_Widget *w, void * data)
{
	CMD_SelectAll();
}

static void edit_do_unselect_all(Fl_Widget *w, void * data)
{
	CMD_UnselectAll();
}

static void edit_do_invert_sel(Fl_Widget *w, void * data)
{
	CMD_InvertSelection();
}


static void edit_do_move(Fl_Widget *w, void * data)
{
	if (edit.Selected->empty())
	{
		Beep("Nothing to move");
		return;
	}

	UI_MoveDialog * dialog = new UI_MoveDialog();

	dialog->Run();

	delete dialog;
}

static void edit_do_scale(Fl_Widget *w, void * data)
{
	if (edit.Selected->empty())
	{
		Beep("Nothing to scale");
		return;
	}

	UI_ScaleDialog * dialog = new UI_ScaleDialog();

	dialog->Run();

	delete dialog;
}

static void edit_do_rotate(Fl_Widget *w, void * data)
{
	if (edit.Selected->empty())
	{
		Beep("Nothing to rotate");
		return;
	}

	UI_RotateDialog * dialog = new UI_RotateDialog();

	dialog->Run();

	delete dialog;
}

static void edit_do_prune_unused(Fl_Widget *w, void * data)
{
	CMD_PruneUnused();
}

static void edit_do_mirror_horiz(Fl_Widget *w, void * data)
{
	ExecuteCommand("Mirror", "horiz");
}

static void edit_do_mirror_vert(Fl_Widget *w, void * data)
{
	ExecuteCommand("Mirror", "vert");
}


//------------------------------------------------------------------------
//  VIEW MENU
//------------------------------------------------------------------------

static void view_do_logs(Fl_Widget *w, void * data)
{
	if (! log_viewer->shown())
		  log_viewer->show();
}

static void view_do_zoom_in(Fl_Widget *w, void * data)
{
	Editor_Zoom(+1, I_ROUND(grid.orig_x), I_ROUND(grid.orig_y));
}

static void view_do_zoom_out(Fl_Widget *w, void * data)
{
	Editor_Zoom(-1, I_ROUND(grid.orig_x), I_ROUND(grid.orig_y));
}

static void view_do_whole_map(Fl_Widget *w, void * data)
{
	CMD_ZoomWholeMap();
}

static void view_do_whole_selection(Fl_Widget *w, void * data)
{
	CMD_ZoomSelection();
}

static void view_do_camera_pos(Fl_Widget *w, void * data)
{
	CMD_GoToCamera();
}

static void view_do_object_nums(Fl_Widget *w, void * data)
{
	ExecuteCommand("Toggle", "obj_nums");
}

static void view_do_grid_type(Fl_Widget *w, void * data)
{
	grid.ToggleMode();
}


//------------------------------------------------------------------------
//  SEARCH MENU
//------------------------------------------------------------------------

static void search_do_find(Fl_Widget *w, void * data)
{
	// TODO: CMD_FindObject()
	Beep("Find: not implemented");
}

static void search_do_find_next(Fl_Widget *w, void * data)
{
	// CMD_FindObject();
	Beep("Find: not implemented");
}

static void search_do_jump(Fl_Widget *w, void * data)
{
	CMD_JumpToObject();
}


//------------------------------------------------------------------------
//  BROWSER MENU
//------------------------------------------------------------------------

static void browser_do_textures(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser('T');
}

static void browser_do_flats(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser('F');
}

static void browser_do_things(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser('O');
}

static void browser_do_lines(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser('L');
}

static void browser_do_sectors(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser('S');
}


#if 0
static void browser_go_wide(Fl_Widget *w, void * data)
{
	// TODO
}

static void browser_go_narrow(Fl_Widget *w, void * data)
{
	// TODO
}
#endif

static void browser_hide(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser(0);
}


//------------------------------------------------------------------------
//  HELP MENU
//------------------------------------------------------------------------

void help_do_about(Fl_Widget *w, void * data)
{
	DLG_AboutText();
}


//------------------------------------------------------------------------

#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] = 
{
	{ "&File", 0, 0, 0, FL_SUBMENU },

		{ "&New Map",   FL_COMMAND + 'n', FCAL file_do_new },
		{ "&Open Map",  FL_COMMAND + 'o', FCAL file_do_open },
		{ "&Save Map",  FL_COMMAND + 's', FCAL file_do_save },
		{ "&Export Map",  FL_COMMAND + 'e', FCAL file_do_export },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Recent Files ",  FL_COMMAND + 'r', FCAL file_do_recent },
		{ "&Manage Wads  ",  FL_COMMAND + 'm', FCAL file_do_manage_wads },
		{ "&Preferences",    FL_COMMAND + 'p', FCAL file_do_prefs },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
#if 0
		{ "&DDF", 0, 0, 0, FL_SUBMENU },
			{ "Load DDF &File",    0, FCAL view_do_zoom_out },
			{ "Load DDF &WAD",    0, FCAL view_do_zoom_out },

			{ "&Open RTS File",   0, FCAL view_do_zoom_out },
			{ "&Import from WAD", 0, FCAL view_do_zoom_out },
			{ "&Save RTS File",   0, FCAL view_do_zoom_out },
			{ 0 },
#endif

		{ "&Build Nodes  ",   FL_COMMAND + 'b', FCAL file_do_build_nodes },
//TODO	{ "&Test Map",        FL_COMMAND + 't', FCAL file_do_test_map },

		{ "&Quit",      FL_COMMAND + 'q', FCAL file_do_quit },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },

		{ "&Undo",   FL_COMMAND + 'z',  FCAL edit_do_undo },
		{ "&Redo",   FL_COMMAND + 'y',  FCAL edit_do_redo },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Cu&t",    FL_COMMAND + 'x',  FCAL edit_do_cut },
		{ "&Copy",   FL_COMMAND + 'c',  FCAL edit_do_copy },
		{ "&Paste",  FL_COMMAND + 'v',  FCAL edit_do_paste },
		{ "Delete",  FL_Delete,         FCAL edit_do_delete },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Select &All",       FL_COMMAND + 'a', FCAL edit_do_select_all },
		{ "Unselect All",      FL_COMMAND + 'u', FCAL edit_do_unselect_all },
		{ "&Invert Selection", FL_COMMAND + 'i', FCAL edit_do_invert_sel },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Move Objects...",      0, FCAL edit_do_move },
		{ "&Scale Objects...",     0, FCAL edit_do_scale },
		{ "Rotate Objects...",     0, FCAL edit_do_rotate },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Prune Unused Objs",    0, FCAL edit_do_prune_unused },
		{ "Mirror &Horizontally",  0, FCAL edit_do_mirror_horiz },
		{ "Mirror &Vertically",    0, FCAL edit_do_mirror_vert },

//??   "~Exchange object numbers", 24,     0,
		{ 0 },

	{ "&View", 0, 0, 0, FL_SUBMENU },

		{ "Logs....",   0, FCAL view_do_logs },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Toggle Object Nums",  0, FCAL view_do_object_nums },
		{ "Toggle Grid Type",    0, FCAL view_do_grid_type },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Zoom &In",      0, FCAL view_do_zoom_in },
		{ "Zoom &Out",     0, FCAL view_do_zoom_out },
		{ "&Whole Map",    0, FCAL view_do_whole_map },
		{ "Whole &Selection", 0, FCAL view_do_whole_selection },
		{ "Go to &Camera", 0, FCAL view_do_camera_pos },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

//TODO		{ "&Find Object",  FL_COMMAND + 'f', FCAL search_do_find },
//TODO		{ "Find &Next",    FL_COMMAND + 'g', FCAL search_do_find_next },
		{ "&Jump to Object",   0, FCAL search_do_jump },
		{ 0 },

	{ "&Browser", 0, 0, 0, FL_SUBMENU },
	
		{ "Textures",     0, FCAL browser_do_textures },
		{ "Flats",        0, FCAL browser_do_flats },
		{ "Thing Types",  0, FCAL browser_do_things },
		{ "Line Types",   0, FCAL browser_do_lines },
		{ "Sector Types", 0, FCAL browser_do_sectors },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Hide",         0, FCAL browser_hide },
		{ 0 },

	{ "&Help", 0, 0, 0, FL_SUBMENU },
		{ "&About Eureka",   0,  FCAL help_do_about },
		{ 0 },

	{ 0 }
};


#ifdef __APPLE__
Fl_Sys_Menu_Bar * Menu_Create(int x, int y, int w, int h)
{
	Fl_Sys_Menu_Bar *bar = new Fl_Sys_Menu_Bar(x, y, w, h);
	bar->menu(menu_items);
	return bar;
}
#else
Fl_Menu_Bar * Menu_Create(int x, int y, int w, int h)
{
	Fl_Menu_Bar *bar = new Fl_Menu_Bar(x, y, w, h);
	bar->textsize(KF_fonth);
	bar->menu(menu_items);
	return bar;
}
#endif


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
