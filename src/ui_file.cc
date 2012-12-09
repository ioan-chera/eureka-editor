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
#include "ui_window.h"
#include "ui_file.h"


UI_ChooseMap::UI_ChooseMap() :
	Fl_Double_Window(360, 180, "Choose Map"),
	action(ACT_none)
{
	resizable(NULL);

	callback(close_callback, this);


	Fl_Box *title = new Fl_Box(20, 23, 285, 32, "Choose map slot");
	title->labelfont(FL_HELVETICA_BOLD);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	map_name = new Fl_Input(90, 65, 110, 25);

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

		Fl_Return_Button *ok_but = new Fl_Return_Button(270, 430, 95, 35, "OK");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		Fl_Button *cancel = new Fl_Button(130, 430, 95, 35, "Cancel");
		cancel->callback(close_callback, this);

		o->end();
	}

	end();
}


UI_ChooseMap::~UI_ChooseMap()
{
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

	return StringDup(map_name->value());
}


void UI_ChooseMap::close_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->action = ACT_CANCEL;
}


void UI_ChooseMap::ok_callback(Fl_Widget *w, void *data)
{
	UI_ChooseMap * that = (UI_ChooseMap *)data;

	that->action = ACT_ACCEPT;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
