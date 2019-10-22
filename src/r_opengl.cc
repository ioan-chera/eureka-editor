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


// The emulation of DOOM lighting here targets a very basic
// version of OpenGL: 1.2 or so.  It works by clipping walls
// and sector triangles against a small set of infinite lines
// at specific distances from the camera.  Once clipped, the
// light level of a primitive is constant, determined from
// the pair of clipping lines it ends up in.

#define LCLIP_NUM  14

static const float light_clip_dists[LCLIP_NUM] =
{
	1280, 640, 428, 320, 256,
	216, 176, 144, 120, 104,
	88, 72, 60, 40
};


static float DoomLightToFloat(int light, float dist)
{
	int map = R_DoomLightingEquation(light, dist);

	int level = (31 - map) * 8 + 7;

	// need to gamma-correct the light level
	if (usegamma > 0)
		level = gammatable[usegamma][level];

	return level / 255.0;
}


struct RendInfo
{
public:
	// we don't test the visibility of sectors directly, instead we
	// mark sectors as visible when any linedef that touches the
	// sector is on-screen.
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

	Img_c *FindFlat(const char *fname, byte& r, byte& g, byte& b, bool& fullbright)
	{
		fullbright = false;

		if (is_sky(fname))
		{
			fullbright = true;
			glBindTexture(GL_TEXTURE_2D, 0);

			IM_DecodePixel(Misc_info.sky_color, r, g, b);
			return NULL;
		}

		if (! r_view.texturing)
		{
			glBindTexture(GL_TEXTURE_2D, 0);

			int col;

			// when lighting and no texturing, use a single color
			if (r_view.lighting)
				col = Misc_info.floor_colors[1];
			else
				col = HashedPalColor(fname, Misc_info.floor_colors);

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

	Img_c *FindTexture(const char *tname, byte& r, byte& g, byte& b, bool& fullbright)
	{
		fullbright = false;

		if (! r_view.texturing)
		{
			glBindTexture(GL_TEXTURE_2D, 0);

			int col;

			// when lighting and no texturing, use a single color
			if (r_view.lighting)
				col = Misc_info.wall_colors[1];
			else
				col = HashedPalColor(tname, Misc_info.wall_colors);

			IM_DecodePixel(col, r, g, b);
			return NULL;
		}

		Img_c *img;

		if (is_null_tex(tname))
		{
			img = IM_MissingTex();
			fullbright = render_missing_bright;
		}
		else
		{
			img = W_GetTexture(tname);

			if (! img)
			{
				img = IM_UnknownTex();
				fullbright = render_unknown_bright;
			}
		}

		img->bind_gl();

		r = g = b = 255;
		return img;
	}

	void RawClippedTriangle(float ax, float ay, float atx, float aty,
							float bx, float by, float btx, float bty,
							float cx, float cy, float ctx, float cty,
							float z, float r, float g, float b, float level)
	{
		glColor3f(level * r, level * g, level * b);

		glBegin(GL_POLYGON);

		glTexCoord2f(atx, aty); glVertex3f(ax, ay, z);
		glTexCoord2f(btx, bty); glVertex3f(bx, by, z);
		glTexCoord2f(ctx, cty); glVertex3f(cx, cy, z);

		glEnd();
	}

	void LightClippedTriangle(double ax, double ay, float atx, float aty,
							  double bx, double by, float btx, float bty,
							  double cx, double cy, float ctx, float cty,
							  int clip, float z,
							  float r, float g, float b, int light)
	{
		float level = DoomLightToFloat(light, light_clip_dists[LCLIP_NUM-1] + 2.0);

		for ( ; clip < LCLIP_NUM ; clip++)
		{
			float cdist = light_clip_dists[clip];

			level = DoomLightToFloat(light, cdist + 2.0);

			// coordinates of an infinite clipping line
			float c_x = r_view.x + r_view.Cos * cdist;
			float c_y = r_view.y + r_view.Sin * cdist;

			// vector of clipping line (if camera is north, this is east)
			float c_dx = r_view.Sin;
			float c_dy = - r_view.Cos;

			// check which side the triangle points are on
			double p1 = (ay - c_y) * c_dx - (ax - c_x) * c_dy;
			double p2 = (by - c_y) * c_dx - (bx - c_x) * c_dy;
			double p3 = (cy - c_y) * c_dx - (cx - c_x) * c_dy;

			int cat1 = (p1 < -0.1) ? -1 : (p1 > 0.1) ? +1 : 0;
			int cat2 = (p2 < -0.1) ? -1 : (p2 > 0.1) ? +1 : 0;
			int cat3 = (p3 < -0.1) ? -1 : (p3 > 0.1) ? +1 : 0;

			// completely on far side?
			if (cat1 >= 0 && cat2 >= 0 && cat3 >= 0)
				break;

			// completely on near side?
			if (cat1 <= 0 && cat2 <= 0 && cat3 <= 0)
				continue;

			// handle cases where partition goes through a vertex AND
			// cuts the opposite edge.
			if (cat1 == 0 || cat2 == 0 || cat3 == 0)
			{
				// rotate triangle so that C is on the partition,
				// and the edge AB is crossing the partition.
				if (cat1 == 0)
				{
					float _x = cx;
					float _y = cy;
					float _tx = ctx;
					float _ty = cty;

					cx = ax; cy = ay; ctx = atx; cty = aty;
					ax = bx; ay = by; atx = btx; aty = bty;
					bx = _x; by = _y; btx = _tx; bty = _ty;

					p1 = p2; p2 = p3; cat1 = cat2;
				}
				else if (cat2 == 0)
				{
					float _x = cx;
					float _y = cy;
					float _tx = ctx;
					float _ty = cty;

					cx = bx; cy = by; ctx = btx; cty = bty;
					bx = ax; by = ay; btx = atx; bty = aty;
					ax = _x; ay = _y; atx = _tx; aty = _ty;

					p2 = p1; p1 = p3; cat1 = cat3;
				}

				// compute intersection point
				double along = p1 / (p1 - p2);

				double ix = ax + (bx - ax) * along;
				double iy = ay + (by - ay) * along;
				float itx = atx + (btx - atx) * along;
				float ity = aty + (bty - aty) * along;

				// draw the piece on FAR side of the clipping line,
				// and keep going with the piece on the NEAR side.
				if (cat1 > 0)
				{
					RawClippedTriangle(ix, iy, itx, ity,
									   cx, cy, ctx, cty,
									   ax, ay, atx, aty,
									   z, r, g, b, level);

					ax = ix; ay = iy; atx = itx; aty = ity;
				}
				else
				{
					RawClippedTriangle(ix, iy, itx, ity,
									   cx, cy, ctx, cty,
									   bx, by, btx, bty,
									   z, r, g, b, level);

					bx = ix; by = iy; btx = itx; bty = ity;
				}

				continue;
			}

			// AT HERE, the partition definitely cuts two edges.
			// The cut produces a triangle piece and a quadrilateral
			// piece.

			int combo = ((cat3>0)?4:0) | ((cat2>0)?2:0) | ((cat1>0)?1:0);

			// rotate triangle so that C is the tip of the triangle piece.
			if (combo == 1 || combo == 6)
			{
				float _x = cx;
				float _y = cy;
				float _tx = ctx;
				float _ty = cty;
				double _p = p3;

				cx = ax; cy = ay; ctx = atx; cty = aty; p3 = p1;
				ax = bx; ay = by; atx = btx; aty = bty; p1 = p2;
				bx = _x; by = _y; btx = _tx; bty = _ty; p2 = _p;
			}
			else if (combo == 2 || combo == 5)
			{
				float _x = cx;
				float _y = cy;
				float _tx = ctx;
				float _ty = cty;
				double _p = p3;

				cx = bx; cy = by; ctx = btx; cty = bty; p3 = p2;
				bx = ax; by = ay; btx = atx; bty = aty; p2 = p1;
				ax = _x; ay = _y; atx = _tx; aty = _ty; p1 = _p;
			}

			// compute intersection points
			double ac_along = p1 / (p1 - p3);
			double bc_along = p2 / (p2 - p3);

			double ac_x = ax + (cx - ax) * ac_along;
			double ac_y = ay + (cy - ay) * ac_along;
			double bc_x = bx + (cx - bx) * bc_along;
			double bc_y = by + (cy - by) * bc_along;

			float ac_tx = atx + (ctx - atx) * ac_along;
			float ac_ty = aty + (cty - aty) * ac_along;
			float bc_tx = btx + (ctx - btx) * bc_along;
			float bc_ty = bty + (cty - bty) * bc_along;

			// handle cases where triangle piece is on NEAR side.
			if (combo == 3 || combo == 5 || combo == 6)
			{
				RawClippedTriangle(ax, ay, atx, aty,
								   bx, by, btx, bty,
								   ac_x, ac_y, ac_tx, ac_ty,
								   z, r, g, b, level);

				RawClippedTriangle(bx, by, btx, bty,
								   bc_x, bc_y, bc_tx, bc_ty,
								   ac_x, ac_y, ac_tx, ac_ty,
								   z, r, g, b, level);

				ax = ac_x; ay = ac_y; atx = ac_tx; aty = ac_ty;
				bx = bc_x; by = bc_y; btx = bc_tx; bty = bc_ty;

				continue;
			}

			// handle cases where triangle piece is on FAR side.
			// these cases require recursion to deal with the
			// quadrilateral on the near side.
			{
				RawClippedTriangle(cx, cy, ctx, cty,
								   ac_x, ac_y, ac_tx, ac_ty,
								   bc_x, bc_y, bc_tx, bc_ty,
								   z, r, g, b, level);

				// recurse!
				LightClippedTriangle(ax, ay, atx, aty,
									 ac_x, ac_y, ac_tx, ac_ty,
									 bc_x, bc_y, bc_tx, bc_ty,
									 (clip + 1), z, r, g, b, light);

				cx = bc_x; cy = bc_y; ctx = bc_tx; cty = bc_ty;
			}
		}

		RawClippedTriangle(ax, ay, atx, aty,
						   bx, by, btx, bty,
						   cx, cy, ctx, cty,
						   z, r, g, b, level);
	}

	void RawClippedQuad(float x1, float y1, float z1,
						float x2, float y2, float z2,
						float tx1, float ty1, float tx2, float ty2,
						float r, float g, float b, float level)
	{
		glColor3f(level * r, level * g, level * b);

		glBegin(GL_QUADS);

		glTexCoord2f(tx1, ty1); glVertex3f(x1, y1, z1);
		glTexCoord2f(tx1, ty2); glVertex3f(x1, y1, z2);
		glTexCoord2f(tx2, ty2); glVertex3f(x2, y2, z2);
		glTexCoord2f(tx2, ty1); glVertex3f(x2, y2, z1);

		glEnd();
	}

	void LightClippedQuad(double x1, double y1, float z1,
						  double x2, double y2, float z2,
						  float tx1, float ty1, float tx2, float ty2,
						  float r, float g, float b, int light)
	{
		float level = DoomLightToFloat(light, light_clip_dists[LCLIP_NUM-1] + 2.0);

		for (int clip = 0 ; clip < LCLIP_NUM ; clip++)
		{
			float cdist = light_clip_dists[clip];

			level = DoomLightToFloat(light, cdist + 2.0);

			// coordinates of an infinite clipping line
			float c_x = r_view.x + r_view.Cos * cdist;
			float c_y = r_view.y + r_view.Sin * cdist;

			// vector of clipping line (if camera is north, this is east)
			float c_dx = r_view.Sin;
			float c_dy = - r_view.Cos;

			// check which side the start/end point is on
			double p1 = (y1 - c_y) * c_dx - (x1 - c_x) * c_dy;
			double p2 = (y2 - c_y) * c_dx - (x2 - c_x) * c_dy;

			int cat1 = (p1 < -0.1) ? -1 : (p1 > 0.1) ? +1 : 0;
			int cat2 = (p2 < -0.1) ? -1 : (p2 > 0.1) ? +1 : 0;

			// completely on far side?
			if (cat1 >= 0 && cat2 >= 0)
				break;

			// does it cross the partition?
			if ((cat1 < 0 && cat2 > 0) || (cat1 > 0 && cat2 < 0))
			{
				// compute intersection point
				double along = p1 / (p1 - p2);

				double ix = x1 + (x2 - x1) * along;
				double iy = y1 + (y2 - y1) * along;

				float itx = tx1 + (tx2 - tx1) * along;

				// draw the piece on FAR side of the clipping line,
				// and keep going with the piece on the NEAR side.
				if (cat2 > 0)
				{
					RawClippedQuad(ix, iy, z1, x2, y2, z2,
								   itx, ty1, tx2, ty2, r, g, b, level);

					x2 = ix; y2 = iy; tx2 = itx;
				}
				else
				{
					RawClippedQuad(x1, y1, z1, ix, iy, z2,
								   tx1, ty1, itx, ty2, r, g, b, level);

					x1 = ix; y1 = iy; tx1 = itx;
				}
			}
		}

		RawClippedQuad(x1, y1, z1, x2, y2, z2,
					   tx1, ty1, tx2, ty2, r, g, b, level);
	}

	void DrawSectorPolygons(const Sector *sec, sector_subdivision_c *subdiv,
		float z, const char *fname)
	{
		byte r0, g0, b0;
		bool fullbright;
		Img_c *img = FindFlat(fname, r0, g0, b0, fullbright);

		float r = r0 / 255.0;
		float g = g0 / 255.0;
		float b = b0 / 255.0;

		for (unsigned int i = 0 ; i < subdiv->polygons.size() ; i++)
		{
			sector_polygon_t *poly = &subdiv->polygons[i];

			if (r_view.lighting && !fullbright)
			{
				float ax = poly->mx[0];
				float ay = poly->my[0];
				float atx = ax / 64.0;  // see note below
				float aty = ay / 64.0;

				float bx = poly->mx[1];
				float by = poly->my[1];
				float btx = bx / 64.0;  // see note below
				float bty = by / 64.0;

				float cx = poly->mx[2];
				float cy = poly->my[2];
				float ctx = cx / 64.0;
				float cty = cy / 64.0;

				LightClippedTriangle(ax, ay, atx, aty,
									 bx, by, btx, bty,
									 cx, cy, ctx, cty,
									 0, z, r, g, b, sec->light);

				if (poly->count == 4)
				{
					float dx = poly->mx[3];
					float dy = poly->my[3];
					float dtx = dx / 64.0;
					float dty = dy / 64.0;

					LightClippedTriangle(ax, ay, atx, aty,
										 cx, cy, ctx, cty,
										 dx, dy, dtx, dty,
										 0, z, r, g, b, sec->light);
				}
			}
			else
			{
				glColor3f(r, g, b);
				glBegin(GL_POLYGON);

				for (int p = 0 ; p < poly->count ; p++)
				{
					float px = poly->mx[p];
					float py = poly->my[p];

					if (img)
					{
						// this logic follows ZDoom, which scales large flats to
						// occupy a 64x64 unit area.  I presume wall textures
						// used on floors or ceilings is the same....
						glTexCoord2f(px / 64.0, py / 64.0);
					}

					glVertex3f(px, py, z);
				}

				glEnd();
			}
		}
	}

	// the "where" parameter can be:
	//   - 'W' for one-sided wall
	//   - 'L' for lower
	//   - 'U' for upper
	void DrawSide(char where, const LineDef *ld, const SideDef *sd,
		const char *texname, const Sector *front, const Sector *back,
		bool sky_upper, float ld_length,
		float x1, float y1, float z1, float x2, float y2, float z2)
	{
		byte r, g, b;
		bool fullbright = true;
		Img_c *img = NULL;

		if (sky_upper && where == 'U')
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			IM_DecodePixel(Misc_info.sky_color, r, g, b);
		}
		else
		{
			img = FindTexture(texname, r, g, b, fullbright);
		}

		// compute texture coords
		float tx1 = 0.0;
		float ty1 = 0.0;
		float tx2 = 1.0;
		float ty2 = 1.0;

		if (img)
		{
			float img_w  = img->width();
			float img_tw = RoundPOW2(img_w);

			float img_h  = img->height();
			float img_th = RoundPOW2(img_h);

			tx1 = 0;
			tx2 = tx1 + ld_length;

			// the common case: top of texture is at z2
			ty2 = img_h;
			ty1 = ty2 - (z2 - z1);

			if (where == 'W' && (ld->flags & MLF_LowerUnpegged))
			{
				ty1 = 0;
				ty2 = ty1 + (z2 - z1);
			}

			if (where == 'L' && (ld->flags & MLF_LowerUnpegged))
			{
				// note "sky_upper" here, this matches original DOOM behavior
				if (sky_upper)
				{
					ty2 = img_h - (back->ceilh - z2);
					ty1 = ty2 - (z2 - z1);
				}
				else
				{
					// this makes an unpegged lower align with a normal 1S wall
					ty2 = img_h - (front->ceilh - z2);
					ty1 = ty2 - (z2 - z1);
				}
			}

			if (where == 'U' && (! (ld->flags & MLF_UpperUnpegged)))
			{
				// when unpegged, ty1 is at bottom of texture
				ty1 = 0;
				ty2 = ty1 + (z2 - z1);
			}

			tx1 = (tx1 + sd->x_offset) / img_tw;
			tx2 = (tx2 + sd->x_offset) / img_tw;

			ty1 = (ty1 - sd->y_offset) / img_th;
			ty2 = (ty2 - sd->y_offset) / img_th;
		}

		glDisable(GL_ALPHA_TEST);

		if (r_view.lighting && !fullbright)
		{
			int light = sd->SecRef()->light;

			// add "fake constrast" for axis-aligned walls
			if (ld->IsVertical())
				light += 16;
			else if (ld->IsHorizontal())
				light -= 16;

			LightClippedQuad(x1, y1, z1, x2, y2, z2, tx1, ty1, tx2, ty2,
							 r / 255.0, g / 255.0, b / 255.0, light);
		}
		else
		{
			glColor3f(r / 255.0, g / 255.0, b / 255.0);

			glBegin(GL_QUADS);

			glTexCoord2f(tx1, ty1); glVertex3f(x1, y1, z1);
			glTexCoord2f(tx1, ty2); glVertex3f(x1, y1, z2);
			glTexCoord2f(tx2, ty2); glVertex3f(x2, y2, z2);
			glTexCoord2f(tx2, ty1); glVertex3f(x2, y2, z1);

			glEnd();
		}
	}

	void DrawMidMasker(const LineDef *ld, const SideDef *sd,
		const Sector *front, const Sector *back,
		bool sky_upper, float ld_length,
		float x1, float y1, float x2, float y2)
	{
		byte r, g, b;
		bool fullbright;
		Img_c *img;

		img = FindTexture(sd->MidTex(), r, g, b, fullbright);
		if (img == NULL)
			return;

		float img_w  = img->width();
		float img_tw = RoundPOW2(img_w);

		float img_h  = img->height();
		float img_th = RoundPOW2(img_h);

		// compute Z coords and texture coords
		float z1 = MAX(front->floorh, back->floorh);
		float z2 = MIN(front->ceilh,  back->ceilh);

		if (z2 <= z1)
			return;

		float tx1 = 0.0;
		float tx2 = tx1 + ld_length;

		tx1 = (tx1 + sd->x_offset) / img_tw;
		tx2 = (tx2 + sd->x_offset) / img_tw;

		float ty1 = 0.0;
		float ty2 = img_h / img_th;

		if (ld->flags & MLF_LowerUnpegged)
		{
			z1 = z1 + sd->y_offset;
			z2 = z1 + img_h;
		}
		else
		{
			z2 = z2 + sd->y_offset;
			z1 = z2 - img_h;
		}

		glEnable(GL_ALPHA_TEST);

		if (r_view.lighting && !fullbright)
		{
			int light = sd->SecRef()->light;

			// add "fake constrast" for axis-aligned walls
			if (ld->IsVertical())
				light += 16;
			else if (ld->IsHorizontal())
				light -= 16;

			LightClippedQuad(x1, y1, z1, x2, y2, z2, tx1, ty1, tx2, ty2,
							 r / 255.0, g / 255.0, b / 255.0, light);
		}
		else
		{
			glColor3f(r / 255.0, g / 255.0, b / 255.0);

			glBegin(GL_QUADS);

			glTexCoord2f(tx1, ty1); glVertex3f(x1, y1, z1);
			glTexCoord2f(tx1, ty2); glVertex3f(x1, y1, z2);
			glTexCoord2f(tx2, ty2); glVertex3f(x2, y2, z2);
			glTexCoord2f(tx2, ty1); glVertex3f(x2, y2, z1);

			glEnd();
		}
	}

	void DrawLine(int ld_index)
	{
		const LineDef *ld = LineDefs[ld_index];

		if (! is_vertex(ld->start) || ! is_vertex(ld->end))
			return;

		if (! ld->Right())
			return;

		float x1 = ld->Start()->x() - r_view.x;
		float y1 = ld->Start()->y() - r_view.y;
		float x2 = ld->End()->x() - r_view.x;
		float y2 = ld->End()->y() - r_view.y;

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
		const SideDef *sd = (side == SIDE_LEFT) ? ld->Left() : ld->Right();

		if (! sd)
			return;

		if (side == SIDE_LEFT)
		{
			float tmp = angle1;
			angle1 = angle2;
			angle2 = tmp;
		}

		// clip angles to view volume

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
		int sx2 = AngleToX(angle2);

		if (sx1 > sx2)
			return;

		// compute distance from eye to wall
		float wdx = x2 - x1;
		float wdy = y2 - y1;

		float wlen = sqrt(wdx * wdx + wdy * wdy);
		float dist = fabs((y1 * wdx / wlen) - (x1 * wdy / wlen));

		if (dist < 0.01)
			return;

		bool self_ref = false;
		if (ld->Left() && ld->Right() && ld->Left()->sector == ld->Right()->sector)
			self_ref = true;

		// mark sectors to be drawn
		// [ this method means we don't need to check visibility of sectors ]
		if (! self_ref)
		{
			if (ld->Left() && is_sector(ld->Left()->sector))
				seen_sectors.set(ld->Left()->sector);

			if (ld->Right() && is_sector(ld->Right()->sector))
				seen_sectors.set(ld->Right()->sector);
		}

		/* actually draw it... */

		x1 = ld->Start()->x();
		y1 = ld->Start()->y();
		x2 = ld->End()->x();
		y2 = ld->End()->y();

		if (side == SIDE_LEFT)
		{
			std::swap(x1, x2);
			std::swap(y1, y2);
		}

		float ld_len = hypotf(x2 - x1, y2 - y1);

		const Sector *front = sd ? sd->SecRef() : NULL;

		if (ld->OneSided())
		{
			DrawSide('W', ld, sd, sd->MidTex(), front, NULL, false,
				ld_len, x1, y1, front->floorh, x2, y2, front->ceilh);
		}
		else
		{
			const SideDef *sd_back = (side == SIDE_LEFT) ? ld->Right() : ld->Left();
			const Sector *back  = sd_back ? sd_back->SecRef() : NULL;

			bool sky_upper = is_sky(front->CeilTex()) && is_sky(back->CeilTex());

			if (back->floorh > front->floorh && !self_ref)
				DrawSide('L', ld, sd, sd->LowerTex(), front, back, sky_upper,
					ld_len, x1, y1, front->floorh, x2, y2, back->floorh);

			if (back->ceilh < front->ceilh && !self_ref)
				DrawSide('U', ld, sd, sd->UpperTex(), front, back, sky_upper,
					ld_len, x1, y1, back->ceilh, x2, y2, front->ceilh);

			if (!is_null_tex(sd->MidTex()) && r_view.texturing)
				DrawMidMasker(ld, sd, front, back, sky_upper,
					ld_len, x1, y1, x2, y2);
		}
	}

	void DrawSector(int sec_index)
	{
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

		// project sprite to check if it is off-screen

		float x = th->x() - r_view.x;
		float y = th->y() - r_view.y;

		float tx = x * r_view.Sin - y * r_view.Cos;
		float ty = x * r_view.Cos + y * r_view.Sin;

		// sprite is complete behind viewplane?
		if (ty < 4)
			return;

		bool fullbright = false;
		if (info->flags & THINGDEF_LIT)
			fullbright = true;

		float scale = info->scale;

		Img_c *img = W_GetSprite(th->type);
		if (! img)
		{
			img = IM_UnknownSprite();
			fullbright = true;
		}

		float scale_w = img->width() * scale;
		float scale_h = img->height() * scale;

		float tx1 = tx - scale_w * 0.5;
		float tx2 = tx + scale_w * 0.5;
		float ty1, ty2;

		double iz = 1 / ty;

		int sx1 = DeltaToX(iz, tx1);
		int sx2 = DeltaToX(iz, tx2);

		if (sx2 < 0 || sx1 > r_view.screen_w)
			return;

		// sprite is potentially visible, so draw it

		// choose X/Y coordinates so quad faces the camera
		float x1 = th->x() - r_view.Sin * scale_w * 0.5;
		float y1 = th->y() + r_view.Cos * scale_w * 0.5;

		float x2 = th->x() + r_view.Sin * scale_w * 0.5;
		float y2 = th->y() - r_view.Cos * scale_w * 0.5;

		int sec_num = r_view.thing_sectors[th_index];

		float z1, z2;

		if (info->flags & THINGDEF_CEIL)
		{
			// IOANCH 9/2015: add thing z (for Hexen format)
			z2 = (is_sector(sec_num) ? Sectors[sec_num]->ceilh : 192) - th->h();
			z1 = z2 - scale_h;
		}
		else
		{
			z1 = (is_sector(sec_num) ? Sectors[sec_num]->floorh : 0) + th->h();
			z2 = z1 + scale_h;
		}

		// bind the sprite image (upload it to OpenGL if needed)
		img->bind_gl();

		// choose texture coords based on image size
		tx1 = 0.0;
		ty1 = 0.0;
		tx2 = (float)img->width()  / (float)RoundPOW2(img->width());
		ty2 = (float)img->height() / (float)RoundPOW2(img->height());

		// lighting
		float L = 1.0;

		if (r_view.lighting && !fullbright)
		{
			int light = is_sector(sec_num) ? Sectors[sec_num]->light : 255;

			L = DoomLightToFloat(light, ty /* dist */);
		}

		glColor3f(L, L, L);

		glBegin(GL_QUADS);

		glTexCoord2f(tx1, ty1); glVertex3f(x1, y1, z1);
		glTexCoord2f(tx1, ty2); glVertex3f(x1, y1, z2);
		glTexCoord2f(tx2, ty2); glVertex3f(x2, y2, z2);
		glTexCoord2f(tx2, ty1); glVertex3f(x2, y2, z1);

		glEnd();
	}

	void HighlightLine(int part, int ld, int side)
	{
		const LineDef *L = LineDefs[ld];

		const SideDef *sd_front = L->Right();
		const SideDef *sd_back  = L->Left();

		if (side == SIDE_LEFT)
		{
			std::swap(sd_front, sd_back);
		}

		if (sd_front == NULL)
			return;

		const Sector *front = sd_front->SecRef();
		const Sector *back  = sd_back ? sd_back->SecRef() : NULL;

		float x1 = L->Start()->x();
		float y1 = L->Start()->y();
		float x2 = L->End()->x();
		float y2 = L->End()->y();

		float z1 = front->floorh;
		float z2 = front->ceilh;

		if (L->TwoSided())
		{
			if (part == PART_RT_LOWER)
			{
				z1 = MIN(front->floorh, back->floorh);
				z2 = MAX(front->floorh, back->floorh);
			}
			else  /* part == PART_RT_UPPER */
			{
				z1 = MIN(front->ceilh, back->ceilh);
				z2 = MAX(front->ceilh, back->ceilh);
			}
		}

		glBegin(GL_LINE_LOOP);

		glVertex3f(x1, y1, z1);
		glVertex3f(x1, y1, z2);
		glVertex3f(x2, y2, z2);
		glVertex3f(x2, y2, z1);

		glEnd();
	}

	void HighlightSector(int part, int sec_num)
	{
		const Sector *sec = Sectors[sec_num];

		float z = (part == PART_CEIL) ? sec->ceilh : sec->floorh;

		for (int n = 0 ; n < NumLineDefs ; n++)
		{
			const LineDef *L = LineDefs[n];

			if (L->TouchesSector(sec_num))
			{
				float x1 = L->Start()->x();
				float y1 = L->Start()->y();
				float x2 = L->End()->x();
				float y2 = L->End()->y();

				glBegin(GL_LINE_STRIP);
				glVertex3f(x1, y1, z);
				glVertex3f(x2, y2, z);
				glEnd();
			}
		}
	}

	void HighlightThing(int th_index)
	{
		Thing *th = Things[th_index];

		const thingtype_t *info = M_GetThingType(th->type);

		float scale = info->scale;

		Img_c *img = W_GetSprite(th->type);
		if (! img)
			img = IM_UnknownSprite();

		float scale_w = img->width() * scale;
		float scale_h = img->height() * scale;

		// choose X/Y coordinates so quad faces the camera
		float x1 = th->x() - r_view.Sin * scale_w * 0.5;
		float y1 = th->y() + r_view.Cos * scale_w * 0.5;
		float x2 = th->x() + r_view.Sin * scale_w * 0.5;
		float y2 = th->y() - r_view.Cos * scale_w * 0.5;

		int sec_num = r_view.thing_sectors[th_index];

		float h1, h2;

		if (info->flags & THINGDEF_CEIL)
		{
			// IOANCH 9/2015: add thing z (for Hexen format)
			h2 = (is_sector(sec_num) ? Sectors[sec_num]->ceilh : 192) - th->h();
			h1 = h2 - scale_h;
		}
		else
		{
			h1 = (is_sector(sec_num) ? Sectors[sec_num]->floorh : 0) + th->h();
			h2 = h1 + scale_h;
		}

		glBegin(GL_LINE_LOOP);

		glVertex3f(x1, y1, h1);
		glVertex3f(x1, y1, h2);
		glVertex3f(x2, y2, h2);
		glVertex3f(x2, y2, h1);

		glEnd();
	}

	inline void HighlightObject(Objid& obj)
	{
		if (!obj.valid())
			return;

		if (obj.type == OBJ_THINGS)
		{
			HighlightThing(obj.num);
		}
		else if (obj.type == OBJ_SECTORS)
		{
			HighlightSector(PART_FLOOR, obj.num);
			HighlightSector(PART_CEIL,  obj.num);
		}
		else if (obj.type == OBJ_LINEDEFS)
		{
			HighlightLine(PART_RT_LOWER, obj.num, 0 /* side */);
			HighlightLine(PART_RT_LOWER, obj.num, 1 /* side */);
		}
	}

	void Highlight()
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);

		glLineWidth(2);

		/* do the selection */

#if 0  // FIXME !!!
		bool saw_hl = false;

		for (unsigned int k = 0 ; k < r_view.sel.size() ; k++)
		{
			if (! r_view.sel[k].valid())
				continue;

			gl_color(SEL_COL);

			if (r_view.hl.valid() && r_view.hl == r_view.sel[k])
			{
				gl_color(HI_AND_SEL_COL);
				saw_hl = true;
			}

			HighlightObject(r_view.sel[k]);
		}

		/* do the highlight */

		if (! saw_hl)
		{
			gl_color(HI_COL);

			HighlightObject(r_view.hl);
		}
#endif

		glLineWidth(1);
	}

	void Render()
	{
		r_view.SaveOffsets();

		for (int i=0 ; i < NumLineDefs ; i++)
			DrawLine(i);

		glDisable(GL_ALPHA_TEST);

		for (int s=0 ; s < NumSectors ; s++)
			if (seen_sectors.get(s))
				DrawSector(s);

		glEnable(GL_ALPHA_TEST);

		if (r_view.sprites)
			for (int t=0 ; t < NumThings ; t++)
				DrawThing(t);

		r_view.RestoreOffsets();
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
		float z_near = x_slope;
		float z_far  = 32767.0;

		glLoadIdentity();
		glFrustum(-x_slope, +x_slope, -y_slope, +y_slope, z_near, z_far);
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
	rend.Highlight();
	rend.Finish();
}

#endif /* NO_OPENGL */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
