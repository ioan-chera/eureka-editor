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

#include "Errors.hpp"
#include "main.h"

#include <algorithm>

#include "e_hover.h"
#include "e_linedef.h"	// SplitLineDefAtVertex()
#include "e_main.h"		// Map_bound_xxx
#include "m_game.h"
#include "r_grid.h"


extern int vertex_radius(double scale);


double ApproxDistToLineDef(const LineDef * L, double x, double y)
{
	double x1 = L->Start()->x();
	double y1 = L->Start()->y();
	double x2 = L->End()->x();
	double y2 = L->End()->y();

	double dx = x2 - x1;
	double dy = y2 - y1;

	if (fabs(dx) > fabs(dy))
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
		double y3 = y1 + (x - x1) * dy / dx;

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

		double x3 = x1 + (y - y1) * dx / dy;

		return fabs(x3 - x);
	}
}


int ClosestLine_CastingHoriz(double x, double y, Side *side)
{
	int    best_match = -1;
	double best_dist  = 9e9;

	// most lines have integral X coords, so offset slightly to
	// avoid hitting vertices.
	y += 0.04;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		double ly1 = gDocument.linedefs[n]->Start()->y();
		double ly2 = gDocument.linedefs[n]->End()->y();

		// ignore purely horizontal lines
		if (ly1 == ly2)
			continue;

		// does the linedef cross the horizontal ray?
		if (MIN(ly1, ly2) >= y || MAX(ly1, ly2) <= y)
			continue;

		double lx1 = gDocument.linedefs[n]->Start()->x();
		double lx2 = gDocument.linedefs[n]->End()->x();

		double dist = lx1 - x + (lx2 - lx1) * (y - ly1) / (ly2 - ly1);

		if (fabs(dist) < best_dist)
		{
			best_match = n;
			best_dist  = fabs(dist);

			if (side)
			{
				if (best_dist < 0.01)
					*side = Side::neither;  // on the line
				else if ( (ly1 > ly2) == (dist > 0))
					*side = Side::right;  // right side
				else
					*side = Side::left; // left side
			}
		}
	}

	return best_match;
}


int ClosestLine_CastingVert(double x, double y, Side *side)
{
	int    best_match = -1;
	double best_dist  = 9e9;

	// most lines have integral X coords, so offset slightly to
	// avoid hitting vertices.
	x += 0.04;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		double lx1 = gDocument.linedefs[n]->Start()->x();
		double lx2 = gDocument.linedefs[n]->End()->x();

		// ignore purely vertical lines
		if (lx1 == lx2)
			continue;

		// does the linedef cross the vertical ray?
		if (MIN(lx1, lx2) >= x || MAX(lx1, lx2) <= x)
			continue;

		double ly1 = gDocument.linedefs[n]->Start()->y();
		double ly2 = gDocument.linedefs[n]->End()->y();

		double dist = ly1 - y + (ly2 - ly1) * (x - lx1) / (lx2 - lx1);

		if (fabs(dist) < best_dist)
		{
			best_match = n;
			best_dist  = fabs(dist);

			if (side)
			{
				if (best_dist < 0.01)
					*side = Side::neither;  // on the line
				else if ( (lx1 > lx2) == (dist < 0))
					*side = Side::right;  // right side
				else
					*side = Side::left; // left side
			}
		}
	}

	return best_match;
}


int ClosestLine_CastAtAngle(double x, double y, float radians)
{
	int    best_match = -1;
	double best_dist  = 9e9;

	double x2 = x + 256 * cos(radians);
	double y2 = y + 256 * sin(radians);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = gDocument.linedefs[n];

		double a = PerpDist(L->Start()->x(), L->Start()->y(),  x, y, x2, y2);
		double b = PerpDist(L->  End()->x(), L->  End()->y(),  x, y, x2, y2);

		// completely on one side of the vector?
		if (a > 0 && b > 0) continue;
		if (a < 0 && b < 0) continue;

		double c = AlongDist(L->Start()->x(), L->Start()->y(),  x, y, x2, y2);
		double d = AlongDist(L->  End()->x(), L->  End()->y(),  x, y, x2, y2);

		double dist;

		if (fabs(a) < 1 && fabs(b) < 1)
			dist = MIN(c, d);
		else if (fabs(a) < 1)
			dist = c;
		else if (fabs(b) < 1)
			dist = d;
		else
		{
			double factor = a / (a - b);
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


bool PointOutsideOfMap(double x, double y)
{
	// this keeps track of directions tested
	int dirs = 0;

	// most end-points will be integral, so look in-between
	double x2 = x + 0.04;
	double y2 = y + 0.04;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		double lx1 = gDocument.linedefs[n]->Start()->x();
		double ly1 = gDocument.linedefs[n]->Start()->y();
		double lx2 = gDocument.linedefs[n]->End()->x();
		double ly2 = gDocument.linedefs[n]->End()->y();

		// does the linedef cross the horizontal ray?
		if (MIN(ly1, ly2) < y2 && MAX(ly1, ly2) > y2)
		{
			double dist = lx1 - x + (lx2 - lx1) * (y2 - ly1) / (ly2 - ly1);

			dirs |= (dist < 0) ? 1 : 2;

			if (dirs == 15) return false;
		}

		// does the linedef cross the vertical ray?
		if (MIN(lx1, lx2) < x2 && MAX(lx1, lx2) > x2)
		{
			double dist = ly1 - y + (ly2 - ly1) * (x2 - lx1) / (lx2 - lx1);

			dirs |= (dist < 0) ? 4 : 8;

			if (dirs == 15) return false;
		}
	}

	return true;
}


//------------------------------------------------------------------------

#define FASTOPP_DIST  320


struct opp_test_state_t
{
	int ld;
	Side ld_side;   // a SIDE_XXX value

	Side * result_side;

	double dx, dy;

	// origin of casting line
	double x, y;

	// casting line is horizontal?
	bool cast_horizontal;

	int    best_match;
	double best_dist;

public:
	void ComputeCastOrigin()
	{
		// choose a coordinate on the source line near the middle, but make
		// sure the casting line is not integral (i.e. lies between two lines
		// on the unit grid) so that we never directly hit a vertex.

		const LineDef * L = gDocument.linedefs[ld];

		dx = L->End()->x() - L->Start()->x();
		dy = L->End()->y() - L->Start()->y();

		cast_horizontal = fabs(dy) >= fabs(dx);

		x = L->Start()->x() + dx * 0.5;
		y = L->Start()->y() + dy * 0.5;

		if (cast_horizontal && fabs(dy) > 0)
		{
			if (fabs(dy) >= 2.0) {
				y = y + 0.4;
				x = x + 0.4 * dx / dy;
			}
		}

		if (!cast_horizontal && fabs(dx) > 0)
		{
			if (fabs(dx) >= 2.0) {
				x = x + 0.4;
				y = y + 0.4 * dy / dx;
			}
		}
	}

	void ProcessLine(int n)
	{
		if (ld == n)  // ignore input line
			return;

		double nx1 = gDocument.linedefs[n]->Start()->x();
		double ny1 = gDocument.linedefs[n]->Start()->y();
		double nx2 = gDocument.linedefs[n]->End()->x();
		double ny2 = gDocument.linedefs[n]->End()->y();

		if (cast_horizontal)
		{
			if (ny1 == ny2)
				return;

			if (MIN(ny1, ny2) > y || MAX(ny1, ny2) < y)
				return;

			double dist = nx1 + (nx2 - nx1) * (y - ny1) / (ny2 - ny1) - x;

			if ( (dy < 0) == (ld_side == Side::right) )
				dist = -dist;

			if (dist > 0 && dist < best_dist)
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

			double dist = ny1 + (ny2 - ny1) * (x - nx1) / (nx2 - nx1) - y;

			if ( (dx > 0) == (ld_side == Side::right) )
				dist = -dist;

			if (dist > 0 && dist < best_dist)
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

};


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

private:
	void Subdivide()
	{
		if (hi - lo <= FASTOPP_DIST)
			return;

		lo_child = new fastopp_node_c(lo, mid);
		hi_child = new fastopp_node_c(mid, hi);
	}

public:
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
		const LineDef *L = gDocument.linedefs[ld];

		// can ignore purely vertical lines
		if (L->IsVertical())
			return;

		double x1 = MIN(L->Start()->x(), L->End()->x());
		double x2 = MAX(L->Start()->x(), L->End()->x());

		AddLine_X(ld, (int)floor(x1), (int)ceil(x2));
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
		const LineDef *L = gDocument.linedefs[ld];

		// can ignore purely horizonal lines
		if (L->IsHorizontal())
			return;

		double y1 = MIN(L->Start()->y(), L->End()->y());
		double y2 = MAX(L->Start()->y(), L->End()->y());

		AddLine_Y(ld, (int)floor(y1), (int)ceil(y2));
	}

	void Process(opp_test_state_t& test, double coord)
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

		if (coord < (double)mid)
			lo_child->Process(test, coord);
		else
			hi_child->Process(test, coord);
	}
};


static fastopp_node_c * fastopp_X_tree;
static fastopp_node_c * fastopp_Y_tree;


void FastOpposite_Begin()
{
	CalculateLevelBounds();

	fastopp_X_tree = new fastopp_node_c(static_cast<int>(Map_bound_x1 - 8), static_cast<int>(Map_bound_x2 + 8));
	fastopp_Y_tree = new fastopp_node_c(static_cast<int>(Map_bound_y1 - 8), static_cast<int>(Map_bound_y2 + 8));

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


int OppositeLineDef(int ld, Side ld_side, Side *result_side, const bitvec_c *ignore_lines)
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

		if (test.cast_horizontal)
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


int OppositeSector(int ld, Side ld_side)
{
	Side opp_side;

	int opp = OppositeLineDef(ld, ld_side, &opp_side);

	// can see the void?
	if (opp < 0)
		return -1;

	return gDocument.linedefs[opp]->WhatSector(opp_side);
}


// result: -1 for back, +1 for front, 0 for _exactly_on_ the line
Side PointOnLineSide(double x, double y, double lx1, double ly1, double lx2, double ly2)
{
	x   -= lx1; y   -= ly1;
	lx2 -= lx1; ly2 -= ly1;

	double tmp = (x * ly2 - y * lx2);

	return (tmp < 0) ? Side::left : (tmp > 0) ? Side::right : Side::neither;
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
static Objid NearestLineDef(double x, double y)
{
	// slack in map units
	double mapslack = 2.5 + 16.0f / grid.Scale;

	double lx = x - mapslack;
	double ly = y - mapslack;
	double hx = x + mapslack;
	double hy = y + mapslack;

	int    best = -1;
	double best_dist = 9e9;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		double x1 = gDocument.linedefs[n]->Start()->x();
		double y1 = gDocument.linedefs[n]->Start()->y();
		double x2 = gDocument.linedefs[n]->End()->x();
		double y2 = gDocument.linedefs[n]->End()->y();

		// Skip all lines of which all points are more than <mapslack>
		// units away from (x,y).  In a typical level, this test will
		// filter out all the linedefs but a handful.
		if (MAX(x1,x2) < lx || MIN(x1,x2) > hx ||
		    MAX(y1,y2) < ly || MIN(y1,y2) > hy)
			continue;

		double dist = ApproxDistToLineDef(gDocument.linedefs[n], x, y);

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
		return Objid(ObjType::linedefs, best);

	// none found
	return Objid();
}


//
// determine which linedef would be split if a new vertex were
// added at the given coordinates.
//
static Objid NearestSplitLine(double x, double y, int ignore_vert)
{
	// slack in map units
	double mapslack = 1.5 + ceil(8.0f / grid.Scale);

	double lx = x - mapslack;
	double ly = y - mapslack;
	double hx = x + mapslack;
	double hy = y + mapslack;

	int    best = -1;
	double best_dist = 9e9;

	double too_small = (Level_format == MAPF_UDMF) ? 0.2 : 4.0;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = gDocument.linedefs[n];

		if (L->start == ignore_vert || L->end == ignore_vert)
			continue;

		double x1 = L->Start()->x();
		double y1 = L->Start()->y();
		double x2 = L->End()->x();
		double y2 = L->End()->y();

		if (MAX(x1,x2) < lx || MIN(x1,x2) > hx ||
		    MAX(y1,y2) < ly || MIN(y1,y2) > hy)
			continue;

		// skip linedef if given point matches a vertex
		if (x == x1 && y == y1) continue;
		if (x == x2 && y == y2) continue;

		// skip linedef if too small to split
		if (fabs(x2 - x1) < too_small && fabs(y2 - y1) < too_small)
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
		return Objid(ObjType::linedefs, best);

	// none found
	return Objid();
}


//
//  determine which sector is under the pointer
//
static Objid NearestSector(double x, double y)
{
	/* hack, hack...  I look for the first LineDef crossing
	   an horizontal half-line drawn from the cursor */

	// -AJA- updated this to look in four directions (N/S/E/W) and
	//       grab the closest linedef.  Now it is possible to access
	//       self-referencing lines, even purely horizontal ones.

	Side side1, side2;

	int line1 = ClosestLine_CastingHoriz(x, y, &side1);
  	int line2 = ClosestLine_CastingVert (x, y, &side2);

	if (line2 < 0)
	{
		/* nothing needed */
	}
	else if (line1 < 0 ||
	         ApproxDistToLineDef(gDocument.linedefs[line2], x, y) <
	         ApproxDistToLineDef(gDocument.linedefs[line1], x, y))
	{
		line1 = line2;
		side1 = side2;
	}

	// grab the sector reference from the appropriate side
	// (Note that side1 = +1 for right, -1 for left, 0 for "on").
	if (line1 >= 0)
	{
		int sd_num = (side1 == Side::left) ? gDocument.linedefs[line1]->left : gDocument.linedefs[line1]->right;

		if (sd_num >= 0)
			return Objid(ObjType::sectors, gDocument.sidedefs[sd_num]->sector);
	}

	// none found
	return Objid();
}


//
// determine which thing is under the mouse pointer
//
static Objid NearestThing(double x, double y)
{
	double mapslack = 1 + 16.0f / grid.Scale;

	double max_radius = MAX_RADIUS + ceil(mapslack);

	double lx = x - max_radius;
	double ly = y - max_radius;
	double hx = x + max_radius;
	double hy = y + max_radius;

	int best = -1;
	thing_comparer_t best_comp;

	for (int n = 0 ; n < NumThings ; n++)
	{
		double tx = gDocument.things[n]->x();
		double ty = gDocument.things[n]->y();

		// filter out things that are outside the search bbox.
		// this search box is enlarged by MAX_RADIUS.
		if (tx < lx || tx > hx || ty < ly || ty > hy)
			continue;

		const thingtype_t &info = M_GetThingType(gDocument.things[n]->type);

		// more accurate bbox test using the real radius
		double r = info.radius + mapslack;

		if (x < tx - r - mapslack || x > tx + r + mapslack ||
			y < ty - r - mapslack || y > ty + r + mapslack)
			continue;

		thing_comparer_t th_comp;

		th_comp.distance = hypot(x - tx, y - ty);
		th_comp.radius   = (int)r;
		th_comp.inside   = (x > tx - r && x < tx + r && y > ty - r && y < ty + r);

		if (best < 0 || th_comp <= best_comp)
		{
			best = n;
			best_comp = th_comp;
		}
	}

	if (best >= 0)
		return Objid(ObjType::things, best);

	// none found
	return Objid();
}


//
// determine which vertex is under the pointer
//
static Objid NearestVertex(double x, double y)
{
	const int screen_pix = vertex_radius(grid.Scale);

	double mapslack = 1 + (4 + screen_pix) / grid.Scale;

	// workaround for overly zealous highlighting when zoomed in far
	if (grid.Scale >= 15.0) mapslack *= 0.7;
	if (grid.Scale >= 31.0) mapslack *= 0.5;

	double lx = x - mapslack - 0.5;
	double ly = y - mapslack - 0.5;
	double hx = x + mapslack + 0.5;
	double hy = y + mapslack + 0.5;

	int    best = -1;
	double best_dist = 9e9;

	for (int n = 0 ; n < NumVertices ; n++)
	{
		double vx = gDocument.vertices[n]->x();
		double vy = gDocument.vertices[n]->y();

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
		return Objid(ObjType::vertices, best);

	// none found
	return Objid();
}


//
//  sets 'o' to which object is under the pointer at the given
//  coordinates.  when several objects are close, the smallest
//  is chosen.
//
void GetNearObject(Objid& o, ObjType objtype, double x, double y)
{
	switch (objtype)
	{
		case ObjType::things:
		{
			o = NearestThing(x, y);
			break;
		}

		case ObjType::vertices:
		{
			o = NearestVertex(x, y);
			break;
		}

		case ObjType::linedefs:
		{
			o = NearestLineDef(x, y);
			break;
		}

		case ObjType::sectors:
		{
			o = NearestSector(x, y);
			break;
		}

		default:
			BugError("GetNearObject: bad objtype %d\n", (int) objtype);
			break; /* NOT REACHED */
	}
}


void FindSplitLine(Objid& out, double& out_x, double& out_y,
				   double ptr_x, double ptr_y, int ignore_vert)
{
	out_x = out_y = 0;

	out = NearestSplitLine(ptr_x, ptr_y, ignore_vert);

	if (! out.valid())
		return;

	const LineDef * L = gDocument.linedefs[out.num];

	double x1 = L->Start()->x();
	double y1 = L->Start()->y();
	double x2 = L->End()->x();
	double y2 = L->End()->y();

	double len = hypot(x2 - x1, y2 - y1);

	if (grid.ratio > 0 && edit.action == ACT_DRAW_LINE)
	{
		Vertex *V = gDocument.vertices[edit.draw_from.num];

		// convert ratio into a vector, use it to intersect the linedef
		double px1 = V->x();
		double py1 = V->y();

		double px2 = ptr_x;
		double py2 = ptr_y;

		grid.RatioSnapXY(px2, py2, px1, py1);

		if (fabs(px1 - px2) < 0.1 && fabs(py1 - py2) < 0.1)
		{
			out.clear();
			return;
		}

		// compute intersection point
		double c = PerpDist(x1, y1,  px1, py1, px2, py2);
		double d = PerpDist(x2, y2,  px1, py1, px2, py2);

		int c_side = (c < -0.02) ? -1 : (c > 0.02) ? +1 : 0;
		int d_side = (d < -0.02) ? -1 : (d > 0.02) ? +1 : 0;

		if (c_side * d_side >= 0)
		{
			out.clear();
			return;
		}

		c = c / (c - d);

		out_x = x1 + c * (x2 - x1);
		out_y = y1 + c * (y2 - y1);
	}
	else if (grid.snap)
	{
		// don't highlight the line if the new vertex would snap onto
		// the same coordinate as the start or end of the linedef.
		// [ I tried a bbox test here, but it was bad for axis-aligned lines ]

		out_x = grid.ForceSnapX(ptr_x);
		out_y = grid.ForceSnapY(ptr_y);

		// snapped onto an end point?
		if (L->TouchesCoord(TO_COORD(out_x), TO_COORD(out_y)))
		{
			out.clear();
			return;
		}

		// require snap coordinate be not TOO FAR from the line
		double perp = PerpDist(out_x, out_y, x1, y1, x2, y2);

		if (fabs(perp) > len * 0.2)
		{
			out.clear();
			return;
		}
	}
	else
	{
		// in FREE mode, ensure split point is directly on the linedef
		out_x = ptr_x;
		out_y = ptr_y;

		MoveCoordOntoLineDef(out.num, &out_x, &out_y);
	}

	// always ensure result is along the linedef (not off the ends)
	double along = AlongDist(out_x, out_y, x1, y1, x2, y2);

	if (along < 0.05 || along > len - 0.05)
	{
		out.clear();
		return;
	}
}


void FindSplitLineForDangler(Objid& out, int v_num)
{
	out = NearestSplitLine(gDocument.vertices[v_num]->x(), gDocument.vertices[v_num]->y(), v_num);
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
	cross_point_t pt;

	pt.vert = v;
	pt.ld   = -1;
	pt.x    = gDocument.vertices[v]->x();
	pt.y    = gDocument.vertices[v]->y();
	pt.dist = dist;

	points.push_back(pt);
}

void crossing_state_c::add_line(int ld, double new_x, double new_y, double dist)
{
	cross_point_t pt;

	pt.vert = -1;
	pt.ld   = ld;
	pt.x    = new_x;
	pt.y    = new_y;
	pt.dist = dist;

	points.push_back(pt);
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
	std::sort(points.begin(), points.end(), point_CMP());
}


void crossing_state_c::SplitAllLines()
{
	for (unsigned int i = 0 ; i < points.size() ; i++)
	{
		if (points[i].ld >= 0)
		{
			points[i].vert = gDocument.basis.addNew(ObjType::vertices);

			Vertex *V = gDocument.vertices[points[i].vert];

			V->SetRawXY(points[i].x, points[i].y);

			SplitLineDefAtVertex(points[i].ld, points[i].vert);
		}
	}
}


#define CROSSING_EPSILON  0.2
#define    ALONG_EPSILON  0.1


static void FindCrossingLines(crossing_state_c& cross,
						double x1, double y1, int possible_v1,
						double x2, double y2, int possible_v2)
{
	// this could happen when two vertices are overlapping
	if (fabs(x1 - x2) < ALONG_EPSILON && fabs(y1 - y2) < ALONG_EPSILON)
		return;

	// distances along WHOLE original line for this segment
	double along1 = AlongDist(x1, y1,  cross.start_x, cross.start_y, cross.end_x, cross.end_y);
	double along2 = AlongDist(x2, y2,  cross.start_x, cross.start_y, cross.end_x, cross.end_y);

	// bounding box of segment
	double bbox_x1 = MIN(x1, x2) - 0.25;
	double bbox_y1 = MIN(y1, y2) - 0.25;

	double bbox_x2 = MAX(x1, x2) + 0.25;
	double bbox_y2 = MAX(y1, y2) + 0.25;


	for (int ld = 0 ; ld < NumLineDefs ; ld++)
	{
		const LineDef * L = gDocument.linedefs[ld];

		double lx1 = L->Start()->x();
		double ly1 = L->Start()->y();
		double lx2 = L->End()->x();
		double ly2 = L->End()->y();

		// bbox test -- eliminate most lines from consideration
		if (MAX(lx1,lx2) < bbox_x1 || MIN(lx1,lx2) > bbox_x2 ||
		    MAX(ly1,ly2) < bbox_y1 || MIN(ly1,ly2) > bbox_y2)
		{
			continue;
		}

		if (L->IsZeroLength())
			continue;

		if (cross.HasLine(ld))
			continue;

		// skip linedef if an end-point is one of the vertices already
		// in the crossing state (including the very start or very end).
		if (cross.HasVertex(L->start) || cross.HasVertex(L->end))
			continue;

		// only need to handle cases where this linedef distinctly crosses
		// the new line (i.e. start and end are clearly on opposite sides).

		double a = PerpDist(lx1,ly1, x1,y1, x2,y2);
		double b = PerpDist(lx2,ly2, x1,y1, x2,y2);

		if (! ((a < -CROSSING_EPSILON && b >  CROSSING_EPSILON) ||
			   (a >  CROSSING_EPSILON && b < -CROSSING_EPSILON)))
		{
			continue;
		}

		// compute intersection point
		double l_along = a / (a - b);

		double new_x = lx1 + l_along * (lx2 - lx1);
		double new_y = ly1 + l_along * (ly2 - ly1);

		double along = AlongDist(new_x, new_y,  cross.start_x, cross.start_y, cross.end_x, cross.end_y);

		// ensure new vertex lies within this segment (and not too close to ends)
		if (along > along1 + ALONG_EPSILON && along < along2 - ALONG_EPSILON)
		{
			cross.add_line(ld, new_x, new_y, along);
		}
	}
}


void FindCrossingPoints(crossing_state_c& cross,
						double x1, double y1, int possible_v1,
						double x2, double y2, int possible_v2)
{
	cross.clear();

	cross.start_x = x1;
	cross.start_y = y1;
	cross.  end_x = x2;
	cross.  end_y = y2;


	// when zooming out, make it easier to hit a vertex
	double close_dist = 4 * sqrt(1.0 / grid.Scale);

	close_dist = CLAMP(1.0, close_dist, 12.0);


	double dx = x2 - x1;
	double dy = y2 - y1;

	double length = hypot(dx, dy);

	// same coords?  (sanity check)
	if (length < 0.01)
		return;


	/* must do all vertices FIRST */

	for (int v = 0 ; v < NumVertices ; v++)
	{
		if (v == possible_v1 || v == possible_v2)
			continue;

		const Vertex * VC = gDocument.vertices[v];

		// ignore vertices at same coordinates as v1 or v2
		if (VC->Matches(TO_COORD(x1), TO_COORD(y1)) ||
			VC->Matches(TO_COORD(x2), TO_COORD(y2)))
			continue;

		// is this vertex sitting on the line?
		double perp = PerpDist(VC->x(), VC->y(), x1,y1, x2,y2);

		if (fabs(perp) > close_dist)
			continue;

		double along = AlongDist(VC->x(), VC->y(), x1,y1, x2,y2);

		if (along > ALONG_EPSILON && along < length - ALONG_EPSILON)
		{
			cross.add_vert(v, along);
		}
	}

	cross.Sort();


	/* process each pair of adjacent vertices to find split lines */

	double cur_x1 = x1;
	double cur_y1 = y1;
	int    cur_v  = possible_v1;

	// grab number of points now, since we will adding split points
	// (at the end) and we only want vertices here.
	size_t num_verts = cross.points.size();

	for (size_t k = 0 ; k < num_verts ; k++)
	{
		double next_x2 = cross.points[k].x;
		double next_y2 = cross.points[k].y;
		double next_v  = cross.points[k].vert;

		FindCrossingLines(cross, cur_x1, cur_y1, static_cast<int>(cur_v),
						  next_x2, next_y2, static_cast<int>(next_v));

		cur_x1 = next_x2;
		cur_y1 = next_y2;
		cur_v  = static_cast<int>(next_v);
	}

	FindCrossingLines(cross, cur_x1, cur_y1, cur_v, x2, y2, possible_v2);

	cross.Sort();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
