//------------------------------------------------------------------------
//  LINEDEF PATHS
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

#include "m_bitvec.h"
#include "m_dialog.h"
#include "editloop.h"
#include "e_path.h"
#include "levels.h"
#include "r_grid.h"
#include "selectn.h"
#include "w_rawdef.h"
#include "x_mirror.h"

#include "ui_window.h"


typedef enum
{
	SLP_Normal = 0,

	SLP_SameTex  = (1 << 1),  // require lines have same textures
	SLP_OneSided = (1 << 2),  // only handle one-sided lines
}
select_lines_in_path_flag_e;


static bool MatchingTextures(int index1, int index2)
{
	LineDef *L1 = LineDefs[index1];
	LineDef *L2 = LineDefs[index2];

	// lines with no sidedefs only match each other
	if (! L1->Right() || ! L2->Right())
		return L1->Right() == L2->Right();

	// determine texture to match from first line
	int texture = 0;

	if (L1->OneSided())
	{
		texture = L1->Right()->mid_tex;
	}
	else
	{
		SYS_ASSERT(L1->TwoSided());

		int f_diff = L1->Left()->SecRef()->floorh - L1->Right()->SecRef()->floorh;
		int c_diff = L1->Left()->SecRef()->ceilh  - L1->Right()->SecRef()->ceilh;

		if (f_diff == 0 && c_diff != 0)
			texture = (c_diff > 0) ? L1->Left()->upper_tex : L1->Right()->upper_tex;
		else
			texture = (f_diff < 0) ? L1->Left()->lower_tex : L1->Right()->lower_tex;
	}

	// match texture with other line

	if (L2->OneSided())
	{
		return (L2->Right()->mid_tex == texture);
	}
	else
	{
		SYS_ASSERT(L2->TwoSided());

		int f_diff = L2->Left()->SecRef()->floorh - L2->Right()->SecRef()->floorh;
		int c_diff = L2->Left()->SecRef()->ceilh  - L2->Right()->SecRef()->ceilh;

		if (c_diff != 0)
			if (texture == ((c_diff > 0) ? L2->Left()->upper_tex : L2->Right()->upper_tex))
				return true;

		if (f_diff != 0)
			if (texture == ((f_diff < 0) ? L2->Left()->lower_tex : L2->Right()->lower_tex))
				return true;

		return false;
	}
}


bool OtherLineDef(int L, int V, int *L_other, int *V_other,
                  int match, int start_L)
{
	*L_other = -1;
	*V_other = -1;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		if (n == L)
			continue;

		if ((match & SLP_OneSided) && ! LineDefs[n]->OneSided())
			continue;

		for (int k = 0 ; k < 2 ; k++)
		{
			int v1 = LineDefs[n]->start;
			int v2 = LineDefs[n]->end;

			if (k == 1)
				std::swap(v1, v2);

			if (v1 != V)
				continue;

			if ((match & SLP_SameTex) && ! MatchingTextures(start_L, n))
				continue;

			if (*L_other >= 0)  // There is a fork in the path. Stop here.
				return false;
			
			*L_other = n;
			*V_other = v2;
		}
	}

	return (*L_other >= 0);
}


// This routine looks for all linedefs other than 'L' which use
// the vertex 'V'.  If there are none or more than one, the search
// stops there and nothing else happens.  If there is exactly one,
// then it is added to the selection and we continue from the new
// linedef and vertex.

static void SelectLinesInHalfPath(int L, int V, selection_c& seen, int match)
{
	int start_L = L;

	for (;;)
	{
		int L_other, V_other;

		// does not exist or is forky
		if (! OtherLineDef(L, V, &L_other, &V_other, match, start_L))
			break;

		// already seen?
		if (seen.get(L_other))
			break;

		seen.set(L_other);

		L = L_other;
		V = V_other;
	}
}


/* Select/unselect all linedefs in non-forked path.
 *
 * Possible flags:
 *    1 : one-sided only
 *    a : additive
 *    s : same texture
 *
 */
void LIN_SelectPath(void)
{
	// determine starting linedef
	if (edit.highlighted.is_nil())
	{
		Beep("No highlighted line");
		return;
	}

	const char *flags = EXEC_Param[0];

	bool additive = strchr(flags, 'a') ? true : false;

	int match = 0;

	if (strchr(flags, '1')) match |= SLP_OneSided;
	if (strchr(flags, 's')) match |= SLP_SameTex;

	if (edit.did_a_move)
	{
		edit.did_a_move = false;
		additive = false;
	}


	int start_L = edit.highlighted.num;

	if ((match & SLP_OneSided) && ! LineDefs[start_L]->OneSided())
		return;

	bool unset_them = false;

	if (additive && edit.Selected->get(start_L))
		unset_them = true;

	selection_c seen(OBJ_LINEDEFS);

	seen.set(start_L);

	SelectLinesInHalfPath(start_L, LineDefs[start_L]->start, seen, match);
	SelectLinesInHalfPath(start_L, LineDefs[start_L]->end,   seen, match);

	if (! additive)
		edit.Selected->clear_all();

	if (unset_them)
		edit.Selected->unmerge(seen);
	else
		edit.Selected->merge(seen);

	 edit.RedrawMap = 1;
}


//------------------------------------------------------------------------


static bool GrowContiguousSectors(selection_c &seen, const char *match)
{
	// returns TRUE when a new sector got added

	bool changed = false;

	bool allow_doors = strchr(match, 'd') ? true : false;
	bool walk_test   = strchr(match, 'w') ? true : false;

	bool do_floor_h   = strchr(match, 'f') ? true : false;
	bool do_floor_tex = strchr(match, 'F') ? true : false;
	bool do_ceil_h    = strchr(match, 'c') ? true : false;
	bool do_ceil_tex  = strchr(match, 'C') ? true : false;

	bool do_light   = strchr(match, 'l') ? true : false;
	bool do_tag     = strchr(match, 't') ? true : false;
	bool do_special = strchr(match, 's') ? true : false;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		if (! L->TwoSided())
			continue;

		int sec1 = L->Right()->sector;
		int sec2 = L-> Left()->sector;

		if (sec1 == sec2)
			continue;

		Sector *S1 = Sectors[sec1];
		Sector *S2 = Sectors[sec2];

		// skip closed doors
		if (! allow_doors && (S1->floorh >= S1->ceilh || S2->floorh >= S2->ceilh))
			continue;

		if (walk_test)
		{
			if (L->flags & MLF_Blocking)
				continue;

			// too big a step?
			if (abs(S1->floorh - S2->floorh) > 24)
				continue;

			// player wouldn't fit vertically?
			int f_max = MAX(S1->floorh, S2->floorh);
			int c_min = MIN(S1-> ceilh, S2-> ceilh);

			if (c_min - f_max < DOOM_PLAYER_HEIGHT)
				continue;
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
		// (doing this _AFTER_ the match since this can be a bit slow)
		bool got1 = seen.get(sec1);
		bool got2 = seen.get(sec2);

		if (got1 == got2)
			continue;

		seen.set(got1 ? sec2 : sec1);

		changed = true;
	}

	return changed;
}


/* Select/unselect a contiguous group of sectors.
 *
 * Possible flags:
 *    a : additive
 *    d : pass through doors (closed sectors)
 *    w : walk check
 *    
 *    f : match floor height
 *    F : match floor texture
 *    c : match ceiling height
 *    C : match ceiling texture
 *
 *    l : match lighting
 *    t : match tag
 *    s : match special
 */
void SEC_SelectGroup(void)
{
	const char *match = EXEC_Param[0];

	bool additive = strchr(match, 'a') ? true : false;

	// determine starting sector
	if (edit.highlighted.is_nil())
	{
		Beep("No highlighted sector");
		return;
	}

	if (edit.did_a_move)
	{
		edit.did_a_move = false;
		additive = false;
	}

	int start_sec = edit.highlighted.num;

	bool unset_them = false;

	if (additive && edit.Selected->get(start_sec))
		unset_them = true;

	selection_c seen(OBJ_SECTORS);

	seen.set(start_sec);

	// TODO: use a bitvec for 'seen' (should be much faster)

	while (GrowContiguousSectors(seen, match))
	{ }

	if (! additive)
		edit.Selected->clear_all();

	if (unset_them)
		edit.Selected->unmerge(seen);
	else
		edit.Selected->merge(seen);

	 edit.RedrawMap = 1;
}


//------------------------------------------------------------------------


void GoToSelection()
{
	int x1, y1, x2, y2;

	Objs_CalcBBox(edit.Selected, &x1, &y1, &x2, &y2);

	int mid_x = (x1 + x2) / 2;
	int mid_y = (y1 + y2) / 2;

	grid.CenterMapAt(mid_x, mid_y);


	// zoom out until selected objects fit on screen
	for (int loop = 0 ; loop < 30 ; loop++)
	{
		int eval = main_win->canvas->ApproxBoxSize(x1, y1, x2, y2);

		if (eval <= 0)
			break;

		grid.AdjustScale(-1);
	}

	// zoom in when bbox is very small (say < 20% of window)
	for (int loop = 0 ; loop < 30 ; loop++)
	{
		if (grid.Scale >= 1.0)
			break;

		int eval = main_win->canvas->ApproxBoxSize(x1, y1, x2, y2);

		if (eval >= 0)
			break;

		grid.AdjustScale(+1);
	}
}


/*
  centre the map around the object and zoom in if necessary
*/
void GoToObject(const Objid& objid)
{
	edit.Selected->clear_all();
	edit.Selected->set(objid.num);

	GoToSelection();
}


void CMD_JumpToObject(void)
{
	const char *buf = fl_input("Enter index number", "");

	if (! buf)   // cancelled
		return;

	// TODO: validate it is a number

	int num = atoi(buf);

	if (num < 0 || num >= NumObjects(edit.obj_type))
	{
		Beep("No such object: #%d", num);
		return;
	}

	edit.Selected->clear_all();
	edit.Selected->set(num);

	// alternatively: focus_on_selection()

	GoToObject(Objid(edit.obj_type, num));
}


void CMD_NextObject()
{
	if (edit.Selected->count_obj() != 1)
	{
		Beep("Next: need a single object");
		return;
	}

	int num = edit.Selected->find_first();

	if (num >= NumObjects(edit.obj_type))
	{
		Beep("Next: no more objects");
		return;
	}

	num += 1;

	edit.Selected->clear_all();
	edit.Selected->set(num);

	GoToObject(Objid(edit.obj_type, num));
}


void CMD_PrevObject()
{
	if (edit.Selected->count_obj() != 1)
	{
		Beep("Prev: need a single object");
		return;
	}

	int num = edit.Selected->find_first();

	if (num <= 0)
	{
		Beep("Prev: no more objects");
		return;
	}

	num -= 1;

	edit.Selected->clear_all();
	edit.Selected->set(num);

	GoToObject(Objid(edit.obj_type, num));
}


void CMD_FindObjectByType()
{
#if 0
	else if (key == 'f' && ( edit.highlighted ())) 
	{
		Objid find_obj;
		int otype;
		obj_no_t omax,onum;
		find_obj.type = edit.highlighted () ? edit.highlighted.type : edit.obj_type;
		onum = find_obj.num  = edit.highlighted () ? edit.highlighted.num  : 0;
		omax = GetMaxObjectNum(find_obj.type);
		switch (find_obj.type)
		{
			case OBJ_SECTORS:
				if ( ! InputSectorType( 84, 21, &otype))
				{
					for (onum = edit.highlighted () ? onum + 1 : onum; onum <= omax; onum++)
						if (Sectors[onum]->type == otype)
						{
							find_obj.num = onum;
							GoToObject(find_obj);
							break;
						}
				}
				break;
			case OBJ_THINGS:
				if ( ! InputThingType( 42, 21, &otype))
				{
					for (onum = edit.highlighted () ? onum + 1 : onum; onum <= omax; onum++)
						if (Things[onum]->type == (wad_ttype_t) otype)
						{
							find_obj.num = onum;
							GoToObject(find_obj);
							break;
						}
				}
				break;
			case OBJ_LINEDEFS:
				if ( ! InputLinedefType( 0, 21, &otype))
				{
					for (onum = edit.highlighted () ? onum + 1 : onum; onum <= omax; onum++)
						if (LineDefs[onum]->type == otype)
						{
							find_obj.num = onum;
							GoToObject(find_obj);
							break;
						}
				}
				break;
		}
		edit.RedrawMap = 1;
	}
#endif
}


void CMD_PruneUnused(void)
{
	selection_c used_secs (OBJ_SECTORS);
	selection_c used_sides(OBJ_SIDEDEFS);
	selection_c used_verts(OBJ_VERTICES);

	for (int i = 0 ; i < NumLineDefs ; i++)
	{
		const LineDef * L = LineDefs[i];

		used_verts.set(L->start);
		used_verts.set(L->end);

		if (L->left >= 0)
		{
			used_sides.set(L->left);
			used_secs.set(L->Left()->sector);
		}

		if (L->right >= 0)
		{
			used_sides.set(L->right);
			used_secs.set(L->Right()->sector);
		}
	}

	used_secs .frob_range(0, NumSectors -1, BOP_TOGGLE);
	used_sides.frob_range(0, NumSideDefs-1, BOP_TOGGLE);
	used_verts.frob_range(0, NumVertices-1, BOP_TOGGLE);

	int num_secs  = used_secs .count_obj();
	int num_sides = used_sides.count_obj();
	int num_verts = used_verts.count_obj();

	if (num_verts == 0 && num_sides == 0 && num_secs == 0)
	{
		Beep("Nothing to prune");
		return;
	}

	BA_Begin();

	DeleteObjects(&used_sides);
	DeleteObjects(&used_secs);
	DeleteObjects(&used_verts);

	Status_Set("Pruned %d SEC, %d SID, %d VRT", num_secs, num_sides, num_verts);

	BA_End();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
