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

#include "DocumentModule.h"

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
	std::vector< Side > sides;

	//  true if the lines face outward (average angle > 180 degrees)
	// false if the lines face  inward (average angle < 180 degrees)
	bool faces_outward = false;

	// Islands are outward facing line-loops which lie inside this one
	// (which must be inward facing).  This list is only created by
	// calling FindIslands() method, which can be very expensive.
	std::vector< lineloop_c * > islands;

	const Document &doc;
public:
	lineloop_c(const Document &doc) : doc(doc)
	{
	}

	~lineloop_c()
	{
		clear();
	}

	void clear();

	void push_back(int ld, Side side);

	// test if the given line/side combo is in the loop
	bool get(int ld, Side side) const;
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

	int DetermineSector() const;

	// return all the sectors which the lineloop faces
	void GetAllSectors(selection_c *list) const;

	// assign a new sector to the whole loop, including islands.
	// the 'flip' parameter will contain lines that should be flipped
	// afterwards (to ensure it has a valid right side).
	// 'new_sec' MUST be a valid sector number.
	void AssignSector(EditOperation &op, int new_sec, selection_c &flip);

	void Dump() const;

private:
	bool LookForIsland();

	void CalcBounds(double *x1, double *y1, double *x2, double *y2) const;
};

//
// Sector module
//
class SectorModule : public DocumentModule
{
	friend class Instance;
public:
	SectorModule(Document &doc) : DocumentModule(doc)
	{
	}

	bool traceLineLoop(int ld, Side side, lineloop_c& loop, bool ignore_bare = false) const;
	bool assignSectorToSpace(EditOperation &op, const v2double_t &map, int new_sec = -1, int model = -1) const;
	void sectorsAdjustLight(int delta) const;
	void safeRaiseLower(EditOperation &op, int sec, int parts, int dz) const;

private:
	friend class lineloop_c;

	void linedefsBetweenSectors(selection_c *list, int sec1, int sec2) const;
	void replaceSectorRefs(EditOperation &op, int old_sec, int new_sec) const;
	inline bool willBeTwoSided(int ld, Side side) const;
	void determineNewTextures(lineloop_c& loop,
									 std::vector<int>& lower_texs,
							  std::vector<int>& upper_texs) const;
	void doAssignSector(EditOperation &op, int ld, Side side, int new_sec,
							   int new_lower, int new_upper,
						selection_c &flip) const;
	bool getLoopForSpace(double map_x, double map_y, lineloop_c& loop) const;
};


#endif  /* __EUREKA_E_SECTOR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
