//------------------------------------------------------------------------
//  INTEGRITY CHECKS II
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
#include "e_cutpaste.h"
#include "editloop.h"
#include "m_game.h"
#include "levels.h"
#include "objects.h"
#include "w_rawdef.h"
#include "w_texture.h"
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
	BA_Message("removed zero-len linedefs");

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


static bool CheckLinesCross(int A, int B)
{
	SYS_ASSERT(A != B);

	const LineDef *AL = LineDefs[A];
	const LineDef *BL = LineDefs[B];

	// ignore zero-length lines
	if (AL->isZeroLength() || BL->isZeroLength())
		return false;

	// ignore lines connected at a vertex
	// TODO: check for sitting on top
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


	// TODO: lines are (roughly) co-linear, check for separation


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

			dialog->AddLine(check_buffer, 2, 250,
			                "Show", &UI_Check_LineDefs::action_show_mis_right);
		}


		LineDefs_FindManualDoors(sel);

		if (sel.empty())
			dialog->AddLine("No manual doors on 1S linedefs");
		else
		{
			sprintf(check_buffer, "%d manual doors on 1S linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 250,
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

			dialog->AddLine(check_buffer, 1, 285,
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


//------------------------------------------------------------------------

class UI_Check_Tags : public UI_Check_base
{
public:
	int fresh_tag;

public:
	UI_Check_Tags(bool all_mode) :
		UI_Check_base(520, 286, all_mode, "Check : Tags",
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

			dialog->AddGap(10);
			dialog->AddLine("Apply fresh tag to selection :", 0, 215, "Apply",
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


// FIXME:  is_unknown_tex   (allow '-' and '#....')


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


		if (game_info.medusa_bug)
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

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
