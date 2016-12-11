//------------------------------------------------------------------------
//  SECTOR OPERATIONS
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

#ifndef __EUREKA_E_SECTOR_H__
#define __EUREKA_E_SECTOR_H__

class lineloop_c
{
public:
	// This contains the linedefs in the line loop, beginning with
	// the first line and going in order until the last line.  There
	// will be at least 3 lines in the loop.  It is possible for a
	// linedef to be present twice (a loop traversing both sides).
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

	// checks if all lines in the loop are facing the same sector.
	// when true, returns that sector via 'sec_num' (if not null).
	// the 'sec_num' value may be -1 if all lines are "bare".
	// NOTE : does not test the islands.
	bool SameSector(int *sec_num = NULL) const;

	// true if all lines in the loop are facing nothing.
	bool AllBare() const;

	// find a sector that neighbors this line loop, i.e. is on the other
	// side of one of the lines.  If there are multiple neighbors, then
	// only one of them is returned.  If there are none, returns -1.
	int NeighboringSector() const;

	// check if an island lies inside a sector, returning the sector
	// number if true, otherwise -1.
	int IslandSector() const;

	// return all the sectors which the lineloop faces
	void GetAllSectors(selection_c *list) const;

	// assign a new sector to the whole loop, including islands.
	// the 'flip' parameter will contain lines that should be flipped
	// afterwards (to ensure it has a valid right side).
	// 'new_sec' MUST be a valid sector number.
	void AssignSector(int new_sec, selection_c& flip);

	void Dump() const;

private:
	bool LookForIsland();

	void CalcBounds(int *x1, int *y1, int *x2, int *y2) const;

	void DoAssignSector(int ld, int side, int new_sec, selection_c& flip);
};


bool TraceLineLoop(int ld, int side, lineloop_c& loop, bool ignore_bare = false);

void AssignSectorToSpace(int map_x, int map_y, int new_sec, bool model_from_neighbor = false);

void SectorsAdjustLight(int delta);


/* commands */

void CMD_SEC_Floor(void);
void CMD_SEC_Ceil(void);

void CMD_SEC_Light(void);

void CMD_SEC_Merge(void);
void CMD_SEC_SwapFlats(void);

#endif  /* __EUREKA_E_SECTOR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
