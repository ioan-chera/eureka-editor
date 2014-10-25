//------------------------------------------------------------------------
//  Main Window
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2013 Andrew Apted
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

#ifndef __EUREKA_UI_WINDOW_H__
#define __EUREKA_UI_WINDOW_H__

#include "ui_menu.h"
#include "ui_canvas.h"
#include "ui_infobar.h"
#include "ui_hyper.h"
#include "ui_nombre.h"
#include "ui_pic.h"
#include "ui_scroll.h"
#include "ui_tile.h"
#include "ui_browser.h"

#include "ui_thing.h"
#include "ui_sector.h"
#include "ui_sidedef.h"
#include "ui_linedef.h"
#include "ui_vertex.h"


#define WINDOW_BG  FL_DARK3

#define MIN_BROWSER_W  260


class Wad_file;
class UI_Render3D;


class UI_MainWin : public Fl_Double_Window
{
public:
	// main child widgets

	Fl_Sys_Menu_Bar *menu_bar;

	int panel_W;

	UI_Tile * tile;

	UI_CanvasScroll *scroll;

	UI_Canvas   *canvas;
	UI_Render3D *render;

	UI_Browser *browser;

	UI_InfoBar *info_bar;

	UI_ThingBox  *thing_box;
	UI_LineBox   *line_box;
	UI_SectorBox *sec_box;
	UI_VertexBox *vert_box;

private:
	Fl_Cursor cursor_shape;

	// remember window size/position after going fullscreen.
	// the 'last_w' and 'last_h' fields are zero when not fullscreen
	int last_x, last_y, last_w, last_h;

public:
	UI_MainWin();
	virtual ~UI_MainWin();

public:
	void SetTitle(const char *wad_name, const char *map_name, bool read_only);

	// add or remove the '*' (etc) in the title
	void UpdateTitle(char want_prefix = 0);

	void Maximize();

	// mode can be 't', 'l', 's', 'v' or 'r'.   FIXME: ENUMERATE
	void NewEditMode(char mode);

	// this is a wrapper around the FLTK cursor() method which
	// prevents the possibly expensive call when the shape hasn't
	// changed.
	void SetCursor(Fl_Cursor shape);

	// show or hide the Browser panel.
	// kind is NUL or '-' to hide, '/' to toggle, 'T' for textures, 'F' flats,
	//         'O' for thing types, 'L' line types, 'S' sector types.
	void ShowBrowser(char kind);

	void UpdateTotals();

	int GetPanelObjNum() const;

	void InvalidatePanelObj();
	void UpdatePanelObj();

	void UnselectPics();

	void Delay(int steps);  // each step is 1/10th second

	// this is used by the browser when user clicks on an entry.
	// kind == 'T' for textures (etc... as above)
	void BrowsedItem(char kind, int number, const char *name, int e_state);

	// this is called when game_info changes (in Main_LoadResources)
	// and can enable / disable stuff in the panels.
	void UpdateGameInfo();
};


extern UI_MainWin * main_win;


//------------------------------------------------------------------------


class UI_Escapable_Window : public Fl_Double_Window
{
public:
	UI_Escapable_Window(int W, int H, const char *L = NULL);
	virtual ~UI_Escapable_Window();

public:
	// FLTK event handling method
	int handle(int event);
};


//------------------------------------------------------------------------


class UI_LogViewer : public UI_Escapable_Window
{
private:
	Fl_Multi_Browser * browser;

	Fl_Button * copy_but;

public:
	UI_LogViewer();
	virtual ~UI_LogViewer();

	void Add(const char *line);

	void Deselect();

	// ensure the very last line is visible
	void JumpEnd();

private:
	int CountSelectedLines() const;

	char * GetSelectedText() const;

	static void     ok_callback(Fl_Widget *, void *);
	static void  clear_callback(Fl_Widget *, void *);
	static void   save_callback(Fl_Widget *, void *);
	static void select_callback(Fl_Widget *, void *);
	static void   copy_callback(Fl_Widget *, void *);
};


extern UI_LogViewer * log_viewer;

void LogViewer_Open();


#endif  /* __EUREKA_UI_WINDOW_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
