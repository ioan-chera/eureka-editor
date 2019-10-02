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

#include <FL/Fl_Gl_Window.H>

#include "m_events.h"
#include "m_select.h"
#include "e_objects.h"
#include "r_grid.h"


class Img_c;

void SectorCache_Invalidate();


class UI_Canvas : public Fl_Gl_Window
{
private:
	Objid highlight;

	// split-able line state
	int split_ld;
	int split_x;
	int split_y;

	// sel-box state
	int  selbox_x1, selbox_y1;  // map coords
	int  selbox_x2, selbox_y2;

	// dragging state
	int  drag_start_x, drag_start_y;
	int  drag_focus_x, drag_focus_y;
	int  drag_cur_x,   drag_cur_y;
	selection_c drag_lines;

	// scaling/rotating state
	int   trans_start_x,  trans_start_y;
	transform_keyword_e trans_mode;
	transform_t trans_param;
	selection_c trans_lines;

	// drawing state only
	int map_lx, map_ly;
	int map_hx, map_hy;

	bitvec_c seen_sectors;

public:
	UI_Canvas(int X, int Y, int W, int H, const char *label = NULL);
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

	void HighlightSet(Objid& obj);
	void HighlightForget();

	void SplitLineSet(int ld, int new_x, int new_y);
	void SplitLineForget();

	void DrawSelection(selection_c *list);
	void DrawSectorSelection(selection_c *list, int dx, int dy);
	void DrawHighlight(int objtype, int objnum,
	                   bool skip_lines = false, int dx=0, int dy=0);
	void DrawHighlightTransform(int objtype, int objnum);
	void DrawTagged(int objtype, int objnum);

	void SelboxBegin(int map_x, int map_y);
	void SelboxUpdate(int map_x, int map_y);
	void SelboxFinish(int *x1, int *y1, int *x2, int *y2);

	void DragBegin(int focus_x, int focus_y, int map_x, int map_y);
	void DragUpdate(int map_x, int map_y);
	void DragFinish(int *dx, int *dy);

	void TransformBegin(int map_x, int map_y, int middle_x, int middle_y, transform_keyword_e mode);
	void TransformUpdate(int map_x, int map_y);
	void TransformFinish(transform_t& param);

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

	void DrawMapPoint(int map_x, int map_y);
	void DrawMapLine(int map_x1, int map_y1, int map_x2, int map_y2);
	void DrawMapVector(int map_x1, int map_y1, int map_x2, int map_y2);
	void DrawMapArrow(int map_x1, int map_y1, int r, int angle);

	void DrawKnobbyLine(int map_x1, int map_y1, int map_x2, int map_y2, bool reverse = false);
	void DrawSplitLine(int map_x1, int map_y1, int map_x2, int map_y2);
	void DrawSplitPoint(int map_x, int map_y);
	void DrawVertex(int map_x, int map_y, int r);
	void DrawThing(int map_x, int map_y, int r, int angle, bool big_arrow);
	void DrawSprite(int map_x, int map_y, Img_c *img, float scale);
	void DrawCamera();

	void DrawLineNumber(int mx1, int my1, int mx2, int my2, int side, int n);
	void DrawSectorNum(int mx1, int my1, int mx2, int my2, int side, int n);
	void DrawObjNum(int x, int y, int num, bool center = false);
	void DrawCurrentLine();

	void RenderSector(int num);

	void SelboxDraw();

	void DragDelta(int *dx, int *dy);

	// convert GL coordinates to map coordinates
	inline float MAPX(int sx) const { return grid.orig_x + (sx - w()/2) / grid.Scale; }
	inline float MAPY(int sy) const { return grid.orig_y + (sy - h()/2) / grid.Scale; }

	// convert map coordinates to GL coordinates
	inline int SCREENX(int mx) const { return (w()/2 + I_ROUND((mx - grid.orig_x) * grid.Scale)); }
	inline int SCREENY(int my) const { return (h()/2 + I_ROUND((my - grid.orig_y) * grid.Scale)); }

	inline bool Vis(int x, int y, int r) const
	{
		return (x+r >= map_lx) && (x-r <= map_hx) &&
		       (y+r >= map_ly) && (y-r <= map_hy);
	}
	inline bool Vis(int x1, int y1, int x2, int y2) const
	{
		return (x2 >= map_lx) && (x1 <= map_hx) &&
		       (y2 >= map_ly) && (y1 <= map_hy);
	}

	/* OpenGL-only stuff */

	void gl_line(int x1, int y1, int x2, int y2);
	void gl_line_width(int w);
};


#endif  /* __EUREKA_UI_CANVAS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
