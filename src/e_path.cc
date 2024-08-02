//------------------------------------------------------------------------
//  LINEDEF PATHS
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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include "LineDef.h"
#include "m_bitvec.h"
#include "e_main.h"
#include "e_objects.h"
#include "e_path.h"
#include "m_game.h"
#include "r_grid.h"
#include "r_render.h"
#include "Sector.h"
#include "w_rawdef.h"

#include "ui_window.h"
#include "ui_misc.h"

#include <assert.h>

typedef enum
{
	SLP_Normal = 0,

	SLP_SameTex  = (1 << 1),  // require lines have same textures
	SLP_OneSided = (1 << 2),  // only handle one-sided lines
}
select_lines_in_path_flag_e;


static bool MatchingTextures(const Document &doc, int index1, int index2)
{
	const auto L1 = doc.linedefs[index1];
	const auto L2 = doc.linedefs[index2];

	// lines with no sidedefs only match each other
	if (! doc.getRight(*L1) || ! doc.getRight(*L2))
		return doc.getRight(*L1) == doc.getRight(*L2);

	// determine texture to match from first line
	StringID texture;

	if (! L1->TwoSided())
	{
		texture = doc.getRight(*L1)->mid_tex;
	}
	else
	{
		int f_diff = doc.getSector(*doc.getLeft(*L1)).floorh - doc.getSector(*doc.getRight(*L1)).floorh;
		int c_diff = doc.getSector(*doc.getLeft(*L1)).ceilh  - doc.getSector(*doc.getRight(*L1)).ceilh;

		if (f_diff == 0 && c_diff != 0)
			texture = (c_diff > 0) ? doc.getLeft(*L1)->upper_tex : doc.getRight(*L1)->upper_tex;
		else
			texture = (f_diff < 0) ? doc.getLeft(*L1)->lower_tex : doc.getRight(*L1)->lower_tex;
	}

	// match texture with other line

	if (! L2->TwoSided())
	{
		return (doc.getRight(*L2)->mid_tex == texture);
	}
	else
	{
		int f_diff = doc.getSector(*doc.getLeft(*L2)).floorh - doc.getSector(*doc.getRight(*L2)).floorh;
		int c_diff = doc.getSector(*doc.getLeft(*L2)).ceilh  - doc.getSector(*doc.getRight(*L2)).ceilh;

		if (c_diff != 0)
			if (texture == ((c_diff > 0) ? doc.getLeft(*L2)->upper_tex : doc.getRight(*L2)->upper_tex))
				return true;

		if (f_diff != 0)
			if (texture == ((f_diff < 0) ? doc.getLeft(*L2)->lower_tex : doc.getRight(*L2)->lower_tex))
				return true;

		return false;
	}
}


static bool OtherLineDef(const Document &doc, int L, int V, int *L_other, int *V_other,
                  int match, int start_L)
{
	*L_other = -1;
	*V_other = -1;

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		if (n == L)
			continue;

		if ((match & SLP_OneSided) && !doc.linedefs[n]->OneSided())
			continue;

		for (int k = 0 ; k < 2 ; k++)
		{
			int v1 = doc.linedefs[n]->start;
			int v2 = doc.linedefs[n]->end;

			if (k == 1)
				std::swap(v1, v2);

			if (v1 != V)
				continue;

			if ((match & SLP_SameTex) && ! MatchingTextures(doc, start_L, n))
				continue;

			if (*L_other >= 0)  // There is a fork in the path. Stop here.
				return false;

			*L_other = n;
			*V_other = v2;
		}
	}

	return (*L_other >= 0);
}


//
// This routine looks for all linedefs other than 'L' which use
// the vertex 'V'.  If there are none or more than one, the search
// stops there and nothing else happens.  If there is exactly one,
// then it is added to the selection and we continue from the new
// linedef and vertex.
//
static void SelectLinesInHalfPath(const Document &doc, int L, int V, selection_c& seen, int match)
{
	int start_L = L;

	for (;;)
	{
		int L_other, V_other;

		// does not exist or is forky
		if (! OtherLineDef(doc, L, V, &L_other, &V_other, match, start_L))
			break;

		// already seen?
		if (seen.get(L_other))
			break;

		seen.set(L_other);

		L = L_other;
		V = V_other;
	}
}


//
// select/unselect all linedefs in a non-forked path.
//
void Instance::CMD_LIN_SelectPath()
{
	// determine starting linedef
	if (edit.highlight.is_nil())
	{
		Beep("No highlighted line");
		return;
	}

	bool fresh_sel = Exec_HasFlag("/fresh");

	int match = 0;

	if (Exec_HasFlag("/onesided")) match |= SLP_OneSided;
	if (Exec_HasFlag("/sametex"))  match |= SLP_SameTex;

	int start_L = edit.highlight.num;

	if ((match & SLP_OneSided) && !level.linedefs[start_L]->OneSided())
		return;

	bool unset_them = false;

	if (!fresh_sel && edit.Selected->get(start_L))
		unset_them = true;

	selection_c seen(ObjType::linedefs);

	seen.set(start_L);

	SelectLinesInHalfPath(level, start_L, level.linedefs[start_L]->start, seen, match);
	SelectLinesInHalfPath(level, start_L, level.linedefs[start_L]->end,   seen, match);

	Editor_ClearErrorMode();

	if (fresh_sel)
		Selection_Clear();

	if (unset_them)
		edit.Selected->unmerge(seen);
	else
		edit.Selected->merge(seen);

	RedrawMap();
}


//------------------------------------------------------------------------

#define PLAYER_STEP_H	24

static bool GrowContiguousSectors(const Instance &inst, selection_c &seen)
{
	// returns TRUE when some new sectors got added

	bool changed = false;

	bool can_walk    = inst.Exec_HasFlag("/can_walk");
	bool allow_doors = inst.Exec_HasFlag("/doors");

	bool do_floor_h   = inst.Exec_HasFlag("/floor_h");
	bool do_floor_tex = inst.Exec_HasFlag("/floor_tex");
	bool do_ceil_h    = inst.Exec_HasFlag("/ceil_h");
	bool do_ceil_tex  = inst.Exec_HasFlag("/ceil_tex");

	bool do_light   = inst.Exec_HasFlag("/light");
	bool do_tag     = inst.Exec_HasFlag("/tag");
	bool do_special = inst.Exec_HasFlag("/special");

	for (const auto &L : inst.level.linedefs)
	{
		if (! L->TwoSided())
			continue;

		int sec1 = inst.level.getRight(*L)->sector;
		int sec2 = inst.level.getLeft(*L)->sector;

		if (sec1 == sec2)
			continue;

		const auto S1 = inst.level.sectors[sec1];
		const auto S2 = inst.level.sectors[sec2];

		// skip closed doors
		if (! allow_doors && (S1->floorh >= S1->ceilh || S2->floorh >= S2->ceilh))
			continue;

		if (can_walk)
		{
			if (L->flags & MLF_Blocking)
				continue;

			// too big a step?
			if (abs(S1->floorh - S2->floorh) > PLAYER_STEP_H)
				continue;

			// player wouldn't fit vertically?
			int f_max = std::max(S1->floorh, S2->floorh);
			int c_min = std::min(S1-> ceilh, S2-> ceilh);

			if (c_min - f_max < inst.conf.miscInfo.player_h)
			{
				// ... but allow doors
				if (! (allow_doors && (S1->floorh == S1->ceilh || S2->floorh == S2->ceilh)))
					continue;
			}
		}

		/* perform match */

		if (do_floor_h && (S1->floorh != S2->floorh)) continue;
		if (do_ceil_h  && (S1->ceilh  != S2->ceilh))  continue;

		if (do_floor_tex && (S1->floor_tex != S2->floor_tex)) continue;
		if (do_ceil_tex  && (S1->ceil_tex  != S2->ceil_tex))  continue;

		if (do_light   && (S1->light != S2->light)) continue;
		if (do_tag     && (S1->tag   != S2->tag  )) continue;
		if (do_special && (S1->type  != S2->type))  continue;

		// check if only one of the sectors is part of current set
		// (doing this _AFTER_ the matches since this can be a bit slow)
		bool got1 = seen.get(sec1);
		bool got2 = seen.get(sec2);

		if (got1 == got2)
			continue;

		seen.set(got1 ? sec2 : sec1);

		changed = true;
	}

	return changed;
}


//
// select/unselect a contiguous group of sectors.
//
void Instance::CMD_SEC_SelectGroup()
{
	// determine starting sector
	if (edit.highlight.is_nil())
	{
		Beep("No highlighted sector");
		return;
	}

	bool fresh_sel = Exec_HasFlag("/fresh");

	int start_sec = edit.highlight.num;

	bool unset_them = false;

	if (!fresh_sel && edit.Selected->get(start_sec))
		unset_them = true;

	selection_c seen(ObjType::sectors);

	seen.set(start_sec);

	while (GrowContiguousSectors(*this, seen))
	{ }


	Editor_ClearErrorMode();

	if (fresh_sel)
		Selection_Clear();

	if (unset_them)
		edit.Selected->unmerge(seen);
	else
		edit.Selected->merge(seen);

	RedrawMap();
}


//------------------------------------------------------------------------


void Instance::GoToSelection()
{
	if (edit.render3d)
		Render3D_Enable(*this, false);

	v2double_t pos1, pos2;
	level.objects.calcBBox(*edit.Selected, pos1, pos2);
	v2double_t mid = (pos1 + pos2) / 2;

	grid.MoveTo(mid);

	// zoom out until selected objects fit on screen
	for (int loop = 0 ; loop < 30 ; loop++)
	{
		int eval = main_win->canvas->ApproxBoxSize(static_cast<int>(pos1.x), static_cast<int>(pos1.y), static_cast<int>(pos2.x), static_cast<int>(pos2.y));

		if (eval <= 0)
			break;

		grid.AdjustScale(-1);
	}

	// zoom in when bbox is very small (say < 20% of window)
	for (int loop = 0 ; loop < 30 ; loop++)
	{
		if (grid.getScale() >= 1.0)
			break;

		int eval = main_win->canvas->ApproxBoxSize(static_cast<int>(pos1.x), static_cast<int>(pos1.y), static_cast<int>(pos2.x), static_cast<int>(pos2.y));

		if (eval >= 0)
			break;

		grid.AdjustScale(+1);
	}

	RedrawMap();
}


void Instance::GoToErrors()
{
	edit.error_mode = true;

	GoToSelection();
}


//
// centre the map around the object and zoom in if necessary
//
void Instance::GoToObject(const Objid& objid)
{
	Selection_Clear();

	edit.Selected->set(objid.num);

	GoToSelection();
}

//
// Allow going to multiple objects
//
void goToMultipleObjects(Instance &inst, const std::vector<int> &items)
{
    inst.Selection_Clear();
    for(int index : items)
        inst.edit.Selected->set(index);
    inst.GoToSelection();
}

void Instance::CMD_JumpToObject()
{
	int total = level.numObjects(edit.mode);

	if (total <= 0)
	{
		Beep("No objects!");
		return;
	}

    std::vector<int> nums;
    {
        auto dialog = std::make_unique<UI_JumpToDialog>(NameForObjectType(edit.mode), total - 1);

        nums = dialog->Run();
    }

	if (nums.empty())	// cancelled
		return;

	// this is guaranteed by the dialog
    for(int num : nums)
	{
		(void)num;
        assert(num >= 0 && num < total);
	}
    goToMultipleObjects(*this, nums);
}


void Instance::CMD_PruneUnused()
{
	selection_c used_secs (ObjType::sectors);
	selection_c used_sides(ObjType::sidedefs);
	selection_c used_verts(ObjType::vertices);

	for (const auto &L : level.linedefs)
	{
		used_verts.set(L->start);
		used_verts.set(L->end);

		if (L->left >= 0)
		{
			used_sides.set(L->left);
			used_secs.set(level.getLeft(*L)->sector);
		}

		if (L->right >= 0)
		{
			used_sides.set(L->right);
			used_secs.set(level.getRight(*L)->sector);
		}
	}

	used_secs .frob_range(0, level.numSectors() -1, BitOp::toggle);
	used_sides.frob_range(0, level.numSidedefs() -1, BitOp::toggle);
	used_verts.frob_range(0, level.numVertices()-1, BitOp::toggle);

	int num_secs  = used_secs .count_obj();
	int num_sides = used_sides.count_obj();
	int num_verts = used_verts.count_obj();

	if (num_verts == 0 && num_sides == 0 && num_secs == 0)
	{
		Beep("Nothing to prune");
		return;
	}

	EditOperation op(level.basis);
	op.setMessage("pruned %d objects", num_secs + num_sides + num_verts);

	level.objects.del(op, used_sides);
	level.objects.del(op, used_secs);
	level.objects.del(op, used_verts);
}


//------------------------------------------------------------------------

static void CalcPropagation(const Instance &inst, std::vector<byte>& vec, bool ignore_doors)
{
	bool changes;

	for (int k = 0 ; k < inst.level.numSectors(); k++)
		vec[k] = 0;

	vec[inst.sound_start_sec] = 2;

	do
	{
		changes = false;

		for (const auto &L : inst.level.linedefs)
		{
			if (! L->TwoSided())
				continue;

			int sec1 = inst.level.getSectorID(*L, Side::right);
			int sec2 = inst.level.getSectorID(*L, Side::left);

			SYS_ASSERT(sec1 >= 0);
			SYS_ASSERT(sec2 >= 0);

			// check for doors
			if (!ignore_doors &&
				(std::min(inst.level.sectors[sec1]->ceilh, inst.level.sectors[sec2]->ceilh) <=
				 std::max(inst.level.sectors[sec1]->floorh, inst.level.sectors[sec2]->floorh)))
			{
				continue;
			}

			int val1 = vec[sec1];
			int val2 = vec[sec2];

			int new_val = std::max(val1, val2);

			if (L->flags & MLF_SoundBlock)
				new_val -= 1;

			if (new_val > val1 || new_val > val2)
			{
				if (new_val > val1)
                    vec[sec1] = static_cast<byte>(new_val);
				if (new_val > val2)
                    vec[sec2] = static_cast<byte>(new_val);

				changes = true;
			}
		}

	} while (changes);
}


static void CalcFinalPropagation(Instance &inst)
{
	for (int s = 0 ; s < inst.level.numSectors(); s++)
	{
		int t1 = inst.sound_temp1_vec[s];
		int t2 = inst.sound_temp2_vec[s];

		if (t1 != t2)
		{
			if (t1 == 0 || t2 == 0)
			{
				inst.sound_prop_vec[s] = PGL_Maybe;
				continue;
			}

			t1 = std::min(t1, t2);
		}

		switch (t1)
		{
			case 0: inst.sound_prop_vec[s] = PGL_Never;   break;
			case 1: inst.sound_prop_vec[s] = PGL_Level_1; break;
			case 2: inst.sound_prop_vec[s] = PGL_Level_2; break;
		}
	}
}


const byte *Instance::SoundPropagation(int start_sec)
{
	if ((int)sound_prop_vec.size() != level.numSectors())
	{
		sound_prop_vec .resize(level.numSectors());
		sound_temp1_vec.resize(level.numSectors());
		sound_temp2_vec.resize(level.numSectors());

		sound_propagation_invalid = true;
	}

	if (sound_propagation_invalid ||
		sound_start_sec != start_sec)
	{
		// cannot used cached data, recompute it

		sound_start_sec = start_sec;
		sound_propagation_invalid = false;

		CalcPropagation(*this, sound_temp1_vec, false);
		CalcPropagation(*this, sound_temp2_vec, true);

		CalcFinalPropagation(*this);
	}

	return &sound_prop_vec[0];
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
