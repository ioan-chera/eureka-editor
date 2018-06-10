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


static bool ValidLumpToEdit(const char *name)
{
	// FIXME : ValidLumpToEdit

	return true;
}


//------------------------------------------------------------------------

class UI_ChooseTextLump : public UI_Escapable_Window
{
private:
	Fl_Input *lump_name;
	Fl_Group *lump_buttons;

	Fl_Return_Button *ok_but;

	enum
	{
		ACT_none = 0,
		ACT_CLOSE,
		ACT_ACCEPT
	};

	int action;

public:
	UI_ChooseTextLump();

	virtual ~UI_ChooseTextLump()
	{ }

	// returns lump name on success, NULL on cancel
	const char * Run();

private:
	static void     ok_callback(Fl_Widget *, void *);
	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
	static void  input_callback(Fl_Widget *, void *);
	static void    new_callback(Fl_Widget *, void *);
};


UI_ChooseTextLump::UI_ChooseTextLump() :
	UI_Escapable_Window(420, 385, "Choose Text Lump"),
	action(ACT_none)
{
	resizable(NULL);

	callback(close_callback, this);

	lump_name = new Fl_Input(120, 35, 120, 25, "Lump name: ");
	lump_name->labelfont(FL_HELVETICA_BOLD);
	lump_name->when(FL_WHEN_CHANGED);
//	lump_name->callback(input_callback, this);

	Fl::focus(lump_name);

	end();
}


void UI_ChooseTextLump::close_callback(Fl_Widget *w, void *data)
{
	UI_ChooseTextLump *win = (UI_ChooseTextLump *)data;

	win->action = ACT_CLOSE;
}


const char * UI_ChooseTextLump::Run()
{
	set_modal();
	show();

	while (action == ACT_none)
	{
		Fl::wait(0.2);
	}

	if (action == ACT_CLOSE)
		return NULL;

	const char *name = lump_name->value();

	if (name[0] == 0)
		return NULL;

	return StringUpper(name);
}


//------------------------------------------------------------------------

void CMD_EditLump()
{
	const char *lump_name = EXEC_Param[0];

	if (lump_name[0] == 0 || lump_name[0] == '/')
	{
		// ask for the lump name
		UI_ChooseTextLump *dialog = new UI_ChooseTextLump();

		lump_name = dialog->Run();

		delete dialog;

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
	UI_TextEditor *editor = new UI_TextEditor();

	if (! edit_wad || edit_wad->IsReadOnly())
		editor->SetReadOnly();

	// if lump exists, load the contents
	if (! editor->LoadLump(wad, lump_name))
	{
		// something went wrong
		delete editor;
		return;
	}

	// run the editor
	for (;;)
	{
		int res = editor->Run();

		if (res != UI_TextEditor::RUN_Save)
			break;

		SYS_ASSERT(wad == edit_wad);

		editor->SaveLump(wad, lump_name);
	}

	delete editor;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
