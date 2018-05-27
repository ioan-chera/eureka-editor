//------------------------------------------------------------------------
//  TEXT LUMP EDITOR
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
#include "m_editlump.h"
#include "w_wad.h"

#include "ui_window.h"


class UI_TextEditor : public Fl_Double_Window
{
private:
	bool want_close;
	bool read_only;

public:
	UI_TextEditor(const char *title);
	virtual ~UI_TextEditor();

	void SetReadOnly()
	{
		read_only = true;
	}

	bool LoadLump(Wad_file *wad, const char *lump_name);
	bool SaveLump(Wad_file *wad, const char *lump_name);

	int Run();

private:
	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
};


UI_TextEditor::UI_TextEditor(const char *title) :
	Fl_Double_Window(580, 400, title),
	want_close(false),
	read_only(false)
{
	// TODO

	callback((Fl_Callback *) close_callback, this);
}


UI_TextEditor::~UI_TextEditor()
{ }


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


bool UI_TextEditor::LoadLump(Wad_file *wad, const char *lump_name)
{
	Lump_c * lump = wad->FindLump(lump_name);

	// does not matter if it doesn't exist, we will create it
	if (! lump)
		return true;

	LogPrintf("Reading '%s' text lump\n", lump_name);

	if (! lump->Seek())
	{
		// FIXME: DLG_Notify
		return false;
	}

	// FIXME: LoadLump

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


//------------------------------------------------------------------------

static bool ValidLumpToEdit(const char *name)
{
	// FIXME : ValidLumpToEdit

	return true;
}


void CMD_EditLump()
{
	const char *lump_name = EXEC_Param[0];

	if (lump_name[0] == 0 || lump_name[0] == '/')
	{
		// ask for the lump name

		// FIXME: custom dialog, which validates the name (among other things)

		lump_name = fl_input("Lump name?");

		if (lump_name == NULL)
			return;
	}

	if (! ValidLumpToEdit(lump_name))
	{
		Beep("Invalid lump: '%s'", lump_name);
		return;
	}

	Wad_file *wad = edit_wad ? edit_wad : game_wad;

	// create the editor window
	UI_TextEditor *editor = new UI_TextEditor(lump_name);

	if (! edit_wad)
		editor->SetReadOnly();

	// if lump exists, load the contents
	if (! editor->LoadLump(wad, lump_name))
	{
		// something went wrong
		delete editor;
		return;
	}

	// run the editor
	int res = editor->Run();

	// save the contents?
	if (res == 123 && wad == edit_wad)
	{
		editor->SaveLump(wad, lump_name);
	}

	delete editor;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
