//------------------------------------------------------------------------
//  OBJECT OPERATIONS
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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include <algorithm>

#include "e_hover.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_sector.h"
#include "e_things.h"
#include "e_vertex.h"
#include "m_config.h"
#include "m_game.h"
#include "e_objects.h"
#include "r_grid.h"
#include "w_rawdef.h"

#include "ui_window.h"


// config items
int  config::new_sector_size = 128;

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

	for (sel_iter_c it(list) ; !it.done() ; it.next())
		objnums.push_back(*it);

	std::sort(objnums.begin(), objnums.end());

	for (int i = (int)objnums.size()-1 ; i >= 0 ; i--)
		gDocument.basis.del(list->what_type(), objnums[i]);
}


static void CreateSquare(int model)
{
	int new_sec = gDocument.basis.addNew(ObjType::sectors);

	if (model >= 0)
		*gDocument.sectors[new_sec] = *gDocument.sectors[model];
	else
		gDocument.sectors[new_sec]->SetDefaults();

	double x1 = grid.QuantSnapX(edit.map_x, false);
	double y1 = grid.QuantSnapX(edit.map_y, false);

	double x2 = x1 + config::new_sector_size;
	double y2 = y1 + config::new_sector_size;

	for (int i = 0 ; i < 4 ; i++)
	{
		int new_v = gDocument.basis.addNew(ObjType::vertices);
		Vertex *V = gDocument.vertices[new_v];

		V->SetRawX((i >= 2) ? x2 : x1);
		V->SetRawY((i==1 || i==2) ? y2 : y1);

		int new_sd = gDocument.basis.addNew(ObjType::sidedefs);

		gDocument.sidedefs[new_sd]->SetDefaults(false);
		gDocument.sidedefs[new_sd]->sector = new_sec;

		int new_ld = gDocument.basis.addNew(ObjType::linedefs);

		LineDef * L = gDocument.linedefs[new_ld];

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


	gDocument.basis.begin();

	int new_t = gDocument.basis.addNew(ObjType::things);
	Thing *T = gDocument.things[new_t];

	if(model >= 0)
		*T = *gDocument.things[model];
	else
	{
		T->type = default_thing;
		T->options = MTF_Easy | MTF_Medium | MTF_Hard;

		if (instance::Level_format != MapFormat::doom)
		{
			T->options |= MTF_Hexen_SP | MTF_Hexen_COOP | MTF_Hexen_DM;
			T->options |= MTF_Hexen_Fighter | MTF_Hexen_Cleric | MTF_Hexen_Mage;
		}
	}

	T->SetRawX(grid.SnapX(edit.map_x));
	T->SetRawY(grid.SnapY(edit.map_y));

	recent_things.insert_number(T->type);

	gDocument.basis.setMessage("added thing #%d", new_t);
	gDocument.basis.end();


	// select it
	Selection_Clear();

	edit.Selected->set(new_t);
}


static int Sector_New(int model = -1, int model2 = -1, int model3 = -1)
{
	int new_sec = gDocument.basis.addNew(ObjType::sectors);

	if (model < 0) model = model2;
	if (model < 0) model = model3;

	if (model < 0)
		gDocument.sectors[new_sec]->SetDefaults();
	else
		*gDocument.sectors[new_sec] = *gDocument.sectors[model];

	return new_sec;
}


static bool CheckClosedLoop(int new_ld, int v1, int v2, selection_c *flip)
{
	// returns true if we assigned a sector (so drawing should stop)

	struct check_closed_data_t
	{
		lineloop_c loop = lineloop_c(gDocument);

		bool ok;

		// what sector we face, -1 for VOID
		int sec;

		double length;

	} left, right;

	// trace the loops on either side of the new line

	 left.ok = gDocument.secmod.traceLineLoop(new_ld, Side::left,   left.loop);
	right.ok = gDocument.secmod.traceLineLoop(new_ld, Side::right, right.loop);

#ifdef DEBUG_LOOP
	fprintf(stderr, "CLOSED LOOP : left_ok:%d right_ok:%d\n",
			left.ok ? 1 : 0, right.ok ? 1 : 0);
#endif

	if (! (left.ok && right.ok))
	{
		// this is harder to trigger than you might think.

		// TODO : find some cases, see what is needed

#ifdef DEBUG_LOOP
	fprintf(stderr, "--> bad  bad  bad  bad  bad <--\n");
#endif
		return false;
	}


	// check if the loops are the same, which means we have NOT
	// split the sector (or created a new one)

	if ( left.loop.get(right.loop.lines[0], right.loop.sides[0]) ||
		right.loop.get( left.loop.lines[0],  left.loop.sides[0]))
	{
		// nothing to do, let user keep drawing
		return false;
	}


#ifdef DEBUG_LOOP
	fprintf(stderr, "--> %s / %s\n",
			 left.loop.faces_outward ? "OUTIE" : "innie",
			right.loop.faces_outward ? "OUTIE" : "innie");
#endif

	 left.sec =  left.loop.DetermineSector();
	right.sec = right.loop.DetermineSector();

#ifdef DEBUG_LOOP
	fprintf(stderr, "sec %d / %d\n", left.sec, right.sec);
#endif

	 left.length =  left.loop.TotalLength();
	right.length = right.loop.TotalLength();

	if (!  left.loop.faces_outward)  left.loop.FindIslands();
	if (! right.loop.faces_outward) right.loop.FindIslands();


	/* --- handle outie --- */

	// it is probably impossible for both loops to face outward, so
	// we only need to handle two cases: both innie, or innie + outie.

	if (left.loop.faces_outward && left.sec >= 0)
	{
		left.loop.AssignSector(left.sec, flip);
	}
	else if (right.loop.faces_outward && right.sec >= 0)
	{
		right.loop.AssignSector(right.sec, flip);
	}

	// create a void island when drawing anti-clockwise inside an
	// existing sector, unless new island surrounds other islands.
	if (right.loop.faces_outward && right.sec >= 0 &&
		left.loop.AllBare() && left.loop.islands.empty())
	{
		return true;
	}

	if (left.loop.faces_outward || right.loop.faces_outward)
	{
		lineloop_c& innie = left.loop.faces_outward ? right.loop : left.loop;

		// TODO : REVIEW NeighboringSector(), it's a bit random what we get
		int new_sec = Sector_New(innie.NeighboringSector());

		innie.AssignSector(new_sec, flip);
		return true;
	}


	/* --- handle two innies --- */

	// check if the sectors in each loop are different.
	// this is not the usual situation!  we assume the user has
	// deleted a linedef or two and is correcting the geometry.

	if (left.sec != right.sec)
	{
		if (right.sec >= 0) right.loop.AssignSector(right.sec, flip);
		if ( left.sec >= 0)  left.loop.AssignSector( left.sec, flip);

		return true;
	}

	// we are creating a NEW sector in one loop (the smallest),
	// and updating the other loop (unless it is VOID).

	// the ordering here is significant, and ensures that the
	// new linedef usually ends at v2 (the final vertex).

	if (left.length < right.length)
	{
		int new_sec = Sector_New(left.sec, right.sec, left.loop.NeighboringSector());

		if (right.sec >= 0)
			right.loop.AssignSector(right.sec, flip);

		left.loop.AssignSector(new_sec, flip);
	}
	else
	{
		int new_sec = Sector_New(right.sec, left.sec, right.loop.NeighboringSector());

		right.loop.AssignSector(new_sec, flip);

		if (left.sec >= 0)
			left.loop.AssignSector(left.sec, flip);
	}

	return true;
}


static void Insert_LineDef(int v1, int v2, bool no_fill = false)
{
	if (gDocument.linemod.linedefAlreadyExists(v1, v2))
		return;

	int new_ld = gDocument.basis.addNew(ObjType::linedefs);

	LineDef * L = gDocument.linedefs[new_ld];

	L->start = v1;
	L->end   = v2;
	L->flags = MLF_Blocking;

	if (no_fill)
		return;

	if (gDocument.vertmod.howManyLinedefs(v1) >= 2 &&
		gDocument.vertmod.howManyLinedefs(v2) >= 2)
	{
		selection_c flip(ObjType::linedefs);

		CheckClosedLoop(new_ld, v1, v2, &flip);

		gDocument.linemod.flipLinedefGroup(&flip);
	}
}


static void Insert_LineDef_autosplit(int v1, int v2, bool no_fill = false)
{
	// Find a linedef which this new line would cross, and if it exists
	// add a vertex there and create TWO lines.  Also handle a vertex
	// that this line crosses (sits on) similarly.

///  fprintf(stderr, "Insert_LineDef_autosplit %d..%d\n", v1, v2);

	crossing_state_c cross(gDocument);

	gDocument.hover.findCrossingPoints(cross,
		gDocument.vertices[v1]->x(), gDocument.vertices[v1]->y(), v1,
		gDocument.vertices[v2]->x(), gDocument.vertices[v2]->y(), v2);

	cross.SplitAllLines();

	int cur_v = v1;

	for (unsigned int k = 0 ; k < cross.points.size() ; k++)
	{
		int next_v = cross.points[k].vert;

		SYS_ASSERT(next_v != v1);
		SYS_ASSERT(next_v != v2);

		Insert_LineDef(cur_v, next_v, no_fill);

		cur_v = next_v;
	}

	Insert_LineDef(cur_v, v2, no_fill);
}


static void Insert_Vertex(bool force_continue, bool no_fill)
{
	bool closed_a_loop = false;

	// when these both >= 0, we will add a linedef between them
	int old_vert = -1;
	int new_vert = -1;

	double new_x = grid.SnapX(edit.map_x);
	double new_y = grid.SnapY(edit.map_y);

	int orig_num_sectors = gDocument.numSectors();


	// are we drawing a line?
	if (edit.action == ACT_DRAW_LINE)
	{
		old_vert = edit.draw_from.num;

		new_x = edit.draw_to_x;
		new_y = edit.draw_to_y;
	}

	// a linedef which we are splitting (usually none)
	int split_ld = edit.split_line.valid() ? edit.split_line.num : -1;

	if (split_ld >= 0)
	{
		new_x = edit.split_x;
		new_y = edit.split_y;

		// prevent creating an overlapping line when splitting
		if (old_vert >= 0 &&
			gDocument.linedefs[split_ld]->TouchesVertex(old_vert))
		{
			old_vert = -1;
		}
	}
	else
	{
		// not splitting a line.
		// check if there is a "nearby" vertex (e.g. the highlighted one)

		if (edit.highlight.valid())
			new_vert = edit.highlight.num;

		// if no highlight, look for a vertex at snapped coord
		if (new_vert < 0 && grid.snap && ! (edit.action == ACT_DRAW_LINE))
			new_vert = gDocument.vertmod.findExact(TO_COORD(new_x), TO_COORD(new_y));

		//
		// handle a highlighted/snapped vertex.
		// either start drawing from it, or finish a loop at it.
		//
		if (new_vert >= 0)
		{
			// just ignore when highlight is same as drawing-start
			if (old_vert >= 0 && *gDocument.vertices[old_vert] == *gDocument.vertices[new_vert])
			{
				edit.Selected->set(old_vert);
				return;
			}

			// a plain INSERT will attempt to fix a dangling vertex
			if (edit.action == ACT_NOTHING)
			{
				if (gDocument.vertmod.tryFixDangler(new_vert))
				{
					// a vertex was deleted, selection/highlight is now invalid
					return;
				}
			}

			// our insertion point is an existing vertex, and we are not
			// in drawing mode, so there is no edit operation to perform.
			if (old_vert < 0)
			{
				old_vert = new_vert;
				new_vert = -1;

				goto begin_drawing;
			}

			// handle case where a line already exists between the two vertices
			if (gDocument.linemod.linedefAlreadyExists(old_vert, new_vert))
			{
				// just continue drawing from the second vertex
				edit.draw_from = Objid(ObjType::vertices, new_vert);
				edit.Selected->set(new_vert);
				return;
			}
		}
	}

	// at here: if new_vert >= 0, then old_vert >= 0 and split_ld < 0


	// would we create a new vertex on top of an existing one?
	if (new_vert < 0 && old_vert >= 0 &&
		gDocument.vertices[old_vert]->Matches(MakeValidCoord(new_x), MakeValidCoord(new_y)))
	{
		edit.Selected->set(old_vert);
		return;
	}


	gDocument.basis.begin();


	if (new_vert < 0)
	{
		new_vert = gDocument.basis.addNew(ObjType::vertices);

		Vertex *V = gDocument.vertices[new_vert];

		V->SetRawXY(new_x, new_y);

		edit.draw_from = Objid(ObjType::vertices, new_vert);
		edit.Selected->set(new_vert);

		// splitting an existing line?
		if (split_ld >= 0)
		{
			gDocument.linemod.splitLinedefAtVertex(split_ld, new_vert);
			gDocument.basis.setMessage("split linedef #%d", split_ld);
		}
		else
		{
			gDocument.basis.setMessage("added vertex #%d", new_vert);
		}
	}


	if (old_vert < 0)
	{
		// there is no starting vertex, therefore no linedef can be added
		old_vert = new_vert;
		new_vert = -1;
	}
	else
	{
		// closing a loop?
		if (!force_continue && gDocument.vertmod.howManyLinedefs(new_vert) > 0)
		{
			closed_a_loop = true;
		}

		//
		// adding a linedef
		//
		SYS_ASSERT(old_vert != new_vert);

		// this can make new sectors too
		Insert_LineDef_autosplit(old_vert, new_vert, no_fill);

		gDocument.basis.setMessage("added linedef");

		edit.draw_from = Objid(ObjType::vertices, new_vert);
		edit.Selected->set(new_vert);
	}


	gDocument.basis.end();


begin_drawing:
	// begin drawing mode?
	if (edit.action == ACT_NOTHING && !closed_a_loop &&
		old_vert >= 0 && new_vert < 0)
	{
		Selection_Clear();

		edit.draw_from = Objid(ObjType::vertices, old_vert);
		edit.Selected->set(old_vert);

		edit.draw_to_x = gDocument.vertices[old_vert]->x();
		edit.draw_to_y = gDocument.vertices[old_vert]->y();

		Editor_SetAction(ACT_DRAW_LINE);
	}

	// stop drawing mode?
	if (closed_a_loop && !force_continue)
	{
		Editor_ClearAction();
	}

	// select vertices of a newly created sector?
	if (select_verts_of_new_sectors && closed_a_loop &&
		gDocument.numSectors() > orig_num_sectors)
	{
		selection_c sel(ObjType::sectors);

		// more than one sector may have been created, pick the last
		sel.set(gDocument.numSectors() - 1);

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
	if (gDocument.hover.isPointOutsideOfMap(edit.map_x, edit.map_y))
	{
		gDocument.basis.begin();
		gDocument.basis.setMessage("added sector (outside map)");

		int model = -1;
		if (sel_count > 0)
			model = edit.Selected->find_first();

		CreateSquare(model);

		gDocument.basis.end();
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


	gDocument.basis.begin();
	gDocument.basis.setMessage("added new sector");

	bool ok = gDocument.secmod.assignSectorToSpace(edit.map_x, edit.map_y, -1 /* create */, model);

	gDocument.basis.end();

	// select the new sector
	if (ok)
	{
		Selection_Clear();
		edit.Selected->set(gDocument.numSectors() - 1);
	}

	RedrawMap();
}


void CMD_Insert(Document &doc)
{
	bool force_cont;
	bool no_fill;

	if (edit.render3d && edit.mode != ObjType::things)
	{
		Beep("Cannot insert in this mode");
		return;
	}

	switch (edit.mode)
	{
		case ObjType::things:
			Insert_Thing();
			break;

		case ObjType::vertices:
			force_cont = Exec_HasFlag("/continue");
			no_fill    = Exec_HasFlag("/nofill");
			Insert_Vertex(force_cont, no_fill);
			break;

		case ObjType::sectors:
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
bool LineTouchesBox(int ld, double x0, double y0, double x1, double y1)
{
	double lx0 = gDocument.linedefs[ld]->Start(gDocument)->x();
	double ly0 = gDocument.linedefs[ld]->Start(gDocument)->y();
	double lx1 = gDocument.linedefs[ld]->End(gDocument)->x();
	double ly1 = gDocument.linedefs[ld]->End(gDocument)->y();

	double i;

	// start is entirely inside the square?
	if (lx0 >= x0 && lx0 <= x1 && ly0 >= y0 && ly0 <= y1)
		return true;

	// end is entirely inside the square?
	if (lx1 >= x0 && lx1 <= x1 && ly1 >= y0 && ly1 <= y1)
		return true;


	if ((ly0 > y0) != (ly1 > y0))
	{
		i = lx0 + (y0 - ly0) * (lx1 - lx0) / (ly1 - ly0);
		if (i >= x0 && i <= x1)
			return true; /* the linedef crosses the left side */
	}
	if ((ly0 > y1) != (ly1 > y1))
	{
		i = lx0 + (y1 - ly0) * (lx1 - lx0) / (ly1 - ly0);
		if (i >= x0 && i <= x1)
			return true; /* the linedef crosses the right side */
	}
	if ((lx0 > x0) != (lx1 > x0))
	{
		i = ly0 + (x0 - lx0) * (ly1 - ly0) / (lx1 - lx0);
		if (i >= y0 && i <= y1)
			return true; /* the linedef crosses the bottom side */
	}
	if ((lx0 > x1) != (lx1 > x1))
	{
		i = ly0 + (x1 - lx0) * (ly1 - ly0) / (lx1 - lx0);
		if (i >= y0 && i <= y1)
			return true; /* the linedef crosses the top side */
	}

	return false;
}


static void DoMoveObjects(selection_c *list, double delta_x, double delta_y, double delta_z)
{
	fixcoord_t fdx = MakeValidCoord(delta_x);
	fixcoord_t fdy = MakeValidCoord(delta_y);
	fixcoord_t fdz = MakeValidCoord(delta_z);

	switch (list->what_type())
	{
		case ObjType::things:
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const Thing * T = gDocument.things[*it];

				gDocument.basis.changeThing(*it, Thing::F_X, T->raw_x + fdx);
				gDocument.basis.changeThing(*it, Thing::F_Y, T->raw_y + fdy);
				gDocument.basis.changeThing(*it, Thing::F_H, MAX(0, T->raw_h + fdz));
			}
			break;

		case ObjType::vertices:
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const Vertex * V = gDocument.vertices[*it];

				gDocument.basis.changeVertex(*it, Vertex::F_X, V->raw_x + fdx);
				gDocument.basis.changeVertex(*it, Vertex::F_Y, V->raw_y + fdy);
			}
			break;

		case ObjType::sectors:
			// apply the Z delta first
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const Sector * S = gDocument.sectors[*it];

				gDocument.basis.changeSector(*it, Sector::F_FLOORH, S->floorh + (int)delta_z);
				gDocument.basis.changeSector(*it, Sector::F_CEILH,  S->ceilh  + (int)delta_z);
			}

			/* FALL-THROUGH !! */

		case ObjType::linedefs:
			{
				selection_c verts(ObjType::vertices);
				ConvertSelection(list, &verts);

				DoMoveObjects(&verts, delta_x, delta_y, delta_z);
			}
			break;

		default:
			break;
	}
}


void MoveObjects(selection_c *list, double delta_x, double delta_y, double delta_z)
{
	if (list->empty())
		return;

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection("moved", *list);

	// move things in sectors too (must do it _before_ moving the
	// sectors, otherwise we fail trying to determine which sectors
	// each thing is in).
	if (edit.mode == ObjType::sectors)
	{
		selection_c thing_sel(ObjType::things);
		ConvertSelection(list, &thing_sel);

		DoMoveObjects(&thing_sel, delta_x, delta_y, 0);
	}

	DoMoveObjects(list, delta_x, delta_y, delta_z);

	gDocument.basis.end();
}


void DragSingleObject(Objid& obj, double delta_x, double delta_y, double delta_z)
{
	if (edit.mode != ObjType::vertices)
	{
		selection_c list(edit.mode);
		list.set(obj.num);

		MoveObjects(&list, delta_x, delta_y, delta_z);
		return;
	}

	/* move a single vertex */

	gDocument.basis.begin();

	int did_split_line = -1;

	// handle a single vertex merging onto an existing one
	if (edit.highlight.valid())
	{
		gDocument.basis.setMessage("merge vertex #%d", obj.num);

		SYS_ASSERT(obj.num != edit.highlight.num);

		selection_c verts(ObjType::vertices);

		verts.set(edit.highlight.num);	// keep the highlight
		verts.set(obj.num);

		gDocument.vertmod.mergeList(&verts);

		gDocument.basis.end();
		return;
	}

	// handle a single vertex splitting a linedef
	if (edit.split_line.valid())
	{
		did_split_line = edit.split_line.num;

		gDocument.linemod.splitLinedefAtVertex(edit.split_line.num, obj.num);

		// now move the vertex!
	}

	selection_c list(edit.mode);

	list.set(obj.num);

	DoMoveObjects(&list, delta_x, delta_y, delta_z);

	if (did_split_line >= 0)
		gDocument.basis.setMessage("split linedef #%d", did_split_line);
	else
		gDocument.basis.setMessageForSelection("moved", list);

	gDocument.basis.end();
}


static void TransferThingProperties(int src_thing, int dest_thing)
{
	const Thing * T = gDocument.things[src_thing];

	gDocument.basis.changeThing(dest_thing, Thing::F_TYPE,    T->type);
	gDocument.basis.changeThing(dest_thing, Thing::F_OPTIONS, T->options);
//	BA_ChangeTH(dest_thing, Thing::F_ANGLE,   T->angle);

	gDocument.basis.changeThing(dest_thing, Thing::F_TID,     T->tid);
	gDocument.basis.changeThing(dest_thing, Thing::F_SPECIAL, T->special);

	gDocument.basis.changeThing(dest_thing, Thing::F_ARG1, T->arg1);
	gDocument.basis.changeThing(dest_thing, Thing::F_ARG2, T->arg2);
	gDocument.basis.changeThing(dest_thing, Thing::F_ARG3, T->arg3);
	gDocument.basis.changeThing(dest_thing, Thing::F_ARG4, T->arg4);
	gDocument.basis.changeThing(dest_thing, Thing::F_ARG5, T->arg5);
}


static void TransferSectorProperties(int src_sec, int dest_sec)
{
	const Sector * sector = gDocument.sectors[src_sec];

	gDocument.basis.changeSector(dest_sec, Sector::F_FLOORH,    sector->floorh);
	gDocument.basis.changeSector(dest_sec, Sector::F_FLOOR_TEX, sector->floor_tex);
	gDocument.basis.changeSector(dest_sec, Sector::F_CEILH,     sector->ceilh);
	gDocument.basis.changeSector(dest_sec, Sector::F_CEIL_TEX,  sector->ceil_tex);

	gDocument.basis.changeSector(dest_sec, Sector::F_LIGHT,  sector->light);
	gDocument.basis.changeSector(dest_sec, Sector::F_TYPE,   sector->type);
	gDocument.basis.changeSector(dest_sec, Sector::F_TAG,    sector->tag);
}


#define LINEDEF_FLAG_KEEP  (MLF_Blocking + MLF_TwoSided)

static void TransferLinedefProperties(int src_line, int dest_line, bool do_tex)
{
	const LineDef * L1 = gDocument.linedefs[src_line];
	const LineDef * L2 = gDocument.linedefs[dest_line];

	// don't transfer certain flags
	int flags = gDocument.linedefs[dest_line]->flags;
	flags = (flags & LINEDEF_FLAG_KEEP) | (L1->flags & ~LINEDEF_FLAG_KEEP);

	// handle textures
	if (do_tex && L1->Right(gDocument) && L2->Right(gDocument))
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
		if (! L1->Left(gDocument))
		{
			int tex = L1->Right(gDocument)->mid_tex;

			if (! L2->Left(gDocument))
			{
				gDocument.basis.changeSidedef(L2->right, SideDef::F_MID_TEX, tex);
			}
			else
			{
				gDocument.basis.changeSidedef(L2->right, SideDef::F_LOWER_TEX, tex);
				gDocument.basis.changeSidedef(L2->right, SideDef::F_UPPER_TEX, tex);

				gDocument.basis.changeSidedef(L2->left,  SideDef::F_LOWER_TEX, tex);
				gDocument.basis.changeSidedef(L2->left,  SideDef::F_UPPER_TEX, tex);

				// this is debatable....   CONFIG ITEM?
				flags |= MLF_LowerUnpegged;
				flags |= MLF_UpperUnpegged;
			}
		}
		else if (! L2->Left(gDocument))
		{
			/* pick which texture to copy */

			const Sector *front = L1->Right(gDocument)->SecRef(gDocument);
			const Sector *back  = L1-> Left(gDocument)->SecRef(gDocument);

			int f_l = L1->Right(gDocument)->lower_tex;
			int f_u = L1->Right(gDocument)->upper_tex;
			int b_l = L1-> Left(gDocument)->lower_tex;
			int b_u = L1-> Left(gDocument)->upper_tex;

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
				gDocument.basis.changeSidedef(L2->right, SideDef::F_MID_TEX, tex);
			}
		}
		else
		{
			const SideDef *RS = L1->Right(gDocument);
			const SideDef *LS = L1->Left(gDocument);

			const Sector *F1 = L1->Right(gDocument)->SecRef(gDocument);
			const Sector *B1 = L1-> Left(gDocument)->SecRef(gDocument);
			const Sector *F2 = L2->Right(gDocument)->SecRef(gDocument);
			const Sector *B2 = L2-> Left(gDocument)->SecRef(gDocument);

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

			gDocument.basis.changeSidedef(L2->right, SideDef::F_LOWER_TEX, RS->lower_tex);
			gDocument.basis.changeSidedef(L2->right, SideDef::F_MID_TEX,   RS->mid_tex);
			gDocument.basis.changeSidedef(L2->right, SideDef::F_UPPER_TEX, RS->upper_tex);

			gDocument.basis.changeSidedef(L2->left, SideDef::F_LOWER_TEX, LS->lower_tex);
			gDocument.basis.changeSidedef(L2->left, SideDef::F_MID_TEX,   LS->mid_tex);
			gDocument.basis.changeSidedef(L2->left, SideDef::F_UPPER_TEX, LS->upper_tex);
		}
	}

	gDocument.basis.changeLinedef(dest_line, LineDef::F_FLAGS, flags);

	gDocument.basis.changeLinedef(dest_line, LineDef::F_TYPE, L1->type);
	gDocument.basis.changeLinedef(dest_line, LineDef::F_TAG,  L1->tag);

	gDocument.basis.changeLinedef(dest_line, LineDef::F_ARG2, L1->arg2);
	gDocument.basis.changeLinedef(dest_line, LineDef::F_ARG3, L1->arg3);
	gDocument.basis.changeLinedef(dest_line, LineDef::F_ARG4, L1->arg4);
	gDocument.basis.changeLinedef(dest_line, LineDef::F_ARG5, L1->arg5);
}


void CMD_CopyProperties(Document &doc)
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
	else if (edit.mode == ObjType::vertices)
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

		doc.basis.begin();
		doc.basis.setMessage("copied properties");

		switch (edit.mode)
		{
			case ObjType::sectors:
				TransferSectorProperties(source, target);
				break;

			case ObjType::things:
				TransferThingProperties(source, target);
				break;

			case ObjType::linedefs:
				TransferLinedefProperties(source, target, true /* do_tex */);
				break;

			default: break;
		}

		doc.basis.end();

	}
	else  /* reverse mode, HILITE --> SEL */
	{
		if (edit.Selected->count_obj() == 1 && edit.Selected->find_first() == edit.highlight.num)
		{
			Beep("No selection for CopyProperties");
			return;
		}

		int source = edit.highlight.num;

		doc.basis.begin();
		doc.basis.setMessage("copied properties");

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			if (*it == source)
				continue;

			switch (edit.mode)
			{
				case ObjType::sectors:
					TransferSectorProperties(source, *it);
					break;

				case ObjType::things:
					TransferThingProperties(source, *it);
					break;

				case ObjType::linedefs:
					TransferLinedefProperties(source, *it, true /* do_tex */);
					break;

				default: break;
			}
		}

		doc.basis.end();
	}
}


static void Drag_CountOnGrid_Worker(ObjType obj_type, int objnum, int *count, int *total)
{
	switch (obj_type)
	{
		case ObjType::things:
			*total += 1;
			if (grid.OnGrid(gDocument.things[objnum]->x(), gDocument.things[objnum]->y()))
				*count += 1;
			break;

		case ObjType::vertices:
			*total += 1;
			if (grid.OnGrid(gDocument.vertices[objnum]->x(), gDocument.vertices[objnum]->y()))
				*count += 1;
			break;

		case ObjType::linedefs:
			Drag_CountOnGrid_Worker(ObjType::vertices, gDocument.linedefs[objnum]->start, count, total);
			Drag_CountOnGrid_Worker(ObjType::vertices, gDocument.linedefs[objnum]->end,   count, total);
			break;

		case ObjType::sectors:
			for (int n = 0 ; n < gDocument.numLinedefs(); n++)
			{
				LineDef *L = gDocument.linedefs[n];

				if (! L->TouchesSector(objnum, gDocument))
					continue;

				Drag_CountOnGrid_Worker(ObjType::linedefs, n, count, total);
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

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		Drag_CountOnGrid_Worker(edit.mode, *it, count, total);
	}
}


static void Drag_UpdateCurrentDist(ObjType obj_type, int objnum, double *x, double *y,
								   double *best_dist, double ptr_x, double ptr_y,
								   bool only_grid)
{
	double x2, y2;

	switch (obj_type)
	{
		case ObjType::things:
			x2 = gDocument.things[objnum]->x();
			y2 = gDocument.things[objnum]->y();
			break;

		case ObjType::vertices:
			x2 = gDocument.vertices[objnum]->x();
			y2 = gDocument.vertices[objnum]->y();
			break;

		case ObjType::linedefs:
			{
				LineDef *L = gDocument.linedefs[objnum];

				Drag_UpdateCurrentDist(ObjType::vertices, L->start, x, y, best_dist,
									   ptr_x, ptr_y, only_grid);

				Drag_UpdateCurrentDist(ObjType::vertices, L->end,   x, y, best_dist,
				                       ptr_x, ptr_y, only_grid);
			}
			return;

		case ObjType::sectors:
			// recursively handle all vertices belonging to the sector
			// (some vertices can be processed two or more times, that
			// won't matter though).

			for (int n = 0 ; n < gDocument.numLinedefs(); n++)
			{
				LineDef *L = gDocument.linedefs[n];

				if (! L->TouchesSector(objnum, gDocument))
					continue;

				Drag_UpdateCurrentDist(ObjType::linedefs, n, x, y, best_dist,
				                       ptr_x, ptr_y, only_grid);
			}
			return;

		default:
			return;
	}

	// handle OBJ_THINGS and OBJ_VERTICES

	if (only_grid && !grid.OnGrid(x2, y2))
		return;

	double dist = hypot(x2 - ptr_x, y2 - ptr_y);

	if (dist < *best_dist)
	{
		*x = x2;
		*y = y2;
		*best_dist = dist;
	}
}


//
// Determine the focus coordinate for dragging multiple objects.
// The focus only has an effect when grid snapping is on, and
// allows a mostly-grid-snapped set of objects to stay snapped
// to the grid.
//
void GetDragFocus(double *x, double *y, double ptr_x, double ptr_y)
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

	// determine object which is closest to mouse pointer AND which
	// honors the 'only_grid' property (when set).
	double best_dist = 9e9;

	if (edit.dragged.valid())  // a single object
	{
		Drag_UpdateCurrentDist(edit.mode, edit.dragged.num, x, y, &best_dist,
							   ptr_x, ptr_y, only_grid);
		return;
	}

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		Drag_UpdateCurrentDist(edit.mode, *it, x, y, &best_dist,
							   ptr_x, ptr_y, only_grid);
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


void transform_t::Apply(double *x, double *y) const
{
	double x0 = *x - mid_x;
	double y0 = *y - mid_y;

	if (rotate)
	{
		double s = sin(rotate);
		double c = cos(rotate);

		double x1 = x0;
		double y1 = y0;

		x0 = x1 * c - y1 * s;
		y0 = y1 * c + x1 * s;
	}

	if (skew_x || skew_y)
	{
		double x1 = x0;
		double y1 = y0;

		x0 = x1 + y1 * skew_x;
		y0 = y1 + x1 * skew_y;
	}

	*x = mid_x + x0 * scale_x;
	*y = mid_y + y0 * scale_y;
}


//
// Return the coordinate of the centre of a group of objects.
//
// This is computed using an average of all the coordinates, which can
// often give a different result than using the middle of the bounding
// box.
//
void Objs_CalcMiddle(selection_c * list, double *x, double *y)
{
	*x = *y = 0;

	if (list->empty())
		return;

	double sum_x = 0;
	double sum_y = 0;

	int count = 0;

	switch (list->what_type())
	{
		case ObjType::things:
		{
			for (sel_iter_c it(list) ; !it.done() ; it.next(), ++count)
			{
				sum_x += gDocument.things[*it]->x();
				sum_y += gDocument.things[*it]->y();
			}
			break;
		}

		case ObjType::vertices:
		{
			for (sel_iter_c it(list) ; !it.done() ; it.next(), ++count)
			{
				sum_x += gDocument.vertices[*it]->x();
				sum_y += gDocument.vertices[*it]->y();
			}
			break;
		}

		// everything else: just use the vertices
		default:
		{
			selection_c verts(ObjType::vertices);
			ConvertSelection(list, &verts);

			Objs_CalcMiddle(&verts, x, y);
			return;
		}
	}

	SYS_ASSERT(count > 0);

	*x = sum_x / count;
	*y = sum_y / count;
}


//
// returns a bounding box that completely includes a list of objects.
// when the list is empty, bottom-left coordinate is arbitrary.
//
void Objs_CalcBBox(selection_c * list, double *x1, double *y1, double *x2, double *y2)
{
	if (list->empty())
	{
		*x1 = *y1 = 0;
		*x2 = *y2 = 0;
		return;
	}

	*x1 = *y1 = +9e9;
	*x2 = *y2 = -9e9;

	switch (list->what_type())
	{
		case ObjType::things:
		{
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const Thing *T = gDocument.things[*it];
				double Tx = T->x();
				double Ty = T->y();

				const thingtype_t &info = M_GetThingType(T->type);
				int r = info.radius;

				if (Tx - r < *x1) *x1 = Tx - r;
				if (Ty - r < *y1) *y1 = Ty - r;
				if (Tx + r > *x2) *x2 = Tx + r;
				if (Ty + r > *y2) *y2 = Ty + r;
			}
			break;
		}

		case ObjType::vertices:
		{
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const Vertex *V = gDocument.vertices[*it];
				double Vx = V->x();
				double Vy = V->y();

				if (Vx < *x1) *x1 = Vx;
				if (Vy < *y1) *y1 = Vy;
				if (Vx > *x2) *x2 = Vx;
				if (Vy > *y2) *y2 = Vy;
			}
			break;
		}

		// everything else: just use the vertices
		default:
		{
			selection_c verts(ObjType::vertices);
			ConvertSelection(list, &verts);

			Objs_CalcBBox(&verts, x1, y1, x2, y2);
			return;
		}
	}

	SYS_ASSERT(*x1 <= *x2);
	SYS_ASSERT(*y1 <= *y2);
}


static void DoMirrorThings(selection_c *list, bool is_vert, double mid_x, double mid_y)
{
	fixcoord_t fix_mx = MakeValidCoord(mid_x);
	fixcoord_t fix_my = MakeValidCoord(mid_y);

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Thing * T = gDocument.things[*it];

		if (is_vert)
		{
			gDocument.basis.changeThing(*it, Thing::F_Y, 2*fix_my - T->raw_y);

			if (T->angle != 0)
				gDocument.basis.changeThing(*it, Thing::F_ANGLE, 360 - T->angle);
		}
		else
		{
			gDocument.basis.changeThing(*it, Thing::F_X, 2*fix_mx - T->raw_x);

			if (T->angle > 180)
				gDocument.basis.changeThing(*it, Thing::F_ANGLE, 540 - T->angle);
			else
				gDocument.basis.changeThing(*it, Thing::F_ANGLE, 180 - T->angle);
		}
	}
}


static void DoMirrorVertices(selection_c *list, bool is_vert, double mid_x, double mid_y)
{
	fixcoord_t fix_mx = MakeValidCoord(mid_x);
	fixcoord_t fix_my = MakeValidCoord(mid_y);

	selection_c verts(ObjType::vertices);
	ConvertSelection(list, &verts);

	for (sel_iter_c it(verts) ; !it.done() ; it.next())
	{
		const Vertex * V = gDocument.vertices[*it];

		if (is_vert)
			gDocument.basis.changeVertex(*it, Vertex::F_Y, 2*fix_my - V->raw_y);
		else
			gDocument.basis.changeVertex(*it, Vertex::F_X, 2*fix_mx - V->raw_x);
	}

	// flip linedefs too !!
	selection_c lines(ObjType::linedefs);
	ConvertSelection(&verts, &lines);

	for (sel_iter_c it(lines) ; !it.done() ; it.next())
	{
		LineDef * L = gDocument.linedefs[*it];

		int start = L->start;
		int end   = L->end;

		gDocument.basis.changeLinedef(*it, LineDef::F_START, end);
		gDocument.basis.changeLinedef(*it, LineDef::F_END, start);
	}
}


static void DoMirrorStuff(selection_c *list, bool is_vert, double mid_x, double mid_y)
{
	if (edit.mode == ObjType::things)
	{
		DoMirrorThings(list, is_vert, mid_x, mid_y);
		return;
	}

	// everything else just modifies the vertices

	if (edit.mode == ObjType::sectors)
	{
		// handle things in Sectors mode too
		selection_c things(ObjType::things);
		ConvertSelection(list, &things);

		DoMirrorThings(&things, is_vert, mid_x, mid_y);
	}

	DoMirrorVertices(list, is_vert, mid_x, mid_y);
}


void CMD_Mirror(Document &doc)
{
	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No objects to mirror");
		return;
	}

	bool is_vert = false;

	if (tolower(EXEC_Param[0][0]) == 'v')
		is_vert = true;

	double mid_x, mid_y;
	Objs_CalcMiddle(edit.Selected, &mid_x, &mid_y);

	doc.basis.begin();
	doc.basis.setMessageForSelection("mirrored", *edit.Selected, is_vert ? " vertically" : " horizontally");

	DoMirrorStuff(edit.Selected, is_vert, mid_x, mid_y);

	doc.basis.end();

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


static void DoRotate90Things(selection_c *list, bool anti_clockwise,
							 double mid_x, double mid_y)
{
	fixcoord_t fix_mx = MakeValidCoord(mid_x);
	fixcoord_t fix_my = MakeValidCoord(mid_y);

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Thing * T = gDocument.things[*it];

		fixcoord_t old_x = T->raw_x;
		fixcoord_t old_y = T->raw_y;

		if (anti_clockwise)
		{
			gDocument.basis.changeThing(*it, Thing::F_X, fix_mx - old_y + fix_my);
			gDocument.basis.changeThing(*it, Thing::F_Y, fix_my + old_x - fix_mx);

			gDocument.basis.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, +90));
		}
		else
		{
			gDocument.basis.changeThing(*it, Thing::F_X, fix_mx + old_y - fix_my);
			gDocument.basis.changeThing(*it, Thing::F_Y, fix_my - old_x + fix_mx);

			gDocument.basis.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, -90));
		}
	}
}


void CMD_Rotate90(Document &doc)
{
	if (EXEC_Param[0].empty())
	{
		Beep("Rotate90: missing keyword");
		return;
	}

	bool anti_clockwise = (tolower(EXEC_Param[0][0]) == 'a');

	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No objects to rotate");
		return;
	}

	double mid_x, mid_y;
	Objs_CalcMiddle(edit.Selected, &mid_x, &mid_y);

	doc.basis.begin();
	doc.basis.setMessageForSelection("rotated", *edit.Selected, anti_clockwise ? " anti-clockwise" : " clockwise");

	if (edit.mode == ObjType::things)
	{
		DoRotate90Things(edit.Selected, anti_clockwise, mid_x, mid_y);
	}
	else
	{
		// handle things inside sectors
		if (edit.mode == ObjType::sectors)
		{
			selection_c things(ObjType::things);
			ConvertSelection(edit.Selected, &things);

			DoRotate90Things(&things, anti_clockwise, mid_x, mid_y);
		}

		// everything else just rotates the vertices
		selection_c verts(ObjType::vertices);
		ConvertSelection(edit.Selected, &verts);

		fixcoord_t fix_mx = MakeValidCoord(mid_x);
		fixcoord_t fix_my = MakeValidCoord(mid_y);

		for (sel_iter_c it(verts) ; !it.done() ; it.next())
		{
			const Vertex * V = doc.vertices[*it];

			fixcoord_t old_x = V->raw_x;
			fixcoord_t old_y = V->raw_y;

			if (anti_clockwise)
			{
				doc.basis.changeVertex(*it, Vertex::F_X, fix_mx - old_y + fix_my);
				doc.basis.changeVertex(*it, Vertex::F_Y, fix_my + old_x - fix_mx);
			}
			else
			{
				doc.basis.changeVertex(*it, Vertex::F_X, fix_mx + old_y - fix_my);
				doc.basis.changeVertex(*it, Vertex::F_Y, fix_my - old_x + fix_mx);
			}
		}
	}

	doc.basis.end();

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


static void DoScaleTwoThings(selection_c *list, transform_t& param)
{
	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Thing * T = gDocument.things[*it];

		double new_x = T->x();
		double new_y = T->y();

		param.Apply(&new_x, &new_y);

		gDocument.basis.changeThing(*it, Thing::F_X, MakeValidCoord(new_x));
		gDocument.basis.changeThing(*it, Thing::F_Y, MakeValidCoord(new_y));

		float rot1 = static_cast<float>(param.rotate / (M_PI / 4));

		int ang_diff = static_cast<int>(I_ROUND(rot1) * 45.0);

		if (ang_diff)
		{
			gDocument.basis.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, ang_diff));
		}
	}
}


static void DoScaleTwoVertices(selection_c *list, transform_t& param)
{
	selection_c verts(ObjType::vertices);
	ConvertSelection(list, &verts);

	for (sel_iter_c it(verts) ; !it.done() ; it.next())
	{
		const Vertex * V = gDocument.vertices[*it];

		double new_x = V->x();
		double new_y = V->y();

		param.Apply(&new_x, &new_y);

		gDocument.basis.changeVertex(*it, Vertex::F_X, MakeValidCoord(new_x));
		gDocument.basis.changeVertex(*it, Vertex::F_Y, MakeValidCoord(new_y));
	}
}


static void DoScaleTwoStuff(selection_c *list, transform_t& param)
{
	if (edit.mode == ObjType::things)
	{
		DoScaleTwoThings(list, param);
		return;
	}

	// everything else just modifies the vertices

	if (edit.mode == ObjType::sectors)
	{
		// handle things in Sectors mode too
		selection_c things(ObjType::things);
		ConvertSelection(list, &things);

		DoScaleTwoThings(&things, param);
	}

	DoScaleTwoVertices(list, param);
}


void TransformObjects(transform_t& param)
{
	// this is called by the MOUSE2 dynamic scaling code

	SYS_ASSERT(edit.Selected->notempty());

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection("scaled", *edit.Selected);

	if (param.scale_x < 0)
	{
		param.scale_x = -param.scale_x;
		DoMirrorStuff(edit.Selected, false /* is_vert */, param.mid_x, param.mid_y);
	}

	if (param.scale_y < 0)
	{
		param.scale_y = -param.scale_y;
		DoMirrorStuff(edit.Selected, true /* is_vert */, param.mid_x, param.mid_y);
	}

	DoScaleTwoStuff(edit.Selected, param);

	gDocument.basis.end();
}


static void DetermineOrigin(transform_t& param, double pos_x, double pos_y)
{
	if (pos_x == 0 && pos_y == 0)
	{
		Objs_CalcMiddle(edit.Selected, &param.mid_x, &param.mid_y);
		return;
	}

	double lx, ly, hx, hy;

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


void ScaleObjects3(double scale_x, double scale_y, double pos_x, double pos_y)
{
	SYS_ASSERT(scale_x > 0);
	SYS_ASSERT(scale_y > 0);

	transform_t param;

	param.Clear();

	param.scale_x = scale_x;
	param.scale_y = scale_y;

	DetermineOrigin(param, pos_x, pos_y);

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection("scaled", *edit.Selected);
	{
		DoScaleTwoStuff(edit.Selected, param);
	}
	gDocument.basis.end();
}


static void DoScaleSectorHeights(selection_c *list, double scale_z, int pos_z)
{
	SYS_ASSERT(! list->empty());

	// determine Z range and origin
	int lz = +99999;
	int hz = -99999;

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Sector * S = gDocument.sectors[*it];

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

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Sector * S = gDocument.sectors[*it];

		int new_f = mid_z + I_ROUND((S->floorh - mid_z) * scale_z);
		int new_c = mid_z + I_ROUND((S-> ceilh - mid_z) * scale_z);

		gDocument.basis.changeSector(*it, Sector::F_FLOORH, new_f);
		gDocument.basis.changeSector(*it, Sector::F_CEILH,  new_c);
	}
}

void ScaleObjects4(double scale_x, double scale_y, double scale_z,
                   double pos_x, double pos_y, double pos_z)
{
	SYS_ASSERT(edit.mode == ObjType::sectors);

	transform_t param;

	param.Clear();

	param.scale_x = scale_x;
	param.scale_y = scale_y;

	DetermineOrigin(param, pos_x, pos_y);

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection("scaled", *edit.Selected);
	{
		DoScaleTwoStuff(edit.Selected, param);
		DoScaleSectorHeights(edit.Selected, scale_z, static_cast<int>(pos_z));
	}
	gDocument.basis.end();
}


void RotateObjects3(double deg, double pos_x, double pos_y)
{
	transform_t param;

	param.Clear();

	param.rotate = deg * M_PI / 180.0;

	DetermineOrigin(param, pos_x, pos_y);

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection("rotated", *edit.Selected);
	{
		DoScaleTwoStuff(edit.Selected, param);
	}
	gDocument.basis.end();
}


static bool SpotInUse(ObjType obj_type, int x, int y)
{
	switch (obj_type)
	{
		case ObjType::things:
			for (const Thing *thing : gDocument.things)
				if (I_ROUND(thing->x()) == x && I_ROUND(thing->y()) == y)
					return true;
			return false;

		case ObjType::vertices:
			for (const Vertex *vertex : gDocument.vertices)
				if (I_ROUND(vertex->x()) == x && I_ROUND(vertex->y()) == y)
					return true;
			return false;

		default:
			BugError("IsSpotVacant: bad object type\n");
			return false;
	}
}


static void DoEnlargeOrShrink(bool do_shrink)
{
	// setup transform parameters...
	float mul = 2.0;

	if (EXEC_Param[0].good())
	{
		mul = static_cast<float>(atof(EXEC_Param[0]));

		if (mul < 0.02 || mul > 50)
		{
			Beep("bad factor: %s", EXEC_Param[0].c_str());
			return;
		}
	}

	if (do_shrink)
		mul = 1.0f / mul;

	transform_t param;

	param.Clear();

	param.scale_x = mul;
	param.scale_y = mul;


	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No objects to %s", do_shrink ? "shrink" : "enlarge");
		return;
	}

	// TODO: CONFIG ITEM (or FLAG)
	if ((true))
	{
		Objs_CalcMiddle(edit.Selected, &param.mid_x, &param.mid_y);
	}
	else
	{
		double lx, ly, hx, hy;
		Objs_CalcBBox(edit.Selected, &lx, &ly, &hx, &hy);

		param.mid_x = lx + (hx - lx) / 2;
		param.mid_y = ly + (hy - ly) / 2;
	}

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection(do_shrink ? "shrunk" : "enlarged", *edit.Selected);

	DoScaleTwoStuff(edit.Selected, param);

	gDocument.basis.end();

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void CMD_Enlarge(Document &doc)
{
	DoEnlargeOrShrink(false /* do_shrink */);
}

void CMD_Shrink(Document &doc)
{
	DoEnlargeOrShrink(true /* do_shrink */);
}


static void Quantize_Things(selection_c *list)
{
	// remember the things which we moved
	// (since we cannot modify the selection while we iterate over it)
	selection_c moved(list->what_type());

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Thing * T = gDocument.things[*it];

		if (grid.OnGrid(T->x(), T->y()))
		{
			moved.set(*it);
			continue;
		}

		for (int pass = 0 ; pass < 4 ; pass++)
		{
			int new_x = grid.QuantSnapX(T->x(), pass & 1);
			int new_y = grid.QuantSnapY(T->y(), pass & 2);

			if (! SpotInUse(ObjType::things, new_x, new_y))
			{
				gDocument.basis.changeThing(*it, Thing::F_X, MakeValidCoord(new_x));
				gDocument.basis.changeThing(*it, Thing::F_Y, MakeValidCoord(new_y));

				moved.set(*it);
				break;
			}
		}
	}

	list->unmerge(moved);

	if (! list->empty())
		Beep("Quantize: could not move %d things", list->count_obj());
}


static void Quantize_Vertices(selection_c *list)
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

	byte * vert_modes = new byte[gDocument.numVertices()];

	for (const LineDef *L : gDocument.linedefs)
	{
		// require both vertices of the linedef to be in the selection
		if (! (list->get(L->start) && list->get(L->end)))
			continue;

		// IDEA: make this a method of LineDef
		double x1 = L->Start(gDocument)->x();
		double y1 = L->Start(gDocument)->y();
		double x2 = L->End(gDocument)->x();
		double y2 = L->End(gDocument)->y();

		if (L->IsHorizontal(gDocument))
		{
			vert_modes[L->start] |= V_HORIZ;
			vert_modes[L->end]   |= V_HORIZ;
		}
		else if (L->IsVertical(gDocument))
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
	selection_c moved(list->what_type());

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Vertex * V = gDocument.vertices[*it];

		if (grid.OnGrid(V->x(), V->y()))
		{
			moved.set(*it);
			continue;
		}

		byte mode = vert_modes[*it];

		for (int pass = 0 ; pass < 4 ; pass++)
		{
			int x_dir, y_dir;

			double new_x = grid.QuantSnapX(V->x(), pass & 1, &x_dir);
			double new_y = grid.QuantSnapY(V->y(), pass & 2, &y_dir);

			// keep horizontal lines horizontal
			if ((mode & V_HORIZ) && (pass & 2))
				continue;

			// keep vertical lines vertical
			if ((mode & V_VERT) && (pass & 1))
				continue;

			// TODO: keep diagonal lines diagonal...

			if (! SpotInUse(ObjType::vertices, static_cast<int>(new_x), static_cast<int>(new_y)))
			{
				gDocument.basis.changeVertex(*it, Vertex::F_X, MakeValidCoord(new_x));
				gDocument.basis.changeVertex(*it, Vertex::F_Y, MakeValidCoord(new_y));

				moved.set(*it);
				break;
			}
		}
	}

	delete[] vert_modes;

	list->unmerge(moved);

	if (list->notempty())
		Beep("Quantize: could not move %d vertices", list->count_obj());
}


void CMD_Quantize(Document &doc)
{
	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("Nothing to quantize");
			return;
		}

		Selection_Add(edit.highlight);
	}

	doc.basis.begin();
	doc.basis.setMessageForSelection("quantized", *edit.Selected);

	switch (edit.mode)
	{
		case ObjType::things:
			Quantize_Things(edit.Selected);
			break;

		case ObjType::vertices:
			Quantize_Vertices(edit.Selected);
			break;

		// everything else merely quantizes vertices
		default:
		{
			selection_c verts(ObjType::vertices);
			ConvertSelection(edit.Selected, &verts);

			Quantize_Vertices(&verts);

			Selection_Clear();
			break;
		}
	}

	doc.basis.end();

	edit.error_mode = true;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
