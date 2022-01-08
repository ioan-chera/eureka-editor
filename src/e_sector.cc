//------------------------------------------------------------------------
//  SECTOR STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2017 Andrew Apted
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

#include <map>

#include "m_bitvec.h"
#include "w_rawdef.h"
#include "e_cutpaste.h"
#include "e_hover.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_objects.h"
#include "e_sector.h"
#include "e_vertex.h"

#include "ui_window.h"


// this function ensures that sector won't get floor > ceil
void SectorModule::safeRaiseLower(EditOperation &op, int sec, int parts, int dz) const
{
	if (parts == 0)
		parts = PART_FLOOR | PART_CEIL;

	int f = doc.sectors[sec]->floorh;
	int c = doc.sectors[sec]->ceilh;

	if ((parts & PART_FLOOR) != 0 && (parts & PART_CEIL) != 0)
	{
		f += dz;
		c += dz;

		// this won't usually happen, only if original sector was bad
		if (f > c)
			f = c;
	}
	else if (parts & PART_FLOOR)
	{
		f += dz;

		if (f > c)
			f = c;
	}
	else if (parts & PART_CEIL)
	{
		c += dz;

		if (c < f)
			c = f;
	}

	if (parts & PART_FLOOR)
		op.changeSector(sec, Sector::F_FLOORH, f);

	if (parts & PART_CEIL)
		op.changeSector(sec, Sector::F_CEILH, c);
}


void Instance::CMD_SEC_Floor()
{
	int diff = atoi(EXEC_Param[0]);

	if (diff == 0)
	{
		Beep("SEC_Floor: bad parameter '%s'", EXEC_Param[0].c_str());
		return;
	}

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No sectors to move");
		return;
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection(diff < 0 ? "lowered floor of" : "raised floor of", *edit.Selected);

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			const Sector *S = level.sectors[*it];

			int new_h = CLAMP(-32767, S->floorh + diff, S->ceilh);

			op.changeSector(*it, Sector::F_FLOORH, new_h);
		}
	}

	main_win->sec_box->UpdateField(Sector::F_FLOORH);

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void Instance::CMD_SEC_Ceil()
{
	int diff = atoi(EXEC_Param[0]);

	if (diff == 0)
	{
		Beep("SEC_Ceil: bad parameter '%s'", EXEC_Param[0].c_str());
		return;
	}

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No sectors to move");
		return;
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection(diff < 0 ? "lowered ceil of" : "raised ceil of", *edit.Selected);

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			const Sector *S = level.sectors[*it];

			int new_h = CLAMP(S->floorh, S->ceilh + diff, 32767);

			op.changeSector(*it, Sector::F_CEILH, new_h);
		}
	}

	main_win->sec_box->UpdateField(Sector::F_CEILH);

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


static int light_add_delta(int level, int delta)
{
	// NOTE: delta is assumed to be a power of two

	if (abs(delta) <= 1)
	{
		level += delta;
	}
	else if (delta > 0)
	{
		level = (level | (delta-1)) + 1;
	}
	else
	{
		level = (level - 1) & ~(abs(delta)-1);
	}

	return CLAMP(0, level, 255);
}


void SectorModule::sectorsAdjustLight(int delta) const
{
	// this uses the current selection (caller must set it up)

	if (inst.edit.Selected->empty())
		return;

	{
		EditOperation op(doc.basis);
		op.setMessageForSelection(delta < 0 ? "darkened" : "brightened", *inst.edit.Selected);

		for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
		{
			const Sector *S = doc.sectors[*it];

			int new_lt = light_add_delta(S->light, delta);

			op.changeSector(*it, Sector::F_LIGHT, new_lt);
		}
	}

	inst.main_win->sec_box->UpdateField(Sector::F_LIGHT);
}


void Instance::CMD_SEC_Light()
{
	int diff = atoi(EXEC_Param[0]);

	if (diff == 0)
	{
		Beep("SEC_Light: bad parameter '%s'", EXEC_Param[0].c_str());
		return;
	}

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No sectors to adjust light");
		return;
	}

	level.secmod.sectorsAdjustLight(diff);

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void Instance::CMD_SEC_SwapFlats()
{
	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No sectors to swap");
		return;
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("swapped flats in", *edit.Selected);

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			const Sector *S = level.sectors[*it];

			int floor_tex = S->floor_tex;
			int  ceil_tex = S->ceil_tex;

			op.changeSector(*it, Sector::F_FLOOR_TEX, ceil_tex);
			op.changeSector(*it, Sector::F_CEIL_TEX, floor_tex);
		}
	}

	main_win->sec_box->UpdateField(Sector::F_FLOOR_TEX);
	main_win->sec_box->UpdateField(Sector::F_CEIL_TEX);

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void SectorModule::linedefsBetweenSectors(selection_c *list, int sec1, int sec2) const
{
	for (int i = 0 ; i < doc.numLinedefs() ; i++)
	{
		const LineDef * L = doc.linedefs[i];

		if (! (L->Left(doc) && L->Right(doc)))
			continue;

		if ((L->Left(doc)->sector == sec1 && L->Right(doc)->sector == sec2) ||
		    (L->Left(doc)->sector == sec2 && L->Right(doc)->sector == sec1))
		{
			list->set(i);
		}
	}
}


void SectorModule::replaceSectorRefs(EditOperation &op, int old_sec, int new_sec) const
{
	for (int i = 0 ; i < doc.numSidedefs() ; i++)
	{
		SideDef * sd = doc.sidedefs[i];

		if (sd->sector == old_sec)
		{
			op.changeSidedef(i, SideDef::F_SECTOR, new_sec);
		}
	}
}


void Instance::commandSectorMerge()
{
	// need a selection
	if (edit.Selected->count_obj() == 1 && edit.highlight.valid())
	{
		edit.Selection_AddHighlighted();
	}

	if (edit.Selected->count_obj() < 2)
	{
		Beep("Need 2 or more sectors to merge");
		return;
	}

	int first = edit.Selected->find_first();

	bool keep_common_lines = Exec_HasFlag("/keep");

	// we require the *lowest* numbered sector, otherwise we can
	// select the wrong sector afterwards (due to renumbering).
	int new_sec = edit.Selected->max_obj();

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		new_sec = std::min(new_sec, *it);
	}

	selection_c common_lines(ObjType::linedefs);
	selection_c unused_secs (ObjType::sectors);

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("merged", *edit.Selected);

		// keep the properties of the first selected sector
		if (new_sec != first)
		{
			const Sector *ref = level.sectors[first];

			op.changeSector(new_sec, Sector::F_FLOORH,    ref->floorh);
			op.changeSector(new_sec, Sector::F_FLOOR_TEX, ref->floor_tex);
			op.changeSector(new_sec, Sector::F_CEILH,     ref->ceilh);
			op.changeSector(new_sec, Sector::F_CEIL_TEX,  ref->ceil_tex);

			op.changeSector(new_sec, Sector::F_LIGHT, ref->light);
			op.changeSector(new_sec, Sector::F_TYPE,  ref->type);
			op.changeSector(new_sec, Sector::F_TAG,   ref->tag);
		}

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			int old_sec = *it;

			if (old_sec == new_sec)
				continue;

			level.secmod.linedefsBetweenSectors(&common_lines, old_sec, new_sec);

			level.secmod.replaceSectorRefs(op, old_sec, new_sec);

			unused_secs.set(old_sec);
		}

		level.objects.del(op, unused_secs);

		if (! keep_common_lines)
		{
			DeleteObjects_WithUnused(op, level, common_lines, false, false, false);
		}

	}

	// re-select the final sector
	Selection_Clear(true /* no_save */);

	edit.Selected->set(new_sec);
}


//------------------------------------------------------------------------


// #define DEBUG_LINELOOP  1

void lineloop_c::clear()
{
	lines.clear();
	sides.clear();

	faces_outward = false;

	for (unsigned int i = 0 ; i < islands.size() ; i++)
		delete islands[i];

	islands.clear();
}


void lineloop_c::push_back(int ld, Side side)
{
	lines.push_back(ld);
	sides.push_back(side);
}


bool lineloop_c::get(int ld, Side side) const
{
	for (unsigned int k = 0 ; k < lines.size() ; k++)
		if (lines[k] == ld && sides[k] == side)
			return true;

	for (unsigned int i = 0 ; i < islands.size() ; i++)
		if (islands[i]->get(ld, side))
			return true;

	return false;
}


bool lineloop_c::get_just_line(int ld) const
{
	for (unsigned int k = 0 ; k < lines.size() ; k++)
		if (lines[k] == ld)
			return true;

	for (unsigned int i = 0 ; i < islands.size() ; i++)
		if (islands[i]->get_just_line(ld))
			return true;

	return false;
}


double lineloop_c::TotalLength() const
{
	// NOTE: does NOT include islands

	double result = 0;

	for (unsigned int k = 0 ; k < lines.size() ; k++)
	{
		const LineDef *L = doc.linedefs[lines[k]];

		result += L->CalcLength(doc);
	}

	return result;
}


bool lineloop_c::SameSector(int *sec_num) const
{
	// NOTE: does NOT include islands

	SYS_ASSERT(lines.size() > 0);

	int sec = doc.linedefs[lines[0]]->WhatSector(sides[0], doc);

	for (unsigned int k = 0 ; k < lines.size() ; k++)
	{
		if (sec != doc.linedefs[lines[k]]->WhatSector(sides[k], doc))
			return false;
	}

	if (sec_num)
		*sec_num = sec;

	return true;
}


bool lineloop_c::AllBare() const
{
	int sec_num;

	if (! SameSector(&sec_num))
		return false;

	return (sec_num < 0);
}


int lineloop_c::NeighboringSector() const
{
	// pick the longest linedef with a sector on the other side

	// NOTE: it does not make sense to handle islands here.

	int best = -1;
	double best_len = -1;

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		const LineDef *L = doc.linedefs[lines[i]];

		// we assume here that SIDE_RIGHT == 0 - SIDE_LEFT
		int sec = doc.linedefs[lines[i]]->WhatSector(- sides[i], doc);

		if (sec < 0)
			continue;

		double len = L->CalcLength(doc);

		if (len > best_len)
		{
			best = sec;
			best_len = len;
		}
	}

	return best;
}


int lineloop_c::IslandSector() const
{
	// test is only valid for islands
	if (! faces_outward)
		return -1;

	// we might need to check multiple lines, as the first line could
	// be facing a linedef which is ALSO part of the island.

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		Side opp_side;
		int opp_ld = doc.hover.getOppositeLinedef(lines[i], sides[i], &opp_side, nullptr);

		// can see "the void" ?
		// this means the geometry around here is broken, but for
		// the usages of this method we can ignore it.
		if (opp_ld < 0)
			continue;

		// part of the island itself?
		if (get_just_line(opp_ld))
			continue;

		return doc.linedefs[opp_ld]->WhatSector(opp_side, doc);
	}

	return -1;
}


int lineloop_c::DetermineSector() const
{
	if (faces_outward)
		return IslandSector();

	for (unsigned int k = 0 ; k < lines.size() ; k++)
	{
		int sec = doc.linedefs[lines[k]]->WhatSector(sides[k], doc);

		if (sec >= 0)
			return sec;
	}

	return -1;  // VOID
}


void lineloop_c::CalcBounds(double *x1, double *y1, double *x2, double *y2) const
{
	SYS_ASSERT(lines.size() > 0);

	*x1 = +9e9; *y1 = +9e9;
	*x2 = -9e9; *y2 = -9e9;

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		const LineDef *L = doc.linedefs[lines[i]];

		*x1 = std::min(*x1, std::min(L->Start(doc)->x(), L->End(doc)->x()));
		*y1 = std::min(*y1, std::min(L->Start(doc)->y(), L->End(doc)->y()));

		*x2 = std::max(*x2, std::max(L->Start(doc)->x(), L->End(doc)->x()));
		*y2 = std::max(*y2, std::max(L->Start(doc)->y(), L->End(doc)->y()));
	}
}


void lineloop_c::GetAllSectors(selection_c *list) const
{
	// assumes the given selection is empty (this adds to it)

	for (unsigned int k = 0 ; k < lines.size() ; k++)
	{
		int sec = doc.linedefs[lines[k]]->WhatSector(sides[k], doc);

		if (sec >= 0)
			list->set(sec);
	}

	for (unsigned int i = 0 ; i < islands.size() ; i++)
	{
		islands[i]->GetAllSectors(list);
	}
}


//
// Follows the path clockwise from the given start line, adding each
// line into the appropriate set.  Returns true if the path was closed,
// or false for failure (in which case the lineloop_c object will not
// be in a valid state).
//
// side is either SIDE_LEFT or SIDE_RIGHT.
//
// -AJA- 2001-05-09
//
bool SectorModule::traceLineLoop(int ld, Side side, lineloop_c& loop, bool ignore_bare) const
{
	int start_ld   = ld;
	Side start_side = side;

	loop.clear();

	int   cur_vert;
	int  prev_vert;

	if (side == Side::right)
	{
		cur_vert  = doc.linedefs[ld]->end;
		prev_vert = doc.linedefs[ld]->start;
	}
	else
	{
		cur_vert  = doc.linedefs[ld]->start;
		prev_vert = doc.linedefs[ld]->end;
	}

#ifdef DEBUG_LINELOOP
	gLog.debugPrintf("TRACE PATH: line:%d  side:%d  cur_vert:%d\n", ld, side, cur_vert);
#endif

	// check for an isolated line
	if (doc.vertmod.howManyLinedefs( cur_vert) == 1 &&
		doc.vertmod.howManyLinedefs(prev_vert) == 1)
		return false;

	// compute the average angle over all the lines
	double average_angle = 0;

	for (;;)
	{
		loop.push_back(ld, side);

		int next_line = -1;
		int next_vert = -1;
		Side next_side =  Side::neither;

		double best_angle = 9999;

		// look for the next linedef in the path, one using the
		// current vertex and having the smallest interior angle.
		// it *can* be the exact same linedef (when hitting a dangling
		// vertex).

		for (int n = 0 ; n < doc.numLinedefs() ; n++)
		{
			const LineDef * N = doc.linedefs[n];

			if (! N->TouchesVertex(cur_vert))
				continue;

			if (ignore_bare && !N->Left(doc) && !N->Right(doc))
				continue;

			int other_vert;
			Side which_side;

			if (N->start == cur_vert)
			{
				other_vert = N->end;
				which_side = Side::right;
			}
			else  /* (N->end == cur_vert) */
			{
				other_vert = N->start;
				which_side = Side::left;
			}

			double angle;

			if (n == ld)
				angle = 361.0;
			else
				angle = doc.linemod.angleBetweenLines(prev_vert, cur_vert, other_vert);

			if (next_line < 0 || angle < best_angle)
			{
				next_line = n;
				next_vert = other_vert;
				next_side = which_side;

				best_angle = angle;
			}
		}

#ifdef DEBUG_LINELOOP
		gLog.debugPrintf("PATH NEXT: line:%d  side:%d  vert:%d  angle:%1.6f\n",
				next_line, next_side, next_vert, best_angle);
#endif

		// no next line?  path cannot be closed
		if (next_line < 0)
			return false;

		// we have come back to the start, so terminate
		if (next_line == start_ld && next_side == start_side)
			break;

		// this won't happen under normal circumstances, but it *can*
		// happen and indicates a non-closed structure.
		if (loop.get(next_line, next_side))
			return false;

		// OK

		ld   = next_line;
		side = next_side;

		prev_vert = cur_vert;
		cur_vert  = next_vert;

		average_angle += best_angle;
	}

	// this might happen if there are overlapping linedefs
	if (loop.lines.size() < 3)
		return false;

	average_angle = average_angle / (double)loop.lines.size();

	loop.faces_outward = (average_angle >= 180.0);

#ifdef DEBUG_LINELOOP
	gLog.debugPrintf("PATH CLOSED!  average_angle:%1.2f\n", average_angle);
#endif

	return true;
}


bool lineloop_c::LookForIsland()
{
	// ALGORITHM:
	//    Iterate over all lines within the bounding box of the
	//    current path (but not *on* the current path).
	//
	//    Use OppositeLineDef() to find starting lines, and trace them.
	//    Tracing is necessary because in certain shapes the opposite
	//    lines will be part of the island too.
	//
	// Returns: true if found one, false otherwise.
	//

	// calc bounding box
	double bbox_x1, bbox_y1, bbox_x2, bbox_y2;
	CalcBounds(&bbox_x1, &bbox_y1, &bbox_x2, &bbox_y2);

	int count = 0;

	for (int ld = 0 ; ld < doc.numLinedefs() ; ld++)
	{
		const LineDef * L = doc.linedefs[ld];

		double x1 = L->Start(doc)->x();
		double y1 = L->Start(doc)->y();
		double x2 = L->End(doc)->x();
		double y2 = L->End(doc)->y();

		if (std::max(x1, x2) < bbox_x1 || std::min(x1, x2) > bbox_x2 ||
			std::max(y1, y2) < bbox_y1 || std::min(y1, y2) > bbox_y2)
			continue;

		// ouch, this is gonna be SLOW

		for (int where = 0 ; where < 2 ; where++)
		{
			Side ld_side = where ? Side::right : Side::left;

			Side opp_side;
			int opp = doc.hover.getOppositeLinedef(ld, ld_side, &opp_side, nullptr);

			if (opp < 0)
				continue;

			bool  ld_in_path = get(ld,   ld_side);
			bool opp_in_path = get(opp, opp_side);

			// need one in the current loop, and the other NOT in it
			if (ld_in_path == opp_in_path)
				continue;

#ifdef DEBUG_LINELOOP
gLog.debugPrintf("Found line:%d side:%d <--> opp:%d opp_side:%d  us:%d them:%d\n",
ld, ld_side, opp, opp_side, ld_in_path?1:0, opp_in_path?1:0);
#endif

			lineloop_c *island = new lineloop_c(doc);

			// treat isolated linedefs like islands
			if (! ld_in_path &&
				doc.vertmod.howManyLinedefs(doc.linedefs[ld]->start) == 1 &&
				doc.vertmod.howManyLinedefs(doc.linedefs[ld]->end)   == 1)
			{
				island->push_back(ld, Side::right);
				island->push_back(ld, Side::left);

				islands.push_back(island);
				count++;

				continue;
			}

			bool ok;

			if (ld_in_path)
				ok = doc.secmod.traceLineLoop(opp, opp_side, *island);
			else
				ok = doc.secmod.traceLineLoop(ld, ld_side, *island);

			if (ok && island->faces_outward)
			{
				islands.push_back(island);
				count++;
			}
			else
			{
				delete island;
			}
		}
	}

	return (count > 0);
}


void lineloop_c::FindIslands()
{
	// Look for "islands", closed linedef paths that lie completely
	// inside the area, i.e. not connected to the main path.
	//
	// Repeat these steps until no more islands are found.
	// (This is needed to handle e.g. a big room full of pillars,
	// since the pillars in the middle won't "see" the outer sector
	// until the neighboring pillars are added).
	//
	// Example: the two pillars at the start of MAP01 of DOOM 2.
	//

	// use a counter for safety
	for (int loop = 0 ; loop < 200 ; loop++)
	{
		if (! LookForIsland())
			break;
	}
}


void lineloop_c::Dump() const
{
	gLog.debugPrintf("Lineloop %p : %zu lines, %zu islands\n",
	            this, lines.size(), islands.size());

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		const LineDef *L = doc.linedefs[lines[i]];

		gLog.debugPrintf("  %s of line #%d : (%f %f) --> (%f %f)\n",
		            sides[i] == Side::left ? " LEFT" : "RIGHT",
					lines[i],
					L->Start(doc)->x(), L->Start(doc)->y(),
					L->End  (doc)->x(), L->End  (doc)->y());
	}
}


inline bool SectorModule::willBeTwoSided(int ld, Side side) const
{
	const LineDef *L = doc.linedefs[ld];

	if (L->WhatSideDef(side) < 0)
	{
		return (L->right >= 0) || (L->left >= 0);
	}

	return L->TwoSided();
}


void SectorModule::determineNewTextures(lineloop_c& loop,
								 std::vector<int>& lower_texs,
								 std::vector<int>& upper_texs) const
{
	unsigned int total = static_cast<unsigned>(loop.lines.size());

	SYS_ASSERT(lower_texs.size() == total);

	int null_tex  = BA_InternaliseString("-");

	int def_lower = BA_InternaliseString(inst.conf.default_wall_tex);
	int def_upper = def_lower;

	unsigned int k;
	unsigned int pass;

	// look for emergency fallback texture
	for (pass = 0 ; pass < 2 ; pass++)
	{
		for (k = 0 ; k < total ; k++)
		{
			int ld   = loop.lines[k];
			Side side = loop.sides[k];

			// check back sides in *first* pass
			// (to allow second pass to override)
			if (pass == 0)
				side = -side;

			int sd = doc.linedefs[ld]->WhatSideDef(side);
			if (sd < 0)
				continue;

			const SideDef *SD = doc.sidedefs[sd];

			if (doc.linedefs[ld]->TwoSided())
			{
				if (SD->lower_tex == null_tex) continue;
				if (SD->upper_tex == null_tex) continue;

				def_lower = SD->lower_tex;
				def_upper = SD->upper_tex;
			}
			else
			{
				if (SD->mid_tex == null_tex) continue;

				def_lower = SD->mid_tex;
				def_upper = SD->mid_tex;
			}

			// stop once we found something
			break;
		}
	}

	// reset "bare" lines to -1,
	// and grab the textures of other lines

	for (k = 0 ; k < total ; k++)
	{
		int ld = loop.lines[k];
		int sd = doc.linedefs[ld]->WhatSideDef(loop.sides[k]);

		if (sd < 0)
		{
			lower_texs[k] = upper_texs[k] = -1;
			continue;
		}

		const SideDef *SD = doc.sidedefs[sd];

		if (doc.linedefs[ld]->TwoSided())
		{
			lower_texs[k] = SD->lower_tex;
			upper_texs[k] = SD->upper_tex;
		}
		else
		{
			lower_texs[k] = upper_texs[k] = SD->mid_tex;
		}

		// prevent the "-" null texture
		if (lower_texs[k] == null_tex) lower_texs[k] = def_lower;
		if (upper_texs[k] == null_tex) upper_texs[k] = def_upper;
	}

	// transfer nearby textures to blank spots

	for (pass = 0 ; pass < total*2 ; pass++)
	{
		for (k = 0 ; k < total ; k++)
		{
			if (lower_texs[k] >= 0)
				continue;

			// next and previous line indices
			unsigned int p = (k > 0) ? (k - 1) : total - 1;
			unsigned int n = (k < total - 1) ? (k + 1) : 0;

			bool two_k = willBeTwoSided(loop.lines[k], loop.sides[k]);

			bool two_p = willBeTwoSided(loop.lines[p], loop.sides[p]);
			bool two_n = willBeTwoSided(loop.lines[n], loop.sides[n]);

			// prefer same sided-ness of lines
			if (pass < total)
			{
				if (two_p != two_k) p = total;
				if (two_n != two_k) n = total;
			}

			// disable p or n if there is no texture there yet
			if (p < total && lower_texs[p] < 0) p = total;
			if (n < total && lower_texs[n] < 0) n = total;

			if (p == total && n == total)
				continue;

			// if p and n both usable, p trumps n
			if (p == total)
				p = n;

			lower_texs[k] = lower_texs[p];
			upper_texs[k] = upper_texs[p];
		}
	}

	// lastly, ensure all textures are valid
	for (k = 0 ; k < total ; k++)
	{
		if (lower_texs[k] < 0) lower_texs[k] = def_lower;
		if (upper_texs[k] < 0) upper_texs[k] = def_upper;
	}
}


//
// update the side on a single linedef, using the given sector
// reference, and creating a new sidedef if necessary.
//
void SectorModule::doAssignSector(EditOperation &op, int ld, Side side, int new_sec,
						   int new_lower, int new_upper,
						   selection_c &flip) const
{
// gLog.debugPrintf("DoAssignSector %d ---> line #%d, side %d\n", new_sec, ld, side);
	const LineDef * L = doc.linedefs[ld];

	int sd_num   = (side == Side::right) ? L->right : L->left;
	int other_sd = (side == Side::right) ? L->left  : L->right;

	if (sd_num >= 0)
	{
		op.changeSidedef(sd_num, SideDef::F_SECTOR, new_sec);
		return;
	}

	// if we're adding a sidedef to a line that has no sides, and
	// the sidedef would be the 2nd one, then flip the linedef.
	// Thus we don't end up with invalid lines -- i.e. ones with a
	// left side but no right side.

	if (side == Side::left && other_sd < 0)
		flip.set(ld);
	else
		flip.clear(ld);

	SYS_ASSERT(new_lower >= 0);
	SYS_ASSERT(new_upper >= 0);

	// create new sidedef
	int new_sd = op.addNew(ObjType::sidedefs);

	SideDef * SD = doc.sidedefs[new_sd];

	if (other_sd >= 0)
	{
		// linedef will be two-sided
		SD->lower_tex = new_lower;
		SD->upper_tex = new_upper;
		SD->  mid_tex = BA_InternaliseString("-");
	}
	else
	{
		// linedef will be one-sided
		SD->lower_tex = new_lower;
		SD->upper_tex = new_lower;
		SD->  mid_tex = new_lower;
	}

	SD->sector = new_sec;

	if (side == Side::right)
		op.changeLinedef(ld, LineDef::F_RIGHT, new_sd);
	else
		op.changeLinedef(ld, LineDef::F_LEFT, new_sd);

	// if we're adding a second side to the linedef, clear out some
	// of the properties that aren't needed anymore: middle texture,
	// two-sided flag, and impassible flag.

	if (other_sd >= 0)
		doc.linemod.addSecondSidedef(op, ld, new_sd, other_sd);
}


void lineloop_c::AssignSector(EditOperation &op, int new_sec, selection_c &flip)
{
	std::vector<int> lower_texs(lines.size());
	std::vector<int> upper_texs(lines.size());

	doc.secmod.determineNewTextures(*this, lower_texs, upper_texs);

	for (unsigned int k = 0 ; k < lines.size() ; k++)
	{
		doc.secmod.doAssignSector(op, lines[k], sides[k], new_sec,
					   lower_texs[k], upper_texs[k], flip);
	}

	for (unsigned int i = 0 ; i < islands.size() ; i++)
	{
		islands[i]->AssignSector(op, new_sec, flip);
	}
}


bool SectorModule::getLoopForSpace(double map_x, double map_y, lineloop_c& loop) const
{
	selection_c seen_lines(ObjType::linedefs);

	int ld;
	Side side;

	ld = doc.hover.getClosestLine_CastingHoriz(map_x, map_y, &side);

	gLog.debugPrintf("GetLoopForSpace : hit line #%d, side %d\n", ld, (int)side);

	while (ld >= 0)
	{
		// never try this line again
		seen_lines.set(ld);

		if (! traceLineLoop(ld, side, loop))
		{
			gLog.debugPrintf("Area is not closed (tracing a loop failed)\n");
			return false;
		}

		// not an island?  GOOD!
		if (! loop.faces_outward)
		{
			return true;
		}

		gLog.debugPrintf("  hit island\n");

		// ensure we don't try any lines of this island
		unsigned int k;

		for (k = 0 ; k < loop.lines.size() ; k++)
			seen_lines.set(loop.lines[k]);

		// look for the surrounding line loop...
		ld = -1;

		for (k = 0 ; k < loop.lines.size() ; k++)
		{
			int new_ld;
			Side new_side;

			new_ld = doc.hover.getOppositeLinedef(loop.lines[k], loop.sides[k], &new_side, nullptr);

			if (new_ld < 0)
				continue;

			if (seen_lines.get(new_ld))
				continue;

			// try again...
			ld   = new_ld;
			side = new_side;

			gLog.debugPrintf("  trying again with line #%d, side %d\n", ld, (int)side);
			break;
		}
	}

	gLog.debugPrintf("Area is not closed (can see infinity)\n");
	return false;
}


//
// the "space" here really means a bunch of sidedefs that all face
// inward to the current area under the mouse cursor.
//
// the 'new_sec' can be < 0 to create a new sector.
//
// the 'model' is what properties to use for a new sector, < 0 means
// look for a neighboring sector to copy.
//
bool SectorModule::assignSectorToSpace(EditOperation &op, double map_x, double map_y, int new_sec, int model) const
{
	lineloop_c loop(doc);

	if (! getLoopForSpace(map_x, map_y, loop))
	{
		inst.Beep("Area is not closed");
		return false;
	}

	loop.FindIslands();

	if (new_sec < 0)
	{
		new_sec = op.addNew(ObjType::sectors);

		if (model < 0)
			model = loop.NeighboringSector();

		if (model < 0)
			doc.sectors[new_sec]->SetDefaults(inst);
		else
			*doc.sectors[new_sec] = *doc.sectors[model];
	}

	selection_c   flip(ObjType::linedefs);
	selection_c unused(ObjType::sectors);

	loop.GetAllSectors(&unused);

	loop.AssignSector(op, new_sec, flip);

	doc.linemod.flipLinedefGroup(op, &flip);

	// detect any sectors which have become unused, and delete them
	for (int n = 0 ; n < doc.numLinedefs() ; n++)
	{
		const LineDef *L = doc.linedefs[n];

		if (L->WhatSector(Side::left, doc) >= 0)
			unused.clear(L->WhatSector(Side::left, doc));
		if (L->WhatSector(Side::right, doc) >= 0)
			unused.clear(L->WhatSector(Side::right, doc));
	}

	doc.objects.del(op, unused);

	return true;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
