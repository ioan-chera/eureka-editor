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
#include "e_cutpaste.h"
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


static int linedef_pos_cmp(int A, int B)
{
	const LineDef *AL = LineDefs[A];
	const LineDef *BL = LineDefs[B];

	int A_x1 = AL->Start()->x;
	int A_y1 = AL->Start()->y;
	int A_x2 = AL->End()->x;
	int A_y2 = AL->End()->y;

	int B_x1 = BL->Start()->x;
	int B_y1 = BL->Start()->y;
	int B_x2 = BL->End()->x;
	int B_y2 = BL->End()->y;

	if (A_x1 > A_x2 || (A_x1 == A_x2 && A_y1 > A_y2))
	{
		std::swap(A_x1, A_x2);
		std::swap(A_y1, A_y2);
	}

	if (B_x1 > B_x2 || (B_x1 == B_x2 && B_y1 > B_y2))
	{
		std::swap(B_x1, B_x2);
		std::swap(B_y1, B_y2);
	}

	// the "normalized" X1 coordinates is the most significant thing in
	// this comparison function.

	if (A_x1 != B_x1) return A_x1 - B_x1;
	if (A_y1 != B_y1) return A_y1 - B_y1;

	if (A_x2 != B_x2) return A_x2 - B_x2;
	if (A_y2 != B_y2) return A_y2 - B_y2;

	return 0;  // equal : lines are overlapping
}


struct linedef_pos_CMP_pred
{
	inline bool operator() (int A, int B) const
	{
		return linedef_pos_cmp(A, B) < 0;
	}
};


void LineDefs_FindZeroLen(selection_c& lines)
{
	lines.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
		if (LineDefs[n]->isZeroLength())
			lines.set(n);
}


void LineDefs_RemoveZeroLen()
{
	selection_c sel;

	BA_Begin();

	for (int n = NumLineDefs-1 ; n >= 0 ; n--)
	{
		const LineDef *L = LineDefs[n];

		if (! L->isZeroLength())
			continue;

		// merge the vertices if possible

		if (L->start == L->end)
		{
			BA_Delete(OBJ_LINEDEFS, n);
		}
		else
		{
			MergeVertex(L->start, L->end, true);
			BA_Delete(OBJ_VERTICES, L->start);
		}
	}

	BA_End();
}


void LineDefs_ShowZeroLen()
{
	if (edit.mode != OBJ_VERTICES)
		Editor_ChangeMode('v');

	selection_c sel;

	LineDefs_FindZeroLen(sel);

	ConvertSelection(&sel, edit.Selected);

	GoToSelection();

	edit.error_mode = true;
	edit.RedrawMap = 1;
}


void LineDefs_FindMissingRight(selection_c& lines)
{
	lines.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
		if (LineDefs[n]->right < 0)
			lines.set(n);
}


void LineDefs_ShowMissingRight()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	LineDefs_FindMissingRight(*edit.Selected);

	GoToSelection();

	edit.error_mode = true;
	edit.RedrawMap = 1;
}


void LineDefs_FindOverlaps(selection_c& lines)
{
	// we only find directly overlapping linedefs here

	lines.change_type(OBJ_LINEDEFS);

	if (NumLineDefs < 2)
		return;

	int n;

	// sort linedefs by their position.  overlapping lines will end up
	// adjacent to each other after the sort.
	std::vector<int> sorted_list(NumLineDefs, 0);

	for (n = 0 ; n < NumLineDefs ; n++)
		sorted_list[n] = n;

	std::sort(sorted_list.begin(), sorted_list.end(), linedef_pos_CMP_pred());

	for (n = 0 ; n < NumLineDefs - 1 ; n++)
	{
		int ld1 = sorted_list[n];
		int ld2 = sorted_list[n + 1];

		// only the second (or third, etc) linedef is stored
		if (linedef_pos_cmp(ld1, ld2) == 0)
			lines.set(ld2);
	}
}


void LineDefs_ShowOverlaps()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	LineDefs_FindOverlaps(*edit.Selected);

	GoToSelection();

	edit.error_mode = true;
	edit.RedrawMap = 1;
}


void LineDefs_RemoveOverlaps()
{
	selection_c lines, unused_verts;

	LineDefs_FindOverlaps(lines);

	UnusedVertices(&lines, &unused_verts);

	BA_Begin();

	DeleteObjects(&lines);
	DeleteObjects(&unused_verts);

	BA_End();
}


//------------------------------------------------------------------------


class UI_Check_LineDefs : public Fl_Double_Window
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
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;

		dialog->want_close = true;
	}

	static void action_show_zero(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowZeroLen();
		dialog->user_action = CKR_Highlight;
	}

	static void action_remove_zero(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_RemoveZeroLen();
		dialog->user_action = CKR_TookAction;
	}

	static void action_show_mis_right(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowMissingRight();
		dialog->user_action = CKR_Highlight;
	}

	static void action_remove_overlap(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_RemoveOverlaps();
		dialog->user_action = CKR_TookAction;
	}

	static void action_show_overlap(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowOverlaps();
		dialog->user_action = CKR_Highlight;
	}

public:
	UI_Check_LineDefs(bool all_mode) :
		Fl_Double_Window(520, 386, "Check : LineDefs"),
		want_close(false), user_action(CKR_OK),
		worst_severity(0)
	{
		cy = 10;

		callback(close_callback, this);

		int ey = h() - 66;

		Fl_Box *title = new Fl_Box(FL_NO_BOX, 10, cy, w() - 20, 30, "LineDef check results");
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
		int cx = 30;

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


check_result_e CHECK_LineDefs(bool all_mode)
{
	UI_Check_LineDefs *dialog = new UI_Check_LineDefs(all_mode);

	selection_c  sel, other;

	for (;;)
	{
		LineDefs_FindZeroLen(sel);

		if (sel.empty())
			dialog->AddLine("No zero-length linedefs");
		else
		{
			sprintf(check_buffer, "%d zero-length linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 220,
			                "Show",   &UI_Check_LineDefs::action_show_zero,
			                "Remove", &UI_Check_LineDefs::action_remove_zero);
		}


		LineDefs_FindOverlaps(sel);

		if (sel.empty())
			dialog->AddLine("No overlapping linedefs");
		else
		{
			sprintf(check_buffer, "%d overlapping linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 200,
			                "Show",   &UI_Check_LineDefs::action_show_overlap,
			                "Remove", &UI_Check_LineDefs::action_remove_overlap);
		}


		LineDefs_FindMissingRight(sel);

		if (sel.empty())
			dialog->AddLine("No linedefs without a right side");
		else
		{
			sprintf(check_buffer, "%d linedefs without right side", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 250,
			                "Show", &UI_Check_LineDefs::action_show_mis_right);
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
