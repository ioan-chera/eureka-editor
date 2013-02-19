//------------------------------------------------------------------------
//  3D RENDERING 
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2013 Andrew Apted
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

#include "main.h"

#include <map>
#include <algorithm>

#include "levels.h"
#include "im_color.h"
#include "im_img.h"
#include "e_things.h"
#include "editloop.h"
#include "m_game.h"
#include "objects.h"
#include "x_hover.h"
#include "w_loadpic.h"
#include "w_rawdef.h"

#include "r_render.h"

#include "w_flats.h"
#include "w_sprite.h"
#include "w_texture.h"

#include "editloop.h"


#define REND_SEL_COL  196


struct Y_View
{
public:
   // player type and position.
   int p_type, px, py;

   // view position.
   float x, y; 
   int z;

   // standard height above the floor.
#define EYE_HEIGHT  41

   // view direction.  angle is in radians
   float angle;
   float Sin, Cos;

   // screen image.
   int sw, sh;
   byte *screen;

   float aspect_sh;
   float aspect_sw;  // sw * aspect_ratio

   bool texturing;
   bool sprites;
   bool lighting;
   bool low_detail;

   bool gravity;  // when true, walk on ground

   std::vector<int> thing_sectors;
   int thsec_sector_num;
   bool thsec_invalidated;

public:
	Y_View() : p_type(0), screen(NULL),
			   texturing(false), sprites(false), lighting(false),
			   low_detail(false), gravity(true),
	           thing_sectors(),
			   thsec_sector_num(0), thsec_invalidated(false)
	{ }

	void SetAngle(float new_ang)
	{
		angle = new_ang;

		if (angle >= 2*M_PI)
			angle -= 2*M_PI;
		else if (angle < 0)
			angle += 2*M_PI;

		Sin = sin(angle);
		Cos = cos(angle);
	}

	void CalcViewZ()
	{
		Objid o;
		GetCurObject(o, OBJ_SECTORS, int(x), int(y));

		int secnum = o.num;

		if (secnum >= 0)
			z = Sectors[secnum]->floorh + EYE_HEIGHT;
	}

	void CalcAspect()
	{
		float monitor_aspect = 4.0 / 3.0;  // FIXME: CONFIG THIS UP

		float screen_aspect = float(sw) / float(sh);

		aspect_sh = sh / monitor_aspect * screen_aspect;
		aspect_sw = sw;
	}

	void ClearScreen()
	{
		// color #0 is black (DOOM, Heretic, Hexen)
		memset(screen, 0, sw * sh);
	}

	void FindThingSectors()
	{
		thing_sectors.resize(NumThings);

		for (int i = 0 ; i < NumThings ; i++)
		{
			Objid obj;

			GetCurObject(obj, OBJ_SECTORS, Things[i]->x, Things[i]->y);

			thing_sectors[i] = obj.num;
		}

		thsec_sector_num  = NumSectors;
		thsec_invalidated = false;
	}

	inline int R_DoomLightingEquation(int L, float dist)
	{
		/* L in the range 0 to 63 */
		int min_L = CLAMP(0, 36 - L, 31);

		int index = (59 - L) - int(1280 / MAX(1, dist));

		/* result is colormap index (0 bright .. 31 dark) */
		return CLAMP(min_L, index, 31);
	}
	
	byte DoomLightRemap(int light, float dist, byte pixel)
	{
		int map = R_DoomLightingEquation(light >> 2, dist);

		return raw_colormap[map][pixel];
	}
};


static Y_View view;


struct DrawSurf
{
public:
	enum
	{
		K_INVIS = 0,
		K_FLAT,
		K_TEXTURE
	};
	int kind;  

	// heights for the surface (h1 is below h2)
	int h1, h2, tex_h;

	Img *img;
	img_pixel_t col;  /* used if img is zero */

	enum
	{
		SOLID_ABOVE = 1,
		SOLID_BELOW = 2
	};
	int y_clip;

	// highlight the surface?
	img_pixel_t border_col;

public:
	DrawSurf() : kind(K_INVIS), img(NULL), border_col(0)
	{ }

	~DrawSurf()
	{ }

	void FindFlat(const char * fname, Sector *sec)
	{
		if (view.texturing)
		{
			img = W_GetFlat(fname);

			if (img)
				return;
		}

		col = 0x70 + ((fname[0]*13+fname[1]*41+fname[2]*11) % 48);
	}

	void FindTex(const char * tname, LineDef *ld)
	{
		if (view.texturing)
		{
			img = W_GetTexture(tname);

			if (img)
				return;
		}

		col = 0x30 + ((tname[0]*17+tname[1]*47+tname[2]*7) % 64);

		if (col >= 0x60)
			col += 0x70;
	}
};


struct DrawWall
{
public:
	typedef std::vector<struct DrawWall *> vec_t;

	// when 'th' is >= 0, this is actually a sprite, and 'ld' and
	// 'sd' will be NULL.  Sprites use the info in the 'ceil' surface.
	int th;

	LineDef *ld;
	SideDef *sd;
	Sector *sec;

	// which side this wall faces (0 right, 1 left)
	// for sprites: a copy of the thinginfo flags
	int side;

	// lighting for wall, adjusted for N/S and E/W walls
	int wall_light;

	// clipped angles
	float ang1, dang, cur_ang;
	float base_ang;

	// line constants
	float dist, t_dist;
	float normal;

	// distance values (inverted, so they can be lerped)
	double iz1, diz, cur_iz; 
	double mid_iz;

	// translate coord, for sprite
	float spr_tx1;

	// screen X coordinates
	int sx1, sx2;

	// for sprites, the remembered open space to clip to
	int oy1, oy2;

	/* surfaces */

	DrawSurf ceil;
	DrawSurf upper;
	DrawSurf lower;
	DrawSurf floor;
	DrawSurf rail;

#define IZ_EPSILON  1e-6

   /* PREDICATES */

	struct MidDistCmp
	{
		inline bool operator() (const DrawWall * A, const DrawWall * B) const
		{
			return A->mid_iz > B->mid_iz;
		}
	};

	struct DistCmp
	{
		inline bool operator() (const DrawWall * A, const DrawWall * B) const
		{
			if (fabs(A->cur_iz - B->cur_iz) >= IZ_EPSILON)
			{
				// this is the normal case
				return A->cur_iz > B->cur_iz;
			}

			// this case usually occurs at a column where two walls share a vertex.
			// 
			// hence we check if they actually share a vertex, and if so then
			// we test whether A is behind B or not -- by checking which side
			// of B the camera and the other vertex of A are on.

			if (A->ld && B->ld)
			{
				// find the vertex of A _not_ shared with B
				int A_other = -1;

				if (B->ld->TouchesVertex(A->ld->start)) A_other = A->ld->end;
				if (B->ld->TouchesVertex(A->ld->end))   A_other = A->ld->start;

				if (A_other >= 0)
				{
					int ax = Vertices[A_other]->x;
					int ay = Vertices[A_other]->y;

					int bx1 = B->ld->Start()->x;
					int by1 = B->ld->Start()->y;
					int bx2 = B->ld->End()->x;
					int by2 = B->ld->End()->y;

					int cx = (int)view.x;  // camera
					int cy = (int)view.y;

					int A_side = PointOnLineSide(ax, ay, bx1, by1, bx2, by2);
					int C_side = PointOnLineSide(cx, cy, bx1, by1, bx2, by2);

					return (A_side * C_side >= 0);
				}
			}

			// a pretty good fallback:
			return A->mid_iz > B->mid_iz;
		}
	};

	struct SX1Cmp
	{
		inline bool operator() (const DrawWall * A, const DrawWall * B) const
		{
			return A->sx1 < B->sx1;
		}

		inline bool operator() (const DrawWall * A, int x) const
		{
			return A->sx1 < x;
		}

		inline bool operator() (int x, const DrawWall * A) const
		{
			return x < A->sx1;
		}
	};

	struct SX2Less
	{
		int x;

		SX2Less(int _x) : x(_x) { }

		inline bool operator() (const DrawWall * A) const
		{
			return A->sx2 < x;
		}
	};

	/* methods */

	void ComputeWallSurface()
	{
		Sector *front = sec;
		Sector *back  = NULL;

		SideDef *back_sd = side ? ld->Right() : ld->Left();
		if (back_sd)
			back = Sectors[back_sd->sector];

		bool sky_upper = back && is_sky(front->CeilTex()) && is_sky(back->CeilTex());
		bool self_ref  = (front == back) ? true : false;

		if ((front->ceilh > view.z || is_sky(front->CeilTex()))
		    && ! sky_upper && ! self_ref) 
		{
			ceil.kind = DrawSurf::K_FLAT;
			ceil.h1 = front->ceilh;
			ceil.h2 = +99999;
			ceil.tex_h = ceil.h1;
			ceil.y_clip = DrawSurf::SOLID_ABOVE;

			if (is_sky(front->CeilTex()))
				ceil.col = g_sky_color;
			else
				ceil.FindFlat(front->CeilTex(), front);
		}

		if (front->floorh < view.z && ! self_ref)
		{
			floor.kind = DrawSurf::K_FLAT;
			floor.h1 = -99999;
			floor.h2 = front->floorh;
			floor.tex_h = floor.h2;
			floor.y_clip = DrawSurf::SOLID_BELOW;

			if (is_sky(front->FloorTex()))
				floor.col = g_sky_color;
			else
				floor.FindFlat(front->FloorTex(), front);
		}

		if (! back)
		{
			/* ONE-sided line */

			lower.kind = DrawSurf::K_TEXTURE;
			lower.h1 = front->floorh;
			lower.h2 = front->ceilh;
			lower.y_clip = DrawSurf::SOLID_ABOVE | DrawSurf::SOLID_BELOW;

			lower.FindTex(sd->MidTex(), ld);

			if (lower.img && (ld->flags & MLF_LowerUnpegged))
				lower.tex_h = lower.h1 + lower.img->height();
			else
				lower.tex_h = lower.h2;

			lower.tex_h += sd->y_offset;
			return;
		}

		/* TWO-sided line */

		if (back->ceilh < front->ceilh && ! sky_upper && ! self_ref)
		{
			upper.kind = DrawSurf::K_TEXTURE;
			upper.h1 = back->ceilh;
			upper.h2 = front->ceilh;
			upper.tex_h = upper.h2;
			upper.y_clip = DrawSurf::SOLID_ABOVE;

			upper.FindTex(sd->UpperTex(), ld);

			if (upper.img && ! (ld->flags & MLF_UpperUnpegged))
				upper.tex_h = upper.h1 + upper.img->height();
			else
				upper.tex_h = upper.h2;

			upper.tex_h += sd->y_offset;
		}

		if (back->floorh > front->floorh && ! self_ref)
		{
			lower.kind = DrawSurf::K_TEXTURE;
			lower.h1 = front->floorh;
			lower.h2 = back->floorh;
			lower.y_clip = DrawSurf::SOLID_BELOW;

			lower.FindTex(sd->LowerTex(), ld);

			if (ld->flags & MLF_LowerUnpegged)
				lower.tex_h = front->ceilh;
			else
				lower.tex_h = lower.h2;

			lower.tex_h += sd->y_offset;
		}

		/* Mid-Masked texture */

		if (! view.texturing)
			return;

		if (! isalnum(sd->MidTex()[0]))
			return;

		rail.FindTex(sd->MidTex(), ld);
		if (! rail.img)
			return;

		int c_h = MIN(front->ceilh,  back->ceilh);
		int f_h = MAX(front->floorh, back->floorh);
		int r_h = rail.img->height();

		if (f_h >= c_h)
			return;

		if (ld->flags & MLF_LowerUnpegged)
		{
			rail.h1 = f_h + sd->y_offset;
			rail.h2 = rail.h1 + r_h;
		}
		else
		{
			rail.h2 = c_h + sd->y_offset;
			rail.h1 = rail.h2 - r_h;
		}

		rail.kind = DrawSurf::K_TEXTURE;
		rail.y_clip = 0;
		rail.tex_h = rail.h2;
	}
};


struct RendInfo
{
public:
	// complete set of walls/sprites to draw.
	DrawWall::vec_t walls;

	// the active list.  Pointers here are always duplicates of ones in
	// the walls list (no need to 'delete' any of them).
	DrawWall::vec_t active;

	// inverse distances over X range, 0 when empty.
	std::vector<double> depth_x;  

	int open_y1;
	int open_y2;

#define Y_SLOPE  1.70

public:
	static void DeleteWall(DrawWall *P)
	{
		delete P;
	}

	~RendInfo()
	{
		std::for_each(walls.begin(), walls.end(), DeleteWall);

		walls.clear ();
		active.clear ();
	}

	void InitDepthBuf (int width)
	{
		depth_x.resize(width);

		std::fill_n(depth_x.begin(), width, 0);
	}

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

		int x = int(view.aspect_sw * t);

		x = (view.sw + x) / 2;

		if (x < 0)
			x = 0;
		else if (x > view.sw)
			x = view.sw;

		return x;
	}

	static inline float XToAngle(int x)
	{
		x = x * 2 - view.sw;

		float ang = M_PI/2 + atan(x / view.aspect_sw);

		if (ang < 0)
			ang = 0;
		else if (ang > M_PI)
			ang = M_PI;

		return ang;
	}

	static inline int DeltaToX(double iz, float tx)
	{
		int x = int(view.aspect_sw * tx * iz);

		x = (x + view.sw) / 2;

		return x;
	}

	static inline float XToDelta(int x, double iz)
	{
		x = x * 2 - view.sw;

		float tx = x / iz / view.aspect_sw;

		return tx;
	}

	static inline int DistToY(double iz, int sec_h)
	{
		if (sec_h > 32770)
			return -9999;

		if (sec_h < -32770)
			return +9999;

		sec_h -= view.z;

		int y = int(view.aspect_sh * sec_h * iz * Y_SLOPE);

		y = (view.sh - y) / 2;

		return y;
	}

	static inline float YToDist(int y, int sec_h)
	{
		sec_h -= view.z;

		y = view.sh - y * 2;

		if (y == 0)
			return 999999;

		return view.aspect_sh * sec_h * Y_SLOPE / y;
	}

	static inline float YToSecH(int y, double iz)
	{
		y = y * 2 - view.sh;

		return view.z - (float(y) / view.aspect_sh / iz / Y_SLOPE);
	}

	void AddLine(int ld_index)
	{
		LineDef *ld = LineDefs[ld_index];

		if (! is_obj(ld->start) || ! is_obj(ld->end))
			return;

		float x1 = ld->Start()->x - view.x;
		float y1 = ld->Start()->y - view.y;
		float x2 = ld->End()->x - view.x;
		float y2 = ld->End()->y - view.y;

		float tx1 = x1 * view.Sin - y1 * view.Cos;
		float ty1 = x1 * view.Cos + y1 * view.Sin;
		float tx2 = x2 * view.Sin - y2 * view.Cos;
		float ty2 = x2 * view.Cos + y2 * view.Sin;

		// reject line if complete behind viewplane
		if (ty1 <= 0 && ty2 <= 0)
			return;

		float angle1 = PointToAngle(tx1, ty1);
		float angle2 = PointToAngle(tx2, ty2);
		float span = angle1 - angle2;

		if (span < 0)
			span += 2*M_PI;

		int side = 0;

		if (span >= M_PI)
			side = 1;

		// ignore the line when there is no facing sidedef
		SideDef *sd = side ? ld->Left() : ld->Right();

		if (! sd)
			return;

		if (side == 1)
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

		// compute normal of wall (translated coords)
		float normal;

		if (side == 1)
			normal = PointToAngle(ty2 - ty1, tx1 - tx2);
		else
			normal = PointToAngle(ty1 - ty2, tx2 - tx1);

		// compute inverse distances
		double iz1 = cos(normal - angle1) / dist / cos(M_PI/2 - angle1);
		double iz2 = cos(normal - angle2) / dist / cos(M_PI/2 - angle2);

		double diz = (iz2 - iz1) / MAX(1, sx2 - sx1);

		// create drawwall structure

		DrawWall *dw = new DrawWall;

		dw->th = -1;
		dw->ld = ld;
		dw->sd = sd;
		dw->sec = sd->SecRef();

		dw->side = side;

		dw->wall_light = dw->sec->light;

		if (ld->Start()->x == ld->End()->x)
			dw->wall_light += 16;
		else if (ld->Start()->y == ld->End()->y)
			dw->wall_light -= 16;

		dw->base_ang = base_ang;
		dw->ang1 = angle1;
		dw->dang = (angle2 - angle1) / MAX(1, sx2 - sx1);

		dw->dist = dist;
		dw->normal = normal;
		dw->t_dist = tan(base_ang - normal) * dist;

		dw->iz1 = iz1;
		dw->diz = diz;
		dw->mid_iz = iz1 + (sx2 - sx1 + 1) * diz / 2;

		dw->sx1 = sx1;
		dw->sx2 = sx2;

		walls.push_back(dw);
	}

	void AddThing(int th_index)
	{
		Thing *th = Things[th_index];

		const thingtype_t *info = M_GetThingType(th->type);

		float x = th->x - view.x;
		float y = th->y - view.y;

		float tx = x * view.Sin - y * view.Cos;
		float ty = x * view.Cos + y * view.Sin;

		// reject sprite if complete behind viewplane
		if (ty < 4)
			return;

		Img *sprite = W_GetSprite(th->type);
		if (! sprite)  // TODO: show a question mark (same color as on 2D map)
			return;

		float tx1 = tx - sprite->width() / 2.0;
		float tx2 = tx + sprite->width() / 2.0;

		double iz = 1 / ty;

		int sx1 = DeltaToX(iz, tx1);
		int sx2 = DeltaToX(iz, tx2) - 1;

		if (sx1 < 0)
			sx1 = 0;

		if (sx2 >= view.sw)
			sx2 = view.sw - 1;

		if (sx1 > sx2)
			return;

		int thsec = view.thing_sectors[th_index];
		
		int h1, h2;

		if (info && (info->flags & THINGDEF_CEIL))
		{
			h2 = is_sector(thsec) ? Sectors[thsec]->ceilh : 192;
			h1 = h2 - sprite->height();
		}
		else
		{
			h1 = is_sector(thsec) ? Sectors[thsec]->floorh : 0;
			h2 = h1 + sprite->height();
		}

		// create drawwall structure

		DrawWall *dw = new DrawWall;

		dw->th  = th_index;
		dw->ld  = NULL;
		dw->sd  = NULL;
		dw->sec = NULL;

		dw->side = info ? info->flags : 0;

		dw->spr_tx1 = tx1;

		dw->ang1 = dw->dang = 0;

		dw->iz1 = dw->mid_iz = iz;
		dw->diz = 0;

		dw->sx1 = sx1;
		dw->sx2 = sx2;

		dw->ceil.img = sprite;
		dw->ceil.h1  = h1;
		dw->ceil.h2  = h2;

		walls.push_back(dw);
	}

	void ComputeSurfaces()
	{
		DrawWall::vec_t::iterator S;

		for (S = walls.begin() ; S != walls.end() ; S++)
			if ((*S)->ld)
				(*S)->ComputeWallSurface();
	}

	void ClipSolids()
	{
		// perform a rough depth sort of the walls and sprites.

		std::sort(walls.begin(), walls.end(), DrawWall::MidDistCmp());

		// go forwards, from closest to furthest away

		DrawWall::vec_t::iterator S;

		for (S = walls.begin() ; S != walls.end() ; S++)
		{
			DrawWall *dw = (*S);

			if (! dw)
				continue;

			int one_sided = dw->ld && ! is_obj(dw->ld->left);
			int vis_count = dw->sx2 - dw->sx1 + 1;

			for (int x = dw->sx1 ; x <= dw->sx2 ; x++)
			{
				double iz = dw->iz1 + (dw->diz * (x - dw->sx1));

				if (iz < depth_x[x])
					vis_count--;
				else if (one_sided)
					depth_x[x] = iz;
			}

			if (vis_count == 0)
			{
				delete dw;
				(*S) = NULL;
			}
		}

		// remove null pointers

		S = std::remove(walls.begin(), walls.end(), (DrawWall *) NULL);

		walls.erase(S, walls.end());
	}

	void RenderFlatColumn(DrawWall *dw, DrawSurf& surf,
			int x, int y1, int y2)
	{
		img_pixel_t *buf = view.screen;
		img_pixel_t *wbuf = surf.img->wbuf ();

		int tw = surf.img->width();
		int th = surf.img->height();

		float ang = XToAngle(x);
		float modv = cos(ang - M_PI/2);

		float t_cos = cos(M_PI + -view.angle + ang) / modv;
		float t_sin = sin(M_PI + -view.angle + ang) / modv;

		buf += x + y1 * view.sw;

		int light = dw->sec->light;

		for ( ; y1 <= y2 ; y1++, buf += view.sw)
		{
			float dist = YToDist(y1, surf.tex_h);

			int tx = int( view.x - t_sin * dist) & (tw - 1);
			int ty = int(-view.y + t_cos * dist) & (th - 1);

			*buf = wbuf[ty * tw + tx];
			
			if (view.lighting)
				*buf = view.DoomLightRemap(light, dist, *buf);
		}
	}

	void RenderTexColumn(DrawWall *dw, DrawSurf& surf,
			int x, int y1, int y2)
	{
		img_pixel_t *buf = view.screen;
		img_pixel_t *wbuf = surf.img->wbuf ();

		int tw = surf.img->width();
		int th = surf.img->height();

		int  light = dw->wall_light;
		float dist = 1.0 / dw->cur_iz;

		/* compute texture X coord */

		int tx = int(dw->t_dist - tan(dw->cur_ang - dw->normal) * dw->dist);

		tx = (dw->sd->x_offset + tx) & (tw - 1);

		/* compute texture Y coords */

		float hh = surf.tex_h - YToSecH(y1, dw->cur_iz);
		float dh = surf.tex_h - YToSecH(y2, dw->cur_iz);

		dh = (dh - hh) / MAX(1, y2 - y1);

		buf  += x + y1 * view.sw;
		wbuf += tx;

		for ( ; y1 <= y2 ; y1++, hh += dh, buf += view.sw)
		{
			int ty = int(hh) % th;

			// handle negative values (use % twice)
			ty = (ty + th) % th;

			img_pixel_t pix = wbuf[ty * tw];

			if (pix == TRANS_PIXEL)
				continue;

			if (view.lighting)
				*buf = view.DoomLightRemap(light, dist, pix);
			else
				*buf = pix;
		}
	}

	void SolidFlatColumn(DrawWall *dw, DrawSurf& surf, int x, int y1, int y2)
	{
		img_pixel_t *buf = view.screen;

		buf += x + y1 * view.sw;

		int light = dw->sec->light;

		for ( ; y1 <= y2 ; y1++, buf += view.sw)
		{
			float dist = YToDist(y1, surf.tex_h);

			*buf = surf.col;

			if (view.lighting && surf.col != g_sky_color)
				*buf = view.DoomLightRemap(light, dist, 0x70);  //TODO: GAME CONFIG
		}
	}

	void SolidTexColumn(DrawWall *dw, DrawSurf& surf, int x, int y1, int y2)
	{
		int  light = dw->wall_light;
		float dist = 1.0 / dw->cur_iz;

		img_pixel_t *buf = view.screen;

		buf += x + y1 * view.sw;

		for ( ; y1 <= y2 ; y1++, buf += view.sw)
		{
			*buf = surf.col;

			if (view.lighting)
				*buf = view.DoomLightRemap(light, dist, 0x50); //TODO GAME CONFIG
		}
	}

	void HighlightColumn(int x, int y1, int y2, byte col)
	{
		img_pixel_t *buf = view.screen;

		buf += x + y1 * view.sw;

		for ( ; y1 <= y2 ; y1++, buf += view.sw)
			*buf = col;
	}


	inline void RenderWallSurface(DrawWall *dw, DrawSurf& surf, int x)
	{
		if (surf.kind == DrawSurf::K_INVIS)
			return;

		int y1 = DistToY(dw->cur_iz, surf.h2);
		int y2 = DistToY(dw->cur_iz, surf.h1) - 1;

		if (y1 < open_y1)
			y1 = open_y1;

		if (y2 > open_y2)
			y2 = open_y2;

		if (y1 > y2)
			return;

		/* clip the open region */

		if (surf.y_clip & DrawSurf::SOLID_ABOVE)
			if (open_y1 < y2)
				open_y1 = y2;

		if (surf.y_clip & DrawSurf::SOLID_BELOW)
			if (open_y2 > y1)
				open_y2 = y1;

		/* highlight the wall, floor or sprite */

		if (surf.border_col)
		{
			// FIXME: this logic doesn't work very well, it seems the
			//        right column is prone to being overdrawn, and also
			//        the top pixel for lower parts.

			if (surf.kind == DrawSurf::K_TEXTURE &&
  	  			((x == dw->sx1 && dw->sx1 > 0) ||
  	  			 (x == dw->sx2 && dw->sx2 < view.sw-1)))
			{
				HighlightColumn(x, y1, y2, surf.border_col);
				return;
			}

			if (y1 <= y2)
			{
				HighlightColumn(x, y1, y1, surf.border_col);
				y1++;
			}

			if (y1 <= y2)
			{
				HighlightColumn(x, y2, y2, surf.border_col);
				y2--;
			}
		}

		/* fill pixels */

		if (! surf.img)
		{
			if (surf.kind == DrawSurf::K_FLAT)
				SolidFlatColumn(dw, surf, x, y1, y2);
			else
				SolidTexColumn(dw, surf, x, y1, y2);
		}
		else switch (surf.kind)
		{
			case DrawSurf::K_FLAT:
				RenderFlatColumn(dw, surf, x, y1, y2);
				break;

			case DrawSurf::K_TEXTURE:
				RenderTexColumn(dw, surf, x, y1, y2);
				break;
		}
	}

	inline void RenderSprite(DrawWall *dw, int x)
	{
		int y1 = DistToY(dw->cur_iz, dw->ceil.h2);
		int y2 = DistToY(dw->cur_iz, dw->ceil.h1) - 1;

		if (y1 < dw->oy1)
			y1 = dw->oy1;

		if (y2 > dw->oy2)
			y2 = dw->oy2;

		if (y1 > y2)
			return;

		/* fill pixels */

		img_pixel_t *buf = view.screen;
		img_pixel_t *wbuf = dw->ceil.img->wbuf ();

		int tw = dw->ceil.img->width();
		int th = dw->ceil.img->height();

		int tx = int(XToDelta(x, dw->cur_iz) - dw->spr_tx1);

		if (tx < 0 || tx >= tw)
			return;

		float hh = dw->ceil.h2 - YToSecH(y1, dw->cur_iz);
		float dh = dw->ceil.h2 - YToSecH(y2, dw->cur_iz);

		dh = (dh - hh) / MAX(1, y2 - y1);

		buf  += x + y1 * view.sw;
		wbuf += tx;

		int thsec = view.thing_sectors[dw->th];
		int light = is_sector(thsec) ? Sectors[thsec]->light : 255;
		float dist = 1.0 / dw->cur_iz;

		for ( ; y1 <= y2 ; y1++, hh += dh, buf += view.sw)
		{
			int ty = int(hh);

			if (ty < 0 || ty >= th)
				continue;

			img_pixel_t pix = wbuf[ty * tw];

			if (pix == TRANS_PIXEL)
				continue;

			if (dw->side & THINGDEF_INVIS)
			{
				*buf = raw_colormap[14][*buf];
				continue;
			}
			
			*buf = pix;

			if (view.lighting && ! (dw->side & THINGDEF_LIT))
				*buf = view.DoomLightRemap(light, dist, *buf);
		}
	}

	inline void RenderMidMasker(DrawWall *dw, DrawSurf& surf, int x)
	{
		if (surf.kind == DrawSurf::K_INVIS)
			return;

		if (! surf.img)
			return;

		int y1 = DistToY(dw->cur_iz, surf.h2);
		int y2 = DistToY(dw->cur_iz, surf.h1) - 1;

		if (y1 < dw->oy1)
			y1 = dw->oy1;

		if (y2 > dw->oy2)
			y2 = dw->oy2;

		if (y1 > y2)
			return;

		/* fill pixels */

		RenderTexColumn(dw, surf, x, y1, y2);
	}

	void UpdateActiveList(int x)
	{
		DrawWall::vec_t::iterator S, E, P;

		bool changes = false;

		// remove walls that have finished.

		S = active.begin();
		E = active.end();

		S = std::remove_if (S, E, DrawWall::SX2Less(x));

		if (S != E)
		{
			active.erase(S, E);
			changes = true;
		}

		// add new walls that start in this column.

		S = walls.begin();
		E = walls.end();

		S = std::lower_bound(S, E, x, DrawWall::SX1Cmp());
		E = std::upper_bound(S, E, x, DrawWall::SX1Cmp());

		if (S != E)
			changes = true;

		for ( ; S != E ; S++)
		{
			active.push_back(*S);
		}

		// calculate new depth values

		S = active.begin();
		E = active.end();

		for (P=S ; (P != E) ; P++)
		{
			DrawWall *dw = (*P);

			dw->cur_iz = dw->iz1 + dw->diz * (x - dw->sx1);

			if (P != S && (*(P-1))->cur_iz < dw->cur_iz + IZ_EPSILON)
				changes = true;

			dw->cur_ang = dw->ang1 + dw->dang * (x - dw->sx1);
		}

		// if there are changes, re-sort the active list...

		if (changes)
		{
			std::sort(active.begin(), active.end(), DrawWall::DistCmp());
		}
	}

	void RenderWalls()
	{
		// sort walls by their starting column, to allow binary search.

		std::sort(walls.begin(), walls.end(), DrawWall::SX1Cmp());

		active.clear ();

		for (int x=0 ; x < view.sw ; x++)
		{
			// clear vertical depth buffer

			open_y1 = 0;
			open_y2 = view.sh - 1;

			UpdateActiveList(x);

			// render, front to back

			DrawWall::vec_t::iterator S, E, P;

			S = active.begin();
			E = active.end();

			for (P=S ; P != E ; P++)
			{
				DrawWall *dw = (*P);

				// for things, just remember the open space
				{
					dw->oy1 = open_y1;
					dw->oy2 = open_y2;
				}
				if (dw->th >= 0)
					continue;

				RenderWallSurface(dw, dw->ceil,  x);
				RenderWallSurface(dw, dw->floor, x);
				RenderWallSurface(dw, dw->upper, x);
				RenderWallSurface(dw, dw->lower, x);

				if (open_y1 >= open_y2)
					break;
			}

			// now render things, back to front
			// (mid-masked textures are done here too)

			if (P == E)
				P--;

			for ( ; P != (S-1) ; P--)
			{
				DrawWall *dw = (*P);

				if (dw->th >= 0)
					RenderSprite(dw, x);
				else
					RenderMidMasker(dw, dw->rail, x);
			}
		}
	}

	void DoRender3D()
	{
		view.ClearScreen();

		InitDepthBuf(view.sw);

		for (int i=0 ; i < NumLineDefs ; i++)
			AddLine(i);

		if (view.sprites)
			for (int j=0 ; j < NumThings ; j++)
				AddThing(j);

		ClipSolids();

		ComputeSurfaces();

		RenderWalls();
	}
};


static Thing *FindPlayer(int typenum)
{
	// need to search backwards (to handle Voodoo dolls properly)

	for (int i = NumThings-1 ; i >= 0 ; i--)
		if (Things[i]->type == typenum)
			return Things[i];

	return NULL;  // not found
}


//------------------------------------------------------------------------

static Thing *player;



#if 0  /// OLD
UI_RenderWin::UI_RenderWin(const char *title) :
    Fl_Window( 640, 400, title ),
    player(NULL)
{
  end(); // cancel begin() in Fl_Group constructor

  size_range(640, 400, 640, 400);

  color(FL_BLACK, FL_BLACK);
}

UI_RenderWin::~UI_RenderWin()
{
}

int UI_RenderWin::handle(int event)
{
  switch (event)
  {
    case FL_FOCUS:
      return 1;

    case FL_KEYDOWN:
    case FL_SHORTCUT:
    {
//      int result = handle_key();
//      handle_mouse();

      int key   = Fl::event_key();
      int state = Fl::event_state();

      switch (key)
      {
        case 0:
        case FL_Num_Lock:
        case FL_Caps_Lock:

        case FL_Shift_L: case FL_Control_L:
        case FL_Shift_R: case FL_Control_R:
        case FL_Meta_L:  case FL_Alt_L:
        case FL_Meta_R:  case FL_Alt_R:

          /* IGNORE */
          return 1;

        default:
          /* OK */
          break;
      }

      if (key < 127 && isalpha(key) && (state & FL_SHIFT))
        key = toupper(key);

      HandleKey(key);

      return 1;
    }

    case FL_ENTER:
      return 1;

    case FL_LEAVE:
      return 1;

    default: break;
  }

  return 0;  // unused
}
#endif


void Render3D_Setup()
{
	if (! view.p_type)
	{
		view.p_type = THING_PLAYER1;
		view.px = 99999;
	}

	player = FindPlayer(view.p_type);

	if (! player)
	{
		if (view.p_type != THING_DEATHMATCH)
			view.p_type = THING_DEATHMATCH;

		player = FindPlayer(view.p_type);
	}

	if (player && (view.px != player->x || view.py != player->y))
	{
		// if player moved, re-create view parameters

		view.x = view.px = player->x;
		view.y = view.py = player->y;

		view.CalcViewZ();
		view.SetAngle(player->angle * M_PI / 180.0);
	}
	else
	{
		view.x = 0;
		view.y = 0;
		view.z = 64;

		view.SetAngle(0);
	}

	/* create image */

	view.sw = -1;
	view.sh = -1;

	view.texturing  = true;   // TODO: CONFIG ITEMS
	view.sprites    = true;
	view.lighting   = true;
	view.low_detail = true;
}


void Render3D_BlitHires(int ox, int oy, int ow, int oh)
{
	for (int ry = 0 ; ry < view.sh ; ry++)
	{
		u8_t line_rgb[view.sw * 3];

		u8_t *dest = line_rgb;
		u8_t *dest_end = line_rgb + view.sw * 3;

		const byte *src = view.screen + ry * view.sw;

		for ( ; dest < dest_end  ; dest += 3, src++)
		{
			u32_t col = palette[*src];

			dest[0] = RGB_RED(col);
			dest[1] = RGB_GREEN(col);
			dest[2] = RGB_BLUE(col);
		}

		fl_draw_image(line_rgb, ox, oy+ry, view.sw, 1);
	}

}


void Render3D_BlitLores(int ox, int oy, int ow, int oh)
{
	for (int ry = 0 ; ry < view.sh ; ry++)
	{
		const byte *src = view.screen + ry * view.sw;

		// if destination width is odd, we store an extra pixel here
		u8_t line_rgb[(ow + 1) * 3];

		u8_t *dest = line_rgb;
		u8_t *dest_end = line_rgb + ow * 3;

		for (; dest < dest_end ; dest += 6, src++)
		{
			u32_t col = palette[*src];

			dest[0] = RGB_RED(col);
			dest[1] = RGB_GREEN(col);
			dest[2] = RGB_BLUE(col);

			dest[3] = dest[0];
			dest[4] = dest[1];
			dest[5] = dest[2];
		}

		fl_draw_image(line_rgb, ox, oy + ry*2, ow, 1);

		if (ry * 2 + 1 < oh)
		{
			fl_draw_image(line_rgb, ox, oy + ry*2 + 1, ow, 1);
		}
	}

}


/*
 *  Render a 3D view from the player's position. 
 */

void Render3D_Draw(int ox, int oy, int ow, int oh)
{
	if (view.thsec_invalidated || !view.screen ||
	    NumThings  != (int)view.thing_sectors.size() ||
		NumSectors != view.thsec_sector_num)
	{
		view.FindThingSectors();
	}

	// in low detail mode, setup size so that expansion always covers
	// our window (i.e. we draw a bit more than we need).

	int new_sw = view.low_detail ? (ow + 1) / 2 : ow;
	int new_sh = view.low_detail ? (oh + 1) / 2 : oh;

	if (!view.screen || view.sw != new_sw || view.sh != new_sh)
	{
		view.sw = new_sw;
		view.sh = new_sh;

		if (view.screen)
			delete[] view.screen;

		view.screen = new byte [view.sw * view.sh];

		view.CalcAspect();
	}

	if (view.gravity)
		view.CalcViewZ();

	RendInfo rend;

	rend.DoRender3D();

	if (view.low_detail)
		Render3D_BlitLores(ox, oy, ow, oh);
	else
		Render3D_BlitHires(ox, oy, ow, oh);
}


void Render3D_Wheel(int delta, keycode_t mod)
{
	int speed = 16;  // TODO: CONFIG ITEM

	if (mod == MOD_SHIFT)
		speed = MAX(1, speed / 8);
	else if (mod == MOD_COMMAND)
		speed *= 4;

	delta = delta * speed * 3;

	view.x += view.Cos * delta;
	view.y += view.Sin * delta;

	edit.RedrawMap = 1;
}


void Render3D_RBScroll(int dx, int dy, keycode_t mod)
{
	// we separate the movement into either turning or moving up/down
	// (never both at the same time : CONFIG IT THOUGH).

	bool force_one_dir = true;

	if (force_one_dir)
	{
		if (abs(dx) >= abs(dy))
			dy = 0;
		else
			dx = 0;
	}

	if (mod == MOD_ALT)  // strafing
	{
		if (dx)
		{
			view.x += view.Sin * dx * 2;
			view.y -= view.Cos * dx * 2;

			dx = 0;
		}
/*
		dy = -dy;  // CONFIG OPT

		if (dy)
		{
			view.x += view.Cos * dy * 2;
			view.y += view.Sin * dy * 2;

			dy = 0;
		}
*/
	}

	if (dx)
	{
		int speed = 12;  // TODO: CONFIG ITEM  [also: reverse]

		if (mod == MOD_SHIFT)
			speed = MAX(1, speed / 4);
		else if (mod == MOD_COMMAND)
			speed *= 3;

		double d_ang = dx * M_PI * speed / (1440.0*4.0);

		view.SetAngle(view.angle - d_ang);
	}

	if (dy)
	{
		int speed = 12;  // TODO: CONFIG ITEM  [also: reverse]

		if (mod == MOD_SHIFT)
			speed = MAX(1, speed / 4);
		else if (mod == MOD_COMMAND)
			speed *= 3;

		view.z -= dy * speed / 16;

		view.gravity = false;
	}

	edit.RedrawMap = 1;
}


/* commands... */

void R3D_Forward(void)
{
	int dist = atoi(EXEC_Param[0]);

	view.x += view.Cos * dist;
	view.y += view.Sin * dist;

	edit.RedrawMap = 1;
}

void R3D_Backward(void)
{
	int dist = atoi(EXEC_Param[0]);

	view.x -= view.Cos * dist;
	view.y -= view.Sin * dist;

	edit.RedrawMap = 1;
}

void R3D_Left(void)
{
	int dist = atoi(EXEC_Param[0]);

	view.x -= view.Sin * dist;
	view.y += view.Cos * dist;

	edit.RedrawMap = 1;
}

void R3D_Right(void)
{
	int dist = atoi(EXEC_Param[0]);

	view.x += view.Sin * dist;
	view.y -= view.Cos * dist;

	edit.RedrawMap = 1;
}

void R3D_Up(void)
{
	int dist = atoi(EXEC_Param[0]);

	view.z += dist;
	view.gravity = false;

	edit.RedrawMap = 1;
}

void R3D_Down(void)
{
	int dist = atoi(EXEC_Param[0]);

	view.z -= dist;
	view.gravity = false;

	edit.RedrawMap = 1;
}


void R3D_Turn(void)
{
	float angle = atof(EXEC_Param[0]);

	// convert to radians
	angle = angle * M_PI / 180.0;

	view.SetAngle(view.angle + angle);

	edit.RedrawMap = 1;
}

void R3D_DropToFloor(void)
{
	view.CalcViewZ();

	edit.RedrawMap = 1;
}


void R3D_Set(void)
{
	const char *var_name = EXEC_Param[0];
	const char *value    = EXEC_Param[1];

	if (! var_name[0])
	{
		Beep("3D_Set: missing var name");
		return;
	}

	if (! value[0])
	{
		Beep("3D_Set: missing value");
		return;
	}

	 int  int_val = atoi(value);
	bool bool_val = (int_val > 0);


	if (y_stricmp(var_name, "gamma") == 0)
	{
		usegamma = int_val % 5;
		if (usegamma < 0) usegamma = 0;
		W_UpdateGamma();
		Status_Set("Gamma level %d", usegamma);
	}
	else if (y_stricmp(var_name, "tex") == 0)
	{
		view.texturing = bool_val;
	}
	else if (y_stricmp(var_name, "obj") == 0)
	{
		view.sprites = bool_val;
		view.thsec_invalidated = true;
	}
	else if (y_stricmp(var_name, "light") == 0)
	{
		view.lighting = bool_val;
	}
	else if (y_stricmp(var_name, "grav") == 0)
	{
		view.gravity = bool_val;
	}
	else if (y_stricmp(var_name, "detail") == 0)
	{
		view.low_detail = bool_val;
	}
	else
	{
		Beep("3D_Set: unknown var: %s", var_name);
		return;
	}

	edit.RedrawMap = 1;
}


void R3D_Toggle(void)
{
	const char *var_name = EXEC_Param[0];

	if (! var_name[0])
	{
		Beep("3D_Toggle: missing var name");
		return;
	}

	if (y_stricmp(var_name, "tex") == 0)
	{
		view.texturing = ! view.texturing;
	}
	else if (y_stricmp(var_name, "obj") == 0)
	{
		view.sprites = ! view.sprites;
		view.thsec_invalidated = true;
	}
	else if (y_stricmp(var_name, "light") == 0)
	{
		view.lighting = ! view.lighting;
	}
	else if (y_stricmp(var_name, "grav") == 0)
	{
		view.gravity = ! view.gravity;
	}
	else if (y_stricmp(var_name, "detail") == 0)
	{
		view.low_detail = ! view.low_detail;
	}
	else
	{
		Beep("3D_Toggle: unknown var: %s", var_name);
		return;
	}

	edit.RedrawMap = 1;
}


void R3D_Gamma(void)
{
	int delta = (atoi(EXEC_Param[0]) >= 0) ? +1 : -1;

	usegamma = (usegamma + delta + 5) % 5;

	W_UpdateGamma();

	Status_Set("Gamma level %d", usegamma);

	edit.RedrawMap = 1;
}


void Render3D_RegisterCommands()
{
	M_RegisterCommand("3D_Forward",  &R3D_Forward);
	M_RegisterCommand("3D_Backward", &R3D_Backward);
	M_RegisterCommand("3D_Left",     &R3D_Left);
	M_RegisterCommand("3D_Right",    &R3D_Right);

	M_RegisterCommand("3D_Up",       &R3D_Up);
	M_RegisterCommand("3D_Down",     &R3D_Down);
	M_RegisterCommand("3D_Turn",     &R3D_Turn);
	M_RegisterCommand("3D_DropToFloor", &R3D_DropToFloor);

	M_RegisterCommand("3D_Gamma",  &R3D_Gamma);
	M_RegisterCommand("3D_Set",    &R3D_Set);
	M_RegisterCommand("3D_Toggle", &R3D_Toggle);
}


void Render3D_Term()
{
	/* all done */

	delete view.screen;
	view.screen = NULL;
}


void Render3D_SetCameraPos(int new_x, int new_y)
{
	view.x = new_x;
	view.y = new_y;

	view.CalcViewZ();
}


void Render3D_GetCameraPos(int *x, int *y, float *angle)
{
	*x = view.x;
	*y = view.y;

	// convert angle from radians to degrees
	*angle = view.angle * 180.0 / M_PI;
}


bool Render3D_ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "camera") == 0 && num_tok >= 5)
	{
		view.x = atof(tokens[1]);
		view.y = atof(tokens[2]);
		view.z = atoi(tokens[3]);

		view.SetAngle(atof(tokens[4]));

		return true;
	}

	if (strcmp(tokens[0], "r_modes") == 0 && num_tok >= 4)
	{
		view.texturing = atoi(tokens[1]) ? true : false;
		view.sprites   = atoi(tokens[2]) ? true : false;
		view.lighting  = atoi(tokens[3]) ? true : false;

		return true;
	}

	if (strcmp(tokens[0], "low_detail") == 0 && num_tok >= 2)
	{
		view.low_detail = atoi(tokens[1]) ? true : false;

		return true;
	}

	if (strcmp(tokens[0], "gamma") == 0 && num_tok >= 2)
	{
		usegamma = MAX(0, atoi(tokens[1])) % 5;

		W_UpdateGamma();

		return true;
	}

	return false;
}


void Render3D_WriteUser(FILE *fp)
{
	fprintf(fp, "camera %1.2f %1.2f %d %1.2f\n",
	        view.x,
			view.y,
			view.z,
			view.angle);

	fprintf(fp, "r_modes %d %d %d\n",
	        view.texturing  ? 1 : 0,
			view.sprites    ? 1 : 0,
			view.lighting   ? 1 : 0);

	fprintf(fp, "low_detail %d\n",
	        view.low_detail ? 1 : 0);

	fprintf(fp, "gamma %d\n",
	        usegamma);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
