//------------------------------------------------------------------------
//  VERTEX OPERATIONS
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

#include "m_dialog.h"
#include "r_grid.h"
#include "editloop.h"
#include "levels.h"
#include "objects.h"
#include "selectn.h"
#include "m_bitvec.h"
#include "e_vertex.h"


/*
 *  centre_of_vertices
 *  Return the coordinates of the centre of a group of vertices.
 */
void centre_of_vertices (SelPtr list, int *x, int *y)
{
	SelPtr cur;
	int nitems = 0;
	long x_sum;
	long y_sum;

	x_sum = 0;
	y_sum = 0;
//!!!!!!	for (nitems = 0, cur = list ; cur ; cur = cur->next, nitems++)
//!!!!!!	{
//!!!!!!		x_sum += Vertices[cur->objnum]->x;
//!!!!!!		y_sum += Vertices[cur->objnum]->y;
//!!!!!!	}
	if (nitems == 0)
	{
		*x = 0;
		*y = 0;
	}
	else
	{
		*x = (int) (x_sum / nitems);
		*y = (int) (y_sum / nitems);
	}
}


/*
 *  centre_of_vertices
 *  Return the coordinates of the centre of a group of vertices.
 */
void centre_of_vertices (const bitvec_c &bv, int &x, int &y)
{
	long x_sum = 0;
	long y_sum = 0;

	int nvertices = 0;
	for (int n = 0 ; n < bv.size() ; n++)
	{
		if (bv.get (n))
		{
			x_sum += Vertices[n]->x;
			y_sum += Vertices[n]->y;
			nvertices++;
		}
	}
	if (nvertices == 0)
	{
		x = 0;
		y = 0;
	}
	else
	{
		x = (int) (x_sum / nvertices);
		y = (int) (y_sum / nvertices);
	}
}



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


void MergeVertex(int v1, int v2)
{
	SYS_ASSERT(v1 >= 0 && v2 >= 0);
	SYS_ASSERT(v1 != v2);

	// update any linedefs which use V1 to use V2 instead
	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		// handle a line that exists between the two vertices
		if ((L->start == v1 && L->end == v2) ||
			(L->start == v2 && L->end == v1))
		{
			// we simply skip it, hence when V1 is deleted this line
			// will automatically be deleted too (as it refers to V1).
			// Clever huh?
			continue;
		}

		if (L->start == v1)
			BA_ChangeLD(n, LineDef::F_START, v2);

		if (L->end == v1)
			BA_ChangeLD(n, LineDef::F_END, v2);
	}

	// delete V1
	BA_Delete(OBJ_VERTICES, v1);
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


void CMD_DisconnectVertices()
{
	if (edit.Selected->empty())
	{
		if (! edit.highlighted())
		{
			Beep();
			return;
		}

		edit.Selected->set(edit.highlighted.num);
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
		Beep();

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


void CMD_DisconnectLineDefs()
{
	// Note: the logic here is significantly different than the logic
	//       in CMD_DisconnectVertices, since we want to keep linedefs
	//       in the selection connected, and only disconnect from
	//       linedefs NOT in the selection.
	//
	// Hence need separate code for this.

	bool unselect = false;

	if (edit.Selected->empty())
	{
		if (! edit.highlighted())
		{
			Beep();
			return;
		}

		edit.Selected->set(edit.highlighted.num);
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
		Beep();

	if (unselect)
		edit.Selected->clear_all();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
