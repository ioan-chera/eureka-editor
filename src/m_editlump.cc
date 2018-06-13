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


#define BUTTON_COL  FL_YELLOW

// special (not real) lump names
#define EDLUMP_HEADER	"MapHeader"
#define EDLUMP_SCRIPTS	"MapScripts"


// Various lumps that definitely should not be edited as text.
// Note: this list is not meant to be exhaustive.
static const char * invalid_text_lumps[] =
{
	// editor stuff
	EUREKA_LUMP,

	// map lumps
	"THINGS", "VERTEXES", "LINEDEFS", "SECTORS",
	"SIDEDEFS", "SEGS", "SSECTORS", "NODES",
	"REJECT", "BLOCKMAP", "BEHAVIOR",
	"TEXTMAP", "ENDMAP", "ZNODES",

	// various binary lumps
	"PLAYPAL", "COLORMAP", "TINTTAB",
	"PNAMES", "TEXTURE1", "TEXTURE2",
	"GENMIDI", "DMXGUS", "DMXGUSC",
	"DEMO1", "DEMO2", "DEMO3", "DEMO4",
	"ENDOOM", "ENDBOOM",

	// various graphics
	"HELP", "HELP1", "CREDIT", "CONBACK",
	"VICTORY", "VICTORY2", "BOSSBACK",
	"TITLEPIC", "INTERPIC", "ENDPIC", "STBAR",
	"M_DOOM", "M_EPI1", "M_EPI2", "M_EPI3", "M_EPI4",

	// source port stuff
	"TRANMAP", "WATERMAP", "FOGMAP",
	"ANIMATED", "SWITCHES",
	"DIALOGUE", "SNDCURVE", "TITLEMAP",
	"AUTOPAGE", "STARTUP",

	// the end
	NULL
};


// TODO : ideally put these in an external file
static const char * common_text_lumps[] =
{
	// Hexen (plus source-port variants)
	"MAPINFO", "ZMAPINFO", "EMAPINFO",

	// Boom / MBF
	"DEHACKED", "OPTIONS",

	// Eternity
	"MUSINFO",

	// EDGE
	"RSCRIPT",

	// Legacy
	"S_SKIN", "",

	// ZDoom and derivatives
	"DECORATE", "LANGUAGE", "SNDINFO",
	"ANIMDEFS", "GLDEFS", "SBARINFO",
	"DECALDEFS", "FONTDEFS", "MODELDEF",

	// the end
	NULL
};


static bool ValidLumpToEdit(const char *p)
{
	size_t len = strlen(p);

	if (len == 0 || len > 8)
		return false;

	// check known binary lumps
	for (int i = 0 ; invalid_text_lumps[i] ; i++)
		if (y_stricmp(p, invalid_text_lumps[i]) == 0)
			return false;

	// markers like S_START and FF_END are not allowed
	if (len >= 4)
	{
		if (y_stricmp(p+1, "_START") == 0 || y_stricmp(p+1, "_END") == 0 ||
			y_stricmp(p+2, "_START") == 0 || y_stricmp(p+2, "_END") == 0)
		{
			return false;
		}
	}

	// check for bad characters [ p is *invalid* afterwards ]
	for ( ; *p ; p++)
		if (! (isalnum(*p) || *p == '_'))
			return false;

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
	void CheckInput();
	void PopulateButtons();

	static void     ok_callback(Fl_Widget *, void *);
	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
	static void  input_callback(Fl_Widget *, void *);
	static void header_callback(Fl_Widget *, void *);
	static void script_callback(Fl_Widget *, void *);
};


UI_ChooseTextLump::UI_ChooseTextLump() :
	UI_Escapable_Window(420, 385, "Choose Text Lump"),
	action(ACT_none)
{
	resizable(NULL);

	callback(close_callback, this);

	lump_name = new Fl_Input(170, 20, 120, 25, "Enter lump name: ");
	lump_name->labelfont(FL_HELVETICA_BOLD);
	lump_name->when(FL_WHEN_CHANGED);
	lump_name->callback(input_callback, this);

	Fl::focus(lump_name);

	{
		Fl_Box* o = new Fl_Box(15, 55, 270, 25, "Or select one of these lumps:");
		o->labelfont(FL_HELVETICA_BOLD);
		o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}

	// all the map buttons go into this group

	lump_buttons = new Fl_Group(0, 90, w(), 205, "");
	lump_buttons->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
	{
		int mhy = lump_buttons->y() + lump_buttons->h() - 25;

		Fl_Button *b1 = new Fl_Button(60, mhy, 120, 25, "Map Header");
		b1->callback(header_callback, this);
		b1->color(BUTTON_COL);

		Fl_Button *b2 = new Fl_Button(230, mhy, 120, 25, "Map Scripts");
		b2->callback(script_callback, this);

		if (Level_format == MAPF_Hexen)
			b2->color(BUTTON_COL);
		else
			b2->deactivate();

		PopulateButtons();
	}
	lump_buttons->end();

	{
		int bottom_y = 320;

		Fl_Group* o = new Fl_Group(0, bottom_y, 420, 65);
		o->box(FL_FLAT_BOX);
		o->color(WINDOW_BG, WINDOW_BG);

		ok_but = new Fl_Return_Button(260, bottom_y + 17, 100, 35, "OK");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);
		ok_but->deactivate();

		Fl_Button *cancel = new Fl_Button(75, bottom_y + 17, 100, 35, "Cancel");
		cancel->callback(close_callback, this);

		o->end();
	}

	end();
}


void UI_ChooseTextLump::PopulateButtons()
{
	int col = 0;
	int row = 0;
	int but_W = 100;

	for (int i = 0 ; common_text_lumps[i] ; i++)
	{
		const char *name = common_text_lumps[i];

		if (name[0] != 0)
		{
			int bx = 40 + col * (but_W + 20);
			int by = lump_buttons->y() + row * 25;

			if (row >= 3)
				by += 15;

			Fl_Button *but = new Fl_Button(bx, by, but_W, 20, name);

			but->color(BUTTON_COL);
			but->callback(button_callback, this);

			// NOTE: no need to add() it
		}

		col++;
		if (col >= 3)
		{
			col = 0;
			row += 1;
		}
	}
}


void UI_ChooseTextLump::close_callback(Fl_Widget *w, void *data)
{
	UI_ChooseTextLump *win = (UI_ChooseTextLump *)data;

	win->action = ACT_CLOSE;
}


void UI_ChooseTextLump::ok_callback(Fl_Widget *w, void *data)
{
	UI_ChooseTextLump * win = (UI_ChooseTextLump *)data;

	// santify check
	if (ValidLumpToEdit(win->lump_name->value()))
		win->action = ACT_ACCEPT;
	else
		fl_beep();
}


void UI_ChooseTextLump::input_callback(Fl_Widget *w, void *data)
{
	UI_ChooseTextLump *win = (UI_ChooseTextLump *)data;

	win->CheckInput();
}


void UI_ChooseTextLump::CheckInput()
{
	bool was_valid = ok_but->active();
	bool  is_valid = ValidLumpToEdit(lump_name->value());

	if (was_valid == is_valid)
		return;

	if (is_valid)
	{
		ok_but->activate();
		lump_name->textcolor(FL_FOREGROUND_COLOR);
	}
	else
	{
		ok_but->deactivate();
		lump_name->textcolor(FL_RED);
	}

	lump_name->redraw();
}


void UI_ChooseTextLump::header_callback(Fl_Widget *w, void *data)
{
	UI_ChooseTextLump *win = (UI_ChooseTextLump *)data;

	win->lump_name->value(EDLUMP_HEADER);
	win->action = ACT_ACCEPT;
}


void UI_ChooseTextLump::script_callback(Fl_Widget *w, void *data)
{
	UI_ChooseTextLump *win = (UI_ChooseTextLump *)data;

	win->lump_name->value(EDLUMP_SCRIPTS);
	win->action = ACT_ACCEPT;
}


void UI_ChooseTextLump::button_callback(Fl_Widget *w, void *data)
{
	Fl_Button *but = (Fl_Button *)w;
	UI_ChooseTextLump *win = (UI_ChooseTextLump *)data;

	// the button's label is the lump name
	win->lump_name->value(but->label());
	win->action = ACT_ACCEPT;
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

	// return a copy of the name, and uppercase it
	return StringUpper(name);
}


//------------------------------------------------------------------------

void CMD_EditLump()
{
	const char *lump_name = EXEC_Param[0];

	if (Exec_HasFlag("/header"))
	{
		lump_name = EDLUMP_HEADER;
	}
	else if (Exec_HasFlag("/scripts"))
	{
		lump_name = EDLUMP_SCRIPTS;
	}

	if (lump_name[0] == 0 || lump_name[0] == '/')
	{
		// ask for the lump name
		UI_ChooseTextLump *dialog = new UI_ChooseTextLump();

		lump_name = dialog->Run();

		delete dialog;

		if (lump_name == NULL)
			return;

		// check if user typed name of current level
		if (y_stricmp(lump_name, Level_name) == 0)
			lump_name = EDLUMP_HEADER;
	}

	// NOTE: there are two special cases for lump_name:
	//       (1) EDLUMP_HEADER  --> edit the HeaderData buffer
	//       (2) EDLUMP_SCRIPTS --> edit the ScriptsData buffer

	// can only create SCRIPTS lump in a Hexen map
	if (strcmp(lump_name, EDLUMP_SCRIPTS) == 0 && Level_format != MAPF_Hexen)
	{
		// FIXME
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

	// FIXME : do window-title stuff here

	// if lump exists, load the contents
	if (strcmp(lump_name, EDLUMP_HEADER) == 0)
	{
		editor->LoadMemory(HeaderData);
	}
	else if (strcmp(lump_name, EDLUMP_SCRIPTS) == 0)
	{
		editor->LoadMemory(ScriptsData);
	}
	else
	{
		if (! editor->LoadLump(wad, lump_name))
		{
			// something went wrong
			delete editor;
			return;
		}
	}

	// run the text editor
	for (;;)
	{
		int res = editor->Run();

		if (res != UI_TextEditor::RUN_Save)
			break;

		SYS_ASSERT(wad == edit_wad);

		if (strcmp(lump_name, EDLUMP_HEADER) == 0)
		{
			editor->SaveMemory(HeaderData);
			MadeChanges = 1;
		}
		else if (strcmp(lump_name, EDLUMP_SCRIPTS) == 0)
		{
			editor->SaveMemory(ScriptsData);
			MadeChanges = 1;
		}
		else
		{
			editor->SaveLump(wad, lump_name);
		}
	}

	delete editor;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
