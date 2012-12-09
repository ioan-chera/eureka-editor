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
#include "w_wad.h"

#include "ui_window.h"
#include "ui_file.h"


UI_ChooseMap::UI_ChooseMap(const char *initial_name) :
	Fl_Double_Window(395, 480, "Choose Map"),
	action(ACT_none)
{
	resizable(NULL);

	callback(close_callback, this);


	Fl_Box *title = new Fl_Box(20, 18, 285, 32, "Enter the map slot:");
	title->labelfont(FL_HELVETICA_BOLD);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	map_name = new Fl_Input(90, 60, 110, 25);
	map_name->when(FL_WHEN_CHANGED);
	map_name->value(initial_name);
//!!	map_name->callback(input_callback, this);

/*
	{ Fl_Button* o = new Fl_Button(90, 155, 70, 20, "MAP02");
		o->color((Fl_Color)213);
	}
	{ Fl_Button* o = new Fl_Button(165, 155, 70, 20, "MAP03");
		o->color((Fl_Color)213);
	}
	{ Fl_Button* o = new Fl_Button(240, 155, 70, 20, "MAP04");
		o->color((Fl_Color)213);
	}
	{ Fl_Button* o = new Fl_Button(315, 155, 70, 20, "MAP05");
		o->color((Fl_Color)213);
	}
	{ Fl_Button* o = new Fl_Button(15, 180, 70, 20, "MAP01");
		o->color((Fl_Color)213);
	}
	{ Fl_Button* o = new Fl_Button(90, 180, 70, 20, "MAP02");
		o->color((Fl_Color)213);
	}
	{ Fl_Button* o = new Fl_Button(165, 180, 70, 20, "MAP03");
		o->color((Fl_Color)213);
	}
	{ Fl_Button* o = new Fl_Button(240, 180, 70, 20, "MAP04");
		o->color((Fl_Color)213);
	}
	{ Fl_Button* o = new Fl_Button(315, 180, 70, 20, "MAP05");
		o->color((Fl_Color)213);
	}
	{ Fl_Button* o = new Fl_Button(15, 215, 70, 20, "MAP01");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(90, 215, 70, 20, "MAP02");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(165, 215, 70, 20, "MAP03");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(240, 215, 70, 20, "MAP04");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(315, 215, 70, 20, "MAP05");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(15, 240, 70, 20, "MAP01");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(90, 240, 70, 20, "MAP02");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(165, 240, 70, 20, "MAP03");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(240, 240, 70, 20, "MAP04");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(315, 240, 70, 20, "MAP05");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(15, 275, 70, 20, "MAP01");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(90, 275, 70, 20, "MAP02");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(165, 275, 70, 20, "MAP03");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(240, 275, 70, 20, "MAP04");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(315, 275, 70, 20, "MAP05");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(15, 300, 70, 20, "MAP01");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(90, 300, 70, 20, "MAP02");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(165, 300, 70, 20, "MAP03");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(240, 300, 70, 20, "MAP04");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(315, 300, 70, 20, "MAP05");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(15, 335, 70, 20, "MAP01");
		o->color((Fl_Color)207);
	}
	{ Fl_Button* o = new Fl_Button(90, 335, 70, 20, "MAP02");
		o->color((Fl_Color)207);
	}

	{ kind_MAPxx = new Fl_Round_Button(40, 115, 75, 25, "MAPxx");
		kind_MAPxx->down_box(FL_ROUND_DOWN_BOX);
	}
	{ kind_ExMx = new Fl_Round_Button(130, 115, 70, 25, "ExMx");
		kind_ExMx->down_box(FL_ROUND_DOWN_BOX);
	}
*/

	{
		Fl_Group* o = new Fl_Group(0, 415, 395, 65);
		o->box(FL_FLAT_BOX);
		o->color(WINDOW_BG, WINDOW_BG);

		Fl_Return_Button *ok_but = new Fl_Return_Button(270, 432, 95, 35, "OK");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		Fl_Button *cancel = new Fl_Button(130, 432, 95, 35, "Cancel");
		cancel->callback(close_callback, this);

		o->end();
	}

	end();
}


UI_ChooseMap::~UI_ChooseMap()
{
}


void UI_ChooseMap::PopulateButtons(char format, Wad_file *test_wad)
{
	Fl_Box * heading = new Fl_Box(FL_NO_BOX, 20, 104, 285, 32, "Or select via these cool buttons:");
	heading->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	heading->labelfont(FL_HELVETICA_BOLD);
	add(heading);

	Fl_Color free_col = fl_rgb_color(0x33, 0xDD, 0x77);
	Fl_Color used_col = fl_rgb_color(0xFF, 0x33, 0x66);

	for (int col = 0 ; col < 4  ; col++)
	for (int row = 0 ; row < 10 ; row++)
	{
		int cx = x() +  45 + col * 82;
		int cy = y() + 145 + row * 25;

		char name_buf[20];

		if (format == 'E')
		{
			int epi = 1 + col;
			int map = 1 + row; // (row % 2) * 5 + col;

			if (map > 9)
				continue;

			sprintf(name_buf, "E%dM%d\n", epi, map);
		}
		else
		{
			int map = 1 + col * 10 + row;

			if (map < 1 || map > 32)
				continue;

			sprintf(name_buf, "MAP%02d", map);
		}

		Fl_Button * but = new Fl_Button(cx, cy, 70, 20);
		but->copy_label(name_buf);
		but->callback(button_callback, this);

		if (test_wad && test_wad->FindLevel(name_buf) >= 0)
			but->color(used_col);
		else
			but->color(free_col);

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


bool UI_ChooseMap::isNameValid() const
{
	const char *p = map_name->value();

	if (strlen(p) == 0)
		return false;

	for ( ; *p ; p++)
		if (! (isalnum(*p) || *p == '_'))
			return false;

	return true;
}


void UI_ChooseMap::close_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->action = ACT_CANCEL;
}


void UI_ChooseMap::ok_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	if (that->isNameValid())
		that->action = ACT_ACCEPT;
	else
		Beep();
}


void UI_ChooseMap::button_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->map_name->value(w->label());
	that->action = ACT_ACCEPT;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
