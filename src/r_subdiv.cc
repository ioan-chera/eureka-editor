//------------------------------------------------------------------------
//  SECTOR SUBDIVISION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2020 Andrew Apted
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
#include "m_game.h"
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
	// determine if low or high are a single vertex
	bool l_single = fabs(lx2 - lx1) < 0.2;
	bool h_single = fabs(hx2 - hx1) < 0.2;

	// skip a degenerate polygon
	if (l_single && h_single)
		return;

	sector_polygon_t poly;

	poly.count = (l_single || h_single) ? 3 : 4;

	// add vertices in clockwise order
	int pos = 0;

	poly.mx[pos] = lx1;
	poly.my[pos] = low_y;
	pos++;

	poly.mx[pos] = hx1;
	poly.my[pos] = high_y;
	pos++;

	if (! h_single)
	{
		poly.mx[pos] = hx2;
		poly.my[pos] = high_y;
		pos++;
	}

	if (! l_single)
	{
		poly.mx[pos] = lx2;
		poly.my[pos] = low_y;
		pos++;
	}

	polygons.push_back(poly);
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
	double bound_x1, bound_x2;
	double bound_y1, bound_y2;

	sector_subdivision_c sub;

	sector_3dfloors_c floors;

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
		floors.Clear();

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
		bound_x1 = MIN(bound_x1, V->x());
		bound_y1 = MIN(bound_y1, V->y());

		bound_x2 = MAX(bound_x2, V->x());
		bound_y2 = MAX(bound_y2, V->y());
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

			CheckBoom242(L);
			CheckExtraFloor(L, n);
			CheckLineSlope(L);

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

		// TODO slope things
		// TODO slope copy things

		for (int n = 0 ; n < NumLineDefs ; n++)
		{
			CheckPlaneCopy(LineDefs[n]);
		}
	}

	void CheckBoom242(const LineDef *L)
	{
		if (Features.gen_types && (L->type == 242 || L->type == 280))
		{ /* ok */ }
		else if (Level_format != MAPF_Doom && L->type == 209)
		{ /* ok */ }
		else
			return;

		if (L->tag <= 0 || L->right < 0)
			return;

		int dummy_sec = L->Right()->sector;

		for (int n = 0 ; n < NumSectors ; n++)
		{
			if (Sectors[n]->tag == L->tag)
				infos[n].floors.heightsec = dummy_sec;
		}
	}

	void CheckExtraFloor(const LineDef *L, int ld_num)
	{
		if (L->tag <= 0 || L->right < 0)
			return;

		int flags = -1;

		// EDGE style
		if (Level_format == MAPF_Doom && (Features.extra_floors & 1))
		{
			switch (L->type)
			{
			case 400: flags = 0; break;
			case 401: flags = EXFL_UPPER; break;
			case 402: flags = EXFL_LOWER; break;
			case 403: flags = EXFL_BOTTOM; break; // liquid
			case 413: flags = EXFL_BOTTOM; break; // thin and solid

			case 404: case 405: case 406: case 407: case 408:  // liquid
			case 414: case 415: case 416: case 417:  // thin and solid
				flags = EXFL_BOTTOM | EXFL_TRANSLUC;
				break;

			default: break;
			}
		}

		// Legacy style
		if (Level_format == MAPF_Doom && (Features.extra_floors & 2))
		{
			switch (L->type)
			{
			case 281: flags = 0; break;
			case 289: flags = 0; break;
			case 300: flags = EXFL_TRANSLUC; break;
			case 301: flags = EXFL_TOP | EXFL_TRANSLUC; break;
			case 304: flags = EXFL_TOP; break;
			case 306: flags = EXFL_TOP | EXFL_TRANSLUC; break; // invisible floor

			default: break;
			}
		}

		// ZDoom style
		if (Level_format != MAPF_Doom && (Features.extra_floors & 4))
		{
			if (L->type != 160)
				return;

			flags = 0;

			if ((L->arg2 & 3) == 0)
				flags |= EXFL_VAVOOM;

			if (L->arg3 & 8)  flags |= EXFL_TOP;
			if (L->arg3 & 16) flags |= EXFL_UPPER;
			if (L->arg3 & 32) flags |= EXFL_LOWER;
		}

		if (flags < 0)
			return;

		extrafloor_c EF;

		EF.ld = ld_num;
		EF.sd = L->right;
		EF.flags = flags;

		// find all matching sectors
		for (int n = 0 ; n < NumSectors ; n++)
		{
			if (Sectors[n]->tag == L->tag)
				infos[n].floors.floors.push_back(EF);
		}
	}

	void CheckLineSlope(const LineDef *L)
	{
		// EDGE style
		if (Level_format == MAPF_Doom && (Features.slopes & 1))
		{
			switch (L->type)
			{
			case 567: PlaneAlign(L, 2, 0); break;
			case 568: PlaneAlign(L, 0, 2); break;
			case 569: PlaneAlign(L, 2, 2); break;
			default: break;
			}
		}

		// Eternity style
		if (Level_format == MAPF_Doom && (Features.slopes & 2))
		{
			switch (L->type)
			{
			case 386: PlaneAlign(L, 1, 0); break;
			case 387: PlaneAlign(L, 0, 1); break;
			case 388: PlaneAlign(L, 1, 1); break;
			case 389: PlaneAlign(L, 2, 0); break;
			case 390: PlaneAlign(L, 0, 2); break;
			case 391: PlaneAlign(L, 2, 2); break;
			case 392: PlaneAlign(L, 2, 1); break;
			case 393: PlaneAlign(L, 1, 2); break;
			default: break;
			}
		}

		// Odamex and ZDoom style
		if (Level_format == MAPF_Doom && (Features.slopes & 4))
		{
			switch (L->type)
			{
			case 340: PlaneAlign(L, 1, 0); break;
			case 341: PlaneAlign(L, 0, 1); break;
			case 342: PlaneAlign(L, 1, 1); break;
			case 343: PlaneAlign(L, 2, 0); break;
			case 344: PlaneAlign(L, 0, 2); break;
			case 345: PlaneAlign(L, 2, 2); break;
			case 346: PlaneAlign(L, 2, 1); break;
			case 347: PlaneAlign(L, 1, 2); break;
			default: break;
			}
		}

		// ZDoom (in hexen format)
		if (Level_format == MAPF_Hexen && (Features.slopes & 8))
		{
			if (L->type == 181)
				PlaneAlign(L, L->tag, L->arg2);
		}
	}

	void CheckPlaneCopy(const LineDef *L)
	{
		// Eternity style
		if (Level_format == MAPF_Doom && (Features.slopes & 2))
		{
			switch (L->type)
			{
			case 394: PlaneCopy(L, L->tag, 0, 0, 0, 0); break;
			case 395: PlaneCopy(L, 0, L->tag, 0, 0, 0); break;
			case 396: PlaneCopy(L, L->tag, L->tag, 0, 0, 0); break;
			default: break;
			}
		}

		// ZDoom (in hexen format)
		if (Level_format == MAPF_Hexen && (Features.slopes & 8))
		{
			if (L->type == 118)
				PlaneCopy(L, L->tag, L->arg2, L->arg3, L->arg4, L->arg5);
		}
	}

	void PlaneAlign(const LineDef *L, int floor_mode, int ceil_mode)
	{
		if (L->left < 0 || L->right < 0)
			return;

		// support undocumented special case from ZDoom
		if (ceil_mode == 0 && (floor_mode & 0x0C) != 0)
			ceil_mode = (floor_mode >> 2);

		switch (floor_mode & 3)
		{
		case 1: PlaneAlignPart(L, SIDE_RIGHT, 0 /* floor */); break;
		case 2: PlaneAlignPart(L, SIDE_LEFT,  0); break;
		}

		switch (ceil_mode & 3)
		{
		case 1: PlaneAlignPart(L, SIDE_RIGHT, 1 /* ceil */); break;
		case 2: PlaneAlignPart(L, SIDE_LEFT,  1); break;
		}
	}

	void PlaneAlignPart(const LineDef *L, int side, int plane)
	{
		int sec_num = L->WhatSector(side);
		const Sector *front = Sectors[L->WhatSector(side)];
		const Sector *back  = Sectors[L->WhatSector(-side)];

		// find a vertex belonging to sector and is far from the line
		const Vertex *v = NULL;
		double best_dist = 0.1;

		double lx1 = L->Start()->x();
		double ly1 = L->Start()->y();
		double lx2 = L->End()->x();
		double ly2 = L->End()->y();

		if (side < 0)
		{
			std::swap(lx1, lx2);
			std::swap(ly1, ly2);
		}

		for (int n = 0 ; n < NumLineDefs ; n++)
		{
			const LineDef *L2 = LineDefs[n];
			if (L2->TouchesSector(sec_num))
			{
				const Vertex *v2 = L2->Start();
				double dist = PerpDist(v2->x(), v2->y(), lx1,ly1, lx2,ly2);

				if (dist > best_dist)
				{
					v = v2;
					best_dist = dist;
				}
			}
		}

		if (v == NULL)
			return;

		// compute point at 90 degrees to the linedef
		double ldx = (ly2 - ly1);
		double ldy = (lx1 - lx2);
		double ldh = hypot(ldx, ldy);

		double vx = lx1 + ldx * best_dist / ldh;
		double vy = ly1 + ldy * best_dist / ldh;

		if (plane > 0)
		{   // ceiling
			SlopeFromLine(infos[sec_num].floors.c_plane,
				lx1, ly1, back->ceilh, vx, vy, front->ceilh);
		}
		else
		{   // floor
			SlopeFromLine(infos[sec_num].floors.f_plane,
				lx1, ly1, back->floorh, vx, vy, front->floorh);
		}
	}

	void PlaneCopy(const LineDef *L, int f1_tag, int c1_tag, int f2_tag, int c2_tag, int share)
	{
		// TODO
	}

	void SlopeFromLine(slope_plane_c& pl, double x1, double y1, double z1,
			double x2, double y2, double z2)
	{
		double dx = x2 - x1;
		double dy = y2 - y1;
		double dz = z2 - z1;

		if (fabs(dz) < 0.5)
			return;

		// make (dx dy) be a unit vector
		double dlen = hypot(dx, dy);
		dx /= dlen;
		dy /= dlen;

		// we want SlopeZ() to compute z1 at (x1 y1) and z2 at (x2 y2).
		// assume xm = (dx * A) and ym = (dy * A) and zadd = B
		// that leads to two simultaneous equations:
		//    x1 * dx * A + y1 * dy * A + B = z1
		//    x2 * dx * A + y2 * dy * A + B = z2
		// which we need to solve for A and B....

		double E = (x1 * dx + y1 * dy);
		double F = (x2 * dx + y2 * dy);

		double A = (z2 -z1) / (F - E);
		double B = z1 - A * E;

		pl.xm = dx * A;
		pl.ym = dy * A;
		pl.zadd = B;
		pl.sloped = true;
	}
};

static sector_info_cache_c sector_info_cache;


static void R_SubdivideSector(int num, sector_extra_info_t& exinfo)
{
	if (exinfo.first_line < 0)
		return;

/* DEBUG
fprintf(stderr, "R_SubdivideSector %d\n", num);
*/

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

		edge.x1 = L->Start()->x();
		edge.y1 = L->Start()->y();
		edge.x2 = L->End()->x();
		edge.y2 = L->End()->y();

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

/* DEBUG
fprintf(stderr, "Line %d  mapped coords (%d %d) .. (%d %d)  flipped:%d  sec:%d/%d\n",
	n, edge.x1, edge.y1, edge.x2, edge.y2, edge.flipped,
	L->WhatSector(SIDE_RIGHT), L->WhatSector(SIDE_LEFT));
*/

		// add the edge
		edgelist.push_back(edge);
	}

	if (edgelist.empty())
		return;

	// sort edges into vertical order (using 'y1' field)

	std::sort(edgelist.begin(), edgelist.end(), sector_edge_t::CMP_Y());


	/*** Part 2 : traverse edge list and create trapezoids ***/

	std::vector<sector_edge_t *> active_edges;

	if (edgelist.empty())
		return;

	unsigned int pos = 0;

	// this Y is minimal (guaranteed by sorting the edgelist)
	int low_y = edgelist[pos].y1;

	for (;;)
	{
		// remove old edges from active list
		for (unsigned int i = 0 ; i < active_edges.size() ; i++)
		{
			sector_edge_t *A = active_edges[i];

			if (A != NULL && A->y2 <= low_y)
				active_edges[i] = NULL;
		}

		// add new edges to active list
		for ( ; pos < edgelist.size() && edgelist[pos].y1 == low_y ; pos++)
		{
			active_edges.push_back(&edgelist[pos]);
		}

		// find next highest Y
		int high_y = (1 << 30);
		int active_num = 0;

		if (pos < edgelist.size())
			high_y = edgelist[pos].y1;

		for (unsigned int k = 0 ; k < active_edges.size() ; k++)
		{
			sector_edge_t *A = active_edges[k];

			if (A != NULL)
			{
				active_num++;

				if (A->y1 > low_y) high_y = MIN(high_y, A->y1);
				if (A->y2 > low_y) high_y = MIN(high_y, A->y2);
			}
		}

/* DEBUG
fprintf(stderr, "  active_num:%d  low_y:%d  high_y:%d\n", active_num, low_y, high_y);
*/
		if (active_num == 0)
		{
			while (pos < edgelist.size() && edgelist[pos].y1 <= low_y)
				pos++;

			// terminating condition : no more rows
			if (pos >= edgelist.size())
				break;

			low_y = edgelist[pos].y1;
			continue;
		}

		// compute a comparison X coordinate for each active edge
		float mid_y = low_y + (high_y - low_y) * 0.5;

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
	E1->cmp_x, E1->side, E2->cmp_x, E2->side);
*/

			if (! (E1->side == SIDE_RIGHT && E2->side == SIDE_LEFT))
				continue;

			// treat lines without a right side as dead
			// [ NOTE that we don't ignore them ]
			if (E1->line->right < 0) continue;
			if (E2->line->right < 0) continue;

			float lx1 = E1->CalcX(low_y);
			float hx1 = E1->CalcX(high_y);

			float lx2 = E2->CalcX(low_y);
			float hx2 = E2->CalcX(high_y);

			exinfo.sub.AddPolygon(lx1, lx2, low_y, hx1, hx2, high_y);
		}

		// ok, repeat process for next row
		low_y = high_y;
	}
}


void Subdiv_InvalidateAll()
{
	// invalidate everything
	sector_info_cache.total = -1;
}


bool Subdiv_SectorOnScreen(int num, double map_lx, double map_ly, double map_hx, double map_hy)
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


//------------------------------------------------------------------------
//  3D Floor and Slope stuff
//------------------------------------------------------------------------

sector_3dfloors_c::sector_3dfloors_c() :
	heightsec(-1)
{ }

sector_3dfloors_c::~sector_3dfloors_c()
{ }


void sector_3dfloors_c::Clear()
{
	heightsec = -1;
	floors.clear();
	f_plane.sloped = false;
	c_plane.sloped = false;
}


extrafloor_c::extrafloor_c() : ld(-1), sd(-1), flags(0)
{ }

extrafloor_c::~extrafloor_c()
{ }


slope_plane_c::slope_plane_c() : sloped(false), xm(0), ym(0), zadd(0)
{ }

slope_plane_c::~slope_plane_c()
{ }


sector_3dfloors_c *Subdiv_3DFloorsForSector(int num)
{
	sector_info_cache.Update();

	sector_extra_info_t& exinfo = sector_info_cache.infos[num];

	return &exinfo.floors;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
