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

	void Cmd_Quit();

private:
	void UpdatePosition();

	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
};

#endif  /* __EUREKA_UI_EDITOR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
