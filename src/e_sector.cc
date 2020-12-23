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

#include "Document.h"
#include "Errors.h"
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
void SEC_SafeRaiseLower(int sec, int parts, int dz)
{
	if (parts == 0)
		parts = PART_FLOOR | PART_CEIL;

	int f = gDocument.sectors[sec]->floorh;
	int c = gDocument.sectors[sec]->ceilh;

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
		gDocument.basis.changeSector(sec, Sector::F_FLOORH, f);

	if (parts & PART_CEIL)
		gDocument.basis.changeSector(sec, Sector::F_CEILH, c);
}


void CMD_SEC_Floor(void)
{
	int diff = atoi(EXEC_Param[0]);

	if (diff == 0)
	{
		Beep("SEC_Floor: bad parameter '%s'", EXEC_Param[0].c_str());
		return;
	}

	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No sectors to move");
		return;
	}

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection(diff < 0 ? "lowered floor of" : "raised floor of", *edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		const Sector *S = gDocument.sectors[*it];

		int new_h = CLAMP(-32767, S->floorh + diff, S->ceilh);

		gDocument.basis.changeSector(*it, Sector::F_FLOORH, new_h);
	}

	gDocument.basis.end();

	main_win->sec_box->UpdateField(Sector::F_FLOORH);

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void CMD_SEC_Ceil(void)
{
	int diff = atoi(EXEC_Param[0]);

	if (diff == 0)
	{
		Beep("SEC_Ceil: bad parameter '%s'", EXEC_Param[0].c_str());
		return;
	}

	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No sectors to move");
		return;
	}

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection(diff < 0 ? "lowered ceil of" : "raised ceil of", *edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		const Sector *S = gDocument.sectors[*it];

		int new_h = CLAMP(S->floorh, S->ceilh + diff, 32767);

		gDocument.basis.changeSector(*it, Sector::F_CEILH, new_h);
	}

	gDocument.basis.end();

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


void SectorsAdjustLight(int delta)
{
	// this uses the current selection (caller must set it up)

	if (edit.Selected->empty())
		return;

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection(delta < 0 ? "darkened" : "brightened", *edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		const Sector *S = gDocument.sectors[*it];

		int new_lt = light_add_delta(S->light, delta);

		gDocument.basis.changeSector(*it, Sector::F_LIGHT, new_lt);
	}

	gDocument.basis.end();

	main_win->sec_box->UpdateField(Sector::F_LIGHT);
}


void CMD_SEC_Light(void)
{
	int diff = atoi(EXEC_Param[0]);

	if (diff == 0)
	{
		Beep("SEC_Light: bad parameter '%s'", EXEC_Param[0].c_str());
		return;
	}

	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No sectors to adjust light");
		return;
	}

	SectorsAdjustLight(diff);

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void CMD_SEC_SwapFlats()
{
	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No sectors to swap");
		return;
	}

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection("swapped flats in", *edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		const Sector *S = gDocument.sectors[*it];

		int floor_tex = S->floor_tex;
		int  ceil_tex = S->ceil_tex;

		gDocument.basis.changeSector(*it, Sector::F_FLOOR_TEX, ceil_tex);
		gDocument.basis.changeSector(*it, Sector::F_CEIL_TEX, floor_tex);
	}

	gDocument.basis.end();

	main_win->sec_box->UpdateField(Sector::F_FLOOR_TEX);
	main_win->sec_box->UpdateField(Sector::F_CEIL_TEX);

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


static void LineDefsBetweenSectors(selection_c *list, int sec1, int sec2)
{
	for (int i = 0 ; i < NumLineDefs ; i++)
	{
		const LineDef * L = gDocument.linedefs[i];

		if (! (L->Left(gDocument) && L->Right(gDocument)))
			continue;

		if ((L->Left(gDocument)->sector == sec1 && L->Right(gDocument)->sector == sec2) ||
		    (L->Left(gDocument)->sector == sec2 && L->Right(gDocument)->sector == sec1))
		{
			list->set(i);
		}
	}
}


static void ReplaceSectorRefs(int old_sec, int new_sec)
{
	for (int i = 0 ; i < NumSideDefs ; i++)
	{
		SideDef * sd = gDocument.sidedefs[i];

		if (sd->sector == old_sec)
		{
			gDocument.basis.changeSidedef(i, SideDef::F_SECTOR, new_sec);
		}
	}
}


void CMD_SEC_Merge(void)
{
	// need a selection
	if (edit.Selected->count_obj() == 1 && edit.highlight.valid())
	{
		Selection_Add(edit.highlight);
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
		new_sec = MIN(new_sec, *it);
	}

	selection_c common_lines(ObjType::linedefs);
	selection_c unused_secs (ObjType::sectors);

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection("merged", *edit.Selected);

	// keep the properties of the first selected sector
	if (new_sec != first)
	{
		const Sector *ref = gDocument.sectors[first];

		gDocument.basis.changeSector(new_sec, Sector::F_FLOORH,    ref->floorh);
		gDocument.basis.changeSector(new_sec, Sector::F_FLOOR_TEX, ref->floor_tex);
		gDocument.basis.changeSector(new_sec, Sector::F_CEILH,     ref->ceilh);
		gDocument.basis.changeSector(new_sec, Sector::F_CEIL_TEX,  ref->ceil_tex);

		gDocument.basis.changeSector(new_sec, Sector::F_LIGHT, ref->light);
		gDocument.basis.changeSector(new_sec, Sector::F_TYPE,  ref->type);
		gDocument.basis.changeSector(new_sec, Sector::F_TAG,   ref->tag);
	}

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		int old_sec = *it;

		if (old_sec == new_sec)
			continue;

		LineDefsBetweenSectors(&common_lines, old_sec, new_sec);

		ReplaceSectorRefs(old_sec, new_sec);

		unused_secs.set(old_sec);
	}

	DeleteObjects(&unused_secs);

	if (! keep_common_lines)
	{
		DeleteObjects_WithUnused(&common_lines);
	}

	gDocument.basis.end();

	// re-select the final sector
	Selection_Clear(true /* no_save */);

	edit.Selected->set(new_sec);
}


//------------------------------------------------------------------------


// #define DEBUG_LINELOOP  1


lineloop_c::lineloop_c() :
	lines(), sides(),
	faces_outward(false),
	islands()
{ }


lineloop_c::~lineloop_c()
{
	clear();
}


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
		const LineDef *L = gDocument.linedefs[lines[k]];

		result += L->CalcLength(gDocument);
	}

	return result;
}


bool lineloop_c::SameSector(int *sec_num) const
{
	// NOTE: does NOT include islands

	SYS_ASSERT(lines.size() > 0);

	int sec = gDocument.linedefs[lines[0]]->WhatSector(sides[0], gDocument);

	for (unsigned int k = 0 ; k < lines.size() ; k++)
	{
		if (sec != gDocument.linedefs[lines[k]]->WhatSector(sides[k], gDocument))
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
		const LineDef *L = gDocument.linedefs[lines[i]];

		// we assume here that SIDE_RIGHT == 0 - SIDE_LEFT
		int sec = gDocument.linedefs[lines[i]]->WhatSector(- sides[i], gDocument);

		if (sec < 0)
			continue;

		double len = L->CalcLength(gDocument);

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
		int opp_ld = gDocument.hover.getOppositeLinedef(lines[i], sides[i], &opp_side, nullptr);

		// can see "the void" ?
		// this means the geometry around here is broken, but for
		// the usages of this method we can ignore it.
		if (opp_ld < 0)
			continue;

		// part of the island itself?
		if (get_just_line(opp_ld))
			continue;

		return gDocument.linedefs[opp_ld]->WhatSector(opp_side, gDocument);
	}

	return -1;
}


int lineloop_c::DetermineSector() const
{
	if (faces_outward)
		return IslandSector();

	for (unsigned int k = 0 ; k < lines.size() ; k++)
	{
		int sec = gDocument.linedefs[lines[k]]->WhatSector(sides[k], gDocument);

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
		const LineDef *L = gDocument.linedefs[lines[i]];

		*x1 = MIN(*x1, MIN(L->Start(gDocument)->x(), L->End(gDocument)->x()));
		*y1 = MIN(*y1, MIN(L->Start(gDocument)->y(), L->End(gDocument)->y()));

		*x2 = MAX(*x2, MAX(L->Start(gDocument)->x(), L->End(gDocument)->x()));
		*y2 = MAX(*y2, MAX(L->Start(gDocument)->y(), L->End(gDocument)->y()));
	}
}


void lineloop_c::GetAllSectors(selection_c *list) const
{
	// assumes the given selection is empty (this adds to it)

	for (unsigned int k = 0 ; k < lines.size() ; k++)
	{
		int sec = gDocument.linedefs[lines[k]]->WhatSector(sides[k], gDocument);

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
bool TraceLineLoop(int ld, Side side, lineloop_c& loop, bool ignore_bare)
{
	int start_ld   = ld;
	Side start_side = side;

	loop.clear();

	int   cur_vert;
	int  prev_vert;

	if (side == Side::right)
	{
		cur_vert  = gDocument.linedefs[ld]->end;
		prev_vert = gDocument.linedefs[ld]->start;
	}
	else
	{
		cur_vert  = gDocument.linedefs[ld]->start;
		prev_vert = gDocument.linedefs[ld]->end;
	}

#ifdef DEBUG_LINELOOP
	DebugPrintf("TRACE PATH: line:%d  side:%d  cur_vert:%d\n", ld, side, cur_vert);
#endif

	// check for an isolated line
	if (Vertex_HowManyLineDefs( cur_vert) == 1 &&
		Vertex_HowManyLineDefs(prev_vert) == 1)
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

		for (int n = 0 ; n < NumLineDefs ; n++)
		{
			const LineDef * N = gDocument.linedefs[n];

			if (! N->TouchesVertex(cur_vert))
				continue;

			if (ignore_bare && !N->Left(gDocument) && !N->Right(gDocument))
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
				angle = LD_AngleBetweenLines(prev_vert, cur_vert, other_vert);

			if (next_line < 0 || angle < best_angle)
			{
				next_line = n;
				next_vert = other_vert;
				next_side = which_side;

				best_angle = angle;
			}
		}

#ifdef DEBUG_LINELOOP
		DebugPrintf("PATH NEXT: line:%d  side:%d  vert:%d  angle:%1.6f\n",
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
	DebugPrintf("PATH CLOSED!  average_angle:%1.2f\n", average_angle);
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

	for (int ld = 0 ; ld < NumLineDefs ; ld++)
	{
		const LineDef * L = gDocument.linedefs[ld];

		double x1 = L->Start(gDocument)->x();
		double y1 = L->Start(gDocument)->y();
		double x2 = L->End(gDocument)->x();
		double y2 = L->End(gDocument)->y();

		if (MAX(x1, x2) < bbox_x1 || MIN(x1, x2) > bbox_x2 ||
		    MAX(y1, y2) < bbox_y1 || MIN(y1, y2) > bbox_y2)
			continue;

		// ouch, this is gonna be SLOW

		for (int where = 0 ; where < 2 ; where++)
		{
			Side ld_side = where ? Side::right : Side::left;

			Side opp_side;
			int opp = gDocument.hover.getOppositeLinedef(ld, ld_side, &opp_side, nullptr);

			if (opp < 0)
				continue;

			bool  ld_in_path = get(ld,   ld_side);
			bool opp_in_path = get(opp, opp_side);

			// need one in the current loop, and the other NOT in it
			if (ld_in_path == opp_in_path)
				continue;

#ifdef DEBUG_LINELOOP
DebugPrintf("Found line:%d side:%d <--> opp:%d opp_side:%d  us:%d them:%d\n",
ld, ld_side, opp, opp_side, ld_in_path?1:0, opp_in_path?1:0);
#endif

			lineloop_c *island = new lineloop_c;

			// treat isolated linedefs like islands
			if (! ld_in_path &&
				Vertex_HowManyLineDefs(gDocument.linedefs[ld]->start) == 1 &&
				Vertex_HowManyLineDefs(gDocument.linedefs[ld]->end)   == 1)
			{
				island->push_back(ld, Side::right);
				island->push_back(ld, Side::left);

				islands.push_back(island);
				count++;

				continue;
			}

			bool ok;

			if (ld_in_path)
				ok = TraceLineLoop(opp, opp_side, *island);
			else
				ok = TraceLineLoop(ld, ld_side, *island);

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
	DebugPrintf("Lineloop %p : %zu lines, %zu islands\n",
	            this, lines.size(), islands.size());

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		const LineDef *L = gDocument.linedefs[lines[i]];

		DebugPrintf("  %s of line #%d : (%f %f) --> (%f %f)\n",
		            sides[i] == Side::left ? " LEFT" : "RIGHT",
					lines[i],
					L->Start(gDocument)->x(), L->Start(gDocument)->y(),
					L->End  (gDocument)->x(), L->End  (gDocument)->y());
	}
}


static inline bool WillBeTwoSided(int ld, Side side)
{
	const LineDef *L = gDocument.linedefs[ld];

	if (L->WhatSideDef(side) < 0)
	{
		return (L->right >= 0) || (L->left >= 0);
	}

	return L->TwoSided();
}


static void DetermineNewTextures(lineloop_c& loop,
								 std::vector<int>& lower_texs,
								 std::vector<int>& upper_texs)
{
	unsigned int total = static_cast<unsigned>(loop.lines.size());

	SYS_ASSERT(lower_texs.size() == total);

	int null_tex  = BA_InternaliseString("-");

	int def_lower = BA_InternaliseString(default_wall_tex);
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

			int sd = gDocument.linedefs[ld]->WhatSideDef(side);
			if (sd < 0)
				continue;

			const SideDef *SD = gDocument.sidedefs[sd];

			if (gDocument.linedefs[ld]->TwoSided())
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
		int sd = gDocument.linedefs[ld]->WhatSideDef(loop.sides[k]);

		if (sd < 0)
		{
			lower_texs[k] = upper_texs[k] = -1;
			continue;
		}

		const SideDef *SD = gDocument.sidedefs[sd];

		if (gDocument.linedefs[ld]->TwoSided())
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

			bool two_k = WillBeTwoSided(loop.lines[k], loop.sides[k]);

			bool two_p = WillBeTwoSided(loop.lines[p], loop.sides[p]);
			bool two_n = WillBeTwoSided(loop.lines[n], loop.sides[n]);

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
static void DoAssignSector(int ld, Side side, int new_sec,
						   int new_lower, int new_upper,
						   selection_c *flip)
{
// DebugPrintf("DoAssignSector %d ---> line #%d, side %d\n", new_sec, ld, side);
	const LineDef * L = gDocument.linedefs[ld];

	int sd_num   = (side == Side::right) ? L->right : L->left;
	int other_sd = (side == Side::right) ? L->left  : L->right;

	if (sd_num >= 0)
	{
		gDocument.basis.changeSidedef(sd_num, SideDef::F_SECTOR, new_sec);
		return;
	}

	// if we're adding a sidedef to a line that has no sides, and
	// the sidedef would be the 2nd one, then flip the linedef.
	// Thus we don't end up with invalid lines -- i.e. ones with a
	// left side but no right side.

	if (side == Side::left && other_sd < 0)
		flip->set(ld);
	else
		flip->clear(ld);

	SYS_ASSERT(new_lower >= 0);
	SYS_ASSERT(new_upper >= 0);

	// create new sidedef
	int new_sd = gDocument.basis.addNew(ObjType::sidedefs);

	SideDef * SD = gDocument.sidedefs[new_sd];

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
		gDocument.basis.changeLinedef(ld, LineDef::F_RIGHT, new_sd);
	else
		gDocument.basis.changeLinedef(ld, LineDef::F_LEFT, new_sd);

	// if we're adding a second side to the linedef, clear out some
	// of the properties that aren't needed anymore: middle texture,
	// two-sided flag, and impassible flag.

	if (other_sd >= 0)
		LD_AddSecondSideDef(ld, new_sd, other_sd);
}


void lineloop_c::AssignSector(int new_sec, selection_c *flip)
{
	std::vector<int> lower_texs(lines.size());
	std::vector<int> upper_texs(lines.size());

	DetermineNewTextures(*this, lower_texs, upper_texs);

	for (unsigned int k = 0 ; k < lines.size() ; k++)
	{
		DoAssignSector(lines[k], sides[k], new_sec,
					   lower_texs[k], upper_texs[k], flip);
	}

	for (unsigned int i = 0 ; i < islands.size() ; i++)
	{
		islands[i]->AssignSector(new_sec, flip);
	}
}


static bool GetLoopForSpace(double map_x, double map_y, lineloop_c& loop)
{
	selection_c seen_lines(ObjType::linedefs);

	int ld;
	Side side;

	ld = gDocument.hover.getClosestLine_CastingHoriz(map_x, map_y, &side);

	DebugPrintf("GetLoopForSpace : hit line #%d, side %d\n", ld, side);

	while (ld >= 0)
	{
		// never try this line again
		seen_lines.set(ld);

		if (! TraceLineLoop(ld, side, loop))
		{
			DebugPrintf("Area is not closed (tracing a loop failed)\n");
			return false;
		}

		// not an island?  GOOD!
		if (! loop.faces_outward)
		{
			return true;
		}

		DebugPrintf("  hit island\n");

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

			new_ld = gDocument.hover.getOppositeLinedef(loop.lines[k], loop.sides[k], &new_side, nullptr);

			if (new_ld < 0)
				continue;

			if (seen_lines.get(new_ld))
				continue;

			// try again...
			ld   = new_ld;
			side = new_side;

			DebugPrintf("  trying again with line #%d, side %d\n", ld, side);
			break;
		}
	}

	DebugPrintf("Area is not closed (can see infinity)\n");
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
bool AssignSectorToSpace(double map_x, double map_y, int new_sec, int model)
{
	lineloop_c loop;

	if (! GetLoopForSpace(map_x, map_y, loop))
	{
		Beep("Area is not closed");
		return false;
	}

	loop.FindIslands();

	if (new_sec < 0)
	{
		new_sec = gDocument.basis.addNew(ObjType::sectors);

		if (model < 0)
			model = loop.NeighboringSector();

		if (model < 0)
			gDocument.sectors[new_sec]->SetDefaults();
		else
			*gDocument.sectors[new_sec] = *gDocument.sectors[model];
	}

	selection_c   flip(ObjType::linedefs);
	selection_c unused(ObjType::sectors);

	loop.GetAllSectors(&unused);

	loop.AssignSector(new_sec, &flip);

	gDocument.linemod.flipLinedefGroup(&flip);

	// detect any sectors which have become unused, and delete them
	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = gDocument.linedefs[n];

		if (L->WhatSector(Side::left, gDocument) >= 0)
			unused.clear(L->WhatSector(Side::left, gDocument));
		if (L->WhatSector(Side::right, gDocument) >= 0)
			unused.clear(L->WhatSector(Side::right, gDocument));
	}

	DeleteObjects(&unused);

	return true;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
