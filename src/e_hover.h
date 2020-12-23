//------------------------------------------------------------------------
//  HIGHLIGHT HELPER
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

#ifndef __EUREKA_X_HOVER_H__
#define __EUREKA_X_HOVER_H__

#include "DocumentModule.h"

class Objid;
class bitvec_c;

//
// The hover module
//
class Hover : public DocumentModule
{
public:
	Hover(Document &doc) : DocumentModule(doc)
	{
	}

	Objid getNearbyObject(ObjType type, double x, double y) const;

private:
	Objid getNearestThing(double x, double y) const;
	Objid getNearestVertex(double x, double y) const;
	Objid getNearestLinedef(double x, double y) const;
};

void FindSplitLine(Objid& out, double& out_x, double& out_y,
				   double ptr_x, double ptr_y, int ignore_vert = -1);

void FindSplitLineForDangler(Objid& out, int v_num);

double ApproxDistToLineDef(const LineDef * L, double x, double y);

int ClosestLine_CastingHoriz(double x, double y, Side *side);
int ClosestLine_CastingVert (double x, double y, Side *side);
int ClosestLine_CastAtAngle (double x, double y, float radians);

int OppositeLineDef(int ld, Side ld_side, Side *result_side, const bitvec_c *ignore_lines = NULL);
int OppositeSector(int ld, Side ld_side);

void FastOpposite_Begin();
void FastOpposite_Finish();

bool PointOutsideOfMap(double x, double y);

// result: -1 for back, +1 for front, 0 for _exactly_on_ the line
Side PointOnLineSide(double x, double y, double lx1, double ly1, double lx2, double ly2);


typedef struct
{
	int vert;	// >= 0 when we hit a vertex
	int ld;     // >= 0 when we hit a linedef instead

	double x, y;	// coordinate of line split point
	double dist;
}
cross_point_t;


class crossing_state_c
{
public:
	std::vector< cross_point_t > points;

	// the start/end coordinates of the whole tested line
	double start_x, start_y;
	double   end_x,   end_y;

public:
	 crossing_state_c();
	~crossing_state_c();

	void clear();

	void add_vert(int v, double dist);
	void add_line(int ld, double new_x, double new_y, double dist);

	bool HasVertex(int v) const;
	bool HasLine(int ld)  const;

	void Sort();

	void SplitAllLines();

private:
	struct point_CMP
	{
		inline bool operator() (const cross_point_t &A, const cross_point_t& B) const
		{
			return A.dist < B.dist;
		}
	};
};

void FindCrossingPoints(crossing_state_c& cross,
						double x1, double y1, int possible_v1,
						double x2, double y2, int possible_v2);

#endif  /* __EUREKA_X_HOVER_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
