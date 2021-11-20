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


//
//  delete a group of objects.
//  this is very raw, e.g. it does not check for stuff that will
//  remain unused afterwards.
//
void ObjectsModule::del(selection_c *list) const
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
		doc.basis.del(list->what_type(), objnums[i]);
}


void ObjectsModule::createSquare(int model) const
{
	int new_sec = doc.basis.addNew(ObjType::sectors);

	if (model >= 0)
		*doc.sectors[new_sec] = *doc.sectors[model];
	else
		doc.sectors[new_sec]->SetDefaults(inst);

	double x1 = inst.grid.QuantSnapX(inst.edit.map_x, false);
	double y1 = inst.grid.QuantSnapX(inst.edit.map_y, false);

	double x2 = x1 + config::new_sector_size;
	double y2 = y1 + config::new_sector_size;

	for (int i = 0 ; i < 4 ; i++)
	{
		int new_v = doc.basis.addNew(ObjType::vertices);
		Vertex *V = doc.vertices[new_v];

		V->SetRawX(inst, (i >= 2) ? x2 : x1);
		V->SetRawY(inst, (i==1 || i==2) ? y2 : y1);

		int new_sd = doc.basis.addNew(ObjType::sidedefs);

		doc.sidedefs[new_sd]->SetDefaults(inst, false);
		doc.sidedefs[new_sd]->sector = new_sec;

		int new_ld = doc.basis.addNew(ObjType::linedefs);

		LineDef * L = doc.linedefs[new_ld];

		L->start = new_v;
		L->end   = (i == 3) ? (new_v - 3) : new_v + 1;
		L->flags = MLF_Blocking;
		L->right = new_sd;
	}

	// select it
	inst.Selection_Clear();

	inst.edit.Selected->set(new_sec);
}


void ObjectsModule::insertThing() const
{
	int model = -1;

	if (inst.edit.Selected->notempty())
		model = inst.edit.Selected->find_first();


	doc.basis.begin();

	int new_t = doc.basis.addNew(ObjType::things);
	Thing *T = doc.things[new_t];

	if(model >= 0)
		*T = *doc.things[model];
	else
	{
		T->type = inst.conf.default_thing;
		T->options = MTF_Easy | MTF_Medium | MTF_Hard;

		if (inst.loaded.levelFormat != MapFormat::doom)
		{
			T->options |= MTF_Hexen_SP | MTF_Hexen_COOP | MTF_Hexen_DM;
			T->options |= MTF_Hexen_Fighter | MTF_Hexen_Cleric | MTF_Hexen_Mage;
		}
	}

	T->SetRawX(inst, inst.grid.SnapX(inst.edit.map_x));
	T->SetRawY(inst, inst.grid.SnapY(inst.edit.map_y));

	inst.recent_things.insert_number(T->type);

	doc.basis.setMessage("added thing #%d", new_t);
	doc.basis.end();


	// select it
	inst.Selection_Clear();

	inst.edit.Selected->set(new_t);
}


int ObjectsModule::sectorNew(int model, int model2, int model3) const
{
	int new_sec = doc.basis.addNew(ObjType::sectors);

	if (model < 0) model = model2;
	if (model < 0) model = model3;

	if (model < 0)
		doc.sectors[new_sec]->SetDefaults(inst);
	else
		*doc.sectors[new_sec] = *doc.sectors[model];

	return new_sec;
}


bool ObjectsModule::checkClosedLoop(int new_ld, int v1, int v2, selection_c *flip) const
{
	// returns true if we assigned a sector (so drawing should stop)

	struct check_closed_data_t
	{
		lineloop_c loop;

		bool ok;

		// what sector we face, -1 for VOID
		int sec;

		double length;

	} left{ lineloop_c(doc) }, right{ lineloop_c(doc) };

	// trace the loops on either side of the new line

	 left.ok = doc.secmod.traceLineLoop(new_ld, Side::left,   left.loop);
	right.ok = doc.secmod.traceLineLoop(new_ld, Side::right, right.loop);

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
		int new_sec = sectorNew(innie.NeighboringSector(), -1, -1);

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
		int new_sec = sectorNew(left.sec, right.sec, left.loop.NeighboringSector());

		if (right.sec >= 0)
			right.loop.AssignSector(right.sec, flip);

		left.loop.AssignSector(new_sec, flip);
	}
	else
	{
		int new_sec = sectorNew(right.sec, left.sec, right.loop.NeighboringSector());

		right.loop.AssignSector(new_sec, flip);

		if (left.sec >= 0)
			left.loop.AssignSector(left.sec, flip);
	}

	return true;
}


void ObjectsModule::insertLinedef(int v1, int v2, bool no_fill) const
{
	if (doc.linemod.linedefAlreadyExists(v1, v2))
		return;

	int new_ld = doc.basis.addNew(ObjType::linedefs);

	LineDef * L = doc.linedefs[new_ld];

	L->start = v1;
	L->end   = v2;
	L->flags = MLF_Blocking;

	if (no_fill)
		return;

	if (doc.vertmod.howManyLinedefs(v1) >= 2 &&
		doc.vertmod.howManyLinedefs(v2) >= 2)
	{
		selection_c flip(ObjType::linedefs);

		checkClosedLoop(new_ld, v1, v2, &flip);

		doc.linemod.flipLinedefGroup(&flip);
	}
}


void ObjectsModule::insertLinedefAutosplit(int v1, int v2, bool no_fill) const
{
	// Find a linedef which this new line would cross, and if it exists
	// add a vertex there and create TWO lines.  Also handle a vertex
	// that this line crosses (sits on) similarly.

///  fprintf(stderr, "Insert_LineDef_autosplit %d..%d\n", v1, v2);

	crossing_state_c cross(inst);

	doc.hover.findCrossingPoints(cross,
		doc.vertices[v1]->x(), doc.vertices[v1]->y(), v1,
		doc.vertices[v2]->x(), doc.vertices[v2]->y(), v2);

	cross.SplitAllLines();

	int cur_v = v1;

	for (unsigned int k = 0 ; k < cross.points.size() ; k++)
	{
		int next_v = cross.points[k].vert;

		SYS_ASSERT(next_v != v1);
		SYS_ASSERT(next_v != v2);

		insertLinedef(cur_v, next_v, no_fill);

		cur_v = next_v;
	}

	insertLinedef(cur_v, v2, no_fill);
}


void ObjectsModule::insertVertex(bool force_continue, bool no_fill) const
{
	bool closed_a_loop = false;

	// when these both >= 0, we will add a linedef between them
	int old_vert = -1;
	int new_vert = -1;

	double new_x = inst.grid.SnapX(inst.edit.map_x);
	double new_y = inst.grid.SnapY(inst.edit.map_y);

	int orig_num_sectors = doc.numSectors();


	// are we drawing a line?
	if (inst.edit.action == ACT_DRAW_LINE)
	{
		old_vert = inst.edit.draw_from.num;

		new_x = inst.edit.draw_to_x;
		new_y = inst.edit.draw_to_y;
	}

	// a linedef which we are splitting (usually none)
	int split_ld = inst.edit.split_line.valid() ? inst.edit.split_line.num : -1;

	if (split_ld >= 0)
	{
		new_x = inst.edit.split_x;
		new_y = inst.edit.split_y;

		// prevent creating an overlapping line when splitting
		if (old_vert >= 0 &&
			doc.linedefs[split_ld]->TouchesVertex(old_vert))
		{
			old_vert = -1;
		}
	}
	else
	{
		// not splitting a line.
		// check if there is a "nearby" vertex (e.g. the highlighted one)

		if (inst.edit.highlight.valid())
			new_vert = inst.edit.highlight.num;

		// if no highlight, look for a vertex at snapped coord
		if (new_vert < 0 && inst.grid.snap && ! (inst.edit.action == ACT_DRAW_LINE))
			new_vert = doc.vertmod.findExact(TO_COORD(new_x), TO_COORD(new_y));

		//
		// handle a highlighted/snapped vertex.
		// either start drawing from it, or finish a loop at it.
		//
		if (new_vert >= 0)
		{
			// just ignore when highlight is same as drawing-start
			if (old_vert >= 0 && *doc.vertices[old_vert] == *doc.vertices[new_vert])
			{
				inst.edit.Selected->set(old_vert);
				return;
			}

			// a plain INSERT will attempt to fix a dangling vertex
			if (inst.edit.action == ACT_NOTHING)
			{
				if (doc.vertmod.tryFixDangler(new_vert))
				{
					// a vertex was deleted, selection/highlight is now invalid
					return;
				}
			}

			// our insertion point is an existing vertex, and we are not
			// in drawing mode, so there is no inst.edit operation to perform.
			if (old_vert < 0)
			{
				old_vert = new_vert;
				new_vert = -1;

				goto begin_drawing;
			}

			// handle case where a line already exists between the two vertices
			if (doc.linemod.linedefAlreadyExists(old_vert, new_vert))
			{
				// just continue drawing from the second vertex
				inst.edit.draw_from = Objid(ObjType::vertices, new_vert);
				inst.edit.Selected->set(new_vert);
				return;
			}
		}
	}

	// at here: if new_vert >= 0, then old_vert >= 0 and split_ld < 0


	// would we create a new vertex on top of an existing one?
	if (new_vert < 0 && old_vert >= 0 &&
		doc.vertices[old_vert]->Matches(inst.MakeValidCoord(new_x), inst.MakeValidCoord(new_y)))
	{
		inst.edit.Selected->set(old_vert);
		return;
	}


	doc.basis.begin();


	if (new_vert < 0)
	{
		new_vert = doc.basis.addNew(ObjType::vertices);

		Vertex *V = doc.vertices[new_vert];

		V->SetRawXY(inst, new_x, new_y);

		inst.edit.draw_from = Objid(ObjType::vertices, new_vert);
		inst.edit.Selected->set(new_vert);

		// splitting an existing line?
		if (split_ld >= 0)
		{
			doc.linemod.splitLinedefAtVertex(split_ld, new_vert);
			doc.basis.setMessage("split linedef #%d", split_ld);
		}
		else
		{
			doc.basis.setMessage("added vertex #%d", new_vert);
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
		if (!force_continue && doc.vertmod.howManyLinedefs(new_vert) > 0)
		{
			closed_a_loop = true;
		}

		//
		// adding a linedef
		//
		SYS_ASSERT(old_vert != new_vert);

		// this can make new sectors too
		insertLinedefAutosplit(old_vert, new_vert, no_fill);

		doc.basis.setMessage("added linedef");

		inst.edit.draw_from = Objid(ObjType::vertices, new_vert);
		inst.edit.Selected->set(new_vert);
	}


	doc.basis.end();


begin_drawing:
	// begin drawing mode?
	if (inst.edit.action == ACT_NOTHING && !closed_a_loop &&
		old_vert >= 0 && new_vert < 0)
	{
		inst.Selection_Clear();

		inst.edit.draw_from = Objid(ObjType::vertices, old_vert);
		inst.edit.Selected->set(old_vert);

		inst.edit.draw_to_x = doc.vertices[old_vert]->x();
		inst.edit.draw_to_y = doc.vertices[old_vert]->y();

		inst.Editor_SetAction(ACT_DRAW_LINE);
	}

	// stop drawing mode?
	if (closed_a_loop && !force_continue)
	{
		inst.Editor_ClearAction();
	}

	// select vertices of a newly created sector?
	if (closed_a_loop &&
		doc.numSectors() > orig_num_sectors)
	{
		selection_c sel(ObjType::sectors);

		// more than one sector may have been created, pick the last
		sel.set(doc.numSectors() - 1);

		inst.edit.Selected->change_type(inst.edit.mode);
		ConvertSelection(doc, &sel, inst.edit.Selected);
	}

	inst.RedrawMap();
}


void ObjectsModule::insertSector() const
{
	int sel_count = inst.edit.Selected->count_obj();
	if (sel_count > 1)
	{
		inst.Beep("Too many sectors to copy from");
		return;
	}

	// if outside of the map, create a square
	if (doc.hover.isPointOutsideOfMap(inst.edit.map_x, inst.edit.map_y))
	{
		doc.basis.begin();
		doc.basis.setMessage("added sector (outside map)");

		int model = -1;
		if (sel_count > 0)
			model = inst.edit.Selected->find_first();

		createSquare(model);

		doc.basis.end();
		return;
	}


	// --- adding a NEW sector to the area ---

	// determine a model sector to copy properties from
	int model;

	if (sel_count > 0)
		model = inst.edit.Selected->find_first();
	else if (inst.edit.highlight.valid())
		model = inst.edit.highlight.num;
	else
		model = -1;  // look for a neighbor to copy


	doc.basis.begin();
	doc.basis.setMessage("added new sector");

	bool ok = doc.secmod.assignSectorToSpace(inst.edit.map_x, inst.edit.map_y, -1 /* create */, model);

	doc.basis.end();

	// select the new sector
	if (ok)
	{
		inst.Selection_Clear();
		inst.edit.Selected->set(doc.numSectors() - 1);
	}

	inst.RedrawMap();
}


void Instance::CMD_ObjectInsert()
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
			level.objects.insertThing();
			break;

		case ObjType::vertices:
			force_cont = Exec_HasFlag("/continue");
			no_fill    = Exec_HasFlag("/nofill");
			level.objects.insertVertex(force_cont, no_fill);
			break;

		case ObjType::sectors:
			level.objects.insertSector();
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
bool ObjectsModule::lineTouchesBox(int ld, double x0, double y0, double x1, double y1) const
{
	double lx0 = doc.linedefs[ld]->Start(doc)->x();
	double ly0 = doc.linedefs[ld]->Start(doc)->y();
	double lx1 = doc.linedefs[ld]->End(doc)->x();
	double ly1 = doc.linedefs[ld]->End(doc)->y();

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


void ObjectsModule::doMoveObjects(selection_c *list, double delta_x, double delta_y, double delta_z) const
{
	fixcoord_t fdx = inst.MakeValidCoord(delta_x);
	fixcoord_t fdy = inst.MakeValidCoord(delta_y);
	fixcoord_t fdz = inst.MakeValidCoord(delta_z);

	switch (list->what_type())
	{
		case ObjType::things:
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const Thing * T = doc.things[*it];

				doc.basis.changeThing(*it, Thing::F_X, T->raw_x + fdx);
				doc.basis.changeThing(*it, Thing::F_Y, T->raw_y + fdy);
				doc.basis.changeThing(*it, Thing::F_H, std::max(0, T->raw_h + fdz));
			}
			break;

		case ObjType::vertices:
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const Vertex * V = doc.vertices[*it];

				doc.basis.changeVertex(*it, Vertex::F_X, V->raw_x + fdx);
				doc.basis.changeVertex(*it, Vertex::F_Y, V->raw_y + fdy);
			}
			break;

		case ObjType::sectors:
			// apply the Z delta first
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const Sector * S = doc.sectors[*it];

				doc.basis.changeSector(*it, Sector::F_FLOORH, S->floorh + (int)delta_z);
				doc.basis.changeSector(*it, Sector::F_CEILH,  S->ceilh  + (int)delta_z);
			}

			/* FALL-THROUGH !! */

		case ObjType::linedefs:
			{
				selection_c verts(ObjType::vertices);
				ConvertSelection(doc, list, &verts);

				doMoveObjects(&verts, delta_x, delta_y, delta_z);
			}
			break;

		default:
			break;
	}
}


void ObjectsModule::move(selection_c *list, double delta_x, double delta_y, double delta_z) const
{
	if (list->empty())
		return;

	doc.basis.begin();
	doc.basis.setMessageForSelection("moved", *list);

	// move things in sectors too (must do it _before_ moving the
	// sectors, otherwise we fail trying to determine which sectors
	// each thing is in).
	if (inst.edit.mode == ObjType::sectors)
	{
		selection_c thing_sel(ObjType::things);
		ConvertSelection(doc, list, &thing_sel);

		doMoveObjects(&thing_sel, delta_x, delta_y, 0);
	}

	doMoveObjects(list, delta_x, delta_y, delta_z);

	doc.basis.end();
}

//
// Returns, if found, a linedef ID between a given line and vertex, if existing.
// Only returns the first one found, if multiple available.
// Normally only 2 can exist, when there's a triangle between lineID and vertID.
// 
// Returns -1 if none found
//
int ObjectsModule::findLineBetweenLineAndVertex(int lineID, int vertID) const
{
	for(int i = 0; i < doc.numLinedefs(); ++i)
	{
		const LineDef *otherLine = doc.linedefs[i];
		if(!otherLine->TouchesVertex(vertID) || i == lineID)
			continue;

		// We have a linedef that is going to overlap the other one to be 
		// split. We need to handle it like above, with merging vertices

		// Identify the hinge, common vertex
		int otherLineOtherVertexID = otherLine->OtherVertex(vertID);
		if(!doc.linedefs[lineID]->TouchesVertex(otherLineOtherVertexID))
			continue;	// not a hinge between them

		return i;
	}

	return -1;
}

//
// Splits a linedef while also considering resulted merged lines.
// It needs delta_x and delta_y in order to properly merge sandwich linedefs.
//
void ObjectsModule::splitLinedefAndMergeSandwich(int splitLineID, int vertID, 
												 double delta_x, double delta_y) const
{
	// Add a vertex there and do the split
	int newVID = doc.basis.addNew(ObjType::vertices);
	Vertex *newV = doc.vertices[newVID];
	*newV = *doc.vertices[vertID];

	// Move it to the actual destination
	newV->raw_x += inst.MakeValidCoord(delta_x);
	newV->raw_y += inst.MakeValidCoord(delta_y);

	doc.linemod.splitLinedefAtVertex(splitLineID, newVID);

	// Now merge the vertices
	selection_c verts(ObjType::vertices);
	verts.set(newVID);
	verts.set(vertID);

	doc.vertmod.mergeList(&verts);
}

//
// Move the vertex after having performed a drag
//
void ObjectsModule::moveVertexPostDrag(const Objid &obj, double delta_x, double delta_y,
									   double delta_z) const
{
	/* move a single vertex */

	doc.basis.begin();

	int did_split_line = -1;

	// handle a single vertex merging onto an existing one
	if (inst.edit.highlight.valid())
	{
		doc.basis.setMessage("merge vertex #%d", obj.num);

		SYS_ASSERT(obj.num != inst.edit.highlight.num);

		selection_c verts(ObjType::vertices);

		verts.set(inst.edit.highlight.num);	// keep the highlight
		verts.set(obj.num);

		doc.vertmod.mergeList(&verts);

		doc.basis.end();
		return;
	}

	// handle a single vertex splitting a linedef
	if (inst.edit.split_line.valid())
	{
		did_split_line = inst.edit.split_line.num;
		
		// Check if it's actually a case of splitting a neighbouring linedef
		if(findLineBetweenLineAndVertex(did_split_line, obj.num) >= 0)
		{
			// Alright, we got it
			doc.basis.setMessage("split linedef #%d", did_split_line);
			splitLinedefAndMergeSandwich(did_split_line, obj.num, delta_x, delta_y);
			doc.basis.end();
			return;
		}

		doc.linemod.splitLinedefAtVertex(did_split_line, obj.num);

		// now move the vertex!
	}

	selection_c list(inst.edit.mode);

	list.set(obj.num);

	doMoveObjects(&list, delta_x, delta_y, delta_z);

	if (did_split_line >= 0)
		doc.basis.setMessage("split linedef #%d", did_split_line);
	else
		doc.basis.setMessageForSelection("moved", list);

	doc.basis.end();
}

void ObjectsModule::singleDrag(const Objid &obj, double delta_x, double delta_y, double delta_z) const
{
	if(inst.edit.mode == ObjType::sectors || inst.edit.mode == ObjType::linedefs)
	{
		selection_c source(inst.edit.mode);
		source.set(obj.num);

		selection_c vertices(ObjType::vertices);
		ConvertSelection(doc, &source, &vertices);

		for(sel_iter_c it(vertices); !it.done(); it.next())
			moveVertexPostDrag(Objid(ObjType::vertices, *it), delta_x, delta_y, delta_z);
		return;
	}
	if (inst.edit.mode != ObjType::vertices)
	{
		selection_c list(inst.edit.mode);
		list.set(obj.num);

		doc.objects.move(&list, delta_x, delta_y, delta_z);
		return;
	}
	moveVertexPostDrag(obj, delta_x, delta_y, delta_z);
}


void ObjectsModule::transferThingProperties(int src_thing, int dest_thing) const
{
	const Thing * T = doc.things[src_thing];

	doc.basis.changeThing(dest_thing, Thing::F_TYPE,    T->type);
	doc.basis.changeThing(dest_thing, Thing::F_OPTIONS, T->options);
//	BA_ChangeTH(dest_thing, Thing::F_ANGLE,   T->angle);

	doc.basis.changeThing(dest_thing, Thing::F_TID,     T->tid);
	doc.basis.changeThing(dest_thing, Thing::F_SPECIAL, T->special);

	doc.basis.changeThing(dest_thing, Thing::F_ARG1, T->arg1);
	doc.basis.changeThing(dest_thing, Thing::F_ARG2, T->arg2);
	doc.basis.changeThing(dest_thing, Thing::F_ARG3, T->arg3);
	doc.basis.changeThing(dest_thing, Thing::F_ARG4, T->arg4);
	doc.basis.changeThing(dest_thing, Thing::F_ARG5, T->arg5);
}


void ObjectsModule::transferSectorProperties(int src_sec, int dest_sec) const
{
	const Sector * sector = doc.sectors[src_sec];

	doc.basis.changeSector(dest_sec, Sector::F_FLOORH,    sector->floorh);
	doc.basis.changeSector(dest_sec, Sector::F_FLOOR_TEX, sector->floor_tex);
	doc.basis.changeSector(dest_sec, Sector::F_CEILH,     sector->ceilh);
	doc.basis.changeSector(dest_sec, Sector::F_CEIL_TEX,  sector->ceil_tex);

	doc.basis.changeSector(dest_sec, Sector::F_LIGHT,  sector->light);
	doc.basis.changeSector(dest_sec, Sector::F_TYPE,   sector->type);
	doc.basis.changeSector(dest_sec, Sector::F_TAG,    sector->tag);
}


#define LINEDEF_FLAG_KEEP  (MLF_Blocking + MLF_TwoSided)

void ObjectsModule::transferLinedefProperties(int src_line, int dest_line, bool do_tex) const
{
	const LineDef * L1 = doc.linedefs[src_line];
	const LineDef * L2 = doc.linedefs[dest_line];

	// don't transfer certain flags
	int flags = doc.linedefs[dest_line]->flags;
	flags = (flags & LINEDEF_FLAG_KEEP) | (L1->flags & ~LINEDEF_FLAG_KEEP);

	// handle textures
	if (do_tex && L1->Right(doc) && L2->Right(doc))
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
		if (! L1->Left(doc))
		{
			int tex = L1->Right(doc)->mid_tex;

			if (! L2->Left(doc))
			{
				doc.basis.changeSidedef(L2->right, SideDef::F_MID_TEX, tex);
			}
			else
			{
				doc.basis.changeSidedef(L2->right, SideDef::F_LOWER_TEX, tex);
				doc.basis.changeSidedef(L2->right, SideDef::F_UPPER_TEX, tex);

				doc.basis.changeSidedef(L2->left,  SideDef::F_LOWER_TEX, tex);
				doc.basis.changeSidedef(L2->left,  SideDef::F_UPPER_TEX, tex);

				// this is debatable....   CONFIG ITEM?
				flags |= MLF_LowerUnpegged;
				flags |= MLF_UpperUnpegged;
			}
		}
		else if (! L2->Left(doc))
		{
			/* pick which texture to copy */

			const Sector *front = L1->Right(doc)->SecRef(doc);
			const Sector *back  = L1-> Left(doc)->SecRef(doc);

			int f_l = L1->Right(doc)->lower_tex;
			int f_u = L1->Right(doc)->upper_tex;
			int b_l = L1-> Left(doc)->lower_tex;
			int b_u = L1-> Left(doc)->upper_tex;

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
				doc.basis.changeSidedef(L2->right, SideDef::F_MID_TEX, tex);
			}
		}
		else
		{
			const SideDef *RS = L1->Right(doc);
			const SideDef *LS = L1->Left(doc);

			const Sector *F1 = L1->Right(doc)->SecRef(doc);
			const Sector *B1 = L1-> Left(doc)->SecRef(doc);
			const Sector *F2 = L2->Right(doc)->SecRef(doc);
			const Sector *B2 = L2-> Left(doc)->SecRef(doc);

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

			doc.basis.changeSidedef(L2->right, SideDef::F_LOWER_TEX, RS->lower_tex);
			doc.basis.changeSidedef(L2->right, SideDef::F_MID_TEX,   RS->mid_tex);
			doc.basis.changeSidedef(L2->right, SideDef::F_UPPER_TEX, RS->upper_tex);

			doc.basis.changeSidedef(L2->left, SideDef::F_LOWER_TEX, LS->lower_tex);
			doc.basis.changeSidedef(L2->left, SideDef::F_MID_TEX,   LS->mid_tex);
			doc.basis.changeSidedef(L2->left, SideDef::F_UPPER_TEX, LS->upper_tex);
		}
	}

	doc.basis.changeLinedef(dest_line, LineDef::F_FLAGS, flags);

	doc.basis.changeLinedef(dest_line, LineDef::F_TYPE, L1->type);
	doc.basis.changeLinedef(dest_line, LineDef::F_TAG,  L1->tag);

	doc.basis.changeLinedef(dest_line, LineDef::F_ARG2, L1->arg2);
	doc.basis.changeLinedef(dest_line, LineDef::F_ARG3, L1->arg3);
	doc.basis.changeLinedef(dest_line, LineDef::F_ARG4, L1->arg4);
	doc.basis.changeLinedef(dest_line, LineDef::F_ARG5, L1->arg5);
}


void Instance::CMD_CopyProperties()
{
	if (edit.highlight.is_nil())
	{
		Beep("No target for CopyProperties");
		return;
	}
	if (edit.Selected->empty())
	{
		Beep("No source for CopyProperties");
		return;
	}
	if (edit.mode == ObjType::vertices)
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

		level.basis.begin();
		level.basis.setMessage("copied properties");

		switch (edit.mode)
		{
			case ObjType::sectors:
				level.objects.transferSectorProperties(source, target);
				break;

			case ObjType::things:
				level.objects.transferThingProperties(source, target);
				break;

			case ObjType::linedefs:
				level.objects.transferLinedefProperties(source, target, true /* do_tex */);
				break;

			default: break;
		}

		level.basis.end();

	}
	else  /* reverse mode, HILITE --> SEL */
	{
		if (edit.Selected->count_obj() == 1 && edit.Selected->find_first() == edit.highlight.num)
		{
			Beep("No selection for CopyProperties");
			return;
		}

		int source = edit.highlight.num;

		level.basis.begin();
		level.basis.setMessage("copied properties");

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			if (*it == source)
				continue;

			switch (edit.mode)
			{
				case ObjType::sectors:
					level.objects.transferSectorProperties(source, *it);
					break;

				case ObjType::things:
					level.objects.transferThingProperties(source, *it);
					break;

				case ObjType::linedefs:
					level.objects.transferLinedefProperties(source, *it, true /* do_tex */);
					break;

				default: break;
			}
		}

		level.basis.end();
	}
}


void ObjectsModule::dragCountOnGridWorker(ObjType obj_type, int objnum, int *count, int *total) const
{
	switch (obj_type)
	{
		case ObjType::things:
			*total += 1;
			if (inst.grid.OnGrid(doc.things[objnum]->x(), doc.things[objnum]->y()))
				*count += 1;
			break;

		case ObjType::vertices:
			*total += 1;
			if (inst.grid.OnGrid(doc.vertices[objnum]->x(), doc.vertices[objnum]->y()))
				*count += 1;
			break;

		case ObjType::linedefs:
			dragCountOnGridWorker(ObjType::vertices, doc.linedefs[objnum]->start, count, total);
			dragCountOnGridWorker(ObjType::vertices, doc.linedefs[objnum]->end,   count, total);
			break;

		case ObjType::sectors:
			for (int n = 0 ; n < doc.numLinedefs(); n++)
			{
				LineDef *L = doc.linedefs[n];

				if (! L->TouchesSector(objnum, doc))
					continue;

				dragCountOnGridWorker(ObjType::linedefs, n, count, total);
			}
			break;

		default:
			break;
	}
}


void ObjectsModule::dragCountOnGrid(int *count, int *total) const
{
	// Note: the results are approximate, vertices can be counted two
	//       or more times.

	for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
	{
		dragCountOnGridWorker(inst.edit.mode, *it, count, total);
	}
}


void ObjectsModule::dragUpdateCurrentDist(ObjType obj_type, int objnum, double *x, double *y,
								   double *best_dist, double ptr_x, double ptr_y,
								   bool only_grid) const
{
	double x2, y2;

	switch (obj_type)
	{
		case ObjType::things:
			x2 = doc.things[objnum]->x();
			y2 = doc.things[objnum]->y();
			break;

		case ObjType::vertices:
			x2 = doc.vertices[objnum]->x();
			y2 = doc.vertices[objnum]->y();
			break;

		case ObjType::linedefs:
			{
				LineDef *L = doc.linedefs[objnum];

				dragUpdateCurrentDist(ObjType::vertices, L->start, x, y, best_dist,
									   ptr_x, ptr_y, only_grid);

				dragUpdateCurrentDist(ObjType::vertices, L->end,   x, y, best_dist,
				                       ptr_x, ptr_y, only_grid);
			}
			return;

		case ObjType::sectors:
			// recursively handle all vertices belonging to the sector
			// (some vertices can be processed two or more times, that
			// won't matter though).

			for (int n = 0 ; n < doc.numLinedefs(); n++)
			{
				LineDef *L = doc.linedefs[n];

				if (! L->TouchesSector(objnum, doc))
					continue;

				dragUpdateCurrentDist(ObjType::linedefs, n, x, y, best_dist,
				                       ptr_x, ptr_y, only_grid);
			}
			return;

		default:
			return;
	}

	// handle OBJ_THINGS and OBJ_VERTICES

	if (only_grid && !inst.grid.OnGrid(x2, y2))
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
void ObjectsModule::getDragFocus(double *x, double *y, double ptr_x, double ptr_y) const
{
	*x = 0;
	*y = 0;

	// determine whether a majority of the object(s) are already on
	// the grid.  If they are, then pick a coordinate that also lies
	// on the grid.
	bool only_grid = false;

	int count = 0;
	int total = 0;

	if (inst.grid.snap)
	{
		dragCountOnGrid(&count, &total);

		if (total > 0 && count > total / 2)
			only_grid = true;
	}

	// determine object which is closest to mouse pointer AND which
	// honors the 'only_grid' property (when set).
	double best_dist = 9e9;

	if (inst.edit.dragged.valid())  // a single object
	{
		dragUpdateCurrentDist(inst.edit.mode, inst.edit.dragged.num, x, y, &best_dist,
							   ptr_x, ptr_y, only_grid);
		return;
	}

	for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
	{
		dragUpdateCurrentDist(inst.edit.mode, *it, x, y, &best_dist,
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
void ObjectsModule::calcMiddle(selection_c * list, double *x, double *y) const
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
				sum_x += doc.things[*it]->x();
				sum_y += doc.things[*it]->y();
			}
			break;
		}

		case ObjType::vertices:
		{
			for (sel_iter_c it(list) ; !it.done() ; it.next(), ++count)
			{
				sum_x += doc.vertices[*it]->x();
				sum_y += doc.vertices[*it]->y();
			}
			break;
		}

		// everything else: just use the vertices
		default:
		{
			selection_c verts(ObjType::vertices);
			ConvertSelection(doc, list, &verts);

			calcMiddle(&verts, x, y);
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
void ObjectsModule::calcBBox(selection_c * list, double *x1, double *y1, double *x2, double *y2) const
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
				const Thing *T = doc.things[*it];
				double Tx = T->x();
				double Ty = T->y();

				const thingtype_t &info = inst.M_GetThingType(T->type);
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
				const Vertex *V = doc.vertices[*it];
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
			ConvertSelection(doc, list, &verts);

			calcBBox(&verts, x1, y1, x2, y2);
			return;
		}
	}

	SYS_ASSERT(*x1 <= *x2);
	SYS_ASSERT(*y1 <= *y2);
}


void ObjectsModule::doMirrorThings(selection_c *list, bool is_vert, double mid_x, double mid_y) const
{
	fixcoord_t fix_mx = inst.MakeValidCoord(mid_x);
	fixcoord_t fix_my = inst.MakeValidCoord(mid_y);

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Thing * T = doc.things[*it];

		if (is_vert)
		{
			doc.basis.changeThing(*it, Thing::F_Y, 2*fix_my - T->raw_y);

			if (T->angle != 0)
				doc.basis.changeThing(*it, Thing::F_ANGLE, 360 - T->angle);
		}
		else
		{
			doc.basis.changeThing(*it, Thing::F_X, 2*fix_mx - T->raw_x);

			if (T->angle > 180)
				doc.basis.changeThing(*it, Thing::F_ANGLE, 540 - T->angle);
			else
				doc.basis.changeThing(*it, Thing::F_ANGLE, 180 - T->angle);
		}
	}
}


void ObjectsModule::doMirrorVertices(selection_c *list, bool is_vert, double mid_x, double mid_y) const
{
	fixcoord_t fix_mx = inst.MakeValidCoord(mid_x);
	fixcoord_t fix_my = inst.MakeValidCoord(mid_y);

	selection_c verts(ObjType::vertices);
	ConvertSelection(doc, list, &verts);

	for (sel_iter_c it(verts) ; !it.done() ; it.next())
	{
		const Vertex * V = doc.vertices[*it];

		if (is_vert)
			doc.basis.changeVertex(*it, Vertex::F_Y, 2*fix_my - V->raw_y);
		else
			doc.basis.changeVertex(*it, Vertex::F_X, 2*fix_mx - V->raw_x);
	}

	// flip linedefs too !!
	selection_c lines(ObjType::linedefs);
	ConvertSelection(doc, &verts, &lines);

	for (sel_iter_c it(lines) ; !it.done() ; it.next())
	{
		LineDef * L = doc.linedefs[*it];

		int start = L->start;
		int end   = L->end;

		doc.basis.changeLinedef(*it, LineDef::F_START, end);
		doc.basis.changeLinedef(*it, LineDef::F_END, start);
	}
}


void ObjectsModule::doMirrorStuff(selection_c *list, bool is_vert, double mid_x, double mid_y) const
{
	if (inst.edit.mode == ObjType::things)
	{
		doMirrorThings(list, is_vert, mid_x, mid_y);
		return;
	}

	// everything else just modifies the vertices

	if (inst.edit.mode == ObjType::sectors)
	{
		// handle things in Sectors mode too
		selection_c things(ObjType::things);
		ConvertSelection(doc, list, &things);

		doMirrorThings(&things, is_vert, mid_x, mid_y);
	}

	doMirrorVertices(list, is_vert, mid_x, mid_y);
}


void Instance::CMD_Mirror()
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
	level.objects.calcMiddle(edit.Selected, &mid_x, &mid_y);

	level.basis.begin();
	level.basis.setMessageForSelection("mirrored", *edit.Selected, is_vert ? " vertically" : " horizontally");

	level.objects.doMirrorStuff(edit.Selected, is_vert, mid_x, mid_y);

	level.basis.end();

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void ObjectsModule::doRotate90Things(selection_c *list, bool anti_clockwise,
							 double mid_x, double mid_y) const
{
	fixcoord_t fix_mx = inst.MakeValidCoord(mid_x);
	fixcoord_t fix_my = inst.MakeValidCoord(mid_y);

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Thing * T = doc.things[*it];

		fixcoord_t old_x = T->raw_x;
		fixcoord_t old_y = T->raw_y;

		if (anti_clockwise)
		{
			doc.basis.changeThing(*it, Thing::F_X, fix_mx - old_y + fix_my);
			doc.basis.changeThing(*it, Thing::F_Y, fix_my + old_x - fix_mx);

			doc.basis.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, +90));
		}
		else
		{
			doc.basis.changeThing(*it, Thing::F_X, fix_mx + old_y - fix_my);
			doc.basis.changeThing(*it, Thing::F_Y, fix_my - old_x + fix_mx);

			doc.basis.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, -90));
		}
	}
}


void Instance::CMD_Rotate90()
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
	level.objects.calcMiddle(edit.Selected, &mid_x, &mid_y);

	level.basis.begin();
	level.basis.setMessageForSelection("rotated", *edit.Selected, anti_clockwise ? " anti-clockwise" : " clockwise");

	if (edit.mode == ObjType::things)
	{
		level.objects.doRotate90Things(edit.Selected, anti_clockwise, mid_x, mid_y);
	}
	else
	{
		// handle things inside sectors
		if (edit.mode == ObjType::sectors)
		{
			selection_c things(ObjType::things);
			ConvertSelection(level, edit.Selected, &things);

			level.objects.doRotate90Things(&things, anti_clockwise, mid_x, mid_y);
		}

		// everything else just rotates the vertices
		selection_c verts(ObjType::vertices);
		ConvertSelection(level, edit.Selected, &verts);

		fixcoord_t fix_mx = MakeValidCoord(mid_x);
		fixcoord_t fix_my = MakeValidCoord(mid_y);

		for (sel_iter_c it(verts) ; !it.done() ; it.next())
		{
			const Vertex * V = level.vertices[*it];

			fixcoord_t old_x = V->raw_x;
			fixcoord_t old_y = V->raw_y;

			if (anti_clockwise)
			{
				level.basis.changeVertex(*it, Vertex::F_X, fix_mx - old_y + fix_my);
				level.basis.changeVertex(*it, Vertex::F_Y, fix_my + old_x - fix_mx);
			}
			else
			{
				level.basis.changeVertex(*it, Vertex::F_X, fix_mx + old_y - fix_my);
				level.basis.changeVertex(*it, Vertex::F_Y, fix_my - old_x + fix_mx);
			}
		}
	}

	level.basis.end();

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void ObjectsModule::doScaleTwoThings(selection_c *list, transform_t& param) const
{
	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Thing * T = doc.things[*it];

		double new_x = T->x();
		double new_y = T->y();

		param.Apply(&new_x, &new_y);

		doc.basis.changeThing(*it, Thing::F_X, inst.MakeValidCoord(new_x));
		doc.basis.changeThing(*it, Thing::F_Y, inst.MakeValidCoord(new_y));

		float rot1 = static_cast<float>(param.rotate / (M_PI / 4));

		int ang_diff = static_cast<int>(I_ROUND(rot1) * 45.0);

		if (ang_diff)
		{
			doc.basis.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, ang_diff));
		}
	}
}


void ObjectsModule::doScaleTwoVertices(selection_c *list, transform_t& param) const 
{
	selection_c verts(ObjType::vertices);
	ConvertSelection(doc, list, &verts);

	for (sel_iter_c it(verts) ; !it.done() ; it.next())
	{
		const Vertex * V = doc.vertices[*it];

		double new_x = V->x();
		double new_y = V->y();

		param.Apply(&new_x, &new_y);

		doc.basis.changeVertex(*it, Vertex::F_X, inst.MakeValidCoord(new_x));
		doc.basis.changeVertex(*it, Vertex::F_Y, inst.MakeValidCoord(new_y));
	}
}


void ObjectsModule::doScaleTwoStuff(selection_c *list, transform_t& param) const
{
	if (inst.edit.mode == ObjType::things)
	{
		doScaleTwoThings(list, param);
		return;
	}

	// everything else just modifies the vertices

	if (inst.edit.mode == ObjType::sectors)
	{
		// handle things in Sectors mode too
		selection_c things(ObjType::things);
		ConvertSelection(doc, list, &things);

		doScaleTwoThings(&things, param);
	}

	doScaleTwoVertices(list, param);
}


void ObjectsModule::transform(transform_t& param) const
{
	// this is called by the MOUSE2 dynamic scaling code

	SYS_ASSERT(inst.edit.Selected->notempty());

	doc.basis.begin();
	doc.basis.setMessageForSelection("scaled", *inst.edit.Selected);

	if (param.scale_x < 0)
	{
		param.scale_x = -param.scale_x;
		doMirrorStuff(inst.edit.Selected, false /* is_vert */, param.mid_x, param.mid_y);
	}

	if (param.scale_y < 0)
	{
		param.scale_y = -param.scale_y;
		doMirrorStuff(inst.edit.Selected, true /* is_vert */, param.mid_x, param.mid_y);
	}

	doScaleTwoStuff(inst.edit.Selected, param);

	doc.basis.end();
}


void ObjectsModule::determineOrigin(transform_t& param, double pos_x, double pos_y) const 
{
	if (pos_x == 0 && pos_y == 0)
	{
		doc.objects.calcMiddle(inst.edit.Selected, &param.mid_x, &param.mid_y);
		return;
	}

	double lx, ly, hx, hy;

	doc.objects.calcBBox(inst.edit.Selected, &lx, &ly, &hx, &hy);

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


void ObjectsModule::scale3(double scale_x, double scale_y, double pos_x, double pos_y) const
{
	SYS_ASSERT(scale_x > 0);
	SYS_ASSERT(scale_y > 0);

	transform_t param;

	param.Clear();

	param.scale_x = scale_x;
	param.scale_y = scale_y;

	determineOrigin(param, pos_x, pos_y);

	doc.basis.begin();
	doc.basis.setMessageForSelection("scaled", *inst.edit.Selected);
	{
		doScaleTwoStuff(inst.edit.Selected, param);
	}
	doc.basis.end();
}


void ObjectsModule::doScaleSectorHeights(selection_c *list, double scale_z, int pos_z) const
{
	SYS_ASSERT(! list->empty());

	// determine Z range and origin
	int lz = +99999;
	int hz = -99999;

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Sector * S = doc.sectors[*it];

		lz = std::min(lz, S->floorh);
		hz = std::max(hz, S->ceilh);
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
		const Sector * S = doc.sectors[*it];

		int new_f = mid_z + I_ROUND((S->floorh - mid_z) * scale_z);
		int new_c = mid_z + I_ROUND((S-> ceilh - mid_z) * scale_z);

		doc.basis.changeSector(*it, Sector::F_FLOORH, new_f);
		doc.basis.changeSector(*it, Sector::F_CEILH,  new_c);
	}
}

void ObjectsModule::scale4(double scale_x, double scale_y, double scale_z,
                   double pos_x, double pos_y, double pos_z) const
{
	SYS_ASSERT(inst.edit.mode == ObjType::sectors);

	transform_t param;

	param.Clear();

	param.scale_x = scale_x;
	param.scale_y = scale_y;

	determineOrigin(param, pos_x, pos_y);

	doc.basis.begin();
	doc.basis.setMessageForSelection("scaled", *inst.edit.Selected);
	{
		doScaleTwoStuff(inst.edit.Selected, param);
		doScaleSectorHeights(inst.edit.Selected, scale_z, static_cast<int>(pos_z));
	}
	doc.basis.end();
}


void ObjectsModule::rotate3(double deg, double pos_x, double pos_y) const
{
	transform_t param;

	param.Clear();

	param.rotate = deg * M_PI / 180.0;

	determineOrigin(param, pos_x, pos_y);

	doc.basis.begin();
	doc.basis.setMessageForSelection("rotated", *inst.edit.Selected);
	{
		doScaleTwoStuff(inst.edit.Selected, param);
	}
	doc.basis.end();
}


bool ObjectsModule::spotInUse(ObjType obj_type, int x, int y) const
{
	switch (obj_type)
	{
		case ObjType::things:
			for (const Thing *thing : doc.things)
				if (I_ROUND(thing->x()) == x && I_ROUND(thing->y()) == y)
					return true;
			return false;

		case ObjType::vertices:
			for (const Vertex *vertex : doc.vertices)
				if (I_ROUND(vertex->x()) == x && I_ROUND(vertex->y()) == y)
					return true;
			return false;

		default:
			BugError("IsSpotVacant: bad object type\n");
			return false;
	}
}


void ObjectsModule::doEnlargeOrShrink(bool do_shrink) const
{
	// setup transform parameters...
	float mul = 2.0;

	if (inst.EXEC_Param[0].good())
	{
		mul = static_cast<float>(atof(inst.EXEC_Param[0]));

		if (mul < 0.02 || mul > 50)
		{
			inst.Beep("bad factor: %s", inst.EXEC_Param[0].c_str());
			return;
		}
	}

	if (do_shrink)
		mul = 1.0f / mul;

	transform_t param;

	param.Clear();

	param.scale_x = mul;
	param.scale_y = mul;


	SelectHighlight unselect = inst.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("No objects to %s", do_shrink ? "shrink" : "enlarge");
		return;
	}

	// TODO: CONFIG ITEM (or FLAG)
	if ((true))
	{
		calcMiddle(inst.edit.Selected, &param.mid_x, &param.mid_y);
	}
	else
	{
		double lx, ly, hx, hy;
		calcBBox(inst.edit.Selected, &lx, &ly, &hx, &hy);

		param.mid_x = lx + (hx - lx) / 2;
		param.mid_y = ly + (hy - ly) / 2;
	}

	doc.basis.begin();
	doc.basis.setMessageForSelection(do_shrink ? "shrunk" : "enlarged", *inst.edit.Selected);

	doScaleTwoStuff(inst.edit.Selected, param);

	doc.basis.end();

	if (unselect == SelectHighlight::unselect)
		inst.Selection_Clear(true /* nosave */);
}


void Instance::CMD_Enlarge()
{
	level.objects.doEnlargeOrShrink(false /* do_shrink */);
}

void Instance::CMD_Shrink()
{
	level.objects.doEnlargeOrShrink(true /* do_shrink */);
}


void ObjectsModule::quantizeThings(selection_c *list) const
{
	// remember the things which we moved
	// (since we cannot modify the selection while we iterate over it)
	selection_c moved(list->what_type());

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const Thing * T = doc.things[*it];

		if (inst.grid.OnGrid(T->x(), T->y()))
		{
			moved.set(*it);
			continue;
		}

		for (int pass = 0 ; pass < 4 ; pass++)
		{
			int new_x = inst.grid.QuantSnapX(T->x(), pass & 1);
			int new_y = inst.grid.QuantSnapY(T->y(), pass & 2);

			if (! spotInUse(ObjType::things, new_x, new_y))
			{
				doc.basis.changeThing(*it, Thing::F_X, inst.MakeValidCoord(new_x));
				doc.basis.changeThing(*it, Thing::F_Y, inst.MakeValidCoord(new_y));

				moved.set(*it);
				break;
			}
		}
	}

	list->unmerge(moved);

	if (! list->empty())
		inst.Beep("Quantize: could not move %d things", list->count_obj());
}


void ObjectsModule::quantizeVertices(selection_c *list) const
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

	byte * vert_modes = new byte[doc.numVertices()];

	for (const LineDef *L : doc.linedefs)
	{
		// require both vertices of the linedef to be in the selection
		if (! (list->get(L->start) && list->get(L->end)))
			continue;

		// IDEA: make this a method of LineDef
		double x1 = L->Start(doc)->x();
		double y1 = L->Start(doc)->y();
		double x2 = L->End(doc)->x();
		double y2 = L->End(doc)->y();

		if (L->IsHorizontal(doc))
		{
			vert_modes[L->start] |= V_HORIZ;
			vert_modes[L->end]   |= V_HORIZ;
		}
		else if (L->IsVertical(doc))
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
		const Vertex * V = doc.vertices[*it];

		if (inst.grid.OnGrid(V->x(), V->y()))
		{
			moved.set(*it);
			continue;
		}

		byte mode = vert_modes[*it];

		for (int pass = 0 ; pass < 4 ; pass++)
		{
			int x_dir, y_dir;

			double new_x = inst.grid.QuantSnapX(V->x(), pass & 1, &x_dir);
			double new_y = inst.grid.QuantSnapY(V->y(), pass & 2, &y_dir);

			// keep horizontal lines horizontal
			if ((mode & V_HORIZ) && (pass & 2))
				continue;

			// keep vertical lines vertical
			if ((mode & V_VERT) && (pass & 1))
				continue;

			// TODO: keep diagonal lines diagonal...

			if (! spotInUse(ObjType::vertices, static_cast<int>(new_x), static_cast<int>(new_y)))
			{
				doc.basis.changeVertex(*it, Vertex::F_X, inst.MakeValidCoord(new_x));
				doc.basis.changeVertex(*it, Vertex::F_Y, inst.MakeValidCoord(new_y));

				moved.set(*it);
				break;
			}
		}
	}

	delete[] vert_modes;

	list->unmerge(moved);

	if (list->notempty())
		inst.Beep("Quantize: could not move %d vertices", list->count_obj());
}


void Instance::CMD_Quantize()
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

	level.basis.begin();
	level.basis.setMessageForSelection("quantized", *edit.Selected);

	switch (edit.mode)
	{
		case ObjType::things:
			level.objects.quantizeThings(edit.Selected);
			break;

		case ObjType::vertices:
			level.objects.quantizeVertices(edit.Selected);
			break;

		// everything else merely quantizes vertices
		default:
		{
			selection_c verts(ObjType::vertices);
			ConvertSelection(level, edit.Selected, &verts);

			level.objects.quantizeVertices(&verts);

			Selection_Clear();
			break;
		}
	}

	level.basis.end();

	edit.error_mode = true;
}

//
// Get the tag info here. Returns true if available and sets the output argument.
//
bool getSpecialTagInfo(ObjType objtype, int objnum, int special, const void *obj,
                       const ConfigData &config, SpecialTagInfo &info)
{
    if(special <= 0)
        return false;

    auto getArg = [objtype, obj](int index)
    {
        if(objtype == ObjType::things)
            return static_cast<const Thing *>(obj)->Arg(index);
        if(objtype == ObjType::linedefs)
            return static_cast<const LineDef *>(obj)->Arg(index);
        return 0;
    };

    // First try generalized
    for(int i = 0; i < config.num_gen_linetypes; ++i)
    {
        const generalized_linetype_t &type = config.gen_linetypes[i];
        if(special >= type.base && special < type.base + type.length)
        {
            info = {};
            info.type = objtype;
            info.objnum = objnum;
            info.numtags = 1;
            info.tags[0] = getArg(1);
            return true;
        }
    }

    // Now try individual specials, including parameterized
    auto it = config.line_types.find(special);
    if(it == config.line_types.end())
        return false;
    info = {};
    info.type = objtype;
    info.objnum = objnum;
    for(int i = 0; i < (int)lengthof(it->second.args); ++i)
    {
        int arg = getArg(i + 1);

        switch(it->second.args[i].type)
        {
            case SpecialArgType::tag:
                info.tags[info.numtags++] = arg;
                break;
            case SpecialArgType::tag_hi:
                info.tags[info.hitags++] += 256 * arg;    // add 256*i to corresponding regular tag
                break;
            case SpecialArgType::tid:
                info.tids[info.numtids++] = arg;
                break;
            case SpecialArgType::line_id:
                info.lineids[info.numlineids++] = arg;
                break;
            case SpecialArgType::self_line_id:
                info.selflineid = arg;
                break;
            case SpecialArgType::self_line_id_hi:
                info.selflineid += 256 * arg;
                break;
            case SpecialArgType::po:
                info.po[info.numpo++] = arg;
                break;
            default:
                break;
        }
    }
    return true;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
