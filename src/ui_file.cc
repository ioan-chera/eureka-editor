//------------------------------------------------------------------------
//  FILE-RELATED DIALOGS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2018 Andrew Apted
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
#include "m_config.h"
#include "m_files.h"
#include "m_game.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_file.h"


#define FREE_COL  fl_rgb_color(0x33, 0xFF, 0xAA)
#define USED_COL  (gui_scheme == 2 ? fl_rgb_color(0xFF, 0x11, 0x11) : fl_rgb_color(0xFF, 0x88, 0x88))


// TODO: find a better home for this
bool ValidateMapName(const char *p)
{
	size_t len = strlen(p);

	if (len == 0 || len > 8)
		return false;

	if (! isalpha(*p))
		return false;

	for ( ; *p ; p++)
		if (! (isalnum(*p) || *p == '_'))
			return false;

	return true;
}


UI_ChooseMap::UI_ChooseMap(const char *initial_name,
						   Wad_file *_rename_wad) :
	UI_Escapable_Window(420, 385, "Choose Map"),
	rename_wad(_rename_wad),
	action(ACT_none)
{
	resizable(NULL);

	callback(close_callback, this);

	{
		map_name = new Fl_Input(120, 35, 120, 25, "Map slot: ");
		map_name->labelfont(FL_HELVETICA_BOLD);
	}

	map_name->when(FL_WHEN_CHANGED);
	map_name->callback(input_callback, this);
	map_name->value(initial_name);

	Fl::focus(map_name);

	map_buttons = new Fl_Group(x(), y() + 60, w(), y() + 320);
	map_buttons->end();

	{
		int bottom_y = 320;

		Fl_Group* o = new Fl_Group(0, bottom_y, 420, 65);
		o->box(FL_FLAT_BOX);
		o->color(WINDOW_BG, WINDOW_BG);

		ok_but = new Fl_Return_Button(260, bottom_y + 17, 100, 35, "OK");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		Fl_Button *cancel = new Fl_Button(75, bottom_y + 17, 100, 35, "Cancel");
		cancel->callback(close_callback, this);

		o->end();
	}

	end();

	CheckMapName();
}


UI_ChooseMap::~UI_ChooseMap()
{
}


void UI_ChooseMap::PopulateButtons(char format, Wad_file *test_wad)
{
	int but_W = 60;

	for (int col = 0 ; col < 5 ; col++)
	for (int row = 0 ; row < 8 ; row++)
	{
		int cx = x() + 30 + col * (but_W + but_W / 5);
		int cy = y() + 80 + row * 24 + (row / 2) * 10;

		char name_buf[20];

		if (format == 'E')
		{
			int epi = 1 + row / 2;
			int map = 1 + col + (row & 1) * 5;

			if (map > 9)
				continue;

			sprintf(name_buf, "E%dM%d", epi, map);
		}
		else
		{
			int map = 1 + col + row * 5;

			// this logic matches UI_OpenMap on the IWAD
			if (row >= 2)
				map--;
			else if (row == 1 && col == 4)
				continue;

			if (map < 1 || map > 32)
				continue;

			sprintf(name_buf, "MAP%02d", map);
		}

		Fl_Button * but = new Fl_Button(cx, cy, 60, 20);
		but->copy_label(name_buf);
		but->callback(button_callback, this);

		if (test_wad && test_wad->LevelFind(name_buf) >= 0)
		{
			if (rename_wad)
				but->deactivate();
			else
				but->color(USED_COL);
		}
		else
			but->color(FREE_COL);

		map_buttons->add(but);
	}
}


const char * UI_ChooseMap::Run()
{
	set_modal();

	show();

	while (action == ACT_none)
	{
		Fl::wait(0.2);
	}

	if (action == ACT_CANCEL)
		return NULL;

	return StringUpper(map_name->value());
}


void UI_ChooseMap::close_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->action = ACT_CANCEL;
}


void UI_ChooseMap::ok_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	// santify check
	if (ValidateMapName(that->map_name->value()))
		that->action = ACT_ACCEPT;
	else
		fl_beep();
}


void UI_ChooseMap::button_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->map_name->value(w->label());
	that->action = ACT_ACCEPT;
}


void UI_ChooseMap::input_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->CheckMapName();
}


void UI_ChooseMap::CheckMapName()
{
	bool was_valid = ok_but->active();
	bool  is_valid = ValidateMapName(map_name->value());

	if (rename_wad && is_valid)
	{
		if (rename_wad->LevelFind(map_name->value()) >= 0)
			is_valid = false;
	}

	if (was_valid == is_valid)
		return;

	if (is_valid)
	{
		ok_but->activate();
		map_name->textcolor(FL_FOREGROUND_COLOR);
	}
	else
	{
		ok_but->deactivate();
		map_name->textcolor(FL_RED);
	}

	map_name->redraw();
}


//------------------------------------------------------------------------


UI_OpenMap::UI_OpenMap() :
	UI_Escapable_Window(420, 475, "Open Map"),
	action(ACT_none),
	loaded_wad(NULL),
	 using_wad(NULL)
{
	resizable(NULL);

	callback(close_callback, this);

	{
		look_where = new Fl_Choice(130, 80, 190, 25, "Find map in:  ");
		look_where->labelfont(FL_HELVETICA_BOLD);
		look_where->add("the PWAD above|the Game IWAD|the Resource wads");
		look_where->callback(look_callback, this);

		look_where->value(edit_wad ? LOOK_PWad : LOOK_IWad);
	}

	{
		Fl_Box* o = new Fl_Box(15, 15, 270, 20, "PWAD file:");
		o->labelfont(FL_HELVETICA_BOLD);
		o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}

	pwad_name = new Fl_Output(20, 40, 295, 26);

	Fl_Button *load_but = new Fl_Button(330, 39, 65, 28, "Load");
	load_but->callback(load_callback, this);


	map_name = new Fl_Input(99, 125, 100, 26, "Map slot: ");
	map_name->labelfont(FL_HELVETICA_BOLD);
	map_name->when(FL_WHEN_CHANGED);
	map_name->callback(input_callback, this);

	{
		Fl_Box *o = new Fl_Box(230, 125, 180, 26, "Available maps:");
		// o->labelfont(FL_HELVETICA_BOLD);
		o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}


	// all the map buttons go into this group

	button_grp = new UI_Scroll(5, 165, w()-10, 230, +1 /* bar_side */);
	button_grp->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
	button_grp->resize_horiz(false);
	button_grp->Line_size(24);
	button_grp->box(FL_FLAT_BOX);

	/* bottom buttons */

	{
		int bottom_y = h() - 70;

		Fl_Group* o = new Fl_Group(0, bottom_y, w(), 70);
		o->box(FL_FLAT_BOX);
		o->color(WINDOW_BG, WINDOW_BG);

		ok_but = new Fl_Return_Button(260, bottom_y + 20, 100, 34, "OK");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		Fl_Button * cancel = new Fl_Button(75, bottom_y + 20, 100, 35, "Cancel");
		cancel->callback(close_callback, this);

		o->end();
	}

	end();

	CheckMapName();
}


UI_OpenMap::~UI_OpenMap()
{ }


Wad_file * UI_OpenMap::Run(const char ** map_v, bool * did_load)
{
	*map_v = NULL;
	*did_load = false;

	if (edit_wad)
		SetPWAD(edit_wad->PathName());

	Populate();

	set_modal();
	show();

	while (action == ACT_none)
	{
		Fl::wait(0.2);
	}

	if (action != ACT_ACCEPT)
		using_wad = NULL;

	if (using_wad)
	{
		*map_v = StringUpper(map_name->value());

		if (using_wad == loaded_wad)
		{
			*did_load  = true;
			loaded_wad = NULL;
		}
	}

	// if we are not returning a pwad which got loaded, e.g. because
	// the user cancelled or chose the game IWAD, then close it now.
	if (loaded_wad)
		delete loaded_wad;

	return using_wad;
}


void UI_OpenMap::CheckMapName()
{
	bool was_valid = ok_but->active();

	bool  is_valid = (using_wad != NULL) &&
	                 ValidateMapName(map_name->value()) &&
					 (using_wad->LevelFind(map_name->value()) >= 0);

	if (was_valid == is_valid)
		return;

	if (is_valid)
	{
		ok_but->activate();
		map_name->textcolor(FL_FOREGROUND_COLOR);
	}
	else
	{
		ok_but->deactivate();
		map_name->textcolor(FL_RED);
	}

	map_name->redraw();
}


void UI_OpenMap::Populate()
{
	button_grp->label("\n\n\n\n\nNO   MAPS   FOUND");
	button_grp->Remove_all();

	using_wad = NULL;

	if (look_where->value() == LOOK_IWad)
	{
		using_wad = game_wad;
		PopulateButtons();
	}
	else if (look_where->value() >= LOOK_Resource)
	{
		int first = 1;
		int last  = (int)master_dir.size() - 1;

		if (edit_wad)
			last--;

		// we simply use the last resource which contains levels

		// TODO: probably should collect ones with a map, add to look_where choices

		for (int r = last ; r >= first ; r--)
		{
			if (master_dir[r]->LevelCount() >= 0)
			{
				using_wad = master_dir[r];
				PopulateButtons();
				break;
			}
		}
	}
	else if (loaded_wad)
	{
		using_wad = loaded_wad;
		PopulateButtons();
	}
	else if (edit_wad)
	{
		using_wad = edit_wad;
		PopulateButtons();
	}

	button_grp->Init_sizes();
	button_grp->redraw();
}


static bool DifferentEpisode(const char *A, const char *B)
{
	if (A[0] != B[0])
		return true;

	// handle ExMx
	if (toupper(A[0]) == 'E')
	{
		return A[1] != B[1];
	}

	// handle MAPxx
	if (strlen(A) < 4 && strlen(B) < 4)
		return false;

	return A[3] != B[3];
}


void UI_OpenMap::PopulateButtons()
{
	Wad_file *wad = using_wad;
	SYS_ASSERT(wad);

	int num_levels = wad->LevelCount();

	if (num_levels == 0)
		return;

	button_grp->label("");

	std::map<std::string, int> level_names;
	std::map<std::string, int>::iterator IT;

	for (int lev = 0 ; lev < num_levels ; lev++)
	{
		Lump_c *lump = wad->GetLump(wad->LevelHeader(lev));

		level_names[std::string(lump->Name())] = 1;
	}

	int cx_base = button_grp->x() + 25;
	int cy_base = button_grp->y() + 5;
	int but_W   = 60;

	// create them buttons!!

	int row = 0;
	int col = 0;

	const char *last_name = NULL;

	for (IT = level_names.begin() ; IT != level_names.end() ; IT++)
	{
		const char *name = IT->first.c_str();

		if (col > 0 && last_name && DifferentEpisode(last_name, name))
		{
			col = 0;
			row++;
		}

		int cx = cx_base + col * (but_W + but_W / 5);
		int cy = cy_base + row * 24 + (row / 2) * 8;

		Fl_Button * but = new Fl_Button(cx, cy, but_W, 20);
		but->copy_label(name);
		but->color(FREE_COL);
		but->callback(button_callback, this);

		button_grp->Add(but);

		col++;
		if (col >= 5)
		{
			col = 0;
			row++;
		}

		last_name = name;
	}

	redraw();
}


void UI_OpenMap::SetPWAD(const char *name)
{
	pwad_name->value(fl_filename_name(name));
}


void UI_OpenMap::close_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	that->action = ACT_CANCEL;
}


void UI_OpenMap::ok_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	// santify check
	if (that->using_wad && ValidateMapName(that->map_name->value()))
		that->action = ACT_ACCEPT;
	else
		fl_beep();
}


void UI_OpenMap::button_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	// sanity check
	if (! that->using_wad)
		return;

	that->map_name->value(w->label());
	that->action = ACT_ACCEPT;
}


void UI_OpenMap::input_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	that->CheckMapName();
}


void UI_OpenMap::look_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	that->Populate();
	that->CheckMapName();
}


void UI_OpenMap::load_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	that->LoadFile();
	that->CheckMapName();
}


void UI_OpenMap::LoadFile()
{
	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to open");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.filter("Wads\t*.wad");
	chooser.directory(Main_FileOpFolder());

	// Show native chooser
	switch (chooser.show())
	{
		case -1:
			LogPrintf("Open Map: error choosing file:\n");
			LogPrintf("   %s\n", chooser.errmsg());

			DLG_Notify("Unable to open the map:\n\n%s",
					   chooser.errmsg());
			return;

		case 1:
			LogPrintf("Open Map: cancelled by user\n");
			return;

		default:
			break;  // OK
	}


	Wad_file * wad = Wad_file::Open(chooser.filename(), 'a');

	if (! wad)
	{
		// FIXME: get an error message, add it here

		DLG_Notify("Unable to open the chosen WAD file.\n\n"
				   "Please try again.");
		return;
	}

	if (wad->LevelCount() < 0)
	{
		DLG_Notify("The chosen WAD contains no levels.\n\n"
				   "Please try again.");
		return;
	}


	// replace existing one
	if (loaded_wad)
		delete loaded_wad;

	loaded_wad = wad;

	SetPWAD(loaded_wad->PathName());

	if (using_wad == loaded_wad)
		using_wad = wad;


	// change the "Find map in ..." setting
	look_where->value(LOOK_PWad);

	Populate();
}


//------------------------------------------------------------------------


UI_ProjectSetup * UI_ProjectSetup::_instance = NULL;


#define STARTUP_MSG  "No IWADs could be found."


UI_ProjectSetup::UI_ProjectSetup(bool new_project, bool is_startup) :
	UI_Escapable_Window(400, is_startup ? 200 : 400, new_project ? "New Project" : "Manage Project"),
	action(ACT_none),
	game(NULL), port(NULL), map_format(MAPF_INVALID)
{
	callback(close_callback, this);

	resizable(NULL);

	_instance = this;  // meh, hacky

	int by = 0;

	if (is_startup)
	{
		Fl_Box * message = new Fl_Box(FL_FLAT_BOX, 15, 15, 370, 46, STARTUP_MSG);
		message->align(FL_ALIGN_INSIDE);
		message->color(FL_RED, FL_RED);
		message->labelcolor(FL_YELLOW);
		message->labelsize(18);

		by += 60;
	}

	game_choice = new Fl_Choice(140, by+25, 150, 29, "Game IWAD: ");
	game_choice->labelfont(FL_HELVETICA_BOLD);
	game_choice->down_box(FL_BORDER_BOX);
	game_choice->callback((Fl_Callback*)game_callback, this);

	{
		Fl_Button* o = new Fl_Button(305, by+27, 75, 25, "Find");
		o->callback((Fl_Callback*)find_callback, this);
	}

	port_choice = new Fl_Choice(140, by+60, 150, 29, "Source Port: ");
	port_choice->labelfont(FL_HELVETICA_BOLD);
	port_choice->down_box(FL_BORDER_BOX);
	port_choice->callback((Fl_Callback*)port_callback, this);

	{
		Fl_Button* o = new Fl_Button(305, by+60, 75, 25, "Setup");
		o->callback((Fl_Callback*)setup_callback, this);

		if (is_startup)
			o->hide();
	}

	format_choice = new Fl_Choice(140, by+95, 150, 29, "Map Type: ");
	format_choice->labelfont(FL_HELVETICA_BOLD);
	format_choice->down_box(FL_BORDER_BOX);
	format_choice->callback((Fl_Callback*)format_callback, this);

	if (is_startup)
	{
		  port_choice->hide();
		format_choice->hide();
	}

	// Resource section

	if (! is_startup)
	{
		Fl_Box *res_title = new Fl_Box(15, by+135, 185, 35, "Resource Files:");
		res_title->labelfont(FL_HELVETICA_BOLD);
		res_title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}

	for (int r = 0 ; r < RES_NUM ; r++)
	{
		res[r] = NULL;

		if (is_startup)
			continue;

		int cy = by + 172 + r * 35;

		char res_label[64];
		sprintf(res_label, "%d. ", 1 + r);

		res_name[r] = new Fl_Output(55, cy, 205, 25);
		res_name[r]->copy_label(res_label);

		Fl_Button *kill = new Fl_Button(270, cy, 30, 25, "x");
		kill->labelsize(20);
		kill->callback((Fl_Callback*)kill_callback, (void *)(long)r);

		Fl_Button *load = new Fl_Button(315, cy, 75, 25, "Load");
		load->callback((Fl_Callback*)load_callback, (void *)(long)r);
	}

	// bottom buttons
	{
		by += is_startup ? 80 : 340;

		Fl_Group *g = new Fl_Group(0, by, 400, h() - by);
		g->box(FL_FLAT_BOX);
		g->color(WINDOW_BG, WINDOW_BG);

		const char *cancel_text = is_startup ? "Quit" : "Cancel";

		cancel = new Fl_Button(90, g->y() + 14, 80, 35, cancel_text);
		cancel->callback((Fl_Callback*)close_callback, this);

		const char *ok_text = (is_startup | new_project) ? "OK" : "Use";

		ok_but = new Fl_Button(240, g->y() + 14, 80, 35, ok_text);
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback((Fl_Callback*)use_callback, this);

		g->end();
	}

	end();
}


UI_ProjectSetup::~UI_ProjectSetup()
{
	_instance = NULL;
}


bool UI_ProjectSetup::Run()
{
	PopulateIWADs();
	PopulatePort();
	PopulateMapFormat();
	PopulateResources();

	set_modal();

	show();

	while (action == ACT_none)
	{
		Fl::wait(0.2);
	}

	return (action == ACT_ACCEPT);
}


void UI_ProjectSetup::PopulateIWADs()
{
	// This is called (a) when dialog is first opened, or (b) when
	// the user has found a new iwad.  For the latter case, we want
	// to show the newly found game.

	const char *prev_game = game;

	if (! prev_game) prev_game = Game_name;
	if (! prev_game) prev_game = "doom2";


	game = NULL;

	game_choice->clear();


	const char *menu_string;
	int menu_value = 0;

	menu_string = M_CollectGamesForMenu(&menu_value, prev_game);

	if (menu_string[0])
	{
		game_choice->add(menu_string);
		game_choice->value(menu_value);

		game = StringDup(game_choice->mvalue()->text);
	}

	if (game)
		ok_but->activate();
	else
		ok_but->deactivate();
}


void UI_ProjectSetup::PopulatePort()
{
	const char *prev_port = NULL;

	if (port_choice->mvalue())
		prev_port = StringDup(port_choice->mvalue()->text);

	if (! prev_port) prev_port = Port_name;
	if (! prev_port) prev_port = "vanilla";


	port = "vanilla";

	port_choice->clear();

	// if no game, port doesn't matter
	if (! game)
		return;


	const char *var_game = NULL;

	if (game_choice->mvalue())
		var_game = M_VariantForGame(game_choice->mvalue()->text);
	else if (Game_name)
		var_game = M_VariantForGame(Game_name);

	if (! var_game)
		var_game = "doom2";


	const char *menu_string;
	int menu_value = 0;

	menu_string = M_CollectPortsForMenu(var_game, &menu_value, prev_port);

	if (menu_string[0])
	{
		port_choice->add  (menu_string);
		port_choice->value(menu_value);

		port = StringDup(port_choice->mvalue()->text);
	}
}


void UI_ProjectSetup::PopulateMapFormat()
{
	map_format_e prev_fmt = Level_format;

	if (format_choice->mvalue())
	{
		if (strstr(format_choice->mvalue()->text, "UDMF"))
			prev_fmt = MAPF_UDMF;
		else if (strstr(format_choice->mvalue()->text, "Hexen"))
			prev_fmt = MAPF_Hexen;
		else
			prev_fmt = MAPF_Doom;
	}


	map_format = MAPF_Doom;

	format_choice->clear();

	// if no game, format doesn't matter
	if (! game)
		return;


	// determine the usable formats, from current game and port
	const char *c_game = "doom2";
	const char *c_port = "vanilla";

	if (game_choice->mvalue())
		c_game = game_choice->mvalue()->text;

	if (port_choice->mvalue())
		c_port = port_choice->mvalue()->text;

	usable_formats = M_DetermineMapFormats(c_game, c_port);

	SYS_ASSERT(usable_formats != 0);


	// reconstruct the menu
	char menu_string[256];
	int  menu_value = 0;

	menu_string[0] = 0;

	int entry_id = 0;

	if (usable_formats & (1 << MAPF_Doom))
	{
		strcat(menu_string, "Doom Format");
		entry_id++;
	}

	if (usable_formats & (1 << MAPF_Hexen))
	{
		if (prev_fmt == MAPF_Hexen)
			menu_value = entry_id;

		if (menu_string[0])
			strcat(menu_string, "|");

		strcat(menu_string, "Hexen Format");
		entry_id++;
	}

	if (true)  // FIXME??  usable_formats & (1 << MAPF_UDMF))
	{
		if (prev_fmt == MAPF_UDMF)
			menu_value = entry_id;

		if (menu_string[0])
			strcat(menu_string, "|");

		strcat(menu_string, "UDMF");
		entry_id++;
	}

	format_choice->add  (menu_string);
	format_choice->value(menu_value);

	// FIXME explain this
	if (usable_formats & (1 << MAPF_Hexen))
	{
		if (prev_fmt == MAPF_Hexen ||
			(usable_formats & (1 << MAPF_Doom)) == 0)
		{
			map_format = MAPF_Hexen;
		}
	}
}


void UI_ProjectSetup::PopulateResources()
{
	// Note: these resource wads may be invalid (not exist) during startup.
	//       This is probably NOT the place to validate them...

	for (int r = 0 ; r < RES_NUM ; r++)
	{
		// the resource widgets are not created for the missing-iwad dialog
		if (! res_name[r])
			continue;

		if (r < (int)Resource_list.size())
		{
			res[r] = StringDup(Resource_list[r]);

			res_name[r]->value(fl_filename_name(res[r]));
		}
	}
}


void UI_ProjectSetup::close_callback(Fl_Widget *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	that->action = ACT_CANCEL;
}


void UI_ProjectSetup::use_callback(Fl_Button *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	that->action = ACT_ACCEPT;
}


void UI_ProjectSetup::game_callback(Fl_Choice *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	const char * name = w->mvalue()->text;

	if (M_QueryKnownIWAD(name))
	{
		that->game = StringDup(name);
		that->ok_but->activate();
	}
	else
	{
		that->game = NULL;
		that->ok_but->deactivate();

		fl_beep();
	}

	that->PopulatePort();
	that->PopulateMapFormat();
}


void UI_ProjectSetup::port_callback(Fl_Choice *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	const char * name = w->mvalue()->text;

	that->port = StringDup(name);

	that->PopulateMapFormat();
}


void UI_ProjectSetup::format_callback(Fl_Choice *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	const char * fmt_str = w->mvalue()->text;

	if (strstr(fmt_str, "UDMF"))
		that->map_format = MAPF_UDMF;
	else if (strstr(fmt_str, "Hexen"))
		that->map_format = MAPF_Hexen;
	else
		that->map_format = MAPF_Doom;
}


void UI_ProjectSetup::find_callback(Fl_Button *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to open");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.filter("Wads\t*.wad");
	chooser.directory(Main_FileOpFolder());

	switch (chooser.show())
	{
		case -1:  // error
			DLG_Notify("Unable to open that wad:\n\n%s", chooser.errmsg());
			return;

		case 1:  // cancelled
			return;

		default:
			break;  // OK
	}

	// check that a game definition exists

	const char *game = GameNameFromIWAD(chooser.filename());

	if (! M_CanLoadDefinitions("games", game))
	{
		DLG_Notify("That game is not supported (no definition file).\n\n"
		           "Please try again.");
		return;
	}


	M_AddKnownIWAD(chooser.filename());
	M_SaveRecent();

	that->game = StringDup(game);

	that->PopulateIWADs();
	that->PopulatePort();
	that->PopulateMapFormat();
}


// m_testmap.cc
extern bool M_PortSetupDialog(const char *port, const char *game);


void UI_ProjectSetup::setup_callback(Fl_Button *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	// FIXME : deactivate button when this is true
	if (that->game == NULL || that->port == NULL)
	{
		fl_beep();
		return;
	}

	M_PortSetupDialog(that->port, that->game);
}


void UI_ProjectSetup::load_callback(Fl_Button *w, void *data)
{
	int r = (int)(long)data;
	SYS_ASSERT(0 <= r && r < RES_NUM);

	UI_ProjectSetup * that = _instance;
	SYS_ASSERT(that);

	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to open");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.filter("Wads\t*.wad\nEureka defs\t*.ugh");
	chooser.directory(Main_FileOpFolder());

	switch (chooser.show())
	{
		case -1:  // error
			DLG_Notify("Unable to open that wad:\n\n%s", chooser.errmsg());
			return;

		case 1:  // cancelled
			return;

		default:
			break;  // OK
	}

	if (that->res[r])
		StringFree(that->res[r]);

	that->res[r] = StringDup(chooser.filename());

	that->res_name[r]->value(fl_filename_name(that->res[r]));
}


void UI_ProjectSetup::kill_callback(Fl_Button *w, void *data)
{
	int r = (int)(long)data;
	SYS_ASSERT(0 <= r && r < RES_NUM);

	UI_ProjectSetup * that = _instance;
	SYS_ASSERT(that);

	if (that->res[r])
	{
		StringFree(that->res[r]);
		that->res[r] = NULL;

		that->res_name[r]->value("");
	}
	else
		fl_beep();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
