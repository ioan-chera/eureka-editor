//------------------------------------------------------------------------
//  VERTEX OPERATIONS
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
#include "e_hover.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_objects.h"
#include "e_vertex.h"
#include "m_bitvec.h"
#include "r_grid.h"
#include "m_game.h"
#include "e_objects.h"
#include "w_rawdef.h"

#include <algorithm>


int Vertex_FindExact(fixcoord_t fx, fixcoord_t fy)
{
	for (int i = 0 ; i < NumVertices ; i++)
	{
		if (Vertices[i]->Matches(fx, fy))
			return i;
	}

	return -1;  // not found
}


int Vertex_HowManyLineDefs(int v_num)
{
	int count = 0;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		if (L->start == v_num || L->end == v_num)
			count++;
	}

	return count;
}


//
// two linedefs are being sandwiched together.
// vertex 'v' is the shared vertex (the "hinge").
// to prevent an overlap, we merge ld1 into ld2.
//
static void MergeSandwichLines(int ld1, int ld2, int v, selection_c& del_lines)
{
	LineDef *L1 = LineDefs[ld1];
	LineDef *L2 = LineDefs[ld2];

	bool ld1_onesided = L1->OneSided();
	bool ld2_onesided = L2->OneSided();

	int new_mid_tex = (ld1_onesided) ? L1->Right()->mid_tex :
					  (ld2_onesided) ? L2->Right()->mid_tex : 0;

	// flip L1 so it would be parallel with L2 (after merging the other
	// endpoint) but going the opposite direction.
	if ((L2->end == v) == (L1->end == v))
	{
		FlipLineDef(ld1);
	}

	bool same_left  = (L2->WhatSector(SIDE_LEFT)  == L1->WhatSector(SIDE_LEFT));
	bool same_right = (L2->WhatSector(SIDE_RIGHT) == L1->WhatSector(SIDE_RIGHT));

	if (same_left && same_right)
	{
		// the merged line would have the same thing on both sides
		// (possibly VOID space), so the linedefs both "vanish".

		del_lines.set(ld1);
		del_lines.set(ld2);
		return;
	}

	if (same_left)
	{
		BA_ChangeLD(ld2, LineDef::F_LEFT, L1->right);
	}
	else if (same_right)
	{
		BA_ChangeLD(ld2, LineDef::F_RIGHT, L1->left);
	}
	else
	{
		// geometry was broken / unclosed sector(s)
	}

	del_lines.set(ld1);


	// fix orientation of remaining linedef if needed
	if (L2->Left() && ! L2->Right())
	{
		FlipLineDef(ld2);
	}

	if (L2->OneSided() && new_mid_tex > 0)
	{
		BA_ChangeSD(L2->right, SideDef::F_MID_TEX, new_mid_tex);
	}

	// fix flags of remaining linedef
	int new_flags = L2->flags;

	if (L2->TwoSided())
	{
		new_flags |=  MLF_TwoSided;
		new_flags &= ~MLF_Blocking;
	}
	else
	{
		new_flags &= ~MLF_TwoSided;
		new_flags |=  MLF_Blocking;
	}

	BA_ChangeLD(ld2, LineDef::F_FLAGS, new_flags);
}


//
// merge v1 into v2
//
static void DoMergeVertex(int v1, int v2, selection_c& del_lines)
{
	SYS_ASSERT(v1 >= 0 && v2 >= 0);
	SYS_ASSERT(v1 != v2);

	// check if two linedefs would overlap after the merge
	// [ but ignore lines already marked for deletion ]

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (! L->TouchesVertex(v1))
			continue;

		if (del_lines.get(n))
			continue;

		int v3 = (L->start == v1) ? L->end : L->start;

		int found = -1;

		for (int k = 0 ; k < NumLineDefs ; k++)
		{
			if (k == n)
				continue;

			const LineDef *K = LineDefs[k];

			if ((K->start == v3 && K->end == v2) ||
				(K->start == v2 && K->end == v3))
			{
				found = k;
				break;
			}
		}

		if (found >= 0 && ! del_lines.get(found))
		{
			MergeSandwichLines(n, found, v3, del_lines);
			break;
		}
	}

	// update all linedefs which use V1 to use V2 instead, and
	// delete any line that exists between the two vertices.

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		// change *ALL* references, this is critical
		// [ to-be-deleted lines will get start == end, that is OK ]

		if (L->start == v1)
			BA_ChangeLD(n, LineDef::F_START, v2);

		if (L->end == v1)
			BA_ChangeLD(n, LineDef::F_END, v2);

		if (L->start == v2 && L->end == v2)
			del_lines.set(n);
	}
}


//
// the first vertex is kept, all the other vertices are deleted
// (after fixing the attached linedefs).
//
void Vertex_MergeList(selection_c *verts)
{
	if (verts->count_obj() < 2)
		return;

	int v = verts->find_first();

#if 0
	double new_x, new_y;
	Objs_CalcMiddle(verts, &new_x, &new_y);

	BA_ChangeVT(v, Vertex::F_X, MakeValidCoord(new_x));
	BA_ChangeVT(v, Vertex::F_Y, MakeValidCoord(new_y));
#endif

	verts->clear(v);

	selection_c del_lines(OBJ_LINEDEFS);
	selection_iterator_c it;

	// this prevents unnecessary sandwich mergers
	ConvertSelection(verts, &del_lines);

	for (verts->begin(&it) ; !it.at_end() ; ++it)
	{
		DoMergeVertex(*it, v, del_lines);
	}

	// all these vertices will be unused now, hence this call
	// shouldn't kill any other objects.
	DeleteObjects(verts);

	// we NEED to keep unused vertices here, otherwise we can merge
	// all vertices of an isolated sector and end up with NOTHING!
	DeleteObjects_WithUnused(&del_lines, false /* keep_things */, true /* keep_verts */, false /* keep_lines */);

	verts->clear_all();
}


void CMD_VT_Merge()
{
	if (edit.Selected->count_obj() == 1 && edit.highlight.valid())
	{
		Selection_Add(edit.highlight);
	}

	if (edit.Selected->count_obj() < 2)
	{
		Beep("Need 2 or more vertices to merge");
		return;
	}

	BA_Begin();
	BA_MessageForSel("merged", edit.Selected);

	Vertex_MergeList(edit.Selected);

	BA_End();

	Selection_Clear(true /* no_save */);
}


bool Vertex_TryFixDangler(int v_num)
{
	// see if this vertex is sitting on another one (or very close to it)
	int v_other  = -1;
	int max_dist = 2;

	for (int i = 0 ; i < NumVertices ; i++)
	{
		if (i == v_num)
			continue;

		double dx = Vertices[v_num]->x() - Vertices[i]->x();
		double dy = Vertices[v_num]->y() - Vertices[i]->y();

		if (abs(dx) <= max_dist && abs(dy) <= max_dist &&
			! LineDefAlreadyExists(v_num, v_other))
		{
			v_other = i;
			break;
		}
	}


	// check for a dangling vertex
	if (Vertex_HowManyLineDefs(v_num) != 1)
	{
		if (v_other >= 0 && Vertex_HowManyLineDefs(v_other) == 1)
			std::swap(v_num, v_other);
		else
			return false;
	}


	if (v_other >= 0)
	{
		Selection_Clear(true /* no_save */);

		// delete highest numbered one  [ so the other index remains valid ]
		if (v_num < v_other)
			std::swap(v_num, v_other);

#if 0 // DEBUG
		fprintf(stderr, "Vertex_TryFixDangler : merge vert %d onto %d\n", v_num, v_other);
#endif

		BA_Begin();
		BA_Message("merged dangling vertex #%d\n", v_num);

		selection_c list(OBJ_VERTICES);

		list.set(v_other);	// first one is the one kept
		list.set(v_num);

		Vertex_MergeList(&list);

		BA_End();

		edit.Selected->set(v_other);

		Beep("Merged a dangling vertex");
		return true;
	}


#if 0
	// find the line joined to this vertex
	int joined_ld = -1;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		if (LineDefs[n]->TouchesVertex(v_num))
		{
			joined_ld = n;
			break;
		}
	}

	SYS_ASSERT(joined_ld >= 0);
#endif


	// see if vertex is sitting on a line

	Objid line_ob;

	GetSplitLineForDangler(line_ob, v_num);

	if (! line_ob.valid())
		return false;

#if 0 // DEBUG
	fprintf(stderr, "Vertex_TryFixDangler : split linedef %d with vert %d\n", line_ob.num, v_num);
#endif

	BA_Begin();

	SplitLineDefAtVertex(line_ob.num, v_num);

	BA_Message("split linedef #%d\n", line_ob.num);

	BA_End();

	// no vertices were added or removed, hence can continue Insert_Vertex
	return false;
}


static void CalcDisconnectCoord(const LineDef *L, int v_num, double *x, double *y)
{
	const Vertex * V = Vertices[v_num];

	double dx = L->End()->x() - L->Start()->x();
	double dy = L->End()->y() - L->Start()->y();

	if (L->end == v_num)
	{
		dx = -dx;
		dy = -dy;
	}

	if (abs(dx) < 4 && abs(dy) < 4)
	{
		dx = dx / 2;
		dy = dy / 2;
	}
	else if (abs(dx) < 16 && abs(dy) < 16)
	{
		dx = dx / 4;
		dy = dy / 4;
	}
	else if (abs(dx) >= abs(dy))
	{
		dy = dy * 8 / abs(dx);
		dx = (dx < 0) ? -8 : 8;
	}
	else
	{
		dx = dx * 8 / abs(dy);
		dy = (dy < 0) ? -8 : 8;
	}

	*x = V->x() + dx;
	*y = V->y() + dy;
}


static void DoDisconnectVertex(int v_num, int num_lines)
{
	int which = 0;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		if (L->start == v_num || L->end == v_num)
		{
			double new_x, new_y;
			CalcDisconnectCoord(L, v_num, &new_x, &new_y);

			// the _LAST_ linedef keeps the current vertex, the rest
			// need a new one.
			if (which != num_lines-1)
			{
				int new_v = BA_New(OBJ_VERTICES);

				Vertices[new_v]->SetRawXY(new_x, new_y);

				if (L->start == v_num)
					BA_ChangeLD(n, LineDef::F_START, new_v);
				else
					BA_ChangeLD(n, LineDef::F_END, new_v);
			}
			else
			{
				BA_ChangeVT(v_num, Vertex::F_X, MakeValidCoord(new_x));
				BA_ChangeVT(v_num, Vertex::F_Y, MakeValidCoord(new_y));
			}

			which++;
		}
	}
}


void CMD_VT_Disconnect(void)
{
	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("Nothing to disconnect");
			return;
		}

		Selection_Add(edit.highlight);
	}

	bool seen_one = false;

	BA_Begin();

	BA_MessageForSel("disconnected", edit.Selected);

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		int v_num = *it;

		// nothing to do unless vertex has 2 or more linedefs
		int num_lines = Vertex_HowManyLineDefs(*it);

		if (num_lines < 2)
			continue;

		DoDisconnectVertex(v_num, num_lines);

		seen_one = true;
	}

	if (! seen_one)
		Beep("Nothing was disconnected");

	BA_End();

	Selection_Clear(true);
}


static void DoDisconnectLineDef(int ld, int which_vert, bool *seen_one)
{
	LineDef *L = LineDefs[ld];

	int v_num = which_vert ? L->end : L->start;

	// see if there are any linedefs NOT in the selection which are
	// connected to this vertex.

	bool touches_non_sel = false;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		if (edit.Selected->get(n))
			continue;

		LineDef *N = LineDefs[n];

		if (N->start == v_num || N->end == v_num)
		{
			touches_non_sel = true;
			break;
		}
	}

	if (! touches_non_sel)
		return;

	double new_x, new_y;
	CalcDisconnectCoord(LineDefs[ld], v_num, &new_x, &new_y);

	int new_v = BA_New(OBJ_VERTICES);

	Vertices[new_v]->SetRawXY(new_x, new_y);

	// fix all linedefs in the selection to use this new vertex

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		LineDef *L2 = LineDefs[*it];

		if (L2->start == v_num)
			BA_ChangeLD(*it, LineDef::F_START, new_v);

		if (L2->end == v_num)
			BA_ChangeLD(*it, LineDef::F_END, new_v);
	}

	*seen_one = true;
}


void CMD_LIN_Disconnect(void)
{
	// Note: the logic here is significantly different than the logic
	//       in VT_Disconnect, since we want to keep linedefs in the
	//       selection connected, and only disconnect from linedefs
	//       NOT in the selection.
	//
	// Hence need separate code for this.

	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("Nothing to disconnect");
		return;
	}

	bool seen_one = false;

	BA_Begin();

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		DoDisconnectLineDef(*it, 0, &seen_one);
		DoDisconnectLineDef(*it, 1, &seen_one);
	}

	BA_MessageForSel("disconnected", edit.Selected);

	BA_End();

	if (! seen_one)
		Beep("Nothing was disconnected");

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* no save */);
}


static void VerticesOfDetachableSectors(selection_c &verts)
{
	SYS_ASSERT(NumVertices > 0);

	bitvec_c  in_verts(NumVertices);
	bitvec_c out_verts(NumVertices);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef * L = LineDefs[n];

		// only process lines which touch a selected sector
		bool  left_in = L->Left()  && edit.Selected->get(L->Left()->sector);
		bool right_in = L->Right() && edit.Selected->get(L->Right()->sector);

		if (! (left_in || right_in))
			continue;

		bool innie = false;
		bool outie = false;

		if (L->Right())
		{
			if (right_in)
				innie = true;
			else
				outie = true;
		}

		if (L->Left())
		{
			if (left_in)
				innie = true;
			else
				outie = true;
		}

		if (innie)
		{
			in_verts.set(L->start);
			in_verts.set(L->end);
		}

		if (outie)
		{
			out_verts.set(L->start);
			out_verts.set(L->end);
		}
	}

	for (int k = 0 ; k < NumVertices ; k++)
	{
		if (in_verts.get(k) && out_verts.get(k))
			verts.set(k);
	}
}


static void DETSEC_SeparateLine(int ld_num, int start2, int end2, int in_side)
{
	const LineDef * L1 = LineDefs[ld_num];

	int new_ld = BA_New(OBJ_LINEDEFS);
	int lost_sd;

	LineDef * L2 = LineDefs[new_ld];

	if (in_side == SIDE_LEFT)
	{
		L2->start = end2;
		L2->end   = start2;
		L2->right = L1->left;

		lost_sd = L1->left;
	}
	else
	{
		L2->start = start2;
		L2->end   = end2;
		L2->right = L1->right;

		lost_sd = L1->right;

		FlipLineDef(ld_num);
	}

	BA_ChangeLD(ld_num, LineDef::F_LEFT, -1);


	// determine new flags

	int new_flags = L1->flags;

	new_flags &= ~MLF_TwoSided;
	new_flags |=  MLF_Blocking;

	BA_ChangeLD(ld_num, LineDef::F_FLAGS, new_flags);

	L2->flags = L1->flags;


	// fix the first line's textures

	int tex = BA_InternaliseString(default_wall_tex);

	const SideDef * SD = SideDefs[L1->right];

	if (! is_null_tex(SD->LowerTex()))
		tex = SD->lower_tex;
	else if (! is_null_tex(SD->UpperTex()))
		tex = SD->upper_tex;

	BA_ChangeSD(L1->right, SideDef::F_MID_TEX, tex);


	// now fix the second line's textures

	SD = SideDefs[lost_sd];

	if (! is_null_tex(SD->LowerTex()))
		tex = SD->lower_tex;
	else if (! is_null_tex(SD->UpperTex()))
		tex = SD->upper_tex;

	BA_ChangeSD(lost_sd, SideDef::F_MID_TEX, tex);
}


static void DETSEC_CalcMoveVector(selection_c * detach_verts, double * dx, double * dy)
{
	double det_mid_x, sec_mid_x;
	double det_mid_y, sec_mid_y;

	Objs_CalcMiddle(detach_verts,  &det_mid_x, &det_mid_y);
	Objs_CalcMiddle(edit.Selected, &sec_mid_x, &sec_mid_y);

	*dx = sec_mid_x - det_mid_x;
	*dy = sec_mid_y - det_mid_y;

	// avoid moving perfectly horizontal or vertical
	// (also handes the case of dx == dy == 0)

	if (abs(*dx) > abs(*dy))
	{
		*dx = (*dx < 0) ? -9 : +9;
		*dy = (*dy < 0) ? -5 : +5;
	}
	else
	{
		*dx = (*dx < 0) ? -5 : +5;
		*dy = (*dy < 0) ? -9 : +9;
	}

	if (abs(*dx) < 2) *dx = (*dx < 0) ? -2 : +2;
	if (abs(*dy) < 4) *dy = (*dy < 0) ? -4 : +4;

	double mul = 1.0 / CLAMP(0.25, grid.Scale, 1.0);

	*dx = (*dx) * mul;
	*dy = (*dy) * mul;
}


void CMD_SEC_Disconnect(void)
{
	if (NumVertices == 0)
	{
		Beep("No sectors to disconnect");
		return;
	}

	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("No sectors to disconnect");
		return;
	}

	int n;

	// collect all vertices which need to be detached
	selection_c detach_verts(OBJ_VERTICES);
	selection_iterator_c it;

	VerticesOfDetachableSectors(detach_verts);

	if (detach_verts.empty())
	{
		Beep("Already disconnected");
		if (unselect == SOH_Unselect)
			Selection_Clear(true /* nosave */);
		return;
	}


	// determine vector to move the detach coords
	double move_dx, move_dy;
	DETSEC_CalcMoveVector(&detach_verts, &move_dx, &move_dy);


	BA_Begin();

	BA_MessageForSel("disconnected", edit.Selected);

	// create new vertices, and a mapping from old --> new

	int * mapping = new int[NumVertices];

	for (n = 0 ; n < NumVertices ; n++)
		mapping[n] = -1;

	for (detach_verts.begin(&it) ; !it.at_end() ; ++it)
	{
		int new_v = BA_New(OBJ_VERTICES);

		mapping[*it] = new_v;

		Vertex *newbie = Vertices[new_v];

		newbie->RawCopy(Vertices[*it]);
	}

	// update linedefs, creating new ones where necessary
	// (go backwards so we don't visit newly created lines)

	for (n = NumLineDefs-1 ; n >= 0 ; n--)
	{
		const LineDef * L = LineDefs[n];

		// only process lines which touch a selected sector
		bool  left_in = L->Left()  && edit.Selected->get(L->Left()->sector);
		bool right_in = L->Right() && edit.Selected->get(L->Right()->sector);

		if (! (left_in || right_in))
			continue;

		bool between_two = (left_in && right_in);

		int start2 = mapping[L->start];
		int end2   = mapping[L->end];

		if (start2 >= 0 && end2 >= 0 && L->TwoSided() && ! between_two)
		{
			DETSEC_SeparateLine(n, start2, end2, left_in ? SIDE_LEFT : SIDE_RIGHT);
		}
		else
		{
			if (start2 >= 0)
				BA_ChangeLD(n, LineDef::F_START, start2);

			if (end2 >= 0)
				BA_ChangeLD(n, LineDef::F_END, end2);
		}
	}

	delete[] mapping;

	// finally move all vertices of selected sectors

	selection_c all_verts(OBJ_VERTICES);

	ConvertSelection(edit.Selected, &all_verts);

	for (all_verts.begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex * V = Vertices[*it];

		BA_ChangeVT(*it, Vertex::F_X, V->raw_x + MakeValidCoord(move_dx));
		BA_ChangeVT(*it, Vertex::F_Y, V->raw_y + MakeValidCoord(move_dy));
	}

	BA_End();

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);
}


//------------------------------------------------------------------------
//   RESHAPING STUFF
//------------------------------------------------------------------------


static double WeightForVertex(const Vertex *V, /* bbox: */ double x1, double y1, double x2, double y2,
							  double width, double height, int side)
{
	double dist;
	double extent;

	if (width >= height)
	{
		dist = (side < 0) ? (V->x() - x1) : (x2 - V->x());
		extent = width;
	}
	else
	{
		dist = (side < 0) ? (V->y() - y1) : (y2 - V->y());
		extent = height;
	}

	if (dist > extent * 0.66)
		return 0;

	if (dist > extent * 0.33)
		return 0.25;

	return 1.0;
}


struct vert_along_t
{
	int vert_num;

	double along;

public:
	vert_along_t(int num, double _along) : vert_num(num), along(_along)
	{ }

	struct CMP
	{
		inline bool operator() (const vert_along_t &A, const vert_along_t& B) const
		{
			return A.along < B.along;
		}
	};
};


void CMD_VT_ShapeLine(void)
{
	if (edit.Selected->count_obj() < 3)
	{
		Beep("Need 3 or more vertices to shape");
		return;
	}

	// determine orientation and position of the line

	double x1, y1, x2, y2;
	Objs_CalcBBox(edit.Selected, &x1, &y1, &x2, &y2);

	double width  = x2 - x1;
	double height = y2 - y1;

	if (width < 4 && height < 4)
	{
		Beep("Too small");
		return;
	}

	// The method here is where split the vertices into two groups and
	// use the center of each group to form the infinite line.  I have
	// modified that a bit, the vertices in a band near the middle all
	// participate in the sum but at 0.25 weighting.  -AJA-

	double ax = 0;
	double ay = 0;
	double a_total = 0;

	double bx = 0;
	double by = 0;
	double b_total = 0;

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex *V = Vertices[*it];

		double weight = WeightForVertex(V, x1,y1, x2,y2, width,height, -1);

		if (weight > 0)
		{
			ax += V->x() * weight;
			ay += V->y() * weight;

			a_total += weight;
		}

		weight = WeightForVertex(V, x1,y1, x2,y2, width,height, +1);

		if (weight > 0)
		{
			bx += V->x() * weight;
			by += V->y() * weight;

			b_total += weight;
		}
	}

	SYS_ASSERT(a_total > 0);
	SYS_ASSERT(b_total > 0);

	ax /= a_total;
	ay /= a_total;

	bx /= b_total;
	by /= b_total;


	// check the two end points are not too close
	double unit_x = (bx - ax);
	double unit_y = (by - ay);

	double unit_len = hypot(unit_x, unit_y);

	if (unit_len < 2)
	{
		Beep("Cannot determine line");
		return;
	}

	unit_x /= unit_len;
	unit_y /= unit_len;


	// collect all vertices and determine where along the line they are,
	// then sort them based on their along value.

	std::vector< vert_along_t > along_list;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex *V = Vertices[*it];

		vert_along_t ALONG(*it, AlongDist(V->x(), V->y(), ax,ay, bx,by));

		along_list.push_back(ALONG);
	}

	std::sort(along_list.begin(), along_list.end(), vert_along_t::CMP());


	// compute proper positions for start and end of the line
	const Vertex *V1 = Vertices[along_list.front().vert_num];
	const Vertex *V2 = Vertices[along_list. back().vert_num];

	double along1 = along_list.front().along;
	double along2 = along_list. back().along;

	if ((true) /* don't move first and last vertices */)
	{
		ax = V1->x();
		ay = V1->y();

		bx = V2->x();
		by = V2->y();
	}
	else
	{
		bx = ax + along2 * unit_x;
		by = ay + along2 * unit_y;

		ax = ax + along1 * unit_x;
		ay = ay + along1 * unit_y;
	}


	BA_Begin();

	BA_Message("shaped %d vertices", (int)along_list.size());

	for (unsigned int i = 0 ; i < along_list.size() ; i++)
	{
		double frac;

		if ((true) /* regular spacing */)
			frac = i / (double)(along_list.size() - 1);
		else
			frac = (along_list[i].along - along1) / (along2 - along1);

		// ANOTHER OPTION: use distances between neighbor verts...

		double nx = ax + (bx - ax) * frac;
		double ny = ay + (by - ay) * frac;

		BA_ChangeVT(along_list[i].vert_num, Thing::F_X, MakeValidCoord(nx));
		BA_ChangeVT(along_list[i].vert_num, Thing::F_Y, MakeValidCoord(ny));
	}

	BA_End();
}


static double BiggestGapAngle(std::vector< vert_along_t > &along_list,
							  unsigned int *start_idx)
{
	double best_diff = 0;
	double best_low  = 0;

	*start_idx = 0;

	for (unsigned int i = 0 ; i < along_list.size() ; i++)
	{
		unsigned int k = i + 1;

		if (k >= along_list.size())
			k = 0;

		double low  = along_list[i].along;
		double high = along_list[k].along;

		if (high < low)
			high = high + M_PI * 2.0;

		double ang_diff = high - low;

		if (ang_diff > best_diff)
		{
			best_diff  = ang_diff;
			best_low   = low;
			*start_idx = k;
		}
	}

	double best = best_low + best_diff * 0.5;

	return best;
}


static double EvaluateCircle(double mid_x, double mid_y, double r,
							 std::vector< vert_along_t > &along_list,
							 unsigned int start_idx, double arc_rad,
							 double ang_offset /* radians */,
							 bool move_vertices = false)
{
	double cost = 0;

	bool partial_circle = (arc_rad < M_PI * 1.9);

	for (unsigned int i = 0 ; i < along_list.size() ; i++)
	{
		unsigned int k = (start_idx + i) % along_list.size();

		const Vertex *V = Vertices[along_list[k].vert_num];

		double frac = i / (double)(along_list.size() - (partial_circle ? 1 : 0));

		double ang = arc_rad * frac + ang_offset;

		double new_x = mid_x + cos(ang) * r;
		double new_y = mid_y + sin(ang) * r;

		if (move_vertices)
		{
			BA_ChangeVT(along_list[k].vert_num, Thing::F_X, MakeValidCoord(new_x));
			BA_ChangeVT(along_list[k].vert_num, Thing::F_Y, MakeValidCoord(new_y));
		}
		else
		{
			double dx = new_x - V->x();
			double dy = new_y - V->y();

			cost = cost + (dx*dx + dy*dy);
		}
	}

	return cost;
}


void CMD_VT_ShapeArc(void)
{
	if (! EXEC_Param[0][0])
	{
		Beep("VT_ShapeArc: missing angle parameter");
		return;
	}

	int arc_deg = atoi(EXEC_Param[0]);

	if (arc_deg < 30 || arc_deg > 360)
	{
		Beep("VT_ShapeArc: bad angle: %s", EXEC_Param[0]);
		return;
	}

	double arc_rad = arc_deg * M_PI / 180.0;


	if (edit.Selected->count_obj() < 3)
	{
		Beep("Need 3 or more vertices to shape");
		return;
	}


	// determine middle point for circle
	double x1, y1, x2, y2;
	Objs_CalcBBox(edit.Selected, &x1, &y1, &x2, &y2);

	double width  = x2 - x1;
	double height = y2 - y1;

	if (width < 4 && height < 4)
	{
		Beep("Too small");
		return;
	}

	double mid_x = (x1 + x2) * 0.5;
	double mid_y = (y1 + y2) * 0.5;


	// collect all vertices and determine their angle (in radians),
	// and sort them.
	//
	// also determine radius of circle -- average of distances between
	// the computed mid-point and each vertex.

	double r = 0;

	std::vector< vert_along_t > along_list;

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex *V = Vertices[*it];

		double dx = V->x() - mid_x;
		double dy = V->y() - mid_y;

		double dist = hypot(dx, dy);

		if (dist < 4)
		{
			Beep("Strange shape");
			return;
		}

		r += dist;

		double angle = atan2(dy, dx);

		vert_along_t ALONG(*it, angle);

		along_list.push_back(ALONG);
	}

	r /= (double)along_list.size();

	std::sort(along_list.begin(), along_list.end(), vert_along_t::CMP());


	// where is the biggest gap?
	unsigned int start_idx;
	unsigned int end_idx;

	double gap_angle = BiggestGapAngle(along_list, &start_idx);

	double gap_dx = cos(gap_angle);
	double gap_dy = sin(gap_angle);


	if (start_idx > 0)
		end_idx = start_idx - 1;
	else
		end_idx = static_cast<unsigned>(along_list.size() - 1);

	const Vertex * start_V = Vertices[along_list[start_idx].vert_num];
	const Vertex * end_V   = Vertices[along_list[  end_idx].vert_num];

	double start_end_dist = hypot(end_V->x() - start_V->x(), end_V->y() - start_V->y());


	// compute new mid-point and radius (except for a full circle)
	// and also compute starting angle.

	double best_offset = 0;
	double best_cost = 1e30;

	if (arc_deg < 360)
	{
		mid_x = (start_V->x() + end_V->x()) * 0.5;
		mid_y = (start_V->y() + end_V->y()) * 0.5;

		r = start_end_dist * 0.5;

		double dx = gap_dx;
		double dy = gap_dy;

		if (arc_deg > 180)
		{
			dx = -dx;
			dy = -dy;
		}

		double theta = fabs(arc_rad - M_PI) / 2.0;

		double away = r * tan(theta);

		mid_x += dx * away;
		mid_y += dy * away;

		r = hypot(r, away);

		best_offset = atan2(start_V->y() - mid_y, start_V->x() - mid_x);
	}
	else
	{
		// find the best orientation, the one that minimises the distances
		// which vertices move.  We try 1000 possibilities.

		for (int pos = 0 ; pos < 1000 ; pos++)
		{
			double ang_offset = pos * M_PI * 2.0 / 1000.0;

			double cost = EvaluateCircle(mid_x, mid_y, r, along_list,
										 start_idx, arc_rad, ang_offset, false);

			if (cost < best_cost)
			{
				best_offset = ang_offset;
				best_cost   = cost;
			}
		}
	}


	// actually move stuff now

	BA_Begin();

	BA_Message("shaped %d vertices", (int)along_list.size());

	EvaluateCircle(mid_x, mid_y, r, along_list, start_idx, arc_rad,
				   best_offset, true);

	BA_End();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
