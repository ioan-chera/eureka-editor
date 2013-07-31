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


class UI_EditKey : public UI_Escapable_Window
{
private:
	bool want_close;
	bool cancelled;

	bool grab_active;

	keycode_t key;

	Fl_Input  *key_name;
	Fl_Choice *context;
	Fl_Input  *func_name;

	Fl_Button *cancel;
	Fl_Button *ok_but;

private:
	bool ValidateKey()
	{
		keycode_t new_key = M_ParseKeyString(key_name->value());

		if (new_key > 0)
		{
			key = new_key;
			return true;
		}

		return false;
	}

	bool ValidateFunc()
	{
		// check for matching brackets
		const char *p = strchr(func_name->value(), '(');

		if (p && ! strchr(p, ')'))
			return false;

		key_context_e ctx = (key_context_e)(1 + context->value());

		return M_IsBindingFuncValid(ctx, func_name->value());
	}

	static void validate_callback(Fl_Widget *w, void *data)
	{
		UI_EditKey *dialog = (UI_EditKey *)data;

		bool valid_key  = dialog->ValidateKey();
		bool valid_func = dialog->ValidateFunc();

		dialog-> key_name->textcolor(valid_key  ? FL_FOREGROUND_COLOR : FL_RED);
		dialog->func_name->textcolor(valid_func ? FL_FOREGROUND_COLOR : FL_RED);

		// need to redraw the input boxes (otherwise get a mix of colors)
		dialog-> key_name->redraw();
		dialog->func_name->redraw();

		if (valid_key && valid_func)
			dialog->ok_but->activate();
		else
			dialog->ok_but->deactivate();
	}

	static void grab_key_callback(Fl_Button *w, void *data)
	{
		// FIXME
	}

	static void find_func_callback(Fl_Button *w, void *data)
	{
		// TODO: show a list of all commands
	}

	static void close_callback(Fl_Widget *w, void *data)
	{
		UI_EditKey *dialog = (UI_EditKey *)data;

		dialog->want_close = true;
		dialog->cancelled  = true;
	}

	static void ok_callback(Fl_Button *w, void *data)
	{
		UI_EditKey *dialog = (UI_EditKey *)data;

		dialog->want_close = true;
	}

public:
	UI_EditKey(keycode_t _key, key_context_e ctx, const char *_func) :
		UI_Escapable_Window(400, 236, "Edit Key Binding"),
		want_close(false), cancelled(false), grab_active(false),
		key(_key)
	{
		callback(close_callback, this);

		{ key_name = new Fl_Input(85, 25, 150, 25, "Key:");
		  if (key)
			  key_name->value(M_KeyToString(key));
		  key_name->when(FL_WHEN_CHANGED);
		  key_name->callback((Fl_Callback*)validate_callback, this);
		}
		{ Fl_Button *o = new Fl_Button(250, 25, 85, 25, "Grab");
		  o->callback((Fl_Callback*)grab_key_callback, this);
		  o->hide();  // TODO: IMPLEMENT THIS
		}

		{ context = new Fl_Choice(85, 65, 150, 25, "View:");
		  context->add("Browser|Render|Linedef|Sector|Thing|Vertex|General");
		  context->value((int)ctx - 1);
		  context->callback((Fl_Callback*)validate_callback, this);
		}

		{ func_name = new Fl_Input(85, 105, 210, 25, "Function:");
		  func_name->value(_func);
		  func_name->when(FL_WHEN_CHANGED);
		  func_name->callback((Fl_Callback*)validate_callback, this);
		}
		{ Fl_Button *o = new Fl_Button(310, 105, 75, 25, "Find");
		  o->callback((Fl_Callback*)find_func_callback, this);
		  o->hide();  // TODO: IMPLEMENT THIS
		}

		{ Fl_Group *o = new Fl_Group(0, 170, 400, 66);

		  o->box(FL_FLAT_BOX);
		  o->color(WINDOW_BG, WINDOW_BG);

		  { cancel = new Fl_Button(170, 184, 80, 35, "Cancel");
			cancel->callback((Fl_Callback*)close_callback, this);
		  }
		  { ok_but = new Fl_Button(295, 184, 80, 35, "OK");
			ok_but->labelfont(1);
			ok_but->callback((Fl_Callback*)ok_callback, this);
			ok_but->deactivate();
		  }
		  o->end();
		}

		end();
	}

	bool Run(keycode_t *key_v, key_context_e *ctx_v, const char **func_v)
	{
		*key_v  = 0;
		*ctx_v  = KCTX_NONE;
		*func_v = NULL;

		// check the initial state
		validate_callback(this, this);

		set_modal();

		show();

		Fl::focus(func_name);

		while (! want_close)
			Fl::wait(0.2);

		if (cancelled)
			return false;

		*key_v  = key;
		*ctx_v  = (key_context_e)(1 + context->value());
		*func_v = StringDup(func_name->value());

		return true;
	}
};


//------------------------------------------------------------------------


class UI_Preferences : public Fl_Double_Window
{
private:
	bool want_quit;
	bool want_discard;

	char key_sort_mode;
	bool key_sort_rev;

	// normally zero (not waiting for a key)
	int awaiting_line;

	static void  close_callback(Fl_Widget *w, void *data);
	static void  color_callback(Fl_Button *w, void *data);
	static void aspect_callback(Fl_Widget *w, void *data);

	static void sort_key_callback(Fl_Button *w, void *data);
	static void bind_key_callback(Fl_Button *w, void *data);
	static void edit_key_callback(Fl_Button *w, void *data);
	static void  add_key_callback(Fl_Button *w, void *data);
	static void  del_key_callback(Fl_Button *w, void *data);
	static void    reset_callback(Fl_Button *w, void *data);

public:
	UI_Preferences();

	void Run();

	void LoadValues();
	void SaveValues();

	void LoadKeys();
	void ReloadKeys();

	int GridSizeToChoice(int size);

	/* FLTK override */
	int handle(int event);

	void ClearWaiting();
	void SetBinding(keycode_t key);


public:
	Fl_Tabs *tabs;

	Fl_Button *apply_but;
	Fl_Button *discard_but;

	/* General Tab */

	Fl_Round_Button *theme_FLTK;
	Fl_Round_Button *theme_GTK;
	Fl_Round_Button *theme_plastic;

	Fl_Round_Button *cols_default;
	Fl_Round_Button *cols_bright;
	Fl_Round_Button *cols_custom;

	Fl_Button *bg_colorbox;
	Fl_Button *ig_colorbox;
	Fl_Button *fg_colorbox;

	Fl_Check_Button *gen_autoload;
	Fl_Check_Button *gen_maximized;
	Fl_Check_Button *gen_swapsides;

	/* Edit Tab */

	Fl_Input  *edit_def_port;
	Fl_Choice *edit_def_mode;

	Fl_Check_Button *edit_newislands;
	Fl_Check_Button *edit_samemode;
	Fl_Check_Button *edit_autoadjustX;
	Fl_Check_Button *edit_multiselect;
	Fl_Choice *edit_modkey;
	Fl_Int_Input *edit_sectorsize;

	/* Grid Tab */

	Fl_Check_Button *gen_scrollbars;

	Fl_Check_Button *grid_snap;
	Fl_Choice *grid_mode;
	Fl_Choice *grid_toggle;
	Fl_Choice *grid_size;

	Fl_Check_Button *gen_digitzoom;
	Fl_Choice *gen_smallscroll;
	Fl_Choice *gen_largescroll;

	Fl_Check_Button *gen_wheelscroll;
	Fl_Check_Button *grid_hide_free;

	Fl_Button *dotty_axis;
	Fl_Button *dotty_major;
	Fl_Button *dotty_minor;
	Fl_Button *dotty_point;

	Fl_Button *normal_axis;
	Fl_Button *normal_main;
	Fl_Button *normal_flat;
	Fl_Button *normal_small;

	/* Keys Tab */

	Fl_Hold_Browser *key_list;
	Fl_Button *key_group;
	Fl_Button *key_key;
	Fl_Button *key_func;

	Fl_Button *key_bind;
	Fl_Button *key_copy;
	Fl_Button *key_edit;
	Fl_Button *key_delete;
	Fl_Button *key_reset;

	/* Mouse Tab */

	/* Other Tab */

	Fl_Choice      *rend_aspect;
	Fl_Float_Input *rend_asp_custom;

	Fl_Check_Button *rend_lock_gravity;

	Fl_Check_Button *bsp_warn;
	Fl_Check_Button *bsp_verbose;
	Fl_Check_Button *bsp_fast;
};


#define R_SPACES  "   "


UI_Preferences::UI_Preferences() :
	  Fl_Double_Window(PREF_WINDOW_W, PREF_WINDOW_H, PREF_WINDOW_TITLE),
	  want_quit(false), want_discard(false),
	  key_sort_mode('c'), key_sort_rev(false),
	  awaiting_line(0)
{
	if (gui_color_set == 2)
		color(fl_gray_ramp(4));
	else
		color(WINDOW_BG);

	callback(close_callback, this);

	{ tabs = new Fl_Tabs(0, 0, 585, 435);

	  /* ---- General Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 405, " General" R_SPACES);
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
		{ Fl_Box* o = new Fl_Box(30, 240, 280, 35, "Miscellaneous");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ gen_autoload = new Fl_Check_Button(50, 280, 350, 25, " automatically open the most recent pwad");
		  gen_autoload->down_box(FL_DOWN_BOX);
		}
		{ gen_swapsides = new Fl_Check_Button(50, 315, 350, 25, " swap upper and lower sidedefs in Linedef panel");
		  gen_swapsides->down_box(FL_DOWN_BOX);
		}
		{ gen_maximized = new Fl_Check_Button(50, 350, 350, 25, " maximize the window when Eureka starts");
		  gen_maximized->down_box(FL_DOWN_BOX);
		  gen_maximized->hide();  // DISABLED, PENDING REMOVAL
		}
		o->end();
	  }

	  /* ---- Editing Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Editing" R_SPACES);
		o->labelsize(16);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 50, 355, 30, "Editing Options");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ edit_def_port = new Fl_Input(150, 85, 95, 25, "default port: ");
		  edit_def_port->align(FL_ALIGN_LEFT);
		}
		{ edit_def_mode = new Fl_Choice(440, 85, 105, 25, "default edit mode: ");
		  edit_def_mode->align(FL_ALIGN_LEFT);
		  edit_def_mode->add("Things|Linedefs|Sectors|Vertices");
		}
		{ edit_newislands = new Fl_Check_Button(50, 120, 265, 30, " new islands have void interior");
		  edit_newislands->down_box(FL_DOWN_BOX);
		}
		{ edit_samemode = new Fl_Check_Button(50, 180, 270, 30, " same mode key will clear selection");
		  edit_samemode->down_box(FL_DOWN_BOX);
		}
		{ edit_autoadjustX = new Fl_Check_Button(50, 150, 260, 30, " auto-adjust X offsets");
		  edit_autoadjustX->down_box(FL_DOWN_BOX);
		}
		{ edit_multiselect = new Fl_Check_Button(50, 210, 275, 30, " multi-select requires a modifier key");
		  edit_multiselect->down_box(FL_DOWN_BOX);
		}
		{ edit_modkey = new Fl_Choice(370, 210, 95, 30, "---->   ");
		  edit_modkey->down_box(FL_BORDER_BOX);
		  edit_modkey->add("CTRL");
		  edit_modkey->value(0);
		}
		{ edit_sectorsize = new Fl_Int_Input(440, 120, 105, 25, "new sector size:");
		}
		o->end();
	  }

	  /* ---- Grid Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Grid" R_SPACES);
		o->labelsize(16);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 50, 355, 30, "Map Grid and Scrolling");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ gen_scrollbars = new Fl_Check_Button(50, 80, 245, 25, " enable scroll-bars for map view");
		  gen_scrollbars->down_box(FL_DOWN_BOX);
		}
		{ grid_snap = new Fl_Check_Button(50, 120, 235, 25, " default SNAP mode");
		  grid_snap->down_box(FL_DOWN_BOX);
		}
		{ grid_mode = new Fl_Choice(435, 120, 95, 25, "default grid type ");
		  grid_mode->down_box(FL_BORDER_BOX);
		  grid_mode->add("OFF|Dotty|Normal");
		}
		{ grid_toggle = new Fl_Choice(435, 155, 95, 25, "grid toggle types ");
		  grid_toggle->down_box(FL_BORDER_BOX);
		  grid_toggle->add("BOTH|Dotty|Normal");
		}
		{ grid_size = new Fl_Choice(435, 190, 95, 25, "default grid size ");
		  grid_size->down_box(FL_BORDER_BOX);
		  grid_size->add("1024|512|256|128|64|32|16|8|4|2");
		}
		{ gen_digitzoom = new Fl_Check_Button(50, 155, 240, 25, " digit keys zoom the map");
		  gen_digitzoom->down_box(FL_DOWN_BOX);
		}
		{ gen_smallscroll = new Fl_Choice(435, 155, 95, 25, "small scroll step ");
		  gen_smallscroll->down_box(FL_BORDER_BOX);
		  gen_smallscroll->hide();
		}
		{ gen_largescroll = new Fl_Choice(435, 190, 95, 25, "large scroll step ");
		  gen_largescroll->down_box(FL_BORDER_BOX);
		  gen_largescroll->hide();
		}
		{ gen_wheelscroll = new Fl_Check_Button(50, 190, 245, 25, " mouse wheel scrolls the map");
		  gen_wheelscroll->down_box(FL_DOWN_BOX);
		}
		{ grid_hide_free = new Fl_Check_Button(50, 230, 245, 25, " hide grid in FREE mode");
		  grid_hide_free->down_box(FL_DOWN_BOX);
		}

		{ Fl_Box* o = new Fl_Box(25, 270, 355, 30, "Grid Colors");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}

		{ normal_axis = new Fl_Button(150 + 0*55, 300, 45, 25, "Normal Grid : ");
		  normal_axis->box(FL_BORDER_BOX);
		  normal_axis->align(FL_ALIGN_LEFT);
		  normal_axis->callback((Fl_Callback*)color_callback, this);
		}
		{ normal_main = new Fl_Button(150 + 1*55, 300, 45, 25, "");
		  normal_main->box(FL_BORDER_BOX);
		  normal_main->align(FL_ALIGN_RIGHT);
		  normal_main->callback((Fl_Callback*)color_callback, this);
		}
		{ normal_flat = new Fl_Button(150 + 2*55, 300, 45, 25, "");
		  normal_flat->box(FL_BORDER_BOX);
		  normal_flat->align(FL_ALIGN_RIGHT);
		  normal_flat->callback((Fl_Callback*)color_callback, this);
		}
		{ normal_small = new Fl_Button(150 + 3*55, 300, 45, 25, "");
		  normal_small->box(FL_BORDER_BOX);
		  normal_small->align(FL_ALIGN_RIGHT);
		  normal_small->callback((Fl_Callback*)color_callback, this);
		}

		{ dotty_axis = new Fl_Button(150 + 0*55, 340, 45, 25, "Dotty Grid : ");
		  dotty_axis->box(FL_BORDER_BOX);
		  dotty_axis->align(FL_ALIGN_LEFT);
		  dotty_axis->callback((Fl_Callback*)color_callback, this);
		}
		{ dotty_major = new Fl_Button(150 + 1*55, 340, 45, 25, "");
		  dotty_major->box(FL_BORDER_BOX);
		  dotty_major->align(FL_ALIGN_RIGHT);
		  dotty_major->callback((Fl_Callback*)color_callback, this);
		}
		{ dotty_minor = new Fl_Button(150 + 2*55, 340, 45, 25, "");
		  dotty_minor->box(FL_BORDER_BOX);
		  dotty_minor->align(FL_ALIGN_RIGHT);
		  dotty_minor->callback((Fl_Callback*)color_callback, this);
		}
		{ dotty_point = new Fl_Button(150 + 3*55, 340, 45, 25, "");
		  dotty_point->box(FL_BORDER_BOX);
		  dotty_point->align(FL_ALIGN_RIGHT);
		  dotty_point->callback((Fl_Callback*)color_callback, this);
		}

		o->end();
	  }

	  /* ---- Key bindings Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Keys" R_SPACES);
		o->labelsize(16);
		o->hide();

		{ Fl_Box* o = new Fl_Box(15, 50, 355, 30, "Key Bindings");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}

		{ key_key = new Fl_Button(30, 90, 120, 25, "KEY");
		  key_key->color((Fl_Color)231);
		  key_key->align(Fl_Align(FL_ALIGN_INSIDE));
		  key_key->callback((Fl_Callback*)sort_key_callback, this);
		}
		{ key_group = new Fl_Button(155, 90, 90, 25, "MODE");
		  key_group->color((Fl_Color)231);
		  key_group->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		  key_group->callback((Fl_Callback*)sort_key_callback, this);
		}
		{ key_func = new Fl_Button(250, 90, 190, 25, "FUNCTION");
		  key_func->color((Fl_Color)231);
		  key_func->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		  key_func->callback((Fl_Callback*)sort_key_callback, this);
		}
		{ key_list = new Fl_Hold_Browser(30, 115, 430, 295);
		  key_list->textfont(FL_COURIER);
		}
		{ key_bind = new Fl_Button(470, 140, 90, 30, "Bind");
		  key_bind->callback((Fl_Callback*)bind_key_callback, this);
		  key_bind->shortcut(FL_Enter);
		}
		{ key_copy = new Fl_Button(470, 185, 90, 30, "&Copy");
		  key_copy->callback((Fl_Callback*)add_key_callback, this);
		}
		{ key_edit = new Fl_Button(470, 230, 90, 30, "&Edit");
		  key_edit->callback((Fl_Callback*)edit_key_callback, this);
		}
		{ key_delete = new Fl_Button(470, 275, 90, 30, "Delete");
		  key_delete->callback((Fl_Callback*)del_key_callback, this);
		  key_delete->shortcut(FL_Delete);
		}
		{ key_reset = new Fl_Button(470, 335, 90, 50, "Reset\nDefaults");
		  key_reset->callback((Fl_Callback*)reset_callback, this);
		}
		o->end();
	  }

	  /* ---- Mouse Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Mouse" R_SPACES);
		o->labelsize(16);
		o->hide();
	    
		o->end();
	  }

	  /* ---- Other Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Other" R_SPACES);
		o->selection_color(FL_LIGHT1);
		o->labelsize(16);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 50, 280, 30, "3D Preview Options");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ rend_aspect = new Fl_Choice(155, 90, 105, 25, "Aspect ratio: ");
		  rend_aspect->down_box(FL_BORDER_BOX);
		  rend_aspect->add("    1:1|    4:3|  16:9|  16:10|  24:10|CUSTOM");
		  rend_aspect->callback(aspect_callback, this);
		}
		{ rend_asp_custom = new Fl_Float_Input(315, 90, 95, 25, "---->  ");
		  rend_asp_custom->deactivate();
		}
		{ rend_lock_gravity = new Fl_Check_Button(60, 125, 360, 30, " Locked gravity -- cannot move up or down");
		  rend_lock_gravity->down_box(FL_DOWN_BOX);
		}

		{ Fl_Box* o = new Fl_Box(25, 250, 280, 30, "glBSP Node Building");
		  o->labelfont(1);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ bsp_warn = new Fl_Check_Button(60, 290, 220, 30, " Show all warning messages");
		  bsp_warn->down_box(FL_DOWN_BOX);
		}
		{ bsp_verbose = new Fl_Check_Button(60, 320, 350, 30, " Verbose -- show information about each level");
		  bsp_verbose->down_box(FL_DOWN_BOX);
		}
		{ bsp_fast = new Fl_Check_Button(60, 350, 440, 30, " Fast -- build the fastest way, but nodes may not be as good");
		  bsp_fast->down_box(FL_DOWN_BOX);
		}
		o->end();
	  }
	  tabs->end();
	}
	{ apply_but = new Fl_Button(480, 450, 95, 35, "Apply");
	  apply_but->labelfont(1);
	  apply_but->callback(close_callback, this);
	}
	{ discard_but = new Fl_Button(350, 450, 95, 35, "Discard");
	  discard_but->callback(close_callback, this);
	}

end();
}


//------------------------------------------------------------------------

void UI_Preferences::close_callback(Fl_Widget *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	prefs->want_quit = true;

	if (w == prefs->discard_but)
		prefs->want_discard = true;
}


void UI_Preferences::color_callback(Fl_Button *w, void *data)
{
//	UI_Preferences *dialog = (UI_Preferences *)data;

	uchar r, g, b;

	Fl::get_color(w->color(), r, g, b);

	if (! fl_color_chooser("New color:", r, g, b, 3))
		return;

	w->color(fl_rgb_color(r, g, b));

	w->redraw();
}


void UI_Preferences::aspect_callback(Fl_Widget *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	float aspect = atof(prefs->rend_asp_custom->value());
	bool  custom = false;

	switch (prefs->rend_aspect->value())
	{
		case 0: aspect =  1.0 / 1.0;  break;
		case 1: aspect =  4.0 / 3.0;  break;
		case 2: aspect = 16.0 / 9.0;  break;
		case 3: aspect = 16.0 / 10.0; break;
		case 4: aspect = 24.0 / 10.0; break;

		case 5: custom = true; break;
	}

	bool was_custom = (prefs->rend_asp_custom->active() ? true : false);

	if (! custom)
	{
		char aspect_buf[64];
		sprintf(aspect_buf, "%1.2f", aspect);
		prefs->rend_asp_custom->value(aspect_buf);
	}

	if (custom != was_custom)
	{
		if (custom)
			prefs->rend_asp_custom->activate();
		else
			prefs->rend_asp_custom->deactivate();

		prefs->redraw();
	}
}


void UI_Preferences::bind_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	int line = prefs->key_list->value();
	if (line < 1)
	{
		fl_beep();
		return;
	}

	int bind_idx = line - 1;

	// show we're ready to accept a new key

	const char *str = M_StringForBinding(bind_idx, true /* changing_key */);
	SYS_ASSERT(str);

	prefs->key_list->text(line, str);

	prefs->key_list->selection_color(FL_YELLOW);

	Fl::focus(prefs);

	prefs->awaiting_line = line;
}


void UI_Preferences::sort_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	if (w == prefs->key_group)
	{
		if (prefs->key_sort_mode != 'c')
		{
			prefs->key_sort_mode  = 'c';
			prefs->key_sort_rev = false;
		}
		else
			prefs->key_sort_rev = !prefs->key_sort_rev;
	}
	else if (w == prefs->key_key)
	{
		if (prefs->key_sort_mode != 'k')
		{
			prefs->key_sort_mode  = 'k';
			prefs->key_sort_rev = false;
		}
		else
			prefs->key_sort_rev = !prefs->key_sort_rev;
	}
	else if (w == prefs->key_func)
	{
		if (prefs->key_sort_mode != 'f')
		{
			prefs->key_sort_mode  = 'f';
			prefs->key_sort_rev = false;
		}
		else
			prefs->key_sort_rev = !prefs->key_sort_rev;
	}

	prefs->LoadKeys();
}


void UI_Preferences::add_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	int line = prefs->key_list->value();
	if (line < 1)
	{
		fl_beep();
		return;
	}

	int bind_idx = line - 1;

	keycode_t     new_key;
	key_context_e new_context;

	M_GetBindingInfo(bind_idx, &new_key, &new_context);

	const char *new_func = M_StringForFunc(bind_idx);

	
	M_AddLocalBinding(bind_idx, new_key, new_context, new_func);

	// we will reload the lines, so use a dummy one here

	prefs->key_list->insert(line + 1, "");
	prefs->key_list->select(line + 1);

	prefs->ReloadKeys();

	bind_key_callback(w, data);
}


void UI_Preferences::edit_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	int line = prefs->key_list->value();
	if (line < 1)
	{
		fl_beep();
		return;
	}

	int bind_idx = line - 1;

	keycode_t     new_key;
	key_context_e new_context;

	M_GetBindingInfo(bind_idx, &new_key, &new_context);

	const char *new_func = M_StringForFunc(bind_idx);


	UI_EditKey *dialog = new UI_EditKey(new_key, new_context, new_func);

	if (dialog->Run(&new_key, &new_context, &new_func))
	{
		// assume it works (since we validated it)
		M_SetLocalBinding(bind_idx, new_key, new_context, new_func);
	}

	delete dialog;

	prefs->ReloadKeys();

	Fl::focus(prefs->key_list);
}


void UI_Preferences::del_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	int line = prefs->key_list->value();
	if (line < 1)
	{
		fl_beep();
		return;
	}

	M_DeleteLocalBinding(line - 1);

	prefs->key_list->remove(line);
	prefs->ReloadKeys();

	if (line <= prefs->key_list->size())
	{
		prefs->key_list->select(line);

		Fl::focus(prefs->key_list);
	}
}


void UI_Preferences::reset_callback(Fl_Button *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	if (true)
	{
		int res = DLG_Confirm("Cancel|Reset",
		                      "You are about to reset all key bindings to their default "
							  "values.  Pressing the preference window's \"Apply\" button "
							  "will cause any changes you have made to be lost."
							  "\n\n"
							  "Are you sure you want to continue?");

		if (res <= 0)
			return;
	}

	M_CopyBindings(true /* from_defaults */);

	prefs->LoadKeys();
}


void UI_Preferences::Run()
{
	if (last_active_tab < tabs->children())
		tabs->value(tabs->child(last_active_tab));

	M_CopyBindings();

	LoadValues();
	LoadKeys();

	set_modal();

	show();

	while (! want_quit)
	{
		Fl::wait(0.2);
	}

	last_active_tab = tabs->find(tabs->value());

	if (want_discard)
	{
		LogPrintf("Preferences: discarded changes\n");
		return;
	}

	SaveValues();
	M_WriteConfigFile();

	M_ApplyBindings();
	M_SaveBindings();
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

	/* General Tab */

	gen_autoload   ->value(auto_load_recent ? 1 : 0);
	gen_maximized  ->value(begin_maximized  ? 1 : 0);
	gen_swapsides  ->value(swap_sidedefs    ? 1 : 0);

	/* Edit Tab */

	edit_def_port->value(default_port);
	edit_def_mode->value(CLAMP(0, default_edit_mode, 3));

	edit_sectorsize->value(Int_TmpStr(new_sector_size));
	edit_newislands->value(new_islands_are_void ? 1 : 0);
	edit_samemode->value(same_mode_clears_selection ? 1 : 0);
	edit_autoadjustX->value(leave_offsets_alone ? 0 : 1);
	edit_multiselect->value(multi_select_modifier ? 2 : 0);

	/* Grid Tab */

	if (default_grid_mode < 0 || default_grid_mode > 2)
		default_grid_mode = 2;

	if (grid_toggle_type < 0 || grid_toggle_type > 2)
		grid_toggle_type = 0;

	grid_snap->value(default_grid_snap ? 1 : 0);
	grid_size->value(GridSizeToChoice(default_grid_size));
	grid_mode->value(default_grid_mode);
	grid_toggle->value(grid_toggle_type);

	gen_digitzoom  ->value(digits_set_zoom ? 1 : 0);
	grid_hide_free ->value(grid_hide_in_free_mode ? 1 : 0);
	gen_wheelscroll->value(mouse_wheel_scrolls_map ? 1 : 0);
	gen_scrollbars ->value(map_scroll_bars ? 1 : 0);

	dotty_axis ->color(dotty_axis_col);
	dotty_major->color(dotty_major_col);
	dotty_minor->color(dotty_minor_col);
	dotty_point->color(dotty_point_col);

	normal_axis ->color(normal_axis_col);
	normal_main ->color(normal_main_col);
	normal_flat ->color(normal_flat_col);
	normal_small->color(normal_small_col);

	// TODO: smallscroll, largescroll

	/* Other Tab */

	render_aspect_ratio = CLAMP(10, render_aspect_ratio, 999);

	char aspect_buf[64];
	sprintf(aspect_buf, "%1.2f", render_aspect_ratio / 100.0);
	rend_asp_custom->value(aspect_buf);

	switch (render_aspect_ratio)
	{
		case  99: case 100: rend_aspect->value(0); break;
		case 133: case 134: rend_aspect->value(1); break;
		case 177: case 178: rend_aspect->value(2); break;
		case 159: case 160: rend_aspect->value(3); break;
		case 239: case 240: rend_aspect->value(4); break;

		default:  /* Custom */
			rend_aspect->value(5);
			rend_asp_custom->activate();
			break;
	}

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

	// update the colors
	// FIXME: how to reset the "default" colors??
	if (gui_color_set == 1)
	{
		Fl::background(236, 232, 228);
		Fl::background2(255, 255, 255);
		Fl::foreground(0, 0, 0);

		main_win->redraw();
	}
	else if (gui_color_set == 2)
	{
		Fl::background (RGB_RED(gui_custom_bg), RGB_GREEN(gui_custom_bg), RGB_BLUE(gui_custom_bg));
		Fl::background2(RGB_RED(gui_custom_ig), RGB_GREEN(gui_custom_ig), RGB_BLUE(gui_custom_ig));
		Fl::foreground (RGB_RED(gui_custom_fg), RGB_GREEN(gui_custom_fg), RGB_BLUE(gui_custom_fg));

		main_win->redraw();
	}

	/* General Tab */

	auto_load_recent  = gen_autoload   ->value() ? true : false;
	begin_maximized   = gen_maximized  ->value() ? true : false;
	swap_sidedefs     = gen_swapsides  ->value() ? true : false;

	/* Edit Tab */

	default_port = StringDup(edit_def_port->value());
	default_edit_mode = edit_def_mode->value();

	new_sector_size = atoi(edit_sectorsize->value());
	new_sector_size = CLAMP(4, new_sector_size, 8192);

	new_islands_are_void = edit_newislands->value() ? true : false;
	same_mode_clears_selection = edit_samemode->value() ? true : false;
	leave_offsets_alone = edit_autoadjustX->value() ? false : true;
	multi_select_modifier = edit_multiselect->value() ? 2 : 0;

	/* Grid Tab */

	default_grid_snap = grid_snap->value() ? true : false;
	default_grid_size = atoi(grid_size->mvalue()->text);
	default_grid_mode = grid_mode->value();
	grid_toggle_type  = grid_toggle->value();

	digits_set_zoom         = gen_digitzoom  ->value() ? true : false;
	grid_hide_in_free_mode  = grid_hide_free ->value() ? true : false;
	mouse_wheel_scrolls_map = gen_wheelscroll->value() ? true : false;
	map_scroll_bars         = gen_scrollbars ->value() ? true : false;

	dotty_axis_col  = (rgb_color_t) dotty_axis ->color();
	dotty_major_col = (rgb_color_t) dotty_major->color();
	dotty_minor_col = (rgb_color_t) dotty_minor->color();
	dotty_point_col = (rgb_color_t) dotty_point->color();

	normal_axis_col  = (rgb_color_t) normal_axis ->color();
	normal_main_col  = (rgb_color_t) normal_main ->color();
	normal_flat_col  = (rgb_color_t) normal_flat ->color();
	normal_small_col = (rgb_color_t) normal_small->color();

	// TODO: smallscroll, largescroll

	/* Other Tab */

	render_aspect_ratio = (int)(100 * atof(rend_asp_custom->value()) + 0.2);
	render_aspect_ratio = CLAMP(10, render_aspect_ratio, 999);

	glbsp_fast = bsp_fast->value() ? true : false;
	glbsp_verbose = bsp_verbose->value() ? true : false;
	glbsp_warn = bsp_warn->value() ? true : false;
}


void UI_Preferences::LoadKeys()
{
	M_SortBindings(key_sort_mode, key_sort_rev);
	M_DetectConflictingBinds();

	key_list->clear();

	for (int i = 0 ; i < M_NumBindings() ; i++)
	{
		const char *str = M_StringForBinding(i);
		SYS_ASSERT(str);

		key_list->add(str);
	}

	key_list->select(1);
}


void UI_Preferences::ReloadKeys()
{
	M_DetectConflictingBinds();

	for (int i = 0 ; i < M_NumBindings() ; i++)
	{
		const char *str = M_StringForBinding(i);

		key_list->text(i + 1, str);
	}
}


void UI_Preferences::ClearWaiting()
{
	if (awaiting_line > 0)
	{
		// restore the text line
		ReloadKeys();

		Fl::focus(key_list);
	}

	awaiting_line = 0;

	key_list->selection_color(FL_SELECTION_COLOR);
}


void UI_Preferences::SetBinding(keycode_t key)
{
	int bind_idx = awaiting_line - 1;

	M_ChangeBindingKey(bind_idx, key);

	ClearWaiting();
}


int UI_Preferences::handle(int event)
{
	if (awaiting_line > 0)
	{
		if (event == FL_KEYDOWN)
		{
			if (Fl::event_key() == FL_Escape)
			{
				ClearWaiting();
				return 1;
			}

			keycode_t key = M_TranslateKey(Fl::event_key(), Fl::event_state());

			if (key != 0)
			{
				SetBinding(key);
				return 1;
			}
		}

		if (event == FL_PUSH)
			ClearWaiting();
	}

	return Fl_Double_Window::handle(event);
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
