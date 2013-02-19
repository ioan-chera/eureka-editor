//------------------------------------------------------------------------
//  LINE LOOP HANDLING
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

#include <map>

#include "levels.h"
#include "selectn.h"
#include "objects.h"
#include "m_bitvec.h"
#include "e_linedef.h"
#include "w_rawdef.h"
#include "x_loop.h"
#include "x_hover.h"


// #define DEBUG_PATH  1


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


void lineloop_c::push_back(int ld, int side)
{
	lines.push_back(ld);
	sides.push_back(side);
}


bool lineloop_c::get(int ld, int side) const
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
		const LineDef *L = LineDefs[k];

		int dx = L->Start()->x - L->End()->x;
		int dy = L->Start()->y - L->End()->y;

		result += ComputeDist(dx, dy);
	}

	return result;
}


/* !!! FIXME: put in e_basis or e_linedef */



bool lineloop_c::SameSector(int *sec_num) const
{
	// NOTE: does NOT include islands

	SYS_ASSERT(lines.size() > 0);

	int sec = LineDefs[lines[0]]->WhatSector(sides[0]);

	for (unsigned int k = 1 ; k < lines.size() ; k++)
	{
		if (sec != LineDefs[lines[k]]->WhatSector(sides[k]))
			return false;
	}

	if (sec_num)
		*sec_num = sec;

	return true;
}


bool lineloop_c::AllNew() const
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
	int best_len = -1;

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		const LineDef *L = LineDefs[lines[i]];

		// we assume here that SIDE_RIGHT == 0 - SIDE_LEFT
		int sec = LineDefs[lines[i]]->WhatSector(- sides[i]);

		if (sec < 0)
			continue;

		int dx = L->Start()->x - L->End()->x;
		int dy = L->Start()->y - L->End()->y;
		int len = ComputeDist(dx, dy);

		if (len > best_len)
		{
			best = sec;
			best_len = len;
		}
	}

	return best;
}


int lineloop_c::FacesSector() const
{
	// test is only valid for islands
	if (! faces_outward)
		return -1;
	
	// we might need to check multiple lines, as the first line could
	// be facing a linedef which is ALSO part of the island.

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		int opp_side;
		int opp = OppositeLineDef(lines[i], sides[i], &opp_side);

		// can see "the void" : outside of map
		if (opp < 0)
			return -1;

		// part of the island?
		if (get_just_line(opp))
			continue;

		return LineDefs[opp]->WhatSector(opp_side);
	}

	return -1;
}


/* TO BE REMOVED ??

void lineloop_c::ToBitvec(bitvec_c& bv)
{
	bv.clear_all();

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		bv.set(lines[i]);
	}
}


void lineloop_c::ToSelection(selection_c& sel)
{
	sel.change_type(OBJ_LINEDEFS);

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		sel.set(lines[i]);
	}
}
*/


/*
   compute the angle between lines AB and BC, going anticlockwise.
   result is in degrees in the range [0, 360).
   A, B and C are vertex indices.

   -AJA- 2001-05-09

   FIXME: move to e_linedef (or so)
 */
double AngleBetweenLines(int A, int B, int C)
{
	int a_dx = Vertices[B]->x - Vertices[A]->x;
	int a_dy = Vertices[B]->y - Vertices[A]->y;

	int c_dx = Vertices[B]->x - Vertices[C]->x;
	int c_dy = Vertices[B]->y - Vertices[C]->y;

	double AB_angle = (a_dx == 0) ? (a_dy >= 0 ? 90 : -90) : atan2(a_dy, a_dx) * 180 / M_PI;
	double CB_angle = (c_dx == 0) ? (c_dy >= 0 ? 90 : -90) : atan2(c_dy, c_dx) * 180 / M_PI;

	double result = CB_angle - AB_angle;

	while (result >= 360.0)
		result -= 360.0;

	while (result < 0)
		result += 360.0;

#if 0  // DEBUGGING
	DebugPrintf("ANGLE %1.6f  (%d,%d) -> (%d,%d) -> (%d,%d)\n", result,
			Vertices[A].x, Vertices[A].y,
			Vertices[B].x, Vertices[B].y,
			Vertices[C].x, Vertices[C].y);
#endif

	return result;
}


void lineloop_c::CalcBounds(int *x1, int *y1, int *x2, int *y2) const
{
	SYS_ASSERT(lines.size() > 0);

	*x1 = +99999; *y1 = +99999;
	*x2 = -99999; *y2 = -99999;

	for (unsigned int i = 0 ; i < lines.size() ; i++)
	{
		const LineDef *L = LineDefs[lines[i]];

		*x1 = MIN(*x1, MIN(L->Start()->x, L->End()->x));
		*y1 = MIN(*y1, MIN(L->Start()->y, L->End()->y));

		*x2 = MAX(*x2, MAX(L->Start()->x, L->End()->x));
		*y2 = MAX(*y2, MAX(L->Start()->y, L->End()->y));
	}
}


/*
   Follows the path clockwise from the given start line, adding each
   line into the appropriate set.  Returns true if the path was closed,
   or false for failure (in which case the lineloop_c object will not
   be in a valid state).

   side is either SIDE_LEFT or SIDE_RIGHT.
   
   -AJA- 2001-05-09
 */
bool TraceLineLoop(int ld, int side, lineloop_c& loop, bool ignore_new)
{
	loop.clear();

	int cur_vert;
	int prev_vert;

	if (side == SIDE_RIGHT)
	{
		cur_vert  = LineDefs[ld]->end;
		prev_vert = LineDefs[ld]->start;
	}
	else
	{
		cur_vert  = LineDefs[ld]->start;
		prev_vert = LineDefs[ld]->end;
	}

	int final_vert = prev_vert;

#ifdef DEBUG_PATH
	DebugPrintf("TRACE PATH: line:%d  side:%d  cur:%d  final:%d\n",
			ld, side, cur_vert, final_vert);
#endif

	// compute the average angle
	double average_angle = 0;

	// add the starting line
	loop.push_back(ld, side);

	while (cur_vert != final_vert)
	{
		int next_line = -1;
		int next_vert = -1;
		int next_side =  0;

		double best_angle = 9999;

		// Look for the next linedef in the path.  It's the linedef which
		// uses the current vertex, not the same as the current line, and
		// has the smallest interior angle.

		for (int n = 0 ; n < NumLineDefs ; n++)
		{
			const LineDef * N = LineDefs[n];

			if (N->start != cur_vert && N->end != cur_vert)
				continue;

			if (n == ld)
				continue;

			if (ignore_new && !N->Left() && !N->Right())
				continue;

			int other_vert;
			int which_side;

			if (N->start == cur_vert)
			{
				other_vert = N->end;
				which_side = SIDE_RIGHT;
			}
			else  // (N->end == cur_vert)
			{
				other_vert = N->start;
				which_side = SIDE_LEFT;
			}

			// found adjoining linedef

			double angle = AngleBetweenLines(prev_vert, cur_vert, other_vert);

			if (next_line < 0 || angle < best_angle)
			{
				next_line = n;
				next_vert = other_vert;
				next_side = which_side;

				best_angle = angle;
			}

			// continue the search...
		}

#ifdef DEBUG_PATH
		DebugPrintf("PATH NEXT: line:%d  side:%d  vert:%d  angle:%1.6f\n",
				next_line, next_side, next_vert, best_angle);
#endif

		// No next line?  Path cannot be closed
		if (next_line < 0)
			return false;

		// Line already seen?  Under normal circumstances this won't
		// happen, but it _can_ happen and indicates a non-closed
		// structure
		if (loop.get_just_line(next_line))
			return false;

		ld   = next_line;
		side = next_side;

		prev_vert = cur_vert;
		cur_vert  = next_vert;

		average_angle += best_angle;

		// add the next line
		loop.push_back(ld, side);
	}

	// this might happen if there are overlapping linedefs
	if (loop.lines.size() < 3)
		return false;

	average_angle = average_angle / (double)loop.lines.size();

	loop.faces_outward = (average_angle >= 180.0);

#ifdef DEBUG_PATH
	DebugPrintf("PATH CLOSED!  average_angle:%1.2f\n", average_angle);
#endif

	return true;
}


bool lineloop_c::LookForIsland()
{
	// ALGORITHM:
	//    Iterate over all lines which lie within the bounding box of
	//    the current path (but not _in_ the current path).
	//
	//    Use OppositeLineDef() to find starting lines, and trace them
	//    (tracing is necessary because in certain shapes, the opposite
	//    lines will be part of the island too).
	//
	// Returns: true if found one, false otherwise.
	//

	// calc bounding box
	int bb_x1, bb_y1, bb_x2, bb_y2;

	CalcBounds(&bb_x1, &bb_y1, &bb_x2, &bb_y2);

	// cut me some slack, dude
	bb_x1--;  bb_y1--;
	bb_x2++;  bb_y2++;

	int count = 0;

	for (int ld = 0 ; ld < NumLineDefs ; ld++)
	{
		const LineDef * L = LineDefs[ld];

		int x1 = L->Start()->x;
		int y1 = L->Start()->y;
		int x2 = L->End()->x;
		int y2 = L->End()->y;

		if (MAX(x1, x2) < bb_x1 || MIN(x1, x2) > bb_x2 ||
		    MAX(y1, y2) < bb_y1 || MIN(y1, y2) > bb_y2)
			continue;

		// ouch, this is gonna be SLOW : O(n^2) on # lines 

		for (int where = 0 ; where < 2 ; where++)
		{
			int ld_side = where ? SIDE_RIGHT : SIDE_LEFT;

			int opp_side;
			int opp = OppositeLineDef(ld, ld_side, &opp_side);

			if (opp < 0)
				continue;

			bool ld_in_path  = get(ld,  ld_side);
			bool opp_in_path = get(opp, opp_side);

			// nothing to do when both (or neither) are in path
			if (ld_in_path == opp_in_path)
				continue;

#ifdef DEBUG_PATH
DebugPrintf("Found line:%d side:%d <--> opp:%d opp_side:%d  us:%d them:%d\n",
ld, ld_side, opp, opp_side, ld_in_path?1:0, opp_in_path?1:0);
#endif

			lineloop_c *island = new lineloop_c;

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
	//  Repeat these steps until no more islands are found.
	//  (This is needed to handle e.g. a big room full of pillars,
	//   since the pillars in the middle won't "see" the outer sector
	//   until the neighboring pillars are added).
	//
	// Example: the two pillars at the start of MAP01 of DOOM 2.

	// use a counter for safety 
	for (int loop = 200 ; loop >= 0 ; loop--)
	{
		if (loop == 0)
			BugError("Infinite loop in FindIslands!\n");

		if (! LookForIsland())
			break;
	}
}


// FIXME: move to e_linedef
void LD_AddSecondSideDef(int ld, int new_sd, int other_sd)
{
	LineDef * L  = LineDefs[ld];
	SideDef * SD = SideDefs[new_sd];

	int new_flags = L->flags;

	new_flags |=  MLF_TwoSided;
	new_flags &= ~MLF_Blocking;

	BA_ChangeLD(ld, LineDef::F_FLAGS, new_flags);

	// FIXME: make this a global pseudo-constant
	int null_tex = BA_InternaliseString("-");

	const SideDef *other = SideDefs[other_sd];

	if (isalnum(other->MidTex()[0]))
	{
		SD->lower_tex = other->mid_tex;
		SD->upper_tex = other->mid_tex;

		if (! isalnum(other->LowerTex()[0]))
			BA_ChangeSD(other_sd, SideDef::F_LOWER_TEX, other->mid_tex);

		if (! isalnum(other->UpperTex()[0]))
			BA_ChangeSD(other_sd, SideDef::F_UPPER_TEX, other->mid_tex);

		BA_ChangeSD(other_sd, SideDef::F_MID_TEX, null_tex);
	}
	else
	{
		SD->lower_tex = other->lower_tex;
		SD->upper_tex = other->upper_tex;
	}
}


void LD_RemoveSideDef(int ld, int ld_side)
{
	const LineDef *L = LineDefs[ld];

	int gone_sd  = (ld_side > 0) ? L->right : L->left;
	int other_sd = (ld_side > 0) ? L->left : L->right;

	if (ld_side > 0)
		BA_ChangeLD(ld, LineDef::F_RIGHT, -1);
	else
		BA_ChangeLD(ld, LineDef::F_LEFT, -1);

	if (other_sd < 0)
		return;

	// The line is changing from TWO SIDED --> ONE SIDED.
	// Hence we need to:
	//    (1) clear the Two-Sided flag
	//    (2) set the Impassible flag
	//	  (3) flip the linedef if right side was removed
	//    (4) set the middle texture

	int new_flags = L->flags;

	new_flags &= ~MLF_TwoSided;
	new_flags |=  MLF_Blocking;

	BA_ChangeLD(ld, LineDef::F_FLAGS, new_flags);

	// FIXME: if sidedef is shared, either don't modify it _OR_ duplicate it

	const SideDef *SD = SideDefs[other_sd];

	int new_tex = BA_InternaliseString(default_mid_tex);

	// grab new texture from lower or upper if possible
	if (isalnum(SD->LowerTex()[0]))
		new_tex = SD->lower_tex;
	else if (isalnum(SD->UpperTex()[0]))
		new_tex = SD->upper_tex;
	else if (gone_sd >= 0)
	{
		SD = SideDefs[gone_sd];

		if (isalnum(SD->LowerTex()[0]))
			new_tex = SD->lower_tex;
		else if (isalnum(SD->UpperTex()[0]))
			new_tex = SD->upper_tex;
	}

	BA_ChangeSD(other_sd, SideDef::F_MID_TEX, new_tex);
}


/*
   update the side on a single linedef, using the given sector
   reference.  Will create a new sidedef if necessary.
 */
static void DoAssignSector(int ld, int side, int new_sec, selection_c& flip)
{
// DebugPrintf("DoAssignSector %d ---> line #%d, side %d\n", new_sec, ld, side);
	const LineDef * L = LineDefs[ld];

	int sd_num   = (side > 0) ? L->right : L->left;
	int other_sd = (side > 0) ? L->left  : L->right;

	if (sd_num >= 0)
	{
		// FIXME: if sidedef is shared, duplicate it

		BA_ChangeSD(sd_num, SideDef::F_SECTOR, new_sec);
		return;
	}

	// if we're adding a sidedef to a line that has no sides, and
	// the sidedef would be the 2nd one, then flip the linedef.
	// Thus we don't end up with invalid lines -- i.e. ones with a
	// left side but no right side.

	if (side < 0 && other_sd < 0)
	{
		flip.set(ld);
	}

	// create new sidedef
	int new_sd = BA_New(OBJ_SIDEDEFS);

	SideDef * SD = SideDefs[new_sd];

	SD->SetDefaults(other_sd >= 0);
	SD->sector = new_sec;

	if (side > 0)
		BA_ChangeLD(ld, LineDef::F_RIGHT, new_sd);
	else
		BA_ChangeLD(ld, LineDef::F_LEFT, new_sd);

	// if we're adding a second side to the linedef, clear out some
	// of the properties that aren't needed anymore: middle texture,
	// two-sided flag, and impassible flag.

	if (other_sd >= 0)
		LD_AddSecondSideDef(ld, new_sd, other_sd);
}


void AssignSectorToLoop(lineloop_c& loop, int new_sec, selection_c& flip)
{
	for (unsigned int k = 0 ; k < loop.lines.size() ; k++)
	{
		int ld   = loop.lines[k];
		int side = loop.sides[k];

		DoAssignSector(ld, side, new_sec, flip);
	}

	for (unsigned int i = 0 ; i < loop.islands.size() ; i++)
	{
		AssignSectorToLoop(* loop.islands[i], new_sec, flip);
	}
}


//
// Change the closed sector at the pointer
//
// "sector" here really means a bunch of sidedefs that all face
// inward to the current area under the mouse cursor.
//
// the 'new_sec' parameter is either a valid sector number (all the
// sidedefs are changed to it), or it is the negated number of a
// model sector and a new sector with the same properties is created.
//
void AssignSectorToSpace(int map_x, int map_y, int new_sec,
                         bool model_from_neighbor)
{
	int ld, side;

	ld = ClosestLine_CastingHoriz(map_x, map_y, &side);

	if (ld < 0)
	{
		Beep("Area is not closed");
		DebugPrintf("Area is not closed (can see infinity)\n");
		return;
	}

	lineloop_c loop;

	if (! TraceLineLoop(ld, side, loop))
	{
		Beep("Area is not closed");
		DebugPrintf("Area is not closed (tracing a loop failed)\n");
		return;
	}

	// FIXME: should look in other directions for an inward line loop

	if (loop.faces_outward)
	{
		Beep("Line loop faces outward");
		DebugPrintf("Line loop faces outward\n");
		return;
	}

	loop.FindIslands();

	if (model_from_neighbor)
	{
		int model = loop.NeighboringSector();

		if (model >= 0)
			Sectors[new_sec]->RawCopy(Sectors[model]);
		else
			Sectors[new_sec]->SetDefaults();
	}

	selection_c flip(OBJ_LINEDEFS);

	AssignSectorToLoop(loop, new_sec, flip);

	FlipLineDefGroup(flip);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
