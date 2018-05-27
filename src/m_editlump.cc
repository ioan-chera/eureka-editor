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

public:
	UI_TextEditor(const char *title);
	virtual ~UI_TextEditor();

	int Run();

private:
	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
};


UI_TextEditor::UI_TextEditor(const char *title) :
	Fl_Double_Window(580, 400, title),
	want_close(false)
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


//------------------------------------------------------------------------

void CMD_EditLump()
{
	const char *lump_name = EXEC_Param[0];

	if (lump_name[0] == 0 || lump_name[0] == '/')
	{
		// ask for the lump name
		// FIXME: custom dialog

		lump_name = fl_input("Lump name?");

		if (lump_name == NULL)
			return;
	}


	// FIXME

	UI_TextEditor *editor = new UI_TextEditor(lump_name);

	editor->Run();

	delete editor;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
