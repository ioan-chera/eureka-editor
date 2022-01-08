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

#include "Errors.h"
#include "Instance.h"

#include "main.h"

#include <algorithm>

#include "e_basis.h"
#include "e_hover.h"
#include "m_game.h"
#include "r_subdiv.h"


/* This file contains code for subdividing map sectors into a set
   of polygons, and also the logic for caching these subdivisions
   and rebuilding them whenever necessary.
*/


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
	Side side;

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

//
// Update
//
void sector_info_cache_c::Update()
{
   if (total != inst.level.numSectors())
   {
	   total = inst.level.numSectors();

	   infos.resize((size_t) total);

	   Rebuild();
   }
}

void sector_info_cache_c::Rebuild()
{
	int sec;

	for (sec = 0 ; sec < total ; sec++)
	{
		const Sector *S = inst.level.sectors[sec];

		infos[sec].Clear();
		infos[sec].floors.f_plane.Init(static_cast<float>(S->floorh));
		infos[sec].floors.c_plane.Init(static_cast<float>(S->ceilh));
	}

	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		const LineDef *L = inst.level.linedefs[n];

		CheckBoom242(L);
		CheckExtraFloor(L, n);
		CheckLineSlope(L);

		for (int side = 0 ; side < 2 ; side++)
		{
			int sd_num = side ? L->left : L->right;
			if (sd_num < 0)
				continue;

			sec = inst.level.sidedefs[sd_num]->sector;

			sector_extra_info_t& info = infos[sec];

			info.AddLine(n);

			info.AddVertex(L->Start(inst.level));
			info.AddVertex(L->End(inst.level));
		}
	}

	for (const Thing *thing : inst.level.things)
	{
		CheckSlopeThing(thing);
	}
	for (const Thing *thing : inst.level.things)
	{
		CheckSlopeCopyThing(thing);
	}

	for (const LineDef *linedef : inst.level.linedefs)
	{
		CheckPlaneCopy(linedef);
	}
}

void sector_info_cache_c::CheckBoom242(const LineDef *L)
{
	if (inst.conf.features.gen_types && (L->type == 242 || L->type == 280))
	{ /* ok */ }
	else if (inst.loaded.levelFormat != MapFormat::doom && L->type == 209)
	{ /* ok */ }
	else
		return;

	if (L->tag <= 0 || L->right < 0)
		return;

	int dummy_sec = L->Right(inst.level)->sector;

	for (int n = 0 ; n < inst.level.numSectors(); n++)
	{
		if (inst.level.sectors[n]->tag == L->tag)
			infos[n].floors.heightsec = dummy_sec;
	}
}

void sector_info_cache_c::CheckExtraFloor(const LineDef *L, int ld_num)
{
	if (L->tag <= 0 || L->right < 0)
		return;

	int flags = -1;
	int sec_tag = L->tag;

	// EDGE style
	if (inst.loaded.levelFormat == MapFormat::doom && (inst.conf.features.extra_floors & 1))
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
	if (inst.loaded.levelFormat == MapFormat::doom && (inst.conf.features.extra_floors & 2))
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
	if (inst.loaded.levelFormat != MapFormat::doom && (inst.conf.features.extra_floors & 4))
	{
		if (L->type != 160)
			return;

		flags = 0;

		if ((L->arg2 & 3) == 0)
			flags |= EXFL_VAVOOM;

		if (L->arg3 & 8)  flags |= EXFL_TOP;
		if (L->arg3 & 16) flags |= EXFL_UPPER;
		if (L->arg3 & 32) flags |= EXFL_LOWER;

		if ((L->arg2 & 8) == 0)
			sec_tag |= (L->arg5 << 8);
	}

	if (flags < 0)
		return;

	extrafloor_c EF;

	EF.ld = ld_num;
	EF.sd = L->right;
	EF.flags = flags;

	// find all matching sectors
	for (int n = 0 ; n < inst.level.numSectors(); n++)
	{
		if (inst.level.sectors[n]->tag == sec_tag)
			infos[n].floors.floors.push_back(EF);
	}
}

void sector_info_cache_c::CheckLineSlope(const LineDef *L)
{
	// EDGE style
	if (inst.loaded.levelFormat == MapFormat::doom && (inst.conf.features.slopes & 1))
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
	if (inst.loaded.levelFormat == MapFormat::doom && (inst.conf.features.slopes & 2))
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
	if (inst.loaded.levelFormat == MapFormat::doom && (inst.conf.features.slopes & 4))
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
	if (inst.loaded.levelFormat != MapFormat::doom && (inst.conf.features.slopes & 8))
	{
		if (L->type == 181)
			PlaneAlign(L, L->tag, L->arg2);
	}
}

void sector_info_cache_c::CheckPlaneCopy(const LineDef *L)
{
	// Eternity style
	if (inst.loaded.levelFormat == MapFormat::doom && (inst.conf.features.slopes & 2))
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
	if (inst.loaded.levelFormat != MapFormat::doom && (inst.conf.features.slopes & 8))
	{
		if (L->type == 118)
			PlaneCopy(L, L->tag, L->arg2, L->arg3, L->arg4, L->arg5);
	}
}

void sector_info_cache_c::CheckSlopeThing(const Thing *T)
{
	if (inst.loaded.levelFormat != MapFormat::doom && (inst.conf.features.slopes & 16))
	{
		switch (T->type)
		{
		// TODO 1500, 1501 Vavoom style
		// TODO 1504, 1505 Vertex height (triangle sectors)
		// TODO 9500, 9501 Line slope things

		case 9502: PlaneTiltByThing(T, 0); break;
		case 9503: PlaneTiltByThing(T, 1); break;
		default: break;
		}
	}
}

void sector_info_cache_c::CheckSlopeCopyThing(const Thing *T)
{
	if (inst.loaded.levelFormat != MapFormat::doom && (inst.conf.features.slopes & 16))
	{
		switch (T->type)
		{
		case 9510: PlaneCopyFromThing(T, 0); break;
		case 9511: PlaneCopyFromThing(T, 1); break;
		default: break;
		}
	}
}

void sector_info_cache_c::PlaneAlign(const LineDef *L, int floor_mode, int ceil_mode)
{
	if (L->left < 0 || L->right < 0)
		return;

	// support undocumented special case from ZDoom
	if (ceil_mode == 0 && (floor_mode & 0x0C) != 0)
		ceil_mode = (floor_mode >> 2);

	switch (floor_mode & 3)
	{
	case 1: PlaneAlignPart(L, Side::right, 0 /* floor */); break;
	case 2: PlaneAlignPart(L, Side::left,  0); break;
	default: break;
	}

	switch (ceil_mode & 3)
	{
	case 1: PlaneAlignPart(L, Side::right, 1 /* ceil */); break;
	case 2: PlaneAlignPart(L, Side::left,  1); break;
	default: break;
	}
}

void sector_info_cache_c::PlaneAlignPart(const LineDef *L, Side side, int plane)
{
	int sec_num = L->WhatSector(side, inst.level);
	const Sector *front = inst.level.sectors[L->WhatSector(side, inst.level)];
	const Sector *back  = inst.level.sectors[L->WhatSector(-side, inst.level)];

	// find a vertex belonging to sector and is far from the line
	const Vertex *v = NULL;
	double best_dist = 0.1;

	double lx1 = L->Start(inst.level)->x();
	double ly1 = L->Start(inst.level)->y();
	double lx2 = L->End(inst.level)->x();
	double ly2 = L->End(inst.level)->y();

	if (side == Side::left)
	{
		std::swap(lx1, lx2);
		std::swap(ly1, ly2);
	}

	for (const LineDef *L2 : inst.level.linedefs)
	{
		if (L2->TouchesSector(sec_num, inst.level))
		{
			for (int pass = 0 ; pass < 2 ; pass++)
			{
				const Vertex *v2 = pass ? L2->End(inst.level) : L2->Start(inst.level);
				double dist = PerpDist(v2->xy(), v2double_t{ lx1,ly1 }, v2double_t{ lx2, ly2 });

				if (dist > best_dist)
				{
					v = v2;
					best_dist = dist;
				}
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

void sector_info_cache_c::PlaneCopy(const LineDef *L, int f1_tag, int c1_tag, int f2_tag, int c2_tag, int share)
{
	for (int n = 0 ; n < inst.level.numSectors(); n++)
	{
		if (f1_tag > 0 && inst.level.sectors[n]->tag == f1_tag && L->Right(inst.level))
		{
			infos[L->Right(inst.level)->sector].floors.f_plane.Copy(infos[n].floors.f_plane);
			f1_tag = 0;
		}
		if (c1_tag > 0 && inst.level.sectors[n]->tag == c1_tag && L->Right(inst.level))
		{
			infos[L->Right(inst.level)->sector].floors.c_plane.Copy(infos[n].floors.c_plane);
			c1_tag = 0;
		}

		if (f2_tag > 0 && inst.level.sectors[n]->tag == f2_tag && L->Left(inst.level))
		{
			infos[L->Left(inst.level)->sector].floors.f_plane.Copy(infos[n].floors.f_plane);
			f2_tag = 0;
		}
		if (c2_tag > 0 && inst.level.sectors[n]->tag == c2_tag && L->Left(inst.level))
		{
			infos[L->Left(inst.level)->sector].floors.c_plane.Copy(infos[n].floors.c_plane);
			c2_tag = 0;
		}
	}

	if (L->left >= 0 && L->right >= 0)
	{
		int front_sec = L->Right(inst.level)->sector;
		int  back_sec = L->Left(inst.level)->sector;

		switch (share & 3)
		{
		case 1: infos[ back_sec].floors.f_plane.Copy(infos[front_sec].floors.f_plane); break;
		case 2: infos[front_sec].floors.f_plane.Copy(infos[ back_sec].floors.f_plane); break;
		default: break;
		}

		switch (share & 12)
		{
		case 4: infos[ back_sec].floors.c_plane.Copy(infos[front_sec].floors.c_plane); break;
		case 8: infos[front_sec].floors.c_plane.Copy(infos[ back_sec].floors.c_plane); break;
		default: break;
		}
	}
}

void sector_info_cache_c::PlaneCopyFromThing(const Thing *T, int plane)
{
	if (T->arg1 == 0)
		return;

	// find sector containing the thing
	Objid o = inst.level.hover.getNearbyObject(ObjType::sectors, { T->x(), T->y() });

	if (!o.valid())
		return;

	for (int n = 0 ; n < inst.level.numSectors(); n++)
	{
		if (inst.level.sectors[n]->tag == T->arg1)
		{
			if (plane > 0)
				infos[o.num].floors.c_plane.Copy(infos[n].floors.c_plane);
			else
				infos[o.num].floors.f_plane.Copy(infos[n].floors.f_plane);

			return;
		}
	}
}

void sector_info_cache_c::PlaneTiltByThing(const Thing *T, int plane)
{
	double tx = T->x();
	double ty = T->y();

	// find sector containing the thing
	Objid o = inst.level.hover.getNearbyObject(ObjType::sectors, { tx, ty });

	if (!o.valid())
		return;

	sector_3dfloors_c *ex = &infos[o.num].floors;

	double tz = ex->PlaneZ(plane ? -1 : +1, tx, ty) + T->h();

	// vector for direction of thing
	double tdx = cos(T->angle * M_PI / 180.0);
	double tdy = sin(T->angle * M_PI / 180.0);

	// get slope angle.
	// when arg1 < 90, a point on plane in front of thing has lower Z.
	int slope_ang = T->arg1 - 90;
	slope_ang = CLAMP(-89, slope_ang, 89);

	double az = sin(slope_ang * M_PI / 180.0);

	// FIXME support tilting an existing slope
#if 1
	tdx *= 128.0;
	tdy *= 128.0;
	az  *= 128.0;

	if (plane > 0)
		SlopeFromLine(ex->c_plane, tx,ty,tz, tx+tdx, ty+tdy, tz+az);
	else
		SlopeFromLine(ex->f_plane, tx,ty,tz, tx+tdx, ty+tdy, tz+az);
#else
	// get normal of existing plane
	double nx, ny, nz;
	if (plane == 0)
	{
		ex->f_plane.GetNormal(nx, ny, nz);
	}
	else
	{
		ex->c_plane.GetNormal(nx, ny, nz);
		nx = -nx; ny = -ny; nz = -nz;
	}
#endif
}

void sector_info_cache_c::SlopeFromLine(slope_plane_c& pl, double x1, double y1, double z1,
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

	pl.xm = static_cast<float>(dx * A);
	pl.ym = static_cast<float>(dy * A);
	pl.zadd = static_cast<float>(B);
	pl.sloped = true;
}


static void R_SubdivideSector(Instance &inst, int num, sector_extra_info_t& exinfo)
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
		const LineDef *L = inst.level.linedefs[n];

		if (! L->TouchesSector(num, inst.level))
			continue;

		// ignore 2S lines with same sector on both sides
		if (L->WhatSector(Side::left, inst.level) == L->WhatSector(Side::right, inst.level))
			continue;

		sector_edge_t edge;

		edge.x1 = static_cast<int>(L->Start(inst.level)->x());
		edge.y1 = static_cast<int>(L->Start(inst.level)->y());
		edge.x2 = static_cast<int>(L->End(inst.level)->x());
		edge.y2 = static_cast<int>(L->End(inst.level)->y());

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
		bool is_right = (L->WhatSector(Side::right, inst.level) == num);

		if (edge.flipped)
			is_right = !is_right;

		edge.side = is_right ? Side::right : Side::left;

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

				if (A->y1 > low_y) 
					high_y = std::min(high_y, A->y1);
				if (A->y2 > low_y) 
					high_y = std::min(high_y, A->y2);
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
		float mid_y = low_y + (high_y - low_y) * 0.5f;

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

			if (! (E1->side == Side::right && E2->side == Side::left))
				continue;

			// treat lines without a right side as dead
			// [ NOTE that we don't ignore them ]
			if (E1->line->right < 0) continue;
			if (E2->line->right < 0) continue;

			float lx1 = static_cast<float>(E1->CalcX(static_cast<float>(low_y)));
			float hx1 = static_cast<float>(E1->CalcX(static_cast<float>(high_y)));

			float lx2 = static_cast<float>(E2->CalcX(static_cast<float>(low_y)));
			float hx2 = static_cast<float>(E2->CalcX(static_cast<float>(high_y)));

			exinfo.sub.AddPolygon(lx1, lx2, static_cast<float>(low_y), hx1, hx2, static_cast<float>(high_y));
		}

		// ok, repeat process for next row
		low_y = high_y;
	}
}


void Instance::Subdiv_InvalidateAll()
{
	// invalidate everything
	sector_info_cache.total = -1;
}


bool Instance::Subdiv_SectorOnScreen(int num, double map_lx, double map_ly, double map_hx, double map_hy)
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


sector_subdivision_c *Instance::Subdiv_PolygonsForSector(int num)
{
	sector_info_cache.Update();

	sector_extra_info_t& exinfo = sector_info_cache.infos[num];

	if (! exinfo.built)
	{
		R_SubdivideSector(*this, num, exinfo);
		exinfo.built = true;
	}

	return &exinfo.sub;
}


//------------------------------------------------------------------------
//  3D Floor and Slope stuff
//------------------------------------------------------------------------

void sector_3dfloors_c::Clear()
{
	heightsec = -1;
	floors.clear();
	f_plane.Init(-2);
	c_plane.Init(-1);
}

void slope_plane_c::Init(float height)
{
	sloped = false;
	xm = ym = 0;
	zadd = height;
}

void slope_plane_c::Copy(const slope_plane_c& other)
{
	sloped = other.sloped;
	xm = other.xm;
	ym = other.ym;
	zadd = other.zadd;
}


sector_3dfloors_c *Instance::Subdiv_3DFloorsForSector(int num)
{
	sector_info_cache.Update();

	sector_extra_info_t& exinfo = sector_info_cache.infos[num];

	return &exinfo.floors;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
