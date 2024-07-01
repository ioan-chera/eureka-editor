//------------------------------------------------------------------------
//  PREFERENCES DIALOG
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
#include "m_parse.h"

#include <algorithm>

#include "ui_window.h"
#include "ui_prefs.h"

#include <FL/Fl_Color_Chooser.H>


#define PREF_WINDOW_W  600
#define PREF_WINDOW_H  520

#define PREF_WINDOW_TITLE  "Eureka Preferences"


static int last_active_tab = 0;


class UI_EditKey : public UI_Escapable_Window
{
private:
	bool want_close;
	bool cancelled;

	bool awaiting_key;

	keycode_t key;

	Fl_Input  *key_name;
	Fl_Button *grab_but;

	Fl_Output *func_name;
	Fl_Menu_Button *func_choose;

	Fl_Choice *context;
	Fl_Input  *params;

	const editor_command_t *cur_cmd;

	Fl_Menu_Button *keyword_menu;
	Fl_Menu_Button *flag_menu;

	Fl_Button *cancel;
	Fl_Button *ok_but;

private:
	void BeginGrab()
	{
		SYS_ASSERT(! awaiting_key);

		awaiting_key = true;

		key_name->color(FL_YELLOW, FL_YELLOW);
		key_name->value("<\077\077\077>");
		key_name->textcolor(FL_FOREGROUND_COLOR);
		grab_but->deactivate();

		Fl::focus(this);

		redraw();
	}

	void FinishGrab()
	{
		if (! awaiting_key)
			return;

		awaiting_key = false;

		key_name->color(FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);
		grab_but->activate();

		redraw();
	}

	int handle(int event)
	{
		if (awaiting_key)
		{
			// escape key cancels
			if (event == FL_KEYDOWN && Fl::event_key() == FL_Escape)
			{
				FinishGrab();

				if (key)
					key_name->value(M_KeyToString(key).c_str());

				// if previous key was invalid, need to re-enable OK button
				validate_callback(this, this);

				return 1;
			}

			if (event == FL_KEYDOWN ||
				event == FL_PUSH    ||
				event == FL_MOUSEWHEEL)
			{
				keycode_t new_key = M_CookedKeyForEvent(event);

				if (new_key)
				{
					FinishGrab();

					key = new_key;
					key_name->value(M_KeyToString(key).c_str());

					// if previous key was invalid, need to re-enable OK button
					validate_callback(this, this);

					return 1;
				}
			}
		}

		return UI_Escapable_Window::handle(event);
	}

private:
	struct name_CMP_pred
	{
		inline bool operator() (const char * A, const char * B) const
		{
			return (strcmp(A, B) < 0);
		}
	};

	// need this because menu is in reverse order
	KeyContext ContextFromMenu()
	{
		int i = context->value();
		SYS_ASSERT(i >= 0 && i <= 6);
		return (KeyContext)((int)KeyContext::general - i);
	}

	void SetContext(KeyContext ctx)
	{
		int i = (int)KeyContext::general - (int)ctx;
		SYS_ASSERT(i >= 0 && i <= 6);
		context->value(i);
	}

	void AddContextToMenu(const char *lab, KeyContext ctx, KeyContext limit_ctx)
	{
		int flags = 0;

		if (limit_ctx != KeyContext::none && ctx != limit_ctx)
		{
			flags = FL_MENU_INACTIVE;
		}

		context->add(lab, 0, 0, 0, flags);
	}

	void PopulateContextMenu(KeyContext want_ctx)
	{
		KeyContext limit_ctx = KeyContext::none;

		if (cur_cmd && cur_cmd->req_context != KeyContext::none)
			limit_ctx = cur_cmd->req_context;

		context->clear();

		AddContextToMenu("General (Any)",  KeyContext::general, limit_ctx);

		AddContextToMenu("Linedefs",  KeyContext::line,    limit_ctx);
		AddContextToMenu("Sectors",   KeyContext::sector,  limit_ctx);
		AddContextToMenu("Things",    KeyContext::thing,   limit_ctx);
		AddContextToMenu("Vertices",  KeyContext::vertex,  limit_ctx);
		AddContextToMenu("3D View",   KeyContext::render,  limit_ctx);
		AddContextToMenu("Browser",   KeyContext::browser, limit_ctx);

		if (want_ctx != KeyContext::none)
			SetContext(want_ctx);
	}

	void PopulateFuncMenu(const char *find_name = NULL)
	{
		func_choose->clear();

		cur_cmd = NULL;

		// add names to menu, and find the current function
		char buffer[512];

		bool did_separator = false;

		for (int i = 0 ; ; i++)
		{
			const editor_command_t *cmd = LookupEditorCommand(i);

			if (! cmd)
				break;

			if (! did_separator && y_stricmp(cmd->group_name, "General") == 0)
			{
				func_choose->add("", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE);
				did_separator = true;
			}

			snprintf(buffer, sizeof(buffer), "%s/%s", cmd->group_name, cmd->name);

			func_choose->add(buffer, 0, 0, (void *)(intptr_t)i, 0 /* flags */);

			if (find_name && strcmp(cmd->name, find_name) == 0)
			{
				cur_cmd = cmd;
			}
		}

		if (cur_cmd)
			func_name->value(cur_cmd->name);
		else
			func_name->value("");
	}

	void Decode(KeyContext ctx, const char *str)
	{
		while (isspace(*str))
			str++;

		char func_buf[100];
		unsigned int pos = 0;

		while (*str && ! (isspace(*str) || *str == ':' || *str == '/') &&
			   pos + 4 < sizeof(func_buf))
		{
			func_buf[pos++] = *str++;
		}

		func_buf[pos] = 0;

		// this sets the 'cur_cmd' variable
		PopulateFuncMenu(func_buf);

		PopulateMenuList(keyword_menu, cur_cmd ? cur_cmd->keyword_list : NULL);
		PopulateMenuList(   flag_menu, cur_cmd ? cur_cmd->   flag_list : NULL);

		if (*str == ':')
			str++;

		while (isspace(*str))
			str++;

		params->value(str);
	}

	SString Encode()
	{
		SString buffer;
		buffer.reserve(1024);

		// should not happen
		if (! cur_cmd)
			return "ERROR";

		buffer = cur_cmd->name;
		buffer += ": ";
		buffer += params->value();

		return buffer;
	}

	void PopulateMenuList(Fl_Menu_Button *menu, const char *list)
	{
		menu->clear();

		if (! list || ! list[0])
		{
			menu->deactivate();
			return;
		}

		std::vector<SString> tokens;

		int num_tok = M_ParseLine(list, tokens, ParseOptions::noStrings);

		if (num_tok < 1)	// shouldn't happen
		{
			menu->deactivate();
			return;
		}

		for (int i = 0 ; i < num_tok ; i++)
			menu->add(tokens[i].c_str());

		menu->activate();
	}

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

	static void validate_callback(Fl_Widget *w, void *data)
	{
		UI_EditKey *dialog = (UI_EditKey *)data;

		bool valid_key = dialog->ValidateKey();

		dialog->key_name->textcolor(valid_key ? FL_FOREGROUND_COLOR : FL_RED);

		// need to redraw the input box (otherwise get a mix of colors)
		dialog->key_name->redraw();

		if (valid_key)
			dialog->ok_but->activate();
		else
			dialog->ok_but->deactivate();
	}

	static void grab_key_callback(Fl_Button *w, void *data)
	{
		UI_EditKey *dialog = (UI_EditKey *)data;

		dialog->BeginGrab();
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

	void SetNewFunction(int cmd_index)
	{
		const editor_command_t *old_cmd = cur_cmd;

		cur_cmd = LookupEditorCommand(cmd_index);

		if (! cur_cmd)  // shouldn't happen
			return;

		func_name->value(cur_cmd->name);

		KeyContext want_ctx = ContextFromMenu();

		if (cur_cmd->req_context != KeyContext::none)
			want_ctx = cur_cmd->req_context;
		else if (y_strnicmp(cur_cmd->name, "BR_", 3) == 0)
			want_ctx = KeyContext::browser;
		else if (old_cmd && old_cmd->req_context != KeyContext::none)
			want_ctx = KeyContext::general;

		PopulateContextMenu(want_ctx);

		PopulateMenuList(keyword_menu, cur_cmd ? cur_cmd->keyword_list : NULL);
		PopulateMenuList(   flag_menu, cur_cmd ? cur_cmd->   flag_list : NULL);

		redraw();
	}

	static void func_callback(Fl_Menu_Button *w, void *data)
	{
		UI_EditKey *dialog = (UI_EditKey *)data;

		int cmd_index = (int)(intptr_t)w->mvalue()->user_data_;
		SYS_ASSERT(cmd_index >= 0);

		dialog->SetNewFunction(cmd_index);
	}

	void ReplaceKeyword(const char *new_word)
	{
		// delete existing keyword, if any
		if (isalnum(params->value()[0]))
		{
			const char *str = params->value();

			int len = 0;

			while (str[len] && (isalnum(str[len]) || str[len] == '_'))
				len++;

			while (str[len] && isspace(str[len]))
				len++;

			params->replace(0, len, NULL);
		}

		if (params->size() > 0)
			params->replace(0, 0, " ");

		params->replace(0, 0, new_word);
	}

	void ReplaceFlag(const char *new_flag)
	{
		const char *str = params->value();

		// if flag is already present, remove it
		const char *pos = strstr(str, new_flag);

		if (pos)
		{
			int a = (int)(pos - str);
			int b = a + (int)strlen(new_flag);

			while (str[b] && isspace(str[b]))
				b++;

			params->replace(a, b, NULL);

			return;
		}

		// append the flag, adding a space if necessary
		int a = params->size();

		if (a > 0 && !isspace(str[a-1]))
		{
			params->replace(a, a, " ");
			a += 1;
		}

		params->replace(a, a, new_flag);
	}

	static void keyword_callback(Fl_Menu_Button *w, void *data)
	{
		UI_EditKey *dialog = (UI_EditKey *)data;

		dialog->ReplaceKeyword(w->text());
	}

	static void flag_callback(Fl_Menu_Button *w, void *data)
	{
		UI_EditKey *dialog = (UI_EditKey *)data;

		dialog->ReplaceFlag(w->text());
	}

public:
	UI_EditKey(keycode_t _key, KeyContext ctx, const char *_funcname) :
		UI_Escapable_Window(400, 306, "Edit Key Binding"),
		want_close(false), cancelled(false),
		awaiting_key(false),
		key(_key)
	{
		// _key may be zero (when "Add" button is used)

		if (ctx == KeyContext::none)
			ctx = KeyContext::general;

		callback(close_callback, this);

		key_name = new Fl_Input(85, 25, 150, 25, "Key:");
		key_name->when(FL_WHEN_CHANGED);
		key_name->callback((Fl_Callback*)validate_callback, this);

		if (key)
			key_name->value(M_KeyToString(key).c_str());

		grab_but = new Fl_Button(255, 25, 90, 25, "Re-bind");
		grab_but->callback((Fl_Callback*)grab_key_callback, this);

		func_name = new Fl_Output(85,  65, 150, 25, "Function:");

		func_choose = new Fl_Menu_Button(255,  65, 90, 25, "Choose");
		func_choose->callback((Fl_Callback*) func_callback, this);

		context = new Fl_Choice(85, 105, 150, 25, "Mode:");

		params = new Fl_Input(85, 145, 300, 25, "Params:");
		params->value("");
		params->when(FL_WHEN_CHANGED);

		keyword_menu = new Fl_Menu_Button( 85, 180, 135, 25, "Keywords...");
		keyword_menu->callback((Fl_Callback*) keyword_callback, this);

		flag_menu = new Fl_Menu_Button(250, 180, 135, 25, "Flags...");
		flag_menu->callback((Fl_Callback*) flag_callback, this);

		{ Fl_Group *o = new Fl_Group(0, 240, 400, 66);

			o->box(FL_FLAT_BOX);
			o->color(WINDOW_BG, WINDOW_BG);

			cancel = new Fl_Button(170, 254, 80, 35, "Cancel");
			cancel->callback((Fl_Callback*)close_callback, this);

			ok_but = new Fl_Button(295, 254, 80, 35, "OK");
			ok_but->labelfont(FL_BOLD);
			ok_but->callback((Fl_Callback*)ok_callback, this);
			ok_but->deactivate();

			o->end();
		}

		end();

		// parse line into function name and parameters
		Decode(ctx, _funcname);

		PopulateContextMenu(ctx == KeyContext::none ? KeyContext::general : ctx);
	}


	bool Run(keycode_t *key_v, KeyContext *ctx_v,
			 SString *func_v, bool start_grabbed)
	{
		*key_v  = 0;
		*ctx_v  = KeyContext::none;
		func_v->clear();

		// check the initial state
		validate_callback(this, this);

		set_modal();
		show();

		// need this for the 'start_grabbed' feature, get FLTK to
		// actually put (map) the window onto the screen.
		Fl::wait(0.1);
		Fl::wait(0.1);

		if (start_grabbed)
			BeginGrab();
		else
			Fl::focus(params);

		while (! want_close)
		{
			Fl::wait(0.2);
		}

		if (cancelled)
			return false;

		*key_v  = key;
		*ctx_v  = ContextFromMenu();
		*func_v = Encode();

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

	const opt_desc_t *const options;

	static void  close_callback(Fl_Widget *w, void *data);
	static void  color_callback(Fl_Button *w, void *data);

	static void sort_key_callback(Fl_Button *w, void *data);
	static void bind_key_callback(Fl_Button *w, void *data);
	static void edit_key_callback(Fl_Button *w, void *data);
	static void  del_key_callback(Fl_Button *w, void *data);

	static void reset_callback(Fl_Button *w, void *data);

public:
	UI_Preferences(const opt_desc_t *options);

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

	void EnsureKeyVisible(int line);

public:
	Fl_Tabs *tabs;

	Fl_Button *apply_but;
	Fl_Button *discard_but;

	/* General Tab */

	Fl_Round_Button *theme_plastic;
	Fl_Round_Button *theme_GTK;
	Fl_Round_Button *theme_FLTK;

	Fl_Round_Button *cols_bright;
	Fl_Round_Button *cols_default;
	Fl_Round_Button *cols_custom;

	Fl_Button *bg_colorbox;
	Fl_Button *ig_colorbox;
	Fl_Button *fg_colorbox;

	Fl_Check_Button *gen_autoload;
	Fl_Check_Button *gen_maximized;
	Fl_Check_Button *gen_swapsides;

	/* Keys Tab */

	Fl_Hold_Browser *key_list;

	Fl_Button *key_group;
	Fl_Button *key_key;
	Fl_Button *key_func;

	Fl_Button *key_add;
	Fl_Button *key_copy;
	Fl_Button *key_edit;
	Fl_Button *key_delete;
	Fl_Button *key_rebind;

	/* Edit Tab */

	Fl_Input  *edit_def_port;
	Fl_Choice *edit_def_mode;

	Fl_Check_Button *edit_samemode;
	Fl_Check_Button *edit_autoadjustX;
	Fl_Check_Button *edit_add_del;
	Fl_Check_Button *edit_full_1S;
	Fl_Int_Input    *edit_sectorsize;
	Fl_Input        *edit_userratio;
	Fl_Choice       *edit_lineinfo;

	Fl_Check_Button *brow_smalltex;
	Fl_Check_Button *brow_combo;

	/* Grid Tab */

	Fl_Check_Button *gen_scrollbars;

	Fl_Choice *grid_cur_style;
	Fl_Check_Button *grid_snap;
	Fl_Check_Button *grid_enabled;
	Fl_Choice *grid_size;

	Fl_Check_Button *grid_hide_free;
	Fl_Check_Button *grid_flatrender;
	Fl_Check_Button *grid_spriterend;
	Fl_Check_Button *grid_indicator;

	Fl_Button *dotty_axis;
	Fl_Button *dotty_major;
	Fl_Button *dotty_minor;
	Fl_Button *dotty_point;

	Fl_Button *normal_axis;
	Fl_Button *normal_main;
	Fl_Button *normal_flat;
	Fl_Button *normal_small;

	/* 3D Tab */

	Fl_Float_Input  *rend_aspect;;

	Fl_Check_Button *rend_high_detail;
	Fl_Check_Button *rend_lock_grav;

	Fl_Choice *rend_far_clip;

	/* Nodes Tab */

	Fl_Check_Button *nod_on_save;
	Fl_Check_Button *nod_fast;
	Fl_Check_Button *nod_warn;

	Fl_Choice *nod_factor;

	Fl_Check_Button *nod_gl_nodes;
	Fl_Check_Button *nod_force_v5;
	Fl_Check_Button *nod_force_zdoom;
	Fl_Check_Button *nod_compress;

	/* Other Tab */

	Fl_Button * reset_conf;
	Fl_Button * reset_keys;
};


#define R_SPACES  "  "


UI_Preferences::UI_Preferences(const opt_desc_t *options) :
	  Fl_Double_Window(PREF_WINDOW_W, PREF_WINDOW_H, PREF_WINDOW_TITLE),
	  want_quit(false), want_discard(false),
	  key_sort_mode('k'), key_sort_rev(false),
	  awaiting_line(0), options(options)
{
	if (config::gui_color_set == 2)
		color(fl_gray_ramp(4));
	else
		color(WINDOW_BG);

	callback(close_callback, this);

	{ tabs = new Fl_Tabs(0, 0, PREF_WINDOW_W-15, PREF_WINDOW_H-70);
	  // tabs->selection_color(FL_WHITE);

	  /* ---- General Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 405, " General" R_SPACES);
		o->labelsize(16);
		o->selection_color(FL_DARK2);
		// o->hide();

		{ Fl_Box* o = new Fl_Box(25, 45, 145, 30, "GUI Appearance");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ Fl_Group* o = new Fl_Group(45, 90, 250, 115);
		  { theme_plastic = new Fl_Round_Button(50, 120, 165, 25, " Plastic theme ");
			theme_plastic->type(102);
			theme_plastic->down_box(FL_ROUND_DOWN_BOX);
		  }
		  { theme_GTK = new Fl_Round_Button(50, 90, 150, 25, " GTK+ theme ");
			theme_GTK->type(102);
			theme_GTK->down_box(FL_ROUND_DOWN_BOX);
		  }
		  { theme_FLTK = new Fl_Round_Button(50, 150, 150, 25, " FLTK theme");
			theme_FLTK->type(102);
			theme_FLTK->down_box(FL_ROUND_DOWN_BOX);
		  }
		  o->end();
		}
		{ Fl_Group* o = new Fl_Group(220, 90, 190, 90);
		  { cols_bright = new Fl_Round_Button(245, 90, 140, 25, "bright colors");
			cols_bright->type(102);
			cols_bright->down_box(FL_ROUND_DOWN_BOX);
		  }
		  { cols_default = new Fl_Round_Button(245, 120, 135, 25, "default colors");
			cols_default->type(102);
			cols_default->down_box(FL_ROUND_DOWN_BOX);
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
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ gen_autoload = new Fl_Check_Button(50, 280, 380, 25, " automatically open the most recent pwad");
		}
		{ gen_swapsides = new Fl_Check_Button(50, 310, 380, 25, " swap upper and lower sidedefs in Linedef panel");
		}
		{ gen_maximized = new Fl_Check_Button(50, 340, 380, 25, " maximize the window when Eureka starts");
		  // not supported on MacOS X
		  // (on that platform we should restore last window position, but I don't
		  //  know how to code that)
#ifdef __APPLE__
		  gen_maximized->hide();
#endif
		}
		o->end();
	  }

	  /* ---- Key bindings Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Keys" R_SPACES);
		o->labelsize(16);
		o->selection_color(FL_DARK2);
		o->hide();

		{ Fl_Box* o = new Fl_Box(20, 45, 355, 30, "Key Bindings");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}

		{ key_key = new Fl_Button(25, 87, 125, 25, "KEY");
		  key_key->color((Fl_Color)231);
		  key_key->align(Fl_Align(FL_ALIGN_INSIDE));
		  key_key->callback((Fl_Callback*)sort_key_callback, this);
		}
		{ key_group = new Fl_Button(155, 87, 75, 25, "MODE");
		  key_group->color((Fl_Color)231);
		  key_group->align(Fl_Align(FL_ALIGN_INSIDE));
		  key_group->callback((Fl_Callback*)sort_key_callback, this);
		}
		{ key_func = new Fl_Button(235, 87, 205, 25, "FUNCTION");
		  key_func->color((Fl_Color)231);
		  key_func->align(Fl_Align(FL_ALIGN_INSIDE));
		  key_func->callback((Fl_Callback*)sort_key_callback, this);
		}
		{ key_list = new Fl_Hold_Browser(20, 115, 442, 308);
		  key_list->textfont(FL_COURIER);
		  key_list->has_scrollbar(Fl_Browser_::VERTICAL);
		}
		{ key_add = new Fl_Button(480, 155, 85, 30, "&Add");
		  key_add->callback((Fl_Callback*)edit_key_callback, this);
		}
		{ key_copy = new Fl_Button(480, 195, 85, 30, "&Copy");
		  key_copy->callback((Fl_Callback*)edit_key_callback, this);
		}
		{ key_edit = new Fl_Button(480, 235, 85, 30, "&Edit");
		  key_edit->callback((Fl_Callback*)edit_key_callback, this);
		}
		{ key_delete = new Fl_Button(480, 275, 85, 30, "Delete");
		  key_delete->callback((Fl_Callback*)del_key_callback, this);
		  key_delete->shortcut(FL_Delete);
		}
		{ key_rebind = new Fl_Button(480, 370, 85, 30, "&Re-bind");
		  key_rebind->callback((Fl_Callback*)bind_key_callback, this);
		  // key_rebind->shortcut(FL_Enter);
		}
		o->end();
	  }

	  /* ---- Editing Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Editing" R_SPACES);
		o->labelsize(16);
		o->selection_color(FL_DARK2);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 45, 355, 30, "Editing Options");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ edit_def_port = new Fl_Input(150, 85, 95, 25, "default port: ");
		  edit_def_port->align(FL_ALIGN_LEFT);
		}
		{ edit_def_mode = new Fl_Choice(440, 85, 105, 25, "default edit mode: ");
		  edit_def_mode->align(FL_ALIGN_LEFT);
		  edit_def_mode->add("Things|Linedefs|Sectors|Vertices");
		}
		{ edit_autoadjustX = new Fl_Check_Button(50, 150, 260, 30, " auto-adjust X offsets");
		}
		{ edit_samemode = new Fl_Check_Button(50, 180, 270, 30, " same mode key will clear selection");
		}
		{ edit_add_del = new Fl_Check_Button(50, 210, 270, 30, " enable sidedef ADD / DEL buttons");
		}
		{ edit_full_1S = new Fl_Check_Button(50, 240, 270, 30, " show all textures on a one-sided linedef");
		}
		{ edit_sectorsize = new Fl_Int_Input(440, 120, 105, 25, "new sector size:");
		}
		{ edit_userratio = new Fl_Input(440, 150, 105, 25, "user ratio:");
		}
		{ edit_lineinfo = new Fl_Choice(440, 180, 105, 25, "line info:");
		  edit_lineinfo->add("NONE|Length|Angle|Ratio|Len+Ang|Len+Ratio");
		}

		{ Fl_Box* o = new Fl_Box(25, 295, 355, 30, "Browser Options");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ brow_smalltex = new Fl_Check_Button(50, 330, 265, 30, " smaller textures");
		}
		{ brow_combo = new Fl_Check_Button(50, 360, 265, 30, " combine flats and textures in a single browser");
		}
		o->end();
	  }

	  /* ---- Grid Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Grid" R_SPACES);
		o->labelsize(16);
		o->selection_color(FL_DARK2);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 45, 355, 30, "Map Grid and Scrolling");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ grid_cur_style = new Fl_Choice(125, 90, 95, 25, "grid style ");
		  grid_cur_style->add("Squares|Dotty");
		}
		{ grid_enabled = new Fl_Check_Button(50, 125, 95, 25, " default grid to ON");
		}
		{ grid_snap = new Fl_Check_Button(50, 155, 235, 25, " default snap to ON");
		}
		{ grid_flatrender = new Fl_Check_Button(50, 185, 270, 25, " default sector-render to ON");
		}
		{ grid_spriterend = new Fl_Check_Button(50, 215, 270, 25, " default sprites to ON");
		}
		{ grid_size = new Fl_Choice(420, 90, 95, 25, "default grid size ");
		  grid_size->add("1024|512|256|192|128|64|32|16|8|4|2");
		}
		{ gen_scrollbars = new Fl_Check_Button(300, 125, 245, 25, " enable scroll-bars for map view");
		}
		{ grid_indicator = new Fl_Check_Button(300, 155, 245, 25, " enable snap-pos indicator");
		}
		{ grid_hide_free = new Fl_Check_Button(300, 185, 245, 25, " hide grid in FREE mode");
		}

		{ Fl_Box* o = new Fl_Box(25, 270, 355, 30, "Grid Colors");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}

		{ normal_axis = new Fl_Button(150 + 0*55, 300, 45, 25, "Normal Grid : ");
		  normal_axis->box(FL_BORDER_BOX);
		  normal_axis->align(FL_ALIGN_LEFT);
		  normal_axis->callback((Fl_Callback*)color_callback, this);
		  normal_axis->tooltip("X/Y axis color");
		}
		{ normal_main = new Fl_Button(150 + 1*55, 300, 45, 25, "");
		  normal_main->box(FL_BORDER_BOX);
		  normal_main->align(FL_ALIGN_RIGHT);
		  normal_main->callback((Fl_Callback*)color_callback, this);
		  normal_main->tooltip("large square color");
		}
		{ normal_flat = new Fl_Button(150 + 2*55, 300, 45, 25, "");
		  normal_flat->box(FL_BORDER_BOX);
		  normal_flat->align(FL_ALIGN_RIGHT);
		  normal_flat->callback((Fl_Callback*)color_callback, this);
		  normal_flat->tooltip("64x64 square color");
		}
		{ normal_small = new Fl_Button(150 + 3*55, 300, 45, 25, "");
		  normal_small->box(FL_BORDER_BOX);
		  normal_small->align(FL_ALIGN_RIGHT);
		  normal_small->callback((Fl_Callback*)color_callback, this);
		  normal_small->tooltip("small square color");
		}

		{ dotty_axis = new Fl_Button(150 + 0*55, 340, 45, 25, "Dotty Grid : ");
		  dotty_axis->box(FL_BORDER_BOX);
		  dotty_axis->align(FL_ALIGN_LEFT);
		  dotty_axis->callback((Fl_Callback*)color_callback, this);
		  dotty_axis->tooltip("X/Y axis color");
		}
		{ dotty_major = new Fl_Button(150 + 1*55, 340, 45, 25, "");
		  dotty_major->box(FL_BORDER_BOX);
		  dotty_major->align(FL_ALIGN_RIGHT);
		  dotty_major->callback((Fl_Callback*)color_callback, this);
		  dotty_major->tooltip("large square color");
		}
		{ dotty_minor = new Fl_Button(150 + 2*55, 340, 45, 25, "");
		  dotty_minor->box(FL_BORDER_BOX);
		  dotty_minor->align(FL_ALIGN_RIGHT);
		  dotty_minor->callback((Fl_Callback*)color_callback, this);
		  dotty_minor->tooltip("small square color");
		}
		{ dotty_point = new Fl_Button(150 + 3*55, 340, 45, 25, "");
		  dotty_point->box(FL_BORDER_BOX);
		  dotty_point->align(FL_ALIGN_RIGHT);
		  dotty_point->callback((Fl_Callback*)color_callback, this);
		  dotty_point->tooltip("dot color");
		}

		o->end();
	  }

	  /* ---- 3D Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " 3D View" R_SPACES);
		o->labelsize(16);
		o->selection_color(FL_DARK2);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 45, 280, 30, "3D View Settings");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ rend_aspect = new Fl_Float_Input(190, 90, 95, 25, "Pixel aspect ratio: ");

		  Fl_Box* o = new Fl_Box(300, 90, 150, 25, "(higher is wider, default is 0.83)");
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ rend_lock_grav = new Fl_Check_Button(50, 125, 360, 30, " Locked gravity -- cannot move up or down");
		}
		{ rend_high_detail = new Fl_Check_Button(50, 155, 360, 30, " High detail -- slower but looks better");
#ifndef NO_OPENGL
		  rend_high_detail->hide();
#endif
		}
		{ rend_far_clip = new Fl_Choice(195, 160, 100, 30, "Far clip distance: ");
		  rend_far_clip->add("32768|16384|8192|4096|2048|1024");
#ifdef NO_OPENGL
		  rend_far_clip->hide();
#endif
		}

		o->end();
	  }

	  /* ---- Nodes Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Nodes" R_SPACES);
		o->labelsize(16);
		o->selection_color(FL_DARK2);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 45, 280, 30, "Node Building");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ nod_on_save = new Fl_Check_Button(50, 80, 220, 30, " Always build nodes after saving   (recommended)");
		}
		{ nod_fast = new Fl_Check_Button(50, 110, 440, 30, " Fast mode   (the nodes may not be as good)");
		}
		{ nod_warn = new Fl_Check_Button(50, 140, 220, 30, " Warning messages in the logs");
		}

		{ Fl_Box* o = new Fl_Box(25, 205, 250, 30, "Advanced BSP Settings");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ nod_gl_nodes = new Fl_Check_Button(50, 245, 150, 30, " Build GL-Nodes");
		}
		{ nod_force_v5 = new Fl_Check_Button(50, 275, 250, 30, " Force V5 of GL-Nodes");
		}
		{ nod_force_zdoom = new Fl_Check_Button(50, 305, 250, 30, " Force ZDoom format of normal nodes");
		}
		// CURRENTLY HIDDEN -- NOT SURE IT IS WORTH HAVING
		{ nod_compress = new Fl_Check_Button(50, 335, 250, 30, " Force zlib compression");
		  nod_compress->hide();
		}
		{ nod_factor = new Fl_Choice(160, 345, 180, 30, "Seg split logic: ");
		  nod_factor->add("NORMAL|Minimize Splits|Balance BSP Tree");
		}
		o->end();
	  }

	  /* ---- Other Tab ---- */

	  { Fl_Group* o = new Fl_Group(0, 25, 585, 410, " Other" R_SPACES);
		o->labelsize(16);
		o->selection_color(FL_DARK2);
		o->hide();

		{ Fl_Box* o = new Fl_Box(25, 255, 280, 30, "Configuration Reset");
		  o->labelfont(FL_BOLD);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}
		{ reset_conf = new Fl_Button(100, 290, 200, 30, "Reset Config Settings");
		  reset_conf->callback((Fl_Callback*)reset_callback, this);
		}
		{ reset_keys = new Fl_Button(100, 330, 200, 30, "Reset Key Bindings");
		  reset_keys->callback((Fl_Callback*)reset_callback, this);
		}
		o->end();
	  }
	  tabs->end();
	}
	{ apply_but = new Fl_Button(PREF_WINDOW_W-150, PREF_WINDOW_H-50, 95, 35, "Apply");
	  apply_but->labelfont(FL_BOLD);
	  apply_but->callback(close_callback, this);
	}
	{ discard_but = new Fl_Button(PREF_WINDOW_W-290, PREF_WINDOW_H-50, 95, 35, "Discard");
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


void UI_Preferences::bind_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	int line = prefs->key_list->value();
	if (line < 1)
	{
		fl_beep();
		return;
	}

	prefs->EnsureKeyVisible(line);


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


void UI_Preferences::edit_key_callback(Fl_Button *w, void *data)
{
	UI_Preferences *prefs = (UI_Preferences *)data;

	bool is_add  = (w == prefs->key_add);
	bool is_copy = (w == prefs->key_copy);


	keycode_t     new_key  = 0;
	KeyContext new_context = KeyContext::general;
	SString  new_func = "Nothing";


	int bind_idx = -1;


	if (! is_add)
	{
		int line = prefs->key_list->value();
		if (line < 1)
		{
			fl_beep();
			return;
		}

		prefs->EnsureKeyVisible(line);

		bind_idx = line - 1;
		SYS_ASSERT(bind_idx >= 0);

		M_GetBindingInfo(bind_idx, &new_key, &new_context);
		new_func = M_StringForFunc(bind_idx);
	}


	bool start_grabbed = false;  //???  is_add || is_copy;

	UI_EditKey *dialog = new UI_EditKey(new_key, new_context, new_func.c_str());

	bool was_ok = dialog->Run(&new_key, &new_context, &new_func, start_grabbed);

	if (was_ok)
	{
		// assume we can set it, since the dialog validated it

		if (is_add || is_copy)
		{
			M_AddLocalBinding(bind_idx, new_key, new_context, new_func.c_str());

			if (is_copy)
				bind_idx++;
			else
				bind_idx = M_NumBindings() - 1;
		}
		else
		{
			M_SetLocalBinding(bind_idx, new_key, new_context, new_func);
		}
	}

	delete dialog;


	// for a new binding, make sure it is visible and selected

	if ((is_add || is_copy) && was_ok && bind_idx >= 0)
	{
		// expand the browser size with a dummy line
		// [ the ReloadKeys() below will grab the correct text ]
		prefs->key_list->add("");

		SYS_ASSERT(bind_idx >= 0);
		int line = 1 + bind_idx;

		prefs->key_list->select(line);
		prefs->EnsureKeyVisible(line);
	}

	prefs->ReloadKeys();
	prefs->redraw();

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

	bool is_keys = (w == prefs->reset_keys);

	int res = DLG_Confirm({ "Cancel", "&Reset" },
		"This will reset all %s to their default values, "
		"removing any changes you may have made."
		"\n\n"
		"If you continue, you can still back out by "
		"using the \"Discard\" button at the bottom of "
		"the Preferences window."
		"\n  ",
		is_keys ? "key bindings" : "preferences");

	if (res <= 0)
		return;

	if (is_keys)
	{
		M_CopyBindings(true /* from_defaults */);

		prefs->LoadKeys();
	}
	else
	{
		if (M_ParseConfigFile(global::install_dir / "defaults.cfg", prefs->options) != 0)
		{
			DLG_Notify("Installation problem: failed to find the \"defaults.cfg\" file!");
		}
		else
		{
			prefs->LoadValues();
		}
	}
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
		gLog.printf("Preferences: discarded changes\n");
		return;
	}

	SaveValues();
	if(global::config_file.empty())
		DLG_ShowError(false, "Configuration file not initialized.");
	else
		M_WriteConfigFile(global::config_file, options);

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

	switch (config::gui_scheme)
	{
		case 0: theme_FLTK->value(1); break;
		case 1: theme_GTK->value(1); break;
		case 2: theme_plastic->value(1); break;
	}

	switch (config::gui_color_set)
	{
		case 0: cols_default->value(1); break;
		case 1: cols_bright->value(1); break;
		case 2: cols_custom->value(1); break;
	}

	bg_colorbox->color(config::gui_custom_bg);
	ig_colorbox->color(config::gui_custom_ig);
	fg_colorbox->color(config::gui_custom_fg);

	/* General Tab */

	gen_autoload   ->value(config::auto_load_recent ? 1 : 0);
	gen_maximized  ->value(config::begin_maximized  ? 1 : 0);
	gen_swapsides  ->value(config::swap_sidedefs    ? 1 : 0);

	/* Edit Tab */

	edit_def_port->value(config::default_port.c_str());
	edit_def_mode->value(clamp(0, config::default_edit_mode, 3));
	edit_lineinfo->value(clamp(0, config::highlight_line_info, 5));

	edit_sectorsize->value(SString(config::new_sector_size).c_str());
	edit_samemode->value(config::same_mode_clears_selection ? 1 : 0);
	edit_add_del->value(config::sidedef_add_del_buttons ? 1 : 0);
	edit_full_1S->value(config::show_full_one_sided ? 1 : 0);
	edit_autoadjustX->value(config::leave_offsets_alone ? 0 : 1);

	brow_smalltex->value(config::browser_small_tex ? 1 : 0);
	brow_combo->value(config::browser_combine_tex ? 1 : 0);

	char ratio_buf[256];
	snprintf(ratio_buf, sizeof(ratio_buf), "%d:%d", config::grid_ratio_high,
			 config::grid_ratio_low);
	edit_userratio->value(ratio_buf);

	/* Grid Tab */

	if (config::grid_style < 0 || config::grid_style > 1)
		config::grid_style = 1;

	grid_cur_style->value(config::grid_style);
	grid_enabled->value(config::grid_default_mode ? 1 : 0);
	grid_snap->value(config::grid_default_snap ? 1 : 0);
	grid_size->value(GridSizeToChoice(config::grid_default_size));
	grid_hide_free ->value(config::grid_hide_in_free_mode ? 1 : 0);
	grid_flatrender->value(config::sector_render_default ? 1 : 0);
	grid_spriterend->value(config::thing_render_default ? 1 : 0);
	grid_indicator->value(config::grid_snap_indicator ? 1 : 0);

	gen_scrollbars ->value(config::map_scroll_bars ? 1 : 0);

	dotty_axis ->color(config::dotty_axis_col);
	dotty_major->color(config::dotty_major_col);
	dotty_minor->color(config::dotty_minor_col);
	dotty_point->color(config::dotty_point_col);

	normal_axis ->color(config::normal_axis_col);
	normal_main ->color(config::normal_main_col);
	normal_flat ->color(config::normal_flat_col);
	normal_small->color(config::normal_small_col);

	/* 3D Tab */

	config::render_pixel_aspect = clamp(25, config::render_pixel_aspect, 400);

	char aspect_buf[64];
	snprintf(aspect_buf, sizeof(aspect_buf), "%1.2f", config::render_pixel_aspect / 100.0);
	rend_aspect->value(aspect_buf);

	rend_high_detail->value(config::render_high_detail ? 1 : 0);
	rend_lock_grav->value(config::render_lock_gravity ? 1 : 0);

	if (config::render_far_clip > 24000)
		rend_far_clip->value(0);
	else if (config::render_far_clip > 12000)
		rend_far_clip->value(1);
	else if (config::render_far_clip > 6000)
		rend_far_clip->value(2);
	else if (config::render_far_clip > 3000)
		rend_far_clip->value(3);
	else if (config::render_far_clip > 1500)
		rend_far_clip->value(4);
	else
		rend_far_clip->value(5);

	/* Nodes Tab */

	nod_on_save->value(config::bsp_on_save ? 1 : 0);
	nod_fast->value(config::bsp_fast ? 1 : 0);
	nod_warn->value(config::bsp_warnings ? 1 : 0);

	if (config::bsp_split_factor < 7)
		nod_factor->value(2);	// Balanced BSP tree
	else if (config::bsp_split_factor > 15)
		nod_factor->value(1);	// Minimize Splits
	else
		nod_factor->value(0);	// NORMAL

	nod_gl_nodes->value(config::bsp_gl_nodes ? 1 : 0);
	nod_force_v5->value(config::bsp_force_v5 ? 1 : 0);
	nod_force_zdoom->value(config::bsp_force_zdoom ? 1 : 0);
	nod_compress->value(config::bsp_compressed ? 1 : 0);

	/* Other Tab */

}


void UI_Preferences::SaveValues()
{
	/* Theme stuff */

	if (theme_FLTK->value())
		config::gui_scheme = 0;
	else if (theme_GTK->value())
		config::gui_scheme = 1;
	else
		config::gui_scheme = 2;

	if (cols_default->value())
		config::gui_color_set = 0;
	else if (cols_bright->value())
		config::gui_color_set = 1;
	else
		config::gui_color_set = 2;

	config::gui_custom_bg = (rgb_color_t) bg_colorbox->color();
	config::gui_custom_ig = (rgb_color_t) ig_colorbox->color();
	config::gui_custom_fg = (rgb_color_t) fg_colorbox->color();

	// update the colors
	// FIXME: how to reset the "default" colors??
	if (config::gui_color_set == 1)
	{
		Fl::background(236, 232, 228);
		Fl::background2(255, 255, 255);
		Fl::foreground(0, 0, 0);

		// TODO: update for ALL windows
		gInstance->main_win->redraw();
	}
	else if (config::gui_color_set == 2)
	{
		Fl::background (RGB_RED(config::gui_custom_bg), RGB_GREEN(config::gui_custom_bg),
						RGB_BLUE(config::gui_custom_bg));
		Fl::background2(RGB_RED(config::gui_custom_ig), RGB_GREEN(config::gui_custom_ig),
						RGB_BLUE(config::gui_custom_ig));
		Fl::foreground (RGB_RED(config::gui_custom_fg), RGB_GREEN(config::gui_custom_fg),
						RGB_BLUE(config::gui_custom_fg));

		// TODO: update for ALL windows
		gInstance->main_win->redraw();
	}

	/* General Tab */

	config::auto_load_recent  = gen_autoload   ->value() ? true : false;
	config::begin_maximized   = gen_maximized  ->value() ? true : false;
	config::swap_sidedefs     = gen_swapsides  ->value() ? true : false;

	/* Edit Tab */

	config::default_port = edit_def_port->value();
	config::default_edit_mode = edit_def_mode->value();
	config::highlight_line_info = edit_lineinfo->value();

	config::new_sector_size = atoi(edit_sectorsize->value());
	config::new_sector_size = clamp(4, config::new_sector_size, 8192);

	config::same_mode_clears_selection = edit_samemode->value() ? true : false;
	config::sidedef_add_del_buttons = !!edit_add_del->value();
	config::show_full_one_sided = edit_full_1S->value() ? true : false;
	config::leave_offsets_alone = edit_autoadjustX->value() ? false : true;

	// changing this requires re-populating the browser
	bool new_small_tex = brow_smalltex->value() ? true : false;
	bool new_combo = brow_combo->value() ? true : false;

	if (new_small_tex != config::browser_small_tex || new_combo != config::browser_combine_tex)
	{
		config::browser_small_tex = new_small_tex;
		config::browser_combine_tex = new_combo;

		// TODO: update for ALL windows
		gInstance->main_win->browser->Populate();
	}

	// decode the user ratio
	config::grid_ratio_low = config::grid_ratio_high = -1;
	sscanf(edit_userratio->value(), "%d:%d", &config::grid_ratio_high, &config::grid_ratio_low);
	if (config::grid_ratio_high < 1) config::grid_ratio_high = 3;
	if (config::grid_ratio_low  < 1) config::grid_ratio_low  = 1;

	if (config::grid_ratio_low > config::grid_ratio_high)
		std::swap(config::grid_ratio_low, config::grid_ratio_high);

	// TODO: update for ALL windows
	gInstance->main_win->info_bar->UpdateRatio();

	/* Grid Tab */

	config::grid_style        = grid_cur_style->value();
	config::grid_default_mode = !!grid_enabled->value();
	config::grid_default_snap = grid_snap->value() ? true : false;
	config::grid_default_size = atoi(grid_size->mvalue()->text);
	config::grid_hide_in_free_mode = grid_hide_free ->value() ? true : false;
	config::grid_snap_indicator    = grid_indicator ->value() ? true : false;
	config::sector_render_default  = grid_flatrender->value() ? 1 : 0;
	config::thing_render_default   = grid_spriterend->value() ? 1 : 0;

	config::map_scroll_bars = gen_scrollbars ->value() ? true : false;

	config::dotty_axis_col  = (rgb_color_t) dotty_axis ->color();
	config::dotty_major_col = (rgb_color_t) dotty_major->color();
	config::dotty_minor_col = (rgb_color_t) dotty_minor->color();
	config::dotty_point_col = (rgb_color_t) dotty_point->color();

	config::normal_axis_col  = (rgb_color_t) normal_axis ->color();
	config::normal_main_col  = (rgb_color_t) normal_main ->color();
	config::normal_flat_col  = (rgb_color_t) normal_flat ->color();
	config::normal_small_col = (rgb_color_t) normal_small->color();

	/* Nodes Tab */

	config::bsp_on_save = nod_on_save->value() ? true : false;
	config::bsp_fast = nod_fast->value() ? true : false;
	config::bsp_warnings = nod_warn->value() ? true : false;

	if (nod_factor->value() == 1)			// Minimize Splits
		config::bsp_split_factor = 29;
	else if (nod_factor->value() == 2)		// Balanced BSP tree
		config::bsp_split_factor = 2;
	else
		config::bsp_split_factor = 11;

	config::bsp_gl_nodes = nod_gl_nodes->value() ? true : false;
	config::bsp_force_v5 = nod_force_v5->value() ? true : false;
	config::bsp_force_zdoom = nod_force_zdoom->value() ? true : false;
	config::bsp_compressed = nod_compress->value() ? true : false;

	/* Other Tab */

	config::render_pixel_aspect = (int)(100 * atof(rend_aspect->value()) + 0.2);
	config::render_pixel_aspect = clamp(25, config::render_pixel_aspect, 400);

	config::render_high_detail  = rend_high_detail->value() ? true : false;
	config::render_lock_gravity = rend_lock_grav->value() ? true : false;
	config::render_far_clip     = atoi(rend_far_clip->mvalue()->text);
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


void UI_Preferences::EnsureKeyVisible(int line)
{
	if (! key_list->displayed(line))
	{
		key_list->middleline(line);
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
		// escape key cancels
		if (event == FL_KEYDOWN && Fl::event_key() == FL_Escape)
		{
			ClearWaiting();
			return 1;
		}

		if (event == FL_KEYDOWN ||
			event == FL_PUSH    ||
			event == FL_MOUSEWHEEL)
		{
			keycode_t new_key = M_CookedKeyForEvent(event);

			if (new_key)
			{
				SetBinding(new_key);
				return 1;
			}
		}
	}

	return Fl_Double_Window::handle(event);
}


//------------------------------------------------------------------------


void Instance::CMD_Preferences()
{
	UI_Preferences * dialog = new UI_Preferences(options);

	dialog->Run();

	delete dialog;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
