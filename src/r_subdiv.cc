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

//#include "m_events.h"
//#include "e_main.h"
//#include "e_hover.h"
//#include "e_sector.h"
//#include "e_things.h"
//#include "m_config.h"
//#include "m_game.h"
//#include "r_grid.h"
//#include "im_color.h"
//#include "im_img.h"
//#include "r_render.h"
//#include "w_rawdef.h"	// MLF_xxx
//#include "w_texture.h"

/* This file contains code for subdividing map sectors into a set
   of polygons, and also the logic for caching these subdivisions
   and rebuilding them whenever necessary.
*/


// this represents a segment of a linedef bounding a sector.
struct sector_edge_t
{
	const LineDef * line;

	// coordinates mapped to screen space, not clipped.
	// we always have scr_y1 <= scr_y2.
	int scr_x1, scr_y1;
	int scr_x2, scr_y2;

	// has the line been flipped (coordinates were swapped) ?
	short flipped;

	// what side this edge faces (SIDE_LEFT or SIDE_RIGHT)
	short side;

	// clipped vertical range, inclusive
	short y1, y2;

	// computed X value
	float x;

	float CalcX(float y) const
	{
		return scr_x1 + (scr_x2 - scr_x1) * (float)(y - scr_y1) / (float)(scr_y2 - scr_y1);
	}

	struct CMP_Y
	{
		inline bool operator() (const sector_edge_t &A, const sector_edge_t& B) const
		{
			return A.scr_y1 < B.scr_y1;
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

			return A->x < B->x;
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

	void Clear()
	{
		first_line = last_line = -1;

		bound_x1 = 32767;
		bound_y1 = 32767;
		bound_x2 = -32767;
		bound_y2 = -32767;
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


static void R_SubdivideSector(int num)
{
	sector_info_cache.Update();

	sector_extra_info_t& exinfo = sector_info_cache.infos[num];

	if (exinfo.first_line < 0)
		return;


///  fprintf(stderr, "RenderSector %d\n", num);

	rgb_color_t light_col = SectorLightColor(Sectors[num]->light);

	const char * tex_name = NULL;

	Img_c * img = NULL;

	if (edit.sector_render_mode == SREND_Lighting)
	{
		gl_color(light_col);
	}
	else if (edit.sector_render_mode == SREND_SoundProp)
	{
		if (edit.mode != OBJ_SECTORS || !edit.highlight.valid())
			return;

		const byte * prop = SoundPropagation(edit.highlight.num);

		switch ((propagate_level_e) prop[num])
		{
			case PGL_Never:   return;
			case PGL_Maybe:   gl_color(fl_rgb_color(64,64,192)); break;
			case PGL_Level_1: gl_color(fl_rgb_color(192,32,32)); break;
			case PGL_Level_2: gl_color(fl_rgb_color(192,96,32)); break;
		}
	}
	else
	{
		if (edit.sector_render_mode == SREND_Ceiling)
			tex_name = Sectors[num]->CeilTex();
		else
			tex_name = Sectors[num]->FloorTex();

		if (is_sky(tex_name))
		{
			gl_color(palette[game_info.sky_color]);
		}
		else
		{
			img = W_GetFlat(tex_name);

			if (! img)
			{
				img = IM_UnknownTex();
			}
		}
	}

	int tw = img ? img->width()  : 1;
	int th = img ? img->height() : 1;


	/*** Part 1 : visit linedefs and create edges ***/


	std::vector<sector_edge_t> edgelist;

	short min_y = 32767;
	short max_y = 0;


	for (int n = exinfo.first_line ; n <= exinfo.last_line ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (! L->TouchesSector(num))
			continue;

		// ignore 2S lines with same sector on both sides
		if (L->WhatSector(SIDE_LEFT) == L->WhatSector(SIDE_RIGHT))
			continue;

		sector_edge_t edge;

		edge.scr_x1 = SCREENX(L->Start()->x);
		edge.scr_y1 = SCREENY(L->Start()->y);
		edge.scr_x2 = SCREENX(L->End()->x);
		edge.scr_y2 = SCREENY(L->End()->y);

		// completely above or below the screen?
		if (MAX(edge.scr_y1, edge.scr_y2) < 0)
			continue;

		if (MIN(edge.scr_y1, edge.scr_y2) >= h())
			continue;

		// skip horizontal lines   [ TODO REVIEW THIS ]
		if (edge.scr_y1 == edge.scr_y2)
			continue;

		edge.flipped = 0;

		if (edge.scr_y1 > edge.scr_y2)
		{
			std::swap(edge.scr_x1, edge.scr_x2);
			std::swap(edge.scr_y1, edge.scr_y2);

			edge.flipped = 1;
		}

		// compute usable range, clipping to screen
		edge.y1 = MAX(edge.scr_y1, 0);
		edge.y2 = MIN(edge.scr_y2, h() - 1);

		// this probably cannot happen....
		if (edge.y2 < edge.y1)
			continue;

		min_y = MIN(min_y, edge.y1);
		max_y = MAX(max_y, edge.y2);

		// compute side
		bool is_right = !(L->WhatSector(SIDE_LEFT) == num);

		if (edge.flipped) is_right = !is_right;

		edge.side = is_right ? SIDE_RIGHT : SIDE_LEFT;

/*
fprintf(stderr, "Line %d  mapped coords (%d %d) .. (%d %d)  flipped:%d  sec:%d/%d\n",
n, edge.scr_x1, edge.scr_y1, edge.scr_x2, edge.scr_y2, edge.flipped,
L->WhatSector(SIDE_RIGHT), L->WhatSector(SIDE_LEFT));
*/

		// add the edge
		edge.line = L;

		edgelist.push_back(edge);
	}

	if (edgelist.empty())
		return;

	// sort edges into vertical order (i.e. by scr_y1)

	std::sort(edgelist.begin(), edgelist.end(), sector_edge_t::CMP_Y());


	/*** Part 2 : traverse edge list and render trapezoids ***/


	unsigned int next_edge = 0;

	std::vector<sector_edge_t *> active_edges;

	unsigned int i;

	short next_y;

	// visit each row of trapezoids
	for (short y = min_y ; y <= max_y ; y = next_y)
	{
		// remove old edges from active list
		for (i = 0 ; i < active_edges.size() ; i++)
		{
			if (active_edges[i]->y2 < y)
				active_edges[i] = NULL;
		}

		// add new edges to active list
		for ( ; next_edge < edgelist.size() && y >= edgelist[next_edge].y1 ; next_edge++)
		{
			active_edges.push_back(&edgelist[next_edge]);
		}

/// fprintf(stderr, "  active @ y=%d --> %d\n", y, (int)active_edges.size());

		// sort active edges by X value
		// [ also puts NULL entries at end, making easy to remove them ]

		next_y = h();

		for (i = 0 ; i < active_edges.size() ; i++)
		{
			if (active_edges[i])
				next_y = MIN(next_y, active_edges[i]->y2);
		}

		for (i = 0 ; i < active_edges.size() ; i++)
		{
			if (active_edges[i])
				active_edges[i]->x = active_edges[i]->CalcX(y + 0.5);
		}

/// fprintf(stderr, "      next_y=%d\n", next_y);

		std::sort(active_edges.begin(), active_edges.end(), sector_edge_t::CMP_X());

		while (active_edges.size() > 0 && active_edges.back() == NULL)
			active_edges.pop_back();

		if (active_edges.empty())
		{
			// FIXME
			next_y = y + 1;
			continue;
		}

		// compute spans

		for (unsigned int i = 1 ; i < active_edges.size() ; i++)
		{
			const sector_edge_t * E1 = active_edges[i - 1];
			const sector_edge_t * E2 = active_edges[i];
#if 1
			if (E1 == NULL || E2 == NULL)
				BugError("RenderSector: did not delete NULLs properly!\n");
#endif

///     fprintf(stderr, "E1 @ x=%1.2f side=%d  |  E2 @ x=%1.2f side=%d\n",
///			 E1->x, E1->side, E2->x, E2->side);

			if (! (E1->side == SIDE_RIGHT && E2->side == SIDE_LEFT))
				continue;

			// treat lines without a right side as dead
			if (E1->line->right < 0) continue;
			if (E2->line->right < 0) continue;

			int lx1 = floor(E1->x);
			int lx2 = floor(E2->x);

			int hx1 = floor(E1->CalcX(next_y));
			int hx2 = floor(E2->CalcX(next_y));

			// completely off the screen?
/* REVIEW
			if (lx2 < 0 && hx2 < 0)
				continue;
			if (lx1 >= w() && hx1 >= w())
				continue;
*/
			// FIXME TEST-ONLY COLORING
			int r1 = rand() & 255;
			int r2 = rand() & 255;
			int r3 = rand() & 255;
			glColor3f(r1 / 255.0, r2 / 255.0, r3 / 255.0);

			// draw polygon
			glBegin(GL_POLYGON);

			glVertex2i(lx1, y);
			glVertex2i(hx1, next_y+1);
			glVertex2i(hx2, next_y+1);
			glVertex2i(lx2, y);

			glEnd();
		}

		if (next_y == y)
			next_y++;
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

	// FIXME Subdiv_PolygonsForSector
	return NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
