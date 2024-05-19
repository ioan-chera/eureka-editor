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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include "e_cutpaste.h"
#include "e_hover.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_objects.h"
#include "e_vertex.h"
#include "LineDef.h"
#include "m_bitvec.h"
#include "r_grid.h"
#include "m_game.h"
#include "e_objects.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"
#include "w_rawdef.h"

#include <algorithm>


int VertexModule::findExact(FFixedPoint fx, FFixedPoint fy) const
{
	for (int i = 0 ; i < doc.numVertices() ; i++)
	{
		if (doc.vertices[i]->Matches(fx, fy))
			return i;
	}

	return -1;  // not found
}


int VertexModule::findDragOther(int v_num) const
{
	// we always return the START of a linedef if possible, but
	// if that doesn't occur then return the END of a linedef.

	int fallback = -1;

	for (int i = 0 ; i < doc.numLinedefs() ; i++)
	{
		const auto L = doc.linedefs[i];

		if (L->end == v_num)
			return L->start;

		if (L->start == v_num && fallback < 0)
			fallback = L->end;
	}

	return fallback;
}


int VertexModule::howManyLinedefs(int v_num) const
{
	int count = 0;

	for (int n = 0 ; n < doc.numLinedefs() ; n++)
	{
		const auto L = doc.linedefs[n];

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
void VertexModule::mergeSandwichLines(EditOperation &op, int ld1, int ld2, int v, selection_c& del_lines) const
{
	const auto L1 = doc.linedefs[ld1];
	const auto L2 = doc.linedefs[ld2];

	bool ld1_onesided = L1->OneSided();
	bool ld2_onesided = L2->OneSided();

	StringID new_mid_tex = (ld1_onesided) ? doc.getRight(*L1)->mid_tex :
			     		  (ld2_onesided) ? doc.getRight(*L2)->mid_tex : StringID();

	// flip L1 so it would be parallel with L2 (after merging the other
	// endpoint) but going the opposite direction.
	if ((L2->end == v) == (L1->end == v))
	{
		doc.linemod.flipLinedef(op, ld1);
	}

	bool same_left  = (doc.getSectorID(*L2, Side::left)  == doc.getSectorID(*L1, Side::left));
	bool same_right = (doc.getSectorID(*L2, Side::right) == doc.getSectorID(*L1, Side::right));

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
		op.changeLinedef(ld2, LineDef::F_LEFT, L1->right);
	}
	else if (same_right)
	{
		op.changeLinedef(ld2, LineDef::F_RIGHT, L1->left);
	}
	else
	{
		// geometry was broken / unclosed sector(s)
	}

	del_lines.set(ld1);


	// fix orientation of remaining linedef if needed
	if (doc.getLeft(*L2) && ! doc.getRight(*L2))
	{
		doc.linemod.flipLinedef(op, ld2);
	}

	if (L2->OneSided() && new_mid_tex.hasContent())
	{
		op.changeSidedef(L2->right, SideDef::F_MID_TEX, new_mid_tex);
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

	op.changeLinedef(ld2, LineDef::F_FLAGS, new_flags);
}


//
// merge v1 into v2
//
void VertexModule::doMergeVertex(EditOperation &op, int v1, int v2, selection_c& del_lines) const
{
	SYS_ASSERT(v1 >= 0 && v2 >= 0);
	SYS_ASSERT(v1 != v2);

	// check if two linedefs would overlap after the merge
	// [ but ignore lines already marked for deletion ]

	int sandwichesMerged = 0;
	for (int n = 0 ; n < doc.numLinedefs() ; n++)
	{
		const auto L = doc.linedefs[n];

		if (! L->TouchesVertex(v1))
			continue;

		if (del_lines.get(n))
			continue;

		int v3 = (L->start == v1) ? L->end : L->start;

		int found = -1;

		for (int k = 0 ; k < doc.numLinedefs(); k++)
		{
			if (k == n)
				continue;

			const auto K = doc.linedefs[k];

			if ((K->start == v3 && K->end == v2) ||
				(K->start == v2 && K->end == v3))
			{
				found = k;
				break;
			}
		}

		if (found >= 0 && ! del_lines.get(found))
		{
			mergeSandwichLines(op, n, found, v3, del_lines);
			if(++sandwichesMerged == 2)	// can't have more than two, on each side
				break;
		}
	}

	// update all linedefs which use V1 to use V2 instead, and
	// delete any line that exists between the two vertices.

	for (int n = 0 ; n < doc.numLinedefs() ; n++)
	{
		const auto L = doc.linedefs[n];

		// change *ALL* references, this is critical
		// [ to-be-deleted lines will get start == end, that is OK ]

		if (L->start == v1)
			op.changeLinedef(n, LineDef::F_START, v2);

		if (L->end == v1)
			op.changeLinedef(n, LineDef::F_END, v2);

		if (L->start == v2 && L->end == v2)
			del_lines.set(n);
	}
}


//
// the first vertex is kept, all the other vertices are deleted
// (after fixing the attached linedefs).
//
void VertexModule::mergeList(EditOperation &op, selection_c &verts, selection_c *delResultList) const
{
	if (verts.count_obj() < 2)
		return;

	int v = verts.find_first();

#if 0
	double new_x, new_y;
	Objs_CalcMiddle(verts, &new_x, &new_y);

	BA_ChangeVT(v, Vertex::F_X, MakeValidCoord(new_x));
	BA_ChangeVT(v, Vertex::F_Y, MakeValidCoord(new_y));
#endif

	verts.clear(v);

	selection_c del_lines(ObjType::linedefs);

	// this prevents unnecessary sandwich mergers
	ConvertSelection(doc, verts, del_lines);

	for (sel_iter_c it(verts) ; !it.done() ; it.next())
	{
		doMergeVertex(op, *it, v, del_lines);
	}

	// all these vertices will be unused now, hence this call
	// shouldn't kill any other objects.
	doc.objects.del(op, verts);

	// we NEED to keep unused vertices here, otherwise we can merge
	// all vertices of an isolated sector and end up with NOTHING!
	DeleteObjects_WithUnused(op, doc, del_lines, false /* keep_things */, true /* keep_verts */, false /* keep_lines */);

	if(delResultList)
	{
		delResultList->clear_all();
		delResultList->change_type(ObjType::vertices);
		delResultList->merge(verts);
	}
	verts.clear_all();
}


void Instance::commandVertexMerge()
{
	if (edit.Selected->count_obj() == 1 && edit.highlight.valid())
	{
		edit.Selection_AddHighlighted();
	}

	if (edit.Selected->count_obj() < 2)
	{
		Beep("Need 2 or more vertices to merge");
		return;
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("merged", *edit.Selected);

		level.vertmod.mergeList(op, *edit.Selected, nullptr);
	}

	Selection_Clear(true /* no_save */);
}


bool VertexModule::tryFixDangler(int v_num) const
{
	// see if this vertex is sitting on another one (or very close to it)
	int v_other  = -1;
	int max_dist = 2;

	for (int i = 0 ; i < doc.numVertices() ; i++)
	{
		if (i == v_num)
			continue;

		double dx = doc.vertices[v_num]->x() - doc.vertices[i]->x();
		double dy = doc.vertices[v_num]->y() - doc.vertices[i]->y();

		if (fabs(dx) <= max_dist && fabs(dy) <= max_dist &&
			!doc.linemod.linedefAlreadyExists(v_num, v_other))
		{
			v_other = i;
			break;
		}
	}


	// check for a dangling vertex
	if (howManyLinedefs(v_num) != 1)
	{
		if (v_other >= 0 && howManyLinedefs(v_other) == 1)
			std::swap(v_num, v_other);
		else
			return false;
	}


	if (v_other >= 0)
	{
		inst.Selection_Clear(true /* no_save */);

		// delete highest numbered one  [ so the other index remains valid ]
		if (v_num < v_other)
			std::swap(v_num, v_other);

#if 0 // DEBUG
		fprintf(stderr, "Vertex_TryFixDangler : merge vert %d onto %d\n", v_num, v_other);
#endif

		{
			EditOperation op(doc.basis);
			op.setMessage("merged dangling vertex #%d\n", v_num);

			selection_c list(ObjType::vertices);

			list.set(v_other);	// first one is the one kept
			list.set(v_num);

			mergeList(op, list, nullptr);
		}

		inst.edit.Selected->set(v_other);

		inst.Beep("Merged a dangling vertex");
		return true;
	}


#if 0
	// find the line joined to this vertex
	int joined_ld = -1;

	for (int n = 0 ; n < doc.numLinedefs(); n++)
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

	Objid line_obj = hover::findSplitLineForDangler(doc, inst.loaded.levelFormat, inst.grid, v_num);

	if (! line_obj.valid())
		return false;

#if 0 // DEBUG
	fprintf(stderr, "Vertex_TryFixDangler : split linedef %d with vert %d\n", line_obj.num, v_num);
#endif

	EditOperation op(doc.basis);
	op.setMessage("split linedef #%d\n", line_obj.num);

	doc.linemod.splitLinedefAtVertex(op, line_obj.num, v_num);

	// no vertices were added or removed, hence can continue Insert_Vertex
	return false;
}


void VertexModule::calcDisconnectCoord(const LineDef *L, int v_num, double *x, double *y) const
{
	const auto V = doc.vertices[v_num];

	double dx = doc.getEnd(*L).x() - doc.getStart(*L).x();
	double dy = doc.getEnd(*L).y() - doc.getStart(*L).y();

	if (L->end == v_num)
	{
		dx = -dx;
		dy = -dy;
	}

	if (fabs(dx) < 4 && fabs(dy) < 4)
	{
		dx = dx / 2;
		dy = dy / 2;
	}
	else if (fabs(dx) < 16 && fabs(dy) < 16)
	{
		dx = dx / 4;
		dy = dy / 4;
	}
	else if (fabs(dx) >= fabs(dy))
	{
		dy = dy * 8 / fabs(dx);
		dx = (dx < 0) ? -8 : 8;
	}
	else
	{
		dx = dx * 8 / fabs(dy);
		dy = (dy < 0) ? -8 : 8;
	}

	*x = V->x() + dx;
	*y = V->y() + dy;
}


void VertexModule::doDisconnectVertex(EditOperation &op, int v_num, int num_lines) const
{
	int which = 0;

	for (int n = 0 ; n < doc.numLinedefs() ; n++)
	{
		const auto L = doc.linedefs[n];

		if (L->start == v_num || L->end == v_num)
		{
			double new_x, new_y;
			calcDisconnectCoord(L.get(), v_num, &new_x, &new_y);

			// the _LAST_ linedef keeps the current vertex, the rest
			// need a new one.
			if (which != num_lines-1)
			{
				int new_v = op.addNew(ObjType::vertices);

				doc.vertices[new_v]->SetRawXY(inst.loaded.levelFormat, { new_x, new_y });

				if (L->start == v_num)
					op.changeLinedef(n, LineDef::F_START, new_v);
				else
					op.changeLinedef(n, LineDef::F_END, new_v);
			}
			else
			{
				op.changeVertex(v_num, Vertex::F_X, MakeValidCoord(inst.loaded.levelFormat, new_x));
				op.changeVertex(v_num, Vertex::F_Y, MakeValidCoord(inst.loaded.levelFormat, new_y));
			}

			which++;
		}
	}
}


void Instance::commandVertexDisconnect()
{
	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("Nothing to disconnect");
			return;
		}

		edit.Selection_AddHighlighted();
	}

	bool seen_one = false;

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("disconnected", *edit.Selected);

		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			int v_num = *it;

			// nothing to do unless vertex has 2 or more linedefs
			int num_lines = level.vertmod.howManyLinedefs(*it);

			if (num_lines < 2)
				continue;

			level.vertmod.doDisconnectVertex(op, v_num, num_lines);

			seen_one = true;
		}

		if (! seen_one)
			Beep("Nothing was disconnected");
	}

	Selection_Clear(true);
}


void VertexModule::doDisconnectLinedef(EditOperation &op, int ld, int which_vert, bool *seen_one) const
{
	const auto L = doc.linedefs[ld];

	int v_num = which_vert ? L->end : L->start;

	// see if there are any linedefs NOT in the selection which are
	// connected to this vertex.

	bool touches_non_sel = false;

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		if (inst.edit.Selected->get(n))
			continue;

		const auto N = doc.linedefs[n];

		if (N->start == v_num || N->end == v_num)
		{
			touches_non_sel = true;
			break;
		}
	}

	if (! touches_non_sel)
		return;

	double new_x, new_y;
	calcDisconnectCoord(doc.linedefs[ld].get(), v_num, &new_x, &new_y);

	int new_v = op.addNew(ObjType::vertices);

	doc.vertices[new_v]->SetRawXY(inst.loaded.levelFormat, { new_x, new_y });

	// fix all linedefs in the selection to use this new vertex
	for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
	{
		const auto L2 = doc.linedefs[*it];

		if (L2->start == v_num)
			op.changeLinedef(*it, LineDef::F_START, new_v);

		if (L2->end == v_num)
			op.changeLinedef(*it, LineDef::F_END, new_v);
	}

	*seen_one = true;
}


void Instance::commandLineDisconnect()
{
	// Note: the logic here is significantly different than the logic
	//       in VT_Disconnect, since we want to keep linedefs in the
	//       selection connected, and only disconnect from linedefs
	//       NOT in the selection.
	//
	// Hence need separate code for this.

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("Nothing to disconnect");
		return;
	}

	bool seen_one = false;

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("disconnected", *edit.Selected);

		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			level.vertmod.doDisconnectLinedef(op, *it, 0, &seen_one);
			level.vertmod.doDisconnectLinedef(op, *it, 1, &seen_one);
		}
	}

	if (! seen_one)
		Beep("Nothing was disconnected");

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* no save */);
}


void VertexModule::verticesOfDetachableSectors(selection_c &verts) const
{
	SYS_ASSERT(doc.numVertices() > 0);

	bitvec_c  in_verts(doc.numVertices());
	bitvec_c out_verts(doc.numVertices());

	for (int n = 0 ; n < doc.numLinedefs() ; n++)
	{
		const auto L = doc.linedefs[n];

		// only process lines which touch a selected sector
		bool  left_in = doc.getLeft(*L)  && inst.edit.Selected->get(doc.getLeft(*L)->sector);
		bool right_in = doc.getRight(*L) && inst.edit.Selected->get(doc.getRight(*L)->sector);

		if (! (left_in || right_in))
			continue;

		bool innie = false;
		bool outie = false;

		if (doc.getRight(*L))
		{
			if (right_in)
				innie = true;
			else
				outie = true;
		}

		if (doc.getLeft(*L))
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

	for (int k = 0 ; k < doc.numVertices() ; k++)
	{
		if (in_verts.get(k) && out_verts.get(k))
			verts.set(k);
	}
}


void VertexModule::DETSEC_SeparateLine(EditOperation &op, int ld_num, int start2, int end2, Side in_side) const
{
	const auto L1 = doc.linedefs[ld_num];

	int new_ld = op.addNew(ObjType::linedefs);
	int lost_sd;

	const auto L2 = doc.linedefs[new_ld];

	if (in_side == Side::left)
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

		doc.linemod.flipLinedef(op, ld_num);
	}

	op.changeLinedef(ld_num, LineDef::F_LEFT, -1);


	// determine new flags

	int new_flags = L1->flags;

	new_flags &= ~MLF_TwoSided;
	new_flags |=  MLF_Blocking;

	op.changeLinedef(ld_num, LineDef::F_FLAGS, new_flags);

	L2->flags = L1->flags;


	// fix the first line's textures

	StringID tex = BA_InternaliseString(inst.conf.default_wall_tex);

	const SideDef * SD = doc.sidedefs[L1->right].get();

	if (! is_null_tex(SD->LowerTex()))
		tex = SD->lower_tex;
	else if (! is_null_tex(SD->UpperTex()))
		tex = SD->upper_tex;

	op.changeSidedef(L1->right, SideDef::F_MID_TEX, tex);


	// now fix the second line's textures

	SD = doc.sidedefs[lost_sd].get();

	if (! is_null_tex(SD->LowerTex()))
		tex = SD->lower_tex;
	else if (! is_null_tex(SD->UpperTex()))
		tex = SD->upper_tex;

	op.changeSidedef(lost_sd, SideDef::F_MID_TEX, tex);
}


void VertexModule::DETSEC_CalcMoveVector(const selection_c & detach_verts, double * dx, double * dy) const
{
	v2double_t det_mid = doc.objects.calcMiddle(detach_verts);
	v2double_t sec_mid = doc.objects.calcMiddle(*inst.edit.Selected);

	*dx = sec_mid.x - det_mid.x;
	*dy = sec_mid.y - det_mid.y;

	// avoid moving perfectly horizontal or vertical
	// (also handes the case of dx == dy == 0)

	if (fabs(*dx) > fabs(*dy))
	{
		*dx = (*dx < 0) ? -9 : +9;
		*dy = (*dy < 0) ? -5 : +5;
	}
	else
	{
		*dx = (*dx < 0) ? -5 : +5;
		*dy = (*dy < 0) ? -9 : +9;
	}

	if (fabs(*dx) < 2) *dx = (*dx < 0) ? -2 : +2;
	if (fabs(*dy) < 4) *dy = (*dy < 0) ? -4 : +4;

	double mul = 1.0 / clamp(0.25, inst.grid.getScale(), 1.0);

	*dx = (*dx) * mul;
	*dy = (*dy) * mul;
}


void Instance::commandSectorDisconnect()
{
	if (level.numVertices() == 0)
	{
		Beep("No sectors to disconnect");
		return;
	}

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No sectors to disconnect");
		return;
	}

	int n;

	// collect all vertices which need to be detached
	selection_c detach_verts(ObjType::vertices);
	level.vertmod.verticesOfDetachableSectors(detach_verts);

	if (detach_verts.empty())
	{
		Beep("Already disconnected");
		if (unselect == SelectHighlight::unselect)
			Selection_Clear(true /* nosave */);
		return;
	}


	// determine vector to move the detach coords
	double move_dx, move_dy;
	level.vertmod.DETSEC_CalcMoveVector(detach_verts, &move_dx, &move_dy);


	{
		EditOperation op(level.basis);
		op.setMessageForSelection("disconnected", *edit.Selected);

		// create new vertices, and a mapping from old --> new

		int * mapping = new int[level.numVertices()];

		for (n = 0 ; n < level.numVertices(); n++)
			mapping[n] = -1;

		for (sel_iter_c it(detach_verts) ; !it.done() ; it.next())
		{
			int new_v = op.addNew(ObjType::vertices);

			mapping[*it] = new_v;

			auto newbie = level.vertices[new_v];

			*newbie = *level.vertices[*it];
		}

		// update linedefs, creating new ones where necessary
		// (go backwards so we don't visit newly created lines)

		for (n = level.numLinedefs() -1 ; n >= 0 ; n--)
		{
			const auto L = level.linedefs[n];

			// only process lines which touch a selected sector
			bool  left_in = level.getLeft(*L)  && edit.Selected->get(level.getLeft(*L)->sector);
			bool right_in = level.getRight(*L) && edit.Selected->get(level.getRight(*L)->sector);

			if (! (left_in || right_in))
				continue;

			bool between_two = (left_in && right_in);

			int start2 = mapping[L->start];
			int end2   = mapping[L->end];

			if (start2 >= 0 && end2 >= 0 && L->TwoSided() && ! between_two)
			{
				level.vertmod.DETSEC_SeparateLine(op, n, start2, end2, left_in ? Side::left : Side::right);
			}
			else
			{
				if (start2 >= 0)
					op.changeLinedef(n, LineDef::F_START, start2);

				if (end2 >= 0)
					op.changeLinedef(n, LineDef::F_END, end2);
			}
		}

		delete[] mapping;

		// finally move all vertices of selected sectors

		selection_c all_verts(ObjType::vertices);

		ConvertSelection(level, *edit.Selected, all_verts);

		for (sel_iter_c it(all_verts) ; !it.done() ; it.next())
		{
			const auto V = level.vertices[*it];

			op.changeVertex(*it, Vertex::F_X, V->raw_x + MakeValidCoord(loaded.levelFormat, move_dx));
			op.changeVertex(*it, Vertex::F_Y, V->raw_y + MakeValidCoord(loaded.levelFormat, move_dy));
		}
	}

	if (unselect == SelectHighlight::unselect)
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


void Instance::CMD_VT_ShapeLine()
{
	if (edit.Selected->count_obj() < 3)
	{
		Beep("Need 3 or more vertices to shape");
		return;
	}

	// determine orientation and position of the line

	v2double_t pos1, pos2;
	level.objects.calcBBox(*edit.Selected, pos1, pos2);

	double width  = pos2.x - pos1.x;
	double height = pos2.y - pos1.y;

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

	for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
	{
		const auto V = level.vertices[*it];

		double weight = WeightForVertex(V.get(), pos1.x,pos1.y, pos2.x,pos2.y, width,height, -1);

		if (weight > 0)
		{
			ax += V->x() * weight;
			ay += V->y() * weight;

			a_total += weight;
		}

		weight = WeightForVertex(V.get(), pos1.x,pos1.y, pos2.x,pos2.y, width,height, +1);

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

	for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
	{
		const auto V = level.vertices[*it];

		vert_along_t ALONG(*it, AlongDist(V->xy(), { ax,ay }, { bx, by }));

		along_list.push_back(ALONG);
	}

	std::sort(along_list.begin(), along_list.end(), vert_along_t::CMP());


	// compute proper positions for start and end of the line
	const auto V1 = level.vertices[along_list.front().vert_num];
	const auto V2 = level.vertices[along_list. back().vert_num];

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

	EditOperation op(level.basis);
	op.setMessage("shaped %d vertices", (int)along_list.size());

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

		op.changeVertex(along_list[i].vert_num, Thing::F_X, MakeValidCoord(loaded.levelFormat, nx));
		op.changeVertex(along_list[i].vert_num, Thing::F_Y, MakeValidCoord(loaded.levelFormat, ny));
	}
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


double VertexModule::evaluateCircle(EditOperation *op, double mid_x, double mid_y, double r,
	std::vector< vert_along_t > &along_list,
	unsigned int start_idx, double arc_rad,
	double ang_offset /* radians */,
	bool move_vertices) const
{
	double cost = 0;

	bool partial_circle = (arc_rad < M_PI * 1.9);

	for (unsigned int i = 0 ; i < along_list.size() ; i++)
	{
		unsigned int k = (start_idx + i) % along_list.size();

		const auto V = doc.vertices[along_list[k].vert_num];

		double frac = i / (double)(along_list.size() - (partial_circle ? 1 : 0));

		double ang = arc_rad * frac + ang_offset;

		double new_x = mid_x + cos(ang) * r;
		double new_y = mid_y + sin(ang) * r;

		if (move_vertices)
		{
			op->changeVertex(along_list[k].vert_num, Thing::F_X, MakeValidCoord(inst.loaded.levelFormat, new_x));
			op->changeVertex(along_list[k].vert_num, Thing::F_Y, MakeValidCoord(inst.loaded.levelFormat, new_y));
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


void Instance::CMD_VT_ShapeArc()
{
	if (EXEC_Param[0].empty())
	{
		Beep("VT_ShapeArc: missing angle parameter");
		return;
	}

	int arc_deg = atoi(EXEC_Param[0]);

	if (arc_deg < 30 || arc_deg > 360)
	{
		Beep("VT_ShapeArc: bad angle: %s", EXEC_Param[0].c_str());
		return;
	}

	double arc_rad = arc_deg * M_PI / 180.0;


	if (edit.Selected->count_obj() < 3)
	{
		Beep("Need 3 or more vertices to shape");
		return;
	}


	// determine middle point for circle
	v2double_t pos1, pos2;
	level.objects.calcBBox(*edit.Selected, pos1, pos2);

	double width  = pos2.x - pos1.x;
	double height = pos2.y - pos1.y;

	if (width < 4 && height < 4)
	{
		Beep("Too small");
		return;
	}
	v2double_t mid = (pos1 + pos2) * 0.5;


	// collect all vertices and determine their angle (in radians),
	// and sort them.
	//
	// also determine radius of circle -- average of distances between
	// the computed mid-point and each vertex.

	double r = 0;

	std::vector< vert_along_t > along_list;

	for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
	{
		const auto V = level.vertices[*it];

		double dx = V->x() - mid.x;
		double dy = V->y() - mid.y;

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

	const auto start_V = level.vertices[along_list[start_idx].vert_num];
	const auto end_V   = level.vertices[along_list[  end_idx].vert_num];

	double start_end_dist = hypot(end_V->x() - start_V->x(), end_V->y() - start_V->y());


	// compute new mid-point and radius (except for a full circle)
	// and also compute starting angle.

	double best_offset = 0;
	double best_cost = 1e30;

	if (arc_deg < 360)
	{
		mid = (start_V->xy() + end_V->xy()) * 0.5;

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

		mid.x += dx * away;
		mid.y += dy * away;

		r = hypot(r, away);

		best_offset = atan2(start_V->y() - mid.y, start_V->x() - mid.x);
	}
	else
	{
		// find the best orientation, the one that minimises the distances
		// which vertices move.  We try 1000 possibilities.

		for (int pos = 0 ; pos < 1000 ; pos++)
		{
			double ang_offset = pos * M_PI * 2.0 / 1000.0;

			double cost = level.vertmod.evaluateCircle(nullptr, mid.x, mid.y, r, along_list,
										 start_idx, arc_rad, ang_offset, false);

			if (cost < best_cost)
			{
				best_offset = ang_offset;
				best_cost   = cost;
			}
		}
	}


	// actually move stuff now

	EditOperation op(level.basis);
	op.setMessage("shaped %d vertices", (int)along_list.size());

	level.vertmod.evaluateCircle(&op, mid.x, mid.y, r, along_list, start_idx, arc_rad,
				   best_offset, true);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
