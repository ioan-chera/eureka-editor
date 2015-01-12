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
}


void UI_FindAndReplace::Open()
{
	show();

	Clear();
}


void UI_FindAndReplace::Clear()
{
	cur_obj.clear();

	find_match->value("");
	find_but->label("Find");

	rep_value->value("");

	rawShowFilter(0);
}


void UI_FindAndReplace::BrowsedItem(char kind, int number, const char *name, int e_state)
{
	// TODO
}


void UI_FindAndReplace::find_but_callback(Fl_Widget *w, void *data)
{
	// TODO
}


void UI_FindAndReplace::select_all_callback(Fl_Widget *w, void *data)
{
	// TODO
}


void UI_FindAndReplace::apply_but_callback(Fl_Widget *w, void *data)
{
	// TODO
}


void UI_FindAndReplace::replace_all_callback(Fl_Widget *w, void *data)
{
	// TODO
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
