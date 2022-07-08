//------------------------------------------------------------------------
//  EDITING CANVAS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2008-2019 Andrew Apted
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

#ifndef __EUREKA_UI_CANVAS_H__
#define __EUREKA_UI_CANVAS_H__

#ifndef NO_OPENGL
#include <FL/Fl_Gl_Window.H>
#else
#include <FL/Fl_Widget.H>
#endif

#include "m_events.h"
#include "m_select.h"
#include "e_objects.h"
#include "r_grid.h"
#include "sys_macro.h"

class Img_c;
enum class Side;
struct v2double_t;

#ifdef NO_OPENGL
class UI_Canvas : public Fl_Widget
#else
class UI_Canvas : public Fl_Gl_Window
#endif
{
private:
	// this is used to detect changes in inst.edit.highlight
	// [ to prevent unnecessary redraws ]
	Objid last_highlight;

	// this is used to detect changes in inst.edit.split_line (etc)
	// [ to prevent unnecessary redraws ]
	int last_splitter;
	double last_split_x;
	double last_split_y;

	// this is the cached grid-snap position
	double snap_x, snap_y;

	// drawing state only
	double map_lx, map_ly;
	double map_hx, map_hy;

	bitvec_c seen_sectors;

	// a copy of x() and y() for software renderer, 0 for OpenGL
	int xx, yy;

	// state for the custom S/W rendering code
#ifdef NO_OPENGL
	byte *rgb_buf;
	int rgb_x, rgb_y;
	int rgb_w, rgb_h;
	int thickness;
	struct { byte r, g, b; } cur_col;
#endif
	int cur_font;  // 14 or 19

public:
	UI_Canvas(Instance &inst, int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_Canvas();

public:
	// FLTK virtual method for handling input events.
	int handle(int event);

	// FLTK virtual method for resizing.
	void resize(int X, int Y, int W, int H);

	// delete any OpenGL context which is assigned to the window.
	// call this whenever OpenGL textures need to be reloaded.
	void DeleteContext();

	void DrawEverything();

	void UpdateHighlight();

	void CheckGridSnap();

	void DrawSelection(selection_c *list);
	void DrawSectorSelection(selection_c *list, double dx, double dy);
	void DrawHighlight(ObjType objtype, int objnum,
	                   bool skip_lines = false, double dx=0, double dy=0);
	void DrawHighlightTransform(ObjType objtype, int objnum);
	void DrawTagged(ObjType objtype, int objnum);

	// returns true if ok, false if box was very small
	bool SelboxGet(v2double_t &pos1, v2double_t &pos2);

	v2double_t DragDelta();

	void PointerPos(bool in_event = false);

	// return -1 if too small, 0 is OK, 1 is too big to fit
	int ApproxBoxSize(int mx1, int my1, int mx2, int my2);

private:
	// FLTK virtual method for drawing
	void draw();

	void DrawMap();

	void DrawGrid_Dotty();
	void DrawGrid_Normal();
	void DrawAxes(Fl_Color col);

	void DrawMapBounds();
	void DrawVertices();
	void DrawLinedefs();
	void DrawThings();
	void DrawThingBodies();
	void DrawThingSprites();

	void DrawMapLine(double map_x1, double map_y1, double map_x2, double map_y2);
	void DrawMapVector(double map_x1, double map_y1, double map_x2, double map_y2);
	void DrawMapArrow(double map_x1, double map_y1, int r, int angle);

	void DrawKnobbyLine(double map_x1, double map_y1, double map_x2, double map_y2, bool reverse = false);
	void DrawSplitLine(double map_x1, double map_y1, double map_x2, double map_y2);
	void DrawSplitPoint(const v2double_t &map_pos);
	void DrawVertex(double map_x, double map_y, int r);
	void DrawThing(double map_x, double map_y, int r, int angle, bool big_arrow);
	void DrawCamera();

	void DrawSectorNum(int mx1, int my1, int mx2, int my2, Side side, int n);
	void DrawLineNumber(int mx1, int my1, int mx2, int my2, Side side, int n);
	void DrawLineInfo(double map_x1, double map_y1, double map_x2, double map_y2, bool force_ratio);
	void DrawNumber(int x, int y, int num);
	void DrawCurrentLine();
	void DrawSnapPoint();

	void SelboxDraw();

	// calc screen-space normal of a line
	int NORMALX(int len, double dx, double dy);
	int NORMALY(int len, double dx, double dy);

#ifdef NO_OPENGL
	// convert screen coordinates to map coordinates
	inline double MAPX(int sx) const;
	inline double MAPY(int sy) const;

	// convert map coordinates to screen coordinates
	inline int SCREENX(double mx) const;
	inline int SCREENY(double my) const;
#else
	// convert GL coordinates to map coordinates
	inline double MAPX(int sx) const;
	inline double MAPY(int sy) const;

	// convert map coordinates to GL coordinates
	inline int SCREENX(double mx) const;
	inline int SCREENY(double my) const;
#endif

	inline bool Vis(double x, double y, int r) const
	{
		return (x+r >= map_lx) && (x-r <= map_hx) &&
		       (y+r >= map_ly) && (y-r <= map_hy);
	}
	inline bool Vis(double x1, double y1, double x2, double y2) const
	{
		return (x2 >= map_lx) && (x1 <= map_hx) &&
		       (y2 >= map_ly) && (y1 <= map_hy);
	}


	void PrepareToDraw();
	void Blit();

	void RenderColor(Fl_Color c);
	void RenderThickness(int w);
	void RenderFontSize(int size);

	void RenderLine(int x1, int y1, int x2, int y2);
	void RenderRect(int rx, int ry, int rw, int rh);

	void RenderNumString(int x, int y, const char *s);
	void RenderFontChar(int rx, int ry, Img_c *img, int ix, int iy, int iw, int ih);

	void RenderSprite(int sx, int sy, float scale, Img_c *img);
	void RenderSector(int num);

#ifdef NO_OPENGL
	int Calc_Outcode(int x, int y);

	// this is raw, it does no checking!
	inline void raw_pixel(int rx, int ry) const
	{
		byte *dest = rgb_buf + (rx + ry * rgb_w) * 3;

		dest[0] = cur_col.r;
		dest[1] = cur_col.g;
		dest[2] = cur_col.b;
	}
#endif

	Instance &inst;
};


#endif  /* __EUREKA_UI_CANVAS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
