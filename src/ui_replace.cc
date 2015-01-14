//------------------------------------------------------------------------
/  FIND AND REPLACE
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

#include "main.h"
#include "ui_window.h"

#include "e_path.h"  // GoToObject
#include "m_game.h"


class number_group_c
{
	// This represents a small group of numbers and number ranges,
	// which the user can type into the Match input box.
#define NUMBER_GROUP_MAX	40

private:
	int size;

	int ranges[NUMBER_GROUP_MAX][2];

	bool everything;

public:
	number_group_c() : size(0), everything(false)
	{ }

	~number_group_c()
	{ }

	void clear()
	{
		size = 0;
		everything = false;
	}

	bool is_single() const
	{
		return (size == 1) && (ranges[0][0] == ranges[0][1]);
	}

	bool is_everything() const
	{
		return everything;
	}

	int grab_first() const
	{
		if (size == 0)
			return 0;

		return ranges[0][0];
	}

	void insert(int low, int high)
	{
		// overflow is silently ignored
		if (size >= NUMBER_GROUP_MAX)
			return;

		// TODO : try to merge with existing range

		ranges[size][0] = low;
		ranges[size][1] = high;

		size++;
	}

	bool get(int num) const
	{
		for (int i = 0 ; i < size ; i++)
		{
			if (ranges[i][0] <= num && num <= ranges[i][1])
				return true;
		}

		return false;
	}

	//
	// Parse a string like "1,3-5,9" and add the numbers (or ranges)
	// to this group.  Returns false for malformed strings.
	// An empty string is considered invalid.
	//
	bool ParseString(const char *str)
	{
		char *endptr;

		for (;;)
		{
			// support an asterix to mean everything
			// (useful when using filters)
			if (*str == '*')
			{
				insert(INT_MIN, INT_MAX);
				everything = true;
				return true;
			}

			int low  = (int)strtol(str, &endptr, 0 /* allow hex */);
			int high = low;

			if (endptr == str)
				return false;

			str = endptr;

			while (isspace(*str))
				str++;

			// check for range
			if (*str == '-' || (str[0] == '.' && str[1] == '.'))
			{
				str += (*str == '-') ? 1 : 2;

				while (isspace(*str))
					str++;

				high = (int)strtol(str, &endptr, 0 /* allow hex */);

				if (endptr == str)
					return false;

				str = endptr;

				// valid range?
				if (high < low)
					return false;

				while (isspace(*str))
					str++;
			}

			insert(low, high);

			if (*str == 0)
				return true;  // OK //

			// valid separator?
			if (*str == ',' || *str == '/' || *str == '|')
				str++;
			else
				return false;
		}
	}
};


//------------------------------------------------------------------------

UI_FindAndReplace::UI_FindAndReplace(int X, int Y, int W, int H) :
	Fl_Group(X, Y, W, H, NULL),
	cur_obj(OBJ_THINGS, -1),
	nums_to_match(new number_group_c)
{
	box(FL_FLAT_BOX);

	color(WINDOW_BG, WINDOW_BG);

	
	/* ---- FIND AREA ---- */

	Fl_Group *grp1 = new Fl_Group(X, Y, W, 210);
	grp1->box(FL_UP_BOX);
	{
		Fl_Box *title = new Fl_Box(X + 50, Y + 10, W - 60, 30, "Find and Replace");
		title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		title->labelsize(18+KF*4);


		what = new Fl_Choice(X+60, Y+46, W - 120, 33);
		what->textsize(17);
		what->add("Things|Line Textures|Sector Flats|Lines by Type|Sectors by Type");
		what->value(0);
		what->callback(what_kind_callback, this);

		UpdateWhatColor();


		find_match = new Fl_Input(X+70, Y+95, 125, 25, "Match: ");
		find_match->when(FL_WHEN_CHANGED);
		find_match->callback(find_match_callback, this);

		find_choose = new Fl_Button(X+210, Y+95, 70, 25, "Choose");

		find_desc = new Fl_Output(X+70, Y+125, 210, 25, "Desc: ");

		find_but = new Fl_Button(X+50, Y+165, 80, 30, "Find");
		find_but->labelfont(FL_HELVETICA_BOLD);
		find_but->callback(find_but_callback, this);

		select_all_but = new Fl_Button(X+165, Y+165, 93, 30, "Select All");
		select_all_but->callback(select_all_callback, this);
	}
	grp1->end();


	/* ---- REPLACE AREA ---- */

	Fl_Group *grp2 = new Fl_Group(X, Y + 214, W, 132);
	grp2->box(FL_UP_BOX);
	{
		rep_value = new Fl_Input(X+80, Y+230, 115, 25, "New val: ");
		rep_value->when(FL_WHEN_CHANGED);
		rep_value->callback(rep_value_callback, this);

		rep_choose = new Fl_Button(X+210, Y+230, 70, 25, "Choose");

		rep_desc = new Fl_Output(X+80, Y+260, 200, 25, "Desc: ");

		apply_but = new Fl_Button(X+45, Y+300, 90, 30, "Replace");
		apply_but->labelfont(FL_HELVETICA_BOLD);
		apply_but->callback(apply_but_callback, this);

		replace_all_but = new Fl_Button(X+160, Y+300, 105, 30, "Replace All");
		replace_all_but->callback(replace_all_callback, this);
	}
	grp2->end();


	/* ---- FILTER AREA ---- */

	Fl_Group *grp3 = new Fl_Group(X, Y + 350, W, H - 350);
	grp3->box(FL_UP_BOX);
	{
		filter_toggle = new Fl_Toggle_Button(X+15, Y+356, 30, 30, "v");
		filter_toggle->labelsize(16);
		filter_toggle->color(FL_DARK3, FL_DARK3);
		filter_toggle->callback(filter_toggle_callback, this);
		filter_toggle->clear_visible_focus();

		Fl_Box *f_text = new Fl_Box(X+60, Y+356, 200, 30, "Search Filters");
		f_text->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		f_text->labelsize(16);

		filter_group = new Fl_Group(X, Y+391, W, H-415);
		{
			only_floors = new Fl_Round_Button(X+35, Y+391, 145, 25, " Only Floors");
			only_floors->down_box(FL_ROUND_DOWN_BOX);

			only_ceilings = new Fl_Round_Button(X+35, Y+411, 165, 30, " Only Ceilings");
			only_ceilings->down_box(FL_ROUND_DOWN_BOX);

			tag_match = new Fl_Input(X+105, Y+442, 130, 24, "Tag Match:");
		}
		filter_group->end();
		filter_group->hide();
	}
	grp3->end();


	grp3->resizable(NULL);
	resizable(grp3);

	end();
}


UI_FindAndReplace::~UI_FindAndReplace()
{ }


void UI_FindAndReplace::UpdateWhatColor()
{
	switch (what->value())
	{
		case 0: /* Things      */ what->color(FL_MAGENTA); break;
		case 1: /* Line Tex    */ what->color(fl_rgb_color(0,128,255)); break;
		case 2: /* Sector Flat */ what->color(FL_YELLOW); break;
		case 3: /* Line Type   */ what->color(FL_GREEN); break;
		case 4: /* Sector Type */ what->color(fl_rgb_color(255,144,0)); break;
	}
}


void UI_FindAndReplace::rawShowFilter(int value)
{
	if (value)
	{
		filter_toggle->label("^");
		filter_group->show();
	}
	else
	{
		filter_toggle->label("v");
		filter_group->hide();
	}
}


void UI_FindAndReplace::filter_toggle_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;
		
	Fl_Toggle_Button *toggle = (Fl_Toggle_Button *)w;

	box->rawShowFilter(toggle->value());
}


void UI_FindAndReplace::what_kind_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->Clear();

	bool want_descs = true;

	switch (box->what->value())
	{
		case 0: box->cur_obj.type = OBJ_THINGS; break;
		case 1: box->cur_obj.type = OBJ_LINEDEFS; want_descs = false; break;
		case 2: box->cur_obj.type = OBJ_SECTORS;  want_descs = false; break;
		case 3: box->cur_obj.type = OBJ_LINEDEFS; break;
		case 4: box->cur_obj.type = OBJ_SECTORS; break;

		default: break;
	}

	box->UpdateWhatColor();

	if (want_descs)
	{
		box->find_desc->activate();
		box-> rep_desc->activate();
	}
	else
	{
		box->find_desc->deactivate();
		box-> rep_desc->deactivate();
	}
}


void UI_FindAndReplace::Open()
{
	show();

	WhatFromEditMode();

	// this will do a Clear() for us
	what->do_callback();

	Fl::focus(find_match);
}


void UI_FindAndReplace::Clear()
{
	cur_obj.clear();

	find_match->value("");
	find_desc->value("");
	find_but->label("Find");

	rep_value->value("");
	rep_desc->value("");

	find_but->deactivate();
	select_all_but->deactivate();

	apply_but->deactivate();
	replace_all_but->deactivate();

	rawShowFilter(0);
}


bool UI_FindAndReplace::WhatFromEditMode()
{
	switch (edit.mode)
	{
		case OBJ_THINGS:   what->value(0); return true;
		case OBJ_LINEDEFS: what->value(1); return true;
		case OBJ_SECTORS:  what->value(2); return true;

		default: return false;
	}
}


char UI_FindAndReplace::GetKind()
{
	// these letters are same as the Browser uses

	int v = what->value();

	if (v < 0 || v >= 5)
		return '?';

	const char *kinds = "OTFLS";

	return kinds[v];
}


void UI_FindAndReplace::BrowsedItem(char kind, int number, const char *name, int e_state)
{
	// TODO
}


void UI_FindAndReplace::find_but_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->FindNext();
}


void UI_FindAndReplace::select_all_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->DoAll(false /* replace */);
}


void UI_FindAndReplace::apply_but_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->DoReplace();
}


void UI_FindAndReplace::replace_all_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->DoAll(true /* replace */);
}


void UI_FindAndReplace::find_match_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	bool is_valid = box->CheckInput(box->find_match, box->find_desc, box->nums_to_match);

	if (is_valid)
	{
		box->find_but->activate();
		box->select_all_but->activate();

		box->find_match->textcolor(FL_FOREGROUND_COLOR);
		box->find_match->redraw();
	}
	else
	{
		box->find_but->deactivate();
		box->select_all_but->deactivate();

		box->find_match->textcolor(FL_RED);
		box->find_match->redraw();
	}

	// update Replace section too
	box->rep_value->do_callback();
}

void UI_FindAndReplace::rep_value_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	bool is_valid = box->CheckInput(box->rep_value, box->rep_desc);

	if (is_valid)
	{
		box->rep_value->textcolor(FL_FOREGROUND_COLOR);
		box->rep_value->redraw();
	}
	else
	{
		box->rep_value->textcolor(FL_RED);
		box->rep_value->redraw();
	}

	bool is_usable = (is_valid && box->find_but->active());

	if (is_usable)
		box->replace_all_but->activate();
	else
		box->replace_all_but->deactivate();

	// require an found object too before 'Replace' button can be used

	if (box->cur_obj.is_nil())
		is_usable = false;

	if (is_usable)
		box->apply_but->activate();
	else
		box->apply_but->deactivate();
}


bool UI_FindAndReplace::CheckInput(Fl_Input *w, Fl_Output *desc, number_group_c *num_grp)
{
	if (strlen(w->value()) == 0)
	{
		desc->value("");
		return false;
	}

	if (what->value() == 1 || what->value() == 2)
		return true;


	// for numeric types, parse the number(s) and/or ranges

	int type_num;

	if (! num_grp)
	{
		// just check the number is valid
		char *endptr;

		type_num = strtol(w->value(), &endptr, 0 /* allow hex */);

		if (*endptr != 0)
		{
			desc->value("(parse error)");
			return false;
		}
	}
	else
	{
		num_grp->clear();

		if (! num_grp->ParseString(w->value()))
		{
			desc->value("(parse error)");
			return false;
		}

		if (num_grp->is_everything())
		{
			desc->value("(everything)");
			return true;
		}
		else if (! num_grp->is_single())
		{
			desc->value("(multi-match)");
			return true;
		}

		type_num = num_grp->grab_first();
	}


	switch (what->value())
	{
		case 0: // Things
		{
			const thingtype_t *info = M_GetThingType(type_num);
			desc->value(info->desc);
			break;
		}

		case 3: // Lines by Type
		{
			const linetype_t *info = M_GetLineType(type_num);
			desc->value(info->desc);
			break;
		}

		case 4: // Lines by Type
		{
			const sectortype_t * info = M_GetSectorType(type_num);
			desc->value(info->desc);
			break;
		}

		default: break;
	}

	return true;
}


void UI_FindAndReplace::find_choose_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	// FIXME
}

void UI_FindAndReplace::rep_choose_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	// FIXME
}


//------------------------------------------------------------------------


bool UI_FindAndReplace::FindNext()
{
	// this can happen via CTRL-G shortcut (View / Go to next)
	if (strlen(find_match->value()) == 0)
	{
		Beep("No find active!");
		return false;
	}

	if (cur_obj.type != edit.mode)
	{
		Editor_ChangeMode_Raw(cur_obj.type);

		// this clears the selection
		edit.Selected->change_type(edit.mode);
	}
	else
	{
		edit.Selected->clear_all();
	}

	edit.RedrawMap = 1;


	bool is_first = cur_obj.is_nil();

	int start_at = cur_obj.is_nil() ? 0 : (cur_obj.num + 1);
	int total    = NumObjects(cur_obj.type);

	for (int idx = start_at ; idx < total ; idx++)
	{
		if (MatchesObject(idx))
		{
			cur_obj.num = idx;

			if (is_first)
			{
				find_but->label("Next");
				rep_value->do_callback();
			}

			GoToObject(cur_obj);

			Status_Set("Found #%d", idx);
			return true;
		}
	}

	// nothing (else) was found

	cur_obj.clear();

	find_but->label("Find");
	rep_value->do_callback();

	if (is_first)
		Beep("Nothing found");
	else
		Beep("No more found");

	return false;
}


void UI_FindAndReplace::DoReplace()
{
	// sanity check  [ should never happen ]
	if (strlen(find_match->value()) == 0 ||
		strlen( rep_value->value()) == 0)
	{
		Beep("Bad replace");
		return;
	}

	// this generally can't happen either
	if (cur_obj.is_nil())
	{
		Beep("No object to replace");
		return;
	}

	BA_Begin();

	ApplyReplace(cur_obj.num);

	BA_End();

	// move onto next object
	FindNext();
}


bool UI_FindAndReplace::MatchesObject(int idx)
{
	switch (what->value())
	{
		case 0: // Things
			return Match_Thing(idx) && Filter_Thing(idx);

		case 1: // LineDefs (texturing)
			return Match_LineDef(idx) && Filter_LineDef(idx);

		case 2: // Sectors (texturing)
			return Match_Sector(idx) && Filter_Sector(idx);

		case 3: // Lines by Type
			return Match_LineType(idx) && Filter_LineDef(idx);

		case 4: // Sectors by Type
			return Match_SectorType(idx) && Filter_Sector(idx);

		default: return false;
	}
}


void UI_FindAndReplace::ApplyReplace(int idx)
{
	SYS_ASSERT(idx >= 0);

	switch (what->value())
	{
		case 0: // Things
			Replace_Thing(idx);
			break;

		case 1: // LineDefs (texturing)
			Replace_LineDef(idx);
			break;

		case 2: // Sectors (texturing)
			Replace_Sector(idx);
			break;

		case 3: // Lines by Type
			Replace_LineType(idx);
			break;

		case 4: // Sectors by Type
			Replace_SectorType(idx);
			break;

		default: break;
	}
}


void UI_FindAndReplace::DoAll(bool replace)
{
	if (strlen(find_match->value()) == 0)
	{
		Beep("No find active!");
		return;
	}

	if (cur_obj.type != edit.mode)
		Editor_ChangeMode_Raw(cur_obj.type);

	if (replace)
	{
		BA_Begin();
	}

	// we select objects even in REPLACE mode
	// (gives the user a visual indication that stuff was done)

	// this clears the selection
	edit.Selected->change_type(edit.mode);

	int total = NumObjects(cur_obj.type);
	int count = 0;

	for (int idx = 0 ; idx < total ; idx++)
	{
		if (! MatchesObject(idx))
			continue;

		count++;

		if (replace)
			ApplyReplace(idx);

		edit.Selected->set(idx);
	}

	if (count == 0)
		Beep("Nothing found");
	else
		Status_Set("Found %d objects", count);

	if (replace)
	{
		BA_End();
	}

	if (count > 0)
		GoToSelection();

	if (replace)
	{
		cur_obj.clear();
		rep_value->do_callback();

		edit.error_mode = true;
	}

	edit.RedrawMap = 1;
}


//------------------------------------------------------------------------
//    MATCHING METHODS
//------------------------------------------------------------------------

bool UI_FindAndReplace::Match_Thing(int idx)
{
	const Thing *T = Things[idx];

	if (! nums_to_match->get(T->type))
		return false;

	return true;
}


bool UI_FindAndReplace::Match_LineDef(int idx)
{
	// TODO
	return false;
}


bool UI_FindAndReplace::Match_Sector(int idx)
{
	// TODO
	return false;
}


bool UI_FindAndReplace::Match_LineType(int idx)
{
	const LineDef *L = LineDefs[idx];

	if (! nums_to_match->get(L->type))
		return false;

	return true;
}


bool UI_FindAndReplace::Match_SectorType(int idx)
{
	const Sector *SEC = Sectors[idx];

	if (! nums_to_match->get(SEC->type))
		return false;

	return true;
}


bool UI_FindAndReplace::Filter_Thing(int idx)
{
	// TODO
	return true;
}


bool UI_FindAndReplace::Filter_LineDef(int idx)
{
	// TODO
	return true;
}


bool UI_FindAndReplace::Filter_Sector(int idx)
{
	// TODO
	return true;
}


//------------------------------------------------------------------------
//    REPLACE METHODS
//------------------------------------------------------------------------

void UI_FindAndReplace::Replace_Thing(int idx)
{
	int new_type = atoi(rep_value->value());

	BA_ChangeTH(idx, Thing::F_TYPE, new_type);
}


void UI_FindAndReplace::Replace_LineDef(int idx)
{
	// TODO
}


void UI_FindAndReplace::Replace_Sector(int idx)
{
	// TODO
}


void UI_FindAndReplace::Replace_LineType(int idx)
{
	int new_type = atoi(rep_value->value());

	BA_ChangeLD(idx, LineDef::F_TYPE, new_type);
}


void UI_FindAndReplace::Replace_SectorType(int idx)
{
	int new_type = atoi(rep_value->value());

	BA_ChangeSEC(idx, Sector::F_TYPE, new_type);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
