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

#ifndef __EUREKA_X_LOOP_H__
#define __EUREKA_X_LOOP_H__

class lineloop_c
{
public:
	// This contains the linedefs in the line loop, beginning with
	// the first line and going in order until the last line.  There
	// will be at least 3 lines in the loop, and the same linedef
	// cannot occur more than onces.
	std::vector< int > lines;

	// This contains which side of the linedefs in 'lines'.
	// Guaranteed to be the same size as 'lines'.
	// Each value is either SIDE_LEFT or SIDE_RIGHT.
	std::vector< int > sides;

	//  true if the lines face outward (average angle > 180 degrees)
	// false if the lines face  inward (average angle < 180 degrees)
	bool faces_outward;

	// Islands are outward facing line-loops which lie inside this one
	// (which must be inward facing).  This list is only created by
	// calling FindIslands() method, which can be very expensive.
	std::vector< lineloop_c * > islands;

public:
	 lineloop_c();
	~lineloop_c();

	void clear();

	void push_back(int ld, int side);

	// test if the given line/side combo is in the loop
	bool get(int ld, int side) const;
	bool get_just_line(int ld) const;

	void FindIslands();

	double TotalLength() const;

	bool SameSector(int *sec_num = NULL) const;
	bool AllNew() const;

	// find a sector that neighbors this line loop, i.e. is on the other
	// side of one of the lines.  If there are multiple neighbors, then
	// only one of them is returned.  If there are none, returns -1.
	int NeighboringSector() const;

	// check if an island (faces_outward is true) lies inside a sector.
	// sector, returning the sector number if true, otherwise -1.
	int FacesSector() const;

#if 0
	void ToBitvec(bitvec_c& bv);
	void ToSelection(selection_c& sel);
#endif

public:
	bool LookForIsland();

	void CalcBounds(int *x1, int *y1, int *x2, int *y2) const;
};


bool TraceLineLoop(int ld, int side, lineloop_c& loop, bool ignore_new = false);

void AssignSectorToSpace(int map_x, int map_y, int new_sec, bool model_from_neighbor = false);
void AssignSectorToLoop(lineloop_c& loop, int new_sec, selection_c& flip);

double AngleBetweenLines(int A, int B, int C);

void LD_AddSecondSideDef(int ld, int new_sd, int other_sd);
void LD_RemoveSideDef(int ld, int ld_side);


#endif  /* __EUREKA_X_LOOP_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
