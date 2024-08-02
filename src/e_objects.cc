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
#include "LineDef.h"
#include "m_config.h"
#include "m_game.h"
#include "m_vector.h"
#include "e_objects.h"
#include "r_grid.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"
#include "w_rawdef.h"

#include "ui_window.h"


// config items
int  config::new_sector_size = 128;


//
//  delete a group of objects.
//  this is very raw, e.g. it does not check for stuff that will
//  remain unused afterwards.
//
void ObjectsModule::del(EditOperation &op, const selection_c &list) const
{
	// we need to process the object numbers from highest to lowest,
	// because each deletion invalidates all higher-numbered refs
	// in the selection.  Our selection iterator cannot give us
	// what we need, hence put them into a vector for sorting.

	if (list.empty())
		return;

	std::vector<int> objnums;

	for (sel_iter_c it(list) ; !it.done() ; it.next())
		objnums.push_back(*it);

	std::sort(objnums.begin(), objnums.end());

	for (int i = (int)objnums.size()-1 ; i >= 0 ; i--)
		op.del(list.what_type(), objnums[i]);
}


void ObjectsModule::createSquare(EditOperation &op, int model) const
{
	int new_sec = op.addNew(ObjType::sectors);

	if (model >= 0)
		*doc.sectors[new_sec] = *doc.sectors[model];
	else
		doc.sectors[new_sec]->SetDefaults(inst.conf);

	double x1 = inst.grid.QuantSnapX(inst.edit.map.x, false);
	double y1 = inst.grid.QuantSnapX(inst.edit.map.y, false);

	double x2 = x1 + config::new_sector_size;
	double y2 = y1 + config::new_sector_size;

	for (int i = 0 ; i < 4 ; i++)
	{
		int new_v = op.addNew(ObjType::vertices);
		auto V = doc.vertices[new_v];

		V->SetRawX(inst.loaded.levelFormat, (i >= 2) ? x2 : x1);
		V->SetRawY(inst.loaded.levelFormat, (i==1 || i==2) ? y2 : y1);

		int new_sd = op.addNew(ObjType::sidedefs);

		doc.sidedefs[new_sd]->SetDefaults(inst.conf, false);
		doc.sidedefs[new_sd]->sector = new_sec;

		int new_ld = op.addNew(ObjType::linedefs);

		auto L = doc.linedefs[new_ld];

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

	int new_t;
	{
		EditOperation op(doc.basis);

		new_t = op.addNew(ObjType::things);
		auto T = doc.things[new_t];

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

		T->SetRawX(inst.loaded.levelFormat, inst.grid.SnapX(inst.edit.map.x));
		T->SetRawY(inst.loaded.levelFormat, inst.grid.SnapY(inst.edit.map.y));

		inst.recent_things.insert_number(T->type);

		op.setMessage("added thing #%d", new_t);
	}


	// select it
	inst.Selection_Clear();

	inst.edit.Selected->set(new_t);
}


int ObjectsModule::sectorNew(EditOperation &op, int model, int model2, int model3) const
{
	int new_sec = op.addNew(ObjType::sectors);

	if (model < 0) model = model2;
	if (model < 0) model = model3;

	if (model < 0)
		doc.sectors[new_sec]->SetDefaults(inst.conf);
	else
		*doc.sectors[new_sec] = *doc.sectors[model];

	return new_sec;
}


bool ObjectsModule::checkClosedLoop(EditOperation &op, int new_ld, int v1, int v2, selection_c &flip) const
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
		left.loop.AssignSector(op, left.sec, flip);
	}
	else if (right.loop.faces_outward && right.sec >= 0)
	{
		right.loop.AssignSector(op, right.sec, flip);
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
		int new_sec = sectorNew(op, innie.NeighboringSector(), -1, -1);

		innie.AssignSector(op, new_sec, flip);
		return true;
	}


	/* --- handle two innies --- */

	// check if the sectors in each loop are different.
	// this is not the usual situation!  we assume the user has
	// deleted a linedef or two and is correcting the geometry.

	if (left.sec != right.sec)
	{
		if (right.sec >= 0) right.loop.AssignSector(op, right.sec, flip);
		if ( left.sec >= 0)  left.loop.AssignSector(op,  left.sec, flip);

		return true;
	}

	// we are creating a NEW sector in one loop (the smallest),
	// and updating the other loop (unless it is VOID).

	// the ordering here is significant, and ensures that the
	// new linedef usually ends at v2 (the final vertex).

	if (left.length < right.length)
	{
		int new_sec = sectorNew(op, left.sec, right.sec, left.loop.NeighboringSector());

		if (right.sec >= 0)
			right.loop.AssignSector(op, right.sec, flip);

		left.loop.AssignSector(op, new_sec, flip);
	}
	else
	{
		int new_sec = sectorNew(op, right.sec, left.sec, right.loop.NeighboringSector());

		right.loop.AssignSector(op, new_sec, flip);

		if (left.sec >= 0)
			left.loop.AssignSector(op, left.sec, flip);
	}

	return true;
}


void ObjectsModule::insertLinedef(EditOperation &op, int v1, int v2, bool no_fill) const
{
	if (doc.linemod.linedefAlreadyExists(v1, v2))
		return;

	int new_ld = op.addNew(ObjType::linedefs);

	auto L = doc.linedefs[new_ld];

	L->start = v1;
	L->end   = v2;
	L->flags = MLF_Blocking;

	if (no_fill)
		return;

	if (doc.vertmod.howManyLinedefs(v1) >= 2 &&
		doc.vertmod.howManyLinedefs(v2) >= 2)
	{
		selection_c flip(ObjType::linedefs);

		checkClosedLoop(op, new_ld, v1, v2, flip);

		doc.linemod.flipLinedefGroup(op, &flip);
	}
}


void ObjectsModule::insertLinedefAutosplit(EditOperation &op, int v1, int v2, bool no_fill) const
{
	// Find a linedef which this new line would cross, and if it exists
	// add a vertex there and create TWO lines.  Also handle a vertex
	// that this line crosses (sits on) similarly.

///  fprintf(stderr, "Insert_LineDef_autosplit %d..%d\n", v1, v2);

	crossing_state_c cross(inst);

	doc.hover.findCrossingPoints(cross, doc.vertices[v1]->xy(), v1, doc.vertices[v2]->xy(), v2);

	cross.SplitAllLines(op);

	int cur_v = v1;

	for (unsigned int k = 0 ; k < cross.points.size() ; k++)
	{
		int next_v = cross.points[k].vert;

		SYS_ASSERT(next_v != v1);
		SYS_ASSERT(next_v != v2);

		insertLinedef(op, cur_v, next_v, no_fill);

		cur_v = next_v;
	}

	insertLinedef(op, cur_v, v2, no_fill);
}


void ObjectsModule::insertVertex(bool force_continue, bool no_fill) const
{
	bool closed_a_loop = false;

	// when these both >= 0, we will add a linedef between them
	int old_vert = -1;
	int new_vert = -1;

	v2double_t newpos = inst.grid.Snap(inst.edit.map.xy);

	int orig_num_sectors = doc.numSectors();


	// are we drawing a line?
	if (inst.edit.action == EditorAction::drawLine)
	{
		old_vert = inst.edit.drawLine.from.num;

		newpos = inst.edit.drawLine.to;
	}

	// a linedef which we are splitting (usually none)
	int split_ld = inst.edit.split_line.valid() ? inst.edit.split_line.num : -1;

	if (split_ld >= 0)
	{
		newpos = inst.edit.split;

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
		if (new_vert < 0 && inst.grid.snaps() && ! (inst.edit.action == EditorAction::drawLine))
			new_vert = doc.vertmod.findExact(FFixedPoint(newpos.x), FFixedPoint(newpos.y));

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
			if (inst.edit.action == EditorAction::nothing)
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
				inst.edit.drawLine.from = Objid(ObjType::vertices, new_vert);
				inst.edit.Selected->set(new_vert);
				return;
			}
		}
	}

	// at here: if new_vert >= 0, then old_vert >= 0 and split_ld < 0


	// would we create a new vertex on top of an existing one?
	if (new_vert < 0 && old_vert >= 0 &&
		doc.vertices[old_vert]->Matches(MakeValidCoord(inst.loaded.levelFormat, newpos.x), MakeValidCoord(inst.loaded.levelFormat, newpos.y)))
	{
		inst.edit.Selected->set(old_vert);
		return;
	}


	{
		EditOperation op(doc.basis);

		if (new_vert < 0)
		{
			new_vert = op.addNew(ObjType::vertices);

			auto V = doc.vertices[new_vert];

			V->SetRawXY(inst.loaded.levelFormat, newpos);

			inst.edit.drawLine.from = Objid(ObjType::vertices, new_vert);
			inst.edit.Selected->set(new_vert);

			// splitting an existing line?
			if (split_ld >= 0)
			{
				doc.linemod.splitLinedefAtVertex(op, split_ld, new_vert);
				op.setMessage("split linedef #%d", split_ld);
			}
			else
			{
				op.setMessage("added vertex #%d", new_vert);
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
			insertLinedefAutosplit(op, old_vert, new_vert, no_fill);

			op.setMessage("added linedef");

			inst.edit.drawLine.from = Objid(ObjType::vertices, new_vert);
			inst.edit.Selected->set(new_vert);
		}
	}


begin_drawing:
	// begin drawing mode?
	if (inst.edit.action == EditorAction::nothing && !closed_a_loop &&
		old_vert >= 0 && new_vert < 0)
	{
		inst.Selection_Clear();

		inst.edit.drawLine.from = Objid(ObjType::vertices, old_vert);
		inst.edit.Selected->set(old_vert);

		inst.edit.drawLine.to = doc.vertices[old_vert]->xy();

		inst.Editor_SetAction(EditorAction::drawLine);
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
		ConvertSelection(doc, sel, *inst.edit.Selected);
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
	if (hover::isPointOutsideOfMap(doc, inst.edit.map.xy))
	{
		EditOperation op(doc.basis);
		op.setMessage("added sector (outside map)");

		int model = -1;
		if (sel_count > 0)
			model = inst.edit.Selected->find_first();

		createSquare(op, model);
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

	bool ok;
	{
		EditOperation op(doc.basis);
		op.setMessage("added new sector");

		ok = doc.secmod.assignSectorToSpace(op, inst.edit.map.xy, -1 /* create */, model);
	}
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
	double lx0 = doc.getStart(*doc.linedefs[ld]).x();
	double ly0 = doc.getStart(*doc.linedefs[ld]).y();
	double lx1 = doc.getEnd(*doc.linedefs[ld]).x();
	double ly1 = doc.getEnd(*doc.linedefs[ld]).y();

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

//
// Move a single vertex, without depending on the user interface highlighting
//
static void doMoveVertex(EditOperation &op, Instance &inst, const int vertexID,
						 const v2double_t &delta, int &deletedVertexID, const selection_c &movingGroup)
{
	const Vertex &vertex = *inst.level.vertices[vertexID];
	deletedVertexID = -1;

	v2double_t dest = vertex.xy() + delta;

	Objid obj = hover::getNearbyObject(ObjType::vertices, inst.level, inst.conf, inst.grid, dest);
	if(obj.valid() && obj.num != vertexID && !movingGroup.get(obj.num))
	{
		// Vertex merging
		// TODO: messaging
		selection_c del_list;

		selection_c verts(ObjType::vertices);
		verts.set(obj.num);
		verts.set(vertexID);
		inst.level.vertmod.mergeList(op, verts, &del_list);

		deletedVertexID = del_list.find_first();
		return;
	}

	v2double_t splitPoint;
	int splitLine = -1;
	obj = hover::findSplitLine(inst.level, inst.loaded.levelFormat, inst.edit, inst.grid,
							   splitPoint, dest, vertexID);
	if(obj.valid())
	{
		const auto& L = inst.level.linedefs[obj.num];
		assert(L);
		if (!movingGroup.get(L->start) && !movingGroup.get(L->end))
		{
			splitLine = obj.num;
			if (inst.level.objects.findLineBetweenLineAndVertex(splitLine, vertexID) >= 0)
			{
				// TODO: messaging
				selection_c del_list;
				inst.level.objects.splitLinedefAndMergeSandwich(op, splitLine, vertexID, delta,
					&del_list);
				deletedVertexID = del_list.find_first();
				return;
			}
			inst.level.linemod.splitLinedefAtVertex(op, splitLine, vertexID);
		}
	}

	op.changeVertex(vertexID, Thing::F_X, vertex.raw_x + MakeValidCoord(inst.loaded.levelFormat,
																		delta.x));
	op.changeVertex(vertexID, Thing::F_Y, vertex.raw_y + MakeValidCoord(inst.loaded.levelFormat,
																		delta.y));
}

void ObjectsModule::doMoveObjects(EditOperation &op, const selection_c &list,
								  const v3double_t &delta) const
{
	switch (list.what_type())
	{
		case ObjType::things:
		{
			FFixedPoint fdx = MakeValidCoord(inst.loaded.levelFormat, delta.x);
			FFixedPoint fdy = MakeValidCoord(inst.loaded.levelFormat, delta.y);
			FFixedPoint fdz = MakeValidCoord(inst.loaded.levelFormat, delta.z);

			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const auto T = doc.things[*it];

				op.changeThing(*it, Thing::F_X, T->raw_x + fdx);
				op.changeThing(*it, Thing::F_Y, T->raw_y + fdy);
				op.changeThing(*it, Thing::F_H, std::max(FFixedPoint{}, T->raw_h + fdz));
			}
			break;
		}

		case ObjType::vertices:
		{
			// We need the selection list as an array so we can easily modify it during iteration
			std::vector<int> sel = list.asArray();
			selection_c movingGroup = list;

			for(auto it = sel.begin(); it != sel.end(); ++it)
			{
				int deletedVertex = -1;
				doMoveVertex(op, inst, *it, delta.xy, deletedVertex, movingGroup);
				if (deletedVertex >= 0 && deletedVertex < list.max_obj())
					for (auto jt = it + 1; jt != sel.end(); ++jt)
						if (*jt > deletedVertex)
						{
							movingGroup.clear(*jt);
							--*jt;
							movingGroup.set(*jt);
						}
				movingGroup.clear(*it);
			}
			break;
		}
		case ObjType::sectors:
			// apply the Z delta first
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const auto S = doc.sectors[*it];

				op.changeSector(*it, Sector::F_FLOORH, S->floorh + (int)delta.z);
				op.changeSector(*it, Sector::F_CEILH,  S->ceilh  + (int)delta.z);
			}

			/* FALL-THROUGH !! */

		case ObjType::linedefs:
			{
				selection_c verts(ObjType::vertices);
				ConvertSelection(doc, list, verts);

				doMoveObjects(op, verts, delta);
			}
			break;

		default:
			break;
	}
}


void ObjectsModule::move(const selection_c &list, const v3double_t &delta) const
{
	if (list.empty())
		return;

	EditOperation op(doc.basis);
	op.setMessageForSelection("moved", list);

	int objectsBeforeMoving = 0;
	if(inst.edit.Selected)
		objectsBeforeMoving = doc.numObjects(inst.edit.Selected->what_type());

	// move things in sectors too (must do it _before_ moving the
	// sectors, otherwise we fail trying to determine which sectors
	// each thing is in).
	if (inst.edit.mode == ObjType::sectors)
	{
		selection_c thing_sel(ObjType::things);
		ConvertSelection(doc, list, thing_sel);

		doMoveObjects(op, thing_sel, { delta.x, delta.y, 0.0 });
	}

	doMoveObjects(op, list, delta);

	// We must invalidate the selection if the moving resulted in invalidation
	if(inst.edit.Selected && doc.numObjects(inst.edit.Selected->what_type()) < objectsBeforeMoving)
		inst.Selection_Clear(true);	// no save
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
		const auto otherLine = doc.linedefs[i];
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
void ObjectsModule::splitLinedefAndMergeSandwich(EditOperation &op, int splitLineID, int vertID,
												 const v2double_t &delta, selection_c *delResultList) const
{
	// Add a vertex there and do the split
	int newVID = op.addNew(ObjType::vertices);
	auto newV = doc.vertices[newVID];
	*newV = *doc.vertices[vertID];

	// Move it to the actual destination
	newV->raw_x += MakeValidCoord(inst.loaded.levelFormat, delta.x);
	newV->raw_y += MakeValidCoord(inst.loaded.levelFormat, delta.y);

	doc.linemod.splitLinedefAtVertex(op, splitLineID, newVID);

	// Now merge the vertices
	selection_c verts(ObjType::vertices);
	verts.set(newVID);
	verts.set(vertID);

	doc.vertmod.mergeList(op, verts, delResultList);
}

//
// Move a single vertex, splitting as necessary
//
static void singleDragVertex(Instance &inst, const int vertexID, const v2double_t &delta)
{
	EditOperation op(inst.level.basis);

	int did_split_line = -1;

	// handle a single vertex merging onto an existing one
	if (inst.edit.highlight.valid())
	{
		op.setMessage("merge vertex #%d", vertexID);

		SYS_ASSERT(vertexID != inst.edit.highlight.num);

		selection_c verts(ObjType::vertices);

		verts.set(inst.edit.highlight.num);	// keep the highlight
		verts.set(vertexID);

		inst.level.vertmod.mergeList(op, verts, nullptr);
		return;
	}

	// handle a single vertex splitting a linedef
	if (inst.edit.split_line.valid())
	{
		did_split_line = inst.edit.split_line.num;

		// Check if it's actually a case of splitting a neighbouring linedef
		if(inst.level.objects.findLineBetweenLineAndVertex(did_split_line, vertexID) >= 0)
		{
			// Alright, we got it
			op.setMessage("split linedef #%d", did_split_line);
			inst.level.objects.splitLinedefAndMergeSandwich(op, did_split_line, vertexID, delta, nullptr);
			return;
		}

		inst.level.linemod.splitLinedefAtVertex(op, did_split_line, vertexID);

		// now move the vertex!
	}

	const Vertex &vertex = *inst.level.vertices[vertexID];
	op.changeVertex(vertexID, Thing::F_X, vertex.raw_x + MakeValidCoord(inst.loaded.levelFormat, delta.x));
	op.changeVertex(vertexID, Thing::F_Y, vertex.raw_y + MakeValidCoord(inst.loaded.levelFormat, delta.y));

	if (did_split_line >= 0)
		op.setMessage("split linedef #%d", did_split_line);
	else
	{
		selection_c list(inst.edit.mode);
		list.set(vertexID);
		op.setMessageForSelection("moved", list);
	}
}

void ObjectsModule::singleDrag(const Objid &obj, const v3double_t &delta) const
{
	if (inst.edit.mode != ObjType::vertices)
	{
		selection_c list(inst.edit.mode);
		list.set(obj.num);

		doc.objects.move(list, delta);
		return;
	}

	/* move a single vertex */
	singleDragVertex(inst, obj.num, delta.xy);
}


void ObjectsModule::transferThingProperties(EditOperation &op, int src_thing, int dest_thing) const
{
	const auto T = doc.things[src_thing];

	op.changeThing(dest_thing, Thing::F_TYPE,    T->type);
	op.changeThing(dest_thing, Thing::F_OPTIONS, T->options);
//	BA_ChangeTH(dest_thing, Thing::F_ANGLE,   T->angle);

	op.changeThing(dest_thing, Thing::F_TID,     T->tid);
	op.changeThing(dest_thing, Thing::F_SPECIAL, T->special);

	op.changeThing(dest_thing, Thing::F_ARG1, T->arg1);
	op.changeThing(dest_thing, Thing::F_ARG2, T->arg2);
	op.changeThing(dest_thing, Thing::F_ARG3, T->arg3);
	op.changeThing(dest_thing, Thing::F_ARG4, T->arg4);
	op.changeThing(dest_thing, Thing::F_ARG5, T->arg5);
}


void ObjectsModule::transferSectorProperties(EditOperation &op, int src_sec, int dest_sec) const
{
	const auto sector = doc.sectors[src_sec];

	op.changeSector(dest_sec, Sector::F_FLOORH,    sector->floorh);
	op.changeSector(dest_sec, Sector::F_FLOOR_TEX, sector->floor_tex);
	op.changeSector(dest_sec, Sector::F_CEILH,     sector->ceilh);
	op.changeSector(dest_sec, Sector::F_CEIL_TEX,  sector->ceil_tex);

	op.changeSector(dest_sec, Sector::F_LIGHT,  sector->light);
	op.changeSector(dest_sec, Sector::F_TYPE,   sector->type);
	op.changeSector(dest_sec, Sector::F_TAG,    sector->tag);
}


#define LINEDEF_FLAG_KEEP  (MLF_Blocking + MLF_TwoSided)

void ObjectsModule::transferLinedefProperties(EditOperation &op, int src_line, int dest_line, bool do_tex) const
{
	const auto L1 = doc.linedefs[src_line];
	const auto L2 = doc.linedefs[dest_line];

	// don't transfer certain flags
	int flags = doc.linedefs[dest_line]->flags;
	flags = (flags & LINEDEF_FLAG_KEEP) | (L1->flags & ~LINEDEF_FLAG_KEEP);

	// handle textures
	if (do_tex && doc.getRight(*L1) && doc.getRight(*L2))
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
		if (! doc.getLeft(*L1))
		{
			StringID tex = doc.getRight(*L1)->mid_tex;

			if (! doc.getLeft(*L2))
			{
				op.changeSidedef(L2->right, SideDef::F_MID_TEX, tex);
			}
			else
			{
				op.changeSidedef(L2->right, SideDef::F_LOWER_TEX, tex);
				op.changeSidedef(L2->right, SideDef::F_UPPER_TEX, tex);

				op.changeSidedef(L2->left,  SideDef::F_LOWER_TEX, tex);
				op.changeSidedef(L2->left,  SideDef::F_UPPER_TEX, tex);

				// this is debatable....   CONFIG ITEM?
				flags |= MLF_LowerUnpegged;
				flags |= MLF_UpperUnpegged;
			}
		}
		else if (! doc.getLeft(*L2))
		{
			/* pick which texture to copy */

			const Sector &front = doc.getSector(*doc.getRight(*L1));
			const Sector &back  = doc.getSector(*doc.getLeft(*L1));

			StringID f_l = doc.getRight(*L1)->lower_tex;
			StringID f_u = doc.getRight(*L1)->upper_tex;
			StringID b_l = doc.getLeft(*L1)->lower_tex;
			StringID b_u = doc.getLeft(*L1)->upper_tex;

			// ignore missing textures
			if (is_null_tex(BA_GetString(f_l))) f_l = StringID();
			if (is_null_tex(BA_GetString(f_u))) f_u = StringID();
			if (is_null_tex(BA_GetString(b_l))) b_l = StringID();
			if (is_null_tex(BA_GetString(b_u))) b_u = StringID();

			// try hard to find a usable texture
			StringID tex = StringID(-1);

				 if (front.floorh < back.floorh && f_l.hasContent()) tex = f_l;
			else if (front.floorh > back.floorh && b_l.hasContent()) tex = b_l;
			else if (front. ceilh > back. ceilh && f_u.hasContent()) tex = f_u;
			else if (front. ceilh < back. ceilh && b_u.hasContent()) tex = b_u;
			else if (f_l.hasContent()) tex = f_l;
			else if (b_l.hasContent()) tex = b_l;
			else if (f_u.hasContent()) tex = f_u;
			else if (b_u.hasContent()) tex = b_u;

			if (tex.hasContent())
			{
				op.changeSidedef(L2->right, SideDef::F_MID_TEX, tex);
			}
		}
		else
		{
			const SideDef *RS = doc.getRight(*L1);
			const SideDef *LS = doc.getLeft(*L1);

			const Sector *F1 = &doc.getSector(*doc.getRight(*L1));
			const Sector *B1 = &doc.getSector(*doc.getLeft(*L1));
			const Sector *F2 = &doc.getSector(*doc.getRight(*L2));
			const Sector *B2 = &doc.getSector(*doc.getLeft(*L2));

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

			op.changeSidedef(L2->right, SideDef::F_LOWER_TEX, RS->lower_tex);
			op.changeSidedef(L2->right, SideDef::F_MID_TEX,   RS->mid_tex);
			op.changeSidedef(L2->right, SideDef::F_UPPER_TEX, RS->upper_tex);

			op.changeSidedef(L2->left, SideDef::F_LOWER_TEX, LS->lower_tex);
			op.changeSidedef(L2->left, SideDef::F_MID_TEX,   LS->mid_tex);
			op.changeSidedef(L2->left, SideDef::F_UPPER_TEX, LS->upper_tex);
		}
	}

	op.changeLinedef(dest_line, LineDef::F_FLAGS, flags);

	op.changeLinedef(dest_line, LineDef::F_TYPE, L1->type);
	op.changeLinedef(dest_line, LineDef::F_TAG,  L1->tag);

	op.changeLinedef(dest_line, LineDef::F_ARG2, L1->arg2);
	op.changeLinedef(dest_line, LineDef::F_ARG3, L1->arg3);
	op.changeLinedef(dest_line, LineDef::F_ARG4, L1->arg4);
	op.changeLinedef(dest_line, LineDef::F_ARG5, L1->arg5);
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

		EditOperation op(level.basis);
		op.setMessage("copied properties");

		switch (edit.mode)
		{
			case ObjType::sectors:
				level.objects.transferSectorProperties(op, source, target);
				break;

			case ObjType::things:
				level.objects.transferThingProperties(op, source, target);
				break;

			case ObjType::linedefs:
				level.objects.transferLinedefProperties(op, source, target, true /* do_tex */);
				break;

			default: break;
		}
	}
	else  /* reverse mode, HILITE --> SEL */
	{
		if (edit.Selected->count_obj() == 1 && edit.Selected->find_first() == edit.highlight.num)
		{
			Beep("No selection for CopyProperties");
			return;
		}

		int source = edit.highlight.num;

		EditOperation op(level.basis);
		op.setMessage("copied properties");

		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			if (*it == source)
				continue;

			switch (edit.mode)
			{
				case ObjType::sectors:
					level.objects.transferSectorProperties(op, source, *it);
					break;

				case ObjType::things:
					level.objects.transferThingProperties(op, source, *it);
					break;

				case ObjType::linedefs:
					level.objects.transferLinedefProperties(op, source, *it, true /* do_tex */);
					break;

				default: break;
			}
		}
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
				const auto L = doc.linedefs[n];

				if (! doc.touchesSector(*L, objnum))
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

	for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
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
				const auto L = doc.linedefs[objnum];

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
				const auto L = doc.linedefs[n];

				if (! doc.touchesSector(*L, objnum))
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
v2double_t ObjectsModule::getDragFocus(const v2double_t &ptr) const
{
	v2double_t result = {};

	// determine whether a majority of the object(s) are already on
	// the grid.  If they are, then pick a coordinate that also lies
	// on the grid.
	bool only_grid = false;

	int count = 0;
	int total = 0;

	if (inst.grid.snaps())
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
		dragUpdateCurrentDist(inst.edit.mode, inst.edit.dragged.num, &result.x, &result.y, &best_dist,
							   ptr.x, ptr.y, only_grid);
		return result;
	}

	for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
	{
		dragUpdateCurrentDist(inst.edit.mode, *it, &result.x, &result.y, &best_dist,
							   ptr.x, ptr.y, only_grid);
	}
	return result;
}


//------------------------------------------------------------------------


void transform_t::Clear()
{
	mid = {};
	scale = { 1.0, 1.0 };
	skew = {};
	rotate = 0;
}


void transform_t::Apply(double *x, double *y) const
{
	double x0 = *x - mid.x;
	double y0 = *y - mid.y;

	if (rotate)
	{
		double s = sin(rotate);
		double c = cos(rotate);

		double x1 = x0;
		double y1 = y0;

		x0 = x1 * c - y1 * s;
		y0 = y1 * c + x1 * s;
	}

	if (skew.nonzero())
	{
		double x1 = x0;
		double y1 = y0;

		x0 = x1 + y1 * skew.x;
		y0 = y1 + x1 * skew.y;
	}

	*x = mid.x + x0 * scale.x;
	*y = mid.y + y0 * scale.y;
}


//
// Return the coordinate of the centre of a group of objects.
//
// This is computed using an average of all the coordinates, which can
// often give a different result than using the middle of the bounding
// box.
//
v2double_t ObjectsModule::calcMiddle(const selection_c & list) const
{
	v2double_t result = {};

	if (list.empty())
		return result;

	double sum_x = 0;
	double sum_y = 0;

	int count = 0;

	switch (list.what_type())
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
			ConvertSelection(doc, list, verts);

			return calcMiddle(verts);
		}
	}

	SYS_ASSERT(count > 0);

	result.x = sum_x / count;
	result.y = sum_y / count;
	return result;
}


//
// returns a bounding box that completely includes a list of objects.
// when the list is empty, bottom-left coordinate is arbitrary.
//
void ObjectsModule::calcBBox(const selection_c & list, v2double_t &pos1, v2double_t &pos2) const
{
	if (list.empty())
	{
		pos1 = {};
		pos2 = {};
		return;
	}

	pos1 = { +9e9, +9e9 };
	pos2 = { -9e9, -9e9 };

	switch (list.what_type())
	{
		case ObjType::things:
		{
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const auto T = doc.things[*it];
				double Tx = T->x();
				double Ty = T->y();

				const thingtype_t &info = inst.conf.getThingType(T->type);
				int r = info.radius;

				if (Tx - r < pos1.x) pos1.x = Tx - r;
				if (Ty - r < pos1.y) pos1.y = Ty - r;
				if (Tx + r > pos2.x) pos2.x = Tx + r;
				if (Ty + r > pos2.y) pos2.y = Ty + r;
			}
			break;
		}

		case ObjType::vertices:
		{
			for (sel_iter_c it(list) ; !it.done() ; it.next())
			{
				const auto V = doc.vertices[*it];
				double Vx = V->x();
				double Vy = V->y();

				if (Vx < pos1.x) pos1.x = Vx;
				if (Vy < pos1.y) pos1.y = Vy;
				if (Vx > pos2.x) pos2.x = Vx;
				if (Vy > pos2.y) pos2.y = Vy;
			}
			break;
		}

		// everything else: just use the vertices
		default:
		{
			selection_c verts(ObjType::vertices);
			ConvertSelection(doc, list, verts);

			calcBBox(verts, pos1, pos2);
			return;
		}
	}

	SYS_ASSERT(pos1.x <= pos2.x);
	SYS_ASSERT(pos1.y <= pos2.y);
}


void ObjectsModule::doMirrorThings(EditOperation &op, const selection_c &list, bool is_vert, double mid_x, double mid_y) const
{
	FFixedPoint fix_mx = MakeValidCoord(inst.loaded.levelFormat, mid_x);
	FFixedPoint fix_my = MakeValidCoord(inst.loaded.levelFormat, mid_y);

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const auto T = doc.things[*it];

		if (is_vert)
		{
			op.changeThing(*it, Thing::F_Y, fix_my * 2 - T->raw_y);

			if (T->angle != 0)
				op.changeThing(*it, Thing::F_ANGLE, 360 - T->angle);
		}
		else
		{
			op.changeThing(*it, Thing::F_X, fix_mx * 2 - T->raw_x);

			if (T->angle > 180)
				op.changeThing(*it, Thing::F_ANGLE, 540 - T->angle);
			else
				op.changeThing(*it, Thing::F_ANGLE, 180 - T->angle);
		}
	}
}


void ObjectsModule::doMirrorVertices(EditOperation &op, const selection_c &list, bool is_vert, double mid_x, double mid_y) const
{
	FFixedPoint fix_mx = MakeValidCoord(inst.loaded.levelFormat, mid_x);
	FFixedPoint fix_my = MakeValidCoord(inst.loaded.levelFormat, mid_y);

	selection_c verts(ObjType::vertices);
	ConvertSelection(doc, list, verts);

	for (sel_iter_c it(verts) ; !it.done() ; it.next())
	{
		const auto V = doc.vertices[*it];

		if (is_vert)
			op.changeVertex(*it, Vertex::F_Y, fix_my * 2 - V->raw_y);
		else
			op.changeVertex(*it, Vertex::F_X, fix_mx * 2 - V->raw_x);
	}

	// flip linedefs too !!
	selection_c lines(ObjType::linedefs);
	ConvertSelection(doc, verts, lines);

	for (sel_iter_c it(lines) ; !it.done() ; it.next())
	{
		const auto L = doc.linedefs[*it];

		int start = L->start;
		int end   = L->end;

		op.changeLinedef(*it, LineDef::F_START, end);
		op.changeLinedef(*it, LineDef::F_END, start);
	}
}


void ObjectsModule::doMirrorStuff(EditOperation &op, const selection_c &list, bool is_vert, double mid_x, double mid_y) const
{
	if (inst.edit.mode == ObjType::things)
	{
		doMirrorThings(op, list, is_vert, mid_x, mid_y);
		return;
	}

	// everything else just modifies the vertices

	if (inst.edit.mode == ObjType::sectors)
	{
		// handle things in Sectors mode too
		selection_c things(ObjType::things);
		ConvertSelection(doc, list, things);

		doMirrorThings(op, things, is_vert, mid_x, mid_y);
	}

	doMirrorVertices(op, list, is_vert, mid_x, mid_y);
}


void Instance::CMD_Mirror()
{
	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No objects to mirror");
		return;
	}

	bool is_vert = false;

	if (tolower(EXEC_Param[0][0]) == 'v')
		is_vert = true;

	v2double_t mid = level.objects.calcMiddle(*edit.Selected);

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("mirrored", *edit.Selected, is_vert ? " vertically" : " horizontally");

		level.objects.doMirrorStuff(op, *edit.Selected, is_vert, mid.x, mid.y);

	}

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void ObjectsModule::doRotate90Things(EditOperation &op, const selection_c &list, bool anti_clockwise,
							 double mid_x, double mid_y) const
{
	FFixedPoint fix_mx = MakeValidCoord(inst.loaded.levelFormat, mid_x);
	FFixedPoint fix_my = MakeValidCoord(inst.loaded.levelFormat, mid_y);

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const auto T = doc.things[*it];

		FFixedPoint old_x = T->raw_x;
		FFixedPoint old_y = T->raw_y;

		if (anti_clockwise)
		{
			op.changeThing(*it, Thing::F_X, fix_mx - old_y + fix_my);
			op.changeThing(*it, Thing::F_Y, fix_my + old_x - fix_mx);

			op.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, +90));
		}
		else
		{
			op.changeThing(*it, Thing::F_X, fix_mx + old_y - fix_my);
			op.changeThing(*it, Thing::F_Y, fix_my - old_x + fix_mx);

			op.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, -90));
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

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No objects to rotate");
		return;
	}

	v2double_t mid = level.objects.calcMiddle(*edit.Selected);

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("rotated", *edit.Selected, anti_clockwise ? " anti-clockwise" : " clockwise");

		if (edit.mode == ObjType::things)
		{
			level.objects.doRotate90Things(op, *edit.Selected, anti_clockwise, mid.x, mid.y);
		}
		else
		{
			// handle things inside sectors
			if (edit.mode == ObjType::sectors)
			{
				selection_c things(ObjType::things);
				ConvertSelection(level, *edit.Selected, things);

				level.objects.doRotate90Things(op, things, anti_clockwise, mid.x, mid.y);
			}

			// everything else just rotates the vertices
			selection_c verts(ObjType::vertices);
			ConvertSelection(level, *edit.Selected, verts);

			FFixedPoint fix_mx = MakeValidCoord(loaded.levelFormat, mid.x);
			FFixedPoint fix_my = MakeValidCoord(loaded.levelFormat, mid.y);

			for (sel_iter_c it(verts) ; !it.done() ; it.next())
			{
				const auto V = level.vertices[*it];

				FFixedPoint old_x = V->raw_x;
				FFixedPoint old_y = V->raw_y;

				if (anti_clockwise)
				{
					op.changeVertex(*it, Vertex::F_X, fix_mx - old_y + fix_my);
					op.changeVertex(*it, Vertex::F_Y, fix_my + old_x - fix_mx);
				}
				else
				{
					op.changeVertex(*it, Vertex::F_X, fix_mx + old_y - fix_my);
					op.changeVertex(*it, Vertex::F_Y, fix_my - old_x + fix_mx);
				}
			}
		}
	}

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void ObjectsModule::doScaleTwoThings(EditOperation &op, const selection_c &list, transform_t& param) const
{
	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const auto T = doc.things[*it];

		double new_x = T->x();
		double new_y = T->y();

		param.Apply(&new_x, &new_y);

		op.changeThing(*it, Thing::F_X, MakeValidCoord(inst.loaded.levelFormat, new_x));
		op.changeThing(*it, Thing::F_Y, MakeValidCoord(inst.loaded.levelFormat, new_y));

		float rot1 = static_cast<float>(param.rotate / (M_PI / 4));

		int ang_diff = static_cast<int>(roundf(rot1) * 45.0);

		if (ang_diff)
		{
			op.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, ang_diff));
		}
	}
}


void ObjectsModule::doScaleTwoVertices(EditOperation &op, const selection_c &list, transform_t& param) const
{
	selection_c verts(ObjType::vertices);
	ConvertSelection(doc, list, verts);

	for (sel_iter_c it(verts) ; !it.done() ; it.next())
	{
		const auto V = doc.vertices[*it];

		double new_x = V->x();
		double new_y = V->y();

		param.Apply(&new_x, &new_y);

		op.changeVertex(*it, Vertex::F_X, MakeValidCoord(inst.loaded.levelFormat, new_x));
		op.changeVertex(*it, Vertex::F_Y, MakeValidCoord(inst.loaded.levelFormat, new_y));
	}
}


void ObjectsModule::doScaleTwoStuff(EditOperation &op, const selection_c &list, transform_t& param) const
{
	if (inst.edit.mode == ObjType::things)
	{
		doScaleTwoThings(op, list, param);
		return;
	}

	// everything else just modifies the vertices

	if (inst.edit.mode == ObjType::sectors)
	{
		// handle things in Sectors mode too
		selection_c things(ObjType::things);
		ConvertSelection(doc, list, things);

		doScaleTwoThings(op, things, param);
	}

	doScaleTwoVertices(op, list, param);
}


void ObjectsModule::transform(transform_t& param) const
{
	// this is called by the MOUSE2 dynamic scaling code

	SYS_ASSERT(inst.edit.Selected->notempty());

	EditOperation op(doc.basis);
	op.setMessageForSelection("scaled", *inst.edit.Selected);

	if (param.scale.x < 0)
	{
		param.scale.x = -param.scale.x;
		doMirrorStuff(op, *inst.edit.Selected, false /* is_vert */, param.mid.x, param.mid.y);
	}

	if (param.scale.y < 0)
	{
		param.scale.y = -param.scale.y;
		doMirrorStuff(op, *inst.edit.Selected, true /* is_vert */, param.mid.x, param.mid.y);
	}

	doScaleTwoStuff(op, *inst.edit.Selected, param);
}


void ObjectsModule::determineOrigin(transform_t& param, double pos_x, double pos_y) const
{
	if (pos_x == 0 && pos_y == 0)
	{
		param.mid = doc.objects.calcMiddle(*inst.edit.Selected);
		return;
	}

	v2double_t lpos, hpos;

	doc.objects.calcBBox(*inst.edit.Selected, lpos, hpos);

	if (pos_x < 0)
		param.mid.x = lpos.x;
	else if (pos_x > 0)
		param.mid.x = hpos.x;
	else
		param.mid.x = lpos.x + (hpos.x - lpos.x) / 2;

	if (pos_y < 0)
		param.mid.y = lpos.y;
	else if (pos_y > 0)
		param.mid.y = hpos.y;
	else
		param.mid.y = lpos.y + (hpos.y - lpos.y) / 2;
}


void ObjectsModule::scale3(double scale_x, double scale_y, double pos_x, double pos_y) const
{
	SYS_ASSERT(scale_x > 0);
	SYS_ASSERT(scale_y > 0);

	transform_t param;

	param.Clear();

	param.scale.x = scale_x;
	param.scale.y = scale_y;

	determineOrigin(param, pos_x, pos_y);

	EditOperation op(doc.basis);
	op.setMessageForSelection("scaled", *inst.edit.Selected);
	{
		doScaleTwoStuff(op, *inst.edit.Selected, param);
	}
}


void ObjectsModule::doScaleSectorHeights(EditOperation &op, const selection_c &list, double scale_z, int pos_z) const
{
	SYS_ASSERT(! list.empty());

	// determine Z range and origin
	int lz = +99999;
	int hz = -99999;

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const auto S = doc.sectors[*it];

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
		const auto S = doc.sectors[*it];

		int new_f = mid_z + iround((S->floorh - mid_z) * scale_z);
		int new_c = mid_z + iround((S-> ceilh - mid_z) * scale_z);

		op.changeSector(*it, Sector::F_FLOORH, new_f);
		op.changeSector(*it, Sector::F_CEILH,  new_c);
	}
}

void ObjectsModule::scale4(double scale_x, double scale_y, double scale_z,
                   double pos_x, double pos_y, double pos_z) const
{
	SYS_ASSERT(inst.edit.mode == ObjType::sectors);

	transform_t param;

	param.Clear();

	param.scale.x = scale_x;
	param.scale.y = scale_y;

	determineOrigin(param, pos_x, pos_y);

	EditOperation op(doc.basis);
	op.setMessageForSelection("scaled", *inst.edit.Selected);
	{
		doScaleTwoStuff(op, *inst.edit.Selected, param);
		doScaleSectorHeights(op, *inst.edit.Selected, scale_z, static_cast<int>(pos_z));
	}
}


void ObjectsModule::rotate3(double deg, double pos_x, double pos_y) const
{
	transform_t param;

	param.Clear();

	param.rotate = deg * M_PI / 180.0;

	determineOrigin(param, pos_x, pos_y);

	EditOperation op(doc.basis);
	op.setMessageForSelection("rotated", *inst.edit.Selected);
	{
		doScaleTwoStuff(op, *inst.edit.Selected, param);
	}
}


bool ObjectsModule::spotInUse(ObjType obj_type, int x, int y) const
{
	switch (obj_type)
	{
		case ObjType::things:
			for (const auto &thing : doc.things)
				if (iround(thing->x()) == x && iround(thing->y()) == y)
					return true;
			return false;

		case ObjType::vertices:
			for (const auto &vertex : doc.vertices)
				if (iround(vertex->x()) == x && iround(vertex->y()) == y)
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

	param.scale = { mul, mul };


	SelectHighlight unselect = inst.edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("No objects to %s", do_shrink ? "shrink" : "enlarge");
		return;
	}

	// TODO: CONFIG ITEM (or FLAG)
	if ((true))
	{
		param.mid = calcMiddle(*inst.edit.Selected);
	}
	else
	{
		v2double_t lpos, hpos;
		calcBBox(*inst.edit.Selected, lpos, hpos);

		param.mid = lpos + (hpos - lpos) / 2;
	}

	{
		EditOperation op(doc.basis);
		op.setMessageForSelection(do_shrink ? "shrunk" : "enlarged", *inst.edit.Selected);

		doScaleTwoStuff(op, *inst.edit.Selected, param);
	}

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


void ObjectsModule::quantizeThings(EditOperation &op, selection_c &list) const
{
	// remember the things which we moved
	// (since we cannot modify the selection while we iterate over it)
	selection_c moved(list.what_type());

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const auto T = doc.things[*it];

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
				op.changeThing(*it, Thing::F_X, MakeValidCoord(inst.loaded.levelFormat, new_x));
				op.changeThing(*it, Thing::F_Y, MakeValidCoord(inst.loaded.levelFormat, new_y));

				moved.set(*it);
				break;
			}
		}
	}

	list.unmerge(moved);

	if (! list.empty())
		inst.Beep("Quantize: could not move %d things", list.count_obj());
}


void ObjectsModule::quantizeVertices(EditOperation &op, selection_c &list) const
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

	for (const auto &L : doc.linedefs)
	{
		// require both vertices of the linedef to be in the selection
		if (! (list.get(L->start) && list.get(L->end)))
			continue;

		// IDEA: make this a method of LineDef
		double x1 = doc.getStart(*L).x();
		double y1 = doc.getStart(*L).y();
		double x2 = doc.getEnd(*L).x();
		double y2 = doc.getEnd(*L).y();

		if (doc.isHorizontal(*L))
		{
			vert_modes[L->start] |= V_HORIZ;
			vert_modes[L->end]   |= V_HORIZ;
		}
		else if (doc.isVertical(*L))
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

	for (sel_iter_c it(list) ; !it.done() ; it.next())
	{
		const auto V = doc.vertices[*it];

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
				op.changeVertex(*it, Vertex::F_X, MakeValidCoord(inst.loaded.levelFormat, new_x));
				op.changeVertex(*it, Vertex::F_Y, MakeValidCoord(inst.loaded.levelFormat, new_y));

				moved.set(*it);
				break;
			}
		}
	}

	delete[] vert_modes;

	list.unmerge(moved);

	if (list.notempty())
		inst.Beep("Quantize: could not move %d vertices", list.count_obj());
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

		edit.Selection_AddHighlighted();
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("quantized", *edit.Selected);

		switch (edit.mode)
		{
			case ObjType::things:
				level.objects.quantizeThings(op, *edit.Selected);
				break;

			case ObjType::vertices:
				level.objects.quantizeVertices(op, *edit.Selected);
				break;

			// everything else merely quantizes vertices
			default:
			{
				selection_c verts(ObjType::vertices);
				ConvertSelection(level, *edit.Selected, verts);

				level.objects.quantizeVertices(op, verts);

				Selection_Clear();
				break;
			}
		}
	}

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
