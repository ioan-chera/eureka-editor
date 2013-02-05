//------------------------------------------------------------------------
//  Main Window
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2008-2012 Andrew Apted
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

#include "editloop.h"
#include "m_select.h"
#include "objects.h"
#include "r_grid.h"
#include "x_mirror.h"


class UI_Canvas : public Fl_Widget
{
private:
	bool render3d;

	Objid highlight;

	int split_ld;

	bool selbox_active;
	int  selbox_x1, selbox_y1;  // map coords
	int  selbox_x2, selbox_y2;

	bool drag_active;
	int  drag_start_x, drag_start_y;
	int  drag_focus_x, drag_focus_y;
	int  drag_cur_x,   drag_cur_y;
	selection_c drag_lines;

	bool  scale_active;
	int   scale_start_x,  scale_start_y;
	scale_param_t scale_param;
	selection_c scale_lines;

	int rbscroll_x, rbscroll_y;

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


	void DrawEverything();

	void HighlightSet(Objid& obj);
	void HighlightForget();

	void SplitLineSet(int ld);
	void SplitLineForget();

	void DrawSelection(selection_c *list);
	void DrawHighlight(int objtype, int objnum, Fl_Color col, bool do_tagged=true, int dx=0, int dy=0);
	void DrawHighlightScaled(int objtype, int objnum, Fl_Color col);

	void SelboxBegin(int map_x, int map_y);
	void SelboxUpdate(int map_x, int map_y);
	void SelboxFinish(int *x1, int *y1, int *x2, int *y2);

	void DragBegin(int focus_x, int focus_y, int map_x, int map_y);
	void DragUpdate(int map_x, int map_y);
	void DragFinish(int *dx, int *dy);

	void ScaleBegin(int map_x, int map_y, int middle_x, int middle_y);
	void ScaleUpdate(int map_x, int map_y, keycode_t mod);
	void ScaleFinish(scale_param_t& param);

	void ChangeRenderMode(int mode);
	void ToggleRenderMode() { ChangeRenderMode(! render3d); }

	// return -1 if too small, 0 is OK, 1 is too big to fit
	int ApproxBoxSize(int mx1, int my1, int mx2, int my2);

	bool isRenderActive() { return render3d; }
	bool isSelboxActive() { return selbox_active; }
	bool isDragActive()   { return drag_active; }
	bool isScaleActive()  { return scale_active; }

private:
	// FLTK virtual method for drawing.
	void draw();

	void DrawMap();
	void DrawGrid();

	void DrawMapBounds();
	void DrawVertices();
	void DrawLinedefs();
	void DrawThings();
	void DrawRTS();

	void DrawMapPoint(int map_x, int map_y);
	void DrawMapLine(int map_x1, int map_y1, int map_x2, int map_y2);
	void DrawMapVector(int map_x1, int map_y1, int map_x2, int map_y2);
	void DrawMapArrow(int map_x1, int map_y1, unsigned angle);
	void DrawKnobbyLine(int map_x1, int map_y1, int map_x2, int map_y2);
	void DrawVertex(int map_x, int map_y, int r);
	void DrawThing(int map_x, int map_y, int r, int angle, bool big_arrow);
	void DrawCamera();

	void DrawLineNumber(int mx1, int my1, int mx2, int my2, int side, int n);
	void DrawSectorNum(int mx1, int my1, int mx2, int my2, int side, int n);
	void DrawObjNum(int x, int y, int num, bool center = false);

	void SelboxDraw();

	void DragDelta(int *dx, int *dy);

	int handle_key();
	int handle_move(bool drag);
	int handle_push();
	int handle_release();
	int handle_wheel();

	void RightButtonScroll(int mode);

	// convert screen coordinates to map coordinates
	inline int MAPX(int sx) const { return I_ROUND(grid.orig_x + (sx - w()/2 - x()) / grid.Scale); }
	inline int MAPY(int sy) const { return I_ROUND(grid.orig_y + (h()/2 - sy + y()) / grid.Scale); }

	// convert map coordinates to screen coordinates
	inline int SCREENX(int mx) const { return (x() + w()/2 + I_ROUND((mx - grid.orig_x) * grid.Scale)); }
	inline int SCREENY(int my) const { return (y() + h()/2 + I_ROUND((grid.orig_y - my) * grid.Scale)); }

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
};


#endif  /* __EUREKA_UI_CANVAS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
