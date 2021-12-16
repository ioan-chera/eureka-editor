//------------------------------------------------------------------------
//  DEFAULT PROPERTIES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
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

#ifndef __EUREKA_UI_DEFAULT_H__
#define __EUREKA_UI_DEFAULT_H__

#include "e_cutpaste.h"
#include "ui_panelinput.h"

class Fl_Int_Input;
class Fl_Output;
class UI_DynIntInput;

class UI_DefaultProps : public Fl_Group
{
private:
	UI_Pic      *w_pic;
	UI_DynInput *w_tex;

	UI_DynIntInput *ceil_h;
	UI_DynIntInput *light;
	UI_DynIntInput *floor_h;

	Fl_Button *ce_down, *ce_up;
	Fl_Button *fl_down, *fl_up;

	UI_Pic      *c_pic;
	UI_DynInput *c_tex;

	UI_Pic      *f_pic;
	UI_DynInput *f_tex;

	UI_DynInput  *thing;
	Fl_Output    *th_desc;
	UI_Pic		 *th_sprite;

	Instance &inst;
	PanelFieldFixUp mFixUp;

public:
	UI_DefaultProps(Instance &inst, int X, int Y, int W, int H);
	virtual ~UI_DefaultProps();

	// see ui_window.h for description of these two methods
	bool ClipboardOp(EditCommand op);
	void BrowsedItem(BrowserMode kind, int number, const char *name, int e_state);

	void UnselectPics();

	void LoadValues();

private:
	void SetIntVal(Fl_Int_Input *w, int value);
	void UpdateThingDesc();
	void SetThing(int number);

	void CB_Copy  (int sel_pics);
	void CB_Paste (int sel_pics);
	void CB_Delete(int sel_pics);

	SString Normalize_and_Dup(UI_DynInput *w);

private:
	static void   hide_callback(Fl_Widget *w, void *data);
	static void    tex_callback(Fl_Widget *w, void *data);
	static void dyntex_callback(Fl_Widget *w, void *data);
	static void   flat_callback(Fl_Widget *w, void *data);
	static void button_callback(Fl_Widget *w, void *data);
	static void height_callback(Fl_Widget *w, void *data);

	static void    thing_callback(Fl_Widget *w, void *data);
	static void dynthing_callback(Fl_Widget *w, void *data);
};


bool Props_ParseUser(Instance &inst, const std::vector<SString> &tokens);
void Props_LoadValues(const Instance &inst);

#endif  /* __EUREKA_UI_DEFAULT_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
