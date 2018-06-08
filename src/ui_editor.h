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

#ifndef __EUREKA_UI_EDITOR_H__
#define __EUREKA_UI_EDITOR_H__

class Wad_file;
class UI_TedStatusBar;
class UI_TedWrapper;

class UI_TextEditor : public Fl_Double_Window
{
private:
	bool want_close;
	bool read_only;
	bool has_changes;

private:
	Fl_Menu_Bar *menu_bar;

	UI_TedStatusBar *status;

	UI_TedWrapper  *ted;
	Fl_Text_Buffer *tbuf;

public:
	UI_TextEditor();
	virtual ~UI_TextEditor();

	void SetReadOnly()
	{
		read_only = true;
	}

	bool LoadLump(Wad_file *wad, const char *lump_name);
	bool SaveLump(Wad_file *wad, const char *lump_name);

	int Run();

private:
	void UpdateStatus();

	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);

	static void text_modified_callback(int, int nInserted, int nDeleted, int, const char*, void *);

public:
	// File menu
	static void menu_do_save(Fl_Widget *w, void *data);
	static void menu_do_include(Fl_Widget *w, void *data);
	static void menu_do_export(Fl_Widget *w, void *data);
	static void menu_do_quit(Fl_Widget *w, void *data);

	// Edit menu
	static void menu_do_undo(Fl_Widget *w, void *data);
	static void menu_do_cut(Fl_Widget *w, void *data);
	static void menu_do_copy(Fl_Widget *w, void *data);
	static void menu_do_paste(Fl_Widget *w, void *data);
	static void menu_do_delete(Fl_Widget *w, void *data);
	static void menu_do_select_all(Fl_Widget *w, void *data);
	static void menu_do_unselect_all(Fl_Widget *w, void *data);

	// Search menu
	static void menu_do_find(Fl_Widget *w, void *data);
	static void menu_do_replace(Fl_Widget *w, void *data);
	static void menu_do_goto_top(Fl_Widget *w, void *data);
	static void menu_do_goto_bottom(Fl_Widget *w, void *data);
};

#endif  /* __EUREKA_UI_EDITOR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
