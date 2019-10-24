//------------------------------------------------------------------------
//  LINEDEFS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
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

#include "e_cutpaste.h"
#include "e_linedef.h"
#include "e_main.h"
#include "im_img.h"
#include "m_game.h"
#include "e_objects.h"
#include "w_rawdef.h"
#include "w_texture.h"


// config items
bool leave_offsets_alone = true;


bool LineDefAlreadyExists(int v1, int v2)
{
	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		if (L->start == v1 && L->end == v2) return true;
		if (L->start == v2 && L->end == v1) return true;
	}

	return false;
}


//
// return true if adding a line between v1 and v2 would overlap an
// existing line.  By "overlap" I mean parallel and sitting on top
// (this does NOT test for lines crossing each other).
//
bool LineDefWouldOverlap(int v1, double x2, double y2)
{
	double x1 = Vertices[v1]->x();
	double y1 = Vertices[v1]->y();

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		// zero-length lines should not exist, but don't get stroppy if they do
		if (L->IsZeroLength())
			continue;

		double lx1 = L->Start()->x();
		double ly1 = L->Start()->y();
		double lx2 = L->End()->x();
		double ly2 = L->End()->y();

		double a, b;

		a = PerpDist(x1, y1, lx1, ly1, lx2, ly2);
		b = PerpDist(x2, y2, lx1, ly1, lx2, ly2);

		if (fabs(a) >= 2.0 || fabs(b) >= 2.0)
			continue;

		a = AlongDist(x1, y1, lx1, ly1, lx2, ly2);
		b = AlongDist(x2, y2, lx1, ly1, lx2, ly2);

		double len = L->CalcLength();

		if (a > b)
			std::swap(a, b);

		if (b < 0.5 || a > len - 0.5)
			continue;

		return true;
	}

	return false;
}


//------------------------------------------------------------------------

static inline const LineDef * LD_ptr(const Objid& obj)
{
	return LineDefs[obj.num];
}

static inline const SideDef * SD_ptr(const Objid& obj)
{
	int where = (obj.parts & PART_LF_ALL) ? SIDE_LEFT : SIDE_RIGHT;

	int sd = LD_ptr(obj)->WhatSideDef(where);

	return (sd >= 0) ? SideDefs[sd] : NULL;
}


// disabled this partial texture comparison, as it can lead to
// unexpected results.  [ perhaps have an option for it? ]
#if 1
#define  PartialTexCmp  y_stricmp
#else
static int PartialTexCmp(const char *A, const char *B)
{
	// only compare the first 6 characters

	char A2[64];
	char B2[64];

	strcpy(A2, A);
	strcpy(B2, B);

	A2[6] = B2[6] = 0;

	return y_stricmp(A2, B2);
}
#endif


static bool PartIsVisible(const Objid& obj, char part)
{
	const LineDef *L  = LD_ptr(obj);
	const SideDef *SD = SD_ptr(obj);

	if (! L->TwoSided())
		return (part == 'l');

	const Sector *front = L->Right()->SecRef();
	const Sector *back  = L->Left ()->SecRef();

	if (obj.parts & PART_LF_ALL)
		std::swap(front, back);

	// ignore sky walls
	if (part == 'u' && is_sky(front->CeilTex()) && is_sky(back->CeilTex()))
		return false;

	if (part == 'l')
	{
		if (is_null_tex(SD->LowerTex()))
			return false;

		return back->floorh > front->floorh;
	}
	else
	{
		if (is_null_tex(SD->UpperTex()))
			return false;

		return back->ceilh < front->ceilh;
	}
}


//
// calculate vertical range that the given surface occupies.
// when part is zero, we use obj.type instead.
//
static void PartCalcExtent(const Objid& obj, char part, int *z1, int *z2)
{
	const LineDef *L  = LD_ptr(obj);
	const SideDef *SD = SD_ptr(obj);

	if (! L->TwoSided())
	{
		if (SD)
		{
			*z1 = SD->SecRef()->floorh;
			*z2 = SD->SecRef()->ceilh;
		}
		else
		{
			*z1 = *z2 = 0;
		}

		return;
	}

	if (! part)
	{
		if (obj.parts & (PART_RT_UPPER | PART_LF_UPPER))
			part = 'u';
		else if (obj.parts & (PART_RT_RAIL | PART_LF_RAIL))
			part = 'r';
		else
			part = 'l';
	}

	const Sector *front = L->Right()->SecRef();
	const Sector *back  = L->Left ()->SecRef();

	if (obj.parts & PART_LF_ALL)
		std::swap(front, back);

	if (part == 'r')
	{
		*z1 = MAX(front->floorh, back->floorh);
		*z2 = MIN(front->ceilh,  back->ceilh);
	}
	else if (part == 'u')
	{
		*z2 = front->ceilh;
		*z1 = MIN(*z2, back->ceilh);
	}
	else  // part == 'l'
	{
		*z1 = front->floorh;
		*z2 = MAX(*z1, back->floorh);
	}
}


static inline int ScoreTextureMatch(const Objid& adj, const Objid& cur)
{
	// result is in the range 1..999

	const LineDef *L  = LD_ptr(cur);
	const SideDef *LS = SD_ptr(cur);

	const LineDef *N  = LD_ptr(adj);
	const SideDef *NS = SD_ptr(adj);

	int adj_z1, adj_z2;
	int cur_z1, cur_z2;

	PartCalcExtent(adj, 0, &adj_z1, &adj_z2);
	PartCalcExtent(cur, 0, &cur_z1, &cur_z2);

	// adjacent surface is not visible?
	if (adj_z2 <= adj_z1)
		return 1;

	// no overlap?
	int overlap = MIN(adj_z2, cur_z2) - MAX(adj_z1, cur_z1);

	if (overlap <= 0)
		return 2;


	const char *adj_tex = NS->MidTex();

	if (N->TwoSided())
	{
		if (adj.parts & (PART_RT_LOWER | PART_LF_LOWER))
			adj_tex = NS->LowerTex();
		else if (adj.parts & (PART_RT_UPPER | PART_LF_UPPER))
			adj_tex = NS->UpperTex();
	}

	if (is_null_tex(adj_tex))
		return 3;

	const char *cur_tex = LS->MidTex();

	if (L->TwoSided())
	{
		if (cur.parts & (PART_RT_LOWER | PART_LF_LOWER))
			cur_tex = LS->LowerTex();
		else if (cur.parts & (PART_RT_UPPER | PART_LF_UPPER))
			cur_tex = LS->UpperTex();
	}

	if (PartialTexCmp(cur_tex, adj_tex) != 0)
		return 4;

	// return a score based on length of line shared between the
	// two surfaces

	overlap = overlap / 8;

	if (overlap > 900) overlap = 900;

	return 10 + overlap;
}


//
// Evaluate whether the given adjacent surface is a good (or even
// possible) candidate to align with.
//
// Having a matching texture is the primary component of the score.
// The secondary component is the angle between the lines (we prefer
// this angle to be close to 180 degrees).
//
// TODO : have a preference for the same sector ??
//
// Returns < 0 for surfaces that cannot be used.
//
static int ScoreAdjoiner(const Objid& adj,
						 const Objid& cur, int align_flags)
{
	bool do_right = (align_flags & LINALIGN_Right) ? true : false;

	const LineDef *L  = LD_ptr(cur);
	const SideDef *LS = SD_ptr(cur);

	const LineDef *N  = LD_ptr(adj);
	const SideDef *NS = SD_ptr(adj);

	// major fail by caller of Line_AlignOffsets()
	if (! LS)
		return -3;

	// no sidedef on adjoining line?
	if (! NS)
		return -2;

	// does the adjoiner sidedef actually mate up with the sidedef
	// we are aligning (and is on the wanted side) ?

	int v1 = (((cur.parts & PART_LF_ALL) ? 0:1) == (do_right ? 1:0)) ? L->end : L->start;
	int v2 = (((adj.parts & PART_LF_ALL) ? 0:1) == (do_right ? 1:0)) ? N->start : N->end;

	if (v1 != v2)
		return -1;

	/* Ok, we have a potential candidate */

	int v0 = (v1 == L->end) ? L->start : L->end;
	int v3 = (v2 == N->end) ? N->start : N->end;

	double ang = LD_AngleBetweenLines(v0, v1, v3);

	int score = ScoreTextureMatch(adj, cur);

	score = score * 1000 + 500 - (int)fabs(ang - 180.0);

	return score;
}


//
// Determine the adjoining surface to align with.
//
// Returns a nil Objid for none.
//
static void DetermineAdjoiner(Objid& result,
							  const Objid& cur, int align_flags)
{
	int best_score = 0;

	const LineDef *L = LD_ptr(cur);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *N = LineDefs[n];

		if (N == L)
			continue;

		if (N->IsZeroLength())
			continue;

		if (! (N->TouchesVertex(L->start) || N->TouchesVertex(L->end)))
			continue;

		for (int side = 0 ; side < 2 ; side++)
		for (int what = 0 ; what < 3 ; what++)
		{
			Objid adj;

			adj.type  = OBJ_LINEDEFS;
			adj.num   = n;
			adj.parts = (what == 0) ? PART_RT_LOWER : (what == 1) ? PART_RT_UPPER : PART_RT_RAIL;

			if (side == 1)
				adj.parts = adj.parts << 4;

			int score = ScoreAdjoiner(adj, cur, align_flags);

			if (score > best_score)
			{
				result     = adj;
				best_score = score;
			}
		}
	}
}


static int CalcReferenceH(const Objid& obj)
{
	const LineDef *L  = LD_ptr(obj);
	const SideDef *SD = SD_ptr(obj);

	if (! L->TwoSided())
	{
		if (! SD)
			return 256;

		const Sector *front = SD->SecRef();

		if (L->flags & MLF_LowerUnpegged)
			return front->floorh + W_GetTextureHeight(SD->MidTex());

		return front->ceilh;
	}


	const Sector *front = L->Right()->SecRef();
	const Sector *back  = L->Left ()->SecRef();

	if (obj.parts & PART_LF_ALL)
		std::swap(front, back);

	SYS_ASSERT(front);
	SYS_ASSERT(back);

	if (obj.parts & (PART_RT_UPPER | PART_LF_UPPER))
	{
		if (! (L->flags & MLF_UpperUnpegged))
			return back->ceilh + W_GetTextureHeight(SD->UpperTex());

		return front->ceilh;
	}
	else
	{
		// TODO : verify if this correct for RAIL

		if (! (L->flags & MLF_LowerUnpegged))
			return back->floorh;

		return front->ceilh;
	}
}


static void DoAlignX(const Objid& cur,
					 const Objid& adj, int align_flags)
{
	const LineDef *cur_L  = LD_ptr(cur);

	const LineDef *adj_L  = LD_ptr(adj);
	const SideDef *adj_SD = SD_ptr(adj);

	bool on_left = adj_L->TouchesVertex((cur.parts & PART_LF_ALL) ? cur_L->end : cur_L->start);

	int new_offset = adj_SD->x_offset;

	if (on_left)
	{
		new_offset += I_ROUND(adj_L->CalcLength());

		if (new_offset > 0)
			new_offset &= 1023;
	}
	else
	{
		new_offset -= I_ROUND(cur_L->CalcLength());

		if (new_offset < 0)
			new_offset = - (-new_offset & 1023);
	}

	int where = (cur.parts & PART_LF_ALL) ? SIDE_LEFT : SIDE_RIGHT;

	int sd = cur_L->WhatSideDef(where);

	BA_ChangeSD(sd, SideDef::F_X_OFFSET, new_offset);
}


static void DoAlignY(const Objid& cur,
					 const Objid& adj, int align_flags)
{
	const LineDef *L  = LD_ptr(cur);
	const SideDef *SD = SD_ptr(cur);

//	const LineDef *adj_L  = LD_ptr(adj);
	const SideDef *adj_SD = SD_ptr(adj);

	bool lower_vis = PartIsVisible(cur, 'l');
	bool upper_vis = PartIsVisible(cur, 'u');

	bool lower_unpeg = (L->flags & MLF_LowerUnpegged) ? true : false;
	bool upper_unpeg = (L->flags & MLF_UpperUnpegged) ? true : false;


	// check for windows (set the unpeg flags)

	int new_flags = L->flags;

	if ((align_flags & LINALIGN_Unpeg) &&
		L->TwoSided() &&
		lower_vis && upper_vis &&
		(! lower_unpeg || ! upper_unpeg) &&
	    (PartialTexCmp(SD->LowerTex(), SD->UpperTex()) == 0) &&
	    is_null_tex(SD->MidTex()) /* no rail */)
	{
		new_flags |= MLF_LowerUnpegged;
		new_flags |= MLF_UpperUnpegged;
	}

	// requirement: adj_tex_h + adj_y_off = cur_tex_h + cur_y_off

	int cur_texh = CalcReferenceH(cur);
	int adj_texh = CalcReferenceH(adj);

	int new_offset = adj_texh + adj_SD->y_offset - cur_texh;


	// normalize value  [TODO: handle BOOM non-power-of-two heights]

	if (new_offset < 0)
		new_offset = - (-new_offset & 255);
	else
		new_offset &= 255;


	int where = (cur.parts & PART_LF_ALL) ? SIDE_LEFT : SIDE_RIGHT;

	int sd = L->WhatSideDef(where);

	BA_ChangeSD(sd, SideDef::F_Y_OFFSET, new_offset);

	if (new_flags != L->flags)
	{
		BA_ChangeLD(cur.num, LineDef::F_FLAGS, new_flags);
	}
}


static void DoClearOfs(const Objid& cur, int align_flags)
{
	int where = (cur.parts & PART_LF_ALL) ? SIDE_LEFT : SIDE_RIGHT;

	int sd = LD_ptr(cur)->WhatSideDef(where);

	if (sd < 0)  // should not happen
		return;

	if (align_flags & LINALIGN_X)
	{
		// when the /right flag is used, make the texture end at the right side
		// (whereas zero makes it begin at the left side)
		if (align_flags & LINALIGN_Right)
			BA_ChangeSD(sd, SideDef::F_X_OFFSET, 0 - I_ROUND(LD_ptr(cur)->CalcLength()));
		else
			BA_ChangeSD(sd, SideDef::F_X_OFFSET, 0);
	}

	if (align_flags & LINALIGN_Y)
		BA_ChangeSD(sd, SideDef::F_Y_OFFSET, 0);
}


//
// Align the X and/or Y offets on the given surface.
//
// The given flags control which stuff to change, and where
// to look for the surface to align with.
//
bool Line_AlignOffsets(const Objid& obj, int align_flags)
{
	if (align_flags & LINALIGN_Clear)
	{
		DoClearOfs(obj, align_flags);
		return true;
	}

	Objid adj;
	DetermineAdjoiner(adj, obj, align_flags);

	if (! adj.valid())
		return false;

	if (align_flags & LINALIGN_X)
		DoAlignX(obj, adj, align_flags);

	if (align_flags & LINALIGN_Y)
		DoAlignY(obj, adj, align_flags);

	return true;
}


// returns true if the surface at 'k' MUST be aligned before the
// surface at 'j'.
static bool Align_CheckAdjacent(std::vector<Objid> & group,
								int j, int k, bool do_right)
{
	const Objid& ob_j = group[j];
	const Objid& ob_k = group[k];

	int vj = 0;

	if (((ob_j.parts & PART_LF_ALL) ? 0 : 1) == (do_right ? 1 : 0))
		vj = LineDefs[ob_j.num]->end;
	else
		vj = LineDefs[ob_j.num]->start;

	int vk = 0;

	if (((ob_k.parts & PART_LF_ALL) ? 0 : 1) == (do_right ? 1 : 0))
		vk = LineDefs[ob_k.num]->start;
	else
		vk = LineDefs[ob_k.num]->end;

	return (vj == vk);
}


//
// find an unvisited surface that has no possible dependency on
// any other unvisited surface.  In the case of loops, we pick
// an arbitrary surface.
//
static int Align_PickNextSurface(std::vector<Objid> & group,
								 const std::vector<byte>& seen, bool do_right)
{
	int fallback = -1;

	for (int j = 0 ; j < (int)group.size() ; j++)
	{
		if (seen[j]) continue;
		if (! group[j].valid()) continue;

		if (fallback < 0)
			fallback = j;

		bool has_better = false;

		for (int k = 0 ; k < (int)group.size() ; k++)
		{
			if (k == j)  continue;
			if (seen[k]) continue;
			if (! group[k].valid()) continue;

			if (Align_CheckAdjacent(group, j, k, do_right))
			{
				has_better = true;
				break;
			}
		}

		if (! has_better)
			return j;
	}

	// this will be -1 when there is no more surfaces left
	return fallback;
}


void Line_AlignGroup(std::vector<Objid> & group, int align_flags)
{
	// we will do each surface in the selection one-by-one,
	// and the order is significant when doing X offsets, so
	// mark them off via this array.
	std::vector<byte> seen;

	seen.resize(group.size());

	unsigned int k;

	for (k = 0 ; k < group.size() ; k++)
		seen[k] = 0;

	bool do_right = (align_flags & LINALIGN_Right) ? true : false;

	for (;;)
	{
		// get next unvisited surface
		int n = Align_PickNextSurface(group, seen, do_right);

		if (n < 0)
			break;

		// mark it seen
		seen[n] = 1;

		Line_AlignOffsets(group[n], align_flags);
	}
}


void CMD_LIN_Align()
{
	// parse the flags
	bool do_X = Exec_HasFlag("/x");
	bool do_Y = Exec_HasFlag("/y");

	if (! (do_X || do_Y))
	{
		Beep("LIN_Align: need x or y flag");
		return;
	}

	bool do_clear = Exec_HasFlag("/clear");
	bool do_right = Exec_HasFlag("/right");
	bool do_unpeg = true;

	int align_flags = 0;

	if (do_X) align_flags = align_flags | LINALIGN_X;
	if (do_Y) align_flags = align_flags | LINALIGN_Y;

	if (do_right) align_flags |= LINALIGN_Right;
	if (do_unpeg) align_flags |= LINALIGN_Unpeg;
	if (do_clear) align_flags |= LINALIGN_Clear;


	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("no lines to align");
		return;
	}

	// convert selection to group of surfaces
	// FIXME : handle parts from selection in 3D

	std::vector< Objid > group;

	selection_iterator_c it;
	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		Objid obj(OBJ_LINEDEFS, *it);

		const LineDef *L = LineDefs[obj.num];

		for (int pass = 0 ; pass < 2 ; pass++)
		{
			int where = pass ? SIDE_LEFT : SIDE_RIGHT;

			if (L->WhatSideDef(where) < 0)
				continue;

			// decide whether to use upper or lower
			// TODO : this could be smarter....

			bool lower_vis = PartIsVisible(obj, 'l');
			bool upper_vis = PartIsVisible(obj, 'u');

			if (! (lower_vis || upper_vis))
				continue;

			obj.parts = lower_vis ? PART_RT_LOWER : PART_RT_UPPER;
			if (where < 0)
				obj.parts = obj.parts << 4;

			group.push_back(obj);
		}
	}

	if (group.empty())
	{
		Beep("no visible surfaces");
		if (unselect == SOH_Unselect)
			Selection_Clear(true /* nosave */);
		return;
	}

	BA_Begin();

	Line_AlignGroup(group, align_flags);

	if (do_clear)
		BA_Message("cleared offsets");
	else
		BA_Message("aligned offsets");

	BA_End();

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);
}


//------------------------------------------------------------------------

static void FlipLine_verts(int ld)
{
	int old_start = LineDefs[ld]->start;
	int old_end   = LineDefs[ld]->end;

	BA_ChangeLD(ld, LineDef::F_START, old_end);
	BA_ChangeLD(ld, LineDef::F_END, old_start);
}


static void FlipLine_sides(int ld)
{
	int old_right = LineDefs[ld]->right;
	int old_left  = LineDefs[ld]->left;

	BA_ChangeLD(ld, LineDef::F_RIGHT, old_left);
	BA_ChangeLD(ld, LineDef::F_LEFT, old_right);
}


void FlipLineDef(int ld)
{
	FlipLine_verts(ld);
	FlipLine_sides(ld);
}


void FlipLineDef_safe(int ld)
{
	// this avoids creating a linedef with only a left side (no right)

	FlipLine_verts(ld);

	if (! LineDefs[ld]->OneSided())
		FlipLine_sides(ld);
}


void FlipLineDefGroup(selection_c *flip)
{
	selection_iterator_c it;

	for (flip->begin(&it) ; !it.at_end() ; ++it)
	{
		FlipLineDef(*it);
	}
}


//
// flip the orientation of some LineDefs
//
void CMD_LIN_Flip()
{
	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("No lines to flip");
		return;
	}

	bool force_it = Exec_HasFlag("/force");

	BA_Begin();
	BA_MessageForSel("flipped", edit.Selected);

	selection_iterator_c it;
	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		if (force_it)
			FlipLineDef(*it);
		else
			FlipLineDef_safe(*it);
	}

	BA_End();

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);
}


void CMD_LIN_SwapSides()
{
	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("No lines to swap sides");
		return;
	}

	BA_Begin();
	BA_MessageForSel("swapped sides on", edit.Selected);

	selection_iterator_c it;
	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		FlipLine_sides(*it);
	}

	BA_End();

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);
}


int SplitLineDefAtVertex(int ld, int new_v)
{
	LineDef * L = LineDefs[ld];
	Vertex  * V = Vertices[new_v];

	// create new linedef
	int new_l = BA_New(OBJ_LINEDEFS);

	LineDef * L2 = LineDefs[new_l];

	// it is OK to directly set fields of newly created objects
	L2->RawCopy(L);

	L2->start = new_v;
	L2->end   = L->end;

	// update vertex on original line
	BA_ChangeLD(ld, LineDef::F_END, new_v);

	// compute lengths (to update sidedef X offsets)
	int orig_length = (int)ComputeDist(
			L->Start()->x() - L->End()->x(),
			L->Start()->y() - L->End()->y());

	int new_length = (int)ComputeDist(
			L->Start()->x() - V->x(),
			L->Start()->y() - V->y());

	// update sidedefs

	if (L->Right())
	{
		L2->right = BA_New(OBJ_SIDEDEFS);
		L2->Right()->RawCopy(L->Right());

		if (! leave_offsets_alone)
			L2->Right()->x_offset += new_length;
	}

	if (L->Left())
	{
		L2->left = BA_New(OBJ_SIDEDEFS);
		L2->Left()->RawCopy(L->Left());

		if (! leave_offsets_alone)
		{
			int new_x_ofs = L->Left()->x_offset + orig_length - new_length;

			BA_ChangeSD(L->left, SideDef::F_X_OFFSET, new_x_ofs);
		}
	}

	return new_l;
}


static bool DoSplitLineDef(int ld)
{
	LineDef * L = LineDefs[ld];

	// prevent creating tiny lines (especially zero-length)
	if (abs(L->Start()->x() - L->End()->x()) < 4 &&
	    abs(L->Start()->y() - L->End()->y()) < 4)
		return false;

	double new_x = (L->Start()->x() + L->End()->x()) / 2;
	double new_y = (L->Start()->y() + L->End()->y()) / 2;

	int new_v = BA_New(OBJ_VERTICES);

	Vertex * V = Vertices[new_v];

	V->SetRawXY(new_x, new_y);

	SplitLineDefAtVertex(ld, new_v);

	return true;
}


//
// split one or more LineDefs in two, adding new Vertices in the middle
//
void CMD_LIN_SplitHalf(void)
{
	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("No lines to split");
		return;
	}

	int new_first = NumLineDefs;
	int new_count = 0;

	BA_Begin();

	selection_iterator_c it;
	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		if (DoSplitLineDef(*it))
			new_count++;
	}

	BA_Message("halved %d lines", new_count);
	BA_End();

	// Hmmmmm -- should abort early if some lines are too short??
	if (new_count < edit.Selected->count_obj())
		Beep("Some lines were too short!");

	if (unselect == SOH_Unselect)
	{
		Selection_Clear(true /* nosave */);
	}
	else if (new_count > 0)
	{
		edit.Selected->frob_range(new_first, new_first + new_count - 1, BOP_ADD);
	}
}


void LD_AddSecondSideDef(int ld, int new_sd, int other_sd)
{
	LineDef * L  = LineDefs[ld];
	SideDef * SD = SideDefs[new_sd];

	int new_flags = L->flags;

	new_flags |=  MLF_TwoSided;
	new_flags &= ~MLF_Blocking;

	BA_ChangeLD(ld, LineDef::F_FLAGS, new_flags);

	// TODO: make this a global pseudo-constant
	int null_tex = BA_InternaliseString("-");

	const SideDef *other = SideDefs[other_sd];

	if (! is_null_tex(other->MidTex()))
	{
		SD->lower_tex = other->mid_tex;
		SD->upper_tex = other->mid_tex;

		BA_ChangeSD(other_sd, SideDef::F_LOWER_TEX, other->mid_tex);
		BA_ChangeSD(other_sd, SideDef::F_UPPER_TEX, other->mid_tex);

		BA_ChangeSD(other_sd, SideDef::F_MID_TEX, null_tex);
	}
	else
	{
		SD->lower_tex = other->lower_tex;
		SD->upper_tex = other->upper_tex;
	}
}


void LD_MergedSecondSideDef(int ld)
{
	// similar to above, but with existing sidedefs

	LineDef * L = LineDefs[ld];

	SYS_ASSERT(L->TwoSided());

	int new_flags = L->flags;

	new_flags |=  MLF_TwoSided;
	new_flags &= ~MLF_Blocking;

	BA_ChangeLD(ld, LineDef::F_FLAGS, new_flags);

	// TODO: make this a global pseudo-constant
	int null_tex = BA_InternaliseString("-");

	// determine textures for each side
	int  left_tex = 0;
	int right_tex = 0;

	if (! is_null_tex(L->Left()->MidTex()))
		left_tex = L->Left()->mid_tex;

	if (! is_null_tex(L->Right()->MidTex()))
		right_tex = L->Right()->mid_tex;

	if (! left_tex)  left_tex = right_tex;
	if (! right_tex) right_tex = left_tex;

	// use default texture if both sides are empty
	if (! left_tex)
	{
		 left_tex = BA_InternaliseString(default_wall_tex);
		right_tex = left_tex;
	}

	BA_ChangeSD(L->left,  SideDef::F_MID_TEX, null_tex);
	BA_ChangeSD(L->right, SideDef::F_MID_TEX, null_tex);

	BA_ChangeSD(L->left,  SideDef::F_LOWER_TEX, left_tex);
	BA_ChangeSD(L->left,  SideDef::F_UPPER_TEX, left_tex);

	BA_ChangeSD(L->right, SideDef::F_LOWER_TEX, right_tex);
	BA_ChangeSD(L->right, SideDef::F_UPPER_TEX, right_tex);
}


void LD_RemoveSideDef(int ld, int ld_side)
{
	const LineDef *L = LineDefs[ld];

	int gone_sd  = (ld_side > 0) ? L->right : L->left;
	int other_sd = (ld_side > 0) ? L->left : L->right;

	if (ld_side > 0)
		BA_ChangeLD(ld, LineDef::F_RIGHT, -1);
	else
		BA_ChangeLD(ld, LineDef::F_LEFT, -1);

	if (other_sd < 0)
		return;

	// The line is changing from TWO SIDED --> ONE SIDED.
	// Hence we need to:
	//    (1) clear the Two-Sided flag
	//    (2) set the Impassible flag
	//	  (3) flip the linedef if right side was removed
	//    (4) set the middle texture

	int new_flags = L->flags;

	new_flags &= ~MLF_TwoSided;
	new_flags |=  MLF_Blocking;

	BA_ChangeLD(ld, LineDef::F_FLAGS, new_flags);

	// FIXME: if sidedef is shared, either don't modify it _OR_ duplicate it

	const SideDef *SD = SideDefs[other_sd];

	int new_tex = BA_InternaliseString(default_wall_tex);

	// grab new texture from lower or upper if possible
	if (! is_null_tex(SD->LowerTex()))
		new_tex = SD->lower_tex;
	else if (! is_null_tex(SD->UpperTex()))
		new_tex = SD->upper_tex;
	else if (gone_sd >= 0)
	{
		SD = SideDefs[gone_sd];

		if (! is_null_tex(SD->LowerTex()))
			new_tex = SD->lower_tex;
		else if (! is_null_tex(SD->UpperTex()))
			new_tex = SD->upper_tex;
	}

	BA_ChangeSD(other_sd, SideDef::F_MID_TEX, new_tex);
}


void CMD_LIN_MergeTwo(void)
{
	if (edit.Selected->count_obj() == 1 && edit.highlight.valid())
	{
		Selection_Add(edit.highlight);
	}

	if (edit.Selected->count_obj() != 2)
	{
		Beep("Need 2 linedefs to merge (got %d)", edit.Selected->count_obj());
		return;
	}

	// we will merge the second into the first

	int ld2 = edit.Selected->find_first();
	int ld1 = edit.Selected->find_second();

	const LineDef * L1 = LineDefs[ld1];
	const LineDef * L2 = LineDefs[ld2];

	if (! (L1->OneSided() && L2->OneSided()))
	{
		Beep("Linedefs to merge must be single sided.");
		return;
	}

	Selection_Clear(true);


	BA_Begin();

	// ld2 steals the sidedef from ld1

	BA_ChangeLD(ld2, LineDef::F_LEFT, L1->right);
	BA_ChangeLD(ld1, LineDef::F_RIGHT, -1);

	LD_MergedSecondSideDef(ld2);

	// fix existing lines connected to ld1 : reconnect to ld2

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		if (n == ld1 || n == ld2)
			continue;

		const LineDef * L = LineDefs[n];

		if (L->start == L1->start)
			BA_ChangeLD(n, LineDef::F_START, L2->end);
		else if (L->start == L1->end)
			BA_ChangeLD(n, LineDef::F_START, L2->start);

		if (L->end == L1->start)
			BA_ChangeLD(n, LineDef::F_END, L2->end);
		else if (L->end == L1->end)
			BA_ChangeLD(n, LineDef::F_END, L2->start);
	}

	// delete ld1 and any unused vertices

	selection_c del_line(OBJ_LINEDEFS);

	del_line.set(ld1);

	DeleteObjects_WithUnused(&del_line);

	BA_Message("merged two linedefs");

	BA_End();
}


void MoveCoordOntoLineDef(int ld, double *x, double *y)
{
	const LineDef *L = LineDefs[ld];

	double x1 = L->Start()->x();
	double y1 = L->Start()->y();
	double x2 = L->End()->x();
	double y2 = L->End()->y();

	double dx = x2 - x1;
	double dy = y2 - y1;

	double len_squared = dx*dx + dy*dy;

	SYS_ASSERT(len_squared > 0);

	// compute along distance
	double along = (*x - x1) * dx + (*y - y1) * dy;

	// result = start + along * line unit vector
	*x = x1 + along * dx / len_squared;
	*y = y1 + along * dy / len_squared;
}


static bool LD_StartWillBeMoved(int ld, selection_c& list)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const LineDef *L = LineDefs[*it];

		if (*it != ld && L->end == LineDefs[ld]->start)
			return true;
	}

	return false;
}


static bool LD_EndWillBeMoved(int ld, selection_c& list)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const LineDef *L = LineDefs[*it];

		if (*it != ld && L->start == LineDefs[ld]->end)
			return true;
	}

	return false;
}


static int PickLineDefToExtend(selection_c& list, bool moving_start)
{
	// We want a line whose new length is not going to be wrecked
	// by a change to a later linedef.  However we must handle loops!

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		if (moving_start)
		{
			if (! LD_EndWillBeMoved(*it, list))
				return *it;
		}
		else
		{
			if (! LD_StartWillBeMoved(*it, list))
				return *it;
		}
	}

	return list.find_first();
}


static void LD_SetLength(int ld, int new_len, int angle)
{
	// the 'new_len' parameter can be negative, which means move
	// the start vertex instead of the end vertex.

	const LineDef *L = LineDefs[ld];

	double dx = abs(new_len) * cos(angle * M_PI / 32768.0);
	double dy = abs(new_len) * sin(angle * M_PI / 32768.0);

	int idx = I_ROUND(dx);
	int idy = I_ROUND(dy);

	if (idx == 0 && idy == 0)
	{
		if (dx < 0) idx = (int)floor(dx); else idx = (int)ceil(dx);
		if (dy < 0) idy = (int)floor(dy); else idy = (int)ceil(dy);
	}

	if (idx == 0 && idy == 0)
		idx = 1;

	if (new_len < 0)
	{
		BA_ChangeVT(L->start, Vertex::F_X, L->End()->raw_x - INT_TO_COORD(idx));
		BA_ChangeVT(L->start, Vertex::F_Y, L->End()->raw_y - INT_TO_COORD(idy));
	}
	else
	{
		BA_ChangeVT(L->end, Vertex::F_X, L->Start()->raw_x + INT_TO_COORD(idx));
		BA_ChangeVT(L->end, Vertex::F_Y, L->Start()->raw_y + INT_TO_COORD(idy));
	}
}


void LineDefs_SetLength(int new_len)
{
	// this works on the current selection (caller must set it up)

	// use a copy of the selection
	selection_c list(OBJ_LINEDEFS);
	ConvertSelection(edit.Selected, &list);

	if (list.empty())
		return;

	// remember angles
	std::vector<int> angles(NumLineDefs);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		angles[n] = ComputeAngle(L->End()->x() - L->Start()->x(),
								 L->End()->y() - L->Start()->y());
	}

	BA_Begin();
	BA_MessageForSel("set length of", &list);

	while (! list.empty())
	{
		int ld = PickLineDefToExtend(list, new_len < 0 /* moving_start */);

		list.clear(ld);

		LD_SetLength(ld, new_len, angles[ld]);
	}

	BA_End();
}


void LD_FixForLostSide(int ld)
{
	LineDef * L = LineDefs[ld];

	SYS_ASSERT(L->Right());

	int tex;

	if (! is_null_tex(L->Right()->LowerTex()))
		tex = L->Right()->lower_tex;
	else if (! is_null_tex(L->Right()->UpperTex()))
		tex = L->Right()->upper_tex;
	else
		tex = BA_InternaliseString(default_wall_tex);

	BA_ChangeSD(L->right, SideDef::F_MID_TEX, tex);
}


//
// Compute the angle between lines AB and BC, going anticlockwise.
// result is in degrees in the range [0, 360).
//
// A, B and C are VERTEX indices.
//
// -AJA- 2001-05-09
//
double LD_AngleBetweenLines(int A, int B, int C)
{
	double a_dx = Vertices[B]->x() - Vertices[A]->x();
	double a_dy = Vertices[B]->y() - Vertices[A]->y();

	double c_dx = Vertices[B]->x() - Vertices[C]->x();
	double c_dy = Vertices[B]->y() - Vertices[C]->y();

	double AB_angle = (a_dx == 0) ? (a_dy >= 0 ? 90 : -90) : atan2(a_dy, a_dx) * 180 / M_PI;
	double CB_angle = (c_dx == 0) ? (c_dy >= 0 ? 90 : -90) : atan2(c_dy, c_dx) * 180 / M_PI;

	double result = CB_angle - AB_angle;

	while (result >= 360.0)
		result -= 360.0;

	while (result < 0)
		result += 360.0;

#if 0  // DEBUGGING
	DebugPrintf("ANGLE %1.6f  (%d,%d) -> (%d,%d) -> (%d,%d)\n", result,
			Vertices[A].x, Vertices[A].y,
			Vertices[B].x, Vertices[B].y,
			Vertices[C].x, Vertices[C].y);
#endif

	return result;
}


bool LD_GetTwoNeighbors(int new_ld, int v1, int v2,
						int *ld1, int *side1,
						int *ld2, int *side2)
{
	// find the two linedefs that are neighbors to the new line at
	// the second vertex (v2).  The first one (ld1) is on new_ld's
	// right side, and the second one (ld2) is on new_ld's left side.

	*ld1 = -1;
	*ld2 = -1;

	double best_angle1 =  9999;
	double best_angle2 = -9999;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		if (n == new_ld)
			continue;

		const LineDef *L = LineDefs[n];

		int other_v;

		if (L->start == v2)
			other_v = L->end;
		else if (L->end == v2)
			other_v = L->start;
		else
			continue;

		double angle = LD_AngleBetweenLines(v1, v2, other_v);

		// overlapping lines
		if (fabs(angle) < 0.0001)
			return false;

		if (angle < best_angle1)
		{
			*ld1 = n;
			*side1 = (other_v == L->start) ? SIDE_LEFT : SIDE_RIGHT;
			best_angle1 = angle;
		}

		if (angle > best_angle2)
		{
			*ld2 = n;
			*side2 = (other_v == L->start) ? SIDE_RIGHT : SIDE_LEFT;
			best_angle2 = angle;
		}
	}

#if 0
	DebugPrintf("best right: line:#%d side:%d angle:%1.2f\n",
	        *ld1, *side1, best_angle1);
	DebugPrintf("best left: line:#%d side:%d angle:%1.2f\n",
	        *ld2, *side2, best_angle2);
#endif

	if (*ld1 < 0 || *ld2 < 0 || *ld1 == *ld2)
		return false;

	return true;
}


bool LD_RailHeights(int& z1, int& z2, const LineDef *L, const SideDef *sd,
					const Sector *front, const Sector *back)
{
	const char *rail_tex = sd->MidTex();
	if (is_null_tex(rail_tex))
		return false;

	z1 = MAX(front->floorh, back->floorh);
	z2 = MIN(front->ceilh,  back->ceilh);

	if (z2 <= z1)
		return false;

	int img_h = W_GetTextureHeight(rail_tex);

	if (L->flags & MLF_LowerUnpegged)
	{
		z1 = z1 + sd->y_offset;
		z2 = z1 + img_h;
	}
	else
	{
		z2 = z2 + sd->y_offset;
		z1 = z2 - img_h;
	}

	return true;
}


//  SideDef packing logic -- raw from glBSP
#if 0

static int SidedefCompare(const void *p1, const void *p2)
{
	int comp;

	int side1 = ((const u16_t *) p1)[0];
	int side2 = ((const u16_t *) p2)[0];

	sidedef_t *A = lev_sidedefs[side1];
	sidedef_t *B = lev_sidedefs[side2];

	if (side1 == side2)
		return 0;

	// don't merge sidedefs on special lines
	if (A->on_special || B->on_special)
		return side1 - side2;

	if (A->sector != B->sector)
	{
		if (A->sector == NULL) return -1;
		if (B->sector == NULL) return +1;

		return (A->sector->index - B->sector->index);
	}

	if ((int)A->x_offset != (int)B->x_offset)
		return A->x_offset - (int)B->x_offset;

	if ((int)A->y_offset != B->y_offset)
		return (int)A->y_offset - (int)B->y_offset;

	// compare textures

	comp = memcmp(A->upper_tex, B->upper_tex, sizeof(A->upper_tex));
	if (comp) return comp;

	comp = memcmp(A->lower_tex, B->lower_tex, sizeof(A->lower_tex));
	if (comp) return comp;

	comp = memcmp(A->mid_tex, B->mid_tex, sizeof(A->mid_tex));
	if (comp) return comp;

	// sidedefs must be the same
	return 0;
}

void DetectDuplicateSidedefs(void)
{
	int i;
	u16_t *array = (u16_t *)UtilCalloc(num_sidedefs * sizeof(u16_t));

	GB_DisplayTicker();

	// sort array of indices
	for (i=0; i < num_sidedefs; i++)
		array[i] = i;

	qsort(array, num_sidedefs, sizeof(u16_t), SidedefCompare);

	// now mark them off
	for (i=0; i < num_sidedefs - 1; i++)
	{
		// duplicate ?
		if (SidedefCompare(array + i, array + i+1) == 0)
		{
			sidedef_t *A = lev_sidedefs[array[i]];
			sidedef_t *B = lev_sidedefs[array[i+1]];

			// found a duplicate !
			B->equiv = A->equiv ? A->equiv : A;
		}
	}

	UtilFree(array);

	// update all linedefs
	for (i=0, new_num=0; i < num_linedefs; i++)
	{
		linedef_t *L = lev_linedefs[i];

		// handle duplicated sidedefs
		while (L->right && L->right->equiv)
		{
			L->right->ref_count--;
			L->right = L->right->equiv;
			L->right->ref_count++;
		}

		while (L->left && L->left->equiv)
		{
			L->left->ref_count--;
			L->left = L->left->equiv;
			L->left->ref_count++;
		}
	}
}
#endif


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
