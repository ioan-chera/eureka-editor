//------------------------------------------------------------------------
//  Main Window
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2012 Andrew Apted
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
#include "ui_dialog.h"
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
#include "ui_radius.h"


#define WINDOW_BG  fl_gray_ramp(3)

#define BUILD_BG   fl_gray_ramp(9)

#define MIN_BROWSER_W  260


class Wad_file;


class UI_MainWin : public Fl_Double_Window
{
public:
	// main child widgets

#ifdef MACOSX
	Fl_Sys_Menu_Bar *menu_bar;
#else
	Fl_Menu_Bar *menu_bar;
#endif

	int panel_W;

	UI_Tile * tile;

	UI_Canvas  *canvas;
	UI_Browser *browser;

	UI_InfoBar *info_bar;

	UI_ThingBox  *thing_box;
	UI_LineBox   *line_box;
	UI_SectorBox *sec_box;
	UI_VertexBox *vert_box;
	UI_RadiusBox *rad_box;


private:
	Fl_Cursor cursor_shape;

public:
	UI_MainWin();
	virtual ~UI_MainWin();

public:
	void SetTitle(Wad_file *wad);

	// mode can be 't', 'l', 's', 'v' or 'r'.   FIXME: ENUMERATE
	void NewEditMode(char mode);

	// this is a wrapper around the FLTK cursor() method which
	// prevents the possibly expensive call when the shape hasn't
	// changed.
	void SetCursor(Fl_Cursor shape);

	// show or hide the Browser panel.
	// kind is 0 to hide, '/' to toggle, 'T' for textures, 'F' flats,
	//         'O' for thing types, 'L' line types, 'S' sector types.
	void ShowBrowser(char kind);

	void UpdateTotals();

	int GetPanelObjNum() const;

	void InvalidatePanelObj();
	void UpdatePanelObj();

	void UnselectPics();
};


extern UI_MainWin * main_win;


const char *Int_TmpStr(int value);  // FIXME: put elsewhere


#endif  /* __EUREKA_UI_WINDOW_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
