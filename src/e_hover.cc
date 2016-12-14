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

#include "main.h"

#include <algorithm>

#include "e_hover.h"
#include "e_linedef.h"	// SplitLineDefAtVertex()
#include "e_main.h"		// Map_bound_xxx
#include "m_game.h"
#include "r_grid.h"


extern int vertex_radius(double scale);


float ApproxDistToLineDef(const LineDef * L, float x, float y)
{
	int x1 = L->Start()->x;
	int y1 = L->Start()->y;
	int x2 = L->End()->x;
	int y2 = L->End()->y;

	int dx = x2 - x1;
	int dy = y2 - y1;

	if (abs(dx) > abs(dy))
	{
		// The linedef is rather horizontal

		// case 1: x is to the left of the linedef
		//         hence return distance to the left-most vertex
		if (x < (dx > 0 ? x1 : x2))
			return hypot(x - (dx > 0 ? x1 : x2),
						 y - (dx > 0 ? y1 : y2));

		// case 2: x is to the right of the linedef
		//         hence return distance to the right-most vertex
		if (x > (dx > 0 ? x2 : x1))
			return hypot(x - (dx > 0 ? x2 : x1),
						 y - (dx > 0 ? y2 : y1));

		// case 3: x is in-between (and not equal to) both vertices
		//         hence use slope formula to get intersection point
		float y3 = y1 + (x - x1) * (float)dy / (float)dx;

		return fabs(y3 - y);
	}
	else
	{
		// The linedef is rather vertical

		if (y < (dy > 0 ? y1 : y2))
			return hypot(x - (dy > 0 ? x1 : x2),
						 y - (dy > 0 ? y1 : y2));

		if (y > (dy > 0 ? y2 : y1))
			return hypot(x - (dy > 0 ? x2 : x1),
						 y - (dy > 0 ? y2 : y1));

		float x3 = x1 + (y - y1) * (float)dx / (float)dy;

		return fabs(x3 - x);
	}
}


int ClosestLine_CastingHoriz(int x, int y, int *side)
{
	int   best_match = -1;
	float best_dist  = 9e9;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		int ly1 = LineDefs[n]->Start()->y;
		int ly2 = LineDefs[n]->End()->y;

		// ignore purely horizontal lines
		if (ly1 == ly2)
			continue;

		// does the linedef cross the horizontal ray?
		if (MIN(ly1, ly2) >= y + 1 || MAX(ly1, ly2) <= y)
			continue;

		int lx1 = LineDefs[n]->Start()->x;
		int lx2 = LineDefs[n]->End()->x;

		float dist = lx1 - (x + 0.5) + (lx2 - lx1) * (y + 0.5 - ly1) / (float)(ly2 - ly1);

		if (fabs(dist) < best_dist)
		{
			best_match = n;
			best_dist  = fabs(dist);

			if (side)
			{
				if (best_dist < 0.2)
					*side = 0;  // on the line
				else if ( (ly1 > ly2) == (dist > 0))
					*side = 1;  // right side
				else
					*side = -1; // left side
			}
		}
	}

	return best_match;
}


int ClosestLine_CastingVert(int x, int y, int *side)
{
	int   best_match = -1;
	float best_dist  = 9e9;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		int lx1 = LineDefs[n]->Start()->x;
		int lx2 = LineDefs[n]->End()->x;

		// ignore purely vertical lines
		if (lx1 == lx2)
			continue;

		// does the linedef cross the vertical ray?
		if (MIN(lx1, lx2) >= x + 1 || MAX(lx1, lx2) <= x)
			continue;

		int ly1 = LineDefs[n]->Start()->y;
		int ly2 = LineDefs[n]->End()->y;

		float dist = ly1 - (y + 0.5) + (ly2 - ly1) * (x + 0.5 - lx1) / (float)(lx2 - lx1);

		if (fabs(dist) < best_dist)
		{
			best_match = n;
			best_dist  = fabs(dist);

			if (side)
			{
				if (best_dist < 0.2)
					*side = 0;  // on the line
				else if ( (lx1 > lx2) == (dist < 0))
					*side = 1;  // right side
				else
					*side = -1; // left side
			}
		}
	}

	return best_match;
}


int ClosestLine_CastAtAngle(int x, int y, float radians)
{
	int   best_match = -1;
	float best_dist  = 9e9;

	double x2 = x + 256 * cos(radians);
	double y2 = y + 256 * sin(radians);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		float a = PerpDist(L->Start()->x, L->Start()->y,  x, y, x2, y2);
		float b = PerpDist(L->  End()->x, L->  End()->y,  x, y, x2, y2);

		// completely on one side of the vector?
		if (a > 0 && b > 0) continue;
		if (a < 0 && b < 0) continue;

		float c = AlongDist(L->Start()->x, L->Start()->y,  x, y, x2, y2);
		float d = AlongDist(L->  End()->x, L->  End()->y,  x, y, x2, y2);

		float dist;

		if (fabs(a) < 1 && fabs(b) < 1)
			dist = MIN(c, d);
		else if (fabs(a) < 1)
			dist = c;
		else if (fabs(b) < 1)
			dist = d;
		else
		{
			float factor = a / (a - b);
			dist = c * (1 - factor) + d * factor;
		}

		// behind or touching the vector?
		if (dist < 1) continue;

		if (dist < best_dist)
		{
			best_match = n;
			best_dist  = dist;
		}
	}

	return best_match;
}


bool PointOutsideOfMap(int x, int y)
{
	// this keeps track of directions tested
	int dirs = 0;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		int lx1 = LineDefs[n]->Start()->x;
		int ly1 = LineDefs[n]->Start()->y;
		int lx2 = LineDefs[n]->End()->x;
		int ly2 = LineDefs[n]->End()->y;

		// does the linedef cross the horizontal ray?
		if (MIN(ly1, ly2) <= y && MAX(ly1, ly2) >= y + 1)
		{
			float dist = lx1 - (x + 0.5) + (lx2 - lx1) * (y + 0.5 - ly1) / (float)(ly2 - ly1);

			dirs |= (dist < 0) ? 1 : 2;

			if (dirs == 15) return false;
		}

		// does the linedef cross the vertical ray?
		if (MIN(lx1, lx2) <= x && MAX(lx1, lx2) >= x + 1)
		{
			float dist = ly1 - (y - 0.5) + (ly2 - ly1) * (x + 0.5 - lx1) / (float)(lx2 - lx1);

			dirs |= (dist < 0) ? 4 : 8;

			if (dirs == 15) return false;
		}
	}

	return true;
}


//------------------------------------------------------------------------

#define FASTOPP_DIST  320


typedef struct
{
	int ld;
	int ld_side;   // a SIDE_XXX value

	int * result_side;

	int dx, dy;

	// origin of casting line
	float x, y;

	bool is_horizontal;

	int   best_match;
	float best_dist;

public:
	void ComputeCastOrigin()
	{
		// choose a coordinate on the source line near the middle, but make
		// sure the casting line is not integral (i.e. lies between two lines
		// on the unit grid) so that we never directly hit a vertex.

		const LineDef * L = LineDefs[ld];

		dx = L->End()->x - L->Start()->x;
		dy = L->End()->y - L->Start()->y;

		is_horizontal = abs(dy) >= abs(dx);

		x = L->Start()->x + dx * 0.5;
		y = L->Start()->y + dy * 0.5;

		if (is_horizontal && (dy & 1) == 0 && abs(dy) > 0)
		{
			y = y + 0.5;
			x = x + 0.5 * dx / (float)dy;
		}

		if (!is_horizontal && (dx & 1) == 0 && abs(dx) > 0)
		{
			x = x + 0.5;
			y = y + 0.5 * dy / (float)dx;
		}
	}

	void ProcessLine(int n)
	{
		if (ld == n)  // ignore input line
			return;

		int nx1 = LineDefs[n]->Start()->x;
		int ny1 = LineDefs[n]->Start()->y;
		int nx2 = LineDefs[n]->End()->x;
		int ny2 = LineDefs[n]->End()->y;

		if (is_horizontal)
		{
			if (ny1 == ny2)
				return;

			if (MIN(ny1, ny2) > y || MAX(ny1, ny2) < y)
				return;

			float dist = nx1 + (nx2 - nx1) * (y - ny1) / (float)(ny2 - ny1) - x;

			if ( (dy < 0) == (ld_side > 0) )
				dist = -dist;

			if (dist > 0.2 && dist < best_dist)
			{
				best_match = n;
				best_dist  = dist;

				if (result_side)
				{
					if ( (dy > 0) != (ny2 > ny1) )
						*result_side = ld_side;
					else
						*result_side = -ld_side;
				}
			}
		}
		else  // casting a vertical ray
		{
			if (nx1 == nx2)
				return;

			if (MIN(nx1, nx2) > x || MAX(nx1, nx2) < x)
				return;

			float dist = ny1 + (ny2 - ny1) * (x - nx1) / (float)(nx2 - nx1) - y;

			if ( (dx > 0) == (ld_side > 0) )
				dist = -dist;

			if (dist > 0.2 && dist < best_dist)
			{
				best_match = n;
				best_dist  = dist;

				if (result_side)
				{
					if ( (dx > 0) != (nx2 > nx1) )
						*result_side = ld_side;
					else
						*result_side = -ld_side;
				}
			}
		}
	}

} opp_test_state_t;


class fastopp_node_c
{
public:
	int lo, hi;   // coordinate range
	int mid;

	fastopp_node_c * lo_child;
	fastopp_node_c * hi_child;

	std::vector<int> lines;

public:
	fastopp_node_c(int _low, int _high) :
		lo(_low), hi(_high),
		lo_child(NULL), hi_child(NULL),
		lines()
	{
		mid = (lo + hi) / 2;

		Subdivide();
	}

	~fastopp_node_c()
	{
		delete lo_child;
		delete hi_child;
	}

	/* horizontal tree */

	void AddLine_X(int ld, int x1, int x2)
	{
		if (lo_child && (x1 > lo_child->lo) &&
		                (x2 < lo_child->hi))
		{
			lo_child->AddLine_X(ld, x1, x2);
			return;
		}

		if (hi_child && (x1 > hi_child->lo) &&
		                (x2 < hi_child->hi))
		{
			hi_child->AddLine_X(ld, x1, x2);
			return;
		}

		lines.push_back(ld);
	}

	void AddLine_X(int ld)
	{
		const LineDef *L = LineDefs[ld];

		int x1 = MIN(L->Start()->x, L->End()->x);
		int x2 = MAX(L->Start()->x, L->End()->x);

		// can ignore purely vertical lines
		if (x1 == x2) return;

		AddLine_X(ld, x1, x2);
	}

	/* vertical tree */

	void AddLine_Y(int ld, int y1, int y2)
	{
		if (lo_child && (y1 > lo_child->lo) &&
		                (y2 < lo_child->hi))
		{
			lo_child->AddLine_Y(ld, y1, y2);
			return;
		}

		if (hi_child && (y1 > hi_child->lo) &&
		                (y2 < hi_child->hi))
		{
			hi_child->AddLine_Y(ld, y1, y2);
			return;
		}

		lines.push_back(ld);
	}

	void AddLine_Y(int ld)
	{
		const LineDef *L = LineDefs[ld];

		int y1 = MIN(L->Start()->y, L->End()->y);
		int y2 = MAX(L->Start()->y, L->End()->y);

		// can ignore purely horizonal lines
		if (y1 == y2) return;

		AddLine_Y(ld, y1, y2);
	}

	void Process(opp_test_state_t& test, float coord)
	{
		for (unsigned int k = 0 ; k < lines.size() ; k++)
			test.ProcessLine(lines[k]);

		if (! lo_child)
			return;

		// the AddLine() methods ensure that lines are not added
		// into a child bucket unless the end points are completely
		// inside it -- and one unit away from the extremes.
		//
		// hence we never need to recurse down BOTH sides here.

		if (coord < mid)
			lo_child->Process(test, coord);
		else
			hi_child->Process(test, coord);
	}

private:
	void Subdivide()
	{
		if (hi - lo <= FASTOPP_DIST)
			return;

		lo_child = new fastopp_node_c(lo, mid);
		hi_child = new fastopp_node_c(mid, hi);
	}
};


static fastopp_node_c * fastopp_X_tree;
static fastopp_node_c * fastopp_Y_tree;


void FastOpposite_Begin()
{
	CalculateLevelBounds();

	fastopp_X_tree = new fastopp_node_c(Map_bound_x1 - 8, Map_bound_x2 + 8);
	fastopp_Y_tree = new fastopp_node_c(Map_bound_y1 - 8, Map_bound_y2 + 8);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		fastopp_X_tree->AddLine_X(n);
		fastopp_Y_tree->AddLine_Y(n);
	}
}


void FastOpposite_Finish()
{
	delete fastopp_X_tree;  fastopp_X_tree = NULL;
	delete fastopp_Y_tree;  fastopp_Y_tree = NULL;
}


int OppositeLineDef(int ld, int ld_side, int *result_side, bitvec_c *ignore_lines)
{
	// ld_side is either SIDE_LEFT or SIDE_RIGHT.
	// result_side uses the same values (never 0).

	opp_test_state_t  test;

	test.ld = ld;
	test.ld_side = ld_side;
	test.result_side = result_side;

	// this sets dx and dy
	test.ComputeCastOrigin();

	if (test.dx == 0 && test.dy == 0)
		return -1;

	test.best_match = -1;
	test.best_dist  = 9e9;

	if (fastopp_X_tree)
	{
		// fast way : use the binary tree

		SYS_ASSERT(ignore_lines == NULL);

		if (test.is_horizontal)
			fastopp_Y_tree->Process(test, test.y);
		else
			fastopp_X_tree->Process(test, test.x);
	}
	else
	{
		// normal way : test all linedefs

		for (int n = 0 ; n < NumLineDefs ; n++)
		{
			if (ignore_lines && ignore_lines->get(n))
				continue;

			test.ProcessLine(n);
		}
	}

	return test.best_match;
}


int OppositeSector(int ld, int ld_side)
{
	int opp_side;

	int opp = OppositeLineDef(ld, ld_side, &opp_side);

	// can see the void?
	if (opp < 0)
		return -1;

	return LineDefs[opp]->WhatSector(opp_side);
}


// result: -1 for back, +1 for front, 0 for _exactly_on_ the line
int PointOnLineSide(int x, int y, int lx1, int ly1, int lx2, int ly2)
{
	x   -= lx1; y   -= ly1;
	lx2 -= lx1; ly2 -= ly1;

	int tmp = (x * ly2 - y * lx2);

	return (tmp < 0) ? -1 : (tmp > 0) ? +1 : 0;
}


//------------------------------------------------------------------------


struct thing_comparer_t
{
public:
	double distance;
	bool   inside;
	int    radius;

	thing_comparer_t()
	{
		distance = 9e9;
		radius   = (1 << 30);
		inside   = false;
	}

	bool operator<= (const thing_comparer_t& other) const
	{
		// being inside is always better
		if (inside && ! other.inside) return true;
		if (! inside && other.inside) return false;

		// small objects should "mask" large objects
		if (radius < other.radius) return true;
		if (radius > other.radius) return false;

		return (distance <= other.distance);
	}
};


//
// determine which linedef is under the pointer
//
static Objid NearestLineDef(float x, float y)
{
	// slack in map units
	float mapslack = 2 + 16.0f / grid.Scale;

	int lx = floor(x - mapslack);
	int ly = floor(y - mapslack);
	int hx =  ceil(x + mapslack);
	int hy =  ceil(y + mapslack);

	int    best = -1;
	double best_dist = 9e9;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		int x1 = LineDefs[n]->Start()->x;
		int y1 = LineDefs[n]->Start()->y;
		int x2 = LineDefs[n]->End()->x;
		int y2 = LineDefs[n]->End()->y;

		// Skip all lines of which all points are more than <mapslack>
		// units away from (x,y). In a typical level, this test will
		// filter out all the linedefs but a handful.
		if (MAX(x1,x2) < lx || MIN(x1,x2) > hx ||
		    MAX(y1,y2) < ly || MIN(y1,y2) > hy)
			continue;

		float dist = ApproxDistToLineDef(LineDefs[n], x, y);

		if (dist > mapslack)
			continue;

		// use "<=" because if there are overlapping linedefs, we want
		// to return the highest-numbered one.
		if (dist <= best_dist)
		{
			best = n;
			best_dist = dist;
		}
	}

	if (best >= 0)
		return Objid(OBJ_LINEDEFS, best);

	// none found
	return Objid();
}


//
// determine which linedef would be split if a new vertex were
// added at the given coordinates.
//
static Objid NearestSplitLine(int x, int y, int ignore_vert)
{
	// slack in map units
	int mapslack = 1 + (int)ceil(8.0f / grid.Scale);

	int lx = x - mapslack;
	int ly = y - mapslack;
	int hx = x + mapslack;
	int hy = y + mapslack;

	int    best = -1;
	double best_dist = 9e9;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		if (L->start == ignore_vert || L->end == ignore_vert)
			continue;

		int x1 = L->Start()->x;
		int y1 = L->Start()->y;
		int x2 = L->End()->x;
		int y2 = L->End()->y;

		if (MAX(x1,x2) < lx || MIN(x1,x2) > hx ||
		    MAX(y1,y2) < ly || MIN(y1,y2) > hy)
			continue;

		// skip linedef if point matches a vertex
		if (x == x1 && y == y1) continue;
		if (x == x2 && y == y2) continue;

		// skip linedef if too small to split
		if (abs(L->Start()->x - L->End()->x) < 4 &&
			abs(L->Start()->y - L->End()->y) < 4)
			continue;

		double dist = ApproxDistToLineDef(L, x, y);

		if (dist > mapslack)
			continue;

		if (dist <= best_dist)
		{
			best = n;
			best_dist = dist;
		}
	}

	if (best >= 0)
		return Objid(OBJ_LINEDEFS, best);

	// none found
	return Objid();
}


//
//  determine which sector is under the pointer
//
static Objid NearestSector(int x, int y)
{
	/* hack, hack...  I look for the first LineDef crossing
	   an horizontal half-line drawn from the cursor */

	// -AJA- updated this to look in four directions (N/S/E/W) and
	//       grab the closest linedef.  Now it is possible to access
	//       self-referencing lines, even purely horizontal ones.

	int side1, side2;

	int line1 = ClosestLine_CastingHoriz(x, y, &side1);
  	int line2 = ClosestLine_CastingVert (x, y, &side2);

	if (line2 < 0)
	{
		/* nothing needed */
	}
	else if (line1 < 0 ||
	         ApproxDistToLineDef(LineDefs[line2], x, y) <
	         ApproxDistToLineDef(LineDefs[line1], x, y))
	{
		line1 = line2;
		side1 = side2;
	}

	// grab the sector reference from the appropriate side
	// (Note that side1 = +1 for right, -1 for left, 0 for "on").
	if (line1 >= 0)
	{
		int sd_num = (side1 < 0) ? LineDefs[line1]->left : LineDefs[line1]->right;

		if (sd_num >= 0)
			return Objid(OBJ_SECTORS, SideDefs[sd_num]->sector);
	}

	// none found
	return Objid();
}


//
// determine which thing is under the mouse pointer
//
static Objid NearestThing(float x, float y)
{
	float mapslack = 1 + 16.0f / grid.Scale;

	int max_radius = MAX_RADIUS + ceil(mapslack);

	int lx = x - max_radius;
	int ly = y - max_radius;
	int hx = x + max_radius;
	int hy = y + max_radius;

	int best = -1;
	thing_comparer_t best_comp;

	for (int n = 0 ; n < NumThings ; n++)
	{
		int tx = Things[n]->x;
		int ty = Things[n]->y;

		// filter out things that are outside the search bbox.
		// this search box is enlarged by MAX_RADIUS.
		if (tx < lx || tx > hx || ty < ly || ty > hy)
			continue;

		const thingtype_t *info = M_GetThingType(Things[n]->type);

		// more accurate bbox test using the real radius
		int r = info->radius + mapslack;

		if (x < tx - r - mapslack || x > tx + r + mapslack ||
			y < ty - r - mapslack || y > ty + r + mapslack)
			continue;

		thing_comparer_t th_comp;

		th_comp.distance = hypot(x - tx, y - ty);
		th_comp.radius   = r;
		th_comp.inside   = (x > tx - r && x < tx + r && y > ty - r && y < ty + r);

		if (best < 0 || th_comp <= best_comp)
		{
			best = n;
			best_comp = th_comp;
		}
	}

	if (best >= 0)
		return Objid(OBJ_THINGS, best);

	// none found
	return Objid();
}


//
// determine which vertex is under the pointer
//
static Objid NearestVertex(float x, float y)
{
	const int screen_pix = vertex_radius(grid.Scale);

	float mapslack = 1 + (4 + screen_pix) / grid.Scale;

	// workaround for overly zealous highlighting when zoomed in far
	if (grid.Scale >= 15.0) mapslack *= 0.7;
	if (grid.Scale >= 31.0) mapslack *= 0.5;

	int lx = floor(x - mapslack);
	int ly = floor(y - mapslack);
	int hx =  ceil(x + mapslack);
	int hy =  ceil(y + mapslack);

	int    best = -1;
	double best_dist = 9e9;

	for (int n = 0 ; n < NumVertices ; n++)
	{
		int vx = Vertices[n]->x;
		int vy = Vertices[n]->y;

		// filter out vertices that are outside the search bbox
		if (vx < lx || vx > hx || vy < ly || vy > hy)
			continue;

		double dist = hypot(x - vx, y - vy);

		if (dist > mapslack)
			continue;

		// use "<=" because if there are superimposed vertices, we want
		// to return the highest-numbered one.
		if (dist <= best_dist)
		{
			best = n;
			best_dist = dist;
		}
	}

	if (best >= 0)
		return Objid(OBJ_VERTICES, best);

	// none found
	return Objid();
}


//
//  sets 'o' to which object is under the pointer at the given
//  coordinates.  when several objects are close, the smallest
//  is chosen.
//
void GetNearObject(Objid& o, obj_type_e objtype, float x, float y)
{
	switch (objtype)
	{
		case OBJ_THINGS:
		{
			o = NearestThing(x, y);
			break;
		}

		case OBJ_VERTICES:
		{
			o = NearestVertex(x, y);
			break;
		}

		case OBJ_LINEDEFS:
		{
			o = NearestLineDef(x, y);
			break;
		}

		case OBJ_SECTORS:
		{
			o = NearestSector(x, y);
			break;
		}

		default:
			BugError("GetNearObject: bad objtype %d\n", (int) objtype);
			break; /* NOT REACHED */
	}
}


void GetSplitLineDef(Objid& o, int x, int y, int drag_vert)
{
	o = NearestSplitLine(x, y, drag_vert);

	// don't highlight the line if the new vertex would snap onto
	// the same coordinate as the start or end of the linedef.
	// [ I tried a bbox test here, but it's bad for axis-aligned lines ]

	if (o.valid() && grid.snap)
	{
		int snap_x = grid.SnapX(x);
		int snap_y = grid.SnapY(y);

		const LineDef * L = LineDefs[o.num];

		if ( (L->Start()->x == snap_x && L->Start()->y == snap_y) ||
			 (L->  End()->x == snap_x && L->  End()->y == snap_y) )
		{
			o.clear();
		}

		// also require snap coordinate be not TOO FAR from the line
		double len = L->CalcLength();

		double along = AlongDist(snap_x, snap_y, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);
		double  perp =  PerpDist(snap_x, snap_y, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);

		if (along <= 0 || along >= len || fabs(perp) > len * 0.2)
		{
			o.clear();
		}
	}
}


void GetSplitLineForDangler(Objid& o, int v_num)
{
	o = NearestSplitLine(Vertices[v_num]->x, Vertices[v_num]->y, v_num);
}


//------------------------------------------------------------------------


crossing_state_c::crossing_state_c() : points()
{ }


crossing_state_c::~crossing_state_c()
{ }


void crossing_state_c::clear()
{
	points.clear();
}


void crossing_state_c::add_vert(int v, double dist)
{
	// FIXME
}

void crossing_state_c::add_line(int ld, int ix, int iy, double dist)
{
	// FIXME
}


bool crossing_state_c::HasVertex(int v) const
{
	for (unsigned int k = 0 ; k < points.size() ; k++)
		if (points[k].vert == v)
			return true;

	return false;  // NOPE!
}

bool crossing_state_c::HasLine(int ld) const
{
	for (unsigned int k = 0 ; k < points.size() ; k++)
		if (points[k].ld == ld)
			return true;

	return false;  // NO WAY!
}


void crossing_state_c::Sort()
{
	// FIXME
}


void crossing_state_c::SplitAllLines()
{
	for (unsigned int i = 0 ; i < points.size() ; i++)
	{
		if (points[i].ld >= 0)
		{
			points[i].vert = BA_New(OBJ_VERTICES);

			Vertex *V = Vertices[points[i].vert];

			V->x = points[i].x;
			V->y = points[i].y;

			SplitLineDefAtVertex(points[i].ld, points[i].vert);
		}
	}
}


#if 0
static void find_closest_cross_handler(int x, int y, double dist, int v, int ld, void *data)
{
	cross_state_t *cross = (cross_state_t *)data;

	if (dist >= cross->distance)
		return;

	cross->distance = dist;

	if (v >= 0)
	{
		cross->vert = v;
		cross->line = -1;

		cross->x = x;
		cross->y = y;

	}
	else
	{
		cross->vert = -1;
		cross->line = ld;

		cross->x = x;
		cross->y = y;
	}
}


bool FindClosestCrossPoint(int v1, int v2, cross_state_t *cross)
{
	SYS_ASSERT(v1 != v2);

	cross->vert = -1;
	cross->line = -1;
	cross->distance = 9e9;

	const Vertex *VA = Vertices[v1];
	const Vertex *VB = Vertices[v2];

	FindAllCrossPoints(VA->x, VA->y, v1, VB->x, VB->y, v2,
					   find_closest_cross_handler, cross);

	return (cross->vert >= 0) || (cross->line >= 0);
}
#endif


#define CROSSING_EPSILON  0.8
#define    ALONG_EPSILON  0.4


static void FindCrossingLines(crossing_state_c& cross,
						int x1, int y1, int possible_v1,
						int x2, int y2, int possible_v2)
{
	// FIXME: don't duplicate this shit
	// when zooming out, make it easier to hit a vertex
	double sk = 1.0 / grid.Scale;
	double close_dist = 8 * sqrt(sk);
	close_dist = CLAMP(1.2, close_dist, 24.0);


	int dx = x2 - x1;
	int dy = y2 - y1;

	// this could happen when two vertices are overlapping
	if (dx == 0 && dy == 0)
		return;

	double length = sqrt(dx*dx + dy*dy);


	for (int ld = 0 ; ld < NumLineDefs ; ld++)
	{
		const LineDef * L = LineDefs[ld];

		double lx1 = L->Start()->x;
		double ly1 = L->Start()->y;
		double lx2 = L->End()->x;
		double ly2 = L->End()->y;

		// FIXME: quick bbox test

		// skip linedef if an end-point is one of the vertices already
		// in the crossing state (including the very start or very end).
		if (L->TouchesCoord(x1, y1) ||
			L->TouchesCoord(x2, y2))
			continue;

		if (L->TouchesCoord(cross.start_x, cross.start_y) ||
			L->TouchesCoord(cross.  end_x, cross.  end_y))
			continue;

		if (cross.HasLine(ld))
			continue;

		if (cross.HasVertex(L->start) || cross.HasVertex(L->end))
			continue;

		// only need to handle cases where this linedef distinctly crosses
		// the new line (i.e. start and end are clearly on opposite sides).

		double a = PerpDist(lx1,ly1, x1,y1, x2,y2);
		double b = PerpDist(lx2,ly2, x1,y1, x2,y2);

		if (! ((a < -CROSSING_EPSILON && b >  CROSSING_EPSILON) ||
			   (a >  CROSSING_EPSILON && b < -CROSSING_EPSILON)))
			continue;

		// compute intersection point
		double l_along = a / (a - b);

		double ix = lx1 + l_along * (lx2 - lx1);
		double iy = ly1 + l_along * (ly2 - ly1);

		int new_x = I_ROUND(ix);
		int new_y = I_ROUND(iy);

		// ensure new vertex does not match the start or end points
		if (new_x == x1 && new_y == y1) continue;
		if (new_x == x2 && new_y == y2) continue;

		double along = AlongDist(new_x, new_y,  x1,y1, x2,y2);

		if (along < ALONG_EPSILON || along > length - ALONG_EPSILON)
			continue;

		// allow vertices to win over a nearby linedef
		along += close_dist * 2;

		// OK, this linedef crosses it

		cross.add_line(ld, new_x, new_y, along);
	}
}


void FindCrossingPoints(crossing_state_c& cross,
						int x1, int y1, int possible_v1,
						int x2, int y2, int possible_v2)
{
	cross.clear();

	cross.start_x = x1;
	cross.start_y = y1;
	cross.  end_x = x2;
	cross.  end_y = y2;


	// FIXME: don't duplicate this shit
	// when zooming out, make it easier to hit a vertex
	double sk = 1.0 / grid.Scale;
	double close_dist = 8 * sqrt(sk);
	close_dist = CLAMP(1.2, close_dist, 24.0);


	int dx = x2 - x1;
	int dy = y2 - y1;

	// same coords?  (sanity check)
	if (dx == 0 && dy == 0)
		return;

	double length = sqrt(dx*dx + dy*dy);


	/* must do all vertices FIRST */

	for (int v = 0 ; v < NumVertices ; v++)
	{
		if (v == possible_v1 || v == possible_v2)
			continue;

		const Vertex * VC = Vertices[v];

		// ignore vertices ar same coordinates as v1 or v2
		if (VC->Matches(x1, y1) || VC->Matches(x2, y2))
			continue;

		// is this vertex sitting on the line?
		double perp = PerpDist(VC->x, VC->y, x1,y1, x2,y2);

		if (fabs(perp) > close_dist)
			continue;

		double along = AlongDist(VC->x, VC->y, x1,y1, x2,y2);

		if (along < ALONG_EPSILON || along > length - ALONG_EPSILON)
			continue;

		cross.add_vert(v, along);
	}

	cross.Sort();


	int cur_x1 = x1;
	int cur_y1 = y1;
	int cur_v  = possible_v1;

	// grab number of points now, since we will add the linedef splits
	unsigned int num_verts = cross.points.size();

	for (unsigned int k = 0 ; k < num_verts ; k++)
	{
		cross_point_t& next_p = cross.points[k];

		FindCrossingLines(cross, cur_x1, cur_y1, cur_v,
						  next_p.x, next_p.y, next_p.vert);

		cur_x1 = next_p.x;
		cur_y1 = next_p.y;
		cur_v  = next_p.vert;
	}

	FindCrossingLines(cross, cur_x1, cur_y1, cur_v,
	                  x2, y2, possible_v2);

	cross.Sort();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
