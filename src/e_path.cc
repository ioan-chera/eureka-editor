//------------------------------------------------------------------------
//  LINEDEF PATHS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
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
#include "editloop.h"
#include "e_path.h"
#include "levels.h"
#include "selectn.h"
#include "w_rawdef.h"


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

static void SelectLinesInHalfPath(int L, int V, selection_c &seen, int match)
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


void CMD_SelectLinesInPath(int flags)
{
	bool additive = ! (flags & SLP_ClearSel);

	// determine starting linedef
	if (edit.highlighted.is_nil())
		return;  // beep?

	int start_L = edit.highlighted.num;

	if ((flags & SLP_OneSided) && ! LineDefs[start_L]->OneSided())
		return;

	bool unset_them = false;

	if (additive && edit.Selected->get(start_L))
		unset_them = true;

	selection_c seen(OBJ_LINEDEFS);

	seen.set(start_L);

	SelectLinesInHalfPath(start_L, LineDefs[start_L]->start, seen, flags);
	SelectLinesInHalfPath(start_L, LineDefs[start_L]->end,   seen, flags);

	if (! additive)
		edit.Selected->clear_all();

	if (unset_them)
		edit.Selected->unmerge(seen);
	else
		edit.Selected->merge(seen);

	 edit.RedrawMap = 1;
}


//------------------------------------------------------------------------


static bool GrowContiguousSectors(selection_c &seen, int match)
{
	// returns TRUE when a new sector got added

	bool changed = false;

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
		if (S1->floorh >= S1->ceilh || S2->floorh >= S2->ceilh)
			continue;

		// check whether match is satisfied
		if ((match & SCS_Floor_H) && (S1->floorh != S2->floorh))
			continue;

		if ((match & SCS_FloorTex) && (S1->floor_tex != S2->floor_tex))
			continue;

		if ((match & SCS_Ceil_H) && (S1->ceilh != S2->ceilh))
			continue;

		if ((match & SCS_CeilTex) && (S1->ceil_tex != S2->ceil_tex))
			continue;

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


void CMD_SelectContiguousSectors(int flags)
{
	bool additive = ! (flags & SCS_ClearSel);

	// determine starting linedef
	if (edit.highlighted.is_nil())
		return;  // beep?

	int start_sec = edit.highlighted.num;

	bool unset_them = false;

	if (additive && edit.Selected->get(start_sec))
		unset_them = true;

	selection_c seen(OBJ_SECTORS);

	seen.set(start_sec);

	// TODO: force 'seen' to be a bitvec (should be much faster)

	while (GrowContiguousSectors(seen, flags))
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

void CMD_JumpToObject()
{
#if 0
	const char *buf = fl_input("Enter index number", "");

	if (! buf)
		return;

	Objid target_obj;

	target_obj.type = edit.obj_type;
	target_obj.num  = atoi(buf);

	if (target_obj ())
	{
		GoToObject (target_obj);

		edit.RedrawMap = 1;
	}
#else
	Beep();
	return;
#endif
}


void CMD_NextObject()
{
	int t = edit.highlighted () ? edit.highlighted.type : edit.obj_type;

	obj_no_t nmax = GetMaxObjectNum (t);

	if (is_obj (nmax))
	{
		if (edit.highlighted.is_nil())
		{
			edit.highlighted.type = (obj_type_e)t;
			edit.highlighted.num = 0;
		}
		else
		{
			edit.highlighted.num++;
			if (edit.highlighted.num > nmax)
				edit.highlighted.num = 0;
		}
		GoToObject (edit.highlighted);
		edit.RedrawMap = 1;
	}
}


void CMD_PrevObject()
{
	int t = edit.highlighted () ? edit.highlighted.type : edit.obj_type;

	obj_no_t nmax = GetMaxObjectNum (t);
	
	if (is_obj (nmax))
	{
		if (edit.highlighted.is_nil())
		{
			edit.highlighted.type = (obj_type_e)t;
			edit.highlighted.num = nmax;
		}
		else
		{
			edit.highlighted.num--;
			if (edit.highlighted.num < 0)
				edit.highlighted.num = nmax;
		}
		GoToObject (edit.highlighted);
		edit.RedrawMap = 1;
	}
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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
