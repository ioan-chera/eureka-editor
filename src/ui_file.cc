//------------------------------------------------------------------------
//  File-related dialogs
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
#include "m_dialog.h"
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
	int len = strlen(p);

	if (len == 0 || len > 8)
		return false;

	if (! isalpha(*p))
		return false;

	for ( ; *p ; p++)
		if (! (isalnum(*p) || *p == '_'))
			return false;

	return true;
}


UI_ChooseMap::UI_ChooseMap(const char *initial_name) :
	Fl_Double_Window(420, 380, "Choose Map"),
	action(ACT_none)
{
	resizable(NULL);

	callback(close_callback, this);


///---	Fl_Box *title = new Fl_Box(20, 18, 285, 32, "Enter the map slot:");
///---	title->labelfont(FL_HELVETICA_BOLD);
///---	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	map_name = new Fl_Input(94, 25, 120, 25, "Map slot: ");
	map_name->labelfont(FL_HELVETICA_BOLD);
	map_name->when(FL_WHEN_CHANGED);
	map_name->callback(input_callback, this);
	map_name->value(initial_name);

	{
		int bottom_y = 315;

		Fl_Group* o = new Fl_Group(0, bottom_y, 420, 65);
		o->box(FL_FLAT_BOX);
		o->color(WINDOW_BG, WINDOW_BG);

		ok_but = new Fl_Return_Button(270, bottom_y + 17, 95, 35, "OK");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		Fl_Button *cancel = new Fl_Button(130, bottom_y + 17, 95, 35, "Cancel");
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
///---	Fl_Box * heading = new Fl_Box(FL_NO_BOX, 20, 104, 285, 32, "Or select via these cool buttons:");
///---	heading->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
///---	heading->labelfont(FL_HELVETICA_BOLD);
///---	add(heading);

	int but_W = 60;

	for (int col = 0 ; col < 5 ; col++)
	for (int row = 0 ; row < 8 ; row++)
	{
		int cx = x() + 30 + col * (but_W + but_W / 5);
		int cy = y() + 75 + row * 24 + (row / 2) * 10;

		char name_buf[20];

		if (format == 'E')
		{
			int epi = 1 + row / 2;
			int map = 1 + col + (row & 1) * 5;

			if (map > 9)
				continue;

			sprintf(name_buf, "E%dM%d\n", epi, map);
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

		if (test_wad && test_wad->FindLevel(name_buf) >= 0)
			but->color(USED_COL);
		else
			but->color(FREE_COL);

		add(but);
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
	Fl_Double_Window(420, 530, "Open Map"),
	action(ACT_none),
	result_wad(NULL), new_pwad(NULL)
{
	resizable(NULL);

	callback(close_callback, this);

	{
		Fl_Box *o = new Fl_Box(10, 10, 300, 37, "Look for the map in which file:");
		o->labelfont(FL_HELVETICA_BOLD);
		o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}

	{
		Fl_Group *o = new Fl_Group(50, 50, 235, 85);

		look_iwad = new Fl_Round_Button(50, 50, 215, 25, " the Game (IWAD) file");
		look_iwad->down_box(FL_ROUND_DOWN_BOX);
		look_iwad->type(FL_RADIO_BUTTON);
		look_iwad->callback(look_callback, this);

		look_res = new Fl_Round_Button(50, 75, 215, 25, " the Resource wads");
		look_res->down_box(FL_ROUND_DOWN_BOX);
		look_res->type(FL_RADIO_BUTTON);
		look_res->callback(look_callback, this);

		look_pwad = new Fl_Round_Button(50, 100, 235, 25, " the currently edited PWAD");
		look_pwad->down_box(FL_ROUND_DOWN_BOX);
		look_pwad->type(FL_RADIO_BUTTON);
		look_pwad->callback(look_callback, this);

		if (edit_wad)
			look_pwad->value(1);
		else
			look_iwad->value(1);

		o->end();
	}

	{
		Fl_Box* o = new Fl_Box(10, 140, 300, 20, "Current PWAD file:");
		o->labelfont(FL_HELVETICA_BOLD);
		o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}

	pwad_name = new Fl_Output(25, 165, 265, 26);

	Fl_Button *load_but = new Fl_Button(305, 164, 70, 28, "Load");
	load_but->callback(load_callback, this);

	map_name = new Fl_Input(94, 205, 100, 26, "Map slot: ");
	map_name->labelfont(FL_HELVETICA_BOLD);
	map_name->when(FL_WHEN_CHANGED);
	map_name->callback(input_callback, this);

	{
		Fl_Box *o = new Fl_Box(205, 205, 180, 26, "Available maps:");
		// o->labelfont(FL_HELVETICA_BOLD);
		o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}


	// all the map buttons go into this group
	
	button_grp = new Fl_Group(0, 235, w(), 230, "\n\nNone Found");
	button_grp->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
	button_grp->end();

	{
		int bottom_y = 470;

		Fl_Group* o = new Fl_Group(0, bottom_y, w(), 60);
		o->box(FL_FLAT_BOX);
		o->color(WINDOW_BG, WINDOW_BG);

		ok_but = new Fl_Return_Button(280, bottom_y + 15, 89, 34, "OK");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		Fl_Button * cancel = new Fl_Button(140, bottom_y + 15, 95, 35, "Cancel");
		cancel->callback(close_callback, this);

		o->end();
	}

	end();

	CheckMapName();
}


UI_OpenMap::~UI_OpenMap()
{
}


void UI_OpenMap::Run(Wad_file ** wad_v, bool * is_new_pwad, const char ** map_v)
{
	*wad_v = NULL;
	*map_v = NULL;
	*is_new_pwad = false;

	if (edit_wad)
		SetPWAD(edit_wad->PathName());

	Populate();

	set_modal();

	show();

	while (action == ACT_none)
	{
		Fl::wait(0.2);
	}

	if (action == ACT_ACCEPT)
	{
		SYS_ASSERT(result_wad);

		*wad_v = result_wad;
		*map_v = StringUpper(map_name->value());

		if (result_wad == new_pwad)
		{
			*is_new_pwad = true;
			new_pwad = NULL;
		}
	}

	if (new_pwad)
	{
		delete new_pwad;
	}
}


void UI_OpenMap::CheckMapName()
{
	bool was_valid = ok_but->active();

	bool  is_valid = (result_wad != NULL) &&
	                 ValidateMapName(map_name->value()) &&
					 (result_wad->FindLevel(map_name->value()) >= 0);

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
	button_grp->label("\n\nNone Found");
	button_grp->clear();

	result_wad = NULL;

	if (look_iwad->value())
	{
		PopulateButtons(game_wad);
	}
	else if (look_res->value())
	{
		int first = 1;
		int last  = (int)master_dir.size() - 1;

		if (edit_wad)
			last--;

		// we simply use the last resource which contains levels
		// TODO: should grab list from each of them and merge into one big list

		for (int r = last ; r >= first ; r--)
		{
			if (master_dir[r]->FindFirstLevel() >= 0)
			{
				PopulateButtons(master_dir[r]);
				break;
			}
		}
	}
	else
	{
		if (new_pwad)
			PopulateButtons(new_pwad);
		else if (edit_wad)
			PopulateButtons(edit_wad);
	}
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


void UI_OpenMap::PopulateButtons(Wad_file *wad)
{
	result_wad = wad;

	int num_levels = wad->NumLevels();

	if (num_levels == 0)
		return;

	button_grp->label("");

	// limit the number based on available space
/*
	int max_rows = 8;
	int max_cols = 5;

	num_levels = MIN(num_levels, max_rows * max_cols);
*/
	std::map<std::string, int> level_names;
	std::map<std::string, int>::iterator IT;

	for (int lev = 0 ; lev < num_levels ; lev++)
	{
		Lump_c *lump = wad->GetLump(wad->GetLevel(lev));

		level_names[std::string(lump->Name())] = 1;
	}

	// determine how many rows and columns, and adjust layout

#if 0
	if (num_levels <= 24) max_rows = 6;
/*
	if (num_levels <=  9) max_rows = 3;
	if (num_levels <=  4) max_rows = 2;
	if (num_levels <=  2) max_rows = 1;
*/
	max_cols = (num_levels + (max_rows - 1)) / max_rows;
#endif

	int cx_base = button_grp->x() + 30;
	int cy_base = button_grp->y() + 5;
	int but_W   = 60;
/*
	if (max_cols <= 4) { cx_base += 20; but_W += 12; }
	if (max_cols <= 2) { cx_base += 40; but_W += 12; }
	if (max_cols <= 1) { cx_base += 40; but_W += 12; }
*/

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

			if (row >= 8)
				break;
		}

		int cx = cx_base + col * (but_W + but_W / 5);
		int cy = cy_base + row * 24 + (row / 2) * 8;

		Fl_Button * but = new Fl_Button(cx, cy, but_W, 20);
		but->copy_label(name);
		but->color(FREE_COL);
		but->callback(button_callback, this);

		button_grp->add(but);

		col++;
		if (col >= 5)
		{
			col = 0;
			row++;

			if (row >= 8)
				break;
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
	if (that->result_wad && ValidateMapName(that->map_name->value()))
		that->action = ACT_ACCEPT;
	else
		fl_beep();
}


void UI_OpenMap::button_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	// sanity check
	if (! that->result_wad)
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

	//??  chooser.directory("xxx");

	// Show native chooser
	switch (chooser.show())
	{
		case -1:
			LogPrintf("Open Map: error choosing file:\n");
			LogPrintf("   %s\n", chooser.errmsg());

			Notify(-1, -1, "Unable to open the map:",
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

		Notify(-1, -1, "Unable to open the chosen WAD file.\n\n"
				"Please try again.", NULL);
		return;
	}

	if (wad->FindFirstLevel() < 0)
	{
		Notify(-1, -1, "The chosen WAD contains no levels.\n\n"
				"Please try again.", NULL);
		return;
	}


	if (new_pwad)
	{
		delete new_pwad;
	}

	new_pwad = wad;

	SetPWAD(new_pwad->PathName());


	// change the "Look in ..." setting to be the current pwad

	look_iwad->value(0);
	look_res ->value(0);
	look_pwad->value(1);

	Populate();
}


//------------------------------------------------------------------------


UI_ProjectSetup * UI_ProjectSetup::_instance = NULL;


#define STARTUP_MSG  "No IWADs could be found."


UI_ProjectSetup::UI_ProjectSetup(bool is_startup) :
	Fl_Double_Window(400, is_startup ? 412 : 372, "Manage Wads"),
	action(ACT_none),
	iwad(NULL), port(NULL)
{
	callback(close_callback, this);

	resizable(NULL);

	_instance = this;  // meh, hacky

	int by = 0;
	
	if (is_startup)
	{
		Fl_Box * message = new Fl_Box(FL_FLAT_BOX, 15, 10, 370, 46, STARTUP_MSG);
		message->align(FL_ALIGN_INSIDE);
		message->color(FL_RED, FL_RED);
		message->labelcolor(FL_YELLOW);
		message->labelsize(18);

		by += 40;
	}
	
	iwad_choice = new Fl_Choice(120, by+25, 170, 29, "Game IWAD: ");
	iwad_choice->labelfont(FL_HELVETICA_BOLD);
	iwad_choice->down_box(FL_BORDER_BOX);
	iwad_choice->callback((Fl_Callback*)iwad_callback, this);

	port_choice = new Fl_Choice(120, by+60, 170, 29, "Port: ");
	port_choice->labelfont(FL_HELVETICA_BOLD);
	port_choice->down_box(FL_BORDER_BOX);
	port_choice->callback((Fl_Callback*)port_callback, this);

	{
		Fl_Button* o = new Fl_Button(305, by+27, 75, 25, "Find");
		o->callback((Fl_Callback*)browse_callback, this);
	}

	// Resource section

	Fl_Box *res_title = new Fl_Box(15, by+110, 185, 35, "Resource Wads:");
	res_title->labelfont(FL_HELVETICA_BOLD);
	res_title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	for (int r = 0 ; r < RES_NUM ; r++)
	{
		res[r] = NULL;

		int cy = by + 145 + r * 35;

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
		Fl_Group *o = new Fl_Group(0, by+312, 400, 60);
		o->box(FL_FLAT_BOX);
		o->color(WINDOW_BG, WINDOW_BG);

		cancel = new Fl_Button(165, o->y() + 14, 80, 35, is_startup ? "Quit" : "Cancel");
		cancel->callback((Fl_Callback*)close_callback, this);

		ok_but = new Fl_Button(290, o->y() + 14, 80, 35, "Use");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback((Fl_Callback*)use_callback, this);

		o->end();
	}

	end();
}


UI_ProjectSetup::~UI_ProjectSetup()
{
	_instance = NULL;
}


bool UI_ProjectSetup::Run()
{
	Populate();

	set_modal();

	show();

	while (action == ACT_none)
	{
		Fl::wait(0.2);
	}

	return (action == ACT_ACCEPT);
}


void UI_ProjectSetup::PopulateIWADs(const char *curr_iwad)
{
	const char *iwad_string;
	int iwad_val = 0;

	iwad_string = M_KnownIWADsForMenu(&iwad_val, curr_iwad ? curr_iwad : "xxx");

	iwad_choice->clear();

	if (iwad_string[0])
	{
		iwad_choice->add(iwad_string);
		iwad_choice->value(iwad_val);

		iwad = M_QueryKnownIWAD(iwad_choice->mvalue()->text);
	}
}


void UI_ProjectSetup::Populate()
{
	iwad = NULL;

	PopulateIWADs(Iwad_name);

	if (! iwad)
		ok_but->deactivate();


	port = NULL;

	const char *port_string;
	int port_val = 0;

	port_string = M_CollectDefsForMenu("ports", &port_val, Port_name ? Port_name : "xxx");

	if (port_string[0])
	{
		port_choice->add  (port_string);
		port_choice->value(port_val);

		port = port_choice->mvalue()->text;
	}


	// Note: these resource wads may be invalid (not exist) during startup.
	//       This is probably NOT the place to validate them...

	for (int r = 0 ; r < RES_NUM ; r++)
	{
		if (r < (int)ResourceWads.size())
		{
			res[r] = StringDup(ResourceWads[r]);

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


void UI_ProjectSetup::iwad_callback(Fl_Choice *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	const char * name = w->mvalue()->text;

	that->iwad = StringDup(M_QueryKnownIWAD(name));

	if (that->iwad)
		that->ok_but->activate();
	else
	{
		that->ok_but->deactivate();

		fl_beep();
	}
}


void UI_ProjectSetup::port_callback(Fl_Choice *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	const char * name = w->mvalue()->text;

	that->port = StringDup(name);
}


void UI_ProjectSetup::browse_callback(Fl_Button *w, void *data)
{
	UI_ProjectSetup * that = (UI_ProjectSetup *)data;

	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to open");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.filter("Wads\t*.wad");

	//??  chooser.directory("xxx");

	switch (chooser.show())
	{
		case -1:  // error
			Notify(-1, -1, "Unable to open that wad:", chooser.errmsg());
			return;

		case 1:  // cancelled
			return;

		default:
			break;  // OK
	}

	// check that a game definition exists

	const char *game = DetermineGame(chooser.filename());

	if (! CanLoadDefinitions("games", game))
	{
		Notify(-1, -1, "That game is not supported (no definition file).\n\n"
		               "Please try again.", NULL);
		return;
	}


	M_AddKnownIWAD(chooser.filename());
	M_SaveRecent();

	that->iwad = StringDup(chooser.filename());

	that->PopulateIWADs(that->iwad);

	that->ok_but->activate();
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
	chooser.filter("Wads\t*.wad");

	//??  chooser.directory("xxx");

	switch (chooser.show())
	{
		case -1:  // error
			Notify(-1, -1, "Unable to open that wad:", chooser.errmsg());
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
