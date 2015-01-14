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
class UI_TripleCheckButton;


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

	// for numeric types, this contains the number(s) to match
	number_group_c *find_numbers;

	// --- REPLACE AREA ---

	Fl_Input  *rep_value;
	Fl_Button *rep_choose;
	Fl_Output *rep_desc;
	Fl_Button *apply_but;
	Fl_Button *replace_all_but;

	// TODO : replace_number

	// --- FILTER AREA ---

	Fl_Toggle_Button *filter_toggle;
	Fl_Group *filter_group;

	// common stuff
	Fl_Input * tag_input;
	number_group_c * tag_numbers;

	// thing stuff
	UI_TripleCheckButton *o_easy;
	UI_TripleCheckButton *o_medium;
	UI_TripleCheckButton *o_hard;

	UI_TripleCheckButton *o_sp;
	UI_TripleCheckButton *o_coop;
	UI_TripleCheckButton *o_dm;

	int options_mask;
	int options_value;

	// sector filters
	Fl_Check_Button *o_floors;
	Fl_Check_Button *o_ceilings;

	// linedef filters
	Fl_Check_Button *o_lowers;
	Fl_Check_Button *o_uppers;
	Fl_Check_Button *o_rail;

	Fl_Check_Button *o_one_sided;
	Fl_Check_Button *o_two_sided;

	// current (found) object
	Objid cur_obj;

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
	void UpdateWhatFilters();
	void ComputeFlagMask();

	void InsertName  (Fl_Input *inp, char append, const char *name);
	void InsertNumber(Fl_Input *inp, char append, int number);

	bool NeedSeparator(Fl_Input *inp) const;

	void rawShowFilter(int value);

	bool MatchesObject(int idx);
	void ApplyReplace (int idx);

	void DoReplace();
	void DoAll(bool replace);

	bool CheckInput(Fl_Input *w, Fl_Output *desc, number_group_c *num_grp = NULL);

	bool CheckNumberInput(Fl_Input *w, number_group_c *num_grp);

	// specialized functions for each search modality

	bool Match_Thing(int idx);
	bool Match_LineDef(int idx);
	bool Match_LineType(int idx);
	bool Match_Sector(int idx);
	bool Match_SectorType(int idx);

	// return 'true' for pass, 'false' to reject
	bool Filter_Tag(int tag);

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
	static void     tag_input_callback(Fl_Widget *w, void *data);
};


#endif  /* __EUREKA_UI_REPLACE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
