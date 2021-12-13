//------------------------------------------------------------------------
//  FIND AND REPLACE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2015-2016 Andrew Apted
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

#include "e_cutpaste.h"

class Fl_Choice;
class Fl_Output;
class LineDef;
class number_group_c;
class UI_TripleCheckButton;


class UI_FindAndReplace : public Fl_Group
{
private:
	enum What
	{
		What_things,
		What_lineTextures,
		What_sectorFlats,
		What_linesByType,
		What_sectorsByType,
		NUM_What
	};

	enum
	{
		NUM_FILTER_WIDGETS = 7
	};

	struct WhatDef
	{
		const char *label;
		Fl_Color color;
		std::vector<Fl_Widget *> filterWidgets;
	};

	// object kind we are finding / replacing
	Fl_Choice *what;

	// current (found) object
	Objid cur_obj;


	// --- FIND AREA ---

	Fl_Input  *find_match;
	UI_Pic    *find_pic;
	Fl_Output *find_desc;
	Fl_Button *find_but;
	Fl_Button *select_all_but;

	// for numeric types, this contains the number(s) to match
	number_group_c *find_numbers;


	// --- REPLACE AREA ---

	Fl_Input  *rep_value;
	UI_Pic    *rep_pic;
	Fl_Output *rep_desc;
	Fl_Button *apply_but;
	Fl_Button *replace_all_but;


	// --- FILTER AREA ---

	Fl_Toggle_Button *filter_toggle;
	Fl_Group *filter_group;

	// common stuff
	Fl_Input * tag_input;
	number_group_c * tag_numbers;

	Fl_Check_Button *restrict_to_sel;

	selection_c *previous_sel;

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
	Fl_Check_Button *o_skies;

	// linedef filters
	Fl_Check_Button *o_lowers;
	Fl_Check_Button *o_uppers;
	Fl_Check_Button *o_rails;

	Fl_Check_Button *o_one_sided;
	Fl_Check_Button *o_two_sided;

	WhatDef whatDefs[NUM_What];
	bool initializedWhatWidgets = false;

public:
	UI_FindAndReplace(Instance &inst, int X, int Y, int W, int H);
	virtual ~UI_FindAndReplace();

	void Open();

	char GetKind();	 // same as browser : 'O' 'T' 'F' 'L' 'S'

	// called by "Find" button in here, or CTRL-G shortcut
	bool FindNext();

	bool ClipboardOp(EditCommand op);
	void BrowsedItem(char kind, int number, const char *name, int e_state);

private:
	void ensureInitWhatFilters();

	void Clear();
	void ResetFilters();

	bool WhatFromEditMode();

	void UpdateWhatColor();
	void UpdateWhatFilters();
	void ComputeFlagMask();

	void UnselectPics();

	void InsertName  (Fl_Input *inp, char append, const SString &name);
	void InsertNumber(Fl_Input *inp, char append, int number);

	bool NeedSeparator(Fl_Input *inp) const;

	void rawShowFilter(int value);

	bool MatchesObject(int idx);
	void ApplyReplace (EditOperation &op, int idx, int new_tex);

	void DoReplace();
	void DoAll(bool replace);

	// validate input and update desc and the picture
	bool CheckInput(Fl_Input *w, Fl_Output *desc, UI_Pic *pic, number_group_c *num_grp = NULL);

	// this used for Tag number
	bool CheckNumberInput(Fl_Input *w, number_group_c *num_grp);

	bool Pattern_Match(const SString &tex, const SString &pattern, bool is_rail = false);

	// specialized functions for each search modality

	bool Match_Thing(int idx);
	bool Match_LineDef(int idx);
	bool Match_LineType(int idx);
	bool Match_Sector(int idx);
	bool Match_SectorType(int idx);

	// return 'true' for pass, 'false' to reject
	bool Filter_Tag(int tag);
	bool Filter_Sides(const LineDef *L);
	bool Filter_PrevSel(int idx);

	void Replace_Thing(EditOperation &op, int idx);
	void Replace_LineDef(EditOperation &op, int idx, int new_tex);
	void Replace_LineType(EditOperation &op, int idx);
	void Replace_Sector(EditOperation &op, int idx, int new_tex);
	void Replace_SectorType(EditOperation &op, int idx);

	// clipboard stuff
	void CB_Copy(bool is_replace);
	void CB_Paste(bool is_replace);
	void CB_Delete(bool is_replace);

private:
	static void      hide_callback(Fl_Widget *w, void *data);
	static void what_kind_callback(Fl_Widget *w, void *data);
	static void    choose_callback(UI_Pic    *w, void *data);

	static void  find_match_callback(Fl_Widget *w, void *data);
	static void    find_but_callback(Fl_Widget *w, void *data);
	static void  select_all_callback(Fl_Widget *w, void *data);

	static void   rep_value_callback(Fl_Widget *w, void *data);
	static void   apply_but_callback(Fl_Widget *w, void *data);
	static void replace_all_callback(Fl_Widget *w, void *data);

	static void filter_toggle_callback(Fl_Widget *w, void *data);
	static void     tag_input_callback(Fl_Widget *w, void *data);

	Instance &inst;
};


#endif  /* __EUREKA_UI_REPLACE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
