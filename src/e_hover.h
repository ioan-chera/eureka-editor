//------------------------------------------------------------------------
//  HIGHLIGHT HELPER
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

#ifndef __EUREKA_X_HOVER_H__
#define __EUREKA_X_HOVER_H__


class Objid;
class bitvec_c;


void GetNearObject(Objid& o, obj_type_e objtype, float x, float y);

void GetSplitLineDef(Objid& o, int x, int y, int drag_vert = -1);
void GetSplitLineForDangler(Objid& o, int v_num);

float ApproxDistToLineDef(const LineDef * L, float x, float y);

int ClosestLine_CastingHoriz(int x, int y, int *side);
int ClosestLine_CastingVert (int x, int y, int *side);
int ClosestLine_CastAtAngle (int x, int y, float radians);

int OppositeLineDef(int ld, int ld_side, int *result_side, bitvec_c *ignore_lines = NULL);
int OppositeSector(int ld, int ld_side);

void FastOpposite_Begin();
void FastOpposite_Finish();

bool PointOutsideOfMap(int x, int y);

// result: -1 for back, +1 for front, 0 for _exactly_on_ the line
int PointOnLineSide(int x, int y, int lx1, int ly1, int lx2, int ly2);


typedef struct
{
	int vert;	// >= 0 when we hit a vertex
	int ld;     // >= 0 when we hit a linedef instead

	int x, y;	// coordinate of line split point

	double dist;
}
cross_point_t;


class crossing_state_c
{
public:
	std::vector< cross_point_t > points;

	// the start/end coordinates of the whole tested line
	int start_x, start_y;
	int   end_x,   end_y;

public:
	 crossing_state_c();
	~crossing_state_c();

	void clear();

	void add_vert(int v, double dist);
	void add_line(int ld, int new_x, int new_y, double dist);

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
						int x1, int y1, int possible_v1,
						int x2, int y2, int possible_v2);

#endif  /* __EUREKA_X_HOVER_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
