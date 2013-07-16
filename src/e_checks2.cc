//------------------------------------------------------------------------
//  INTEGRITY CHECKS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2013 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include <algorithm>

#include "e_checks.h"
#include "e_path.h"
#include "e_vertex.h"
#include "editloop.h"
#include "m_dialog.h"
#include "m_game.h"
#include "levels.h"
#include "objects.h"
#include "selectn.h"
#include "w_rawdef.h"
#include "ui_window.h"
#include "x_hover.h"


#define   ERROR_MSG_COLOR	FL_RED
#define WARNING_MSG_COLOR	FL_BLUE


static char check_buffer[MSG_BUF_LEN];


struct vertex_X_CMP_pred
{
	inline bool operator() (int A, int B) const
	{
		const Vertex *V1 = Vertices[A];
		const Vertex *V2 = Vertices[B];

		return V1->x < V2->x;
	}
};


void Vertex_FindOverlaps(selection_c& sel, bool one_coord = false)
{
	// the 'one_coord' parameter limits the selection to a single
	// vertex coordinate.

	sel.change_type(OBJ_VERTICES);

	if (NumVertices < 2)
		return;

	// sort the vertices into order of the 'X' value.
	// hence any overlapping vertices will be near each other.

	std::vector<int> sorted_list(NumVertices, 0);

	for (int i = 0 ; i < NumVertices ; i++)
		sorted_list[i] = i;

	std::sort(sorted_list.begin(), sorted_list.end(), vertex_X_CMP_pred());

	bool seen_one = false;
	int last_y = 0;

#define VERT_K  Vertices[sorted_list[k]]
#define VERT_N  Vertices[sorted_list[n]]

	for (int k = 0 ; k < NumVertices ; k++)
	{
		for (int n = k + 1 ; n < NumVertices && VERT_N->x == VERT_K->x ; n++)
		{
			if (VERT_N->y == VERT_K->y)
			{
				if (one_coord && seen_one && VERT_K->y != last_y)
					continue;

				sel.set(sorted_list[k]);
				sel.set(sorted_list[n]);

				seen_one = true; last_y = VERT_K->y;
			}
		}
	}

#undef VERT_K
#undef VERT_N
}


void Vertex_MergeOverlaps()
{
	for (;;)
	{
		selection_c sel;

		Vertex_FindOverlaps(sel, true /* one_coord */);

		if (sel.empty())
			break;

		Vertex_MergeList(&sel);
	}
}


void Vertex_ShowOverlaps()
{
	if (edit.mode != OBJ_VERTICES)
		Editor_ChangeMode('v');

	Vertex_FindOverlaps(*edit.Selected);

	GoToSelection();

	edit.error_mode = true;
	edit.RedrawMap = 1;
}


void Vertex_FindUnused(selection_c& sel)
{
	sel.change_type(OBJ_VERTICES);

	if (NumVertices == 0)
		return;

	for (int i = 0 ; i < NumLineDefs ; i++)
	{
		sel.set(LineDefs[i]->start);
		sel.set(LineDefs[i]->end);
	}

	sel.frob_range(0, NumVertices - 1, BOP_TOGGLE);
}


void Vertex_RemoveUnused()
{
	selection_c sel;

	Vertex_FindUnused(sel);

	BA_Begin();
	DeleteObjects(&sel);
	BA_End();

//??	Status_Set("Removed %d vertices", sel.count_obj());
}


//------------------------------------------------------------------------


class UI_Check_Vertices : public Fl_Double_Window
{
private:
	bool want_close;

	check_result_e user_action;

	Fl_Group  *line_group;
	Fl_Button *ok_but;

	int cy;

public:
	int worst_severity;

public:
	static void close_callback(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;

		dialog->want_close = true;
	}

	static void action_merge(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;

		Vertex_MergeOverlaps();

		dialog->user_action = CKR_TookAction;
	}

	static void action_highlight(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;

		Vertex_ShowOverlaps();

		dialog->user_action = CKR_Highlight;
	}

	static void action_remove(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;

		Vertex_RemoveUnused();

		dialog->user_action = CKR_TookAction;
	}

public:
	UI_Check_Vertices(bool all_mode) :
		Fl_Double_Window(520, 186, "Check : Vertices"),
		want_close(false), user_action(CKR_OK),
		worst_severity(0)
	{
		cy = 10;

		callback(close_callback, this);

		int ey = h() - 66;

		Fl_Box *title = new Fl_Box(FL_NO_BOX, 10, cy, w() - 20, 30, "Vertex check results");
		title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		title->labelfont(FL_HELVETICA_BOLD);
		title->labelsize(FL_NORMAL_SIZE + 2);

		cy = 45;

		line_group = new Fl_Group(0, 0, w(), ey);
		line_group->end();

		{ Fl_Group *o = new Fl_Group(0, ey, w(), 66);

		  o->box(FL_FLAT_BOX);
		  o->color(WINDOW_BG, WINDOW_BG);

		  int but_W = all_mode ? 110 : 70;

		  { ok_but = new Fl_Button(w()/2 - but_W/2, ey + 18, but_W, 34,
		                           all_mode ? "Continue" : "OK");
			ok_but->labelfont(1);
			ok_but->callback(close_callback, this);
		  }
		  o->end();
		}

		end();
	}

	void Reset()
	{
		want_close = false;
		user_action = CKR_OK;

		cy = 45;

		line_group->clear();	

		redraw();
	}

	void AddLine(const char *msg, int severity = 0, int W = -1,
	             const char *button1 = NULL, Fl_Callback *cb1 = NULL,
	             const char *button2 = NULL, Fl_Callback *cb2 = NULL,
	             const char *button3 = NULL, Fl_Callback *cb3 = NULL)
	{
		int cx = 25;

		if (W < 0)
			W = w() - 40;

		Fl_Box *box = new Fl_Box(FL_NO_BOX, cx, cy, W, 25, NULL);
		box->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
		box->copy_label(msg);

		if (severity == 2)
		{
			box->labelcolor(ERROR_MSG_COLOR);
			box->labelfont(FL_HELVETICA_BOLD);
		}
		else if (severity == 1)
		{
			box->labelcolor(WARNING_MSG_COLOR);
			box->labelfont(FL_HELVETICA_BOLD);
		}

		line_group->add(box);

		cx += W;

		if (button1)
		{
			Fl_Button *but = new Fl_Button(cx, cy, 80, 25, button1);
			but->callback(cb1, this);

			line_group->add(but);

			cx += but->w() + 10;
		}

		if (button2)
		{
			Fl_Button *but = new Fl_Button(cx, cy, 80, 25, button2);
			but->callback(cb2, this);

			line_group->add(but);

			cx += but->w() + 10;
		}

		if (button3)
		{
			Fl_Button *but = new Fl_Button(cx, cy, 80, 25, button3);
			but->callback(cb3, this);

			line_group->add(but);
		}

		cy = cy + 30;

		if (severity > worst_severity)
			worst_severity = severity;
	}

	void AddGap(int H)
	{
		cy += H;
	}

	check_result_e Run()
	{
		set_modal();

		show();

		while (! (want_close || user_action != CKR_OK))
			Fl::wait(0.2);

		if (user_action != CKR_OK)
			return user_action;

		switch (worst_severity)
		{
			case 0:  return CKR_OK;
			case 1:  return CKR_MinorProblem;
			default: return CKR_MajorProblem;
		}
	}
};


check_result_e CHECK_Vertices(bool all_mode = false)
{
	UI_Check_Vertices *dialog = new UI_Check_Vertices(all_mode);

	selection_c  sel;

	for (;;)
	{
		Vertex_FindOverlaps(sel);

		if (sel.empty())
			dialog->AddLine("No overlapping vertices");
		else
		{
			int approx_num = sel.count_obj() / 2;

			sprintf(check_buffer, "%d overlapping vertices", approx_num);

			dialog->AddLine(check_buffer, 2, 210,
			                "Merge", &UI_Check_Vertices::action_merge,
			                "Show",  &UI_Check_Vertices::action_highlight);
		}


		Vertex_FindUnused(sel);

		if (sel.empty())
			dialog->AddLine("No unused vertices");
		else
		{
			sprintf(check_buffer, "%d unused vertices", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 170,
			                "Remove", &UI_Check_Vertices::action_remove);
		}


		int worst_severity = dialog->worst_severity;

		// when checking "ALL" stuff, ignore any minor problems
		if (all_mode && worst_severity < 2)
		{
			delete dialog;

			return CKR_OK;
		}

		check_result_e result = dialog->Run();

		if (result == CKR_TookAction)
		{
			// repeat the tests
			dialog->Reset();
			continue;
		}

		delete dialog;

		return result;
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
