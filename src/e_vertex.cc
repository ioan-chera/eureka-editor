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


typedef struct  /* Used only by AutoMergeVertices() and SortLinedefs() */
{
	int vertexl;
	int vertexh;
	int linedefno;
}
linedef_t;

/* Called only by AutoMergeVertices() */
static int SortLinedefs  (const void *item1, const void *item2);


/*
   merge several vertices into one
*/

void MergeVertices (SelPtr *list)
{
#if 0 // FIXME !!!!!
	int    v, l;

	v = (*list)->objnum;
	UnSelectObject (list, v);
	if (*list == NULL)
	{
		Beep ();
		Notify (-1, -1, "You must select at least two vertices", NULL);
		return;
	}
	/* change the linedefs starts & ends */
	for (l = 0 ; l < NumLineDefs ; l++)
	{
		if (IsSelected (*list, LineDefs[l]->start))
		{
			/* don't change a linedef that has both ends on the same spot */
			if (!IsSelected (*list, LineDefs[l]->end) && LineDefs[l]->end != v)
				LineDefs[l]->start = v;
		}
		else if (IsSelected (*list, LineDefs[l]->end))
		{
			/* idem */
			if (LineDefs[l]->start != v)
				LineDefs[l]->end = v;
		}
	}
	/* delete the vertices (and some linedefs too) */
	DeleteObjects (OBJ_VERTICES, list);

	MarkChanges(2);
#endif
}



/*
   check if some vertices should be merged into one

   <operation> can have the following values :
      'i' : called after insertion of new vertex
      'm' : called after moving of vertices

   Returns a non-zero value if the screen needs to be redrawn.
*/

bool AutoMergeVertices (SelPtr *list, int obj_type, char operation)
{
#if 0 // FIXME !!!!
	SelPtr ref, cur;
	bool   redraw;
	bool   flipped, mergedone, isldend;
	int    v, refv;
	int    ld, sd;
	int    oldnumld;
	confirm_t confirm_flag;

	/* FIXME this is quick-and-dirty hack ! The right thing
	   would be to :
	   - if obj_type == OBJ_THINGS, return
	   - if obj_type == OBJ_SECTORS or OBJ_LINEDEFS, select
	   all the vertices used by those sectors/linedefs and
	   proceed as usually. */
	if (obj_type != OBJ_VERTICES)
		return false;


	redraw = false;
	mergedone = false;
	isldend = false;

	if (operation == 'i')
		confirm_flag = insert_vertex_merge_vertices;
	else
		confirm_flag = YC_ASK_ONCE;

	/* first, check if two (or more) vertices should be merged */
	ref = *list;
	while (ref)
	{
		refv = ref->objnum;
		ref = ref->next;
		/* check if there is a vertex at the same position (same X and Y) */
		for (v = 0 ; v < NumVertices ; v++)
			if (v != refv
					&& Vertices[refv]->x == Vertices[v]->x
					&& Vertices[refv]->y == Vertices[v]->y)
			{
				char buf[81];
				redraw = true;
				sprintf (buf, "Vertices %d and %d occupy the same position", refv, v);
				if (Confirm2 (-1, -1, &confirm_flag,
							buf,
							"Do you want to merge them into one?"))
				{
					/* merge the two vertices */
					mergedone = true;
					cur = NULL;
					SelectObject (&cur, refv);
					SelectObject (&cur, v);
					MergeVertices (&cur);
					/* not useful but safer... */

					/* update the references in the selection list */
					for (cur = *list ; cur ; cur = cur->next)
						if (cur->objnum > refv)
							cur->objnum = cur->objnum - 1;
					if (v > refv)
						v--;
					/* the old vertex has been deleted */
					UnSelectObject (list, refv);
					/* select the new vertex instead */
					if (!IsSelected (*list, v))
						SelectObject (list, v);
					break;
				}
				else
					return redraw;
			}
	}

	/* Now, check if one or more vertices are on a linedef */
	//DisplayMessage  (-1, -1, "Checking vertices on a linedef");

	// Distance in map units that is equivalent to 4 pixels
	int tolerance = (int) (4 / grid.Scale);

	if (operation == 'i')
		confirm_flag = insert_vertex_split_linedef;
	else
		confirm_flag = YC_ASK_ONCE;
	ref = *list;
	while (ref)
	{
		refv = ref->objnum;
		ref = ref->next;
		oldnumld = NumLineDefs;
		//printf ("V%d %d\n", refv, NumLineDefs); // DEBUG;
		/* check if this vertex is on a linedef */
		for (ld = 0 ; ld < oldnumld ; ld++)
		{

			if (LineDefs[ld]->start == refv || LineDefs[ld]->end == refv)
			{
				/* one vertex had a linedef bound to it -- check it later */
				isldend = true;
			}
			else if (IsLineDefInside (ld, Vertices[refv]->x - tolerance,
						Vertices[refv]->y - tolerance, 
						Vertices[refv]->x + tolerance,
						Vertices[refv]->y + tolerance))
			{
				char buf[81];
				redraw = true;
				sprintf (buf, "Vertex %d is on top of linedef %d", refv, ld);
				if (Confirm2 (-1, -1, &confirm_flag,
							buf,
							"Do you want to split the linedef there?"))
				{
					/* split the linedef */
					mergedone = true;
					DoInsertObject (OBJ_LINEDEFS, ld, 0, 0);
					LineDefs[ld]->end = refv;
					LineDefs[NumLineDefs - 1]->start = refv;
					sd = LineDefs[ld]->right;
					if (sd >= 0)
					{
						DoInsertObject (OBJ_SIDEDEFS, sd, 0, 0);

						LineDefs[NumLineDefs - 1]->right = NumSideDefs - 1;
					}
					sd = LineDefs[ld]->side_L;
					if (sd >= 0)
					{
						DoInsertObject (OBJ_SIDEDEFS, sd, 0, 0);

						LineDefs[NumLineDefs - 1]->side_L = NumSideDefs - 1;
					}
					MarkChanges(2);
				}
				else
					return redraw;
			}
		}
	}

	/* Don't continue if this isn't necessary */
	if (! isldend || ! mergedone)
		return redraw;

	/* finally, test if two linedefs are between the same pair of vertices */
	/* AYM 1997-07-17
	   Here I put a new algorithm. The original one led to a very large
	   number of tests (N*(N-1)/2 where N is number of linedefs). For
	   example, for a modest sized level (E1M4, 861 linedefs), it made
	   about 350,000 tests which, on my DX4/75, took about 3 seconds.
	   That IS irritating.
	   The new algorithm makes about N*log(N)+N tests, which is, in the
	   same example as above, about 3,400 tests. That's about 100 times
	   less. To be rigorous, it is not really 100 times faster because the
	   N*log(N) operations are function calls from within qsort() that
	   must take longer than inline tests. Anyway, the checking is now
	   almost instantaneous and that's what I wanted.
	   The principle is to sort the linedefs by smallest numbered vertex
	   then highest numbered vertex. Thus, two overlapping linedefs must
	   be neighbours in the array. No need to test the N-2 others like
	   before. */

	//DisplayMessage (-1, -1, "Checking superimposed linedefs");
	confirm_flag = YC_ASK_ONCE;
	{
		linedef_t *linedefs;

		/* Copy the linedefs into array 'linedefs' and sort it */
		linedefs = (linedef_t *) GetMemory (NumLineDefs * sizeof (linedef_t));
		for (ld = 0 ; ld < NumLineDefs ; ld++)
		{
			linedefs[ld].vertexl = MIN(LineDefs[ld]->start, LineDefs[ld]->end);
			linedefs[ld].vertexh = MAX(LineDefs[ld]->start, LineDefs[ld]->end);
			linedefs[ld].linedefno = ld;
		}
		qsort (linedefs, NumLineDefs, sizeof (linedef_t), SortLinedefs);

		/* Search for superimposed linedefs in the array */
		for (ld = 0 ; ld+1 < NumLineDefs ; ld++)
		{
			if (linedefs[ld+1].vertexl != linedefs[ld].vertexl
					|| linedefs[ld+1].vertexh != linedefs[ld].vertexh)
				continue;
			int ld1 = linedefs[ld].linedefno;
			int ld2 = linedefs[ld+1].linedefno;
			char prompt[81];
			y_snprintf (prompt, sizeof prompt, "Linedefs %d and %d are superimposed",
					ld1, ld2);
			redraw = true;
			if (Expert || Confirm2 (-1, -1, &confirm_flag,
						prompt, "(and perhaps others too). Merge them ?"))
			{
				LDPtr ldo = LineDefs + ld1;
				LDPtr ldn = LineDefs + ld2;
				/* Test if the linedefs have the same orientation */
				if (ldn->start == ldo->end)
					flipped = true;
				else
					flipped = false;
				/* Merge linedef ldo (ld) into linedef ldn (ld+1) */
				/* FIXME When is this done ? Most of the time when adding a
				   door/corridor/window between two rooms, right ? So we should
				   handle this in a smarter way by copying middle texture into
				   lower and upper texture if the sectors don't have the same
				   heights and maybe setting the LTU and UTU flags.
				   This also applies when a linedef becomes two-sided as a
				   result of creating a new sector. */
				if (ldn->right < 0)
				{
					if (flipped)
					{
						ldn->right = ldo->side_L;
						ldo->side_L = OBJ_NO_NONE;
					}
					else
					{
						ldn->right = ldo->right;
						ldo->right = OBJ_NO_NONE;
					}
				}
				if (ldn->side_L < 0)
				{
					if (flipped)
					{
						ldn->side_L = ldo->right;
						ldo->right = OBJ_NO_NONE;
					}
					else
					{
						ldn->side_L = ldo->side_L;
						ldo->side_L = OBJ_NO_NONE;
					}
				}
				if (ldn->right >= 0 && ldn->side_L >= 0 && (ldn->flags & 0x04) == 0)
					ldn->flags = 0x04;
				DeleteObject (Objid (OBJ_LINEDEFS, ld1));
			}
		}

		FreeMemory (linedefs);
	}

	return redraw;
#else
	return true;
#endif
}


/*
   compare two linedefs for sorting by lowest numbered vertex
*/
static int SortLinedefs (const void *item1, const void *item2)
{
#define ld1 ((const linedef_t *) item1)
#define ld2 ((const linedef_t *) item2)
	if (ld1->vertexl != ld2->vertexl)
		return ld1->vertexl - ld2->vertexl;
	if (ld1->vertexh != ld2->vertexh)
		return ld1->vertexh - ld2->vertexh;
	return 0;
}



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
