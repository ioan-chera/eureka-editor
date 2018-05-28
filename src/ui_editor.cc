//------------------------------------------------------------------------
//  TEXT EDITOR WINDOW
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2018 Andrew Apted
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


class UI_TedStatusBar : public Fl_Group
{
private:
	Fl_Output *column;
	Fl_Output *row;

public:
	UI_TedStatusBar(int X, int Y, int W, int H, const char *label = NULL) :
		Fl_Group(X, Y, W, H, label)
	{
		box(FL_UP_BOX);

		// TODO

		end();
	}

	virtual ~UI_TedStatusBar()
	{ }

public:
	void SetColumn(int col)
	{
		// TODO
	}

	void SetRow(int col)
	{
		// TODO
	}
};


//------------------------------------------------------------------------

static void ted_do_save(Fl_Widget *w, void *data)
{
	// FIXME
}

static void ted_do_quit(Fl_Widget *w, void *data)
{
	// FIXME
}

static void ted_do_undo(Fl_Widget *w, void *data)
{
	// FIXME
}

static void ted_do_redo(Fl_Widget *w, void *data)
{
	// FIXME
}

static void ted_do_find(Fl_Widget *w, void *data)
{
	// FIXME
}


#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item ted_menu_items[] =
{
	{ "&File", 0, 0, 0, FL_SUBMENU },
		{ "&Insert File...",       FL_COMMAND + 'i', FCAL ted_do_save },
		{ "&Export to File...  ",   FL_COMMAND + 'e', FCAL ted_do_save },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Save Lump",   FL_COMMAND + 's', FCAL ted_do_save },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Close",       FL_COMMAND + 'q', FCAL ted_do_quit },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },
		{ "&Undo",    FL_COMMAND + 'z',  FCAL ted_do_undo },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Cu&t",     FL_COMMAND + 'x',  FCAL ted_do_redo },
		{ "&Copy",    FL_COMMAND + 'c',  FCAL ted_do_redo },
		{ "&Paste",   FL_COMMAND + 'v',  FCAL ted_do_redo },
		{ "&Delete",  0,                 FCAL ted_do_redo },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Select &All",  FL_COMMAND + 'a',  FCAL ted_do_redo },
		{ "Unselect All  ", FL_COMMAND + 'u',  FCAL ted_do_redo },
		{ 0 },

	{ "&Search", 0, 0, 0, FL_SUBMENU },
		{ "&Find",     FL_COMMAND + 'f',  FCAL ted_do_find },
		{ "Find Next", FL_COMMAND + 'g',  FCAL ted_do_find },
		{ "&Replace",  FL_COMMAND + 'r',  FCAL ted_do_find },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Next Wotsit  ",  FL_COMMAND + 'n',  FCAL ted_do_find },
		{ "Go to &Top",     FL_COMMAND + 't',  FCAL ted_do_find },
		{ "Go to &Bottom  ",  FL_COMMAND + 'b',  FCAL ted_do_find },
		{ 0 },

	{ "&View", 0, 0, 0, FL_SUBMENU },
		// TODO : flesh these out   [ will need config-file vars too ]
		{ "Colors",   0,                 FCAL ted_do_find },
		{ "Font",     0,                 FCAL ted_do_find },
		{ "Line Numbers", 0,                 FCAL ted_do_find },
		{ 0 },

	{ 0 }
};


//------------------------------------------------------------------------

UI_TextEditor::UI_TextEditor() :
	Fl_Double_Window(580, 400, ""),
	want_close(false),
	read_only(false)
{
	callback((Fl_Callback *) close_callback, this);

	color(WINDOW_BG, WINDOW_BG);

	int MW = w() / 2;

	menu_bar = new Fl_Menu_Bar(0, 0, MW, 28);
	menu_bar->copy(ted_menu_items, this /* userdata for every menu item */);

	status = new UI_TedStatusBar(MW, 0, w() - MW, 28);

	ted = new Fl_Text_Editor(0, 28, w(), h() - 28);

	ted->color(FL_BLACK, FL_BLACK);
	ted->textfont(FL_COURIER);
	ted->textsize(18);
	ted->textcolor(fl_rgb_color(192,192,192));

	ted->cursor_color(FL_RED);
	ted->cursor_style(Fl_Text_Display::HEAVY_CURSOR);

	tbuf = new Fl_Text_Buffer();
	ted->buffer(tbuf);

	// TODO : set color and font

	resizable(ted);

	end();
}


UI_TextEditor::~UI_TextEditor()
{
	ted->buffer(NULL);

	delete tbuf; tbuf = NULL;
}


int UI_TextEditor::Run()
{
	set_modal();

	show();

	while (! want_close)
	{
		Fl::wait(0.2);
	}

	return 0;
}


void UI_TextEditor::close_callback(Fl_Widget *w, void *data)
{
	UI_TextEditor * that = (UI_TextEditor *)data;

	that->want_close = true;
}


// this sets the window's title too
bool UI_TextEditor::LoadLump(Wad_file *wad, const char *lump_name)
{
	static char title_buf[FL_PATH_MAX];

	Lump_c * lump = wad->FindLump(lump_name);

	// if the lump does not exist, we will create it
	if (! lump)
	{
		if (read_only)
		{
			// FIXME DLG_Notify
//!!!!			return false;
		}

		sprintf(title_buf, "%s lump (new)", lump_name);
		copy_label(title_buf);

		return true;
	}

	LogPrintf("Reading '%s' text lump\n", lump_name);

	if (! lump->Seek())
	{
		// FIXME: DLG_Notify
		return false;
	}

	// FIXME: LoadLump

	if (read_only)
		sprintf(title_buf, "%s lump (read-only)", lump_name);
	else
		sprintf(title_buf, "%s lump", lump_name);

	copy_label(title_buf);

	return true;
}


bool UI_TextEditor::SaveLump(Wad_file *wad, const char *lump_name)
{
	LogPrintf("Writing '%s' text lump\n", lump_name);

	wad->BeginWrite();

	int oldie = wad->FindLumpNum(lump_name);
	if (oldie >= 0)
		wad->RemoveLumps(oldie, 1);

	Lump_c *lump = wad->AddLump(lump_name);

	// FIXME: SaveLump

	return true;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
