//------------------------------------------------------------------------
//  INTEGRITY CHECKS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2018 Andrew Apted
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
#include "e_cutpaste.h"
#include "e_hover.h"
#include "e_main.h"
#include "e_path.h"
#include "e_vertex.h"
#include "m_game.h"
#include "e_objects.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "ui_window.h"


#define   ERROR_MSG_COLOR	FL_RED
#define WARNING_MSG_COLOR	FL_BLUE


#define CAMERA_PEST  32000


static char check_message[MSG_BUF_LEN];
static char check_buffer [MSG_BUF_LEN];


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


void Vertex_FindOverlaps(selection_c& sel)
{
	// NOTE: when two or more vertices share the same coordinates,
	//       only the second and subsequent ones are stored in 'sel'.

	sel.change_type(OBJ_VERTICES);

	if (NumVertices < 2)
		return;

	// sort the vertices into order of the 'X' value.
	// hence any overlapping vertices will be near each other.

	std::vector<int> sorted_list(NumVertices, 0);

	for (int i = 0 ; i < NumVertices ; i++)
		sorted_list[i] = i;

	std::sort(sorted_list.begin(), sorted_list.end(), vertex_X_CMP_pred());

#define VERT_K  Vertices[sorted_list[k]]
#define VERT_N  Vertices[sorted_list[n]]

	for (int k = 0 ; k < NumVertices ; k++)
	{
		for (int n = k + 1 ; n < NumVertices && VERT_N->x == VERT_K->x ; n++)
		{
			if (VERT_N->y == VERT_K->y)
			{
				sel.set(sorted_list[k]);
			}
		}
	}

#undef VERT_K
#undef VERT_N
}


static void Vertex_MergeOne(int idx, selection_c& merge_verts)
{
	const Vertex *V = Vertices[idx];

	// find the base vertex (the one V is sitting on)
	for (int n = 0 ; n < NumVertices ; n++)
	{
		if (n == idx)
			continue;

		// skip any in the merge list
		if (merge_verts.get(n))
			continue;

		const Vertex *N = Vertices[n];

		if (! N->Matches(V))
			continue;

		// Ok, found it, so update linedefs

		for (int ld = 0 ; ld < NumLineDefs ; ld++)
		{
			LineDef *L = LineDefs[ld];

			if (L->start == idx)
				BA_ChangeLD(ld, LineDef::F_START, n);

			if (L->end == idx)
				BA_ChangeLD(ld, LineDef::F_END, n);
		}

		return;
	}

	// SHOULD NOT GET HERE
	LogPrintf("VERTEX MERGE FAILURE.\n");
}


void Vertex_MergeOverlaps()
{
	selection_c verts;
	selection_iterator_c it;

	Vertex_FindOverlaps(verts);

	BA_Begin();
	BA_Message("merged overlapping vertices");

	for (verts.begin(&it) ; !it.at_end() ; ++it)
	{
		Vertex_MergeOne(*it, verts);
	}

	// nothing should reference these vertices now
	DeleteObjects(&verts);

	BA_End();

	RedrawMap();
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
	BA_Message("removed unused vertices");

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
		Vertex_FindOverlaps(sel);

		if (sel.empty())
			dialog->AddLine("No overlapping vertices");
		else
		{
			sprintf(check_message, "%d overlapping vertices", sel.count_obj());

			dialog->AddLine(check_message, 2, 210,
			                "Show",  &UI_Check_Vertices::action_highlight,
			                "Merge", &UI_Check_Vertices::action_merge);
		}


		Vertex_FindDanglers(sel);

		if (sel.empty())
			dialog->AddLine("No dangling vertices");
		else
		{
			sprintf(check_message, "%d dangling vertices", sel.count_obj());

			dialog->AddLine(check_message, 2, 210,
			                "Show",  &UI_Check_Vertices::action_show_danglers);
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
	//
	// Note from RQ:
	// This is a very simple idea, but it works!  The first test (above)
	// checks that all Sectors are closed.  But if a closed set of LineDefs
	// is moved out of a Sector and has all its "external" SideDefs pointing
	// to that Sector instead of the new one, then we need a second test.
	// That's why I check if the SideDefs facing each other are bound to
	// the same Sector.
	//
	// Other note from RQ:
	// Nowadays, what makes the power of a good editor is its automatic tests.
	// So, if you are writing another Doom editor, you will probably want
	// to do the same kind of tests in your program.  Fine, but if you use
	// these ideas, don't forget to credit DEU...  Just a reminder... :-)
	//

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

	int max_type = (game_info.gen_sectors == 2) ? 8191 : 2047;

	for (int n = 0 ; n < NumSectors ; n++)
	{
		int type_num = Sectors[n]->type;

		// always ignore type #0
		if (type_num == 0)
			continue;

		if (type_num < 0 || type_num > max_type)
		{
			bump_unknown_type(types, type_num);
			list.set(n);
			continue;
		}

		// Boom and ZDoom generalized sectors
		if (game_info.gen_sectors == 2)
			type_num &= 255;
		else if (game_info.gen_sectors)
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
	BA_Message("cleared unknown sector types");

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
	BA_Message("removed unused sectors");

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
	BA_Message("fixed bad sector heights");

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
	BA_Message("removed unused sidedefs");

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


void SideDefs_Unpack(bool is_after_load)
{
	selection_c sides;
	selection_c lines;

	SideDefs_FindPacking(sides, lines);

	if (sides.empty())
		return;

	if ((false) /* confirm_it */)
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

	if (is_after_load)
		BA_Abort(true /* keep changes */);
	else
	{
		BA_Message("unpacked all sidedefs");
		BA_End();
	}

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

			dialog->AddLine(check_message, 2, 220,
			                "Show",  &UI_Check_Sectors::action_show_unclosed,
			                "Verts", &UI_Check_Sectors::action_show_un_verts);
		}


		Sectors_FindMismatches(sel, other);

		if (sel.empty())
			dialog->AddLine("No mismatched sectors");
		else
		{
			sprintf(check_message, "%d mismatched sectors", sel.count_obj());

			dialog->AddLine(check_message, 2, 220,
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
	BA_Message("removed unknown things");

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
	BA_Message("removed things in the void");

	DeleteObjects(&sel);

	BA_End();
}


// returns true if the game engine ALWAYS spawns this thing
// (i.e. the skill-flags and mode-flags are ignored).
static bool TH_always_spawned(int type)
{
	const thingtype_t *info = M_GetThingType(type);

	// a player?
	if (1 <= type && type <= 4)
		return true;

	// a deathmatch start?
	if (type == 11)
		return true;

	// Polyobject things
	if (strstr(info->desc, "Polyobj") != NULL ||
		strstr(info->desc, "PolyObj") != NULL)
		return true;

	// ambient sounds in Heretic and Hexen
	if (strstr(info->desc, "Snd") != NULL ||
		strstr(info->desc, "Sound") != NULL)
		return true;

	return false;
}


void Things_FindDuds(selection_c& list)
{
	list.change_type(OBJ_THINGS);

	for (int n = 0 ; n < NumThings ; n++)
	{
		const Thing *T = Things[n];

		if (T->type == CAMERA_PEST)
			continue;

		int skills  = T->options & (MTF_Easy | MTF_Medium | MTF_Hard);
		int modes   = 1;
		int classes = 1;

		if (Level_format == MAPF_Hexen)
		{
			modes = T->options & (MTF_Hexen_SP | MTF_Hexen_COOP | MTF_Hexen_DM);
		}
		else if (game_info.coop_dm_flags)
		{
			modes = (~T->options) & (MTF_Not_SP | MTF_Not_COOP | MTF_Not_DM);
		}

		if (Level_format == MAPF_Hexen)
		{
			classes = T->options & (MTF_Hexen_Cleric | MTF_Hexen_Fighter | MTF_Hexen_Mage);
		}

		if (skills == 0 || modes == 0 || classes == 0)
		{
			if (! TH_always_spawned(T->type))
				list.set(n);
		}
	}
}


void Things_ShowDuds()
{
	if (edit.mode != OBJ_THINGS)
		Editor_ChangeMode('t');

	Things_FindDuds(*edit.Selected);

	GoToErrors();
}


void Things_FixDuds()
{
	BA_Begin();
	BA_Message("fixed unspawnable things");

	for (int n = 0 ; n < NumThings ; n++)
	{
		const Thing *T = Things[n];

		// NOTE: we also "fix" things that are always spawned
		////   if (TH_always_spawned(T->type)) continue;

		if (T->type == CAMERA_PEST)
			continue;

		int new_options = T->options;

		int skills  = T->options & (MTF_Easy | MTF_Medium | MTF_Hard);
		int modes   = 1;
		int classes = 1;

		if (skills == 0)
			new_options |= MTF_Easy | MTF_Medium | MTF_Hard;

		if (Level_format == MAPF_Hexen)
		{
			modes = T->options & (MTF_Hexen_SP | MTF_Hexen_COOP | MTF_Hexen_DM);

			if (modes == 0)
				new_options |= MTF_Hexen_SP | MTF_Hexen_COOP | MTF_Hexen_DM;
		}
		else if (game_info.coop_dm_flags)
		{
			modes = (~T->options) & (MTF_Not_SP | MTF_Not_COOP | MTF_Not_DM);

			if (modes == 0)
				new_options &= ~(MTF_Not_SP | MTF_Not_COOP | MTF_Not_DM);
		}

		if (Level_format == MAPF_Hexen)
		{
			classes = T->options & (MTF_Hexen_Cleric | MTF_Hexen_Fighter | MTF_Hexen_Mage);

			if (classes == 0)
				new_options |= MTF_Hexen_Cleric | MTF_Hexen_Fighter | MTF_Hexen_Mage;
		}

		if (new_options != T->options)
		{
			BA_ChangeTH(n, Thing::F_OPTIONS, new_options);
		}
	}

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
		UI_Check_base(520, 316, all_mode, "Check : Things",
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


	static void action_show_duds(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_ShowDuds();
		dialog->user_action = CKR_Highlight;
	}

	static void action_fix_duds(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_FixDuds();
		dialog->user_action = CKR_TookAction;
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


		Things_FindDuds(sel);

		if (sel.empty())
			dialog->AddLine("No unspawnable things -- skill flags are OK");
		else
		{
			sprintf(check_message, "%d unspawnable things", sel.count_obj());
			dialog->AddLine(check_message, 1, 200,
			                "Show", &UI_Check_Things::action_show_duds,
			                "Fix",  &UI_Check_Things::action_fix_duds);
		}


		dialog->AddGap(10);


		int dm_num, mask;

		mask = Things_FindStarts(&dm_num);

		if (game_info.no_need_players)
			dialog->AddLine("Player starts not needed, no check done");
		else if (! (mask & 1))
			dialog->AddLine("Player 1 start is missing!", 2);
		else if (! (mask & 2))
			dialog->AddLine("Player 2 start is missing", 1);
		else if (! (mask & 4))
			dialog->AddLine("Player 3 start is missing", 1);
		else if (! (mask & 8))
			dialog->AddLine("Player 4 start is missing", 1);
		else
			dialog->AddLine("Found all 4 player starts");

		if (game_info.no_need_players)
		{
			// leave a blank space
		}
		else if (dm_num == 0)
		{
			dialog->AddLine("Map is missing deathmatch starts", 1);
		}
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


void LineDefs_FindZeroLen(selection_c& lines)
{
	lines.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
		if (LineDefs[n]->isZeroLength())
			lines.set(n);
}


void LineDefs_RemoveZeroLen()
{
	selection_c lines(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		if (LineDefs[n]->isZeroLength())
			lines.set(n);
	}

	BA_Begin();
	BA_Message("removed zero-len linedefs");

	// NOTE: the vertex overlapping test handles cases where the
	//       vertices of other lines joining a zero-length one
	//       need to be merged.

	DeleteObjects_WithUnused(&lines);

	BA_End();
}


void LineDefs_ShowZeroLen()
{
	if (edit.mode != OBJ_VERTICES)
		Editor_ChangeMode('v');

	selection_c sel;

	LineDefs_FindZeroLen(sel);

	ConvertSelection(&sel, edit.Selected);

	GoToErrors();
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

	GoToErrors();
}


void LineDefs_FindManualDoors(selection_c& lines)
{
	// find D1/DR manual doors on one-sided linedefs

	lines.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->type <= 0)
			continue;

		if (L->left >= 0)
			continue;

		const linetype_t *info = M_GetLineType(L->type);

		if (info->desc[0] == 'D' &&
			(info->desc[1] == '1' || info->desc[1] == 'R'))
		{
			lines.set(n);
		}
	}
}


void LineDefs_ShowManualDoors()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	LineDefs_FindManualDoors(*edit.Selected);

	GoToErrors();
}


void LineDefs_FixManualDoors()
{
	BA_Begin();
	BA_Message("fixed manual doors");

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->type <= 0 || L->left >= 0)
			continue;

		const linetype_t *info = M_GetLineType(L->type);

		if (info->desc[0] == 'D' &&
			(info->desc[1] == '1' || info->desc[1] == 'R'))
		{
			BA_ChangeLD(n, LineDef::F_TYPE, 0);
		}
	}

	BA_End();
}


void LineDefs_FindLackImpass(selection_c& lines)
{
	lines.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->OneSided() && (L->flags & MLF_Blocking) == 0)
			lines.set(n);
	}
}


void LineDefs_ShowLackImpass()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	LineDefs_FindLackImpass(*edit.Selected);

	GoToErrors();
}


void LineDefs_FixLackImpass()
{
	BA_Begin();
	BA_Message("fixed impassible flags");

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->OneSided() && (L->flags & MLF_Blocking) == 0)
		{
			int new_flags = L->flags | MLF_Blocking;

			BA_ChangeLD(n, LineDef::F_FLAGS, new_flags);
		}
	}

	BA_End();
}


void LineDefs_FindBad2SFlag(selection_c& lines)
{
	lines.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->OneSided() && (L->flags & MLF_TwoSided))
			lines.set(n);

		if (L->TwoSided() && ! (L->flags & MLF_TwoSided))
			lines.set(n);
	}
}


void LineDefs_ShowBad2SFlag()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	LineDefs_FindBad2SFlag(*edit.Selected);

	GoToErrors();
}


void LineDefs_FixBad2SFlag()
{
	BA_Begin();
	BA_Message("fixed two-sided flags");

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->OneSided() && (L->flags & MLF_TwoSided))
			BA_ChangeLD(n, LineDef::F_FLAGS, L->flags & ~MLF_TwoSided);

		if (L->TwoSided() && ! (L->flags & MLF_TwoSided))
			BA_ChangeLD(n, LineDef::F_FLAGS, L->flags | MLF_TwoSided);
	}

	BA_End();
}


static void bung_unknown_type(std::map<int, int>& t_map, int type)
{
	int count = 0;

	if (t_map.find(type) != t_map.end())
		count = t_map[type];

	t_map[type] = count + 1;
}


void LineDefs_FindUnknown(selection_c& list, std::map<int, int>& types)
{
	types.clear();

	list.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		int type_num = LineDefs[n]->type;

		// always ignore type #0
		if (type_num == 0)
			continue;

		const linetype_t *info = M_GetLineType(type_num);

		// Boom generalized line type?
		if (game_info.gen_types && is_genline(type_num))
			continue;

		if (strncmp(info->desc, "UNKNOWN", 7) == 0)
		{
			bung_unknown_type(types, type_num);

			list.set(n);
		}
	}
}


void LineDefs_ShowUnknown()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	std::map<int, int> types;

	LineDefs_FindUnknown(*edit.Selected, types);

	GoToErrors();
}


void LineDefs_LogUnknown()
{
	selection_c sel;

	std::map<int, int> types;
	std::map<int, int>::iterator IT;

	LineDefs_FindUnknown(sel, types);

	LogPrintf("\n");
	LogPrintf("Unknown Line Types:\n");
	LogPrintf("{\n");

	for (IT = types.begin() ; IT != types.end() ; IT++)
		LogPrintf("  %5d  x %d\n", IT->first, IT->second);

	LogPrintf("}\n");

	LogViewer_Open();
}


void LineDefs_ClearUnknown()
{
	selection_c sel;
	std::map<int, int> types;

	LineDefs_FindUnknown(sel, types);

	selection_iterator_c it;

	BA_Begin();
	BA_Message("cleared unknown line types");

	for (sel.begin(&it) ; !it.at_end() ; ++it)
		BA_ChangeLD(*it, LineDef::F_TYPE, 0);

	BA_End();
}


//------------------------------------------------------------------------


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


struct linedef_minx_CMP_pred
{
	inline bool operator() (int A, int B) const
	{
		const LineDef *AL = LineDefs[A];
		const LineDef *BL = LineDefs[B];

		int A_x1 = MIN(AL->Start()->x, AL->End()->x);
		int B_x1 = MIN(BL->Start()->x, BL->End()->x);

		return A_x1 < B_x1;
	}
};


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

		// ignore zero-length lines
		if (LineDefs[ld2]->isZeroLength())
			continue;

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

	GoToErrors();
}


void LineDefs_RemoveOverlaps()
{
	selection_c lines, unused_verts;

	LineDefs_FindOverlaps(lines);

	UnusedVertices(&lines, &unused_verts);

	BA_Begin();
	BA_Message("removed overlapping lines");

	DeleteObjects(&lines);
	DeleteObjects(&unused_verts);

	BA_End();
}


static int CheckLinesCross(int A, int B)
{
	// return values:
	//    0 : the lines do not cross
	//    1 : A is sitting on B (a 'T' junction)
	//    2 : B is sitting on A (a 'T' junction)
	//    3 : the lines cross each other (an 'X' junction)
	//    4 : the lines are co-linear and partially overlap

	const double epsilon = 0.02;


	SYS_ASSERT(A != B);

	const LineDef *AL = LineDefs[A];
	const LineDef *BL = LineDefs[B];

	// ignore zero-length lines
	if (AL->isZeroLength() || BL->isZeroLength())
		return 0;

	// ignore directly overlapping here
	if (linedef_pos_cmp(A, B) == 0)
		return 0;


	// bbox test
	//
	// the algorithm in LineDefs_FindCrossings() ensures that A and B
	// already overlap on the X axis.  hence only check Y axis here.

	if (MIN(AL->Start()->y, AL->End()->y) >
	    MAX(BL->Start()->y, BL->End()->y))
	{
		return 0;
	}

	if (MIN(BL->Start()->y, BL->End()->y) >
	    MAX(AL->Start()->y, AL->End()->y))
	{
		return 0;
	}


	// precise (but slower) intersection test

	int ax1 = AL->Start()->x;
	int ay1 = AL->Start()->y;
	int ax2 = AL->End()->x;
	int ay2 = AL->End()->y;

	int bx1 = BL->Start()->x;
	int by1 = BL->Start()->y;
	int bx2 = BL->End()->x;
	int by2 = BL->End()->y;

	double c = PerpDist(bx1, by1,  ax1, ay1, ax2, ay2);
	double d = PerpDist(bx2, by2,  ax1, ay1, ax2, ay2);

	int c_side = (c < -epsilon) ? -1 : (c > epsilon) ? +1 : 0;
	int d_side = (d < -epsilon) ? -1 : (d > epsilon) ? +1 : 0;

	if (c_side != 0 && c_side == d_side)
		return 0;

	double e = PerpDist(ax1, ay1,  bx1, by1, bx2, by2);
	double f = PerpDist(ax2, ay2,  bx1, by1, bx2, by2);

	int e_side = (e < -epsilon) ? -1 : (e > epsilon) ? +1 : 0;
	int f_side = (f < -epsilon) ? -1 : (f > epsilon) ? +1 : 0;

	if (e_side != 0 && e_side == f_side)
		return 0;


	// check whether the two lines definitely cross each other
	// at a single point (like an 'X' shape), or not.
	bool a_crossed = (c_side * d_side != 0);
	bool b_crossed = (e_side * f_side != 0);

	if (a_crossed && b_crossed)
		return 3;


	// are the two lines are co-linear (or very close to it) ?
	// if so, check the separation between them...
	if ((c_side == 0 && d_side == 0) ||
		(e_side == 0 && f_side == 0))
	{
		// choose longest line as the measuring stick
		if (AL->CalcLength() < BL->CalcLength())
		{
			std::swap(ax1, bx1);  std::swap(ax2, bx2);
			std::swap(ay1, by1);  std::swap(ay2, by2);

			// A, B, AL, BL should not be used from here on!
		}

		c = AlongDist(bx1, by1,  ax1, ay1, ax2, ay2);
		d = AlongDist(bx2, by2,  ax1, ay1, ax2, ay2);
		e = AlongDist(ax2, ay2,  ax1, ay1, ax2, ay2);	// just the length

		if (MAX(c, d) < epsilon)
			return 0;

		if (MIN(c, d) > e - epsilon)
			return 0;

		// colinear and partially overlapping
		return 4;
	}


	// this handles the case where the two linedefs meet at a vertex
	// but are not overlapping at all.
	if (! a_crossed && ! b_crossed)
		return 0;


	// in this case we have a 'T' junction, where the end-point of
	// one linedef is sitting along the other one.
	return a_crossed ? 2 : 1;
}


void LineDefs_FindCrossings(selection_c& lines)
{
	lines.change_type(OBJ_LINEDEFS);

	if (NumLineDefs < 2)
		return;

	int n;

	// sort linedefs by their position.  linedefs which cross will be
	// near each other in this list.
	std::vector<int> sorted_list(NumLineDefs, 0);

	for (n = 0 ; n < NumLineDefs ; n++)
		sorted_list[n] = n;

	std::sort(sorted_list.begin(), sorted_list.end(), linedef_minx_CMP_pred());


	for (n = 0 ; n < NumLineDefs ; n++)
	{
		int n2 = sorted_list[n];

		const LineDef *L1 = LineDefs[n2];

		int max_x = MAX(L1->Start()->x, L1->End()->x);

		for (int k = n + 1 ; k < NumLineDefs ; k++)
		{
			int k2 = sorted_list[k];

			const LineDef *L2 = LineDefs[k2];

			int min_x = MIN(L2->Start()->x, L2->End()->x);

			// stop when all remaining linedefs are to the right of L1
			if (min_x > max_x)
				break;

			int res = CheckLinesCross(n2, k2);

			if (res)
			{
				lines.set(n2);
				lines.set(k2);
			}
		}
	}
}


void LineDefs_ShowCrossings()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	LineDefs_FindCrossings(*edit.Selected);

	GoToErrors();
}


//------------------------------------------------------------------------

class UI_Check_LineDefs : public UI_Check_base
{
public:
	UI_Check_LineDefs(bool all_mode) :
		UI_Check_base(530, 370, all_mode, "Check : LineDefs",
		              "LineDef test results")
	{ }

public:
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


	static void action_show_manual_doors(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowManualDoors();
		dialog->user_action = CKR_Highlight;
	}

	static void action_fix_manual_doors(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_FixManualDoors();
		dialog->user_action = CKR_TookAction;
	}


	static void action_show_lack_impass(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowLackImpass();
		dialog->user_action = CKR_Highlight;
	}

	static void action_fix_lack_impass(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_FixLackImpass();
		dialog->user_action = CKR_TookAction;
	}


	static void action_show_bad_2s_flag(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowBad2SFlag();
		dialog->user_action = CKR_Highlight;
	}

	static void action_fix_bad_2s_flag(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_FixBad2SFlag();
		dialog->user_action = CKR_TookAction;
	}


	static void action_show_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowUnknown();
		dialog->user_action = CKR_Highlight;
	}

	static void action_log_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_LogUnknown();
		dialog->user_action = CKR_Highlight;
	}

	static void action_clear_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ClearUnknown();
		dialog->user_action = CKR_TookAction;
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

	static void action_show_crossing(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowCrossings();
		dialog->user_action = CKR_Highlight;
	}
};


check_result_e CHECK_LineDefs(int min_severity)
{
	UI_Check_LineDefs *dialog = new UI_Check_LineDefs(min_severity > 0);

	selection_c  sel, other;

	std::map<int, int> types;

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

			dialog->AddLine(check_buffer, 2, 220,
			                "Show",   &UI_Check_LineDefs::action_show_overlap,
			                "Remove", &UI_Check_LineDefs::action_remove_overlap);
		}


		LineDefs_FindCrossings(sel);

		if (sel.empty())
			dialog->AddLine("No criss-crossing linedefs");
		else
		{
			sprintf(check_buffer, "%d criss-crossing linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 220,
			                "Show", &UI_Check_LineDefs::action_show_crossing);
		}

		dialog->AddGap(10);


		LineDefs_FindUnknown(sel, types);

		if (sel.empty())
			dialog->AddLine("No unknown line types");
		else
		{
			sprintf(check_buffer, "%d unknown line types", (int)types.size());

			dialog->AddLine(check_buffer, 1, 210,
			                "Show",   &UI_Check_LineDefs::action_show_unknown,
			                "Log",    &UI_Check_LineDefs::action_log_unknown,
			                "Clear",  &UI_Check_LineDefs::action_clear_unknown);
		}


		LineDefs_FindMissingRight(sel);

		if (sel.empty())
			dialog->AddLine("No linedefs without a right side");
		else
		{
			sprintf(check_buffer, "%d linedefs without right side", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 300,
			                "Show", &UI_Check_LineDefs::action_show_mis_right);
		}


		LineDefs_FindManualDoors(sel);

		if (sel.empty())
			dialog->AddLine("No manual doors on 1S linedefs");
		else
		{
			sprintf(check_buffer, "%d manual doors on 1S linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 300,
			                "Show", &UI_Check_LineDefs::action_show_manual_doors,
			                "Fix",  &UI_Check_LineDefs::action_fix_manual_doors);
		}


		LineDefs_FindLackImpass(sel);

		if (sel.empty())
			dialog->AddLine("No non-blocking one-sided linedefs");
		else
		{
			sprintf(check_buffer, "%d non-blocking one-sided linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 300,
			                "Show", &UI_Check_LineDefs::action_show_lack_impass,
			                "Fix",  &UI_Check_LineDefs::action_fix_lack_impass);
		}


		LineDefs_FindBad2SFlag(sel);

		if (sel.empty())
			dialog->AddLine("No linedefs with wrong 2S flag");
		else
		{
			sprintf(check_buffer, "%d linedefs with wrong 2S flag", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 300,
			                "Show", &UI_Check_LineDefs::action_show_bad_2s_flag,
			                "Fix",  &UI_Check_LineDefs::action_fix_bad_2s_flag);
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

void Tags_UsedRange(int *min_tag, int *max_tag)
{
	int i;

	*min_tag = +999999;
	*max_tag = -999999;

	for (i = 0 ; i < NumLineDefs ; i++)
	{
		int tag = LineDefs[i]->tag;

		if (tag > 0)
		{
			*min_tag = MIN(*min_tag, tag);
			*max_tag = MAX(*max_tag, tag);
		}
	}

	for (i = 0 ; i < NumSectors ; i++)
	{
		int tag = Sectors[i]->tag;

		// ignore special tags
		if (game_info.tag_666 && (tag == 666 || tag == 667))
			continue;

		if (tag > 0)
		{
			*min_tag = MIN(*min_tag, tag);
			*max_tag = MAX(*max_tag, tag);
		}
	}

	// none at all?
	if (*min_tag > *max_tag)
	{
		*min_tag = *max_tag = 0;
	}
}


void Tags_ApplyNewValue(int new_tag)
{
	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		BA_MessageForSel("new tag for", &list);

		for (list.begin(&it); !it.at_end(); ++it)
		{
			if (edit.mode == OBJ_LINEDEFS)
				BA_ChangeLD(*it, LineDef::F_TAG, new_tag);
			else if (edit.mode == OBJ_SECTORS)
				BA_ChangeSEC(*it, Sector::F_TAG, new_tag);
		}

		BA_End();
	}
}


void CMD_ApplyTag()
{
	if (! (edit.mode == OBJ_SECTORS || edit.mode == OBJ_LINEDEFS))
	{
		Beep("ApplyTag: wrong mode");
		return;
	}

	bool do_last = false;

	const char *mode = EXEC_Param[0];

	if (mode[0] == 0 || y_stricmp(mode, "fresh") == 0)
	{
		// fresh tag
	}
	else if (y_stricmp(mode, "last") == 0)
	{
		do_last = true;
	}
	else
	{
		Beep("ApplyTag: unknown keyword: %s\n", mode);
		return;
	}


	bool unselect = false;

	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("ApplyTag: nothing selected");
			return;
		}

		edit.Selected->set(edit.highlight.num);
		unselect = true;
	}


	int min_tag, max_tag;

	Tags_UsedRange(&min_tag, &max_tag);

	int new_tag = max_tag + (do_last ? 0 : 1);

	if (new_tag <= 0)
	{
		Beep("No last tag");
		return;
	}
	else if (new_tag > 32767)
	{
		Beep("Out of tag numbers");
		return;
	}

	Tags_ApplyNewValue(new_tag);

	if (unselect)
		Selection_Clear(true);
}


static bool LD_tag_exists(int tag)
{
	for (int n = 0 ; n < NumLineDefs ; n++)
		if (LineDefs[n]->tag == tag)
			return true;

	return false;
}


static bool SEC_tag_exists(int tag)
{
	for (int s = 0 ; s < NumSectors ; s++)
		if (Sectors[s]->tag == tag)
			return true;

	return false;
}


void Tags_FindUnmatchedSectors(selection_c& secs)
{
	secs.change_type(OBJ_SECTORS);

	for (int s = 0 ; s < NumSectors ; s++)
	{
		int tag = Sectors[s]->tag;

		if (tag <= 0)
			continue;

		// DOOM and Heretic use tag #666 to open doors (etc) on the
		// death of boss monsters.
		if (game_info.tag_666 && (tag == 666 || tag == 667))
			continue;

		if (! LD_tag_exists(tag))
			secs.set(s);
	}
}


void Tags_FindUnmatchedLineDefs(selection_c& lines)
{
	lines.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->tag <= 0)
			continue;

		// TODO: handle special BOOM types (e.g. line-to-line teleporter)

		if (L->type <= 0)
			continue;

		if (! SEC_tag_exists(L->tag))
			lines.set(n);
	}
}


void Tags_ShowUnmatchedSectors()
{
	if (edit.mode != OBJ_SECTORS)
		Editor_ChangeMode('s');

	Tags_FindUnmatchedSectors(*edit.Selected);

	GoToErrors();
}


void Tags_ShowUnmatchedLineDefs()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	Tags_FindUnmatchedLineDefs(*edit.Selected);

	GoToErrors();
}


void Tags_FindMissingTags(selection_c& lines)
{
	lines.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->type <= 0)
			continue;

		if (L->tag > 0)
			continue;

		// use type description to determine if a tag is needed
		// e.g. D1, DR, --, and lowercase first letter all mean "no tag".

		// TODO: boom generalized manual doors (etc??)
		const linetype_t *info = M_GetLineType(L->type);

		char first = info->desc[0];

		if (first == 'D' || first == '-' || ('a' <= first && first <= 'z'))
			continue;

		lines.set(n);
	}
}


void Tags_ShowMissingTags()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	Tags_FindMissingTags(*edit.Selected);

	GoToErrors();
}


static bool SEC_check_beast_mark(int tag)
{
	if (! game_info.tag_666)
		return true;

	if (tag == 667)
	{
		// tag #667 can only be used on MAP07
		return (y_stricmp(Level_name, "MAP07") == 0);
	}

	if (tag == 666)
	{
		// for Heretic, the map must be an end-of-episode map: ExM8
		if (game_info.tag_666 == 2)
		{
			if (strlen(Level_name) != 4)
				return false;

			return (Level_name[3] == '8');
		}

		// for Doom, either need a particular map, or the presence
		// of a KEEN thing.
		if (y_stricmp(Level_name, "E1M8")  == 0 ||
			y_stricmp(Level_name, "E4M6")  == 0 ||
			y_stricmp(Level_name, "E4M8")  == 0 ||
			y_stricmp(Level_name, "MAP07") == 0)
		{
			return true;
		}

		for (int n = 0 ; n < NumThings ; n++)
		{
			const thingtype_t *info = M_GetThingType(Things[n]->type);

			if (y_stricmp(info->desc, "Commander Keen") == 0)
				return true;
		}

		return false;
	}

	return true; // Ok
}


void Tags_FindBeastMarks(selection_c& secs)
{
	secs.change_type(OBJ_SECTORS);

	for (int s = 0 ; s < NumSectors ; s++)
	{
		int tag = Sectors[s]->tag;

		if (! SEC_check_beast_mark(tag))
			secs.set(s);
	}
}


void Tags_ShowBeastMarks()
{
	if (edit.mode != OBJ_SECTORS)
		Editor_ChangeMode('s');

	Tags_FindBeastMarks(*edit.Selected);

	GoToErrors();
}


//------------------------------------------------------------------------

class UI_Check_Tags : public UI_Check_base
{
public:
	int fresh_tag;

public:
	UI_Check_Tags(bool all_mode) :
		UI_Check_base(520, 326, all_mode, "Check : Tags", "Tag test results"),
		fresh_tag(0)
	{ }

public:
	static void action_fresh_tag(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;

		// fresh_tag is set externally
		Tags_ApplyNewValue(dialog->fresh_tag);

		dialog->want_close = true;
	}

	static void action_show_unmatch_sec(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;
		Tags_ShowUnmatchedSectors();
		dialog->user_action = CKR_Highlight;
	}

	static void action_show_unmatch_line(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;
		Tags_ShowUnmatchedLineDefs();
		dialog->user_action = CKR_Highlight;
	}

	static void action_show_missing_tag(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;
		Tags_ShowMissingTags();
		dialog->user_action = CKR_Highlight;
	}

	static void action_show_beast_marks(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;
		Tags_ShowBeastMarks();
		dialog->user_action = CKR_Highlight;
	}
};


check_result_e CHECK_Tags(int min_severity)
{
	UI_Check_Tags *dialog = new UI_Check_Tags(min_severity > 0);

	selection_c  sel;

	for (;;)
	{
		Tags_FindMissingTags(sel);

		if (sel.empty())
			dialog->AddLine("No linedefs missing a needed tag");
		else
		{
			sprintf(check_buffer, "%d linedefs missing a needed tag", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 320,
			                "Show", &UI_Check_Tags::action_show_missing_tag);
		}


		Tags_FindUnmatchedLineDefs(sel);

		if (sel.empty())
			dialog->AddLine("No tagged linedefs w/o a matching sector");
		else
		{
			sprintf(check_buffer, "%d tagged linedefs w/o a matching sector", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 350,
			                "Show", &UI_Check_Tags::action_show_unmatch_line);
		}


		Tags_FindUnmatchedSectors(sel);

		if (sel.empty())
			dialog->AddLine("No tagged sectors w/o a matching linedef");
		else
		{
			sprintf(check_buffer, "%d tagged sectors w/o a matching linedef", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 350,
			                "Show", &UI_Check_Tags::action_show_unmatch_sec);
		}


		Tags_FindBeastMarks(sel);

		if (sel.empty())
			dialog->AddLine("No sectors with tag 666 or 667 used on the wrong map");
		else
		{
			sprintf(check_buffer, "%d sectors have an invalid 666/667 tag", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 350,
			                "Show", &UI_Check_Tags::action_show_beast_marks);
		}

		dialog->AddGap(10);


		int min_tag, max_tag;

		Tags_UsedRange(&min_tag, &max_tag);

		if (max_tag <= 0)
			dialog->AddLine("No tags are in use");
		else
		{
			sprintf(check_buffer, "Lowest tag: %d   Highest tag: %d", min_tag, max_tag);
			dialog->AddLine(check_buffer);
		}

		if ((edit.mode == OBJ_LINEDEFS || edit.mode == OBJ_SECTORS) &&
		    edit.Selected->notempty())
		{
			dialog->fresh_tag = max_tag + 1;

			// skip two special tag numbers
			if (dialog->fresh_tag == 666)
				dialog->fresh_tag = 670;

			dialog->AddGap(10);
			dialog->AddLine("Apply a fresh tag to the selection", 0, 250, "Apply",
			                &UI_Check_Tags::action_fresh_tag);
		}

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


static void bump_unknown_name(std::map<std::string, int>& list,
                              const char *name)
{
	std::string t_name = name;

	int count = 0;

	if (list.find(t_name) != list.end())
		count = list[t_name];

	list[t_name] = count + 1;
}


void Textures_FindMissing(selection_c& lines)
{
	lines.change_type(OBJ_LINEDEFS);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->right < 0)
			continue;

		if (L->OneSided())
		{
			if (is_null_tex(L->Right()->MidTex()))
				lines.set(n);
		}
		else  // Two Sided
		{
			const Sector *front = L->Right()->SecRef();
			const Sector *back  = L->Left() ->SecRef();

			if (front->floorh < back->floorh && is_null_tex(L->Right()->LowerTex()))
				lines.set(n);

			if (back->floorh < front->floorh && is_null_tex(L->Left()->LowerTex()))
				lines.set(n);

			// missing uppers are OK when between two sky ceilings
			if (is_sky(front->CeilTex()) && is_sky(back->CeilTex()))
				continue;

			if (front->ceilh > back->ceilh && is_null_tex(L->Right()->UpperTex()))
				lines.set(n);

			if (back->ceilh > front->ceilh && is_null_tex(L->Left()->UpperTex()))
				lines.set(n);
		}
	}
}


void Textures_ShowMissing()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	Textures_FindMissing(*edit.Selected);

	GoToErrors();
}


void Textures_FixMissing()
{
	int new_wall = BA_InternaliseString(default_wall_tex);

	BA_Begin();
	BA_Message("fixed missing textures");

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->right < 0)
			continue;

		if (L->OneSided())
		{
			if (is_null_tex(L->Right()->MidTex()))
				BA_ChangeSD(L->right, SideDef::F_MID_TEX, new_wall);
		}
		else  // Two Sided
		{
			const Sector *front = L->Right()->SecRef();
			const Sector *back  = L->Left() ->SecRef();

			if (front->floorh < back->floorh && is_null_tex(L->Right()->LowerTex()))
				BA_ChangeSD(L->right, SideDef::F_LOWER_TEX, new_wall);

			if (back->floorh < front->floorh && is_null_tex(L->Left()->LowerTex()))
				BA_ChangeSD(L->left, SideDef::F_LOWER_TEX, new_wall);

			// missing uppers are OK when between two sky ceilings
			if (is_sky(front->CeilTex()) && is_sky(back->CeilTex()))
				continue;

			if (front->ceilh > back->ceilh && is_null_tex(L->Right()->UpperTex()))
				BA_ChangeSD(L->right, SideDef::F_UPPER_TEX, new_wall);

			if (back->ceilh > front->ceilh && is_null_tex(L->Left()->UpperTex()))
				BA_ChangeSD(L->left, SideDef::F_UPPER_TEX, new_wall);
		}
	}

	BA_End();
}


static bool is_transparent(const char *tex)
{
	// ignore lack of texture here
	// [ technically "-" is the poster-child of transparency,
	//   but it is handled by the Missing Texture checks ]
	if (is_null_tex(tex))
		return false;

	Img_c *img = W_GetTexture(tex);
	if (! img)
		return false;

	// note : this is slow
	return img->has_transparent();
}


static int check_transparent(const char *tex,
                             std::map<std::string, int>& names)
{
	if (is_transparent(tex))
	{
		bump_unknown_name(names, tex);
		return 1;
	}

	return 0;
}


void Textures_FindTransparent(selection_c& lines,
                              std::map<std::string, int>& names)
{
	lines.change_type(OBJ_LINEDEFS);

	names.clear();

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->right < 0)
			continue;

		if (L->OneSided())
		{
			if (check_transparent(L->Right()->MidTex(), names))
				lines.set(n);
		}
		else  // Two Sided
		{
			// note : plain OR operator here to check all parts (do NOT want short-circuit)
			if (check_transparent(L->Right()->LowerTex(), names) |
				check_transparent(L->Right()->UpperTex(), names) |
				check_transparent(L-> Left()->LowerTex(), names) |
				check_transparent(L-> Left()->UpperTex(), names))
			{
				lines.set(n);
			}
		}
	}
}


void Textures_ShowTransparent()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	std::map<std::string, int> names;

	Textures_FindTransparent(*edit.Selected, names);

	GoToErrors();
}


void Textures_FixTransparent()
{
	int new_wall = BA_InternaliseString(default_wall_tex);

	BA_Begin();
	BA_Message("fixed transparent textures");

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->right < 0)
			continue;

		if (L->OneSided())
		{
			if (is_transparent(L->Right()->MidTex()))
				BA_ChangeSD(L->right, SideDef::F_MID_TEX, new_wall);
		}
		else  // Two Sided
		{
			if (is_transparent(L->Left()->LowerTex()))
				BA_ChangeSD(L->left, SideDef::F_LOWER_TEX, new_wall);

			if (is_transparent(L->Left()->UpperTex()))
				BA_ChangeSD(L->left, SideDef::F_UPPER_TEX, new_wall);

			if (is_transparent(L->Right()->LowerTex()))
				BA_ChangeSD(L->right, SideDef::F_LOWER_TEX, new_wall);

			if (is_transparent(L->Right()->UpperTex()))
				BA_ChangeSD(L->right, SideDef::F_UPPER_TEX, new_wall);
		}
	}

	BA_End();
}


void Textures_LogTransparent()
{
	selection_c sel;

	std::map<std::string, int> names;
	std::map<std::string, int>::iterator IT;

	Textures_FindTransparent(sel, names);

	LogPrintf("\n");
	LogPrintf("Transparent textures on solid walls:\n");
	LogPrintf("{\n");

	for (IT = names.begin() ; IT != names.end() ; IT++)
		LogPrintf("  %-9s x %d\n", IT->first.c_str(), IT->second);

	LogPrintf("}\n");

	LogViewer_Open();
}


static int check_medusa(const char *tex,
                        std::map<std::string, int>& names)
{
	if (is_null_tex(tex) || is_special_tex(tex))
		return 0;

	if (! W_TextureCausesMedusa(tex))
		return 0;

	bump_unknown_name(names, tex);
	return 1;
}


void Textures_FindMedusa(selection_c& lines,
                         std::map<std::string, int>& names)
{
	lines.change_type(OBJ_LINEDEFS);

	names.clear();

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->right < 0 || L->left < 0)
			continue;

		if (check_medusa(L->Right()->MidTex(), names) |  /* plain OR */
			check_medusa(L-> Left()->MidTex(), names))
		{
			lines.set(n);
		}
	}
}


void Textures_ShowMedusa()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	std::map<std::string, int> names;

	Textures_FindMedusa(*edit.Selected, names);

	GoToErrors();
}


void Textures_RemoveMedusa()
{
	int null_tex = BA_InternaliseString("-");

	std::map<std::string, int> names;

	BA_Begin();
	BA_Message("fixed medusa textures");

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->right < 0 || L->left < 0)
			continue;

		if (check_medusa(L->Right()->MidTex(), names))
		{
			BA_ChangeSD(L->right, SideDef::F_MID_TEX, null_tex);
		}

		if (check_medusa(L-> Left()->MidTex(), names))
		{
			BA_ChangeSD(L->left, SideDef::F_MID_TEX, null_tex);
		}
	}

	BA_End();
}


void Textures_LogMedusa()
{
	selection_c sel;

	std::map<std::string, int> names;
	std::map<std::string, int>::iterator IT;

	Textures_FindMedusa(sel, names);

	LogPrintf("\n");
	LogPrintf("Medusa effect textures:\n");
	LogPrintf("{\n");

	for (IT = names.begin() ; IT != names.end() ; IT++)
		LogPrintf("  %-9s x %d\n", IT->first.c_str(), IT->second);

	LogPrintf("}\n");

	LogViewer_Open();
}


void Textures_FindUnknownTex(selection_c& lines,
                             std::map<std::string, int>& names)
{
	lines.change_type(OBJ_LINEDEFS);

	names.clear();

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		for (int side = 0 ; side < 2 ; side++)
		{
			const SideDef *SD = side ? L->Left() : L->Right();

			if (! SD)
				continue;

			for (int part = 0 ; part < 3 ; part++)
			{
				const char *tex = (part == 0) ? SD->LowerTex() :
								  (part == 1) ? SD->UpperTex() : SD->MidTex();

				if (! W_TextureIsKnown(tex))
				{
					bump_unknown_name(names, tex);

					lines.set(n);
				}
			}
		}
	}
}


void Textures_FindUnknownFlat(selection_c& secs,
                              std::map<std::string, int>& names)
{
	secs.change_type(OBJ_SECTORS);

	names.clear();

	for (int s = 0 ; s < NumSectors ; s++)
	{
		const Sector *S = Sectors[s];

		for (int part = 0 ; part < 2 ; part++)
		{
			const char *flat = part ? S->CeilTex() : S->FloorTex();

			if (! W_FlatIsKnown(flat))
			{
				bump_unknown_name(names, flat);

				secs.set(s);
			}
		}
	}
}


void Textures_ShowUnknownTex()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	std::map<std::string, int> names;

	Textures_FindUnknownTex(*edit.Selected, names);

	GoToErrors();
}


void Textures_ShowUnknownFlat()
{
	if (edit.mode != OBJ_SECTORS)
		Editor_ChangeMode('s');

	std::map<std::string, int> names;

	Textures_FindUnknownFlat(*edit.Selected, names);

	GoToErrors();
}


void Textures_LogUnknown(bool do_flat)
{
	selection_c sel;

	std::map<std::string, int> names;
	std::map<std::string, int>::iterator IT;

	if (do_flat)
		Textures_FindUnknownFlat(sel, names);
	else
		Textures_FindUnknownTex(sel, names);

	LogPrintf("\n");
	LogPrintf("Unknown %s:\n", do_flat ? "Flats" : "Textures");
	LogPrintf("{\n");

	for (IT = names.begin() ; IT != names.end() ; IT++)
		LogPrintf("  %-9s x %d\n", IT->first.c_str(), IT->second);

	LogPrintf("}\n");

	LogViewer_Open();
}


void Textures_FixUnknownTex()
{
	int new_wall = BA_InternaliseString(default_wall_tex);

	int null_tex = BA_InternaliseString("-");

	BA_Begin();
	BA_Message("fixed unknown textures");

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		bool two_sided = L->TwoSided();

		for (int side = 0 ; side < 2 ; side++)
		{
			int sd_num = side ? L->left : L->right;

			if (sd_num < 0)
				continue;

			const SideDef *SD = SideDefs[sd_num];

			if (! W_TextureIsKnown(SD->LowerTex()))
				BA_ChangeSD(sd_num, SideDef::F_LOWER_TEX, new_wall);

			if (! W_TextureIsKnown(SD->UpperTex()))
				BA_ChangeSD(sd_num, SideDef::F_UPPER_TEX, new_wall);

			if (! W_TextureIsKnown(SD->MidTex()))
				BA_ChangeSD(sd_num, SideDef::F_MID_TEX, two_sided ? null_tex : new_wall);
		}
	}

	BA_End();
}


void Textures_FixUnknownFlat()
{
	int new_floor = BA_InternaliseString(default_floor_tex);
	int new_ceil  = BA_InternaliseString(default_ceil_tex);

	BA_Begin();
	BA_Message("fixed unknown flats");

	for (int s = 0 ; s < NumSectors ; s++)
	{
		const Sector *S = Sectors[s];

		if (! W_FlatIsKnown(S->FloorTex()))
			BA_ChangeSEC(s, Sector::F_FLOOR_TEX, new_floor);

		if (! W_FlatIsKnown(S->CeilTex()))
			BA_ChangeSEC(s, Sector::F_CEIL_TEX, new_ceil);
	}

	BA_End();
}


//------------------------------------------------------------------------

class UI_Check_Textures : public UI_Check_base
{
public:
	UI_Check_Textures(bool all_mode) :
		UI_Check_base(565, 286, all_mode, "Check : Textures",
		              "Texture test results")
	{ }

public:
	static void action_show_unk_tex(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowUnknownTex();
		dialog->user_action = CKR_Highlight;
	}

	static void action_log_unk_tex(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_LogUnknown(false);
		dialog->user_action = CKR_Highlight;
	}

	static void action_fix_unk_tex(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_FixUnknownTex();
		dialog->user_action = CKR_TookAction;
	}


	static void action_show_unk_flat(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowUnknownFlat();
		dialog->user_action = CKR_Highlight;
	}

	static void action_log_unk_flat(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_LogUnknown(true);
		dialog->user_action = CKR_Highlight;
	}

	static void action_fix_unk_flat(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_FixUnknownFlat();
		dialog->user_action = CKR_TookAction;
	}


	static void action_show_missing(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowMissing();
		dialog->user_action = CKR_Highlight;
	}

	static void action_fix_missing(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_FixMissing();
		dialog->user_action = CKR_TookAction;
	}


	static void action_show_transparent(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowTransparent();
		dialog->user_action = CKR_Highlight;
	}

	static void action_fix_transparent(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_FixTransparent();
		dialog->user_action = CKR_TookAction;
	}

	static void action_log_transparent(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_LogTransparent();
		dialog->user_action = CKR_Highlight;
	}


	static void action_show_medusa(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowMedusa();
		dialog->user_action = CKR_Highlight;
	}

	static void action_remove_medusa(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_RemoveMedusa();
		dialog->user_action = CKR_TookAction;
	}

	static void action_log_medusa(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_LogMedusa();
		dialog->user_action = CKR_Highlight;
	}
};


check_result_e CHECK_Textures(int min_severity)
{
	UI_Check_Textures *dialog = new UI_Check_Textures(min_severity > 0);

	selection_c  sel;

	std::map<std::string, int> names;

	for (;;)
	{
		Textures_FindUnknownTex(sel, names);

		if (sel.empty())
			dialog->AddLine("No unknown textures");
		else
		{
			sprintf(check_buffer, "%d unknown textures", (int)names.size());

			dialog->AddLine(check_buffer, 2, 200,
			                "Show", &UI_Check_Textures::action_show_unk_tex,
			                "Log",  &UI_Check_Textures::action_log_unk_tex,
			                "Fix",  &UI_Check_Textures::action_fix_unk_tex);
		}


		Textures_FindUnknownFlat(sel, names);

		if (sel.empty())
			dialog->AddLine("No unknown flats");
		else
		{
			sprintf(check_buffer, "%d unknown flats", (int)names.size());

			dialog->AddLine(check_buffer, 2, 200,
			                "Show", &UI_Check_Textures::action_show_unk_flat,
			                "Log",  &UI_Check_Textures::action_log_unk_flat,
			                "Fix",  &UI_Check_Textures::action_fix_unk_flat);
		}


		if (! game_info.medusa_fixed)
		{
			Textures_FindMedusa(sel, names);

			if (sel.empty())
				dialog->AddLine("No textures causing Medusa Effect");
			else
			{
				sprintf(check_buffer, "%d Medusa textures", (int)names.size());

				dialog->AddLine(check_buffer, 2, 200,
								"Show", &UI_Check_Textures::action_show_medusa,
								"Log",  &UI_Check_Textures::action_log_medusa,
								"Fix",  &UI_Check_Textures::action_remove_medusa);
			}
		}

		dialog->AddGap(10);


		Textures_FindMissing(sel);

		if (sel.empty())
			dialog->AddLine("No missing textures on walls");
		else
		{
			sprintf(check_buffer, "%d missing textures on walls", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 270,
			                "Show", &UI_Check_Textures::action_show_missing,
			                "Fix",  &UI_Check_Textures::action_fix_missing);
		}


		Textures_FindTransparent(sel, names);

		if (sel.empty())
			dialog->AddLine("No transparent textures on solids");
		else
		{
			sprintf(check_buffer, "%d transparent textures on solids", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 270,
			                "Show", &UI_Check_Textures::action_show_transparent,
			                "Fix",  &UI_Check_Textures::action_fix_transparent,
			                "Log",  &UI_Check_Textures::action_log_transparent);
		}


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


void CMD_MapCheck()
{
	const char *what = EXEC_Param[0];

	if (! what[0])
	{
		Beep("MapCheck: missing keyword");
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
		Beep("MapCheck: unknown keyword: %s\n", what);
	}
}


void Debug_CheckUnusedStuff()
{
	selection_c sel;

	Sectors_FindUnused(sel);

	int num = sel.count_obj();

	if (num > 0)
	{
		fl_beep();
		DLG_Notify("Operation left %d sectors unused.", num);

		Sectors_RemoveUnused();
		return;
	}

	SideDefs_FindUnused(sel);

	num = sel.count_obj();

	if (num > 0)
	{
		fl_beep();
		DLG_Notify("Operation left %d sidedefs unused.", num);

		SideDefs_RemoveUnused();
		return;
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
