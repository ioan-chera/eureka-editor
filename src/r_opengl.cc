//------------------------------------------------------------------------
//  3D RENDERING : OPENGL
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

#ifndef NO_OPENGL

#include "main.h"

#include <map>
#include <algorithm>

#include "FL/gl.h"

#include "e_main.h"
#include "m_game.h"
#include "m_bitvec.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "r_render.h"
#include "r_subdiv.h"


extern rgb_color_t transparent_col;

extern bool render_high_detail;
extern bool render_lock_gravity;
extern bool render_missing_bright;
extern bool render_unknown_bright;
extern int  render_pixel_aspect;


// convert from our coordinate system (looking along +X)
// to OpenGL's coordinate system (looking down -Z).
static GLdouble flip_matrix[16] =
{
	 0,  0, -1,  0,
	-1,  0,  0,  0,
	 0, +1,  0,  0,
	 0,  0,  0, +1
};


struct RendInfo
{
public:
	bitvec_c seen_sectors;

public:
	RendInfo() : seen_sectors(NumSectors + 1)
	{ }

	~RendInfo()
	{ }

	static inline float PointToAngle(float x, float y)
	{
		if (-0.01 < x && x < 0.01)
			return (y > 0) ? M_PI/2 : (3 * M_PI/2);

		float angle = atan2(y, x);

		if (angle < 0)
			angle += 2*M_PI;

		return angle;
	}

	static inline int AngleToX(float ang)
	{
		float t = tan(M_PI/2 - ang);

		int x = int(r_view.aspect_sw * t);

		x = (r_view.screen_w + x) / 2;

		if (x < 0)
			x = 0;
		else if (x > r_view.screen_w)
			x = r_view.screen_w;

		return x;
	}

	static inline int DeltaToX(double iz, float tx)
	{
		int x = int(r_view.aspect_sw * tx * iz);

		x = (x + r_view.screen_w) / 2;

		return x;
	}

	Img_c *FindFlat(const char *fname, byte& r, byte& g, byte& b)
	{
		bool fullbright = false;  // TODO no lighting yet...

		if (is_sky(fname))
		{
			glBindTexture(GL_TEXTURE_2D, 0);

			IM_DecodePixel(game_info.sky_color, r, g, b);
			return NULL;
		}

		if (! r_view.texturing)
		{
			glBindTexture(GL_TEXTURE_2D, 0);

			int col = HashedPalColor(fname, game_info.floor_colors);
			IM_DecodePixel(col, r, g, b);
			return NULL;
		}

		Img_c *img = W_GetFlat(fname);
		if (! img)
		{
			img = IM_UnknownFlat();
			fullbright = render_unknown_bright;
		}

		img->bind_gl();

		r = g = b = 255;
		return img;
	}

	void DrawSectorPolygons(const Sector *sec, sector_subdivision_c *subdiv,
		float z, const char *fname)
	{
		byte r, g, b;
		Img_c *img = FindFlat(fname, r, g, b);

		glColor3f(r / 255.0, g / 255.0, b / 255.0);

		for (unsigned int i = 0 ; i < subdiv->polygons.size() ; i++)
		{
			sector_polygon_t *poly = &subdiv->polygons[i];

			// draw polygon
			glBegin(GL_POLYGON);

			for (int p = 0 ; p < poly->count ; p++)
			{
				float px = poly->mx[p];
				float py = poly->my[p];

				if (img)
				{
					// this logic follows ZDoom, which scales large flats to
					// occupy a 64x64 unit area.  I presume wall textures are
					// handled similarily....
					glTexCoord2f(px / 64.0, py / 64.0);
				}

				glVertex3f(px, py, z);
			}

			glEnd();
		}
	}

	void DrawLine(int ld_index)
	{
		LineDef *ld = LineDefs[ld_index];

		if (! is_vertex(ld->start) || ! is_vertex(ld->end))
			return;

		if (! ld->Right())
			return;

		float x1 = ld->Start()->x - r_view.x;
		float y1 = ld->Start()->y - r_view.y;
		float x2 = ld->End()->x - r_view.x;
		float y2 = ld->End()->y - r_view.y;

		float tx1 = x1 * r_view.Sin - y1 * r_view.Cos;
		float ty1 = x1 * r_view.Cos + y1 * r_view.Sin;
		float tx2 = x2 * r_view.Sin - y2 * r_view.Cos;
		float ty2 = x2 * r_view.Cos + y2 * r_view.Sin;

		// reject line if complete behind viewplane
		if (ty1 <= 0 && ty2 <= 0)
			return;

		float angle1 = PointToAngle(tx1, ty1);
		float angle2 = PointToAngle(tx2, ty2);
		float span = angle1 - angle2;

		if (span < 0)
			span += 2*M_PI;

		int side = SIDE_RIGHT;

		if (span >= M_PI)
			side = SIDE_LEFT;

		// ignore the line when there is no facing sidedef
		SideDef *sd = (side == SIDE_LEFT) ? ld->Left() : ld->Right();

		if (! sd)
			return;

		if (side == SIDE_LEFT)
		{
			float tmp = angle1;
			angle1 = angle2;
			angle2 = tmp;
		}

		// clip angles to view volume

		float base_ang = angle1;

		float leftclip  = (3 * M_PI / 4);
		float rightclip = M_PI / 4;

		float tspan1 = angle1 - rightclip;
		float tspan2 = leftclip - angle2;

		if (tspan1 < 0) tspan1 += 2*M_PI;
		if (tspan2 < 0) tspan2 += 2*M_PI;

		if (tspan1 > M_PI/2)
		{
			// Totally off the left edge?
			if (tspan2 >= M_PI)
				return;

			angle1 = leftclip;
		}

		if (tspan2 > M_PI/2)
		{
			// Totally off the left edge?
			if (tspan1 >= M_PI)
				return;

			angle2 = rightclip;
		}

		// convert angles to on-screen X positions
		int sx1 = AngleToX(angle1);
		int sx2 = AngleToX(angle2) - 1;

		if (sx1 > sx2)
			return;

		// compute distance from eye to wall
		float wdx = x2 - x1;
		float wdy = y2 - y1;

		float wlen = sqrt(wdx * wdx + wdy * wdy);
		float dist = fabs((y1 * wdx / wlen) - (x1 * wdy / wlen));

		if (dist < 0.01)
			return;

		/* TODO : mark sectors to draw */

		/* actually draw it... */

		// FIXME DrawLine
	}

	void DrawSector(int sec_index)
	{
		// FIXME DrawSector

		sector_subdivision_c *subdiv = Subdiv_PolygonsForSector(sec_index);

		if (! subdiv)
			return;

		const Sector *sec = Sectors[sec_index];

		glColor3f(1, 1, 1);

		if (r_view.z > sec->floorh)
		{
			DrawSectorPolygons(sec, subdiv, sec->floorh, sec->FloorTex());
		}

		if (r_view.z < sec->ceilh)
		{
			DrawSectorPolygons(sec, subdiv, sec->ceilh, sec->CeilTex());
		}
	}

	void DrawThing(int th_index)
	{
		Thing *th = Things[th_index];

		const thingtype_t *info = M_GetThingType(th->type);

		float x = th->x - r_view.x;
		float y = th->y - r_view.y;

		float tx = x * r_view.Sin - y * r_view.Cos;
		float ty = x * r_view.Cos + y * r_view.Sin;

		// reject sprite if complete behind viewplane
		if (ty < 4)
			return;

		bool is_unknown = false;

		float scale = info->scale;

		Img_c *sprite = W_GetSprite(th->type);
		if (! sprite)
		{
			sprite = IM_UnknownSprite();
			is_unknown = true;
		}

		float tx1 = tx - sprite->width() * scale / 2.0;
		float tx2 = tx + sprite->width() * scale / 2.0;

		double iz = 1 / ty;

		int sx1 = DeltaToX(iz, tx1);
		int sx2 = DeltaToX(iz, tx2) - 1;

		if (sx1 < 0)
			sx1 = 0;

		if (sx2 >= r_view.screen_w)
			sx2 = r_view.screen_w - 1;

		if (sx1 > sx2)
			return;

		int thsec = r_view.thing_sectors[th_index];

		int h1, h2;

		if (info && (info->flags & THINGDEF_CEIL))
		{
			// IOANCH 9/2015: also add z
			h2 = (is_sector(thsec) ? Sectors[thsec]->ceilh : 192) - th->z;
			h1 = h2 - sprite->height() * scale;
		}
		else
		{
			h1 = (is_sector(thsec) ? Sectors[thsec]->floorh : 0) + th->z;
			h2 = h1 + sprite->height() * scale;
		}

		/* actually draw it... */

		// FIXME DrawThing
	}

	void Render()
	{
		for (int i=0 ; i < NumLineDefs ; i++)
			DrawLine(i);

		for (int s=0 ; s < NumSectors ; s++)
			DrawSector(s);

		if (r_view.sprites)
			for (int t=0 ; t < NumThings ; t++)
				DrawThing(t);
	}

	void Begin(int ow, int oh)
	{
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_ALPHA_TEST);
		glEnable(GL_ALPHA_TEST);

		glDisable(GL_CULL_FACE);

		glAlphaFunc(GL_GREATER, 0.5);

		// setup projection

		glViewport(0, 0, ow, oh);

		GLdouble angle = r_view.angle * 180.0 / M_PI;

		// the model-view matrix does three things:
		//   1. translates X/Y/Z by view position
		//   2. rotates around Y axis by view angle
		//   3. flips coordinates so camera looks down -Z
		//
		// due to how matrix multiplication works, these things
		// must be specified in the reverse order as above.
		glMatrixMode(GL_MODELVIEW);

		glLoadMatrixd(flip_matrix);
		glRotated(-angle, 0, 0, +1);
		glTranslated(-r_view.x, -r_view.y, -r_view.z);

		// the projection matrix creates the 3D perspective
		glMatrixMode(GL_PROJECTION);

		float x_slope = 100.0 / render_pixel_aspect;
		float y_slope = (float)oh / (float)ow;

		// this matches behavior of S/W renderer.
		// [ currently it is important since we use the S/W path
		//   for querying what the mouse is pointing at ]
		float near = x_slope;
		float far  = 32767.0;

		glLoadIdentity();
		glFrustum(-x_slope, +x_slope, -y_slope, +y_slope, near, far);
	}

	void Finish()
	{
		// reset state
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);

		// reset matrices
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
};


void RGL_RenderWorld(int ox, int oy, int ow, int oh)
{
	RendInfo rend;

	rend.Begin(ow, oh);
	rend.Render();
	rend.Finish();
}

#endif /* NO_OPENGL */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
