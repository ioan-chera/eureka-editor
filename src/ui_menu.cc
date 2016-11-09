//------------------------------------------------------------------------
//  MENUS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
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
#include "m_files.h"
#include "r_grid.h"
#include "r_render.h"
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

static void file_do_copy_map(Fl_Widget *w, void * data)
{
	CMD_CopyMap();
}

static void file_do_rename(Fl_Widget *w, void * data)
{
	CMD_RenameMap();
}

static void file_do_delete(Fl_Widget *w, void * data)
{
	CMD_DeleteMap();
}

static void file_do_new_project(Fl_Widget *w, void * data)
{
	CMD_NewProject();
}

static void file_do_manage_project(Fl_Widget *w, void * data)
{
	CMD_ManageProject();
}

static void file_do_default_props(Fl_Widget *w, void * data)
{
	main_win->ShowDefaultProps();
}

static void file_do_prefs(Fl_Widget *w, void * data)
{
	CMD_Preferences();
}

static void file_do_build_nodes(Fl_Widget *w, void * data)
{
	CMD_BuildNodes();
}

#if 0
static void file_do_test_map(Fl_Widget *w, void * data)
{
	CMD_TestMap();
}
#endif


static void file_do_load_given(Fl_Widget *w, void *data)
{
	const char *filename = (const char *) data;

	int given_idx = M_FindGivenFile(filename);

	if (given_idx >= 0)
		last_given_file = given_idx;

	CMD_OpenFileMap(filename);
}


static void file_do_load_recent(Fl_Widget *w, void *data)
{
	M_OpenRecentFromMenu(data);
}


//------------------------------------------------------------------------
//  EDIT MENU
//------------------------------------------------------------------------

static void edit_do_undo(Fl_Widget *w, void * data)
{
	if (BA_Undo())
		RedrawMap();
	else
		Beep("No operation to undo");
}

static void edit_do_redo(Fl_Widget *w, void * data)
{
	if (BA_Redo())
		RedrawMap();
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

static void edit_do_last_sel(Fl_Widget *w, void * data)
{
	CMD_LastSelection();
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
	LogViewer_Open();
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

static void view_do_toggle_3d(Fl_Widget *w, void * data)
{
	Render3D_Enable(! edit.render3d);
}

static void view_do_object_nums(Fl_Widget *w, void * data)
{
	ExecuteCommand("Toggle", "obj_nums");
}

static void view_do_grid_type(Fl_Widget *w, void * data)
{
	grid.ToggleMode();
}

static void view_do_sector_render(Fl_Widget *w, void * data)
{
	const Fl_Menu_Item *item = ((Fl_Menu_*)w)->mvalue();

	if (strcmp(item->text, "Floors") == 0)
		edit.sector_render_mode = SREND_Floor;
	else if (strcmp(item->text, "Ceilings") == 0)
		edit.sector_render_mode = SREND_Ceiling;
	else if (strcmp(item->text, "Lighting") == 0)
		edit.sector_render_mode = SREND_Lighting;
	else
		edit.sector_render_mode = SREND_Nothing;

	main_win->redraw();
}

static void view_do_find(Fl_Widget *w, void * data)
{
	main_win->ShowFindAndReplace();
}

static void view_do_next(Fl_Widget *w, void * data)
{
	main_win->find_box->FindNext();
}

static void view_do_jump(Fl_Widget *w, void * data)
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

static void browser_do_gen_types(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser('G');
}


static void browser_do_recent_things(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser('O');
	main_win->browser->ToggleRecent(true /* force */);
}

static void browser_do_recent_tex(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser('T');
	main_win->browser->ToggleRecent(true /* force */);
}

static void browser_do_recent_flats(Fl_Widget *w, void * data)
{
	main_win->ShowBrowser('F');
	main_win->browser->ToggleRecent(true /* force */);
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
//  CHECK MENU
//------------------------------------------------------------------------

static void checks_do_all(Fl_Widget *w, void * data)
{
	ExecuteCommand("Check", "all");
}

static void checks_do_major(Fl_Widget *w, void * data)
{
	ExecuteCommand("Check", "major");
}

static void checks_do_vertices(Fl_Widget *w, void * data)
{
	ExecuteCommand("Check", "vertices");
}

static void checks_do_sectors(Fl_Widget *w, void * data)
{
	ExecuteCommand("Check", "sectors");
}

static void checks_do_linedefs(Fl_Widget *w, void * data)
{
	ExecuteCommand("Check", "linedefs");
}

static void checks_do_things(Fl_Widget *w, void * data)
{
	ExecuteCommand("Check", "things");
}

static void checks_do_textures(Fl_Widget *w, void * data)
{
	ExecuteCommand("Check", "textures");
}

static void checks_do_tags(Fl_Widget *w, void * data)
{
	ExecuteCommand("Check", "tags");
}


//------------------------------------------------------------------------
//  HELP MENU
//------------------------------------------------------------------------

void help_do_online_docs(Fl_Widget *w, void * data)
{
	fl_open_uri("http://eureka-editor.sourceforge.net/?n=Docs.Index");
}

void help_do_about(Fl_Widget *w, void * data)
{
	DLG_AboutText();
}


//------------------------------------------------------------------------

#define M_GIVEN_FILES	"&Given Files"
#define M_RECENT_FILES	"&Recent Files"


#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] =
{
	{ "&File", 0, 0, 0, FL_SUBMENU },

		{ "&New Project   ",  FL_COMMAND + 'n', FCAL file_do_new_project },
		{ "New Map",   FL_COMMAND + FL_SHIFT + 'n', FCAL file_do_new },
		{ "&Manage Project  ",  FL_COMMAND + 'm', FCAL file_do_manage_project },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Open Map",  FL_COMMAND + 'o', FCAL file_do_open },

		{ M_GIVEN_FILES, 0, 0, 0, FL_SUBMENU|FL_MENU_INACTIVE },
			{ 0 },

		{ M_RECENT_FILES, 0, 0, 0, FL_SUBMENU|FL_MENU_INACTIVE },
			{ 0 },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Save Map",    FL_COMMAND + 's', FCAL file_do_save },
		{ "&Export Map",  FL_COMMAND + 'e', FCAL file_do_export },
		{ "&Copy Map",    0,  FCAL file_do_copy_map },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Rename Map",   0,  FCAL file_do_rename },
		{ "Delete Map",   0,  FCAL file_do_delete },

		{ "&Build Nodes  ",  FL_COMMAND + 'b', FCAL file_do_build_nodes },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
#if 0
		{ "&Test Map",       FL_COMMAND + 't', FCAL file_do_test_map },
#endif

		{ "&Preferences",      FL_COMMAND + 'p', FCAL file_do_prefs },
		{ "&Quit",             FL_COMMAND + 'q', FCAL file_do_quit },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },

		{ "&Undo",   FL_COMMAND + 'z',  FCAL edit_do_undo },
		{ "&Redo",   FL_COMMAND + 'y',  FCAL edit_do_redo },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Cu&t",     FL_COMMAND + 'x', FCAL edit_do_cut },
		{ "&Copy",    FL_COMMAND + 'c', FCAL edit_do_copy },
		{ "&Paste",   FL_COMMAND + 'v', FCAL edit_do_paste },
		{ "&Delete",  FL_Delete,        FCAL edit_do_delete },
		{ "Prune Unused", 0,            FCAL edit_do_prune_unused },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Select &All",       FL_COMMAND + 'a', FCAL edit_do_select_all },
		{ "Unselect All",      FL_COMMAND + 'u', FCAL edit_do_unselect_all },
		{ "&Invert Selection", FL_COMMAND + 'i', FCAL edit_do_invert_sel },
		{ "&Last Selection",   FL_COMMAND + 'l', FCAL edit_do_last_sel },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Move Objects...",      0, FCAL edit_do_move },
		{ "&Scale Objects...",     0, FCAL edit_do_scale },
		{ "Rotate Objects...",     0, FCAL edit_do_rotate },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Mirror &Horizontally",  0, FCAL edit_do_mirror_horiz },
		{ "Mirror &Vertically",    0, FCAL edit_do_mirror_vert },

//??   "~Exchange object numbers", 24,     0,
		{ 0 },

	{ "&View", 0, 0, 0, FL_SUBMENU },

		{ "&Logs....",   0, FCAL view_do_logs },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Toggle &3D View",      0, FCAL view_do_toggle_3d },
		{ "Toggle Object &Nums",  0, FCAL view_do_object_nums },
		{ "&Toggle Grid Type",    0, FCAL view_do_grid_type },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Sector Rendering", 0, 0, 0, FL_SUBMENU },
			{ "Nothing",   0, FCAL view_do_sector_render },
			{ "Floors",    0, FCAL view_do_sector_render },
			{ "Ceilings",  0, FCAL view_do_sector_render },
			{ "Lighting",  0, FCAL view_do_sector_render },
			{ 0 },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Zoom &In",      0, FCAL view_do_zoom_in },
		{ "Zoom &Out",     0, FCAL view_do_zoom_out },
		{ "&Whole Map",    0, FCAL view_do_whole_map },
		{ "Whole &Selection", 0, FCAL view_do_whole_selection },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Default Props  ",  FL_COMMAND + 'd', FCAL file_do_default_props },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Find / Replace",   FL_COMMAND + 'f', FCAL view_do_find },
		{ "&Go to next",       FL_COMMAND + 'g', FCAL view_do_next },
		{ "Go to &Camera",     0, FCAL view_do_camera_pos },
		{ "&Jump to Object",   0, FCAL view_do_jump },
		{ 0 },

	{ "&Browser", 0, 0, 0, FL_SUBMENU },

		{ "&Things",       0, FCAL browser_do_things },
		{ "Te&xtures",     0, FCAL browser_do_textures },
		{ "&Flats",        0, FCAL browser_do_flats },
		{ "&Line Types",   0, FCAL browser_do_lines },
		{ "&Sector Types", 0, FCAL browser_do_sectors },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Generalized Types", 0, FCAL browser_do_gen_types },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Recent Textures", 0, FCAL browser_do_recent_tex },
		{ "Recent Flats",     0, FCAL browser_do_recent_flats },
		{ "Recent Things",    0, FCAL browser_do_recent_things },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Hide",         0, FCAL browser_hide },
		{ 0 },

	{ "&Check", 0, 0, 0, FL_SUBMENU },

		{ "&ALL",          0, FCAL checks_do_all },
		{ "&Major stuff",  0, FCAL checks_do_major },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Vertices",     0, FCAL checks_do_vertices },
		{ "&Sectors",      0, FCAL checks_do_sectors },
		{ "&LineDefs",     0, FCAL checks_do_linedefs },
		{ "&Things",       0, FCAL checks_do_things },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Te&xtures",     0, FCAL checks_do_textures },
		{ "Ta&gs",         0, FCAL checks_do_tags },

		{ 0 },

	{ "&Help", 0, 0, 0, FL_SUBMENU },
		{ "&Online Docs...",    0,  FCAL help_do_online_docs },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&About Eureka...",   0,  FCAL help_do_about },
		{ 0 },

	{ 0 }
};


//------------------------------------------------------------------------


#define MAX_PWAD_LIST  20


static int Menu_FindItem(const Fl_Menu_Item *items, const char *text)
{
	int total = items[0].size();  // includes {0} at end

	for (int i = 0 ; i < total ; i++)
		if (items[i].text && y_stricmp(items[i].text, text) == 0)
			return i;

	return -1;  // not found
}


static void Menu_CopyItem(Fl_Menu_Item* &pos, const Fl_Menu_Item &orig)
{
	memcpy(pos, &orig, sizeof(orig));

	pos++;
}


static void Menu_AddItem(Fl_Menu_Item* &pos, const char *text,
						 Fl_Callback *cb, void *data, int flags)
{
	Fl_Menu_Item item;

	memset(&item, 0, sizeof(item));

	item.text = text;
	item.flags = flags;
	item.callback_ = cb;
	item.user_data_ = data;

	Menu_CopyItem(pos, item);
}


static Fl_Menu_Item * Menu_PopulateGivenFiles(Fl_Menu_Item *items)
{
	int count = (int)Pwad_list.size();

	if (count < 2)
		return items;

	// silently ignore excess pwads
	if (count > MAX_PWAD_LIST)
		count = MAX_PWAD_LIST;


	// find Given Files sub-menu and activate it
	int menu_pos = Menu_FindItem(items, M_GIVEN_FILES);

	if (menu_pos < 0)  // [should not happen]
		return items;

	items[menu_pos++].activate();


	// create new array
	int total = items[0].size();  // includes {0} at end

	Fl_Menu_Item * new_array = new Fl_Menu_Item[total + count];
	Fl_Menu_Item * pos = new_array;

	for (int i = 0 ; i < menu_pos ; i++)
		Menu_CopyItem(pos, items[i]);

	for (int k = 0 ; k < count ; k++)
	{
		const char *short_name = fl_filename_name(Pwad_list[k]);

		short_name = StringPrintf("%s%d. %s", (k < 9) ? "&" : "", 1+k, short_name);

		Menu_AddItem(pos, short_name,
					 FCAL file_do_load_given,
					 (void *) Pwad_list[k], 0);
	}

	for ( ; menu_pos < total ; menu_pos++)
		Menu_CopyItem(pos, items[menu_pos]);

	return new_array;
}


static Fl_Menu_Item * Menu_PopulateRecentFiles(Fl_Menu_Item *items, Fl_Callback *cb)
{
	int count = M_RecentCount();

	if (count < 1)
		return items;


	// find Recent Files sub-menu and activate it
	int menu_pos = Menu_FindItem(items, M_RECENT_FILES);

	if (menu_pos < 0)  // [should not happen]
		return items;

	items[menu_pos++].activate();


	// create new array
	int total = items[0].size();  // includes {0} at end

	Fl_Menu_Item * new_array = new Fl_Menu_Item[total + count];
	Fl_Menu_Item * pos = new_array;

	for (int i = 0 ; i < menu_pos ; i++)
		Menu_CopyItem(pos, items[i]);

	for (int k = 0 ; k < count ; k++)
	{
		char name_buf[256];

		M_RecentShortName(k, name_buf);

		void *data = M_RecentData(k);

		Menu_AddItem(pos, StringDup(name_buf), cb, data, 0);
	}

	for ( ; menu_pos < total ; menu_pos++)
		Menu_CopyItem(pos, items[menu_pos]);

	return new_array;
}


Fl_Sys_Menu_Bar * Menu_Create(int x, int y, int w, int h)
{
	Fl_Sys_Menu_Bar *bar = new Fl_Sys_Menu_Bar(x, y, w, h);

#ifndef __APPLE__
	bar->textsize(KF_fonth);
#endif

	Fl_Menu_Item *items = menu_items;

	items = Menu_PopulateGivenFiles(items);
	items = Menu_PopulateRecentFiles(items, FCAL file_do_load_recent);

	bar->menu(items);

	return bar;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
