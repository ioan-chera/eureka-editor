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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include <algorithm>

#include "e_hover.h"
#include "e_linedef.h"	// SplitLineDefAtVertex()
#include "e_main.h"		// Map_bound_xxx
#include "LineDef.h"
#include "m_game.h"
#include "r_grid.h"
#include "Thing.h"
#include "Vertex.h"


extern int vertex_radius(double scale);

//------------------------------------------------------------------------

#define FASTOPP_DIST  320


struct opp_test_state_t
{
	int ld = 0;
	Side ld_side = Side::right;   // a SIDE_XXX value

	Side * result_side = nullptr;

	double dx = 0, dy = 0;

	// origin of casting line
	double x = 0, y = 0;

	// casting line is horizontal?
	bool cast_horizontal = false;

	int    best_match = 0;
	double best_dist = 0;

	const Document &doc;

public:
	explicit opp_test_state_t(const Document &doc) : doc(doc)
	{
	}

	void ComputeCastOrigin()
	{
		SYS_ASSERT(ld >= 0 && ld < doc.numLinedefs());
		// choose a coordinate on the source line near the middle, but make
		// sure the casting line is not integral (i.e. lies between two lines
		// on the unit grid) so that we never directly hit a vertex.

		const auto L = doc.linedefs[ld];

		dx = doc.getEnd(*L).x() - doc.getStart(*L).x();
		dy = doc.getEnd(*L).y() - doc.getStart(*L).y();

		cast_horizontal = fabs(dy) >= fabs(dx);

		x = doc.getStart(*L).x() + dx * 0.5;
		y = doc.getStart(*L).y() + dy * 0.5;

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

		double nx1 = doc.getStart(*doc.linedefs[n]).x();
		double ny1 = doc.getStart(*doc.linedefs[n]).y();
		double nx2 = doc.getEnd(*doc.linedefs[n]).x();
		double ny2 = doc.getEnd(*doc.linedefs[n]).y();

		if (cast_horizontal)
		{
			if (ny1 == ny2)
				return;

			if (std::min(ny1, ny2) > y || std::max(ny1, ny2) < y)
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

			if (std::min(nx1, nx2) > x || std::max(nx1, nx2) < x)
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

void fastopp_node_c::Subdivide()
{
	if (hi - lo <= FASTOPP_DIST)
		return;

	lo_child = std::make_unique<fastopp_node_c>(lo, mid, doc);
	hi_child = std::make_unique<fastopp_node_c>(mid, hi, doc);
}

void fastopp_node_c::AddLine_X(int ld, int x1, int x2)
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

void fastopp_node_c::AddLine_X(int ld)
{
	const auto L = doc.linedefs[ld];

	// can ignore purely vertical lines
	if (doc.isVertical(*L))
		return;

	double x1 = std::min(doc.getStart(*L).x(), doc.getEnd(*L).x());
	double x2 = std::max(doc.getStart(*L).x(), doc.getEnd(*L).x());

	AddLine_X(ld, (int)floor(x1), (int)ceil(x2));
}

void fastopp_node_c::AddLine_Y(int ld, int y1, int y2)
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

void fastopp_node_c::AddLine_Y(int ld)
{
	const auto L = doc.linedefs[ld];

	// can ignore purely horizonal lines
	if (doc.isHorizontal(*L))
		return;

	double y1 = std::min(doc.getStart(*L).y(), doc.getEnd(*L).y());
	double y2 = std::max(doc.getStart(*L).y(), doc.getEnd(*L).y());

	AddLine_Y(ld, (int)floor(y1), (int)ceil(y2));
}

void fastopp_node_c::Process(opp_test_state_t& test, double coord) const
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
	double distance = 9e9;
	bool   inside = false;
	int    radius = 1 << 30;

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

static Objid getNearestThing(const Document &doc, const ConfigData &config,
							 const grid::State &grid, const v2double_t &pos);
static Objid getNearestVertex(const Document &doc, const grid::State &grid, const v2double_t &pos);
static Objid getNearestLinedef(const Document &doc, const grid::State &grid, const v2double_t &pos);

//
//  Returns the object which is under the pointer at the given
//  coordinates.  When several objects are close, the smallest
//  is chosen.
//
Objid hover::getNearbyObject(ObjType type, const Document &doc, const ConfigData &config,
							 const grid::State &grid, const v2double_t &pos)
{
	switch(type)
	{
	case ObjType::things:
		return getNearestThing(doc, config, grid, pos);

	case ObjType::vertices:
		return getNearestVertex(doc, grid, pos);

	case ObjType::linedefs:
		return getNearestLinedef(doc, grid, pos);

	case ObjType::sectors:
		return getNearestSector(doc, pos);

	default:
		BugError("Hover::getNearbyObject: bad objtype %d\n", (int)type);
		return Objid(); /* NOT REACHED */
	}
}

//
// Get the closest line, by casting horizontally
//
int hover::getClosestLine_CastingHoriz(const Document &doc, v2double_t pos, Side *side)
{
	int    best_match = -1;
	double best_dist = 9e9;

	// most lines have integral X coords, so offset slightly to
	// avoid hitting vertices.
	pos.y += 0.04;

	for(int n = 0; n < doc.numLinedefs(); n++)
	{
		v2double_t lpos1, lpos2;
		lpos1.y = doc.getStart(*doc.linedefs[n]).y();
		lpos2.y = doc.getEnd(*doc.linedefs[n]).y();

		// ignore purely horizontal lines
		if(lpos1.y == lpos2.y)
			continue;

		// does the linedef cross the horizontal ray?
		if(std::min(lpos1.y, lpos2.y) >= pos.y || std::max(lpos1.y, lpos2.y) <= pos.y)
			continue;

		lpos1.x = doc.getStart(*doc.linedefs[n]).x();
		lpos2.x = doc.getEnd(*doc.linedefs[n]).x();

		double dist = lpos1.x - pos.x + (lpos2.x - lpos1.x) * (pos.y - lpos1.y) / (lpos2.y - lpos1.y);

		if(fabs(dist) < best_dist)
		{
			best_match = n;
			best_dist = fabs(dist);

			if(side)
			{
				if(best_dist < 0.01)
					*side = Side::neither;  // on the line
				else if((lpos1.y > lpos2.y) == (dist > 0))
					*side = Side::right;  // right side
				else
					*side = Side::left; // left side
			}
		}
	}

	return best_match;
}

//
// Gets the closest line, casting vertically
//
static int getClosestLine_CastingVert(const Document &doc, v2double_t pos, Side *side)
{
	int    best_match = -1;
	double best_dist = 9e9;

	// most lines have integral X coords, so offset slightly to
	// avoid hitting vertices.
	pos.x += 0.04;

	for(int n = 0; n < doc.numLinedefs(); n++)
	{
		v2double_t lpos1, lpos2;
		lpos1.x = doc.getStart(*doc.linedefs[n]).x();
		lpos2.x = doc.getEnd(*doc.linedefs[n]).x();

		// ignore purely vertical lines
		if(lpos1.x == lpos2.x)
			continue;

		// does the linedef cross the vertical ray?
		if(std::min(lpos1.x, lpos2.x) >= pos.x || std::max(lpos1.x, lpos2.x) <= pos.x)
			continue;

		lpos1.y = doc.getStart(*doc.linedefs[n]).y();
		lpos2.y = doc.getEnd(*doc.linedefs[n]).y();

		double dist = lpos1.y - pos.y + (lpos2.y - lpos1.y) * (pos.x - lpos1.x) / (lpos2.x - lpos1.x);

		if(fabs(dist) < best_dist)
		{
			best_match = n;
			best_dist = fabs(dist);

			if(side)
			{
				if(best_dist < 0.01)
					*side = Side::neither;  // on the line
				else if((lpos1.x > lpos2.x) == (dist < 0))
					*side = Side::right;  // right side
				else
					*side = Side::left; // left side
			}
		}
	}

	return best_match;
}

static Objid getNearestSplitLine(const Document &doc, MapFormat format, const grid::State &grid,
								 const v2double_t &pos, int ignore_vert);

//
// Finds a split line
//
Objid hover::findSplitLine(const Document &doc, MapFormat format, const Editor_State_t &edit,
						   const grid::State &grid, v2double_t &out_pos, const v2double_t &ptr,
						   int ignore_vert)
{
	out_pos = {};

	Objid out = getNearestSplitLine(doc, format, grid, ptr, ignore_vert);

	if(!out.valid())
		return Objid();

	const auto L = doc.linedefs[out.num];

	v2double_t v1 = doc.getStart(*L).xy();
	v2double_t v2 = doc.getEnd(*L).xy();

	double len = (v2 - v1).hypot();

	if(grid.getRatio() > 0 && edit.action == EditorAction::drawLine)
	{
		const auto V = doc.vertices[edit.drawLine.from.num];

		// convert ratio into a vector, use it to intersect the linedef
		v2double_t ppos1 = V->xy();
		v2double_t ppos2 = ptr;

		grid.RatioSnapXY(ppos2, ppos1);

		if(fabs(ppos1.x - ppos2.x) < 0.1 && fabs(ppos1.y - ppos2.y) < 0.1)
			return Objid();

		// compute intersection point
		double c = PerpDist(v1, ppos1, ppos2);
		double d = PerpDist(v2, ppos1, ppos2);

		int c_side = (c < -0.02) ? -1 : (c > 0.02) ? +1 : 0;
		int d_side = (d < -0.02) ? -1 : (d > 0.02) ? +1 : 0;

		if(c_side * d_side >= 0)
			return Objid();

		c = c / (c - d);

		out_pos = v1 + (v2 - v1) * c;
	}
	else if(grid.snaps())
	{
		// don't highlight the line if the new vertex would snap onto
		// the same coordinate as the start or end of the linedef.
		// [ I tried a bbox test here, but it was bad for axis-aligned lines ]

		out_pos = v2double_t(grid.ForceSnap(ptr));

		// snapped onto an end point?
		if(doc.touchesCoord(*L, FFixedPoint(out_pos.x), FFixedPoint(out_pos.y)))
			return Objid();

		// require snap coordinate be not TOO FAR from the line
		double perp = PerpDist(out_pos, v1, v2);

		if(fabs(perp) > len * 0.2)
			return Objid();
	}
	else
	{
		// in FREE mode, ensure split point is directly on the linedef
		out_pos = ptr;

		linemod::moveCoordOntoLinedef(doc, out.num, out_pos);
	}

	// always ensure result is along the linedef (not off the ends)
	double along = AlongDist(out_pos, v1, v2);

	if(along < 0.05 || along > len - 0.05)
		return Objid();

	return out;
}

//
// Find a split line for a dangling vertex
//
Objid hover::findSplitLineForDangler(const Document &doc, MapFormat format,
									 const grid::State &grid, int v_num)
{
	return getNearestSplitLine(doc, format, grid, doc.vertices[v_num]->xy(), v_num);
}

//
// Get the opposite linedef
//
int Hover::getOppositeLinedef(int ld, Side ld_side, Side *result_side, const bitvec_c *ignore_lines, FastOppositeTree *tree) const
{
	// ld_side is either SIDE_LEFT or SIDE_RIGHT.
	// result_side uses the same values (never 0).

	opp_test_state_t  test(doc);

	test.ld = ld;
	test.ld_side = ld_side;
	test.result_side = result_side;

	// this sets dx and dy
	test.ComputeCastOrigin();

	if(test.dx == 0 && test.dy == 0)
		return -1;

	test.best_match = -1;
	test.best_dist = 9e9;

	if(tree)
	{
		// fast way : use the binary tree

		SYS_ASSERT(ignore_lines == NULL);

		if(test.cast_horizontal)
			tree->m_fastopp_Y_tree->Process(test, test.y);
		else
			tree->m_fastopp_X_tree->Process(test, test.x);
	}
	else
	{
		// normal way : test all linedefs

		for(int n = 0; n < doc.numLinedefs(); n++)
		{
			if(ignore_lines && ignore_lines->get(n))
				continue;

			test.ProcessLine(n);
		}
	}

	return test.best_match;
}

//
// Get oppossite sector
//
int Hover::getOppositeSector(int ld, Side ld_side, FastOppositeTree *tree) const
{
	Side opp_side;

	int opp = getOppositeLinedef(ld, ld_side, &opp_side, nullptr, tree);

	// can see the void?
	if(opp < 0)
		return -1;

	return doc.getSectorID(*doc.linedefs[opp], opp_side);
}

//
// Begin fast-opposite mode
//
FastOppositeTree::FastOppositeTree(Instance &inst)
{
	inst.level.CalculateLevelBounds();
	Document &doc = inst.level;

	m_fastopp_X_tree.emplace(static_cast<int>(inst.level.Map_bound1.x - 8), static_cast<int>(inst.level.Map_bound2.x + 8), doc);
	m_fastopp_Y_tree.emplace(static_cast<int>(inst.level.Map_bound1.y - 8), static_cast<int>(inst.level.Map_bound2.y + 8), doc);

	for(int n = 0; n < doc.numLinedefs(); n++)
	{
		m_fastopp_X_tree->AddLine_X(n);
		m_fastopp_Y_tree->AddLine_Y(n);
	}
}

//
// whether point is outside of map
//
bool hover::isPointOutsideOfMap(const Document &doc, const v2double_t &v)
{
	// this keeps track of directions tested
	int dirs = 0;

	// most end-points will be integral, so look in-between
	v2double_t v2 = v + v2double_t{ 0.04, 0.04 };

	for(int n = 0; n < doc.numLinedefs(); n++)
	{
		v2double_t lv1 = doc.getStart(*doc.linedefs[n]).xy();
		v2double_t lv2 = doc.getEnd(*doc.linedefs[n]).xy();

		// does the linedef cross the horizontal ray?
		if(std::min(lv1.y, lv2.y) < v2.y && std::max(lv1.y, lv2.y) > v2.y)
		{
			double dist = lv1.x - v.x + (lv2.x - lv1.x) * (v2.y - lv1.y) / (lv2.y - lv1.y);

			dirs |= (dist < 0) ? 1 : 2;

			if(dirs == 15) return false;
		}

		// does the linedef cross the vertical ray?
		if(std::min(lv1.x, lv2.x) < v2.x && std::max(lv1.x, lv2.x) > v2.x)
		{
			double dist = lv1.y - v.y + (lv2.y - lv1.y) * (v2.x - lv1.x) / (lv2.x - lv1.x);

			dirs |= (dist < 0) ? 4 : 8;

			if(dirs == 15) return false;
		}
	}

	return true;
}

#define CROSSING_EPSILON  0.2
#define    ALONG_EPSILON  0.1

//
// Find crossing points
//
void Hover::findCrossingPoints(crossing_state_c &cross,
	v2double_t p1, int possible_v1,
	v2double_t p2, int possible_v2) const
{
	cross.clear();

	cross.start = p1;
	cross.end = p2;

	// when zooming out, make it easier to hit a vertex
	double close_dist = 4 * sqrt(1.0 / inst.grid.getScale());

	close_dist = clamp(1.0, close_dist, 12.0);


	double dx = p2.x - p1.x;
	double dy = p2.y - p1.y;

	double length = hypot(dx, dy);

	// same coords?  (sanity check)
	if(length < 0.01)
		return;


	/* must do all vertices FIRST */

	for(int v = 0; v < doc.numVertices(); v++)
	{
		if(v == possible_v1 || v == possible_v2)
			continue;

		const auto VC = doc.vertices[v];

		// ignore vertices at same coordinates as v1 or v2
		if(VC->Matches(FFixedPoint(p1.x), FFixedPoint(p1.y)) ||
			VC->Matches(FFixedPoint(p2.x), FFixedPoint(p2.y)))
			continue;

		// is this vertex sitting on the line?
		double perp = PerpDist(VC->xy(), p1, p2);

		if(fabs(perp) > close_dist)
			continue;

		double along = AlongDist(VC->xy(), p1, p2);

		if(along > ALONG_EPSILON && along < length - ALONG_EPSILON)
		{
			cross.add_vert(v, along);
		}
	}

	cross.Sort();


	/* process each pair of adjacent vertices to find split lines */

	v2double_t cur_pos1 = p1;
	int    cur_v = possible_v1;

	// grab number of points now, since we will adding split points
	// (at the end) and we only want vertices here.
	size_t num_verts = cross.points.size();

	for(size_t k = 0; k < num_verts; k++)
	{
		v2double_t next_pos2 = cross.points[k].pos;
		double next_v = cross.points[k].vert;

		findCrossingLines(cross, cur_pos1, static_cast<int>(cur_v),
						  next_pos2, static_cast<int>(next_v));

		cur_pos1 = next_pos2;
		cur_v = static_cast<int>(next_v);
	}

	findCrossingLines(cross, cur_pos1, cur_v, p2, possible_v2);

	cross.Sort();
}

//
// determine which thing is under the mouse pointer
//
static Objid getNearestThing(const Document &doc, const ConfigData &config,
							 const grid::State &grid, const v2double_t &pos)
{
	double mapslack = 1 + 16.0f / grid.getScale();

	double max_radius = MAX_RADIUS + ceil(mapslack);

	v2double_t lpos = pos - v2double_t(max_radius);
	v2double_t hpos = pos + v2double_t(max_radius);

	int best = -1;
	thing_comparer_t best_comp;

	for(int n = 0; n < doc.numThings(); n++)
	{
		const auto thing = doc.things[n];
		v2double_t tpos = thing->xy();

		// filter out things that are outside the search bbox.
		// this search box is enlarged by MAX_RADIUS.
		if(!tpos.inbounds(lpos, hpos))
			continue;

		const thingtype_t &info = config.getThingType(thing->type);

		// more accurate bbox test using the real radius
		double r = info.radius + mapslack;

		if(!pos.inbounds(tpos - v2double_t(r + mapslack), tpos + v2double_t(r + mapslack)))
			continue;

		thing_comparer_t th_comp;

		th_comp.distance = (pos - tpos).hypot();
		th_comp.radius = (int)r;
		th_comp.inside = pos.inboundsStrict(tpos - v2double_t(r), tpos + v2double_t(r));

		if(best < 0 || th_comp <= best_comp)
		{
			best = n;
			best_comp = th_comp;
		}
	}

	if(best >= 0)
		return Objid(ObjType::things, best);

	// none found
	return Objid();
}

//
// determine which vertex is under the pointer
//
static Objid getNearestVertex(const Document &doc, const grid::State &grid, const v2double_t &pos)
{
	const int screen_pix = vertex_radius(grid.getScale());

	double mapslack = 1 + (4 + screen_pix) / grid.getScale();

	// workaround for overly zealous highlighting when zoomed in far
	if(grid.getScale() >= 15.0) mapslack *= 0.7;
	if(grid.getScale() >= 31.0) mapslack *= 0.5;

	v2double_t lpos = pos - v2double_t(mapslack + 0.5);
	v2double_t hpos = pos + v2double_t(mapslack + 0.5);

	int    best = -1;
	double best_dist = 9e9;

	for(int n = 0; n < doc.numVertices(); n++)
	{
		v2double_t vpos = doc.vertices[n]->xy();

		// filter out vertices that are outside the search bbox
		if(!vpos.inbounds(lpos, hpos))
			continue;

		double dist = (pos - vpos).hypot();

		if(dist > mapslack)
			continue;

		// use "<=" because if there are superimposed vertices, we want
		// to return the highest-numbered one.
		if(dist <= best_dist)
		{
			best = n;
			best_dist = dist;
		}
	}

	if(best >= 0)
		return Objid(ObjType::vertices, best);

	// none found
	return Objid();
}

static double getApproximateDistanceToLinedef(const Document &doc, const LineDef &line, const v2double_t &pos);

//
// determine which linedef is under the pointer
//
static Objid getNearestLinedef(const Document &doc, const grid::State &grid, const v2double_t &pos)
{
	// slack in map units
	double mapslack = 2.5 + 16.0f / grid.getScale();

	v2double_t lpos = pos - v2double_t(mapslack);
	v2double_t hpos = pos + v2double_t(mapslack);

	int    best = -1;
	double best_dist = 9e9;

	for(int n = 0; n < doc.numLinedefs(); n++)
	{
		v2double_t pos1 = doc.getStart(*doc.linedefs[n]).xy();
		v2double_t pos2 = doc.getEnd(*doc.linedefs[n]).xy();

		// Skip all lines of which all points are more than <mapslack>
		// units away from (x,y).  In a typical level, this test will
		// filter out all the linedefs but a handful.
		if(std::max(pos1.x, pos2.x) < lpos.x || std::min(pos1.x, pos2.x) > hpos.x ||
		   std::max(pos1.y, pos2.y) < lpos.y || std::min(pos1.y, pos2.y) > hpos.y)
			continue;

		double dist = getApproximateDistanceToLinedef(doc, *doc.linedefs[n], pos);

		if(dist > mapslack)
			continue;

		// use "<=" because if there are overlapping linedefs, we want
		// to return the highest-numbered one.
		if(dist <= best_dist)
		{
			best = n;
			best_dist = dist;
		}
	}

	if(best >= 0)
		return Objid(ObjType::linedefs, best);

	// none found
	return Objid();
}

//
//  determine which sector is under the pointer
//
Objid hover::getNearestSector(const Document &doc, const v2double_t &pos)
{
	/* hack, hack...  I look for the first LineDef crossing
	   an horizontal half-line drawn from the cursor */

	   // -AJA- updated this to look in four directions (N/S/E/W) and
	   //       grab the closest linedef.  Now it is possible to access
	   //       self-referencing lines, even purely horizontal ones.

	Side side1, side2;

	int line1 = hover::getClosestLine_CastingHoriz(doc, pos, &side1);
	int line2 = getClosestLine_CastingVert(doc, pos, &side2);

	if(line2 < 0)
	{
		/* nothing needed */
	}
	else if(line1 < 0 ||
		getApproximateDistanceToLinedef(doc, *doc.linedefs[line2], pos) <
		getApproximateDistanceToLinedef(doc, *doc.linedefs[line1], pos))
	{
		line1 = line2;
		side1 = side2;
	}

	// grab the sector reference from the appropriate side
	// (Note that side1 = +1 for right, -1 for left, 0 for "on").
	if(line1 >= 0)
	{
		int sd_num = (side1 == Side::left) ? doc.linedefs[line1]->left : doc.linedefs[line1]->right;

		if(sd_num >= 0)
			return Objid(ObjType::sectors, doc.sidedefs[sd_num]->sector);
	}

	// none found
	return Objid();
}

//
// Gets an approximate distance from a point to a linedef
//
static double getApproximateDistanceToLinedef(const Document &doc, const LineDef &line, const v2double_t &pos)
{
	v2double_t pos1 = doc.getStart(line).xy();
	v2double_t pos2 = doc.getEnd(line).xy();
	v2double_t dpos = pos2 - pos1;

	if(fabs(dpos.x) > fabs(dpos.y))
	{
		// The linedef is rather horizontal

		// case 1: x is to the left of the linedef
		//         hence return distance to the left-most vertex
		if(pos.x < (dpos.x > 0 ? pos1.x : pos2.x))
			return hypot(pos.x - (dpos.x > 0 ? pos1.x : pos2.x), pos.y - (dpos.x > 0 ? pos1.y : pos2.y));

		// case 2: x is to the right of the linedef
		//         hence return distance to the right-most vertex
		if(pos.x > (dpos.x > 0 ? pos2.x : pos1.x))
			return hypot(pos.x - (dpos.x > 0 ? pos2.x : pos1.x), pos.y - (dpos.x > 0 ? pos2.y : pos1.y));

		// case 3: x is in-between (and not equal to) both vertices
		//         hence use slope formula to get intersection point
		double y3 = pos1.y + (pos.x - pos1.x) * dpos.y / dpos.x;

		return fabs(y3 - pos.y);
	}
	else
	{
		// The linedef is rather vertical

		if(pos.y < (dpos.y > 0 ? pos1.y : pos2.y))
			return hypot(pos.x - (dpos.y > 0 ? pos1.x : pos2.x), pos.y - (dpos.y > 0 ? pos1.y : pos2.y));

		if(pos.y > (dpos.y > 0 ? pos2.y : pos1.y))
			return hypot(pos.x - (dpos.y > 0 ? pos2.x : pos1.x), pos.y - (dpos.y > 0 ? pos2.y : pos1.y));

		double x3 = pos1.x + (pos.y - pos1.y) * dpos.x / dpos.y;

		return fabs(x3 - pos.x);
	}
}

//
// determine which linedef would be split if a new vertex were
// added at the given coordinates.
//
static Objid getNearestSplitLine(const Document &doc, MapFormat format, const grid::State &grid,
								 const v2double_t &pos, int ignore_vert)
{
	// slack in map units
	double mapslack = 1.5 + ceil(8.0f / grid.getScale());

	v2double_t lpos = pos - v2double_t(mapslack);
	v2double_t hpos = pos + v2double_t(mapslack);

	int    best = -1;
	double best_dist = 9e9;

	double too_small = (format == MapFormat::udmf) ? 0.2 : 4.0;

	for(int n = 0; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		if(L->start == ignore_vert || L->end == ignore_vert)
			continue;

		v2double_t pos1 = doc.getStart(*L).xy();
		v2double_t pos2 = doc.getEnd(*L).xy();

		if(std::max(pos1.x, pos2.x) < lpos.x || std::min(pos1.x, pos2.x) > hpos.x ||
		   std::max(pos1.y, pos2.y) < lpos.y || std::min(pos1.y, pos2.y) > hpos.y)
			continue;

		// skip linedef if given point matches a vertex
		if(pos == pos1 || pos == pos2)
			continue;

		// skip linedef if too small to split
		if(fabs(pos2.x - pos1.x) < too_small && fabs(pos2.y - pos1.y) < too_small)
			continue;

		double dist = getApproximateDistanceToLinedef(doc, *L, pos);

		if(dist > mapslack)
			continue;

		if(dist <= best_dist)
		{
			best = n;
			best_dist = dist;
		}
	}

	if(best >= 0)
		return Objid(ObjType::linedefs, best);

	// none found
	return Objid();
}

//
// Find crossing lines
//
void Hover::findCrossingLines(crossing_state_c &cross, const v2double_t &pos1, int possible_v1, const v2double_t &pos2, int possible_v2) const
{
	// this could happen when two vertices are overlapping
	if((pos1 - pos2).chebyshev() < ALONG_EPSILON)
		return;

	// distances along WHOLE original line for this segment
	double along1 = AlongDist(pos1, cross.start, cross.end);
	double along2 = AlongDist(pos2, cross.start, cross.end);

	// bounding box of segment
	const v2double_t bbox1 = {
		std::min(pos1.x, pos2.x) - 0.25,
		std::min(pos1.y, pos2.y) - 0.25
	};

	const v2double_t bbox2 = {
		std::max(pos1.x, pos2.x) + 0.25,
		std::max(pos1.y, pos2.y) + 0.25
	};


	for (int ld = 0 ; ld < doc.numLinedefs() ; ld++)
	{
		const auto L = doc.linedefs[ld];

		v2double_t lpos1 = doc.getStart(*L).xy();
		v2double_t lpos2 = doc.getEnd(*L).xy();

		// bbox test -- eliminate most lines from consideration
		if (std::max(lpos1.x,lpos2.x) < bbox1.x || std::min(lpos1.x,lpos2.x) > bbox2.x ||
			std::max(lpos1.y,lpos2.y) < bbox1.y || std::min(lpos1.y,lpos2.y) > bbox2.y)
		{
			continue;
		}

		if (doc.isZeroLength(*L))
			continue;

		if (cross.HasLine(ld))
			continue;

		// skip linedef if an end-point is one of the vertices already
		// in the crossing state (including the very start or very end).
		if (cross.HasVertex(L->start) || cross.HasVertex(L->end))
			continue;

		// only need to handle cases where this linedef distinctly crosses
		// the new line (i.e. start and end are clearly on opposite sides).

		double a = PerpDist(lpos1, pos1, pos2);
		double b = PerpDist(lpos2, pos1, pos2);

		if (! ((a < -CROSSING_EPSILON && b >  CROSSING_EPSILON) ||
			   (a >  CROSSING_EPSILON && b < -CROSSING_EPSILON)))
		{
			continue;
		}

		// compute intersection point
		double l_along = a / (a - b);

		v2double_t newpos = lpos1 + (lpos2 - lpos1) * l_along;

		double along = AlongDist(newpos, cross.start, cross.end);

		// ensure new vertex lies within this segment (and not too close to ends)
		if (along > along1 + ALONG_EPSILON && along < along2 - ALONG_EPSILON)
		{
			cross.add_line(ld, newpos, along);
		}
	}
}

//------------------------------------------------------------------------

void crossing_state_c::clear()
{
	points.clear();
}


void crossing_state_c::add_vert(int v, double dist)
{
	cross_point_t pt;

	pt.vert = v;
	pt.ld   = -1;
	pt.pos  = inst.level.vertices[v]->xy();
	pt.dist = dist;

	points.push_back(pt);
}

void crossing_state_c::add_line(int ld, const v2double_t &newpos, double dist)
{
	cross_point_t pt;

	pt.vert = -1;
	pt.ld   = ld;
	pt.pos  = newpos;
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


void crossing_state_c::SplitAllLines(EditOperation &op)
{
	for (unsigned int i = 0 ; i < points.size() ; i++)
	{
		if (points[i].ld >= 0)
		{
			points[i].vert = op.addNew(ObjType::vertices);

			auto V = inst.level.vertices[points[i].vert];

			V->SetRawXY(inst.loaded.levelFormat, points[i].pos);

			inst.level.linemod.splitLinedefAtVertex(op, points[i].ld, points[i].vert);
		}
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
