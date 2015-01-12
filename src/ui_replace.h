//------------------------------------------------------------------------
//  Find and Replace
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2015 Andrew Apted
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

#ifndef __EUREKA_UI_REPLACE_H__
#define __EUREKA_UI_REPLACE_H__


class UI_FindAndReplace : public Fl_Group
{
private:
	// main thing we are finding / replacing
	Fl_Choice *what;

	// --- FIND AREA ---

	Fl_Input  *find_match;
	Fl_Button *find_choose;
	Fl_Output *find_desc;
	Fl_Button *find_but;
	Fl_Button *select_all_but;

	// --- REPLACE AREA ---

	Fl_Toggle_Button *rep_toggle;
	Fl_Group *rep_group;

	Fl_Output *rep_value;
	Fl_Button *rep_choose;
	Fl_Output *rep_desc;
	Fl_Button *apply_but;
	Fl_Button *replace_all_but;

	// --- FILTER AREA ---

	Fl_Toggle_Button *filter_toggle;
	Fl_Group *filter_group;

	// FIXME: stuff for Things, LineDefs

	// sector filters
	Fl_Round_Button *only_floors;
	Fl_Round_Button *only_ceilings;

	Fl_Input *tag_match;

	// current object
	Objid cur_obj;

public:
	UI_FindAndReplace(int X, int Y, int W, int H);
	virtual ~UI_FindAndReplace();

	void Open();

	void BrowsedItem(char kind, int number, const char *name, int e_state);

private:
	void Clear();

	void rawShowReplace(int value);
	void rawShowFilter(int value);

	static void     what_kind_callback(Fl_Widget *w, void *data);
	static void    rep_toggle_callback(Fl_Widget *w, void *data);
	static void filter_toggle_callback(Fl_Widget *w, void *data);

	static void  find_but_callback(Fl_Widget *w, void *data);
	static void apply_but_callback(Fl_Widget *w, void *data);

	static void  select_all_callback(Fl_Widget *w, void *data);
	static void replace_all_callback(Fl_Widget *w, void *data);
};


#endif  /* __EUREKA_UI_REPLACE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
