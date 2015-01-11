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
	Fl_Group(X, Y, W, H, NULL)
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


		what = new Fl_Choice(X+70, Y+55, 150, 25);


		find_match = new Fl_Input(X+70, Y+95, 127, 25, "Match: ");

		find_choose = new Fl_Button(X+210, Y+95, 70, 25, "Choose");

		find_desc = new Fl_Output(X+70, Y+125, 210, 25, "Desc: ");

		find_but = new Fl_Button(X+50, Y+165, 80, 30, "Find");
		find_but->labelfont(FL_HELVETICA_BOLD);

		select_all_but = new Fl_Button(X+165, Y+165, 93, 30, "Select All");
	}
	grp1->end();


	/* ---- REPLACE AREA ---- */

	Fl_Group *grp2 = new Fl_Group(X, Y + 214, W, 156);
	grp2->box(FL_UP_BOX);
	{
		rep_toggle = new Fl_Toggle_Button(X+15, Y+220, 160, 30, "Replace Info");
		rep_toggle->labelsize(16);

		Fl_Group *rep_group = new Fl_Group(X+50, Y+260, 230, 95);
		{
			rep_value = new Fl_Output(X+70, Y+260, 127, 25, "New: ");

			rep_choose = new Fl_Button(X+210, Y+260, 70, 25, "Choose");

			rep_desc = new Fl_Output(X+70, Y+290, 210, 25, "Desc: ");

			apply_but = new Fl_Button(X+50, Y+325, 80, 30, "Apply");
			apply_but->labelfont(FL_HELVETICA_BOLD);

			replace_all_but = new Fl_Button(X+160, Y+325, 105, 30, "Replace All");
		}
		rep_group->end();

	}
	grp2->end();


	/* ---- FILTER AREA ---- */

	Fl_Group *grp3 = new Fl_Group(X, Y + 374, W, H - 374);
	grp3->box(FL_UP_BOX);
	{
		filter_toggle = new Fl_Toggle_Button(X+15, Y+380, 100, 30, "Filters");
		filter_toggle->labelsize(16);

		filter_group = new Fl_Group(X+35, Y+415, 200, 75);
		{
			only_floors = new Fl_Round_Button(X+35, Y+415, 145, 25, " Only Floors");
			only_floors->down_box(FL_ROUND_DOWN_BOX);

			only_ceilings = new Fl_Round_Button(X+35, Y+435, 165, 30, " Only Ceilings");
			only_ceilings->down_box(FL_ROUND_DOWN_BOX);

			tag_match = new Fl_Input(X+105, Y+466, 130, 24, "Tag Match:");
		}
		filter_group->end();
	}
	grp3->end();


	grp3->resizable(NULL);
	resizable(grp3);

	end();
}


UI_FindAndReplace::~UI_FindAndReplace()
{ }


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
