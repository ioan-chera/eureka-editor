//------------------------------------------------------------------------
//  VERTEX OPERATIONS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2015 Andrew Apted
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

#include "editloop.h"
#include "e_linedef.h"
#include "e_vertex.h"
#include "m_bitvec.h"
#include "r_grid.h"
#include "levels.h"
#include "objects.h"
#include "w_rawdef.h"
#include "x_mirror.h"

#include <algorithm>


int Vertex_FindExact(int x, int y)
{
	for (int i = 0 ; i < NumVertices ; i++)
	{
		if (Vertices[i]->x == x && Vertices[i]->y == y)
			return i;
	}

	return -1;  // not found
}


// FIXME: InsertPolygonVertices
#if 0

/*
   insert the vertices of a new polygon
*/
void InsertPolygonVertices (int centerx, int centery, int sides, int radius)
{
	for (int n = 0 ; n < sides ; n++)
	{
		DoInsertObject (OBJ_VERTICES, -1,
				centerx + (int) ((double)radius * cos (2*M_PI * (double)n / (double)sides)),
				centery + (int) ((double)radius * sin (2*M_PI * (double)n / (double)sides)));
	}
}
#endif


/* we merge ld1 into ld2 -- v is the common vertex
 */
static void MergeConnectedLines(int ld1, int ld2, int v)
{
	// TODO: review if we should e.g. muck around with sidedefs

	BA_Delete(OBJ_LINEDEFS, ld1);
}


void MergeVertex(int v1, int v2, bool v1_will_be_deleted)
{
	SYS_ASSERT(v1 >= 0 && v2 >= 0);
	SYS_ASSERT(v1 != v2);

	// first check if two linedefs would overlap after the merge
	for (int n = NumLineDefs - 1 ; n >= 0 ; n--)
	{
		const LineDef *L = LineDefs[n];

		if (! (L->start == v1 || L->end == v1))
			continue;

		int v3 = (L->start == v1) ? L->end : L->start;

		int found = -1;

		for (int k = NumLineDefs - 1 ; k >= 0 ; k--)
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

		if (found >= 0)
		{
			// this deletes linedef [n]
			MergeConnectedLines(n, found, v3);
		}
	}

	// update any linedefs which use V1 to use V2 instead
	for (int n = NumLineDefs - 1 ; n >= 0 ; n--)
	{
		const LineDef *L = LineDefs[n];

		// handle a line that exists between the two vertices
		if ((L->start == v1 && L->end == v2) ||
			(L->start == v2 && L->end == v1))
		{
			if (v1_will_be_deleted)
			{
				// we simply skip it, hence when V1 is deleted this line
				// will automatically be deleted too (as it refers to V1).
				// Clever huh?
			}
			else
			{
				BA_Delete(OBJ_LINEDEFS, n);
			}
			continue;
		}

		if (L->start == v1)
			BA_ChangeLD(n, LineDef::F_START, v2);

		if (L->end == v1)
			BA_ChangeLD(n, LineDef::F_END, v2);
	}
}


int VertexHowManyLineDefs(int v_num)
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


static void CalcDisconnectCoord(const LineDef *L, int v_num, int *x, int *y)
{
	const Vertex * V = Vertices[v_num];

	int dx = L->End()->x - L->Start()->x;
	int dy = L->End()->y - L->Start()->y;

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

	*x = V->x + dx;
	*y = V->y + dy;
}


static void DoDisconnectVertex(int v_num, int num_lines)
{
	int which = 0;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		if (L->start == v_num || L->end == v_num)
		{
			int new_x, new_y;

			CalcDisconnectCoord(L, v_num, &new_x, &new_y);
			
			// the _LAST_ linedef keeps the current vertex, the rest
			// need a new one.
			if (which != num_lines-1)
			{
				int new_v = BA_New(OBJ_VERTICES);

				Vertices[new_v]->x = new_x;
				Vertices[new_v]->y = new_y;

				if (L->start == v_num)
					BA_ChangeLD(n, LineDef::F_START, new_v);
				else
					BA_ChangeLD(n, LineDef::F_END, new_v);
			}
			else
			{
				BA_ChangeVT(v_num, Vertex::F_X, new_x);
				BA_ChangeVT(v_num, Vertex::F_Y, new_y);
			}

			which++;
		}
	}
}


void Vertex_MergeList(selection_c *list)
{
	if (list->count_obj() < 2)
		return;

	// the first vertex is kept (but moved to the middle coordinate),
	// all the other vertices are removed.

	int new_x, new_y;

	Objs_CalcMiddle(list, &new_x, &new_y);

	int v = list->find_first();

	list->clear(v);

	BA_Begin();

	BA_ChangeVT(v, Vertex::F_X, new_x);
	BA_ChangeVT(v, Vertex::F_Y, new_y);

	selection_iterator_c it;

	for (list->begin(&it) ; !it.at_end() ; ++it)
	{
		MergeVertex(*it, v, true /* v1_will_be_deleted */);
	}

	DeleteObjects(list);

	BA_End();

	list->clear_all();
}


void VERT_Merge()
{
	if (edit.Selected->count_obj() == 1 && edit.highlight.valid())
	{
		edit.Selected->set(edit.highlight.num);
	}

	if (edit.Selected->count_obj() < 2)
	{
		Beep("Need 2 or more vertices to merge");
		return;
	}

	Vertex_MergeList(edit.Selected);
}


void VERT_Disconnect(void)
{
	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("Nothing to disconnect");
			return;
		}

		edit.Selected->set(edit.highlight.num);
	}

	bool seen_one = false;

	BA_Begin();

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		int v_num = *it;

		// nothing to do unless vertex has 2 or more linedefs
		int num_lines = VertexHowManyLineDefs(*it);
		
		if (num_lines < 2)
			continue;

		DoDisconnectVertex(v_num, num_lines);

		seen_one = true;
	}

	if (! seen_one)
		Beep("Nothing was disconnected");

	BA_End();

	edit.Selected->clear_all();
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

	int new_x, new_y;

	CalcDisconnectCoord(LineDefs[ld], v_num, &new_x, &new_y);

	int new_v = BA_New(OBJ_VERTICES);

	Vertices[new_v]->x = new_x;
	Vertices[new_v]->y = new_y;

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


void LIN_Disconnect(void)
{
	// Note: the logic here is significantly different than the logic
	//       in VERT_Disconnect, since we want to keep linedefs
	//       in the selection connected, and only disconnect from
	//       linedefs NOT in the selection.
	//
	// Hence need separate code for this.

	bool unselect = false;

	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("Nothing to disconnect");
			return;
		}

		edit.Selected->set(edit.highlight.num);
		unselect = true;
	}

	bool seen_one = false;

	BA_Begin();

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		DoDisconnectLineDef(*it, 0, &seen_one);
		DoDisconnectLineDef(*it, 1, &seen_one);
	}

	BA_End();

	if (! seen_one)
		Beep("Nothing was disconnected");

	if (unselect)
		edit.Selected->clear_all();
}


static void VerticesOfDetachableSectors(selection_c &verts)
{
	SYS_ASSERT(NumVertices > 0);

	bitvec_c  in_verts(NumVertices);
	bitvec_c out_verts(NumVertices);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef * L = LineDefs[n];

		bool innie = false;
		bool outie = false;

		// TODO: what about no-sided lines??

		if (L->Right())
		{
			if (edit.Selected->get(L->Right()->sector))
				innie = true;
			else
				outie = true;
		}

		if (L->Left())
		{
			if (edit.Selected->get(L->Left()->sector))
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

	int tex = BA_InternaliseString(default_mid_tex);

	const SideDef * SD = SideDefs[L1->right];

	if (isalnum(SD->LowerTex()[0]))
		tex = SD->lower_tex;
	else if (isalnum(SD->UpperTex()[0]))
		tex = SD->upper_tex;

	BA_ChangeSD(L1->right, SideDef::F_MID_TEX, tex);


	// now fix the second line's textures

	SD = SideDefs[lost_sd];

	if (isalnum(SD->LowerTex()[0]))
		tex = SD->lower_tex;
	else if (isalnum(SD->UpperTex()[0]))
		tex = SD->upper_tex;

	BA_ChangeSD(lost_sd, SideDef::F_MID_TEX, tex);
}


static void DETSEC_CalcMoveVector(selection_c * detach_verts, int * dx, int * dy)
{
	int det_mid_x, sec_mid_x;
	int det_mid_y, sec_mid_y;

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

	int mul = 1.0 / CLAMP(0.25, grid.Scale, 1.0);

	*dx = (*dx) * mul;
	*dy = (*dy) * mul;
}


void SEC_Disconnect(void)
{
	if (NumVertices == 0)
	{
		Beep("No sectors to disconnect");
		return;
	}

	int n;
	bool unselect = false;

	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("No sectors to disconnect");
			return;
		}

		edit.Selected->set(edit.highlight.num);
		unselect = true;
	}


	// collect all vertices which need to be detached
	selection_c detach_verts(OBJ_VERTICES);
	selection_iterator_c it;

	VerticesOfDetachableSectors(detach_verts);

	if (detach_verts.empty())
	{
		Beep("Already disconnected");
		return;
	}


	// determine vector to move the detach coords
	int move_dx, move_dy;

	DETSEC_CalcMoveVector(&detach_verts, &move_dx, &move_dy);


	BA_Begin();

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

	// finally move all vertices of selected sectors

	selection_c all_verts(OBJ_VERTICES);

	ConvertSelection(edit.Selected, &all_verts);

	for (all_verts.begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex * V = Vertices[*it];

		BA_ChangeVT(*it, Vertex::F_X, V->x + move_dx);
		BA_ChangeVT(*it, Vertex::F_Y, V->y + move_dy);
	}

	BA_End();

	if (unselect)
		edit.Selected->clear_all();
}


//------------------------------------------------------------------------
//   RESHAPING STUFF
//------------------------------------------------------------------------


static double WeightForVertex(const Vertex *V, /* bbox: */ int x1, int y1, int x2, int y2,
							  int width, int height, int side)
{
	double dist;
	double extent;

	if (width >= height)
	{
		dist = (side < 0) ? (V->x - x1) : (x2 - V->x);
		extent = width;
	}
	else
	{
		dist = (side < 0) ? (V->y - y1) : (y2 - V->y);
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


static void Reshape_Line()
{
	// determine orientation and position of the line

	int x1, y1, x2, y2;

	Objs_CalcBBox(edit.Selected, &x1, &y1, &x2, &y2);

	int width  = x2 - x1;
	int height = y2 - y1;

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
			ax += V->x * weight;
			ay += V->y * weight;

			a_total += weight;
		}

		weight = WeightForVertex(V, x1,y1, x2,y2, width,height, +1);

		if (weight > 0)
		{
			bx += V->x * weight;
			by += V->y * weight;

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

		vert_along_t ALONG(*it, AlongDist(V->x, V->y, ax,ay, bx,by));

		along_list.push_back(ALONG);
	}

	std::sort(along_list.begin(), along_list.end(), vert_along_t::CMP());


	// compute proper positions for start and end of the line
	const Vertex *V1 = Vertices[along_list.front().vert_num];
	const Vertex *V2 = Vertices[along_list. back().vert_num];

	double along1 = along_list.front().along;
	double along2 = along_list. back().along;

	if (true /* don't move first and last vertices */)
	{
		ax = V1->x;
		ay = V1->y;

		bx = V2->x;
		by = V2->y;
	}
	else
	{
		bx = ax + along2 * unit_x;
		by = ay + along2 * unit_y;

		ax = ax + along1 * unit_x;
		ay = ay + along1 * unit_y;
	}


	BA_Begin();

	for (unsigned int i = 0 ; i < along_list.size() ; i++)
	{
		double frac;
		
		if (true /* regular spacing */)
			frac = i / (double)(along_list.size() - 1);
		else
			frac = (along_list[i].along - along1) / (along2 - along1);

		// ANOTHER OPTION: use distances between neighbor verts...

		double nx = ax + (bx - ax) * frac;
		double ny = ay + (by - ay) * frac;

		BA_ChangeVT(along_list[i].vert_num, Thing::F_X, I_ROUND(nx));
		BA_ChangeVT(along_list[i].vert_num, Thing::F_Y, I_ROUND(ny));
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
							 unsigned int start_idx, double arc_size,
							 double ang_offset /* radians */,
							 bool move_vertices = false)
{
	double cost = 0;

	for (unsigned int i = 0 ; i < along_list.size() ; i++)
	{
		unsigned int k = (start_idx + i) % along_list.size();

		const Vertex *V = Vertices[along_list[k].vert_num];

		double frac = i / (double)along_list.size();

		double ang = arc_size * frac + ang_offset;

		double new_x = mid_x + cos(ang) * r;
		double new_y = mid_y + sin(ang) * r;

		if (move_vertices)
		{
			BA_ChangeVT(along_list[k].vert_num, Thing::F_X, I_ROUND(new_x));
			BA_ChangeVT(along_list[k].vert_num, Thing::F_Y, I_ROUND(new_y));
		}
		else
		{
			double dx = new_x - V->x;
			double dy = new_y - V->y;

			cost = cost + (dx*dx + dy*dy);
		}
	}

	return cost;
}


static void Reshape_Circle(char mode)
{
	//
	// The 'mode' parameter can be :
	//   'O' : full circle (360 degrees)
	//   'D' : half circle (180 degrees)
	//   'C' : quarter circle (90 degrees)
	//


	// determine middle point for circle
	// TODO : average of all vertices might be better...

	int x1, y1, x2, y2;

	Objs_CalcBBox(edit.Selected, &x1, &y1, &x2, &y2);

	int width  = x2 - x1;
	int height = y2 - y1;

	if (width < 4 && height < 4)
	{
		Beep("Too small");
		return;
	}

	double mid_x = (x1 + x2) * 0.5;
	double mid_y = (y1 + y2) * 0.5;

	double r = 0;

	// collect all vertices and determine their angle (in radians),
	// and sort them.
	//
	// also determine radius of circle -- average of distances between
	// the computed mid-point and each vertex.

	std::vector< vert_along_t > along_list;

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex *V = Vertices[*it];

		double dx = V->x - mid_x;
		double dy = V->y - mid_y;

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

	double gap_angle = BiggestGapAngle(along_list, &start_idx);

	double gap_dx = cos(gap_angle);
	double gap_dy = sin(gap_angle);


	double arc_size = M_PI * 2.0;

	if (mode == 'D')
	{
		arc_size = M_PI;

		// these constants were derived from trial-and-error
		mid_x = mid_x + r * 0.85 * gap_dx;
		mid_y = mid_y + r * 0.85 * gap_dy;

		r = r * 1.37;
	}
	else if (mode == 'C')
	{
		arc_size = M_PI * 0.6;

		mid_x = mid_x + r * 1.65 * gap_dx;
		mid_y = mid_y + r * 1.65 * gap_dy;

		r = r * 2.10;
	}


	// find the best orientation, the one that minimises the distances
	// which vertices move.  We try 1000 possibilities.

	double best_offset = 0;
	double best_cost   = 1e30;

	for (int pos = 0 ; pos < 1000 ; pos++)
	{
		double ang_offset = pos * M_PI * 2.0 / 1000.0;

		double cost = EvaluateCircle(mid_x, mid_y, r, along_list,
									 start_idx, arc_size, ang_offset);

		if (cost < best_cost)
		{
			best_offset = ang_offset;
			best_cost   = cost;
		}
	}

	// actually move stuff now

	BA_Begin();

	EvaluateCircle(mid_x, mid_y, r, along_list, start_idx, arc_size,
				   best_offset, true);

	BA_End();
}


void VERT_Reshape()
{
	const char *kind = EXEC_Param[0];

	if (! kind[0])
	{
		Beep("VERT_Reshape: missing kind name");
		return;
	}

	if (edit.Selected->count_obj() < 3)
	{
		Beep("Need 3 or more vertices to shape");
		return;
	}

	if (y_stricmp(kind, "line") == 0)
		Reshape_Line();
	else if (y_stricmp(kind, "circle") == 0)
		Reshape_Circle('O');
	else if (y_stricmp(kind, "arc180") == 0)
		Reshape_Circle('D');
	else if (y_stricmp(kind, "arc90") == 0)
		Reshape_Circle('C');
	else
	{
		Beep("VERT_Reshape: unknown kind: %s", kind);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
