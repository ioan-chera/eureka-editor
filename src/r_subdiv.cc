//------------------------------------------------------------------------
//  SECTOR SUBDIVISION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2019 Andrew Apted
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

#include "main.h"

#include <algorithm>

#include "e_basis.h"
#include "r_subdiv.h"


/* This file contains code for subdividing map sectors into a set
   of polygons, and also the logic for caching these subdivisions
   and rebuilding them whenever necessary.
*/


sector_subdivision_c::sector_subdivision_c() :
	polygons()
{ }

sector_subdivision_c::~sector_subdivision_c()
{ }

void sector_subdivision_c::Clear()
{
	polygons.clear();
}


void sector_subdivision_c::AddPolygon(float lx1, float lx2, float low_y,
	float hx1, float hx2, float high_y)
{
	// FIXME AddPolygon
}


// this represents a segment of a linedef bounding a sector.
struct sector_edge_t
{
	const LineDef * line;

	// coordinates of line, possibly flipped.
	// we always have: y1 <= y2.
	int x1, y1;
	int x2, y2;

	// has the line been flipped (coordinates were swapped) ?
	short flipped;

	// what side this edge faces (SIDE_LEFT or SIDE_RIGHT)
	short side;

	// comparison for CMP_X
	float cmp_x;

	float CalcX(float y) const
	{
		return x1 + (x2 - x1) * (float)(y - y1) / (float)(y2 - y1);
	}

	struct CMP_Y
	{
		inline bool operator() (const sector_edge_t &A, const sector_edge_t& B) const
		{
			// NULL not allowed here

			return A.y1 < B.y1;
		}
	};

	struct CMP_X
	{
		inline bool operator() (const sector_edge_t *A, const sector_edge_t *B) const
		{
			// NULL is always > than a valid pointer

			if (A == NULL)
				return false;

			if (B == NULL)
				return true;

			return A->cmp_x < B->cmp_x;
		}
	};
};


struct sector_extra_info_t
{
	// these are < 0 when the sector has no lines
	int first_line;
	int last_line;

	// these are random junk when sector has no lines
	int bound_x1, bound_x2;
	int bound_y1, bound_y2;

	sector_subdivision_c sub;

	// true when polygons have been built for this sector.
	bool built;

	void Clear()
	{
		first_line = last_line = -1;

		bound_x1 = 32767;
		bound_y1 = 32767;
		bound_x2 = -32767;
		bound_y2 = -32767;

		sub.Clear();

		built = false;
	}

	void AddLine(int n)
	{
		if (first_line < 0 || first_line > n)
			first_line = n;

		if (last_line < n)
			last_line = n;
	}

	void AddVertex(const Vertex *V)
	{
		bound_x1 = MIN(bound_x1, V->x);
		bound_y1 = MIN(bound_y1, V->y);

		bound_x2 = MAX(bound_x2, V->x);
		bound_y2 = MAX(bound_y2, V->y);
	}
};


class sector_info_cache_c
{
public:
	int total;

	std::vector<sector_extra_info_t> infos;

public:
	sector_info_cache_c() : total(-1), infos()
	{ }

	~sector_info_cache_c()
	{ }

public:
	void Update()
	{
		if (total != NumSectors)
		{
			total = NumSectors;

			infos.resize((size_t) total);

			Rebuild();
		}
	}

	void Rebuild()
	{
		int sec;

		for (sec = 0 ; sec < total ; sec++)
			infos[sec].Clear();

		for (int n = 0 ; n < NumLineDefs ; n++)
		{
			const LineDef *L = LineDefs[n];

			for (int side = 0 ; side < 2 ; side++)
			{
				int sd_num = side ? L->left : L->right;

				if (sd_num < 0)
					continue;

				sec = SideDefs[sd_num]->sector;

				sector_extra_info_t& info = infos[sec];

				info.AddLine(n);

				info.AddVertex(L->Start());
				info.AddVertex(L->End());
			}
		}
	}
};

static sector_info_cache_c sector_info_cache;


static void R_SubdivideSector(int num, sector_extra_info_t& exinfo)
{
	if (exinfo.first_line < 0)
		return;

///  fprintf(stderr, "R_SubdivideSector %d\n", num);


	/*** Part 1 : visit linedefs and create edges ***/

	std::vector<sector_edge_t> edgelist;

	for (int n = exinfo.first_line ; n <= exinfo.last_line ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (! L->TouchesSector(num))
			continue;

		// ignore 2S lines with same sector on both sides
		if (L->WhatSector(SIDE_LEFT) == L->WhatSector(SIDE_RIGHT))
			continue;

		sector_edge_t edge;

		edge.x1 = L->Start()->x;
		edge.y1 = L->Start()->y;
		edge.x2 = L->End()->x;
		edge.y2 = L->End()->y;

		// skip purely horizontal lines
		if (edge.y1 == edge.y2)
			continue;

		edge.line = L;
		edge.flipped = 0;

		if (edge.y1 > edge.y2)
		{
			std::swap(edge.x1, edge.x2);
			std::swap(edge.y1, edge.y2);

			edge.flipped = 1;
		}

		// compute side
		bool is_right = (L->WhatSector(SIDE_RIGHT) == num);

		if (edge.flipped)
			is_right = !is_right;

		edge.side = is_right ? SIDE_RIGHT : SIDE_LEFT;

// DEBUG
fprintf(stderr, "Line %d  mapped coords (%d %d) .. (%d %d)  flipped:%d  sec:%d/%d\n",
n, edge.x1, edge.y1, edge.x2, edge.y2, edge.flipped,
L->WhatSector(SIDE_RIGHT), L->WhatSector(SIDE_LEFT));
//

		// add the edge
		edgelist.push_back(edge);
	}

	if (edgelist.empty())
		return;

	// sort edges into vertical order (using 'y1' field)

	std::sort(edgelist.begin(), edgelist.end(), sector_edge_t::CMP_Y());


	/*** Part 2 : traverse edge list and create trapezoids ***/

	std::vector<sector_edge_t *> active_edges;

	unsigned int pos = 0;

	while (pos < edgelist.size())
	{
		// this Y is minimal (guaranteed by sorting the edgelist)
		int min_y = edgelist[pos].y1;

		// remove old edges from active list
		for (unsigned int i = 0 ; i < active_edges.size() ; i++)
		{
			if (active_edges[i]->y2 <= min_y)
				active_edges[i] = NULL;
		}

		// add new edges to active list
		unsigned int next = pos;

		for ( ; next < edgelist.size() && edgelist[next].y1 == min_y ; next++)
		{
			active_edges.push_back(&edgelist[next]);
		}

		// find next highest Y
		int next_y = edgelist[pos].y2;

		for (unsigned int k = 0 ; k < active_edges.size() ; k++)
		{
			sector_edge_t *A = active_edges[k];

			if (A != NULL)
			{
				if (A->y1 > min_y) next_y = MIN(next_y, A->y1);
				if (A->y2 > min_y) next_y = MIN(next_y, A->y2);
			}
		}

/* DEBUG
	fprintf(stderr, "  min_y:%d  next_y:%d\n", min_y, next_y);
*/

		// compute a comparison X coordinate for each active edge
		float mid_y = min_y + (next_y - min_y) * 0.5;

		for (unsigned int k = 0 ; k < active_edges.size() ; k++)
		{
			sector_edge_t *A = active_edges[k];

			if (A != NULL)
				A->cmp_x = A->CalcX(mid_y);
		}

		// sort edges horizontally
		std::sort(active_edges.begin(), active_edges.end(), sector_edge_t::CMP_X());

		// remove NULLs
		while (active_edges.size() > 0 && active_edges.back() == NULL)
			active_edges.pop_back();

		// visit pairs of edges
		for (unsigned int i = 1 ; i < active_edges.size() ; i++)
		{
			const sector_edge_t * E1 = active_edges[i - 1];
			const sector_edge_t * E2 = active_edges[i];
#if 1
			if (E1 == NULL || E2 == NULL)
				BugError("RenderSector: did not delete NULLs properly!\n");
#endif

/* DEBUG
	fprintf(stderr, "E1 @ x=%1.2f side=%d  |  E2 @ x=%1.2f side=%d\n",
		E1->x, E1->side, E2->x, E2->side);
*/

			if (! (E1->side == SIDE_RIGHT && E2->side == SIDE_LEFT))
				continue;

			// treat lines without a right side as dead
			if (E1->line->right < 0) continue;
			if (E2->line->right < 0) continue;

			float lx1 = E1->CalcX(min_y);
			float hx1 = E1->CalcX(next_y);

			float lx2 = E2->CalcX(min_y);
			float hx2 = E2->CalcX(next_y);

			exinfo.sub.AddPolygon(lx1, lx2, min_y, hx1, hx2, next_y);
		}

		// ok, repeat process for next row
		pos++;

		while (pos < edgelist.size() && edgelist[pos].y1 < next_y)
			pos++;
	}
}


void Subdiv_InvalidateAll()
{
	// FIXME Subdiv_InvalidateAll

	// invalidate everything
	sector_info_cache.total = -1;
}


bool Subdiv_SectorOnScreen(int num, int map_lx, int map_ly, int map_hx, int map_hy)
{
	sector_info_cache.Update();

	sector_extra_info_t& exinfo = sector_info_cache.infos[num];

	if (exinfo.bound_x1 > map_hx || exinfo.bound_x2 < map_lx ||
		exinfo.bound_y1 > map_hy || exinfo.bound_y2 < map_ly)
	{
		// sector is off-screen
		return false;
	}

	return true;
}


sector_subdivision_c *Subdiv_PolygonsForSector(int num)
{
	sector_info_cache.Update();

	sector_extra_info_t& exinfo = sector_info_cache.infos[num];

	if (! exinfo.built)
	{
		R_SubdivideSector(num, exinfo);
		exinfo.built = true;
	}

	return &exinfo.sub;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
