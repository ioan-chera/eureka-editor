//------------------------------------------------------------------------
//  FIND AND REPLACE
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

class number_group_c;


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

	Fl_Input  *rep_value;
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

	// for numeric types, this contains the number(s) to match
	number_group_c *nums_to_match;

public:
	UI_FindAndReplace(int X, int Y, int W, int H);
	virtual ~UI_FindAndReplace();

	void Open();

	char GetKind();	 // same as browser : 'O' 'T' 'F' 'L' 'S'

	// called by "Find" button in here, or CTRL-G shortcut
	bool FindNext();

	void BrowsedItem(char kind, int number, const char *name, int e_state);

private:
	void Clear();

	bool WhatFromEditMode();

	void UpdateWhatColor();

	void rawShowFilter(int value);

	bool MatchesObject(int idx);
	void ApplyReplace (int idx);

	void DoReplace();
	void DoAll(bool replace);

	bool CheckInput(Fl_Input *w, Fl_Output *desc, number_group_c *num_grp = NULL);

	// specialized functions for each search modality

	bool Match_Thing(int idx);
	bool Match_LineDef(int idx);
	bool Match_LineType(int idx);
	bool Match_Sector(int idx);
	bool Match_SectorType(int idx);

	bool Filter_Thing(int idx);
	bool Filter_LineDef(int idx);
	bool Filter_Sector(int idx);

	void Replace_Thing(int idx);
	void Replace_LineDef(int idx);
	void Replace_LineType(int idx);
	void Replace_Sector(int idx);
	void Replace_SectorType(int idx);

private:
	static void what_kind_callback(Fl_Widget *w, void *data);

	static void  find_match_callback(Fl_Widget *w, void *data);
	static void find_choose_callback(Fl_Widget *w, void *data);
	static void    find_but_callback(Fl_Widget *w, void *data);
	static void  select_all_callback(Fl_Widget *w, void *data);

	static void   rep_value_callback(Fl_Widget *w, void *data);
	static void  rep_choose_callback(Fl_Widget *w, void *data);
	static void   apply_but_callback(Fl_Widget *w, void *data);
	static void replace_all_callback(Fl_Widget *w, void *data);

	static void filter_toggle_callback(Fl_Widget *w, void *data);
};


#endif  /* __EUREKA_UI_REPLACE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
