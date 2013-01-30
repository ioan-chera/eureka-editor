//------------------------------------------------------------------------
//  HIGHLIGHT HELPER
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

#include <algorithm>

#include "r_grid.h"
#include "levels.h"
#include "m_game.h"
#include "x_hover.h"


float ApproxDistToLineDef(const LineDef * L, int x, int y)
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


int OppositeLineDef(int ld, int ld_side, int *result_side)
{
	// ld_side is -1 for left, +1 for right.
	// result_side uses the same values (never 0).

	const LineDef * L = LineDefs[ld];

	int dx = L->End()->x - L->Start()->x;
	int dy = L->End()->y - L->Start()->y;

	if (dx == 0 && dy == 0)
		return -1;

	float x = L->Start()->x + dx / 2.0;
	float y = L->Start()->y + dy / 2.0;

	int   best_match = -1;
	float best_dist  = 9e9;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		if (ld == n)  // ignore input line
			continue;

		int nx1 = LineDefs[n]->Start()->x;
		int ny1 = LineDefs[n]->Start()->y;
		int nx2 = LineDefs[n]->End()->x;
		int ny2 = LineDefs[n]->End()->y;

		if (abs(dy) >= abs(dx))  // casting a horizontal ray
		{
			if (MIN(ny1, ny2) >= y + 1 || MAX(ny1, ny2) <= y)
				continue;

			float dist = nx1 - (x + 0.5) + (nx2 - nx1) * (y + 0.5 - ny1) / (float)(ny2 - ny1);

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
			if (MIN(nx1, nx2) >= x + 1 || MAX(nx1, nx2) <= x)
				continue;

			float dist = ny1 - (y + 0.5) + (ny2 - ny1) * (x + 0.5 - nx1) / (float)(nx2 - nx1);

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

	return best_match;
}


// result: -1 for back, +1 for front, 0 for _exactly_on_ the line
int PointOnLineSide(int x, int y, int lx1, int ly1, int lx2, int ly2)
{
	x   -= lx1; y   -= ly1;
	lx2 -= lx1; ly2 -= ly1;

	int tmp = (x * ly2 - y * lx2);

	return (tmp < 0) ? -1 : (tmp > 0) ? +1 : 0;
}


class Close_obj
{
public :
	Objid  obj;
	double distance;
	bool   inside;
	int    radius;

	Close_obj()
	{
		clear();
	}

	void clear()
	{
		obj.clear();
		distance = 9e9;
		radius   = (1 << 30);
		inside   = false;
	}

	bool operator== (const Close_obj& other) const
	{
		return (inside == other.inside &&
			    radius == other.radius &&
			    distance == other.distance) ? true : false;
	}

	bool operator< (const Close_obj& other) const
	{
		if (inside && ! other.inside) return true;
		if (! inside && other.inside) return false;

		// Small objects should "mask" large objects
		if (radius < other.radius) return true;
		if (radius > other.radius) return false;

		if (distance < other.distance) return true;
		if (distance > other.distance) return false;

		return radius < other.radius;
	}

	bool operator<= (const Close_obj& other) const
	{
		return *this == other || *this < other;
	}
};


/*
 *  get_cur_linedef - determine which linedef is under the pointer
 */
static void get_cur_linedef(Close_obj& closest, int x, int y)
{
	// slack in map units
	int mapslack = 2 + (int)ceil(16.0f / grid.Scale);

	int lx = x - mapslack;
	int ly = y - mapslack;
	int hx = x + mapslack;
	int hy = y + mapslack;

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

		// "<=" because if there are superimposed vertices, we want to
		// return the highest-numbered one.
		if (dist > closest.distance)
			continue;

		closest.obj.type = OBJ_LINEDEFS;
		closest.obj.num  = n;
		closest.distance = dist;
	}

#if 0  // TESTING CRUD
	if (closest.obj.type == OBJ_LINEDEFS)
	{
		closest.obj.num = OppositeLineDef(closest.obj.num, +1, NULL);

		if (closest.obj.num < 0)
			closest.clear();
	}
#endif
}


/*
 *  get_split_linedef - determine which linedef would be split if a
 *                      new vertex was added to the given point.
 */
static void get_split_linedef(Close_obj& closest, int x, int y, int drag_vert)
{
	// slack in map units
	int mapslack = 1 + (int)ceil(8.0f / grid.Scale);

	int lx = x - mapslack;
	int ly = y - mapslack;
	int hx = x + mapslack;
	int hy = y + mapslack;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		// ignore the dragging vertex (drag_vert can be -1 for none)
		if (L->start == drag_vert || L->end == drag_vert)
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

		float dist = ApproxDistToLineDef(L, x, y);

		if (dist > mapslack)
			continue;

		if (dist > closest.distance)
			continue;

		closest.obj.type = OBJ_LINEDEFS;
		closest.obj.num  = n;
		closest.distance = dist;
	}
}


/*
 *  get_cur_sector - determine which sector is under the pointer
 */
static void get_cur_sector(Close_obj& closest,int x, int y)
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
		{
			closest.obj.type = OBJ_SECTORS;
			closest.obj.num  = SideDefs[sd_num]->sector;
		}
	}
}


/*
 *  get_cur_thing - determine which thing is under the pointer
 */
static void get_cur_thing(Close_obj& closest, int x, int y)
{
	int mapslack = 1 + (int)ceil(16.0f / grid.Scale);

	int max_radius = MAX_RADIUS + mapslack;

	int lx = x - max_radius;
	int ly = y - max_radius;
	int hx = x + max_radius;
	int hy = y + max_radius;

	for (int n = 0 ; n < NumThings ; n++)
	{
		int tx = Things[n]->x;
		int ty = Things[n]->y;

		// Filter out things that are farther than <max_radius> units away.
		if (tx < lx || tx > hx || ty < ly || ty > hy)
			continue;

		const thingtype_t *info = M_GetThingType(Things[n]->type);

		// So how far is that thing exactly ?

		int thing_radius = info->radius + mapslack;

		if (x < tx - thing_radius || x > tx + thing_radius ||
		    y < ty - thing_radius || y > ty + thing_radius)
			continue;

		Close_obj current;

		current.obj.type = OBJ_THINGS;
		current.obj.num  = n;
		current.distance = hypot(x - tx, y - ty);
		current.radius   = info->radius;
		current.inside   = x > tx - current.radius
			&& x < tx + current.radius
			&& y > ty - current.radius
			&& y < ty + current.radius;

		// "<=" because if there are superimposed vertices, we want to
		// return the highest-numbered one.
		if (current <= closest)
			closest = current;
	}
}


/*
 *  get_cur_vertex - determine which vertex is under the pointer
 */
static void get_cur_vertex(Close_obj& closest, int x, int y, bool snap)
{
	const int screen_pix = vertex_radius(grid.Scale);

	int mapslack = 1 + (int)ceil((4 + screen_pix) / grid.Scale);

	if (snap)
		mapslack = grid.step / 2 + 1;

	int lx = x - mapslack;
	int ly = y - mapslack;
	int hx = x + mapslack;
	int hy = y + mapslack;

	for (int n = 0 ; n < NumVertices ; n++)
	{
		int vx = Vertices[n]->x;
		int vy = Vertices[n]->y;

		/* Filter out objects that are farther than <radius> units away. */
		if (vx < lx || vx > hx || vy < ly || vy > hy)
			continue;

		double dist = hypot(x - vx, y - vy);

		// "<=" because if there are superimposed vertices, we want to
		// return the highest-numbered one.
		if (dist > closest.distance)
			continue;

		closest.obj.type = OBJ_VERTICES;
		closest.obj.num  = n;
		closest.distance = dist;
	}
}


/*
 *  get_cur_rts - determine which RTS trigger is under the pointer
 */
static void get_cur_rts(Close_obj& closest, int x, int y)
{
// TODO: get_cur_rts

#if 0
	const int screenradius = vertex_radius (grid.Scale);  // Radius in pixels
	const int screenslack  = screenradius + 16;   // Slack in pixels
	double    mapslack     = fabs (screenslack / grid.Scale); // Slack in map units

	int lx = x - mapslack;
	int ly = y - mapslack;
	int hx = x + mapslack;
	int hy = y + mapslack;
#endif
}


/*
 *  GetCurObject - determine which object is under the pointer
 * 
 *  Set <o> to point to the object under the pointer (map
 *  coordinates (<x>, <y>). If several objects are close
 *  enough to the pointer, the smallest object is chosen.
 */
void GetCurObject(Objid& o, obj_type_e objtype, int x, int y, bool snap)
{
	Close_obj closest;

	switch (objtype)
	{
		case OBJ_THINGS:
		{
			get_cur_thing(closest, x, y);
			o = closest.obj;
			break;
		}

		case OBJ_VERTICES:
		{
			get_cur_vertex(closest, x, y, snap);
			o = closest.obj;
			break;
		}

		case OBJ_LINEDEFS:
		{
			get_cur_linedef(closest, x, y);
			o = closest.obj;
			break;
		}

		case OBJ_SECTORS:
		{
			get_cur_sector(closest, x, y);
			o = closest.obj;
			break;
		}

		case OBJ_RADTRIGS:
		{
			get_cur_rts(closest, x, y);
			o = closest.obj;
			break;
		}

		default:
			BugError("GetCurObject: bad objtype %d", (int) objtype);
			break; /* NOT REACHED */
	}
}


void GetSplitLineDef(Objid& o, int x, int y, int drag_vert)
{
	Close_obj closest;

	get_split_linedef(closest, x, y, drag_vert);

	o = closest.obj;

	// don't highlight the line if the new vertex would snap onto
	// the same coordinate as the start or end of the linedef.
	// (I tried a bbox test here, but it's bad for axis-aligned lines)

	if (o() && grid.snap)
	{
		int snap_x = grid.SnapX(x);
		int snap_y = grid.SnapY(y);

		const LineDef * L = LineDefs[o.num];

		if ( (L->Start()->x == snap_x && L->Start()->y == snap_y) ||
			 (L->  End()->x == snap_x && L->  End()->y == snap_y) )
		{
			o.clear();
		}
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
