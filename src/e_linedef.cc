//------------------------------------------------------------------------
//  LINEDEFS
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

#include "e_cutpaste.h"
#include "e_linedef.h"
#include "editloop.h"
#include "im_img.h"
#include "levels.h"
#include "m_game.h"
#include "objects.h"
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


/* return true if adding a line between v1 and v2 would overlap an
   existing line.  By "overlap" I mean parallel and sitting on top
   (this does NOT test for lines crossing each other).
*/
bool LineDefWouldOverlap(int v1, int x2, int y2)
{
	int x1 = Vertices[v1]->x;
	int y1 = Vertices[v1]->y;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		// zero-length lines should not exist, but don't get stroppy if they do
		if (L->isZeroLength())
			continue;

		double a, b;
		
		a = PerpDist(x1, y1, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);
		b = PerpDist(x2, y2, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);

		if (fabs(a) >= 2.0 || fabs(b) >= 2.0)
			continue;

		a = AlongDist(x1, y1, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);
		b = AlongDist(x2, y2, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);

		double len = L->CalcLength();

		if (a > b)
			std::swap(a, b);

		if (b < 0.5 || a > len - 0.5)
			continue;

		return true;
	}

	return false;
}


/*
  deletes all the linedefs AND unused vertices AND unused sidedefs
*/
void DeleteLineDefs(selection_c *lines)
{
	selection_c  verts(OBJ_VERTICES);
	selection_c  sides(OBJ_SIDEDEFS);

	UnusedVertices(lines, &verts);
	UnusedSideDefs(lines, &sides);

	DeleteObjects(lines);
	DeleteObjects(&verts);
	DeleteObjects(&sides);
}


/*
   get the absolute height from which the textures are drawn
*/

int GetTextureRefHeight (int sidedef)
{
	int l, sector;
	int otherside = NIL_OBJ;

	/* find the sidedef on the other side of the LineDef, if any */

	for (l = 0 ; l < NumLineDefs ; l++)
	{
		if (LineDefs[l]->right == sidedef)
		{
			otherside = LineDefs[l]->left;
			break;
		}
		if (LineDefs[l]->left == sidedef)
		{
			otherside = LineDefs[l]->right;
			break;
		}
	}
	/* get the Sector number */

	sector = SideDefs[sidedef]->sector;
	/* if the upper texture is displayed,
	   then the reference is taken from the other Sector */
	if (otherside >= 0)
	{
		l = SideDefs[otherside]->sector;
		if (l > 0)
		{

			if (Sectors[l]->ceilh < Sectors[sector]->ceilh
					&& Sectors[l]->ceilh > Sectors[sector]->floorh)
				sector = l;
		}
	}
	/* return the altitude of the ceiling */

	if (sector >= 0)
		return Sectors[sector]->ceilh; /* textures are drawn from the ceiling down */
	else
		return 0; /* yuck! */
}


// this type represents one side of a linedef
typedef int side_on_a_line_t;

static inline side_on_a_line_t soal_make(int ld, int side)
{
	return (ld << 1) | (side < 0 ? 1 : 0);
}

static inline int soal_ld(side_on_a_line_t zz)
{
	return (zz >> 1);
}

static inline const LineDef * soal_LD_ptr(side_on_a_line_t zz)
{
	return LineDefs[soal_ld(zz)];
}

static inline int soal_side(side_on_a_line_t zz)
{
	return (zz & 1) ? SIDE_LEFT : SIDE_RIGHT;
}

static inline int soal_sd(side_on_a_line_t zz)
{
	return soal_LD_ptr(zz)->WhatSideDef(soal_side(zz));
}

static inline const SideDef * soal_SD_ptr(side_on_a_line_t zz)
{
	int sd = soal_sd(zz);
	return (sd >= 0) ? SideDefs[sd] : NULL;
}


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


static int ScoreAdjoiner(side_on_a_line_t zz, side_on_a_line_t adj,
						 char part, int align_flags)
{
	const LineDef *L = soal_LD_ptr(zz);
	const LineDef *N = soal_LD_ptr(adj);

	const SideDef *LS = soal_SD_ptr(zz);
	const SideDef *NS = soal_SD_ptr(adj);

	SYS_ASSERT(LS);

	// no sidedef on adjoining line?
	if (! NS)
		return -2;

	// wrong side?
	if (soal_side(zz) == soal_side(adj))
	{
		if (N->start == L->start) return -1;
		if (N->end   == L->end)   return -1;
	}
	else
	{
		if (N->start == L->end)   return -1;
		if (N->end   == L->start) return -1;
	}


	// require adjoiner is "to the left" of this sidedef
	// [or "to the right" if the 'r' flag is present]

	bool on_left = N->TouchesVertex(soal_side(zz) < 0 ? L->end : L->start);

	if (align_flags & LINALIGN_Right)
	{
		if (on_left)
			return -2;
	}
	else
	{
		if (! on_left)
			return -2;
	}

	int score = 1;

	// FIXME : take 'part' into account !!

	// Main requirement is a matching texture.
	// There are three cases depending on number of sides:
	//
	// (a) single <-> single : easy
	//
	// (b) double <-> double : compare lower/lower and upper/upper
	//                         [only need one to match]
	//
	// (c) single <-> double : compare mid/lower and mid/upper
	//
	bool matched = false;

	for (int what = 0 ; what < 2 ; what++)
	{
//???		if (what == 0 && only_U) continue;
//???		if (what == 1 && only_L) continue;

		const char *L_tex = (! L->TwoSided()) ? LS->MidTex() :
							(what & 1)        ? LS->UpperTex() :
							                    LS->LowerTex();

		const char *N_tex = (! N->TwoSided()) ? NS->MidTex() :
							(what & 1)        ? NS->UpperTex() :
							                    NS->LowerTex();

		if (L_tex[0] == '-') continue;
		if (N_tex[0] == '-') continue;

		if (PartialTexCmp(L_tex, N_tex) == 0)
			matched = true;
	}

///---	// require a texture match?
///---	if (/* strchr(flags, 't') && */  ! matched)
///---		return -1;

	if (matched)
		score = score + 20;


	// preference for same sector
	if (LS->sector == NS->sector)
		score = score + 1;

	// prefer both lines to have same sided-ness
	if (L->OneSided() == N->OneSided())
		score = score + 5;

	return score;
}


static side_on_a_line_t DetermineAdjoiner(side_on_a_line_t cur, char part,
                                          int align_flags)
{
	// returns -1 for none

	int best_adj   = -1;
	int best_score = -1;

	const LineDef *L = soal_LD_ptr(cur);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *N = LineDefs[n];

		if (N == L)
			continue;

		if (! (N->TouchesVertex(L->start) || N->TouchesVertex(L->end)))
			continue;

		for (int pass = 0 ; pass < 2 ; pass++)
		{
			int adj_side = pass ? SIDE_LEFT : SIDE_RIGHT;
			int adjoiner = soal_make(n, adj_side);

			int score = ScoreAdjoiner(cur, adjoiner, part, align_flags);

// fprintf(stderr, "Score for %d:%d --> %d\n", n, adj_side, score);

			if (score > 0 && score > best_score)
			{
				best_adj   = adjoiner;
				best_score = score;
			}
		}
	}

	return best_adj;
}


#if 0  // TEST CRUD
int TestAdjoinerLineDef(int ld)
{
	side_on_a_line_t zz = soal_make(ld, SIDE_RIGHT);

	if (soal_sd(zz) < 0)
		return -1;

	side_on_a_line_t result = DetermineAdjoiner(zz, 0, 0);

	if (result >= 0)
		return soal_ld(result);

	return -1;
}
#endif


static int GetTextureHeight(const char *name)
{
	if (name[0] == '-')
		return 128;

	Img_c *img = W_GetTexture(name);

	if (! img)
		return 128;

	return img->height();
}


static bool PartIsVisible(side_on_a_line_t zz, char part)
{
	const LineDef *L  = soal_LD_ptr(zz);
	const SideDef *SD = soal_SD_ptr(zz);

	if (! L->TwoSided())
		return (part == 'l');
	
	const Sector *front = L->Right()->SecRef();
	const Sector *back  = L->Left ()->SecRef();

	if (soal_side(zz) == SIDE_LEFT)
		std::swap(front, back);
	
	// ignore sky walls
	if (part == 'u' && is_sky(front->CeilTex()) && is_sky(back->CeilTex()))
		return false;

	if (part == 'l')
	{
		if (SD->LowerTex()[0] == '-')
			return false;

		return back->floorh > front->floorh;
	}
	else
	{
		if (SD->UpperTex()[0] == '-')
			return false;

		return back->ceilh < front->ceilh;
	}
}


static char PickAdjoinerPart(side_on_a_line_t cur, char part,
							 side_on_a_line_t adj, int align_flags)
{
	const LineDef *L  = soal_LD_ptr(cur);
	const SideDef *SD = soal_SD_ptr(cur);

	const LineDef *adj_L  = soal_LD_ptr(adj);
	const SideDef *adj_SD = soal_SD_ptr(adj);

	if (! adj_L->TwoSided())
		return 'l';

	// the adjoiner part (upper or lower) should be visible

	bool lower_vis = PartIsVisible(adj, 'l');
	bool upper_vis = PartIsVisible(adj, 'u');

	if (lower_vis != upper_vis)
		return upper_vis ? 'u' : 'l';
	else if (! lower_vis)
		return 'l';

	// check for a matching texture

	if (L->TwoSided())
	{
		// TODO: this logic would mean sometimes aligning an upper with
		//       a lower (or vice versa).  This should only be done when
		//       those parts are actually adjacent (on the Y axis).
#if 0
		bool lower_match = (PartialTexCmp(SD->LowerTex(), adj_SD->LowerTex()) == 0);
		bool upper_match = (PartialTexCmp(SD->UpperTex(), adj_SD->UpperTex()) == 0);

		if (lower_match != upper_match)
			return upper_match ? 'u' : 'l';
#endif

		return part;
	}
	else
	{
		bool lower_match = (PartialTexCmp(SD->MidTex(), adj_SD->LowerTex()) == 0);
		bool upper_match = (PartialTexCmp(SD->MidTex(), adj_SD->UpperTex()) == 0);

		if (lower_match != upper_match)
			return upper_match ? 'u' : 'l';
	}

	return part;
}


static int CalcReferenceH(side_on_a_line_t zz, char part)
{
	const LineDef *L  = soal_LD_ptr(zz);
	const SideDef *SD = soal_SD_ptr(zz);

	if (! L->TwoSided())
	{
		if (! L->Right())
			return 256;

		const Sector *front = L->Right()->SecRef();

		if (L->flags & MLF_LowerUnpegged)
			return front->floorh + GetTextureHeight(SD->MidTex());

		return front->ceilh;
	}

	const Sector *front = L->Right()->SecRef();
	const Sector *back  = L->Left ()->SecRef();

	if (soal_side(zz) == SIDE_LEFT)
		std::swap(front, back);

	if (part == 'l')
	{
		if (! (L->flags & MLF_LowerUnpegged))
			return back->floorh;

		return front->ceilh;
	}
	else
	{
		if (! (L->flags & MLF_UpperUnpegged))
			return back->ceilh + GetTextureHeight(SD->UpperTex());

		return front->ceilh;
	}
}


static void DoAlignX(side_on_a_line_t cur, char part,
					 side_on_a_line_t adj, int align_flags)
{
	const LineDef *L  = soal_LD_ptr(cur);

	const LineDef *adj_L  = soal_LD_ptr(adj);
	const SideDef *adj_SD = soal_SD_ptr(adj);

	bool on_left = adj_L->TouchesVertex(soal_side(cur) < 0 ? L->end : L->start);

	int new_offset;

	if (on_left)
	{
		int adj_length = I_ROUND(adj_L->CalcLength());

		new_offset = adj_SD->x_offset + adj_length;

		if (new_offset > 0)
			new_offset &= 1023;
	}
	else
	{
		int length = I_ROUND(L->CalcLength());

		new_offset = adj_SD->x_offset - length;

		if (new_offset < 0)
			new_offset = - (-new_offset & 1023);
	}

	BA_ChangeSD(soal_sd(cur), SideDef::F_X_OFFSET, new_offset);
}


static void DoAlignY(side_on_a_line_t cur, char part,
					 side_on_a_line_t adj, int align_flags)
{
	const LineDef *L  = soal_LD_ptr(cur);
	const SideDef *SD = soal_SD_ptr(cur);

//	const LineDef *adj_L  = soal_LD_ptr(adj);
	const SideDef *adj_SD = soal_SD_ptr(adj);

	bool lower_vis = PartIsVisible(cur, 'l');
	bool upper_vis = PartIsVisible(cur, 'u');


	// handle unpeg flags : check for windows

	if (L->TwoSided() &&
	    (lower_vis && upper_vis) &&
	    ((L->flags & MLF_LowerUnpegged) == 0) &&
	    ((L->flags & MLF_UpperUnpegged) == 0) &&
	    PartialTexCmp(SD->LowerTex(), SD->UpperTex()) == 0 &&
	    SD->MidTex()[0] == '-' /* no rail */)
	{
		int new_flags = L->flags;

		if (lower_vis) new_flags |= MLF_LowerUnpegged;
		if (upper_vis) new_flags |= MLF_UpperUnpegged;

		BA_ChangeLD(soal_ld(cur), LineDef::F_FLAGS, new_flags);
	}

	// determine which parts (upper or lower) we will use for alignment

	char cur_part = part;
	char adj_part = PickAdjoinerPart(cur, part, adj, align_flags);

	// requirement: adj_tex_h + adj_y_off = cur_tex_h + cur_y_off

	int cur_texh = CalcReferenceH(cur, cur_part);
	int adj_texh = CalcReferenceH(adj, adj_part);

	int new_offset = adj_texh + adj_SD->y_offset - cur_texh;

	// normalize value  [TODO: handle BOOM non-power-of-two heights]

	if (new_offset < 0)
		new_offset = - (-new_offset & 255);
	else
		new_offset &= 255;

	BA_ChangeSD(soal_sd(cur), SideDef::F_Y_OFFSET, new_offset);
}


void LineDefs_Align(int ld, int side, int sd, char part, int align_flags)
{
	side_on_a_line_t cur = soal_make(ld, side);

	side_on_a_line_t adj = DetermineAdjoiner(cur, part, align_flags);

	if (adj < 0)
	{
		Beep("No nearby wall to align with");
		return;
	}

	BA_Begin();

	if (align_flags & LINALIGN_X)
		DoAlignX(cur, part, adj, align_flags);

	if (align_flags & LINALIGN_Y)
		DoAlignY(cur, part, adj, align_flags);

	BA_End();
}


//------------------------------------------------------------------------


void FlipLineDef(int ld)
{
	int old_start = LineDefs[ld]->start;
	int old_end   = LineDefs[ld]->end;

	BA_ChangeLD(ld, LineDef::F_START, old_end);
	BA_ChangeLD(ld, LineDef::F_END, old_start);

	int old_right = LineDefs[ld]->right;
	int old_left  = LineDefs[ld]->left;

	BA_ChangeLD(ld, LineDef::F_RIGHT, old_left);
	BA_ChangeLD(ld, LineDef::F_LEFT, old_right);
}


void FlipLineDefGroup(selection_c& flip)
{
	selection_iterator_c it;

	for (flip.begin(&it) ; !it.at_end() ; ++it)
	{
		FlipLineDef(*it);
	}
}


/*
   flip one or several LineDefs
*/
void LIN_Flip(void)
{
	selection_c list;

	if (! GetCurrentObjects(&list))
	{
		Beep("No lines to flip");
		return;
	}

	BA_Begin();

	FlipLineDefGroup(list);

	BA_End();
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
			L->Start()->x - L->End()->x,
			L->Start()->y - L->End()->y);

	int new_length = (int)ComputeDist(
			L->Start()->x - V->x,
			L->Start()->y - V->y);

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

	int new_x = (L->Start()->x + L->End()->x) / 2;
	int new_y = (L->Start()->y + L->End()->y) / 2;

	// prevent creating tiny lines (especially zero-length)
	if (abs(L->Start()->x - L->End()->x) < 4 &&
	    abs(L->Start()->y - L->End()->y) < 4)
		return false;

	int new_v = BA_New(OBJ_VERTICES);

	Vertex * V = Vertices[new_v];

	V->x = new_x;
	V->y = new_y;

	SplitLineDefAtVertex(ld, new_v);

	return true;
}


/*
   split one or more LineDefs in two, adding new Vertices in the middle
*/
void LIN_SplitHalf(void)
{
	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No lines to split");
		return;
	}

	bool was_selected = edit.Selected->notempty();

	// clear current selection, since the size needs to grow due to
	// new linedefs being added to the map.
	Selection_Clear(true);

	int new_first = NumLineDefs;
	int new_count = 0;

	BA_Begin();

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		if (DoSplitLineDef(*it))
			new_count++;
	}

	BA_End();

	// Hmmmmm -- should abort early if some lines are too short??
	if (new_count < list.count_obj())
		Beep("Some lines were too short!");

	if (was_selected && new_count > 0)
	{
		// reselect the old _and_ new linedefs
		for (list.begin(&it) ; !it.at_end() ; ++it)
			edit.Selected->set(*it);

		edit.Selected->frob_range(new_first, new_first + new_count - 1, BOP_ADD);
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

	// FIXME: make this a global pseudo-constant
	int null_tex = BA_InternaliseString("-");

	// determine textures for each side
	int  left_tex = 0;
	int right_tex = 0;

	if (isalnum(L->Left()->MidTex()[0]))
		left_tex = L->Left()->mid_tex;

	if (isalnum(L->Right()->MidTex()[0]))
		right_tex = L->Right()->mid_tex;
	
	if (! left_tex)  left_tex = right_tex;
	if (! right_tex) right_tex = left_tex;

	// use default texture if both sides are empty
	if (! left_tex)
	{
		 left_tex = BA_InternaliseString(default_mid_tex);
		right_tex = left_tex;
	}

	BA_ChangeSD(L->left,  SideDef::F_MID_TEX, null_tex);
	BA_ChangeSD(L->right, SideDef::F_MID_TEX, null_tex);

	BA_ChangeSD(L->left,  SideDef::F_LOWER_TEX, left_tex);
	BA_ChangeSD(L->left,  SideDef::F_UPPER_TEX, left_tex);

	BA_ChangeSD(L->right, SideDef::F_LOWER_TEX, right_tex);
	BA_ChangeSD(L->right, SideDef::F_UPPER_TEX, right_tex);
}


void LIN_MergeTwo(void)
{
	if (edit.Selected->count_obj() == 1 && edit.highlight.valid())
	{
		edit.Selected->set(edit.highlight.num);
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

	DeleteLineDefs(&del_line);

	BA_End();
}


void MoveCoordOntoLineDef(int ld, int *x, int *y)
{
	const LineDef *L = LineDefs[ld];

	double x1 = L->Start()->x;
	double y1 = L->Start()->y;
	double x2 = L->End()->x;
	double y2 = L->End()->y;

	double dx = x2 - x1;
	double dy = y2 - y1;

	double len_squared = dx*dx + dy*dy;

	SYS_ASSERT(len_squared > 0);

	// compute along distance
	double along = (*x - x1) * dx + (*y - y1) * dy;

	// result = start + along * line unit vector
	double new_x = x1 + along * dx / len_squared;
	double new_y = y1 + along * dy / len_squared;

	*x = I_ROUND(new_x);
	*y = I_ROUND(new_y);
}


static bool LineDefStartWillBeMoved(int ld, selection_c& list)
{
	int start = LineDefs[ld]->start;

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		if (*it == ld)
			continue;

		const LineDef *L = LineDefs[*it];

		if (L->end == start)
			return true;
	}

	return false;
}


static int PickLineDefToExtend(selection_c& list)
{
	// we want a line whose start is not going to be moved in the future
	// (otherwise the length will be wrecked by the later change).
	// however there could be loops, so need to always pick something.

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
		if (! LineDefStartWillBeMoved(*it, list))
			return *it;

	return list.find_first();
}


static void LD_SetLength(int ld, int new_len, int angle)
{
	const LineDef *L = LineDefs[ld];

	double dx = new_len * cos(angle * M_PI / 32768.0);
	double dy = new_len * sin(angle * M_PI / 32768.0);

	int idx = I_ROUND(dx);
	int idy = I_ROUND(dy);

	if (idx == 0 && idy == 0)
	{
		if (dx < 0) idx = (int)floor(dx); else idx = (int)ceil(dx);
		if (dy < 0) idy = (int)floor(dy); else idy = (int)ceil(dy);
	}

	if (idx == 0 && idy == 0)
		idx = 1;

	BA_ChangeVT(L->end, Vertex::F_X, L->Start()->x + idx);
	BA_ChangeVT(L->end, Vertex::F_Y, L->Start()->y + idy);
}


void LineDefs_SetLength(int new_len)
{
	selection_c list;

	if (! GetCurrentObjects(&list))
	{
		Beep("No lines to extend");
		return;
	}

	// remember angles
	std::vector<int> angles(NumLineDefs);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		angles[n] = ComputeAngle(L->End()->x - L->Start()->x,
								 L->End()->y - L->Start()->y);
	}

	BA_Begin();

	while (! list.empty())
	{
		int ld = PickLineDefToExtend(list);

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

	if (L->Right()->LowerTex()[0] != '-')
		tex = L->Right()->lower_tex;
	else if (L->Right()->UpperTex()[0] != '-')
		tex = L->Right()->upper_tex;
	else
		tex = BA_InternaliseString(default_mid_tex);

	BA_ChangeSD(L->right, SideDef::F_MID_TEX, tex);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
