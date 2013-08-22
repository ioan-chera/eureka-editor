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

#include "im_color.h"
#include "im_img.h"
#include "e_linedef.h"
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
#include "ui_window.h"


#define REND_SEL_COL  196


// config items
int  render_aspect_ratio = 177;   // 100 * width / height, default 16:9

bool render_lock_gravity   = false;
bool render_missing_bright = true;
bool render_unknown_bright = true;


struct highlight_3D_info_t
{
public:
	int line;    // -1 for none
	int sector;  // -1 for none
	int side;    // SIDE_XXX, or -1=floor +1=ceiling
	query_part_e part;

public:
	highlight_3D_info_t() : line(-1), sector(-1), side(0), part(QRP_Lower)
	{ }

	highlight_3D_info_t(const highlight_3D_info_t& other) :
		line(other.line), sector(other.sector),
		side(other.side),   part(other.part)
	{ }

	void Clear()
	{
		line = sector = -1;
		side = 0;
		part = QRP_Lower;
	}

	bool isSame(const highlight_3D_info_t& other) const
	{
		return	(line == other.line) && (sector == other.sector) &&
				(side == other.side) && (part == other.part);
	}

};


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

	Img *missing_tex;  int missing_col;
	Img *unknown_tex;  int unk_tex_col;
	Img *unknown_flat; int unk_flat_col;

	// state for adjusting offsets via the mouse
	int   adjust_ld;
	int   adjust_sd;

	float adjust_dx, adjust_dx_factor;
	float adjust_dy, adjust_dy_factor;

	// current highlighted wotsit
	highlight_3D_info_t hl;

public:
	Y_View() : p_type(0), screen(NULL),
			   texturing(false), sprites(false), lighting(false),
			   low_detail(false), gravity(true),
	           thing_sectors(),
			   thsec_sector_num(0), thsec_invalidated(false),
			   missing_tex(NULL),  missing_col(-1),
			   unknown_tex(NULL),  unk_tex_col(-1),
			   unknown_flat(NULL), unk_flat_col(-1),
			   adjust_ld(-1), adjust_sd(-1),
			   hl()
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
		float screen_aspect = float(sw) / float(sh);

		aspect_sh = sh / (render_aspect_ratio / 100.0) * screen_aspect;
		aspect_sw = sw;
	}

	void UpdateScreen(int ow, int oh)
	{
		// in low detail mode, setup size so that expansion always covers
		// our window (i.e. we draw a bit more than we need).

		int new_sw = low_detail ? (ow + 1) / 2 : ow;
		int new_sh = low_detail ? (oh + 1) / 2 : oh;

		if (!screen || sw != new_sw || sh != new_sh)
		{
			sw = new_sw;
			sh = new_sh;

			if (screen)
				delete[] screen;

			screen = new byte [sw * sh];
		}

		CalcAspect();
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

	void UpdateDummies()
	{
		if (missing_col != game_info.missing_color)
		{
			missing_col = game_info.missing_color;
			if (missing_tex) delete missing_tex;
			missing_tex = IM_CreateMissingTex(missing_col, 0);
		}

		if (unk_tex_col != game_info.unknown_tex)
		{
			unk_tex_col = game_info.unknown_tex;
			if (unknown_tex) delete unknown_tex;
			unknown_tex = IM_CreateUnknownTex(unk_tex_col, 0);
		}

		if (unk_flat_col != game_info.unknown_flat)
		{
			unk_flat_col = game_info.unknown_flat;
			if (unknown_flat) delete unknown_flat;
			unknown_flat = IM_CreateUnknownTex(unk_flat_col, 0);
		}
	}

	void PrepareToRender(int ow, int oh)
	{
		if (thsec_invalidated || !screen ||
			NumThings  != (int)thing_sectors.size() ||
			NumSectors != thsec_sector_num)
		{
			FindThingSectors();
		}

		UpdateDummies();

		UpdateScreen(ow, oh);

		if (gravity)
			CalcViewZ();
	}

	void ClearHighlight()
	{
		hl.Clear();
	}

	void FindHighlight()
	{
		hl.sector = -1;

		hl.line = main_win->render->query(&hl.side, &hl.part);

		if (hl.part == QRP_Floor || hl.part == QRP_Ceil)
		{
			// FIXME: get sector
			hl.line = -1;
		}
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
	img_pixel_t col;  /* used when no image */

	enum
	{
		SOLID_ABOVE = 1,
		SOLID_BELOW = 2
	};
	int y_clip;

	bool fullbright;

public:
	DrawSurf() : kind(K_INVIS), img(NULL), fullbright(false)
	{ }

	~DrawSurf()
	{ }

	int hashed_color(const char *name, const int *cols)
	{
		int hash = name[0]*17 + name[2]*7  + name[4]*3 +
		           name[5]*13 + name[6]*47 + name[7];

		hash ^= (hash >> 5);

		int c1 = cols[0];
		int c2 = cols[1];

		if (c1 > c2)
			std::swap(c1, c2);

		if (c1 == c2)
			return c1;

		return c1 + hash % (c2 - c1 + 1);
	}

	void FindFlat(const char * fname, Sector *sec)
	{
		fullbright = false;

		if (is_sky(fname))
		{
			col = game_info.sky_color;
			fullbright = true;
			return;
		}

		if (view.texturing)
		{
			img = W_GetFlat(fname);

			if (! img)
			{
				img = view.unknown_flat;
				fullbright = render_unknown_bright;
			}

			return;
		}

		col = hashed_color(fname, game_info.floor_colors);
	}

	void FindTex(const char * tname, LineDef *ld)
	{
		fullbright = false;

		if (view.texturing)
		{
			if (tname[0] == '-')
			{
				img = view.missing_tex;
				fullbright = render_missing_bright;
				return;
			}

			img = W_GetTexture(tname);

			if (! img)
			{
				img = view.unknown_tex;
				fullbright = render_unknown_bright;
			}

			return;
		}

		col = hashed_color(tname, game_info.wall_colors);
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

	// which side this wall faces (SIDE_LEFT or SIDE_RIGHT)
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
	double iz1, iz2;
	double diz, cur_iz; 
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

		SideDef *back_sd = (side == SIDE_LEFT) ? ld->Right() : ld->Left();
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

			ceil.FindFlat(front->CeilTex(), front);
		}

		if (front->floorh < view.z && ! self_ref)
		{
			floor.kind = DrawSurf::K_FLAT;
			floor.h1 = -99999;
			floor.h2 = front->floorh;
			floor.tex_h = floor.h2;
			floor.y_clip = DrawSurf::SOLID_BELOW;

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


struct RenderLine
{
	short sx1, sy1, sx2, sy2;

	Fl_Color color;
};


struct RendInfo
{
public:
	// complete set of walls/sprites to draw.
	DrawWall::vec_t walls;

	// the active list.  Pointers here are always duplicates of ones in
	// the walls list (no need to 'delete' any of them).
	DrawWall::vec_t active;

	// query state
	int query_mode;  // 0 for normal render
	int query_sx;
	int query_sy;

	DrawWall     *query_wall;  // the hit wall
	query_part_e  query_part;  // the part of the hit wall

	// inverse distances over X range, 0 when empty.
	std::vector<double> depth_x;  

	int open_y1;
	int open_y2;

#define Y_SLOPE  1.70

	// remembered lines for drawing highlight (etc)
	std::vector<RenderLine> hl_lines;

	// saved offsets for mouse adjustment mode
	int saved_x_offset;
	int saved_y_offset;

private:
	static void DeleteWall(DrawWall *P)
	{
		delete P;
	}

public:
	RendInfo() : walls(), active(), query_mode(0), depth_x(),
				 hl_lines()
	{ }

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

	void AddRenderLine(int sx1, int sy1, int sx2, int sy2, Fl_Color color)
	{
		if (view.low_detail)
		{
			sx1 *= 2;  sy1 *= 2;
			sx2 *= 2;  sy2 *= 2;
		}

		RenderLine new_line;

		new_line.sx1 = sx1; new_line.sy1 = sy1;
		new_line.sx2 = sx2; new_line.sy2 = sy2;
		new_line.color = color;

		hl_lines.push_back(new_line);
	}

	void SaveOffsets()
	{
		if (view.adjust_ld < 0)
			return;

		SideDef *SD = SideDefs[view.adjust_sd];

		saved_x_offset = SD->x_offset;
		saved_y_offset = SD->y_offset;

		// change it temporarily (just for the render)
		SD->x_offset += (int)view.adjust_dx;
		SD->y_offset += (int)view.adjust_dy;
	}

	void RestoreOffsets()
	{
		if (view.adjust_ld < 0)
			return;

		SideDef *SD = SideDefs[view.adjust_sd];

		SD->x_offset = saved_x_offset;
		SD->y_offset = saved_y_offset;
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

		if (! ld->Right())
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

		// optimisation for query mode
		if (query_mode && (sx2 < query_sx || sx1 > query_sx))
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

		if (side == SIDE_LEFT)
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
		dw->iz2 = iz2;
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

	void HighlightWall(DrawWall *dw)
	{
		if (dw->side != view.hl.side)
			return;

		int h1, h2;

		if (! dw->ld->TwoSided())
		{
			h1 = dw->sd->SecRef()->floorh;
			h2 = dw->sd->SecRef()->ceilh;
		}
		else
		{
			const Sector *front = dw->ld->Right()->SecRef();
			const Sector *back  = dw->ld-> Left()->SecRef();

			if (view.hl.part == QRP_Lower)
			{
				h1 = MIN(front->floorh, back->floorh);
				h2 = MAX(front->floorh, back->floorh);
			}
			else /* part == QRP_Upper */
			{
				h1 = MIN(front->ceilh, back->ceilh);
				h2 = MAX(front->ceilh, back->ceilh);
			}
		}

		int x1 = dw->sx1 - 1;
		int x2 = dw->sx2 + 1;

		int ly1 = DistToY(dw->iz1, h2);
		int ly2 = DistToY(dw->iz1, h1);

		int ry1 = DistToY(dw->iz2, h2);
		int ry2 = DistToY(dw->iz2, h1);

		AddRenderLine(x1, ly1, x1, ly2, HI_COL); 
		AddRenderLine(x2, ry1, x2, ry2, HI_COL); 
		AddRenderLine(x1, ly1, x2, ry1, HI_COL);
		AddRenderLine(x1, ly2, x2, ry2, HI_COL);
	}

	void ComputeSurfaces()
	{
		const LineDef *hl_linedef = is_linedef(view.hl.line) ?
			LineDefs[view.hl.line] : NULL;

		DrawWall::vec_t::iterator S;

		for (S = walls.begin() ; S != walls.end() ; S++)
		{
			if ((*S)->ld)
			{
				(*S)->ComputeWallSurface();

				if ((*S)->ld == hl_linedef)
					HighlightWall(*S);
			}
		}
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
			
			if (view.lighting && ! surf.fullbright)
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

			if (view.lighting && ! surf.fullbright)
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

			if (view.lighting && ! surf.fullbright)
				*buf = view.DoomLightRemap(light, dist, game_info.floor_colors[1]);
			else
				*buf = surf.col;
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
			if (view.lighting && ! surf.fullbright)
				*buf = view.DoomLightRemap(light, dist, game_info.wall_colors[1]);
			else
				*buf = surf.col;
		}
	}

	void HighlightColumn(int x, int y1, int y2, byte col)
	{
		img_pixel_t *buf = view.screen;

		buf += x + y1 * view.sw;

		for ( ; y1 <= y2 ; y1++, buf += view.sw)
			*buf = col;
	}


	inline void RenderWallSurface(DrawWall *dw, DrawSurf& surf, int x,
								  query_part_e part)
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

		/* query mode : is mouse over this wall part? */

		if (query_mode)
		{
			if (y1 <= query_sy && query_sy <= y2)
			{
				query_wall = dw;
				query_part = part;
			}

			return;
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

			// in query mode, only care about a single column
			if (query_mode && x != query_sx)
				continue;

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

				RenderWallSurface(dw, dw->ceil,  x, QRP_Ceil);
				RenderWallSurface(dw, dw->floor, x, QRP_Floor);
				RenderWallSurface(dw, dw->upper, x, QRP_Upper);
				RenderWallSurface(dw, dw->lower, x, QRP_Lower);

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

		SaveOffsets();

		for (int i=0 ; i < NumLineDefs ; i++)
			AddLine(i);

		if (view.sprites && ! query_mode)
			for (int k=0 ; k < NumThings ; k++)
				AddThing(k);

		ClipSolids();

		ComputeSurfaces();

		RenderWalls();

		RestoreOffsets();
	}

	void DoQuery(int sx, int sy)
	{
		query_mode = 1;
		query_sx   = sx;
		query_sy   = sy;

		query_wall = NULL;

		DoRender3D();

		query_mode = 0;
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


UI_Render3D::UI_Render3D(int X, int Y, int W, int H) :
	Fl_Widget(X, Y, W, H)
{ }


UI_Render3D::~UI_Render3D()
{ }


void UI_Render3D::draw()
{
	int ox = x();
	int oy = y();
	int ow = w();
	int oh = h();

	view.PrepareToRender(ow, oh);

	RendInfo rend;

	rend.DoRender3D();

	if (view.low_detail)
		BlitLores(ox, oy, ow, oh);
	else
		BlitHires(ox, oy, ow, oh);
	
	// draw the highlight (etc)
	for (unsigned int k = 0 ; k < rend.hl_lines.size() ; k++)
	{
		RenderLine& line = rend.hl_lines[k];

		fl_color(line.color);
		fl_line(ox + line.sx1, oy + line.sy1, ox + line.sx2, oy + line.sy2);
	}
}


int UI_Render3D::query(int *side, query_part_e *part)
{
	int ow = w();
	int oh = h();

	view.PrepareToRender(ow, oh);

	int sx = Fl::event_x() - x();
	int sy = Fl::event_y() - y();

	if (view.low_detail)
	{
		sx = sx / 2;
		sy = sy / 2;
	}

	RendInfo rend;

	rend.DoQuery(sx, sy);

	if (rend.query_wall)
	{
		*side = rend.query_wall->side;
		*part = rend.query_part;

		// ouch -- fix?
		for (int n = 0 ; n < NumLineDefs ; n++)
			if (rend.query_wall->ld == LineDefs[n])
				return n;
	}

	// nothing was hit
	return -1;
}


void UI_Render3D::BlitHires(int ox, int oy, int ow, int oh)
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


void UI_Render3D::BlitLores(int ox, int oy, int ow, int oh)
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


int UI_Render3D::handle(int event)
{
	switch (event)
	{
		case FL_FOCUS:
			return 1;

		case FL_ENTER:
			// we greedily grab the focus
			if (Fl::focus() != this)
				take_focus(); 

			return 1;

		case FL_KEYDOWN:
		case FL_KEYUP:
		case FL_SHORTCUT:
			return Editor_RawKey(event);

		case FL_PUSH:
		case FL_RELEASE:
			return Editor_RawButton(event);

		case FL_MOUSEWHEEL:
			return Editor_RawWheel(event);

		case FL_DRAG:
		case FL_MOVE:
			return Editor_RawMouse(event);

		default:
			break;  // pass it on
	}

	return Fl_Widget::handle(event);
}


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


void Render3D_MouseMotion(int x, int y, keycode_t mod, bool drag)
{
	highlight_3D_info_t old(view.hl);

	view.FindHighlight();

	if (old.isSame(view.hl))
		return;

	main_win->render->redraw();
}


void Render3D_Wheel(int dx, int dy, keycode_t mod)
{
	float speed = 48;  // TODO: CONFIG ITEM

	if (mod == MOD_SHIFT)
		speed = MAX(1, speed / 8);
	else if (mod == MOD_COMMAND)
		speed *= 4;

	view.x += speed * (view.Cos * dy + view.Sin * dx);
	view.y += speed * (view.Sin * dy - view.Cos * dx);

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

	if (dy && ! (render_lock_gravity && view.gravity))
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


void Render3D_AdjustOffsets(int mode, int dx, int dy)
{
	// started?
	if (mode < 0)
	{
		// find the line / side to adjust
		if (! is_linedef(view.hl.line))
			return;

		if (view.hl.part == QRP_Floor || view.hl.part == QRP_Ceil)
			return;

		const LineDef *L = LineDefs[view.hl.line];

		int sd = (view.hl.side < 0) ? L->left : L->right;

		if (sd < 0)  // WTF?
			return;

		// OK
		view.adjust_ld = view.hl.line;
		view.adjust_sd = sd;

		// reset offset deltas to 0
		view.adjust_dx = 0;
		view.adjust_dy = 0;

		float dist = ApproxDistToLineDef(L, view.x, view.y);
		if (dist < 20) dist = 20;

		// TODO: take perspective into account (angled wall --> reduce dx_factor)
		view.adjust_dx_factor = dist / view.aspect_sw;
		view.adjust_dy_factor = dist / view.aspect_sh / Y_SLOPE;

		Editor_SetAction(ACT_ADJUST_OFS);
		return;
	}


	if (edit.action != ACT_ADJUST_OFS)
		return;

	SYS_ASSERT(view.adjust_ld >= 0);


	// finished?
	if (mode > 0)
	{
		// apply the offset deltas now
		dx = (int)view.adjust_dx;
		dy = (int)view.adjust_dy;

		if (dx || dy)
		{
			const SideDef * SD = SideDefs[view.adjust_sd];

			BA_Begin();
			BA_ChangeSD(view.adjust_sd, SideDef::F_X_OFFSET, SD->x_offset + dx);
			BA_ChangeSD(view.adjust_sd, SideDef::F_Y_OFFSET, SD->y_offset + dy);
			BA_End();
		}

		view.adjust_ld = -1;
		view.adjust_sd = -1;

		Editor_ClearAction();
		return;
	}


	if (dx == 0 && dy == 0)
		return;


	bool force_one_dir = true;

	if (force_one_dir)
	{
		if (abs(dx) >= abs(dy))
			dy = 0;
		else
			dx = 0;
	}


	keycode_t mod = Fl::event_state() & MOD_ALL_MASK;

	float factor = (mod == MOD_SHIFT) ? 0.25 : 1.0;

	if (! view.low_detail)
		factor = factor * 2.0;

	view.adjust_dx -= dx * factor * view.adjust_dx_factor;
	view.adjust_dy -= dy * factor * view.adjust_dy_factor;

	edit.RedrawMap = 1;
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


//------------------------------------------------------------------------
//  COMMAND FUNCTIONS
//------------------------------------------------------------------------

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
	if (view.gravity && render_lock_gravity)
	{
		Beep("Gravity is on");
		return;
	}

	int dist = atoi(EXEC_Param[0]);

	view.z += dist;
	view.gravity = false;

	edit.RedrawMap = 1;
}

void R3D_Down(void)
{
	if (view.gravity && render_lock_gravity)
	{
		Beep("Gravity is on");
		return;
	}

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


/* Align texture on a sidedef
 *
 * Possible flags:
 *    x : align X offset  \ one must be present
 *    y : align Y offset  /
 *
 *    c : clear offset(s) instead of aligning
 *
 * ?? t : require a Texture match
 */
void R3D_Align(void)
{
	if (! edit.render3d)
	{
		Beep("3D mode required");
		return;
	}

	// parse parameter
	const char *flags = EXEC_Param[0];

	bool do_X = strchr(flags, 'x') ? true : false;
	bool do_Y = strchr(flags, 'y') ? true : false;

	if (! (do_X || do_Y))
	{
		Beep("3D_Align: need x or y flag");
		return;
	}

	bool do_clear = strchr(flags, 'c') ? true : false;

	// find the line / side to align
	if (! is_linedef(view.hl.line) ||
		view.hl.part == QRP_Floor || view.hl.part == QRP_Ceil)
	{
		Beep("No sidedef there!");
		return;
	}

	const LineDef *L = LineDefs[view.hl.line];

	int sd = (view.hl.side < 0) ? L->left : L->right;

	if (sd < 0)  // should NOT happen
	{
		Beep("No sidedef there!");
		return;
	}

	if (do_clear)
	{
		BA_Begin();
		
		if (do_X) BA_ChangeSD(sd, SideDef::F_X_OFFSET, 0);
		if (do_Y) BA_ChangeSD(sd, SideDef::F_Y_OFFSET, 0);

		BA_End();

		return;
	}

	char part_c = (view.hl.part == QRP_Upper) ? 'u' : 'l';

	LineDefs_Align(view.hl.line, view.hl.side, sd, part_c, flags);
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
	M_RegisterCommand("3D_Align",  &R3D_Align);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
