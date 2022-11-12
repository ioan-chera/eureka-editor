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

#include "Errors.h"

#include "Instance.h"
#include "main.h"
#include "m_streams.h"
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

void UI_TextEditor::menu_do_save(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	if (win->read_only)
	{
		if (DLG_Confirm({ "Cancel", "&Export" },
						"The current wad is READ-ONLY.  "
						"Do you want to export the text into a new file?") == 1)
		{
			menu_do_export(w, data);
		}

		return;
	}

	win->want_save = true;
}

void UI_TextEditor::menu_do_insert(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	win->InsertFile();
}

void UI_TextEditor::menu_do_export(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	win->ExportToFile();
}


void UI_TextEditor::menu_do_close(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	// this asks for confirmation if changes have been made
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

void UI_TextEditor::menu_do_select_all(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	Fl_Text_Editor::kf_select_all(0, win->ted);
}

void UI_TextEditor::menu_do_unselect_all(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	win->tbuf->unselect();
}

void UI_TextEditor::menu_do_find(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	if (! win->AskFindString())
		return;

	win->FindNext(+2);
}

void UI_TextEditor::menu_do_find_next(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	if (win->find_string.empty())
	{
		if (! win->AskFindString())
			return;
	}

	win->FindNext(+1);
}

void UI_TextEditor::menu_do_find_prev(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	if (win->find_string.empty())
	{
		if (! win->AskFindString())
			return;
	}

	win->FindNext(-1);
}

void UI_TextEditor::menu_do_goto_top(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	win->tbuf->unselect();

	win->ted->insert_position(0);
	win->ted->show_insert_position();
}

void UI_TextEditor::menu_do_goto_bottom(Fl_Widget *w, void *data)
{
	UI_TextEditor *win = (UI_TextEditor *)data;
	SYS_ASSERT(win);

	win->tbuf->unselect();

	int len = win->tbuf->length();

	win->ted->insert_position(len);
	win->ted->show_insert_position();
}


#undef FCAL
#define FCAL  (Fl_Callback *)

static const Fl_Menu_Item ted_menu_items[] =
{
	{ "&File", 0, 0, 0, FL_SUBMENU },
		{ "&Insert File...",      FL_COMMAND + 'i', FCAL UI_TextEditor::menu_do_insert },
		{ "&Export to File...  ", FL_COMMAND + 'e', FCAL UI_TextEditor::menu_do_export },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Save Lump",   FL_COMMAND + 's', FCAL UI_TextEditor::menu_do_save },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Close",       FL_COMMAND + 'q', FCAL UI_TextEditor::menu_do_close },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },
		{ "&Undo",    FL_COMMAND + 'z',  FCAL UI_TextEditor::menu_do_undo },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Cu&t",     FL_COMMAND + 'x',  FCAL UI_TextEditor::menu_do_cut },
		{ "&Copy",    FL_COMMAND + 'c',  FCAL UI_TextEditor::menu_do_copy },
		{ "&Paste",   FL_COMMAND + 'v',  FCAL UI_TextEditor::menu_do_paste },
		{ "&Delete",  0,                 FCAL UI_TextEditor::menu_do_delete },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Select &All",    FL_COMMAND + 'a', FCAL UI_TextEditor::menu_do_select_all },
		{ "Unselect All  ", FL_COMMAND + 'u', FCAL UI_TextEditor::menu_do_unselect_all },
		{ 0 },

	{ "&Search", 0, 0, 0, FL_SUBMENU },
		{ "&Find",      FL_COMMAND + 'f',  FCAL UI_TextEditor::menu_do_find },
		{ "Find &Next", FL_COMMAND + 'g',  FCAL UI_TextEditor::menu_do_find_next },
		{ "Find &Prev", FL_COMMAND + 'p',  FCAL UI_TextEditor::menu_do_find_prev },
	//	{ "&Replace",   FL_COMMAND + 'r',  FCAL menu_PLACEHOLDER },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Go to &Top",      FL_COMMAND + 't',  FCAL UI_TextEditor::menu_do_goto_top },
		{ "Go to &Bottom  ", FL_COMMAND + 'b',  FCAL UI_TextEditor::menu_do_goto_bottom },
		{ 0 },

#if 0
	{ "&View", 0, 0, 0, FL_SUBMENU },
		// Todo: flesh these out   [ will need config-file vars too ]
		{ "Colors",   0,          FCAL menu_PLACEHOLDER },
		{ "Font",     0,          FCAL menu_PLACEHOLDER },
		{ "Line Numbers", 0,      FCAL menu_PLACEHOLDER },
		{ 0 },
#endif

	{ 0 }
};


//------------------------------------------------------------------------

UI_TextEditor::UI_TextEditor(Instance &inst) :
	Fl_Double_Window(600, 400, ""),
	want_close(false), want_save(false),
	is_new(true), read_only(false), has_changes(false), inst(inst)
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
	// it is safe to call these a second/third/etc time.
	set_modal();
	show();

	// reset this here, as LoadLump() sets it to true, and we
	// want to reset it after a RUN_Save as well.
	has_changes = false;

	for (;;)
	{
		if (want_save)
		{
			want_save = false;
			return RUN_Save;
		}

		if (want_close)
			return RUN_Quit;

		Fl::wait(0.2);

		UpdateStatus();
	}

	/* NOT REACHED */
	return RUN_Error;
}


void UI_TextEditor::close_callback(Fl_Widget *w, void *data)
{
	UI_TextEditor * win = (UI_TextEditor *)data;

	if (win->has_changes)
	{
		if (DLG_Confirm({ "Cancel", "&Discard" },
						"You have unsaved changes to this lump.  "
						"Are you sure you want to discard them?") <= 0)
		{
			return;
		}
	}

	win->want_close = true;
}


void UI_TextEditor::text_modified_callback(int, int nInserted, int nDeleted, int, const char*, void *data)
{
	UI_TextEditor * win = (UI_TextEditor *)data;

	if (nInserted + nDeleted > 0)
		win->has_changes = true;
}


bool UI_TextEditor::LoadLump(Wad_file *wad, const SString &lump_name)
{
	Lump_c * lump = wad->FindLump(lump_name);

	// if the lump does not exist, we will create it
	if (! lump)
	{
		if (read_only)
		{
			DLG_Notify("The %s lump does not exist, and it cannot be "
						"created since the current wad is READ-ONLY.",
					   lump_name.c_str());
			return false;
		}

		return true;
	}

	gLog.printf("Reading '%s' text lump\n", lump_name.c_str());

	lump->Seek();

	SString line;

	while (lump->GetLine(line))
	{
		line.trimTrailingSpaces();

		tbuf->append(line.c_str());
		tbuf->append("\n");
	}

	is_new = false;

	return true;
}

void UI_TextEditor::LoadMemory(std::vector<byte> &buf)
{
	// this code is slow, but simple

	char charbuf[2];
	charbuf[1] = 0;

	unsigned int len = static_cast<unsigned>(buf.size());

	for (unsigned int k = 0 ; k < len ; k++)
	{
		// ignore NULs and CRs
		if (buf[k] == 0 || buf[k] == '\r')
			continue;

		charbuf[0] = buf[k];
		tbuf->append(charbuf);
	}

	is_new = false;
}


void UI_TextEditor::SaveLump(Wad_file *wad, const SString &lump_name)
{
	gLog.printf("Writing '%s' text lump\n", lump_name.c_str());

	int oldie = wad->FindLumpNum(lump_name);
	if (oldie >= 0)
		wad->RemoveLumps(oldie, 1);

	Lump_c *lump = wad->AddLump(lump_name);

	int len = tbuf->length();

	for (int i = 0 ; i < len ; i++)
	{
		// this is not optimal (one byte at a time), but is adequate
		byte ch = tbuf->byte_at(i);

		// ignore NULs and CRs
		if (ch == 0 || ch == '\r')
			continue;

		lump->Write(&ch, 1);
	}

	wad->writeToDisk();
}

void UI_TextEditor::SaveMemory(std::vector<byte> &buf)
{
	buf.clear();

	int len = tbuf->length();

	for (int i = 0 ; i < len ; i++)
	{
		byte ch = tbuf->byte_at(i);

		// ignore NULs and CRs
		if (ch == 0 || ch == '\r')
			continue;

		buf.push_back(ch);
	}
}


void UI_TextEditor::SetTitle(const SString &lump_name)
{
	static char title_buf[256];

	const char *suffix = "";

	if (is_new)
		suffix = " [NEW]";
	else if (read_only)
		suffix = " [Read-Only]";

	snprintf(title_buf, sizeof(title_buf), "Editing '%s' lump%s", lump_name.c_str(), suffix);

	copy_label(title_buf);
}


void UI_TextEditor::UpdateStatus()
{
	int row, column;

	if (ted->GetLineAndColumn(&row, &column))
		status->SetPosition(row, column);

	status->SetModified(has_changes);
}


void UI_TextEditor::InsertFile()
{
	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to insert");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.directory(inst.Main_FileOpFolder().c_str());

	switch (chooser.show())
	{
		case -1:
			DLG_Notify("Unable to open the file:\n\n%s", chooser.errmsg());
			return;

		case 1:
			// cancelled
			return;

		default:
			// Ok
			break;
	}

	// if a selection is active, delete that text
	tbuf->remove_selection();

	SString line;

	fs::path filename = fs::u8path(chooser.filename());

	// TODO: for WIN32, ideally examine the file and determine
	//       whether the charset is UTF-8 or CP-1252, based on
	//       quantity of invalid UTF-8 sequences.

	// open file in binary mode (we handle CR/LF ourselves)
	LineFile file(filename);

	if (!file.isOpen())
	{
		line = GetErrorMessage(errno);
		DLG_Notify("Unable to open text file:\n\n%s", line.c_str());
		return;
	}

	gLog.printf("Reading text from file: %s\n", filename.u8string().c_str());

	int pos = ted->insert_position();

	while (file.readLine(line))
	{
		tbuf->insert(pos, line.c_str());
		pos += static_cast<int>(line.length());

		tbuf->insert(pos, "\n");
		pos += 1;
	}
}


void UI_TextEditor::ExportToFile()
{
	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to export to");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);
	chooser.directory(inst.Main_FileOpFolder().c_str());

	switch (chooser.show())
	{
		case -1:
			DLG_Notify("Unable to export the file:\n\n%s", chooser.errmsg());
			return;

		case 1:
			// cancelled
			return;

		default:
			// Ok
			break;
	}

	const char *filename = chooser.filename();

	// open file in binary mode (we handle CR/LF ourselves)
	FILE * fp = fopen(filename, "wb");

	if (fp == NULL)
	{
		DLG_Notify("Unable to create output file:\n\n%s", GetErrorMessage(errno).c_str());
		return;
	}

	gLog.printf("Writing text to file: %s\n", filename);

	int len = tbuf->length();

	for (int p = 0 ; p < len ; p++)
	{
		byte ch = (byte) tbuf->byte_at(p);

		// skip raw CR characters
		if (ch == '\r')
			continue;

#ifdef WIN32
		// under Windows, convert LF --> CR/LF
		if (ch == '\n')
		{
			fputc('\r', fp);
		}
#endif

		if (fputc(ch, fp) == EOF)
		{
			DLG_Notify("A write error occurred:\n\n%s", GetErrorMessage(errno).c_str());

			fclose(fp);
			return;
		}
	}

	fclose(fp);

	has_changes = false;
}


// returns false if the user cancelled
bool UI_TextEditor::AskFindString()
{
	// we don't pre-seed with the last search string, because FLTK does
	// not select the text and it is a pain to delete it first.
	const char *str = fl_input("Find what:", find_string.c_str());

	if (str == NULL || str[0] == 0)
		return false;

	SetFindString(str);
	return true;
}


void UI_TextEditor::SetFindString(const char *str)
{
	if (str)
		find_string = str;
	else
		find_string.clear();
}


bool UI_TextEditor::FindNext(int dir)
{
	SYS_ASSERT(!find_string.empty());

	int pos = ted->insert_position();
//	int len = tbuf->length();

	int new_pos;
	int found;

	if (dir >= 2 && !tbuf->selected())
		// a normal "Find" can match at exactly the starting spot
		found = tbuf->search_forward(pos, find_string.c_str(), &new_pos);
	else if (dir > 0)
		found = tbuf->search_forward(pos+1, find_string.c_str(), &new_pos);
	else
		found = tbuf->search_backward(pos-1, find_string.c_str(), &new_pos);

	if (! found && dir > 0 && pos > 0)
	{
		fl_beep();

		if (DLG_Confirm({ "Cancel", "&Search" },
						"No more results.\n\n"
						"Search again from the top?") <= 0)
		{
			return false;
		}

		pos = 0;

		found = tbuf->search_forward(pos, find_string.c_str(), &new_pos);
	}

	if (! found)
	{
		fl_beep();
		DLG_Notify("No %sresults.", (dir > 0 && pos == 0) ? "" : "more ");
		return false;
	}

	// found it, so select the text
	int end_pos = static_cast<int>(new_pos + find_string.length());

	tbuf->select(new_pos, end_pos);

	ted->insert_position(new_pos);
	ted->show_insert_position();

	return true;
}


bool UI_TextEditor::ContainsUnicode() const
{
	int len = tbuf->length();

	for (int i = 0 ; i < len ; i++)
		if ((byte)tbuf->byte_at(i) & 0x80)
			return true;

	return false;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
