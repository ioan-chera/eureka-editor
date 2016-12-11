//------------------------------------------------------------------------
//  OBJECT OPERATIONS
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

#include <algorithm>

#include "e_hover.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_sector.h"
#include "e_things.h"
#include "e_vertex.h"
#include "m_game.h"
#include "e_objects.h"
#include "r_grid.h"
#include "w_rawdef.h"

#include "ui_window.h"


// config items
bool new_islands_are_void = false;
int  new_sector_size = 128;

bool select_verts_of_new_sectors = true;


//
//  delete a group of objects.
//  this is very raw, e.g. it does not check for stuff that will
//  remain unused afterwards.
//
void DeleteObjects(selection_c *list)
{
	// we need to process the object numbers from highest to lowest,
	// because each deletion invalidates all higher-numbered refs
	// in the selection.  Our selection iterator cannot give us
	// what we need, hence put them into a vector for sorting.

	if (list->empty())
		return;

	std::vector<int> objnums;

	selection_iterator_c it;
	for (list->begin(&it) ; !it.at_end() ; ++it)
		objnums.push_back(*it);

	std::sort(objnums.begin(), objnums.end());

	for (int i = (int)objnums.size()-1 ; i >= 0 ; i--)
	{
		BA_Delete(list->what_type(), objnums[i]);
	}
}


static void CreateSquare(int model)
{
	int new_sec = BA_New(OBJ_SECTORS);

	if (model >= 0)
		Sectors[new_sec]->RawCopy(Sectors[model]);
	else
		Sectors[new_sec]->SetDefaults();

	int x1 = grid.QuantSnapX(edit.map_x, false);
	int y1 = grid.QuantSnapX(edit.map_y, false);

	int x2 = x1 + new_sector_size;
	int y2 = y1 + new_sector_size;

	for (int i = 0 ; i < 4 ; i++)
	{
		int new_v = BA_New(OBJ_VERTICES);

		Vertices[new_v]->x = (i >= 2) ? x2 : x1;
		Vertices[new_v]->y = (i==1 || i==2) ? y2 : y1;

		int new_sd = BA_New(OBJ_SIDEDEFS);

		SideDefs[new_sd]->SetDefaults(false);
		SideDefs[new_sd]->sector = new_sec;

		int new_ld = BA_New(OBJ_LINEDEFS);

		LineDef * L = LineDefs[new_ld];

		L->start = new_v;
		L->end   = (i == 3) ? (new_v - 3) : new_v + 1;
		L->flags = MLF_Blocking;
		L->right = new_sd;
	}

	// select it
	Selection_Clear();

	edit.Selected->set(new_sec);
}


static void Insert_Thing()
{
	int model = -1;

	if (edit.Selected->notempty())
		model = edit.Selected->find_first();


	BA_Begin();

	int new_t = BA_New(OBJ_THINGS);

	Thing *T = Things[new_t];

	T->type = default_thing;

	if (model >= 0)
		T->RawCopy(Things[model]);

	T->x = grid.SnapX(edit.map_x);
	T->y = grid.SnapY(edit.map_y);

	recent_things.insert_number(T->type);

	BA_Message("added thing #%d", new_t);

	BA_End();


	// select it
	Selection_Clear();

	edit.Selected->set(new_t);
}


static void ClosedLoop_Simple(int new_ld, int v2, selection_c& flip)
{
	lineloop_c right_loop;
	lineloop_c  left_loop;

	bool right_ok = TraceLineLoop(new_ld, SIDE_RIGHT, right_loop);
	bool  left_ok = TraceLineLoop(new_ld, SIDE_LEFT,   left_loop);

	// require all lines to be "bare" (no sidedefs)
	right_ok = right_ok && right_loop.AllBare();
	 left_ok =  left_ok &&  left_loop.AllBare();

	// check if one of the loops faces outward (an island), in
	// which case we just make the island part of the surrounding
	// sector (i.e. DON'T put a new sector on the inside).

	bool did_outer = false;

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		lineloop_c& loop = (pass == 0) ? right_loop : left_loop;

		bool ok = (pass == 0) ? right_ok : left_ok;

		if (ok && loop.faces_outward)
		{
			int sec_num = loop.IslandSector();
			if (sec_num >= 0)
			{
				loop.AssignSector(sec_num, flip);
				did_outer = true;
			}
		}
	}

	// otherwise try to create new sector in the inside area

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		lineloop_c& loop = (pass == 0) ? right_loop : left_loop;

		bool ok = (pass == 0) ? right_ok : left_ok;

		if (ok && ! loop.faces_outward)
		{
			loop.FindIslands();

			// if the loop is inside a sector, only _have_ to create
			// the inner sector if we surrounded something.

			if (new_islands_are_void && did_outer && loop.islands.empty())
				return;

			int new_sec = BA_New(OBJ_SECTORS);

			int model = loop.NeighboringSector();

			if (model >= 0)
				Sectors[new_sec]->RawCopy(Sectors[model]);
			else
				Sectors[new_sec]->SetDefaults();

			loop.AssignSector(new_sec, flip);
		}
	}
}


static bool TwoNeighboringLineDefs(int new_ld, int v1, int v2,
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

		double angle = AngleBetweenLines(v1, v2, other_v);

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


static void ClosedLoop_Complex(int new_ld, int v1, int v2, selection_c& flip)
{
DebugPrintf("COMPLEX LOOP : LINE #%d : %d --> %d\n", new_ld, v1, v2);

	// find the two linedefs which are nearest to the new line

	int left_ld = 0,   right_ld = 0;
	int left_side = 0, right_side = 0;

	if (! TwoNeighboringLineDefs(new_ld, v1, v2, &right_ld, &right_side, &left_ld, &left_side))
	{
		// Beep();
		return;
	}

	int right_front = LineDefs[right_ld]->WhatSector(right_side);
	int  left_front = LineDefs[ left_ld]->WhatSector( left_side);

	int right_back = LineDefs[right_ld]->WhatSector(- right_side);
	int  left_back = LineDefs[ left_ld]->WhatSector(-  left_side);

	bool right_new = (right_front < 0) && (right_back < 0);
	bool  left_new = ( left_front < 0) && ( left_back < 0);

DebugPrintf("RIGHT LINE #%d : front=%d back=%d\n", right_ld, right_front, right_back);
DebugPrintf(" LEFT LINE #%d : front=%d back=%d\n",  left_ld,  left_front,  left_back);

	if (right_new || left_new)
	{
		// OK
	}
	else if (right_front != left_front)
	{
		// geometry is broken : disable auto-split or auto-sectoring.

		// Beep();
		return;
	}

	// AT HERE : either splitting a sector or extending one

	lineloop_c right_loop;
	lineloop_c  left_loop;

	// trace the loops on either side of the new line

	bool right_ok = TraceLineLoop(new_ld, SIDE_RIGHT, right_loop);
	bool  left_ok = TraceLineLoop(new_ld, SIDE_LEFT,   left_loop);

DebugPrintf("right_ok : %s\n", right_ok ? "yes" : "NO!");
DebugPrintf(" left_ok : %s\n",  left_ok ? "yes" : "NO!");

if (right_ok) DebugPrintf("right faces outward : %s\n", right_loop.faces_outward ? "YES!" : "no");
if ( left_ok) DebugPrintf(" left faces outward : %s\n",  left_loop.faces_outward ? "YES!" : "no");

	if (right_front >= 0 &&
	    right_front == left_front &&
	    (right_ok && !right_loop.faces_outward) &&
	    ( left_ok && ! left_loop.faces_outward))
	{
		// the SPLITTING case....
		DebugPrintf("SPLITTING sector #%d\n", right_front);

		// ensure original sector is OK
		lineloop_c orig_loop;

		if (! TraceLineLoop(right_ld, right_side, orig_loop, true /* ignore_new */))
		{
			DebugPrintf("Traced original : failed\n");
			return;
		}

		if (! orig_loop.SameSector())
		{
			DebugPrintf("Original not all same\n");
			return;
		}

		// OK WE ARE SPLITTING IT : pick which side will stay the same

		double right_total = right_loop.TotalLength();
		double  left_total =  left_loop.TotalLength();

		DebugPrintf("Left total: %1.1f   Right total: %1.1f\n", left_total, right_total);

		lineloop_c&  mod_loop = (left_total < right_total) ? left_loop : right_loop;
		lineloop_c& keep_loop = (left_total < right_total) ? right_loop : left_loop;

		// we'll need the islands too
		mod_loop.FindIslands();

		int new_sec = BA_New(OBJ_SECTORS);

		Sectors[new_sec]->RawCopy(Sectors[right_front]);

		// ensure the new linedef usually ends at v2 (the final vertex)
		if (left_total < right_total)
		{
			keep_loop.AssignSector(right_front, flip);
			 mod_loop.AssignSector(new_sec,     flip);
		}
		else
		{
			 mod_loop.AssignSector(new_sec,     flip);
			keep_loop.AssignSector(right_front, flip);
		}

		return;
	}


	// the SPLIT-VOID case....

	if (right_ok && right_front < 0 && !right_loop.faces_outward &&
		 left_ok &&  left_front < 0 && ! left_loop.faces_outward)
	{
		DebugPrintf("SPLITTING VOID...\n");

		// pick which side gets the new sector

		double right_total = right_loop.TotalLength();
		double  left_total =  left_loop.TotalLength();

		DebugPrintf("Left total: %1.1f   Right total: %1.1f\n", left_total, right_total);

		lineloop_c& loop = (left_total < right_total) ? left_loop : right_loop;

		loop.FindIslands();

		int model = (left_total < right_total) ? left_back : right_back;

		int new_sec = BA_New(OBJ_SECTORS);

		if (model < 0)
			Sectors[new_sec]->SetDefaults();
		else
			Sectors[new_sec]->RawCopy(Sectors[model]);

		loop.AssignSector(new_sec, flip);
		return;
	}


	// the EXTENDING case....
	DebugPrintf("EXTENDING....\n");

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		lineloop_c& loop = (pass == 0) ? right_loop : left_loop;

		int front = (pass == 0) ? right_front : left_front;

		bool ok = (pass == 0) ? right_ok : left_ok;

		// bad geometry?
		if (! ok)
			continue;

		if (loop.faces_outward)
		{
			// we are extending an island, see if that island lies
			// within an existing sector

			int sec_num = (front >= 0) ? front : loop.IslandSector();
			if (sec_num >= 0)
			{
				loop.AssignSector(sec_num, flip);
			}
		}
		else
		{
			loop.FindIslands();
DebugPrintf("ISLANDS = %u\n", loop.islands.size());

			int model = loop.NeighboringSector();

			int new_sec = BA_New(OBJ_SECTORS);

			if (model < 0)
				Sectors[new_sec]->SetDefaults();
			else
				Sectors[new_sec]->RawCopy(Sectors[model]);

			loop.AssignSector(new_sec, flip);
		}
	}
}


void Insert_LineDef(int v1, int v2, bool no_fill = false)
{
	int new_ld = BA_New(OBJ_LINEDEFS);

	LineDef * L = LineDefs[new_ld];

	L->start = v1;
	L->end   = v2;
	L->flags = MLF_Blocking;

	if (no_fill)
		return;

	selection_c flip(OBJ_LINEDEFS);

	switch (Vertex_HowManyLineDefs(v2))
	{
		case 0:
			// this should not happen!
			return;

		case 1:
			// joined onto an isolated vertex : nothing to do
			return;

		case 2:
			ClosedLoop_Simple(new_ld, v2, flip);
			break;

		default:  // 3 or more
			ClosedLoop_Complex(new_ld, v1, v2, flip);
			break;
	}

	FlipLineDefGroup(flip);
}


void Insert_LineDef_autosplit(int v1, int v2, bool no_fill = false)
{
	if (LineDefAlreadyExists(v1, v2))
		return;

///  fprintf(stderr, "Insert_LineDef_autosplit %d..%d\n", v1, v2);

	// Find a linedef which this new line would cross, and if it exists
	// add a vertex there and create TWO lines.  Also handle a vertex
	// that this line crosses (sits on) similarly.

	cross_state_t cross;

	if (! FindClosestCrossPoint(v1, v2, &cross))
	{
		Insert_LineDef(v1, v2, no_fill);
		return;
	}

	if (cross.line >= 0)
	{
		cross.vert = BA_New(OBJ_VERTICES);

		Vertex *V = Vertices[cross.vert];

		V->x = cross.x;
		V->y = cross.y;

		SplitLineDefAtVertex(cross.line, cross.vert);
	}

	// recursively handle both sides

	Insert_LineDef_autosplit(v1, cross.vert, no_fill);
	Insert_LineDef_autosplit(cross.vert, v2, no_fill);
}


void Insert_Vertex(bool force_continue, bool no_fill, bool is_button)
{
	bool closed_a_loop = false;

	int old_vert = -1;
	int new_vert = -1;

	Vertex *V = NULL;

	int new_x = grid.SnapX(edit.map_x);
	int new_y = grid.SnapY(edit.map_y);

	int orig_num_sectors = NumSectors;


	// are we drawing a line?
	if (edit.action == ACT_DRAW_LINE)
		old_vert = edit.drawing_from;

	// a linedef which we are splitting (usually none)
	int split_ld = edit.split_line.valid() ? edit.split_line.num : -1;


	// only use the highlight when not splitting a line
	if (split_ld < 0)
	{
		// the "nearby" vertex is usually the highlighted one.
		int hi_vert = -1;

		if (edit.highlight.valid())
			hi_vert = edit.highlight.num;

		// if no highlight, look for a vertex at snapped coord
		if (hi_vert < 0 && grid.snap && ! (edit.action == ACT_DRAW_LINE))
			hi_vert = Vertex_FindExact(new_x, new_y);

		//
		// handle a highlighted vertex:
		// either start drawing from it, or finish a loop at it.
		//
		if (hi_vert >= 0)
		{
			// just ignore when same as drawing_from vert
			if (hi_vert == old_vert)
			{
#if 1
				edit.Selected->set(old_vert);
				return;
#else
				// simply unselect it and stop drawing
				edit.Selected->clear(old_vert);
				Editor_ClearAction();
				return;
#endif
			}

			// a plain INSERT will attempt to fix a dangling vertex
			if (!is_button && edit.action == ACT_NOTHING)
			{
				if (Vertex_TryFixDangler(hi_vert))
				{
					// a vertex was deleted, selection/highlight is now invalid
					return;
				}
			}

			if (old_vert < 0)
			{
				old_vert = hi_vert;
				goto begin_drawing;
			}

			new_vert = hi_vert;
		}

		// handle case where a line already exists between the two vertices
		if (old_vert >= 0 && new_vert >= 0 &&
			LineDefAlreadyExists(old_vert, new_vert))
		{
			// just continue drawing from the second vertex
			edit.drawing_from = new_vert;
			edit.Selected->set(new_vert);
			return;
		}
	}


	// prevent creating an overlapping line when splitting
	if (split_ld >= 0 && old_vert >= 0 &&
		LineDefs[split_ld]->TouchesVertex(old_vert))
	{
		old_vert = -1;
	}


	BA_Begin();


	if (new_vert < 0)
	{
		new_vert = BA_New(OBJ_VERTICES);

		BA_Message("added vertex #%d", new_vert);

		V = Vertices[new_vert];

		V->x = new_x;
		V->y = new_y;

		edit.Selected->set(new_vert);
		edit.drawing_from = new_vert;

		if (old_vert < 0)
		{
			old_vert = new_vert;
			new_vert = -1;
		}
	}


	// splitting an existing line?
	if (split_ld >= 0)
	{
		SYS_ASSERT(V);

		V->x = edit.split_x;
		V->y = edit.split_y;

		BA_Message("split linedef #%d", split_ld);

		SplitLineDefAtVertex(split_ld, new_vert >= 0 ? new_vert : old_vert);
	}


	// closing a loop?
	if (!force_continue && new_vert >= 0 && Vertex_HowManyLineDefs(new_vert) > 0)
	{
		closed_a_loop = true;
	}


	// adding a linedef?
	if (old_vert >= 0 && new_vert >= 0)
	{
		SYS_ASSERT(old_vert != new_vert);

		// sanity check
		if (Vertices[old_vert]->Matches(Vertices[new_vert]))
			BugError("Bug detected (creation of zero-length line)\n");

		// this can make new sectors too
		Insert_LineDef_autosplit(old_vert, new_vert, no_fill);

		BA_Message("added linedef");

		edit.Selected->set(new_vert);
		edit.drawing_from = new_vert;
	}

	BA_End();


	// begin drawing mode?
begin_drawing:
	if (edit.action == ACT_NOTHING && !closed_a_loop &&
		old_vert >= 0 && new_vert < 0)
	{
		Selection_Clear();

		edit.Selected->set(old_vert);

		Editor_SetAction(ACT_DRAW_LINE);
		edit.drawing_from = old_vert;
	}

	// stop drawing mode?
	if (closed_a_loop && !force_continue)
	{
		Editor_ClearAction();
	}

	// select vertices of a newly created sector?
	if (select_verts_of_new_sectors && closed_a_loop &&
		NumSectors > orig_num_sectors)
	{
		selection_c sel(OBJ_SECTORS);

		// more than one sector may have been created, pick the last
		sel.set(NumSectors - 1);

		edit.Selected->change_type(edit.mode);
		ConvertSelection(&sel, edit.Selected);
	}

	RedrawMap();
}


static void Insert_Sector()
{
	int sel_count = edit.Selected->count_obj();
	if (sel_count > 1)
	{
		Beep("Too many sectors to copy from");
		return;
	}

	// if outside of the map, create a square
	if (PointOutsideOfMap(edit.map_x, edit.map_y))
	{
		BA_Begin();

		int model = -1;
		if (sel_count > 0)
			model = edit.Selected->find_first();

		CreateSquare(model);

		BA_Message("added sector (outside map)");
		BA_End();

		return;
	}


	// --- adding a NEW sector to the area ---

	// determine a model sector to copy properties from
	int model;

	if (sel_count > 0)
		model = edit.Selected->find_first();
	else if (edit.highlight.valid())
		model = edit.highlight.num;
	else
		model = -1;  // look for a neighbor to copy


	BA_Begin();
	BA_Message("added new sector");

	int new_sec = BA_New(OBJ_SECTORS);

	if (model >= 0)
	{
		Sectors[new_sec]->RawCopy(Sectors[model]);
	}

	AssignSectorToSpace(edit.map_x, edit.map_y, new_sec, model < 0);

	BA_End();

	// select the new sector
	Selection_Clear();

	edit.Selected->set(NumSectors - 1);

	RedrawMap();
}


void CMD_Insert()
{
	bool force_cont;
	bool no_fill;

	if (edit.render3d)
	{
		Beep("Insert: not usable in 3D view");
		return;
	}

	switch (edit.mode)
	{
		case OBJ_THINGS:
			Insert_Thing();
			break;

		case OBJ_VERTICES:
			force_cont = Exec_HasFlag("/continue");
			no_fill    = Exec_HasFlag("/nofill");
			Insert_Vertex(force_cont, no_fill);
			break;

		case OBJ_SECTORS:
			Insert_Sector();
			break;

		default:
			Beep("Cannot insert in this mode");
			break;
	}

	RedrawMap();
}


//
// check if any part of a LineDef is inside the given box
//
bool LineTouchesBox (int ld, int x0, int y0, int x1, int y1)
{
	int lx0 = LineDefs[ld]->Start()->x;
	int ly0 = LineDefs[ld]->Start()->y;
	int lx1 = LineDefs[ld]->End()->x;
	int ly1 = LineDefs[ld]->End()->y;

	int i;

	// start is entirely inside the square?
	if (lx0 >= x0 && lx0 <= x1 && ly0 >= y0 && ly0 <= y1)
		return true;

	// end is entirely inside the square?
	if (lx1 >= x0 && lx1 <= x1 && ly1 >= y0 && ly1 <= y1)
		return true;


	if ((ly0 > y0) != (ly1 > y0))
	{
		i = lx0 + (int) ((double) (y0 - ly0) * (double) (lx1 - lx0) / (double) (ly1 - ly0));
		if (i >= x0 && i <= x1)
			return true; /* the linedef crosses the left side */
	}
	if ((ly0 > y1) != (ly1 > y1))
	{
		i = lx0 + (int) ((double) (y1 - ly0) * (double) (lx1 - lx0) / (double) (ly1 - ly0));
		if (i >= x0 && i <= x1)
			return true; /* the linedef crosses the right side */
	}
	if ((lx0 > x0) != (lx1 > x0))
	{
		i = ly0 + (int) ((double) (x0 - lx0) * (double) (ly1 - ly0) / (double) (lx1 - lx0));
		if (i >= y0 && i <= y1)
			return true; /* the linedef crosses the bottom side */
	}
	if ((lx0 > x1) != (lx1 > x1))
	{
		i = ly0 + (int) ((double) (x1 - lx0) * (double) (ly1 - ly0) / (double) (lx1 - lx0));
		if (i >= y0 && i <= y1)
			return true; /* the linedef crosses the top side */
	}

	return false;
}



static void DoMoveObjects(selection_c *list, int delta_x, int delta_y, int delta_z)
{
	selection_iterator_c it;

	switch (list->what_type())
	{
		case OBJ_THINGS:
			for (list->begin(&it) ; !it.at_end() ; ++it)
			{
				const Thing * T = Things[*it];

				BA_ChangeTH(*it, Thing::F_X, T->x + delta_x);
				BA_ChangeTH(*it, Thing::F_Y, T->y + delta_y);
			}
			break;

		case OBJ_VERTICES:
			for (list->begin(&it) ; !it.at_end() ; ++it)
			{
				const Vertex * V = Vertices[*it];

				BA_ChangeVT(*it, Vertex::F_X, V->x + delta_x);
				BA_ChangeVT(*it, Vertex::F_Y, V->y + delta_y);
			}
			break;

		case OBJ_SECTORS:
			// apply the Z delta first
			for (list->begin(&it) ; !it.at_end() ; ++it)
			{
				const Sector * S = Sectors[*it];

				BA_ChangeSEC(*it, Sector::F_FLOORH, S->floorh + delta_z);
				BA_ChangeSEC(*it, Sector::F_CEILH,  S->ceilh  + delta_z);
			}

			/* FALL-THROUGH !! */

		case OBJ_LINEDEFS:
			{
				selection_c verts(OBJ_VERTICES);

				ConvertSelection(list, &verts);

				DoMoveObjects(&verts, delta_x, delta_y, delta_z);
			}
			break;

		default:
			break;
	}
}


void MoveObjects(selection_c *list, int delta_x, int delta_y, int delta_z)
{
	if (list->empty())
		return;

	BA_Begin();

	// move things in sectors too (must do it _before_ moving the
	// sectors, otherwise we fail trying to determine which sectors
	// each thing is in).
	if (edit.mode == OBJ_SECTORS)
	{
		selection_c thing_sel(OBJ_THINGS);

		ConvertSelection(list, &thing_sel);

		DoMoveObjects(&thing_sel, delta_x, delta_y, delta_z);
	}

	DoMoveObjects(list, delta_x, delta_y, delta_z);

	BA_MessageForSel("moved", list);

	BA_End();
}


void DragSingleObject(int obj_num, int delta_x, int delta_y, int delta_z)
{
	BA_Begin();

	int did_split_line = -1;

	// handle a single vertex merging onto an existing one
	if (edit.mode == OBJ_VERTICES && edit.highlight.valid())
	{
		BA_Message("merge vertex #%d", obj_num);

		SYS_ASSERT(obj_num != edit.highlight.num);

		selection_c verts(OBJ_VERTICES);

		verts.set(edit.highlight.num);	// keep the highlight
		verts.set(obj_num);

		Vertex_MergeList(&verts);

		BA_End();
		return;
	}

	// handle a single vertex splitting a linedef
	if (edit.mode == OBJ_VERTICES && edit.split_line.valid())
	{
		did_split_line = edit.split_line.num;

		SplitLineDefAtVertex(edit.split_line.num, obj_num);

		// now move the vertex!
	}

	selection_c list(edit.mode);

	list.set(obj_num);

	DoMoveObjects(&list, delta_x, delta_y, delta_z);

	if (did_split_line >= 0)
		BA_Message("split linedef #%d", did_split_line);
	else
		BA_MessageForSel("moved", &list);

	BA_End();
}


static void TransferThingProperties(int src_thing, int dest_thing)
{
	const Thing * T = Things[src_thing];

	BA_ChangeTH(dest_thing, Thing::F_TYPE,    T->type);
	BA_ChangeTH(dest_thing, Thing::F_OPTIONS, T->options);
//	BA_ChangeTH(dest_thing, Thing::F_ANGLE,   T->angle);

	BA_ChangeTH(dest_thing, Thing::F_TID,     T->tid);
	BA_ChangeTH(dest_thing, Thing::F_SPECIAL, T->special);

	BA_ChangeTH(dest_thing, Thing::F_ARG1, T->arg1);
	BA_ChangeTH(dest_thing, Thing::F_ARG2, T->arg2);
	BA_ChangeTH(dest_thing, Thing::F_ARG3, T->arg3);
	BA_ChangeTH(dest_thing, Thing::F_ARG4, T->arg4);
	BA_ChangeTH(dest_thing, Thing::F_ARG5, T->arg5);
}


static void TransferSectorProperties(int src_sec, int dest_sec)
{
	const Sector * SEC = Sectors[src_sec];

	BA_ChangeSEC(dest_sec, Sector::F_FLOORH,    SEC->floorh);
	BA_ChangeSEC(dest_sec, Sector::F_FLOOR_TEX, SEC->floor_tex);
	BA_ChangeSEC(dest_sec, Sector::F_CEILH,     SEC->ceilh);
	BA_ChangeSEC(dest_sec, Sector::F_CEIL_TEX,  SEC->ceil_tex);

	BA_ChangeSEC(dest_sec, Sector::F_LIGHT,  SEC->light);
	BA_ChangeSEC(dest_sec, Sector::F_TYPE,   SEC->type);
	BA_ChangeSEC(dest_sec, Sector::F_TAG,    SEC->tag);
}


#define LINEDEF_FLAG_KEEP  (MLF_Blocking + MLF_TwoSided)

static void TransferLinedefProperties(int src_line, int dest_line, bool do_tex)
{
	const LineDef * L1 = LineDefs[src_line];
	const LineDef * L2 = LineDefs[dest_line];

	// don't transfer certain flags
	int flags = LineDefs[dest_line]->flags;
	flags = (flags & LINEDEF_FLAG_KEEP) | (L1->flags & ~LINEDEF_FLAG_KEEP);

	// handle textures
	if (do_tex && L1->Right() && L2->Right())
	{
		/* There are four cases, depending on number of sides:
		 *
		 * (a) single --> single : easy
		 *
		 * (b) single --> double : copy mid_tex to both sides upper and lower
		 *                         [alternate idea: copy mid_tex to VISIBLE sides]
		 *
		 * (c) double --> single : pick a texture (e.g. visible lower) to copy
		 *
		 * (d) double --> double : copy each side, but possibly flip the
		 *                         second linedef based on floor or ceil diff.
		 */
		if (! L1->Left())
		{
			int tex = L1->Right()->mid_tex;

			if (! L2->Left())
			{
				BA_ChangeSD(L2->right, SideDef::F_MID_TEX, tex);
			}
			else
			{
				BA_ChangeSD(L2->right, SideDef::F_LOWER_TEX, tex);
				BA_ChangeSD(L2->right, SideDef::F_UPPER_TEX, tex);

				BA_ChangeSD(L2->left,  SideDef::F_LOWER_TEX, tex);
				BA_ChangeSD(L2->left,  SideDef::F_UPPER_TEX, tex);

				// this is debatable....   CONFIG ITEM?
				flags |= MLF_LowerUnpegged;
				flags |= MLF_UpperUnpegged;
			}
		}
		else if (! L2->Left())
		{
			/* pick which texture to copy */

			const Sector *front = L1->Right()->SecRef();
			const Sector *back  = L1-> Left()->SecRef();

			int f_l = L1->Right()->lower_tex;
			int f_u = L1->Right()->upper_tex;
			int b_l = L1-> Left()->lower_tex;
			int b_u = L1-> Left()->upper_tex;

			// ignore missing textures
			if (is_null_tex(BA_GetString(f_l))) f_l = 0;
			if (is_null_tex(BA_GetString(f_u))) f_u = 0;
			if (is_null_tex(BA_GetString(b_l))) b_l = 0;
			if (is_null_tex(BA_GetString(b_u))) b_u = 0;

			// try hard to find a usable texture
			int tex = -1;

				 if (front->floorh < back->floorh && f_l > 0) tex = f_l;
			else if (front->floorh > back->floorh && b_l > 0) tex = b_l;
			else if (front-> ceilh > back-> ceilh && f_u > 0) tex = f_u;
			else if (front-> ceilh < back-> ceilh && b_u > 0) tex = b_u;
			else if (f_l > 0) tex = f_l;
			else if (b_l > 0) tex = b_l;
			else if (f_u > 0) tex = f_u;
			else if (b_u > 0) tex = b_u;

			if (tex > 0)
			{
				BA_ChangeSD(L2->right, SideDef::F_MID_TEX, tex);
			}
		}
		else
		{
			const SideDef *RS = L1->Right();
			const SideDef *LS = L1->Left();

			const Sector *F1 = L1->Right()->SecRef();
			const Sector *B1 = L1-> Left()->SecRef();
			const Sector *F2 = L2->Right()->SecRef();
			const Sector *B2 = L2-> Left()->SecRef();

			// logic to determine which sides we copy

			int f_diff1 = B1->floorh - F1->floorh;
			int f_diff2 = B2->floorh - F2->floorh;
			int c_diff1 = B1->ceilh  - F1->ceilh;
			int c_diff2 = B2->ceilh  - F2->ceilh;

			if (f_diff1 * f_diff2 > 0)
			  { /* no change */ }
			else if (f_diff1 * f_diff2 < 0)
			  std::swap(LS, RS);
			else if (c_diff1 * c_diff2 > 0)
			  { /* no change */ }
			else if (c_diff1 * c_diff2 < 0)
			  std::swap(LS, RS);
			else if (L1->start == L2->end || L1->end == L2->start)
			  { /* no change */ }
			else if (L1->start == L2->start || L1->end == L2->end)
			  std::swap(LS, RS);
			else if (F1 == F2 || B1 == B2)
			  { /* no change */ }
			else if (F1 == B1 || F1 == B2 || F2 == B1 || F2 == B2)
			  std::swap(LS, RS);

			// TODO; review if we should copy '-' into lowers or uppers

			BA_ChangeSD(L2->right, SideDef::F_LOWER_TEX, RS->lower_tex);
			BA_ChangeSD(L2->right, SideDef::F_MID_TEX,   RS->mid_tex);
			BA_ChangeSD(L2->right, SideDef::F_UPPER_TEX, RS->upper_tex);

			BA_ChangeSD(L2->left, SideDef::F_LOWER_TEX, LS->lower_tex);
			BA_ChangeSD(L2->left, SideDef::F_MID_TEX,   LS->mid_tex);
			BA_ChangeSD(L2->left, SideDef::F_UPPER_TEX, LS->upper_tex);
		}
	}

	BA_ChangeLD(dest_line, LineDef::F_FLAGS, flags);

	BA_ChangeLD(dest_line, LineDef::F_TYPE, L1->type);
	BA_ChangeLD(dest_line, LineDef::F_TAG,  L1->tag);

	BA_ChangeLD(dest_line, LineDef::F_ARG2, L1->arg2);
	BA_ChangeLD(dest_line, LineDef::F_ARG3, L1->arg3);
	BA_ChangeLD(dest_line, LineDef::F_ARG4, L1->arg4);
	BA_ChangeLD(dest_line, LineDef::F_ARG5, L1->arg5);
}


void CMD_CopyProperties()
{
	if (edit.highlight.is_nil())
	{
		Beep("No target for CopyProperties");
		return;
	}
	else if (edit.Selected->empty())
	{
		Beep("No source for CopyProperties");
		return;
	}
	else if (edit.mode == OBJ_VERTICES)
	{
		Beep("No properties to copy");
		return;
	}


	/* normal mode, SEL --> HILITE */

	if (! Exec_HasFlag("/reverse"))
	{
		if (edit.Selected->count_obj() != 1)
		{
			Beep("Too many sources for CopyProperties");
			return;
		}

		int source = edit.Selected->find_first();
		int target = edit.highlight.num;

		// silently allow copying onto self
		if (source == target)
			return;

		BA_Begin();

		switch (edit.mode)
		{
			case OBJ_SECTORS:
				TransferSectorProperties(source, target);
				break;

			case OBJ_THINGS:
				TransferThingProperties(source, target);
				break;

			case OBJ_LINEDEFS:
				TransferLinedefProperties(source, target, true /* do_tex */);
				break;

			default: break;
		}

		BA_Message("copied properties");

		BA_End();

	}
	else  /* reverse mode, HILITE --> SEL */
	{
		if (edit.Selected->count_obj() == 1 && edit.Selected->find_first() == edit.highlight.num)
		{
			Beep("No selection for CopyProperties");
			return;
		}

		int source = edit.highlight.num;

		selection_iterator_c it;

		BA_Begin();

		for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
		{
			if (*it == source)
				continue;

			switch (edit.mode)
			{
				case OBJ_SECTORS:
					TransferSectorProperties(source, *it);
					break;

				case OBJ_THINGS:
					TransferThingProperties(source, *it);
					break;

				case OBJ_LINEDEFS:
					TransferLinedefProperties(source, *it, true /* do_tex */);
					break;

				default: break;  // fuck you, compiler
			}
		}

		BA_Message("copied properties");

		BA_End();
	}
}



static void Drag_CountOnGrid_Worker(int obj_type, int objnum, int *count, int *total)
{
	switch (obj_type)
	{
		case OBJ_THINGS:
			*total += 1;
			if (grid.OnGrid(Things[objnum]->x, Things[objnum]->y))
				*count += 1;
			break;

		case OBJ_VERTICES:
			*total += 1;
			if (grid.OnGrid(Vertices[objnum]->x, Vertices[objnum]->y))
				*count += 1;
			break;

		case OBJ_LINEDEFS:
			Drag_CountOnGrid_Worker(OBJ_VERTICES, LineDefs[objnum]->start, count, total);
			Drag_CountOnGrid_Worker(OBJ_VERTICES, LineDefs[objnum]->end,   count, total);
			break;

		case OBJ_SECTORS:
			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				LineDef *L = LineDefs[n];

				if (! L->TouchesSector(objnum))
					continue;

				Drag_CountOnGrid_Worker(OBJ_LINEDEFS, n, count, total);
			}
			break;

		default:
			break;
	}
}


static void Drag_CountOnGrid(int *count, int *total)
{
	// Note: the results are approximate, vertices can be counted two
	//       or more times.

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		Drag_CountOnGrid_Worker(edit.mode, *it, count, total);
	}
}


static void Drag_UpdateObjectDist(int obj_type, int objnum, int *x, int *y,
                                  int *best_dist, int map_x, int map_y,
								  bool only_grid)
{
	int x2, y2;

	switch (obj_type)
	{
		case OBJ_THINGS:
			x2 = Things[objnum]->x;
			y2 = Things[objnum]->y;
			break;

		case OBJ_VERTICES:
			x2 = Vertices[objnum]->x;
			y2 = Vertices[objnum]->y;
			break;

		case OBJ_LINEDEFS:
			{
				LineDef *L = LineDefs[objnum];

				Drag_UpdateObjectDist(OBJ_VERTICES, L->start, x, y, best_dist,
									  map_x, map_y, only_grid);

				Drag_UpdateObjectDist(OBJ_VERTICES, L->end,   x, y, best_dist,
				                      map_x, map_y, only_grid);
			}
			return;

		case OBJ_SECTORS:
			// recursively handle all vertices belonging to the sector
			// (some vertices can be processed two or more times, that
			// won't matter though).

			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				LineDef *L = LineDefs[n];

				if (! L->TouchesSector(objnum))
					continue;

				Drag_UpdateObjectDist(OBJ_LINEDEFS, n, x, y, best_dist,
				                      map_x, map_y, only_grid);
			}
			return;

		default:
			return;
	}

	// handle OBJ_THINGS and OBJ_VERTICES

	if (only_grid && ! grid.OnGrid(x2, y2))
		return;

	int dist = ComputeDist(x2 - map_x, y2 - map_y);

	if (dist < *best_dist)
	{
		*x = x2;
		*y = y2;

		*best_dist = dist;
	}
}


void GetDragFocus(int *x, int *y, int map_x, int map_y)
{
	*x = 0;
	*y = 0;

	// determine whether a majority of the object(s) are already on
	// the grid.  If they are, then pick a coordinate that also lies
	// on the grid.
	bool only_grid = false;

	int count = 0;
	int total = 0;

	if (grid.snap)
	{
		Drag_CountOnGrid(&count, &total);

		if (total > 0 && count > total / 2)
			only_grid = true;
	}

	int best_dist = 99999;

	selection_iterator_c it;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		Drag_UpdateObjectDist(edit.mode, *it, x, y, &best_dist,
		                      map_x, map_y, only_grid);
	}
}


//------------------------------------------------------------------------


void transform_t::Clear()
{
	mid_x = mid_y = 0;

	scale_x = scale_y = 1;

	skew_x = skew_y = 0;

	rotate = 0;
}


void transform_t::Apply(int *x, int *y) const
{
	float x0 = *x - mid_x;
	float y0 = *y - mid_y;

	if (rotate)
	{
		float s = sin(rotate * M_PI / 32768.0);
		float c = cos(rotate * M_PI / 32768.0);

		float x1 = x0;
		float y1 = y0;

		x0 = x1 * c - y1 * s;
		y0 = y1 * c + x1 * s;
	}

	if (skew_x || skew_y)
	{
		float x1 = x0;
		float y1 = y0;

		x0 = x1 + y1 * skew_x;
		y0 = y1 + x1 * skew_y;
	}

	x0 = x0 * scale_x;
	y0 = y0 * scale_y;

	*x = mid_x + I_ROUND(x0);
	*y = mid_y + I_ROUND(y0);
}


//
// Return the coordinate of the centre of a group of objects.
//
// This is computed using an average of all the coordinates, which can
// often give a different result than using the middle of the bounding
// box.
//
void Objs_CalcMiddle(selection_c * list, int *x, int *y)
{
	*x = *y = 0;

	if (list->empty())
		return;

	selection_iterator_c it;

	double sum_x = 0;
	double sum_y = 0;

	int count = 0;

	switch (list->what_type())
	{
		case OBJ_THINGS:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it, ++count)
			{
				sum_x += Things[*it]->x;
				sum_y += Things[*it]->y;
			}
			break;
		}

		case OBJ_VERTICES:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it, ++count)
			{
				sum_x += Vertices[*it]->x;
				sum_y += Vertices[*it]->y;
			}
			break;
		}

		// everything else: just use the vertices
		default:
		{
			selection_c verts(OBJ_VERTICES);

			ConvertSelection(list, &verts);

			Objs_CalcMiddle(&verts, x, y);
			return;
		}
	}

	SYS_ASSERT(count > 0);

	*x = I_ROUND(sum_x / count);
	*y = I_ROUND(sum_y / count);
}


//
// returns a bounding box that completely includes a list of objects.
// when the list is empty, bottom-left coordinate is arbitrary.
//
void Objs_CalcBBox(selection_c * list, int *x1, int *y1, int *x2, int *y2)
{
	if (list->empty())
	{
		*x1 = *y1 = 0;
		*x2 = *y2 = 0;
		return;
	}

	*x1 = *y1 = +777777;
	*x2 = *y2 = -777777;

	selection_iterator_c it;

	switch (list->what_type())
	{
		case OBJ_THINGS:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it)
			{
				const Thing *T = Things[*it];

				const thingtype_t *info = M_GetThingType(T->type);
				int r = info->radius;

				if (T->x - r < *x1) *x1 = T->x - r;
				if (T->y - r < *y1) *y1 = T->y - r;
				if (T->x + r > *x2) *x2 = T->x + r;
				if (T->y + r > *y2) *y2 = T->y + r;
			}
			break;
		}

		case OBJ_VERTICES:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it)
			{
				const Vertex *V = Vertices[*it];

				if (V->x < *x1) *x1 = V->x;
				if (V->y < *y1) *y1 = V->y;
				if (V->x > *x2) *x2 = V->x;
				if (V->y > *y2) *y2 = V->y;
			}
			break;
		}

		// everything else: just use the vertices
		default:
		{
			selection_c verts(OBJ_VERTICES);

			ConvertSelection(list, &verts);

			Objs_CalcBBox(&verts, x1, y1, x2, y2);
			return;
		}
	}

	SYS_ASSERT(*x1 <= *x2);
	SYS_ASSERT(*y1 <= *y2);
}


static void DoMirrorThings(selection_c& list, bool is_vert, int mid_x, int mid_y)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		if (is_vert)
		{
			BA_ChangeTH(*it, Thing::F_Y, 2*mid_y - T->y);

			if (T->angle != 0)
				BA_ChangeTH(*it, Thing::F_ANGLE, 360 - T->angle);
		}
		else
		{
			BA_ChangeTH(*it, Thing::F_X, 2*mid_x - T->x);

			if (T->angle > 180)
				BA_ChangeTH(*it, Thing::F_ANGLE, 540 - T->angle);
			else
				BA_ChangeTH(*it, Thing::F_ANGLE, 180 - T->angle);
		}
	}
}


static void DoMirrorVertices(selection_c& list, bool is_vert, int mid_x, int mid_y)
{
	selection_c verts(OBJ_VERTICES);

	ConvertSelection(&list, &verts);

	selection_iterator_c it;

	for (verts.begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex * V = Vertices[*it];

		if (is_vert)
			BA_ChangeVT(*it, Vertex::F_Y, 2*mid_y - V->y);
		else
			BA_ChangeVT(*it, Vertex::F_X, 2*mid_x - V->x);
	}

	// flip linedefs too !!
	selection_c lines(OBJ_LINEDEFS);

	ConvertSelection(&verts, &lines);

	for (lines.begin(&it) ; !it.at_end() ; ++it)
	{
		LineDef * L = LineDefs[*it];

		int start = L->start;
		int end   = L->end;

		BA_ChangeLD(*it, LineDef::F_START, end);
		BA_ChangeLD(*it, LineDef::F_END, start);
	}
}


static void DoMirrorStuff(selection_c& list, bool is_vert, int mid_x, int mid_y)
{
	if (edit.mode == OBJ_THINGS)
	{
		DoMirrorThings(list, is_vert, mid_x, mid_y);
		return;
	}

	// everything else just modifies the vertices

	if (edit.mode == OBJ_SECTORS)
	{
		// handle things in Sectors mode too
		selection_c things(OBJ_THINGS);

		ConvertSelection(&list, &things);

		DoMirrorThings(things, is_vert, mid_x, mid_y);
	}

	DoMirrorVertices(list, is_vert, mid_x, mid_y);
}


void CMD_Mirror()
{
	selection_c list;

	if (! GetCurrentObjects(&list))
	{
		Beep("No objects to mirror");
		return;
	}

	bool is_vert = false;

	if (tolower(EXEC_Param[0][0]) == 'v')
		is_vert = true;

	int mid_x, mid_y;

	Objs_CalcMiddle(&list, &mid_x, &mid_y);

	BA_Begin();

	BA_MessageForSel("mirrored", &list, is_vert ? " vertically" : " horizontally");

	DoMirrorStuff(list, is_vert, mid_x, mid_y);

	BA_End();
}


static void DoRotate90Things(selection_c& list, bool anti_clockwise, int mid_x, int mid_y)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		int old_x = T->x;
		int old_y = T->y;

		if (anti_clockwise)
		{
			BA_ChangeTH(*it, Thing::F_X, mid_x - old_y + mid_y);
			BA_ChangeTH(*it, Thing::F_Y, mid_y + old_x - mid_x);

			BA_ChangeTH(*it, Thing::F_ANGLE, calc_new_angle(T->angle, +90));
		}
		else
		{
			BA_ChangeTH(*it, Thing::F_X, mid_x + old_y - mid_y);
			BA_ChangeTH(*it, Thing::F_Y, mid_y - old_x + mid_x);

			BA_ChangeTH(*it, Thing::F_ANGLE, calc_new_angle(T->angle, -90));
		}
	}
}


void CMD_Rotate90()
{
	if (EXEC_Param[0] == 0)
	{
		Beep("Rotate90: missing keyword");
		return;
	}

	bool anti_clockwise = (tolower(EXEC_Param[0][0]) == 'a');

	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No objects to rotate");
		return;
	}

	int mid_x, mid_y;

	Objs_CalcMiddle(&list, &mid_x, &mid_y);

	BA_Begin();

	BA_MessageForSel("rotated", &list, anti_clockwise ? " anti-clockwise" : " clockwise");

	if (edit.mode == OBJ_THINGS)
	{
		DoRotate90Things(list, anti_clockwise, mid_x, mid_y);
	}
	else
	{
		// handle things inside sectors
		if (edit.mode == OBJ_SECTORS)
		{
			selection_c things(OBJ_THINGS);

			ConvertSelection(&list, &things);

			DoRotate90Things(things, anti_clockwise, mid_x, mid_y);
		}

		// everything else just rotates the vertices
		selection_c verts(OBJ_VERTICES);

		ConvertSelection(&list, &verts);

		for (verts.begin(&it) ; !it.at_end() ; ++it)
		{
			const Vertex * V = Vertices[*it];

			int old_x = V->x;
			int old_y = V->y;

			if (anti_clockwise)
			{
				BA_ChangeVT(*it, Vertex::F_X, mid_x - old_y + mid_y);
				BA_ChangeVT(*it, Vertex::F_Y, mid_y + old_x - mid_x);
			}
			else
			{
				BA_ChangeVT(*it, Vertex::F_X, mid_x + old_y - mid_y);
				BA_ChangeVT(*it, Vertex::F_Y, mid_y - old_x + mid_x);
			}
		}
	}

	BA_End();
}


static void DoEnlargeThings(selection_c& list, int mul, int mid_x, int mid_y)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		int dx = T->x - mid_x;
		int dy = T->y - mid_y;

		BA_ChangeTH(*it, Thing::F_X, mid_x + dx * mul);
		BA_ChangeTH(*it, Thing::F_Y, mid_y + dy * mul);
	}
}


void CMD_Enlarge()
{
	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No objects to enlarge");
		return;
	}

	int mul = 2;
	if (EXEC_Param[0][0])
		mul = atoi(EXEC_Param[0]);

	if (mul < 1 || mul > 64)
	{
		Beep("Bad parameter for enlarge: '%s'", EXEC_Param[0]);
		return;
	}

	int mid_x, mid_y, hx, hy;

	// TODO: CONFIG ITEM
	if (true)
		Objs_CalcMiddle(&list, &mid_x, &mid_y);
	else
	{
		Objs_CalcBBox(&list, &mid_x, &mid_y, &hx, &hy);

		mid_x = mid_x + (hx - mid_x) / 2;
		mid_y = mid_y + (hy - mid_y) / 2;
	}

	BA_Begin();

	BA_MessageForSel("enlarged", &list);

	if (edit.mode == OBJ_THINGS)
	{
		DoEnlargeThings(list, mul, mid_x, mid_y);
	}
	else
	{
		// handle things inside sectors
		if (edit.mode == OBJ_SECTORS)
		{
			selection_c things(OBJ_THINGS);

			ConvertSelection(&list, &things);

			DoEnlargeThings(things, mul, mid_x, mid_y);
		}

		// everything else just scales the vertices
		selection_c verts(OBJ_VERTICES);

		ConvertSelection(&list, &verts);

		for (verts.begin(&it) ; !it.at_end() ; ++it)
		{
			const Vertex * V = Vertices[*it];

			int dx = V->x - mid_x;
			int dy = V->y - mid_y;

			BA_ChangeVT(*it, Vertex::F_X, mid_x + dx * mul);
			BA_ChangeVT(*it, Vertex::F_Y, mid_y + dy * mul);
		}
	}

	BA_End();
}


static void DoShrinkThings(selection_c& list, int div, int mid_x, int mid_y)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		int dx = T->x - mid_x;
		int dy = T->y - mid_y;

		BA_ChangeTH(*it, Thing::F_X, mid_x + dx / div);
		BA_ChangeTH(*it, Thing::F_Y, mid_y + dy / div);
	}
}


void CMD_Shrink()
{
	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No objects to shrink");
		return;
	}

	int div = 2;
	if (EXEC_Param[0][0])
		div = atoi(EXEC_Param[0]);

	if (div < 1 || div > 64)
	{
		Beep("Bad parameter for shrink: '%s'", EXEC_Param[0]);
		return;
	}

	int mid_x, mid_y, hx, hy;

	// TODO: CONFIG ITEM
	if (true)
		Objs_CalcMiddle(&list, &mid_x, &mid_y);
	else
	{
		Objs_CalcBBox(&list, &mid_x, &mid_y, &hx, &hy);

		mid_x = mid_x + (hx - mid_x) / 2;
		mid_y = mid_y + (hy - mid_y) / 2;
	}

	BA_Begin();

	BA_MessageForSel("shrunk", &list);

	if (edit.mode == OBJ_THINGS)
	{
		DoShrinkThings(list, div, mid_x, mid_y);
	}
	else
	{
		// handle things inside sectors
		if (edit.mode == OBJ_SECTORS)
		{
			selection_c things(OBJ_THINGS);

			ConvertSelection(&list, &things);

			DoShrinkThings(things, div, mid_x, mid_y);
		}

		// everything else just scales the vertices
		selection_c verts(OBJ_VERTICES);

		ConvertSelection(&list, &verts);

		for (verts.begin(&it) ; !it.at_end() ; ++it)
		{
			const Vertex * V = Vertices[*it];

			int dx = V->x - mid_x;
			int dy = V->y - mid_y;

			BA_ChangeVT(*it, Vertex::F_X, mid_x + dx / div);
			BA_ChangeVT(*it, Vertex::F_Y, mid_y + dy / div);
		}
	}

	BA_End();
}


static void DoScaleTwoThings(selection_c& list, transform_t& param)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		int new_x = T->x;
		int new_y = T->y;

		param.Apply(&new_x, &new_y);

		BA_ChangeTH(*it, Thing::F_X, new_x);
		BA_ChangeTH(*it, Thing::F_Y, new_y);

		float rot1 = param.rotate / 8192.0;

		int ang_diff = I_ROUND(rot1) * 45.0;

		if (ang_diff)
		{
			BA_ChangeTH(*it, Thing::F_ANGLE, calc_new_angle(T->angle, ang_diff));
		}
	}
}


static void DoScaleTwoVertices(selection_c& list, transform_t& param)
{
	selection_c verts(OBJ_VERTICES);

	ConvertSelection(&list, &verts);

	selection_iterator_c it;

	for (verts.begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex * V = Vertices[*it];

		int new_x = V->x;
		int new_y = V->y;

		param.Apply(&new_x, &new_y);

		BA_ChangeVT(*it, Vertex::F_X, new_x);
		BA_ChangeVT(*it, Vertex::F_Y, new_y);
	}
}


static void DoScaleTwoStuff(selection_c& list, transform_t& param)
{
	if (edit.mode == OBJ_THINGS)
	{
		DoScaleTwoThings(list, param);
		return;
	}

	// everything else just modifies the vertices

	if (edit.mode == OBJ_SECTORS)
	{
		// handle things in Sectors mode too
		selection_c things(OBJ_THINGS);

		ConvertSelection(&list, &things);

		DoScaleTwoThings(things, param);
	}

	DoScaleTwoVertices(list, param);
}


void TransformObjects(transform_t& param)
{
	// this is called by the MOUSE2 dynamic scaling code

	SYS_ASSERT(edit.Selected->notempty());

	BA_Begin();

	BA_MessageForSel("scaled", edit.Selected);

	if (param.scale_x < 0)
	{
		param.scale_x = -param.scale_x;

		DoMirrorStuff(*edit.Selected, false /* is_vert */, param.mid_x, param.mid_y);
	}

	if (param.scale_y < 0)
	{
		param.scale_y = -param.scale_y;

		DoMirrorStuff(*edit.Selected, true /* is_vert */, param.mid_x, param.mid_y);
	}

	DoScaleTwoStuff(*edit.Selected, param);

	BA_End();
}


static void DetermineOrigin(transform_t& param, int pos_x, int pos_y)
{
	if (pos_x == 0 && pos_y == 0)
	{
		Objs_CalcMiddle(edit.Selected, &param.mid_x, &param.mid_y);
		return;
	}

	int lx, ly, hx, hy;

	Objs_CalcBBox(edit.Selected, &lx, &ly, &hx, &hy);

	if (pos_x < 0)
		param.mid_x = lx;
	else if (pos_x > 0)
		param.mid_x = hx;
	else
		param.mid_x = lx + (hx - lx) / 2;

	if (pos_y < 0)
		param.mid_y = ly;
	else if (pos_y > 0)
		param.mid_y = hy;
	else
		param.mid_y = ly + (hy - ly) / 2;
}


void ScaleObjects3(double scale_x, double scale_y, int pos_x, int pos_y)
{
	SYS_ASSERT(scale_x > 0);
	SYS_ASSERT(scale_y > 0);

	transform_t param;

	param.Clear();

	param.scale_x = scale_x;
	param.scale_y = scale_y;

	DetermineOrigin(param, pos_x, pos_y);

	BA_Begin();
	BA_MessageForSel("scaled", edit.Selected);
	{
		DoScaleTwoStuff(*edit.Selected, param);
	}
	BA_End();
}


static void DoScaleSectorHeights(selection_c& list, double scale_z, int pos_z)
{
	SYS_ASSERT(! list.empty());

	selection_iterator_c it;

	// determine Z range and origin
	int lz = +99999;
	int hz = -99999;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Sector * S = Sectors[*it];

		lz = MIN(lz, S->floorh);
		hz = MAX(hz, S->ceilh);
	}

	int mid_z;

	if (pos_z < 0)
		mid_z = lz;
	else if (pos_z > 0)
		mid_z = hz;
	else
		mid_z = lz + (hz - lz) / 2;

	// apply the scaling

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Sector * S = Sectors[*it];

		int new_f = mid_z + I_ROUND((S->floorh - mid_z) * scale_z);
		int new_c = mid_z + I_ROUND((S-> ceilh - mid_z) * scale_z);

		BA_ChangeSEC(*it, Sector::F_FLOORH, new_f);
		BA_ChangeSEC(*it, Sector::F_CEILH,  new_c);
	}
}

void ScaleObjects4(double scale_x, double scale_y, double scale_z,
                   int pos_x, int pos_y, int pos_z)
{
	SYS_ASSERT(edit.mode == OBJ_SECTORS);

	transform_t param;

	param.Clear();

	param.scale_x = scale_x;
	param.scale_y = scale_y;

	DetermineOrigin(param, pos_x, pos_y);

	BA_Begin();
	BA_MessageForSel("scaled", edit.Selected);
	{
		DoScaleTwoStuff(*edit.Selected, param);
		DoScaleSectorHeights(*edit.Selected, scale_z, pos_z);
	}
	BA_End();
}


void RotateObjects3(double deg, int pos_x, int pos_y)
{
	transform_t param;

	param.Clear();

	param.rotate = I_ROUND(deg * 65536.0 / 360.0);

	DetermineOrigin(param, pos_x, pos_y);

	BA_Begin();
	BA_MessageForSel("rotated", edit.Selected);
	{
		DoScaleTwoStuff(*edit.Selected, param);
	}
	BA_End();
}


static bool SpotInUse(obj_type_e obj_type, int map_x, int map_y)
{
	switch (obj_type)
	{
		case OBJ_THINGS:
			for (int n = 0 ; n < NumThings ; n++)
				if (Things[n]->x == map_x && Things[n]->y == map_y)
					return true;
			return false;

		case OBJ_VERTICES:
			for (int n = 0 ; n < NumVertices ; n++)
				if (Vertices[n]->x == map_x && Vertices[n]->y == map_y)
					return true;
			return false;

		default:
			BugError("IsSpotVacant: bad object type\n");
			return false;
	}
}


static void Quantize_Things(selection_c& list)
{
	// remember the things which we moved
	// (since we cannot modify the selection while we iterate over it)
	selection_c moved(list.what_type());

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		if (grid.OnGrid(T->x, T->y))
		{
			moved.set(*it);
			continue;
		}

		for (int pass = 0 ; pass < 4 ; pass++)
		{
			int new_x = grid.QuantSnapX(T->x, pass & 1);
			int new_y = grid.QuantSnapY(T->y, pass & 2);

			if (! SpotInUse(OBJ_THINGS, new_x, new_y))
			{
				BA_ChangeTH(*it, Thing::F_X, new_x);
				BA_ChangeTH(*it, Thing::F_Y, new_y);

				moved.set(*it);
				break;
			}
		}
	}

	list.unmerge(moved);

	if (list.notempty())
		Beep("Quantize: could not move %d things", list.count_obj());
}


static void Quantize_Vertices(selection_c& list)
{
	// first : do an analysis pass, remember vertices that are part
	// of a horizontal or vertical line (and both in the selection)
	// and limit the movement of those vertices to ensure the lines
	// stay horizontal or vertical.

	enum
	{
		V_HORIZ   = (1 << 0),
		V_VERT    = (1 << 1),
		V_DIAG_NE = (1 << 2),
		V_DIAG_SE = (1 << 3)
	};

	byte * vert_modes = new byte[NumVertices];

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		// require both vertices of the linedef to be in the selection
		if (! (list.get(L->start) && list.get(L->end)))
			continue;

		// IDEA: make this a method of LineDef
		int x1 = L->Start()->x;
		int y1 = L->Start()->y;
		int x2 = L->End()->x;
		int y2 = L->End()->y;

		if (y1 == y2)
		{
			vert_modes[L->start] |= V_HORIZ;
			vert_modes[L->end]   |= V_HORIZ;
		}
		else if (x1 == x2)
		{
			vert_modes[L->start] |= V_VERT;
			vert_modes[L->end]   |= V_VERT;
		}
		else if ((x1 < x2 && y1 < y2) || (x1 > x2 && y1 > y2))
		{
			vert_modes[L->start] |= V_DIAG_NE;
			vert_modes[L->end]   |= V_DIAG_NE;
		}
		else
		{
			vert_modes[L->start] |= V_DIAG_SE;
			vert_modes[L->end]   |= V_DIAG_SE;
		}
	}

	// remember the vertices which we moved
	// (since we cannot modify the selection while we iterate over it)
	selection_c moved(list.what_type());

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex * V = Vertices[*it];

		if (grid.OnGrid(V->x, V->y))
		{
			moved.set(*it);
			continue;
		}

		byte mode = vert_modes[*it];

		for (int pass = 0 ; pass < 4 ; pass++)
		{
			int x_dir, y_dir;

			int new_x = grid.QuantSnapX(V->x, pass & 1, &x_dir);
			int new_y = grid.QuantSnapY(V->y, pass & 2, &y_dir);

			// keep horizontal lines horizontal
			if ((mode & V_HORIZ) && (pass & 2))
				continue;

			// keep vertical lines vertical
			if ((mode & V_VERT) && (pass & 1))
				continue;

			// TODO: keep diagonal lines diagonal...

			if (! SpotInUse(OBJ_VERTICES, new_x, new_y))
			{
				BA_ChangeVT(*it, Vertex::F_X, new_x);
				BA_ChangeVT(*it, Vertex::F_Y, new_y);

				moved.set(*it);
				break;
			}
		}
	}

	delete[] vert_modes;

	list.unmerge(moved);

	if (list.notempty())
		Beep("Quantize: could not move %d vertices", list.count_obj());
}


void CMD_Quantize()
{
	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("Nothing to quantize");
			return;
		}

		edit.Selected->set(edit.highlight.num);
	}

	BA_Begin();

	BA_MessageForSel("quantized", edit.Selected);

	switch (edit.mode)
	{
		case OBJ_THINGS:
			Quantize_Things(*edit.Selected);
			break;

		case OBJ_VERTICES:
			Quantize_Vertices(*edit.Selected);
			break;

		// everything else merely quantizes vertices
		default:
		{
			selection_c verts(OBJ_VERTICES);

			ConvertSelection(edit.Selected, &verts);

			Quantize_Vertices(verts);

			Selection_Clear();
			break;
		}
	}

	BA_End();

	edit.error_mode = true;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
