//------------------------------------------------------------------------
//  INTEGRITY CHECKS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
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
#include "m_game.h"
#include "levels.h"
#include "objects.h"
#include "w_rawdef.h"
#include "ui_window.h"
#include "x_hover.h"


#define   ERROR_MSG_COLOR	FL_RED
#define WARNING_MSG_COLOR	FL_BLUE


static char check_message[MSG_BUF_LEN];


//------------------------------------------------------------------------
//  BASE CLASS
//------------------------------------------------------------------------

void UI_Check_base::close_callback(Fl_Widget *w, void *data)
{
	UI_Check_base *dialog = (UI_Check_base *)data;

	dialog->want_close = true;
}


UI_Check_base::UI_Check_base(int W, int H, bool all_mode,
                             const char *L, const char *header_txt) :
	UI_Escapable_Window(W, H, L),
	want_close(false), user_action(CKR_OK),
	worst_severity(0)
{
	cy = 10;

	callback(close_callback, this);

	int ey = h() - 66;

	Fl_Box *title = new Fl_Box(FL_NO_BOX, 10, cy, w() - 20, 30, header_txt);
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

	  { Fl_Button *ok_but;

	    ok_but = new Fl_Button(w()/2 - but_W/2, ey + 18, but_W, 34,
							   all_mode ? "Continue" : "OK");
		ok_but->labelfont(1);
		ok_but->callback(close_callback, this);
	  }
	  o->end();
	}

	end();
}


UI_Check_base::~UI_Check_base()
{ }


void UI_Check_base::Reset()
{
	want_close = false;
	user_action = CKR_OK;

	cy = 45;

	line_group->clear();	

	redraw();
}


void UI_Check_base::AddGap(int H)
{
	cy += H;
}


void UI_Check_base::AddLine(
		const char *msg, int severity, int W,
		 const char *button1, Fl_Callback *cb1,
		 const char *button2, Fl_Callback *cb2,
		 const char *button3, Fl_Callback *cb3)
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


check_result_e UI_Check_base::Run()
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


//------------------------------------------------------------------------

void Vertex_FindDanglers(selection_c& sel)
{
	sel.change_type(OBJ_VERTICES);

	if (NumVertices == 0 || NumLineDefs == 0)
		return;

	byte * line_counts = new byte[NumVertices];

	memset(line_counts, 0, NumVertices);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		int v1 = L->start;
		int v2 = L->end;

		// dangling vertices are fine for lines setting inside a sector
		// (i.e. with same sector on both sides)
		if (L->TwoSided() && (L->WhatSector(SIDE_LEFT) == L->WhatSector(SIDE_RIGHT)))
		{
			line_counts[v1] = line_counts[v2] = 2;
			continue;
		}

		if (line_counts[v1] < 2) line_counts[v1] += 1;
		if (line_counts[v2] < 2) line_counts[v2] += 1;
	}

	for (int k = 0 ; k < NumVertices ; k++)
	{
		if (line_counts[k] == 1)
			sel.set(k);
	}

	delete[] line_counts;
}


void Vertex_ShowDanglers()
{
	if (edit.mode != OBJ_VERTICES)
		Editor_ChangeMode('v');

	Vertex_FindDanglers(*edit.Selected);

	GoToErrors();
}


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

	GoToErrors();
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
}


void Vertex_ShowUnused()
{
	if (edit.mode != OBJ_VERTICES)
		Editor_ChangeMode('v');

	Vertex_FindUnused(*edit.Selected);

	GoToErrors();
}


//------------------------------------------------------------------------

class UI_Check_Vertices : public UI_Check_base
{
public:
	UI_Check_Vertices(bool all_mode) :
		UI_Check_base(520, 224, all_mode, "Check : Vertices",
				      "Vertex test results")
	{ }

public:
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

	static void action_show_unused(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;
		Vertex_ShowUnused();
		dialog->user_action = CKR_Highlight;
	}

	static void action_remove(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;
		Vertex_RemoveUnused();
		dialog->user_action = CKR_TookAction;
	}

	static void action_show_danglers(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;
		Vertex_ShowDanglers();
		dialog->user_action = CKR_Highlight;
	}

};


check_result_e CHECK_Vertices(int min_severity = 0)
{
	UI_Check_Vertices *dialog = new UI_Check_Vertices(min_severity > 0);

	selection_c  sel;

	for (;;)
	{
		Vertex_FindDanglers(sel);

		if (sel.empty())
			dialog->AddLine("No dangling vertices");
		else
		{
			sprintf(check_message, "%d dangling vertices", sel.count_obj());

			dialog->AddLine(check_message, 2, 210,
			                "Show",  &UI_Check_Vertices::action_show_danglers);
		}


		Vertex_FindOverlaps(sel);

		if (sel.empty())
			dialog->AddLine("No overlapping vertices");
		else
		{
			int approx_num = sel.count_obj() / 2;

			sprintf(check_message, "%d overlapping vertices", approx_num);

			dialog->AddLine(check_message, 2, 210,
			                "Show",  &UI_Check_Vertices::action_highlight,
			                "Merge", &UI_Check_Vertices::action_merge);
		}


		Vertex_FindUnused(sel);

		if (sel.empty())
			dialog->AddLine("No unused vertices");
		else
		{
			sprintf(check_message, "%d unused vertices", sel.count_obj());

			dialog->AddLine(check_message, 1, 210,
			                "Show",   &UI_Check_Vertices::action_show_unused,
			                "Remove", &UI_Check_Vertices::action_remove);
		}


		// in "ALL" mode, just continue if not too severe
		if (dialog->WorstSeverity() < min_severity)
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


//------------------------------------------------------------------------

void Sectors_FindUnclosed(selection_c& secs, selection_c& verts)
{
	 secs.change_type(OBJ_SECTORS);
	verts.change_type(OBJ_VERTICES);

	if (NumVertices == 0 || NumSectors == 0)
		return;

	byte *ends = new byte[NumVertices];
	int v;

	for (int s = 0 ; s < NumSectors ; s++)
	{
		// clear the "ends" array
		for (v = 0 ; v < NumVertices ; v++)
			ends[v] = 0;

		// for each sidedef bound to the Sector, store a "1" in the "ends"
		// array for its starting vertex, and a "2" for its ending vertex.
		for (int i = 0 ; i < NumLineDefs ; i++)
		{
			const LineDef *L = LineDefs[i];

			if (! L->TouchesSector(s))
				continue;

			// ignore lines with same sector on both sides
			if (L->left >= 0 && L->right >= 0 &&
			    L->Left()->sector == L->Right()->sector)
				continue;

			if (L->right >= 0 && L->Right()->sector == s)
			{
				ends[L->start] |= 1;
				ends[L->end]   |= 2;
			}

			if (L->left >= 0 && L->Left()->sector == s)
			{
				ends[L->start] |= 2;
				ends[L->end]   |= 1;
			}
		}

		// every entry in the "ends" array should be 0 or 3

		for (v = 0 ; v < NumVertices ; v++)
		{
			if (ends[v] == 1 || ends[v] == 2)
			{
				 secs.set(s);
				verts.set(v);
			}
		}
	}

	delete[] ends;
}


void Sectors_ShowUnclosed(obj_type_e what)
{
	if (edit.mode != what)
		Editor_ChangeMode((what == OBJ_SECTORS) ? 's' : 'v');

	selection_c other;

	if (what == OBJ_SECTORS)
		Sectors_FindUnclosed(*edit.Selected, other);
	else
		Sectors_FindUnclosed(other, *edit.Selected);

	GoToErrors();
}


void Sectors_FindMismatches(selection_c& secs, selection_c& lines)
{
	/*
	   Note from RQ:
	   This is a very simple idea, but it works!  The first test (above)
	   checks that all Sectors are closed.  But if a closed set of LineDefs
	   is moved out of a Sector and has all its "external" SideDefs pointing
	   to that Sector instead of the new one, then we need a second test.
	   That's why I check if the SideDefs facing each other are bound to
	   the same Sector.

	   Other note from RQ:
	   Nowadays, what makes the power of a good editor is its automatic tests.
	   So, if you are writing another Doom editor, you will probably want
	   to do the same kind of tests in your program.  Fine, but if you use
	   these ideas, don't forget to credit DEU...  Just a reminder... :-)
	*/

	 secs.change_type(OBJ_SECTORS);
	lines.change_type(OBJ_LINEDEFS);

	if (NumLineDefs == 0 || NumSectors == 0)
		return;

	FastOpposite_Begin();

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->right >= 0)
		{
			int s = OppositeSector(n, SIDE_RIGHT);

			if (s < 0 || L->Right()->sector != s)
			{
				 secs.set(L->Right()->sector);
				lines.set(n);
			}
		}

		if (L->left >= 0)
		{
			int s = OppositeSector(n, SIDE_LEFT);

			if (s < 0 || L->Left()->sector != s)
			{
				 secs.set(L->Left()->sector);
				lines.set(n);
			}
		}
	}

	FastOpposite_Finish();
}


void Sectors_ShowMismatches(obj_type_e what)
{
	if (edit.mode != what)
		Editor_ChangeMode((what == OBJ_SECTORS) ? 's' : 'l');

	selection_c other;

	if (what == OBJ_SECTORS)
		Sectors_FindMismatches(*edit.Selected, other);
	else
		Sectors_FindMismatches(other, *edit.Selected);

	GoToErrors();
}


static void bump_unknown_type(std::map<int, int>& t_map, int type)
{
	int count = 0;

	if (t_map.find(type) != t_map.end())
		count = t_map[type];

	t_map[type] = count + 1;
}


void Sectors_FindUnknown(selection_c& list, std::map<int, int>& types)
{
	types.clear();

	list.change_type(OBJ_SECTORS);

	for (int n = 0 ; n < NumSectors ; n++)
	{
		int type_num = Sectors[n]->type;

		if (type_num < 0 || type_num >= 2048)
		{
			bump_unknown_type(types, type_num);
			list.set(n);
			continue;
		}

		// Boom generalized sectors
		if (game_info.gen_types)
			type_num &= 31;

		const sectortype_t *info = M_GetSectorType(type_num);

		if (strncmp(info->desc, "UNKNOWN", 7) == 0)
		{
			bump_unknown_type(types, type_num);
			list.set(n);
		}
	}
}


void Sectors_ShowUnknown()
{
	if (edit.mode != OBJ_SECTORS)
		Editor_ChangeMode('s');

	std::map<int, int> types;

	Sectors_FindUnknown(*edit.Selected, types);

	GoToErrors();
}


void Sectors_LogUnknown()
{
	selection_c sel;

	std::map<int, int> types;
	std::map<int, int>::iterator IT;

	Sectors_FindUnknown(sel, types);

	LogPrintf("\n");
	LogPrintf("Unknown Sector Types:\n");
	LogPrintf("{\n");

	for (IT = types.begin() ; IT != types.end() ; IT++)
		LogPrintf("  %5d  x %d\n", IT->first, IT->second);

	LogPrintf("}\n");

	LogViewer_Open();
}


void Sectors_ClearUnknown()
{
	selection_c sel;
	std::map<int, int> types;

	Sectors_FindUnknown(sel, types);

	selection_iterator_c it;

	BA_Begin();

	for (sel.begin(&it) ; !it.at_end() ; ++it)
		BA_ChangeSEC(*it, Sector::F_TYPE, 0);

	BA_End();
}


void Sectors_FindUnused(selection_c& sel)
{
	sel.change_type(OBJ_SECTORS);

	if (NumSectors == 0)
		return;

	for (int i = 0 ; i < NumLineDefs ; i++)
	{
		const LineDef *L = LineDefs[i];

		if (L->left >= 0)
			sel.set(L->Left()->sector);

		if (L->right >= 0)
			sel.set(L->Right()->sector);
	}

	sel.frob_range(0, NumSectors - 1, BOP_TOGGLE);
}


void Sectors_RemoveUnused()
{
	selection_c sel;

	Sectors_FindUnused(sel);

	BA_Begin();
	DeleteObjects(&sel);
	BA_End();

//??	Status_Set("Removed %d vertices", sel.count_obj());
}


void Sectors_FindBadCeil(selection_c& sel)
{
	sel.change_type(OBJ_SECTORS);

	if (NumSectors == 0)
		return;

	for (int i = 0 ; i < NumSectors ; i++)
	{
		if (Sectors[i]->ceilh < Sectors[i]->floorh)
			sel.set(i);
	}
}


void Sectors_FixBadCeil()
{
	selection_c sel;

	Sectors_FindBadCeil(sel);

	BA_Begin();

	for (int i = 0 ; i < NumSectors ; i++)
	{
		if (Sectors[i]->ceilh < Sectors[i]->floorh)
		{
			BA_ChangeSEC(i, Sector::F_CEILH, Sectors[i]->floorh);
		}
	}

	BA_End();
}


void Sectors_ShowBadCeil()
{
	if (edit.mode != OBJ_SECTORS)
		Editor_ChangeMode('s');

	Sectors_FindBadCeil(*edit.Selected);

	GoToErrors();
}


void SideDefs_FindUnused(selection_c& sel)
{
	sel.change_type(OBJ_SIDEDEFS);

	if (NumSideDefs == 0)
		return;

	for (int i = 0 ; i < NumLineDefs ; i++)
	{
		const LineDef *L = LineDefs[i];

		if (L->left  >= 0) sel.set(L->left);
		if (L->right >= 0) sel.set(L->right);
	}

	sel.frob_range(0, NumSideDefs - 1, BOP_TOGGLE);
}


void SideDefs_RemoveUnused()
{
	selection_c sel;

	SideDefs_FindUnused(sel);

	BA_Begin();
	DeleteObjects(&sel);
	BA_End();

//??	Status_Set("Removed %d vertices", sel.count_obj());
}


void SideDefs_FindPacking(selection_c& sides, selection_c& lines)
{
	sides.change_type(OBJ_SIDEDEFS);
	lines.change_type(OBJ_LINEDEFS);

	for (int i = 0 ; i < NumLineDefs ; i++)
	for (int k = 0 ; k < i ; k++)
	{
		const LineDef * A = LineDefs[i];
		const LineDef * B = LineDefs[k];

		bool AA = (A->left  >= 0 && A->left == A->right);

		bool AL = (A->left  >= 0 && (A->left  == B->left || A->left  == B->right));
		bool AR = (A->right >= 0 && (A->right == B->left || A->right == B->right));

		if (AL || AA) sides.set(A->left);
		if (AR)       sides.set(A->right);

		if (AL || AR)
		{
			lines.set(i);
			lines.set(k);
		}
		else if (AA)
		{
			lines.set(i);
		}
	}
}


void SideDefs_ShowPacked()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	selection_c sides;

	SideDefs_FindPacking(sides, *edit.Selected);

	GoToErrors();
}


static int Copy_SideDef(int num)
{
	int sd = BA_New(OBJ_SIDEDEFS);

	SideDefs[sd]->RawCopy(SideDefs[num]);

	return sd;
}


static const char *unpack_confirm_message =
	"This map contains shared sidedefs.  It it recommended to unpack "
	"them, otherwise it may cause unexpected behavior during editing "
	"(such as random walls changing their texture).\n\n"
	"Unpack the sidedefs now?";


void SideDefs_Unpack(bool no_history)
{
	selection_c sides;
	selection_c lines;

	SideDefs_FindPacking(sides, lines);

	if (sides.empty())
		return;

	if (false /* confirm_it */)
	{
		if (DLG_Confirm("&No Change|&Unpack", unpack_confirm_message) <= 0)
			return;
	}


	BA_Begin();

	for (int sd = 0 ; sd < NumSideDefs ; sd++)
	{
		if (! sides.get(sd))
			continue;

		// find the first linedef which uses this sidedef
		int first;

		for (first = 0 ; first < NumLineDefs ; first++)
		{
			const LineDef *F = LineDefs[first];

			if (F->left == sd || F->right == sd)
				break;
		}

		if (first >= NumLineDefs)
			continue;

		// handle it when first linedef uses sidedef on both sides
		if (LineDefs[first]->left == LineDefs[first]->right)
		{
			BA_ChangeLD(first, LineDef::F_LEFT, Copy_SideDef(sd));
		}

		// duplicate any remaining references
		for (int ld = first + 1 ; ld < NumLineDefs ; ld++)
		{
			if (LineDefs[ld]->left == sd)
				BA_ChangeLD(ld, LineDef::F_LEFT, Copy_SideDef(sd));

			if (LineDefs[ld]->right == sd)
				BA_ChangeLD(ld, LineDef::F_RIGHT, Copy_SideDef(sd));
		}
	}

	if (no_history)
		BA_Abort(true /* keep changes */);
	else
		BA_End();

	LogPrintf("Unpacked %d shared sidedefs --> %d\n", sides.count_obj(), NumSideDefs);
}


//------------------------------------------------------------------------

class UI_Check_Sectors : public UI_Check_base
{
public:
	UI_Check_Sectors(bool all_mode) :
		UI_Check_base(530, 346, all_mode, "Check : Sectors",
				      "Sector test results")
	{ }

public:
	static void action_remove(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_RemoveUnused();
		dialog->user_action = CKR_TookAction;
	}

	static void action_remove_sidedefs(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		SideDefs_RemoveUnused();
		dialog->user_action = CKR_TookAction;
	}


	static void action_fix_ceil(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_FixBadCeil();
		dialog->user_action = CKR_TookAction;
	}

	static void action_show_ceil(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowBadCeil();
		dialog->user_action = CKR_Highlight;
	}


	static void action_unpack(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		SideDefs_Unpack();
		dialog->user_action = CKR_Highlight;
	}

	static void action_show_packed(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		SideDefs_ShowPacked();
		dialog->user_action = CKR_Highlight;
	}


	static void action_show_unclosed(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowUnclosed(OBJ_SECTORS);
		dialog->user_action = CKR_Highlight;
	}

	static void action_show_un_verts(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowUnclosed(OBJ_VERTICES);
		dialog->user_action = CKR_Highlight;
	}


	static void action_show_mismatch(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowMismatches(OBJ_SECTORS);
		dialog->user_action = CKR_Highlight;
	}

	static void action_show_mis_lines(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowMismatches(OBJ_LINEDEFS);
		dialog->user_action = CKR_Highlight;
	}


	static void action_show_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowUnknown();
		dialog->user_action = CKR_Highlight;
	}

	static void action_log_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_LogUnknown();
		dialog->user_action = CKR_Highlight;
	}

	static void action_clear_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ClearUnknown();
		dialog->user_action = CKR_TookAction;
	}
};


check_result_e CHECK_Sectors(int min_severity = 0)
{
	UI_Check_Sectors *dialog = new UI_Check_Sectors(min_severity > 0);

	selection_c  sel, other;

	std::map<int, int> types;

	for (;;)
	{
		Sectors_FindUnclosed(sel, other);

		if (sel.empty())
			dialog->AddLine("No unclosed sectors");
		else
		{
			sprintf(check_message, "%d unclosed sectors", sel.count_obj());

			dialog->AddLine(check_message, 2, 210,
			                "Show",  &UI_Check_Sectors::action_show_unclosed,
			                "Verts", &UI_Check_Sectors::action_show_un_verts);
		}


		Sectors_FindMismatches(sel, other);

		if (sel.empty())
			dialog->AddLine("No mismatched sectors");
		else
		{
			sprintf(check_message, "%d mismatched sectors", sel.count_obj());

			dialog->AddLine(check_message, 2, 210,
			                "Show",  &UI_Check_Sectors::action_show_mismatch,
			                "Lines", &UI_Check_Sectors::action_show_mis_lines);
		}


		Sectors_FindBadCeil(sel);

		if (sel.empty())
			dialog->AddLine("No sectors with ceil < floor");
		else
		{
			sprintf(check_message, "%d sectors with ceil < floor", sel.count_obj());

			dialog->AddLine(check_message, 2, 220,
			                "Show", &UI_Check_Sectors::action_show_ceil,
			                "Fix",  &UI_Check_Sectors::action_fix_ceil);
		}

		dialog->AddGap(10);


		Sectors_FindUnknown(sel, types);

		if (sel.empty())
			dialog->AddLine("No unknown sector types");
		else
		{
			sprintf(check_message, "%d unknown sector types", (int)types.size());

			dialog->AddLine(check_message, 2, 220,
			                "Show",   &UI_Check_Sectors::action_show_unknown,
			                "Log",    &UI_Check_Sectors::action_log_unknown,
			                "Clear",  &UI_Check_Sectors::action_clear_unknown);
		}


		SideDefs_FindPacking(sel, other);

		if (sel.empty())
			dialog->AddLine("No shared sidedefs");
		else
		{
			int approx_num = sel.count_obj();

			sprintf(check_message, "%d shared sidedefs", approx_num);

			dialog->AddLine(check_message, 1, 200,
			                "Show",   &UI_Check_Sectors::action_show_packed,
			                "Unpack", &UI_Check_Sectors::action_unpack);
		}


		Sectors_FindUnused(sel);

		if (sel.empty())
			dialog->AddLine("No unused sectors");
		else
		{
			sprintf(check_message, "%d unused sectors", sel.count_obj());

			dialog->AddLine(check_message, 1, 170,
			                "Remove", &UI_Check_Sectors::action_remove);
		}


		SideDefs_FindUnused(sel);

		if (sel.empty())
			dialog->AddLine("No unused sidedefs");
		else
		{
			sprintf(check_message, "%d unused sidedefs", sel.count_obj());

			dialog->AddLine(check_message, 1, 170,
			                "Remove", &UI_Check_Sectors::action_remove_sidedefs);
		}


		// in "ALL" mode, just continue if not too severe
		if (dialog->WorstSeverity() < min_severity)
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


//------------------------------------------------------------------------

void Things_FindUnknown(selection_c& list, std::map<int, int>& types)
{
	types.clear();

	list.change_type(OBJ_THINGS);

	for (int n = 0 ; n < NumThings ; n++)
	{
		const thingtype_t *info = M_GetThingType(Things[n]->type);

		if (strncmp(info->desc, "UNKNOWN", 7) == 0)
		{
			bump_unknown_type(types, Things[n]->type);

			list.set(n);
		}
	}
}


void Things_ShowUnknown()
{
	if (edit.mode != OBJ_THINGS)
		Editor_ChangeMode('t');

	std::map<int, int> types;

	Things_FindUnknown(*edit.Selected, types);

	GoToErrors();
}


void Things_LogUnknown()
{
	selection_c sel;

	std::map<int, int> types;
	std::map<int, int>::iterator IT;

	Things_FindUnknown(sel, types);

	LogPrintf("\n");
	LogPrintf("Unknown Things:\n");
	LogPrintf("{\n");

	for (IT = types.begin() ; IT != types.end() ; IT++)
		LogPrintf("  %5d  x %d\n", IT->first, IT->second);

	LogPrintf("}\n");

	LogViewer_Open();
}


void Things_RemoveUnknown()
{
	selection_c sel;

	std::map<int, int> types;

	Things_FindUnknown(sel, types);

	BA_Begin();
	DeleteObjects(&sel);
	BA_End();
}


// this returns a bitmask : bits 0..3 for players 1..4
int Things_FindStarts(int *dm_num)
{
	*dm_num = 0;

	int mask = 0;

	for (int n = 0 ; n < NumThings ; n++)
	{
		const Thing * T = Things[n];

		// ideally, these type numbers would not be hard-coded....

		switch (T->type)
		{
			case 1: mask |= (1 << 0); break;
			case 2: mask |= (1 << 1); break;
			case 3: mask |= (1 << 2); break;
			case 4: mask |= (1 << 3); break;

			case 11: *dm_num += 1; break;
		}
	}

	return mask;
}


void Things_FindInVoid(selection_c& list)
{
	list.change_type(OBJ_THINGS);

	for (int n = 0 ; n < NumThings ; n++)
	{
		int x = Things[n]->x;
		int y = Things[n]->y;

		Objid obj;

		GetNearObject(obj, OBJ_SECTORS, x, y);

		if (! obj.is_nil())
			continue;

		// allow certain things in the void (Heretic sounds)
		const thingtype_t *info = M_GetThingType(Things[n]->type);

		if (info->flags & THINGDEF_VOID)
			continue;

		// check more coords around the thing's centre, to be sure
		int out_count = 0;

		for (int corner = 0 ; corner < 4 ; corner++)
		{
			int x2 = x + ((corner & 1) ? -4 : +4);
			int y2 = y + ((corner & 2) ? -4 : +4);

			GetNearObject(obj, OBJ_SECTORS, x2, y2);

			if (obj.is_nil())
				out_count++;
		}

		if (out_count == 4)
			list.set(n);
	}
}


void Things_ShowInVoid()
{
	if (edit.mode != OBJ_THINGS)
		Editor_ChangeMode('t');

	Things_FindInVoid(*edit.Selected);

	GoToErrors();
}


void Things_RemoveInVoid()
{
	selection_c sel;

	Things_FindInVoid(sel);

	BA_Begin();
	DeleteObjects(&sel);
	BA_End();
}


//------------------------------------------------------------------------

static void CollectBlockingThings(std::vector<int>& list,
                                  std::vector<int>& sizes)
{
	for (int n = 0 ; n < NumThings ; n++)
	{
		const Thing *T = Things[n];

		const thingtype_t *info = M_GetThingType(T->type);

		if (info->flags & THINGDEF_PASS)
			continue;

		// ignore unknown things
		if (strncmp(info->desc, "UNKNOWN", 7) == 0)
			continue;

		// TODO: config option: treat ceiling things as non-blocking

		 list.push_back(n);
		sizes.push_back(info->radius);
	}
}


/*
   andrewj: the DOOM movement code for monsters works by moving
   the actor by a stepping distance which is based on its 'speed'
   value.  The move is allowed when the *new position* has no
   blocking things or walls, which means that things can overlap
   a short distance and won't be stuck.
  
   Properly taking this into account requires knowing the speed of
   each individual monster, but we don't have that information here.
   Hence I've chosen a conservative value based on the speed of the
   slowest monster (8 units).
   
   TODO: make it either game config or user preference.
*/
#define MONSTER_STEP_DIST  8


static bool ThingStuckInThing(const Thing *T1, const thingtype_t *info1,
							  const Thing *T2, const thingtype_t *info2)
{
	SYS_ASSERT(T1 != T2);

	// require one thing to be a monster or player
	bool is_actor1 = (info1->group == 'm' || info1->group == 'p');
	bool is_actor2 = (info2->group == 'm' || info2->group == 'p');

	if (! (is_actor1 || is_actor2))
		return false;

	// check if T1 is stuck in T2
	int r1 = info1->radius;
	int r2 = info2->radius;

	if (info1->group == 'm' && info2->group != 'p')
		r1 = MAX(4, r1 - MONSTER_STEP_DIST);

	else if (info2->group == 'm' && info1->group != 'p')
		r2 = MAX(4, r2 - MONSTER_STEP_DIST);

	if (T1->x - r1 >= T2->x + r2) return false;
	if (T1->y - r1 >= T2->y + r2) return false;

	if (T1->x + r1 <= T2->x - r2) return false;
	if (T1->y + r1 <= T2->y - r2) return false;

	// teleporters and DM starts can safely overlap moving actors
	if ((info1->flags & THINGDEF_TELEPT) && is_actor2) return false;
	if ((info2->flags & THINGDEF_TELEPT) && is_actor1) return false;

	// check skill bits, except for players
	int opt1 = T1->options;
	int opt2 = T2->options;

	if (Level_format == MAPF_Hexen)
	{
		if (info1->group == 'p') opt1 |= 0x7E7;
		if (info2->group == 'p') opt2 |= 0x7E7;

		// check skill bits
		if ((opt1 & opt2 & 0x07) == 0) return false;

		// check class bits
		if ((opt1 & opt2 & 0xE0) == 0) return false;

		// check game mode
		if ((opt1 & opt2 & 0x700) == 0) return false;
	}
	else
	{
		// invert game-mode bits (MTF_Not_COOP etc)
		opt1 ^= 0x70; opt2 ^= 0x70;

		if (info1->group == 'p') opt1 |= 0x77;
		if (info2->group == 'p') opt2 |= 0x77;

		// check skill bits
		if ((opt1 & opt2 & 0x07) == 0) return false;

		// check game mode
		if ((opt1 & opt2 & 0x70) == 0) return false;
	}

	return true;
}


static inline bool LD_is_blocking(const LineDef *L)
{
#define MONSTER_HEIGHT  36

	// ignore virtual linedefs
	if (L->right < 0 && L->left < 0)
		return false;
	
	if (L->right < 0 || L->left < 0)
		return true;
	
	const Sector *S1 = L->Right()->SecRef();
	const Sector *S2 = L-> Left()->SecRef();

	int f_max = MAX(S1->floorh, S2->floorh);
	int c_min = MIN(S1-> ceilh, S2-> ceilh);

	return (c_min < f_max + MONSTER_HEIGHT);
}


static bool ThingStuckInWall(const Thing *T, int r, char group)
{
	// only check players and monsters
	if (! (group == 'p' || group == 'm'))
		return false;

	if (group == 'm')
		r = MAX(4, r - MONSTER_STEP_DIST);

	// shrink a tiny bit, because we need to find lines which CROSS the
	// bounding box, not just touch it.
	r = r - 1;

	int x1 = T->x - r;
	int y1 = T->y - r;
	int x2 = T->x + r;
	int y2 = T->y + r;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (! LD_is_blocking(L))
			continue;

		if (LineTouchesBox(n, x1, y1, x2, y2))
			return true;
	}

	return false;
}


void Things_FindStuckies(selection_c& list)
{
	list.change_type(OBJ_THINGS);

	std::vector<int> blockers;
	std::vector<int> sizes;

	CollectBlockingThings(blockers, sizes);

	for (int n = 0 ; n < (int)blockers.size() ; n++)
	{
		const Thing *T = Things[blockers[n]];

		const thingtype_t *info = M_GetThingType(T->type);

		if (ThingStuckInWall(T, info->radius, info->group))
			list.set(blockers[n]);

		for (int n2 = n + 1 ; n2 < (int)blockers.size() ; n2++)
		{
			const Thing *T2 = Things[blockers[n2]];

			const thingtype_t *info2 = M_GetThingType(T2->type);

			if (ThingStuckInThing(T, info, T2, info2))
				list.set(blockers[n]);
		}
	}
}


void Things_ShowStuckies()
{
	if (edit.mode != OBJ_THINGS)
		Editor_ChangeMode('t');

	Things_FindStuckies(*edit.Selected);

	GoToErrors();
}


//------------------------------------------------------------------------

class UI_Check_Things : public UI_Check_base
{
public:
	UI_Check_Things(bool all_mode) :
		UI_Check_base(520, 286, all_mode, "Check : Things",
				      "Thing test results")
	{ }

public:
	static void action_show_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_ShowUnknown();
		dialog->user_action = CKR_Highlight;
	}

	static void action_log_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_LogUnknown();
		dialog->user_action = CKR_Highlight;
	}

	static void action_remove_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_RemoveUnknown();
		dialog->user_action = CKR_TookAction;
	}


	static void action_show_void(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_ShowInVoid();
		dialog->user_action = CKR_Highlight;
	}

	static void action_remove_void(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_RemoveInVoid();
		dialog->user_action = CKR_TookAction;
	}

	static void action_show_stuck(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_ShowStuckies();
		dialog->user_action = CKR_Highlight;
	}
};


check_result_e CHECK_Things(int min_severity = 0)
{
	UI_Check_Things *dialog = new UI_Check_Things(min_severity > 0);

	selection_c  sel;

	std::map<int, int> types;

	for (;;)
	{
		Things_FindUnknown(sel, types);

		if (sel.empty())
			dialog->AddLine("No unknown thing types");
		else
		{
			sprintf(check_message, "%d unknown things", (int)types.size());

			dialog->AddLine(check_message, 2, 200,
			                "Show",   &UI_Check_Things::action_show_unknown,
			                "Log",    &UI_Check_Things::action_log_unknown,
			                "Remove", &UI_Check_Things::action_remove_unknown);
		}


		Things_FindStuckies(sel);

		if (sel.empty())
			dialog->AddLine("No stuck actors");
		else
		{
			sprintf(check_message, "%d stuck actors", sel.count_obj());

			dialog->AddLine(check_message, 2, 200,
			                "Show",  &UI_Check_Things::action_show_stuck);
		}


		Things_FindInVoid(sel);

		if (sel.empty())
			dialog->AddLine("No things in the void");
		else
		{
			sprintf(check_message, "%d things in the void", sel.count_obj());

			dialog->AddLine(check_message, 1, 200,
			                "Show",   &UI_Check_Things::action_show_void,
			                "Remove", &UI_Check_Things::action_remove_void);
		}


		dialog->AddGap(10);


		int dm_num, mask;

		mask = Things_FindStarts(&dm_num);

		if (! (mask & 1))
			dialog->AddLine("Player 1 start is missing!", 2);
		else if (! (mask & 2))
			dialog->AddLine("Player 2 start is missing", 1);
		else if (! (mask & 4))
			dialog->AddLine("Player 3 start is missing", 1);
		else if (! (mask & 8))
			dialog->AddLine("Player 4 start is missing", 1);
		else
			dialog->AddLine("Found all 4 player starts");

		if (dm_num == 0)
			dialog->AddLine("Map is missing deathmatch starts", 1);
		else if (dm_num < game_info.min_dm_starts)
		{
			sprintf(check_message, "Found %d deathmatch starts -- need at least %d", dm_num,
			        game_info.min_dm_starts);
			dialog->AddLine(check_message, 1);
		}
		else if (dm_num > game_info.max_dm_starts)
		{
			sprintf(check_message, "Found %d deathmatch starts -- maximum is %d", dm_num,
			        game_info.max_dm_starts);
			dialog->AddLine(check_message, 2);
		}
		else
		{
			sprintf(check_message, "Found %d deathmatch starts -- OK", dm_num);
			dialog->AddLine(check_message);
		}


		// in "ALL" mode, just continue if not too severe
		if (dialog->WorstSeverity() < min_severity)
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


//------------------------------------------------------------------------


void CHECK_All(bool major_stuff)
{
	bool no_worries = true;

	int min_severity = major_stuff ? 2 : 1;

	check_result_e result;


	result = CHECK_Vertices(min_severity);
	if (result == CKR_Highlight) return;
	if (result != CKR_OK) no_worries = false;

	result = CHECK_Sectors(min_severity);
	if (result == CKR_Highlight) return;
	if (result != CKR_OK) no_worries = false;

	result = CHECK_LineDefs(min_severity);
	if (result == CKR_Highlight) return;
	if (result != CKR_OK) no_worries = false;

	result = CHECK_Things(min_severity);
	if (result == CKR_Highlight) return;
	if (result != CKR_OK) no_worries = false;

	result = CHECK_Textures(min_severity);
	if (result == CKR_Highlight) return;
	if (result != CKR_OK) no_worries = false;

	result = CHECK_Tags(min_severity);
	if (result == CKR_Highlight) return;
	if (result != CKR_OK) no_worries = false;


	if (no_worries)
	{
		DLG_Notify(major_stuff ? "No major problems." :
		                         "All tests were successful.");
	}
}


//------------------------------------------------------------------------


void CMD_CheckMap()
{
	const char *what = EXEC_Param[0];

	if (! what[0])
	{
		Beep("Check: missing keyword");
		return;
	}
	else if (y_stricmp(what, "all") == 0)
	{
		CHECK_All(false);
	}
	else if (y_stricmp(what, "major") == 0)
	{
		CHECK_All(true);
	}
	else if (y_stricmp(what, "vertices") == 0)
	{
		CHECK_Vertices();
	}
	else if (y_stricmp(what, "sectors") == 0)
	{
		CHECK_Sectors();
	}
	else if (y_stricmp(what, "linedefs") == 0)
	{
		CHECK_LineDefs();
	}
	else if (y_stricmp(what, "things") == 0)
	{
		CHECK_Things();
	}
	else if (y_stricmp(what, "current") == 0)  // current editing mode
	{
		switch (edit.mode)
		{
			case OBJ_VERTICES:
				CHECK_Vertices();
				break;

			case OBJ_SECTORS:
				CHECK_Sectors();
				break;

			case OBJ_LINEDEFS:
				CHECK_LineDefs();
				break;

			case OBJ_THINGS:
				CHECK_Things();
				break;

			default:
				Beep("Nothing to check");
				break;
		}
	}
	else if (y_stricmp(what, "textures") == 0)
	{
		CHECK_Textures();
	}
	else if (y_stricmp(what, "tags") == 0)
	{
		CHECK_Tags();
	}
	else
	{
		Beep("Check: unknown keyword: %s\n", what);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
