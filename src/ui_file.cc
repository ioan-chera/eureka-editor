//------------------------------------------------------------------------
//  FILE-RELATED DIALOGS
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
#include "m_files.h"
#include "m_game.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_file.h"

#include <set>

#define FREE_COL  fl_rgb_color(0x33, 0xFF, 0xAA)
#define USED_COL  (config::gui_scheme == 2 ? fl_rgb_color(0xFF, 0x11, 0x11) : fl_rgb_color(0xFF, 0x88, 0x88))


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
						   const std::shared_ptr<const Wad_file> &_rename_wad) :
	UI_Escapable_Window(420, 385, "Choose Map"),
	rename_wad(_rename_wad)
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

	FLFocusOnCreation(map_name);

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

void UI_ChooseMap::PopulateButtons(char format, const Wad_file *test_wad)
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

			snprintf(name_buf, sizeof(name_buf), "E%dM%d", epi, map);
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

			snprintf(name_buf, sizeof(name_buf), "MAP%02d", map);
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


SString UI_ChooseMap::Run()
{
	set_modal();

	show();

	while (action == ACT_none)
	{
		Fl::wait(0.2);
	}

	if (action == ACT_CANCEL)
		return "";

	return SString(map_name->value()).asUpper();
}


void UI_ChooseMap::close_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->action = ACT_CANCEL;
}


void UI_ChooseMap::ok_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	// sanity check
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


UI_OpenMap::UI_OpenMap(Instance &inst) :
	UI_Escapable_Window(420, 475, "Open Map"), inst(inst)
{
	resizable(NULL);

	callback(close_callback, this);

	{
		look_where = new Fl_Choice(130, 80, 190, 25, "Find map in:  ");
		look_where->labelfont(FL_HELVETICA_BOLD);
		look_where->add("the PWAD above|the Game IWAD|the Resource wads");
		look_where->callback(look_callback, this);

		look_where->value(inst.wad.master.editWad() ? LOOK_PWad : LOOK_IWad);
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


std::shared_ptr<Wad_file> UI_OpenMap::Run(SString* map_v, bool * did_load)
{
	map_v->clear();
	*did_load = false;

	if (inst.wad.master.editWad())
		SetPWAD(inst.wad.master.editWad()->PathName().u8string());

	Populate();

	set_modal();
	show();

	while (action == Action::none)
	{
		Fl::wait(0.2);
	}

	if (action != Action::accept)
		using_wad.reset();

	if (using_wad)
	{
		*map_v = SString(map_name->value()).asUpper();

		if (using_wad == loaded_wad)
		{
			*did_load  = true;
			loaded_wad.reset();
		}
	}

	// if we are not returning a pwad which got loaded, e.g. because
	// the user cancelled or chose the game IWAD, then close it now.
	loaded_wad.reset();

	return using_wad;
}


void UI_OpenMap::CheckMapName()
{
	bool was_valid = ok_but->active();

	bool  is_valid = using_wad &&
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

	using_wad.reset();

	if (look_where->value() == LOOK_IWad)
	{
		using_wad = inst.wad.master.gameWad();
		PopulateButtons();
	}
	else if (look_where->value() >= LOOK_Resource)
	{
		// we simply use the last resource which contains levels

		// TODO: probably should collect ones with a map, add to look_where choices

		for (auto it = inst.wad.master.resourceWads().rbegin();
			 it != inst.wad.master.resourceWads().rend(); ++it)
		{
			if ((*it)->LevelCount() >= 0)
			{
				using_wad = *it;
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
	else if (inst.wad.master.editWad())
	{
		using_wad = inst.wad.master.editWad();
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
	std::shared_ptr<const Wad_file> wad = using_wad;
	SYS_ASSERT(wad);

	int num_levels = wad->LevelCount();

	if (num_levels == 0)
		return;

	button_grp->label("");

	std::set<SString> level_names;

	for (int lev = 0 ; lev < num_levels ; lev++)
	{
		Lump_c *lump = wad->GetLump(wad->LevelHeader(lev));

		level_names.insert(lump->Name());
	}

	int cx_base = button_grp->x() + 25;
	int cy_base = button_grp->y() + 5;
	int but_W   = 60;

	// create them buttons!!

	int row = 0;
	int col = 0;

	SString last_name;

	for (const SString &name : level_names)
	{
		if (col > 0 && !last_name.empty() && DifferentEpisode(last_name.c_str(), name.c_str()))
		{
			col = 0;
			row++;
		}

		int cx = cx_base + col * (but_W + but_W / 5);
		int cy = cy_base + row * 24 + (row / 2) * 8;

		Fl_Button * but = new Fl_Button(cx, cy, but_W, 20);
		but->copy_label(name.c_str());
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


void UI_OpenMap::SetPWAD(const SString &name)
{
	pwad_name->value(fl_filename_name(name.c_str()));
}


void UI_OpenMap::close_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	that->action = Action::cancel;
}


void UI_OpenMap::ok_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	// sanity check
	if (that->using_wad && ValidateMapName(that->map_name->value()))
		that->action = Action::accept;
	else
		fl_beep();
}


void UI_OpenMap::button_callback(Fl_Widget *w, void *data)
{
	auto that = (UI_OpenMap *)data;

	// sanity check
	if (! that->using_wad)
		return;

	that->map_name->value(w->label());
	that->action = Action::accept;
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
	chooser.directory(inst.Main_FileOpFolder().u8string().c_str());

	// Show native chooser
	switch (chooser.show())
	{
		case -1:
			gLog.printf("Open Map: error choosing file:\n");
			gLog.printf("   %s\n", chooser.errmsg());

			DLG_Notify("Unable to open the map:\n\n%s",
					   chooser.errmsg());
			return;

		case 1:
			gLog.printf("Open Map: cancelled by user\n");
			return;

		default:
			break;  // OK
	}


	std::shared_ptr<Wad_file> wad = Wad_file::Open(fs::u8path(chooser.filename()),
												   WadOpenMode::append);

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

	loaded_wad = wad;

	SetPWAD(loaded_wad->PathName().u8string());

	if (using_wad == loaded_wad)
		using_wad = wad;


	// change the "Find map in ..." setting
	look_where->value(LOOK_PWad);

	Populate();
}


//------------------------------------------------------------------------

#define STARTUP_MSG  "No IWADs could be found."


UI_ProjectSetup::UI_ProjectSetup(Instance &inst, bool new_project, bool is_startup) :
	UI_Escapable_Window(is_startup ? 400 : 500, is_startup ? 200 : 440, new_project ? "New Project" : "Manage Project"),
	inst(inst)
{
	callback(close_callback, this);

	resizable(NULL);
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

	port_choice = new Fl_Choice(140, by+62, 150, 29, "Source Port: ");
	port_choice->labelfont(FL_HELVETICA_BOLD);
	port_choice->down_box(FL_BORDER_BOX);
	port_choice->callback((Fl_Callback*)port_callback, this);

	{
		Fl_Button* o = new Fl_Button(305, by+64, 75, 25, "Setup");
		o->callback((Fl_Callback*)setup_callback, this);

		if (is_startup)
			o->hide();
	}

	format_choice = new Fl_Choice(140, by+99, 150, 29, "Map Type: ");
	format_choice->labelfont(FL_HELVETICA_BOLD);
	format_choice->down_box(FL_BORDER_BOX);
	format_choice->callback((Fl_Callback*)format_callback, this);

#if 0  // Disabled for now
	namespace_choice = new Fl_Choice(140, by+140, 150, 29, "Namespace: ");
	namespace_choice->labelfont(FL_HELVETICA_BOLD);
	namespace_choice->down_box(FL_BORDER_BOX);
	namespace_choice->callback((Fl_Callback*)namespace_callback, this);
	namespace_choice->hide();
#endif

	if (is_startup)
	{
		port_choice->hide();
		format_choice->hide();
	}

	// Resource section

	if (! is_startup)
	{
		Fl_Box *res_title = new Fl_Box(15, by+190, 185, 30, "Resource Files or Folders:");
		res_title->labelfont(FL_HELVETICA_BOLD);
		res_title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}

	for (int r = 0 ; r < RES_NUM ; r++)
	{
		result.resources[r].clear();

		if (is_startup)
			continue;

		int cy = by + 220 + r * 35;

		char res_label[64];
		snprintf(res_label, sizeof(res_label), "%d. ", 1 + r);

		res_name[r] = new Fl_Output(55, cy, 205, 25);
		res_name[r]->copy_label(res_label);

		Fl_Button *kill = new Fl_Button(270, cy, 30, 25, "x");
		mClearButtons[r] = kill;
		kill->labelsize(20);
		kill->callback((Fl_Callback*)kill_callback, this);

		// NOTE: on Apple, when selecting a folder picker, we can already pick files too, so unify the buttons
#ifdef __APPLE__
		Fl_Button *load = new Fl_Button(315, cy, 75 + 10 + 90, 25, "Load File or Folder");
		mResourceFileButtons[r] = load;
		load->callback((Fl_Callback*)load_callback, this);
#else
		Fl_Button *load = new Fl_Button(315, cy, 75, 25, "Load File");
		mResourceFileButtons[r] = load;
		load->callback((Fl_Callback*)load_callback, this);
		
		load = new Fl_Button(load->x() + load->w() + 10, cy, 90, 25, "Load Folder");
		mResourceDirButtons[r] = load;
		load->callback((Fl_Callback*)load_callback, this);
#endif
	}

	// bottom buttons
	{
		by += is_startup ? 80 : 375;

		Fl_Group *g = new Fl_Group(0, by, w(), h() - by);
		g->box(FL_FLAT_BOX);
		g->color(WINDOW_BG, WINDOW_BG);

		const char *cancel_text = is_startup ? "Quit" : "Cancel";

		cancel = new Fl_Button(90, g->y() + 14, 80, 35, cancel_text);
		cancel->callback((Fl_Callback*)close_callback, this);

		const char *ok_text = (is_startup | new_project) ? "OK" : "Use";

		ok_but = new Fl_Button(w() - 160, g->y() + 14, 80, 35, ok_text);
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback((Fl_Callback*)use_callback, this);

		g->end();
	}

	end();
}


tl::optional<UI_ProjectSetup::Result> UI_ProjectSetup::Run()
{
	PopulateIWADs();
	PopulatePort();
	PopulateMapFormat();
	PopulateResources();

	set_modal();

	show();

	while (action == Action::none)
	{
		Fl::wait(0.2);
	}

	return (action == Action::accept) ? result : tl::optional<UI_ProjectSetup::Result>{};
}

void UI_ProjectSetup::PopulateIWADs()
{
	// This is called (a) when dialog is first opened, or (b) when
	// the user has found a new iwad.  For the latter case, we want
	// to show the newly found game.

	SString prev_game = result.game;

	if (prev_game.empty())
		prev_game = inst.loaded.gameName;
	if (prev_game.empty())
		prev_game = "doom2";


	result.game.clear();
	game_choice->clear();


	SString menu_string;
	int menu_value = 0;

	menu_string = global::recent.collectGamesForMenu(&menu_value, prev_game.c_str());

	if (!menu_string.empty())
	{
		game_choice->add(menu_string.c_str());
		game_choice->value(menu_value);

		result.game = game_choice->mvalue()->text;
	}

	if (!result.game.empty())
		ok_but->activate();
	else
		ok_but->deactivate();
}


void UI_ProjectSetup::PopulatePort()
{
	SString prev_port;

	if (port_choice->mvalue())
		prev_port = port_choice->mvalue()->text;

	if (prev_port.empty())
		prev_port = inst.loaded.portName;
	if (prev_port.empty())
		prev_port = "vanilla";


	result.port = "vanilla";

	port_choice->clear();

	// if no game, port doesn't matter
	if (result.game.empty())
		return;


	SString base_game;

	if (game_choice->mvalue())
		base_game = M_GetBaseGame(game_choice->mvalue()->text);
	else if (!inst.loaded.gameName.empty())
		base_game = M_GetBaseGame(inst.loaded.gameName);

	if (base_game.empty())
		base_game = "doom2";


	int menu_value = 0;

	SString menu_string = M_CollectPortsForMenu(base_game.c_str(), &menu_value, prev_port.c_str());

	if (!menu_string.empty())
	{
		port_choice->add  (menu_string.c_str());
		port_choice->value(menu_value);

		result.port = port_choice->mvalue()->text;
	}
}


void UI_ProjectSetup::PopulateMapFormat()
{
	MapFormat prev_fmt = result.mapFormat;

	if (prev_fmt == MapFormat::invalid)
		prev_fmt = inst.loaded.levelFormat;


	format_choice->clear();

	// if no game, format doesn't matter
	if (result.game.empty())
	{
		result.mapFormat = MapFormat::doom;
		result.nameSpace = "";
		return;
	}


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
	int menu_value = 0;
	int entry_id = 0;

	if (usable_formats & (1 << static_cast<int>(MapFormat::doom)))
	{
		format_choice->add("Doom Format");
		entry_id++;
	}

	if (usable_formats & (1 << static_cast<int>(MapFormat::hexen)))
	{
		if (prev_fmt == MapFormat::hexen)
			menu_value = entry_id;

		format_choice->add("Hexen Format");
		entry_id++;
	}

	if (global::udmf_testing && (usable_formats & (1 << static_cast<int>(MapFormat::udmf))))
	{
		if (prev_fmt == MapFormat::udmf)
			menu_value = entry_id;

		format_choice->add("UDMF");
		entry_id++;
	}

	format_choice->value(menu_value);

	// set map_format field based on current menu entry.
	format_callback(format_choice, (void *)this);


	// determine the UDMF namespace
	result.nameSpace = "";

	const PortInfo_c *pinfo = M_LoadPortInfo(port_choice->mvalue()->text);
	if (pinfo)
		result.nameSpace = pinfo->udmf_namespace;

	// don't leave namespace as "" when chosen format is UDMF.
	// [ this is to handle broken config files somewhat sanely ]
	if (result.nameSpace.empty() && result.mapFormat == MapFormat::udmf)
		result.nameSpace = "Hexen";
}


void UI_ProjectSetup::PopulateNamespaces()
{
#if 0  // Disabled for now

	if (map_format != MAPF_UDMF)
	{
		namespace_choice->hide();
		return;
	}

	namespace_choice->show();

	// get previous value
	const char *prev_ns = name_space.c_str();

	if (prev_ns[0] == 0)
		prev_ns = Udmf_namespace.c_str();


	namespace_choice->clear();

	if (! port_choice->mvalue())
		return;

	PortInfo_c *pinfo = M_LoadPortInfo(port_choice->mvalue()->text);
	if (! pinfo)
		return;

	int menu_value = 0;

	for (int i = 0 ; i < (int)pinfo->namespaces.size() ; i++)
	{
		const char * ns = pinfo->namespaces[i].c_str();

		namespace_choice->add(ns);

		// keep same entry as before, when possible
		if (strcmp(prev_ns, ns) == 0)
			menu_value = i;
	}

	namespace_choice->value(menu_value);

	if (menu_value < (int)pinfo->namespaces.size())
		name_space = pinfo->namespaces[menu_value];
#endif
}

inline static fs::path fileOrDirName(const fs::path &path)
{
	return path.has_filename() ? path.filename() : path.parent_path().filename();
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

		if (r < (int)inst.loaded.resourceList.size())
		{
			result.resources[r] = inst.loaded.resourceList[r];

			res_name[r]->value(fileOrDirName(result.resources[r]).u8string().c_str());
		}
	}
}


void UI_ProjectSetup::close_callback(Fl_Widget *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	that->action = Action::cancel;
}


void UI_ProjectSetup::use_callback(Fl_Button *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	that->action = Action::accept;
}


void UI_ProjectSetup::game_callback(Fl_Choice *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	const char * name = w->mvalue()->text;

	if (global::recent.queryIWAD(name))
	{
		that->result.game = name;
		that->ok_but->activate();
	}
	else
	{
		that->result.game.clear();
		that->ok_but->deactivate();
	}

	that->PopulatePort();
	that->PopulateMapFormat();
}


void UI_ProjectSetup::port_callback(Fl_Choice *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	const char * name = w->mvalue()->text;

	that->result.port = name;

	that->PopulateMapFormat();
}


void UI_ProjectSetup::format_callback(Fl_Choice *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	const char * fmt_str = w->mvalue()->text;

	if (strstr(fmt_str, "UDMF"))
		that->result.mapFormat = MapFormat::udmf;
	else if (strstr(fmt_str, "Hexen"))
		that->result.mapFormat = MapFormat::hexen;
	else
		that->result.mapFormat = MapFormat::doom;

	that->PopulateNamespaces();
}


void UI_ProjectSetup::namespace_callback(Fl_Choice *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	that->result.nameSpace = w->mvalue()->text;
}


void UI_ProjectSetup::find_callback(Fl_Button *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to open");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.filter("Wads\t*.wad");
	chooser.directory(that->inst.Main_FileOpFolder().u8string().c_str());

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

	SString game = GameNameFromIWAD(fs::u8path(chooser.filename()));

	if (! M_CanLoadDefinitions(global::home_dir, global::install_dir, GAMES_DIR, game))
	{
		DLG_Notify("That game is not supported (no definition file).\n\n"
		           "Please try again.");
		return;
	}

	global::recent.addIWAD(fs::u8path(chooser.filename()));
	global::recent.save(global::home_dir);

	that->result.game = game;

	that->PopulateIWADs();
	that->PopulatePort();
	that->PopulateMapFormat();
}

void UI_ProjectSetup::setup_callback(Fl_Button *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	// FIXME : deactivate button when this is true
	if (that->result.game.empty() || that->result.port.empty())
	{
		fl_beep();
		return;
	}

	that->inst.M_PortSetupDialog(that->result.port, that->result.game, {});
}


void UI_ProjectSetup::load_callback(Fl_Button *w, void *data)
{
	auto that = static_cast<UI_ProjectSetup*>(data);
	int r;
	bool isdir = false;
	for (r = 0; r < RES_NUM; ++r)
		if (w == that->mResourceFileButtons[r] || w == that->mResourceDirButtons[r])
		{
			isdir = w == that->mResourceDirButtons[r];
			break;
		}
	SYS_ASSERT(0 <= r && r < RES_NUM);

	SYS_ASSERT(that);
	
	const char *title = isdir ? "Pick folder to open" : "Pick file to open";
#ifdef __APPLE__
	title = "Pick file or folder to open";
	isdir = true;
#endif

	Fl_Native_File_Chooser chooser;

	chooser.title(title);
	if(isdir)
	{
		chooser.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
#ifdef __APPLE__
		chooser.filter("Wads\t*.wad\nEureka defs\t*.ugh\nDehacked files\t*.deh\nBEX files\t*.bex");
#endif
	}
	else
	{
		chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
		chooser.filter("Wads\t*.wad\nEureka defs\t*.ugh\nDehacked files\t*.deh\nBEX files\t*.bex");
	}
	chooser.directory(that->inst.Main_FileOpFolder().u8string().c_str());

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

	that->result.resources[r] = fs::u8path(chooser.filename());

	that->res_name[r]->value(fileOrDirName(that->result.resources[r]).u8string().c_str());
}


void UI_ProjectSetup::kill_callback(Fl_Button *w, void *data)
{
	auto that = static_cast<UI_ProjectSetup*>(data);
	int r;
	for (r = 0; r < RES_NUM; ++r)
		if (w == that->mClearButtons[r])
			break;
	SYS_ASSERT(0 <= r && r < RES_NUM);

	SYS_ASSERT(that);

	if (!that->result.resources[r].empty())
	{
		that->result.resources[r].clear();

		that->res_name[r]->value("");
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
