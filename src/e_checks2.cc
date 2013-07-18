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

	GoToSelection();

	edit.error_mode = true;
	edit.RedrawMap = 1;
}


void LineDefs_FixLackImpass()
{
	BA_Begin();

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


static bool CheckLinesCross(int A, int B)
{
	SYS_ASSERT(A != B);

	const LineDef *AL = LineDefs[A];
	const LineDef *BL = LineDefs[B];

	// ignore zero-length lines
	if (AL->isZeroLength() || BL->isZeroLength())
		return false;

	// ignore lines connected at a vertex
	// FIXME: check for sitting on top
	if (AL->start == BL->start || AL->start == BL->end) return false;
	if (AL->end   == BL->start || AL->end   == BL->end) return false;

	// bbox test
	//
	// the algorithm in LineDefs_FindCrossings() ensures that A and B
	// already overlap on the X axis.  hence only check Y axis here.

	if (MIN(AL->Start()->y, AL->End()->y) >
	    MAX(BL->Start()->y, BL->End()->y))
	{
		return false;
	}

	if (MIN(BL->Start()->y, BL->End()->y) >
	    MAX(AL->Start()->y, AL->End()->y))
	{
		return false;
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


	const double DIST = 0.6;

	double c = PerpDist(bx1, by1,  ax1, ay1, ax2, ay2);
	double d = PerpDist(bx2, by2,  ax1, ay1, ax2, ay2);

	if (c < -DIST && d < -DIST) return false;
	if (c >  DIST && d >  DIST) return false;


	double e = PerpDist(ax1, ay1,  bx1, by1, bx2, by2);
	double f = PerpDist(ax2, ay2,  bx1, by1, bx2, by2);

	if (e < -DIST && f < -DIST) return false;
	if (e >  DIST && f >  DIST) return false;


	// FIXME: lines are (roughly) co-linear, check for separation


	return true;
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

			// ignore direct overlapping here
			if (linedef_pos_cmp(n2, k2) == 0)
				continue;

			if (CheckLinesCross(n2, k2))
			{
				// only the second (or third, etc) linedef is stored
				lines.set(k2);
lines.set(n2);
			}
		}
	}
}


void LineDefs_ShowCrossings()
{
	if (edit.mode != OBJ_LINEDEFS)
		Editor_ChangeMode('l');

	LineDefs_FindCrossings(*edit.Selected);

	GoToSelection();

	edit.error_mode = true;
	edit.RedrawMap = 1;
}


//------------------------------------------------------------------------


class UI_Check_LineDefs : public UI_Check_base
{
public:
	UI_Check_LineDefs(bool all_mode) :
		UI_Check_base(530, 336, all_mode, "Check : LineDefs",
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


		LineDefs_FindMissingRight(sel);

		if (sel.empty())
			dialog->AddLine("No linedefs without a right side");
		else
		{
			sprintf(check_buffer, "%d linedefs without right side", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 250,
			                "Show", &UI_Check_LineDefs::action_show_mis_right);
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

		for (list.begin(&it); !it.at_end(); ++it)
		{
			if (edit.mode == OBJ_LINEDEFS)
				BA_ChangeLD(*it, LineDef::F_TAG, new_tag);
			else if (edit.mode == OBJ_SECTORS)
				BA_ChangeSEC(*it, Sector::F_TAG, new_tag);
		}

		BA_End();
		MarkChanges();
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
		if (! edit.highlighted())
		{
			Beep("ApplyTag: nothing selected");
			return;
		}

		edit.Selected->set(edit.highlighted.num);
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
		edit.Selected->clear_all();
}


//------------------------------------------------------------------------

class UI_Check_Tags : public UI_Check_base
{
public:
	int fresh_tag;

public:
	UI_Check_Tags(bool all_mode) :
		UI_Check_base(520, 226, all_mode, "Check : Tags",
		              "Tag test results")
	{ }

public:
	static void action_fresh_tag(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;

		// fresh_tag is set externally
		Tags_ApplyNewValue(dialog->fresh_tag);

		dialog->want_close = true;
	}
};


check_result_e CHECK_Tags(int min_severity)
{
	UI_Check_Tags *dialog = new UI_Check_Tags(min_severity > 0);

	selection_c  sel;

	for (;;)
	{
		int min_tag, max_tag;

		Tags_UsedRange(&min_tag, &max_tag);

		if (max_tag <= 0)
			dialog->AddLine("No tags are in use");
		else
		{
			sprintf(check_buffer, "Lowest  tag: %d", min_tag);
			dialog->AddLine(check_buffer);

			sprintf(check_buffer, "Highest tag: %d", max_tag);
			dialog->AddLine(check_buffer);
		}

		if ((edit.mode == OBJ_LINEDEFS || edit.mode == OBJ_SECTORS) &&
		    edit.Selected->notempty())
		{
			dialog->fresh_tag = max_tag + 1;

			dialog->AddGap(10);
			dialog->AddLine("Apply fresh tag to selection :", 0, 215, "Apply",
			                &UI_Check_Tags::action_fresh_tag);
		}

		check_result_e result = dialog->Run();

		if (dialog->WorstSeverity() < min_severity)
		{
			delete dialog;

			return CKR_OK;
		}

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


check_result_e CHECK_Textures(int min_severity)
{
	// FIXME

	return CKR_OK;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
