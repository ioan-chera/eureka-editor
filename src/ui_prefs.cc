//------------------------------------------------------------------------
//  Preferences Dialog
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2013 Andrew Apted
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

#include "ui_window.h"
#include "ui_prefs.h"

#include <FL/Fl_Color_Chooser.H>


#define PREF_WINDOW_W  600
#define PREF_WINDOW_H  500

#define PREF_WINDOW_TITLE  "Eureka Preferences"


static int last_active_tab = 0;


class UI_Preferences : public Fl_Double_Window
{
private:
	bool want_quit;

	static void close_callback(Fl_Widget *w, void *data);
	static void color_callback(Fl_Button *w, void *data);

	static void bind_key_callback(Fl_Button *w, void *data);
	static void edit_key_callback(Fl_Button *w, void *data);
	static void sort_key_callback(Fl_Button *w, void *data);

public:
	UI_Preferences();

	void Run();

	void LoadValues();
	void SaveValues();

	void LoadKeys();

	int GridSizeToChoice(int size);


	Fl_Tabs *tabs;

	Fl_Round_Button *theme_FLTK;
	Fl_Round_Button *theme_GTK;
	Fl_Round_Button *theme_plastic;
	Fl_Round_Button *cols_default;
	Fl_Round_Button *cols_bright;
	Fl_Round_Button *cols_custom;
	Fl_Button *bg_colorbox;
	Fl_Button *ig_colorbox;
	Fl_Button *fg_colorbox;

	Fl_Check_Button *grid_snap;
	Fl_Choice *grid_mode;
	Fl_Choice *grid_size;
	Fl_Check_Button *gen_digitzoom;
	Fl_Choice *gen_smallscroll;
	Fl_Choice *gen_largescroll;
	Fl_Check_Button *gen_wheelscroll;

	Fl_Check_Button *edit_newislands;
	Fl_Check_Button *edit_samemode;
	Fl_Check_Button *edit_autoadjustX;
	Fl_Check_Button *edit_multiselect;
	Fl_Choice *edit_modkey;
	Fl_Int_Input *edit_sectorsize;

	Fl_Check_Button *bsp_warn;
	Fl_Check_Button *bsp_verbose;
	Fl_Check_Button *bsp_fast;

	Fl_Hold_Browser *key_list;
	Fl_Button *key_group;
	Fl_Button *key_key;
	Fl_Button *key_func;

	Fl_Button *key_change;
	Fl_Button *key_edit;

	std::vector<int> key_order;

	char key_sort_mode;
	bool key_sort_rev;
};


UI_Preferences::UI_Preferences() :
	  Fl_Double_Window(PREF_WINDOW_W, PREF_WINDOW_H, PREF_WINDOW_TITLE),
	  want_quit(false), key_order(),
	  key_sort_mode('c'), key_sort_rev(false)
{
	color(fl_gray_ramp(4));
	callback(close_callback, this);

	{ tabs = new Fl_Tabs(0, 0, 585, 435);

	  /* ---- General Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 405, " General     ");
		o->labelsize(16);
		// o->hide();

		{ Fl_Box* o = new Fl_Box(25, 50, 145, 30, "GUI Appearance");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ Fl_Group* o = new Fl_Group(45, 90, 250, 115);
		  { theme_FLTK = new Fl_Round_Button(50, 90, 150, 25, " FLTK theme");
			theme_FLTK->type(102);
			theme_FLTK->down_box(FL_ROUND_DOWN_BOX);
		  }
		  { theme_GTK = new Fl_Round_Button(50, 120, 150, 25, " GTK+ theme ");
			theme_GTK->type(102);
			theme_GTK->down_box(FL_ROUND_DOWN_BOX);
		  }
		  { theme_plastic = new Fl_Round_Button(50, 150, 165, 25, " plastic theme ");
			theme_plastic->type(102);
			theme_plastic->down_box(FL_ROUND_DOWN_BOX);
		  }
		  o->end();
		}
		{ Fl_Group* o = new Fl_Group(220, 90, 190, 90);
		  { cols_default = new Fl_Round_Button(245, 90, 135, 25, "default colors");
			cols_default->type(102);
			cols_default->down_box(FL_ROUND_DOWN_BOX);
		  }
		  { cols_bright = new Fl_Round_Button(245, 120, 140, 25, "bright colors");
			cols_bright->type(102);
			cols_bright->down_box(FL_ROUND_DOWN_BOX);
		  }
		  { cols_custom = new Fl_Round_Button(245, 150, 165, 25, "custom colors   ---->");
			cols_custom->type(102);
			cols_custom->down_box(FL_ROUND_DOWN_BOX);
		  }
		  o->end();
		}
		{ Fl_Group* o = new Fl_Group(385, 80, 205, 100);
		  o->color(FL_LIGHT1);
		  o->align(Fl_Align(FL_ALIGN_BOTTOM_LEFT|FL_ALIGN_INSIDE));
		  { bg_colorbox = new Fl_Button(430, 90, 45, 25, "background");
			bg_colorbox->box(FL_BORDER_BOX);
			bg_colorbox->align(Fl_Align(FL_ALIGN_RIGHT));
			bg_colorbox->callback((Fl_Callback*)color_callback, this);
		  }
		  { ig_colorbox = new Fl_Button(430, 120, 45, 25, "input bg");
			ig_colorbox->box(FL_BORDER_BOX);
			ig_colorbox->color(FL_BACKGROUND2_COLOR);
			ig_colorbox->align(Fl_Align(FL_ALIGN_RIGHT));
			ig_colorbox->callback((Fl_Callback*)color_callback, this);
		  }
		  { fg_colorbox = new Fl_Button(430, 150, 45, 25, "text color");
			fg_colorbox->box(FL_BORDER_BOX);
			fg_colorbox->color(FL_GRAY0);
			fg_colorbox->align(Fl_Align(FL_ALIGN_RIGHT));
			fg_colorbox->callback((Fl_Callback*)color_callback, this);
		  }
		  o->end();
		}
		{ Fl_Box* o = new Fl_Box(30, 195, 195, 25, "Map Grid and Scrolling");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ grid_snap = new Fl_Check_Button(50, 230, 235, 25, " default SNAP mode");
		  grid_snap->down_box(FL_DOWN_BOX);
		}
		{ grid_mode = new Fl_Choice(435, 230, 95, 25, "default grid mode ");
		  grid_mode->down_box(FL_BORDER_BOX);
		  grid_mode->add("OFF|Normal|Simple");
		}
		{ grid_size = new Fl_Choice(435, 265, 95, 25, "default grid size ");
		  grid_size->down_box(FL_BORDER_BOX);
		  grid_size->add("1024|512|256|128|64|32|16|8|4|2");
		}
		{ gen_digitzoom = new Fl_Check_Button(50, 265, 240, 25, " digit keys zoom the map");
		  gen_digitzoom->down_box(FL_DOWN_BOX);
		}
		{ gen_smallscroll = new Fl_Choice(435, 265, 95, 25, "small scroll step ");
		  gen_smallscroll->down_box(FL_BORDER_BOX);
		  gen_smallscroll->hide();
		}
		{ gen_largescroll = new Fl_Choice(435, 300, 95, 25, "large scroll step ");
		  gen_largescroll->down_box(FL_BORDER_BOX);
		}
		{ gen_wheelscroll = new Fl_Check_Button(50, 300, 245, 25, " mouse wheel scrolls the map");
		  gen_wheelscroll->down_box(FL_DOWN_BOX);
		}
		{ Fl_Box* o = new Fl_Box(30, 340, 280, 35, "Other Stuff");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		o->end();
	  }

	  /* ---- Editing Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Editing     ");
		o->labelsize(16);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 50, 355, 30, "Editing Options");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ edit_newislands = new Fl_Check_Button(50, 80, 265, 30, " new islands have void interior");
		  edit_newislands->down_box(FL_DOWN_BOX);
		}
		{ edit_samemode = new Fl_Check_Button(50, 140, 270, 30, " same mode key will clear selection");
		  edit_samemode->down_box(FL_DOWN_BOX);
		}
		{ edit_autoadjustX = new Fl_Check_Button(50, 110, 260, 30, " auto-adjust X offsets");
		  edit_autoadjustX->down_box(FL_DOWN_BOX);
		}
		{ edit_multiselect = new Fl_Check_Button(50, 170, 275, 30, " multi-select requires a modifier key");
		  edit_multiselect->down_box(FL_DOWN_BOX);
		}
		{ edit_modkey = new Fl_Choice(370, 170, 95, 30, "---->   ");
		  edit_modkey->down_box(FL_BORDER_BOX);
		  edit_modkey->add("CTRL");
		  edit_modkey->value(0);
		}
		{ edit_sectorsize = new Fl_Int_Input(440, 80, 105, 25, "new sector size:");
		  edit_sectorsize->type(2);
		}
		o->end();
	  }

	  /* ---- Key bindings Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Keys     ");
		o->labelsize(16);
		o->hide();

		{ Fl_Box* o = new Fl_Box(15, 50, 355, 30, "Key Bindings");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}

		{ key_group = new Fl_Button(25, 90, 80, 25, "CONTEXT");
		  key_group->color((Fl_Color)231);
		  key_group->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		  key_group->callback((Fl_Callback*)sort_key_callback, this);
		}
		{ key_key = new Fl_Button(110, 90, 115, 25, "KEY");
		  key_key->color((Fl_Color)231);
		  key_key->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		  key_key->callback((Fl_Callback*)sort_key_callback, this);
		}
		{ key_func = new Fl_Button(230, 90, 210, 25, "FUNCTION");
		  key_func->color((Fl_Color)231);
		  key_func->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		  key_func->callback((Fl_Callback*)sort_key_callback, this);
		}
		{ key_list = new Fl_Hold_Browser(30, 115, 430, 295);
		  key_list->textfont(FL_COURIER);
		}
		{ key_change = new Fl_Button(470, 145, 90, 30, "Bind");
		  key_change->callback((Fl_Callback*)bind_key_callback, this);
		}
		{ key_edit = new Fl_Button(470, 190, 90, 30, "Edit");
		  key_edit->callback((Fl_Callback*)edit_key_callback, this);
		}
		o->end();
	  }

	  /* ---- glBSP Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " glBSP     ");
		o->selection_color(FL_LIGHT1);
		o->labelsize(16);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 50, 280, 30, "glBSP Node Building Options");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ bsp_warn = new Fl_Check_Button(60, 90, 220, 30, " Show all warning messages");
		  bsp_warn->down_box(FL_DOWN_BOX);
		}
		{ bsp_verbose = new Fl_Check_Button(60, 120, 350, 30, " Verbose -- show information about each level");
		  bsp_verbose->down_box(FL_DOWN_BOX);
		}
		{ bsp_fast = new Fl_Check_Button(60, 150, 440, 30, " Fast -- build the fastest way, but nodes may not be as good");
		  bsp_fast->down_box(FL_DOWN_BOX);
		}
		o->end();
	  }
	  tabs->end();
	}
	{ Fl_Button *o = new Fl_Button(460, 450, 85, 35, "OK");
	  o->callback(close_callback, this);
	}

end();
}


//------------------------------------------------------------------------

void UI_Preferences::close_callback(Fl_Widget *w, void *data)
{
	UI_Preferences *dialog = (UI_Preferences *)data;

	dialog->want_quit = true;
}


void UI_Preferences::color_callback(Fl_Button *w, void *data)
{
//	UI_Preferences *dialog = (UI_Preferences *)data;

	uchar r, g, b;

	Fl::get_color(w->color(), r, g, b);

	if (! fl_color_chooser("New color:", r, g, b, 3))
		return;

	w->color(fl_rgb_color(r, g, b));
}


void UI_Preferences::bind_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *dialog = (UI_Preferences *)data;

	int line = dialog->key_list->value();
	if (line < 1)
	{
		fl_beep();
		return;
	}

	int bind_idx = (int)(long)dialog->key_list->data(line);
	SYS_ASSERT(bind_idx >= 0);

	// mark line with ??? (show ready to accept new key)

	const char *str = M_StringForBinding(bind_idx, true /* changing_key */);
	SYS_ASSERT(str);

	dialog->key_list->text(line, str);

	// FIXME !!!  set flag, have handle() method to grab key
	//            and update the binding.
}


void UI_Preferences::edit_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *dialog = (UI_Preferences *)data;

	int line = dialog->key_list->value();
	if (line < 1)
	{
		fl_beep();
		return;
	}

	int bind_idx = (int)(long)dialog->key_list->data(line);

	const char *str = M_StringForBinding(bind_idx);
	str += MIN(strlen(str), 26);

	const char *new_str = fl_input("Enter new function", str);

	// FIXME !!
}


void UI_Preferences::sort_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *dialog = (UI_Preferences *)data;

	if (w == dialog->key_group)
	{
		if (dialog->key_sort_mode != 'c')
		{
			dialog->key_sort_mode  = 'c';
			dialog->key_sort_rev = false;
		}
		else
			dialog->key_sort_rev = !dialog->key_sort_rev;
	}
	else if (w == dialog->key_key)
	{
		if (dialog->key_sort_mode != 'k')
		{
			dialog->key_sort_mode  = 'k';
			dialog->key_sort_rev = false;
		}
		else
			dialog->key_sort_rev = !dialog->key_sort_rev;
	}
	else if (w == dialog->key_func)
	{
		if (dialog->key_sort_mode != 'f')
		{
			dialog->key_sort_mode  = 'f';
			dialog->key_sort_rev = false;
		}
		else
			dialog->key_sort_rev = !dialog->key_sort_rev;
	}

	dialog->LoadKeys();
}


void UI_Preferences::Run()
{
	if (last_active_tab < tabs->children())
		tabs->value(tabs->child(last_active_tab));

	LoadValues();
	LoadKeys();

	set_modal();

	show();

	while (! want_quit)
	{
		Fl::wait(0.2);
	}

	SaveValues();

	M_WriteConfigFile();

	last_active_tab = tabs->find(tabs->value());
}


int UI_Preferences::GridSizeToChoice(int size)
{
	if (size > 512) return 0;
	if (size > 256) return 1;
	if (size > 128) return 2;
	if (size >  64) return 3;
	if (size >  32) return 4;
	if (size >  16) return 5;
	if (size >   8) return 6;
	if (size >   4) return 7;
	if (size >   2) return 8;

	return 9;
}


void UI_Preferences::LoadValues()
{
	/* Theme stuff */
	
	switch (gui_scheme)
	{
		case 0: theme_FLTK->value(1); break;
		case 1: theme_GTK->value(1); break;
		case 2: theme_plastic->value(1); break;
	}

	switch (gui_color_set)
	{
		case 0: cols_default->value(1); break;
		case 1: cols_bright->value(1); break;
		case 2: cols_custom->value(1); break;
	}

	bg_colorbox->color(gui_custom_bg);
	ig_colorbox->color(gui_custom_ig);
	fg_colorbox->color(gui_custom_fg);

	/* General stuff */

	if (default_grid_mode < 0 || default_grid_mode > 2)
		default_grid_mode = 1;

	grid_snap->value(default_grid_snap ? 1 : 0);
	grid_size->value(GridSizeToChoice(default_grid_size));
	grid_mode->value(default_grid_mode);

	gen_digitzoom  ->value(digits_set_zoom ? 1 : 0);
	gen_wheelscroll->value(mouse_wheel_scrolls_map ? 1 : 0);

	// TODO: smallscroll, largescroll

	/* Edit panel */

	edit_sectorsize->value(Int_TmpStr(new_sector_size));
	edit_newislands->value(new_islands_are_void ? 1 : 0);
	edit_samemode->value(same_mode_clears_selection ? 1 : 0);
	edit_autoadjustX->value(leave_offsets_alone ? 0 : 1);
	edit_multiselect->value(multi_select_modifier ? 2 : 0);

	/* glBSP panel */

	bsp_fast->value(glbsp_fast ? 1 : 0);
	bsp_verbose->value(glbsp_verbose ? 1 : 0);
	bsp_warn->value(glbsp_warn ? 1 : 0);
}


void UI_Preferences::SaveValues()
{
	/* Theme stuff */

	if (theme_FLTK->value())
		gui_scheme = 0;
	else if (theme_GTK->value())
		gui_scheme = 1;
	else
		gui_scheme = 2;

	if (cols_default->value())
		gui_color_set = 0;
	else if (cols_bright->value())
		gui_color_set = 1;
	else
		gui_color_set = 2;

	gui_custom_bg = (rgb_color_t) bg_colorbox->color();
	gui_custom_ig = (rgb_color_t) ig_colorbox->color();
	gui_custom_fg = (rgb_color_t) fg_colorbox->color();

	/* General stuff */

	default_grid_snap = grid_snap->value() ? true : false;
	default_grid_size = atoi(grid_size->mvalue()->text);
	default_grid_mode = grid_mode->value();

	digits_set_zoom         = gen_digitzoom  ->value() ? true : false;
	mouse_wheel_scrolls_map = gen_wheelscroll->value() ? true : false;

	// TODO: smallscroll, largescroll

	/* Edit panel */

	new_sector_size = atoi(edit_sectorsize->value());
	new_sector_size = CLAMP(4, new_sector_size, 8192);

	new_islands_are_void = edit_newislands->value() ? true : false;
	same_mode_clears_selection = edit_samemode->value() ? true : false;
	leave_offsets_alone = edit_autoadjustX->value() ? false : true;
	multi_select_modifier = edit_multiselect->value() ? 2 : 0;

	/* glBSP panel */

	glbsp_fast = bsp_fast->value() ? true : false;
	glbsp_verbose = bsp_verbose->value() ? true : false;
	glbsp_warn = bsp_warn->value() ? true : false;
}


void UI_Preferences::LoadKeys()
{
	M_SortBindingsToVec(key_order, key_sort_mode, key_sort_rev);

	key_list->clear();

	for (unsigned int i = 0 ; i < key_order.size() ; i++)
	{
		const char *str = M_StringForBinding(key_order[i]);

		if (! str)
			break;

		key_list->add(str, (void *)(long)i);
	}

	key_list->select(1);
}


//------------------------------------------------------------------------

void CMD_Preferences()
{
	UI_Preferences * dialog = new UI_Preferences();

	dialog->Run();

	delete dialog;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
