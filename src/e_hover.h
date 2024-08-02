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
#include "m_vector.h"
#include "objid.h"
#include "tl/optional.hpp"
#include <memory>
#include <vector>

namespace grid
{
class State;
}

class bitvec_c;
class crossing_state_c;
class EditOperation;
class fastopp_node_c;
class LineDef;
class Objid;
enum class MapFormat;
enum class Side;
struct ConfigData;
struct Editor_State_t;
struct v2double_t;

namespace hover
{
Objid findSplitLine(const Document &doc, MapFormat format, const Editor_State_t &edit,
					const grid::State &grid, v2double_t &out_pos, const v2double_t &ptr,
					int ignore_vert);
Objid findSplitLineForDangler(const Document &doc, MapFormat format,
							  const grid::State &grid, int v_num);
int getClosestLine_CastingHoriz(const Document &doc, v2double_t pos, Side *side);
Objid getNearbyObject(ObjType type, const Document &doc, const ConfigData &config,
					  const grid::State &grid, const v2double_t &pos);
Objid getNearestSector(const Document &doc, const v2double_t &pos);
bool isPointOutsideOfMap(const Document &doc, const v2double_t &v);
}

struct opp_test_state_t;
class fastopp_node_c
{
public:
	int lo, hi;   // coordinate range
	int mid;

	std::unique_ptr<fastopp_node_c> lo_child;
	std::unique_ptr<fastopp_node_c> hi_child;

	std::vector<int> lines;

	const Document &doc;

public:
	fastopp_node_c(int _low, int _high, const Document &doc) :
		lo(_low), hi(_high), mid((_low + _high) / 2), doc(doc)
	{
		Subdivide();
	}

private:
	void Subdivide();

public:
	/* horizontal tree */
	void AddLine_X(int ld, int x1, int x2);
	void AddLine_X(int ld);

	/* vertical tree */
	void AddLine_Y(int ld, int y1, int y2);
	void AddLine_Y(int ld);

	void Process(opp_test_state_t& test, double coord) const;
};

struct FastOppositeTree
{
	explicit FastOppositeTree(Instance &inst);
	
	tl::optional<fastopp_node_c> m_fastopp_X_tree;
	tl::optional<fastopp_node_c> m_fastopp_Y_tree;
};

//
// The hover module
//
class Hover : public DocumentModule
{
public:
	Hover(Document &doc) : DocumentModule(doc)
	{
	}

	int getOppositeLinedef(int ld, Side ld_side, Side *result_side, const bitvec_c *ignore_lines, FastOppositeTree *tree) const;
	int getOppositeSector(int ld, Side ld_side, FastOppositeTree *tree) const;

	void findCrossingPoints(crossing_state_c &cross,
		v2double_t p1, int possible_v1,
		v2double_t p2, int possible_v2) const;

private:
	void findCrossingLines(crossing_state_c &cross, const v2double_t &pos1, int possible_v1, const v2double_t &pos2, int possible_v2) const;
};

// result: -1 for back, +1 for front, 0 for _exactly_on_ the line
Side PointOnLineSide(double x, double y, double lx1, double ly1, double lx2, double ly2);


struct cross_point_t
{
	int vert;	// >= 0 when we hit a vertex
	int ld;     // >= 0 when we hit a linedef instead

	v2double_t pos;	// coordinates of line split point
	double dist;
};


class crossing_state_c
{
public:
	std::vector< cross_point_t > points;

	// the start/end coordinates of the whole tested line
	v2double_t start = {};
	v2double_t end = {};

	Instance &inst;

public:
	explicit crossing_state_c(Instance &inst) : inst(inst)
	{
	}

	void clear();

	void add_vert(int v, double dist);
	void add_line(int ld, const v2double_t &newpos, double dist);

	bool HasVertex(int v) const;
	bool HasLine(int ld)  const;

	void Sort();

	void SplitAllLines(EditOperation &op);

private:
	struct point_CMP
	{
		inline bool operator() (const cross_point_t &A, const cross_point_t& B) const
		{
			return A.dist < B.dist;
		}
	};
};

#endif  /* __EUREKA_X_HOVER_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
