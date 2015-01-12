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

#include "main.h"
#include "ui_window.h"

#include "e_path.h"  // GoToObject
#include "m_game.h"


UI_FindAndReplace::UI_FindAndReplace(int X, int Y, int W, int H) :
	Fl_Group(X, Y, W, H, NULL),
	cur_obj()
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


		what = new Fl_Choice(X+60, Y+45, W - 70, 35);
		what->textsize(18);
		what->add("Things|LineDefs|Sectors|Line by Type|Sec by Type");
		what->value(0);
		what->callback(what_kind_callback, this);


		find_match = new Fl_Input(X+70, Y+95, 127, 25, "Match: ");
		find_match->callback(find_match_callback, this);
		find_match->when(FL_WHEN_CHANGED);

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

	Fl_Group *grp2 = new Fl_Group(X, Y + 214, W, 156);
	grp2->box(FL_UP_BOX);
	{
		rep_toggle = new Fl_Toggle_Button(X+15, Y+220, 30, 30, "^");
		rep_toggle->labelsize(16);
		rep_toggle->color(FL_DARK3, FL_DARK3);
		rep_toggle->value(1);
		rep_toggle->callback(rep_toggle_callback, this);
		rep_toggle->clear_visible_focus();

		Fl_Box * r_text = new Fl_Box(X+60, Y+220, 200, 30, "Replace Info");
		r_text->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		r_text->labelsize(16);

		rep_group = new Fl_Group(X, Y+260, W, 100);
		{
			rep_value = new Fl_Output(X+70, Y+260, 127, 25, "New: ");
			rep_value->when(FL_WHEN_CHANGED);

			rep_choose = new Fl_Button(X+210, Y+260, 70, 25, "Choose");

			rep_desc = new Fl_Output(X+70, Y+290, 210, 25, "Desc: ");

			apply_but = new Fl_Button(X+50, Y+325, 80, 30, "Apply");
			apply_but->labelfont(FL_HELVETICA_BOLD);
			apply_but->callback(apply_but_callback, this);

			replace_all_but = new Fl_Button(X+160, Y+325, 105, 30, "Replace All");
			replace_all_but->callback(replace_all_callback, this);
		}
		rep_group->end();
	}
	grp2->end();


	/* ---- FILTER AREA ---- */

	Fl_Group *grp3 = new Fl_Group(X, Y + 374, W, H - 374);
	grp3->box(FL_UP_BOX);
	{
		filter_toggle = new Fl_Toggle_Button(X+15, Y+380, 30, 30, "v");
		filter_toggle->labelsize(16);
		filter_toggle->color(FL_DARK3, FL_DARK3);
		filter_toggle->callback(filter_toggle_callback, this);
		filter_toggle->clear_visible_focus();

		Fl_Box *f_text = new Fl_Box(X+60, Y+380, 200, 30, "Search Filters");
		f_text->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		f_text->labelsize(16);

		filter_group = new Fl_Group(X, Y+415, W, H-415);
		{
			only_floors = new Fl_Round_Button(X+35, Y+415, 145, 25, " Only Floors");
			only_floors->down_box(FL_ROUND_DOWN_BOX);

			only_ceilings = new Fl_Round_Button(X+35, Y+435, 165, 30, " Only Ceilings");
			only_ceilings->down_box(FL_ROUND_DOWN_BOX);

			tag_match = new Fl_Input(X+105, Y+466, 130, 24, "Tag Match:");
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


void UI_FindAndReplace::rawShowReplace(int value)
{
	if (value)
	{
		rep_toggle->label("^");
		rep_group->show();
	}
	else
	{
		rep_toggle->label("v");
		rep_group->hide();
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


void UI_FindAndReplace::rep_toggle_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	Fl_Toggle_Button *toggle = (Fl_Toggle_Button *)w;
		
	box->rawShowReplace(toggle->value());
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

	Clear();

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
	// TODO
}


void UI_FindAndReplace::replace_all_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->DoAll(true /* replace */);
}


void UI_FindAndReplace::find_match_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	bool  is_valid = box->CheckInput(box->find_match, box->find_desc);
	bool was_valid = (box->find_but->active());

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
}


bool UI_FindAndReplace::CheckInput(Fl_Input *w, Fl_Output *desc)
{
	if (strlen(w->value()) == 0)
	{
		desc->value("");
		return false;
	}

	if (what->value() == 1 || what->value() == 2)
		return true;

	/* decide what to show in description */

	// wildcard match?
	if (strchr(w->value(), '*') || strchr(w->value(), '?') ||
		strchr(w->value(), '[') || strchr(w->value(), '{'))
	{
		desc->value("(wildcard match)");
		return true;
	}

	// valid integer?
	char *endptr;
	int type_num = (int)strtol(w->value(), &endptr, 0 /* allow 0x etc */);

	if (*endptr != 0)
	{
		desc->value("(bad number)");
		return false;
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

//------------------------------------------------------------------------


void UI_FindAndReplace::FindNext()
{
	// this can happen via CTRL-G shortcut (View / Go to next)
	if (cur_obj.type == OBJ_NONE || strlen(find_match->value()) == 0)
	{
		Beep("No find active!");
		return;
	}

	bool is_first = (cur_obj.num < 0);

	int start_at = (cur_obj.num < 0) ? 0 : (cur_obj.num + 1);
	int total    = NumObjects(cur_obj.type);

	for (int idx = start_at ; idx < total ; idx++)
	{
		if (MatchesObject(idx))
		{
			cur_obj.num = idx;

			if (cur_obj.type != edit.mode)
			{
				Editor_ChangeMode_Raw(cur_obj.type);

				// this clears the selection
				edit.Selected->change_type(edit.mode);
			}

			GoToObject(cur_obj);
			return;
		}
	}

	// nothing (else) was found

	cur_obj.num = -1;

	find_but->label("Find");

	if (is_first)
		Beep("Nothing found");
	else
		Beep("No more found");
}


bool UI_FindAndReplace::MatchesObject(int idx)
{
	// FIXME !!!

	return false;
}


void UI_FindAndReplace::ApplyReplace(int idx)
{
	// TODO
}


void UI_FindAndReplace::DoAll(bool replace)
{
	if (cur_obj.type == OBJ_NONE)
	{
		Beep("No find active!");
		return;
	}

	if (replace)
	{
		BA_Begin();
	}
	else
	{
		Editor_ChangeMode_Raw(cur_obj.type);

		// this clears the selection
		edit.Selected->change_type(edit.mode);
	}

	int total = NumObjects(cur_obj.type);

	for (int idx = 0 ; idx < total ; idx++)
	{
		if (! MatchesObject(idx))
			continue;

		if (replace)
			ApplyReplace(idx);
		else
			edit.Selected->set(idx);
	}

	if (replace)
	{
		BA_End();
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
