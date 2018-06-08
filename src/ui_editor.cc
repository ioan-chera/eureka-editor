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
	Fl_Box *row_col;
	Fl_Box *mod_box;

	int  cur_row;
	int  cur_column;
	bool cur_modified;

public:
	UI_TedStatusBar(int X, int Y, int W, int H, const char *label = NULL) :
		Fl_Group(X, Y, W, H, label),
		cur_row(1), cur_column(1), cur_modified(false)
	{
		box(FL_UP_BOX);

		row_col = new Fl_Box(FL_FLAT_BOX, X, Y+1, W*2/3, H-2, "");
		row_col->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

		mod_box = new Fl_Box(FL_FLAT_BOX, X+W*2/3, Y+1, W/3, H-2, "");
		mod_box->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

		end();

		Update();
	}

	virtual ~UI_TedStatusBar()
	{ }

public:
	void SetPosition(int row, int column)
	{
		if (row != cur_row || column != cur_column)
		{
			cur_row = row;
			cur_column = column;

			Update();
		}
	}

	void SetModified(bool modified)
	{
		if (modified != cur_modified)
		{
			cur_modified = modified;
			Update();
		}
	}

private:
	void Update()
	{
		static char buffer[256];

		snprintf(buffer, sizeof(buffer), " Line: %-6d Col: %d", cur_row, cur_column);

		row_col->copy_label(buffer);

		if (cur_modified)
			mod_box->label("MODIFIED ");
		else
			mod_box->label("");

		// ensure background gets redrawn
		redraw();
	}
};


//------------------------------------------------------------------------

// andrewj: this class only exists because a very useful method of
//          Fl_Text_Display is not public.  FFS.
class UI_TedWrapper : public Fl_Text_Editor
{
public:
	UI_TedWrapper(int X, int Y, int W, int H, const char *l=0) :
		Fl_Text_Editor(X, Y, W, H, l)
	{ }

	virtual ~UI_TedWrapper()
	{ }

	bool GetLineAndColumn(int *line, int *col)
	{
		if (position_to_linecol(insert_position(), line, col) == 0)
			return false;

		*col += 1;

		return true;
	}
};


//------------------------------------------------------------------------

static void menu_PLACEHOLDER(Fl_Widget *w, void *data)
{ }

void UI_TextEditor::menu_do_save(Fl_Widget *w, void *data)
{
	// FIXME
}

// TODO menu_do_include
// TODO menu_do_export

void UI_TextEditor::menu_do_quit(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	win->close_callback(win, win);
}

void UI_TextEditor::menu_do_undo(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	Fl_Text_Editor::kf_undo(0, win->ted);
}

void UI_TextEditor::menu_do_cut(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	Fl_Text_Editor::kf_cut(0, win->ted);
}

void UI_TextEditor::menu_do_copy(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	Fl_Text_Editor::kf_copy(0, win->ted);
}

void UI_TextEditor::menu_do_paste(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	Fl_Text_Editor::kf_paste(0, win->ted);
}

void UI_TextEditor::menu_do_delete(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	win->tbuf->remove_selection();
}

// TODO menu_do_find
// TODO menu_do_find_next
// TODO menu_do_replace

void UI_TextEditor::menu_do_goto_top(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	win->ted->insert_position(0);
	win->ted->show_insert_position();
}

void UI_TextEditor::menu_do_goto_bottom(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	int len = win->tbuf->length();

	win->ted->insert_position(len);
	win->ted->show_insert_position();
}


#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item ted_menu_items[] =
{
	{ "&File", 0, 0, 0, FL_SUBMENU },
		{ "&Insert File...",       FL_COMMAND + 'i', FCAL menu_PLACEHOLDER },
		{ "&Export to File...  ",   FL_COMMAND + 'e', FCAL menu_PLACEHOLDER },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Save Lump",   FL_COMMAND + 's', FCAL UI_TextEditor::menu_do_save },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Close",       FL_COMMAND + 'q', FCAL UI_TextEditor::menu_do_quit },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },
		{ "&Undo",    FL_COMMAND + 'z',  FCAL UI_TextEditor::menu_do_undo },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Cu&t",     FL_COMMAND + 'x',  FCAL UI_TextEditor::menu_do_cut },
		{ "&Copy",    FL_COMMAND + 'c',  FCAL UI_TextEditor::menu_do_copy },
		{ "&Paste",   FL_COMMAND + 'v',  FCAL UI_TextEditor::menu_do_paste },
		{ "&Delete",  0,                 FCAL UI_TextEditor::menu_do_delete },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Select &All",    FL_COMMAND + 'a',  FCAL menu_PLACEHOLDER },
		{ "Unselect All  ", FL_COMMAND + 'u',  FCAL menu_PLACEHOLDER },
		{ 0 },

	{ "&Search", 0, 0, 0, FL_SUBMENU },
		{ "&Find",     FL_COMMAND + 'f',  FCAL menu_PLACEHOLDER },
		{ "Find Next", FL_COMMAND + 'g',  FCAL menu_PLACEHOLDER },
		{ "&Replace",  FL_COMMAND + 'r',  FCAL menu_PLACEHOLDER },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Go to &Top",      FL_COMMAND + 't',  FCAL UI_TextEditor::menu_do_goto_top },
		{ "Go to &Bottom  ", FL_COMMAND + 'b',  FCAL UI_TextEditor::menu_do_goto_bottom },
		{ 0 },

	{ "&View", 0, 0, 0, FL_SUBMENU },
		// TODO : flesh these out   [ will need config-file vars too ]
		{ "Colors",   0,          FCAL menu_PLACEHOLDER },
		{ "Font",     0,          FCAL menu_PLACEHOLDER },
		{ "Line Numbers", 0,      FCAL menu_PLACEHOLDER },
		{ 0 },

	{ 0 }
};


//------------------------------------------------------------------------

UI_TextEditor::UI_TextEditor() :
	Fl_Double_Window(600, 400, ""),
	want_close(false),
	read_only(false),
	has_changes(false)
{
	size_range(520, 200);

	callback((Fl_Callback *) close_callback, this);

	color(WINDOW_BG, WINDOW_BG);

	int MW = w() / 2;

	menu_bar = new Fl_Menu_Bar(0, 0, MW, 28);
	menu_bar->copy(ted_menu_items, this /* userdata for every menu item */);

	status = new UI_TedStatusBar(MW, 0, w() - MW, 28);

	ted = new UI_TedWrapper(0, 28, w(), h() - 28);

	ted->color(FL_BLACK, FL_BLUE);
	ted->textfont(FL_COURIER);
	ted->textsize(18);
	ted->textcolor(fl_rgb_color(192,192,192));

	ted->cursor_color(FL_RED);
	ted->cursor_style(Fl_Text_Display::HEAVY_CURSOR);

	tbuf = new Fl_Text_Buffer();
	tbuf->add_modify_callback(text_modified_callback, this);

	ted->buffer(tbuf);

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

		UpdateStatus();
	}

	return 0;
}


void UI_TextEditor::close_callback(Fl_Widget *w, void *data)
{
	UI_TextEditor * that = (UI_TextEditor *)data;

	// FIXME : if modified...

	that->want_close = true;
}


void UI_TextEditor::text_modified_callback(int, int nInserted, int nDeleted, int, const char*, void *data)
{
	UI_TextEditor * that = (UI_TextEditor *)data;

	if (nInserted + nDeleted > 0)
		that->has_changes = true;
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


void UI_TextEditor::UpdateStatus()
{
	int row, column;

	if (ted->GetLineAndColumn(&row, &column))
		status->SetPosition(row, column);

	status->SetModified(has_changes);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
