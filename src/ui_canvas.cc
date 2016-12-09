//------------------------------------------------------------------------
//  EDITING CANVAS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2016 Andrew Apted
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

#include "ui_window.h"

#include "m_events.h"
#include "e_main.h"
#include "e_sector.h"
#include "e_things.h"
#include "m_config.h"
#include "m_game.h"
#include "r_grid.h"
#include "im_color.h"
#include "im_img.h"
#include "r_render.h"
#include "w_texture.h"


#define CAMERA_COLOR  fl_rgb_color(255, 192, 255)


// config items
rgb_color_t dotty_axis_col  = RGB_MAKE(0, 128, 255);
rgb_color_t dotty_major_col = RGB_MAKE(0, 0, 238);
rgb_color_t dotty_minor_col = RGB_MAKE(0, 0, 187);
rgb_color_t dotty_point_col = RGB_MAKE(0, 0, 255);

rgb_color_t normal_axis_col  = RGB_MAKE(0, 128, 255);
rgb_color_t normal_main_col  = RGB_MAKE(0, 0, 238);
rgb_color_t normal_flat_col  = RGB_MAKE(60, 60, 120);
rgb_color_t normal_small_col = RGB_MAKE(60, 60, 120);


int vertex_radius(double scale);


//
// UI_Canvas Constructor
//
UI_Canvas::UI_Canvas(int X, int Y, int W, int H, const char *label) :
    Fl_Widget(X, Y, W, H, label),
	highlight(), split_ld(-1),
	drag_lines(),
	trans_lines(),
	seen_sectors()
{ }

//
// UI_Canvas Destructor
//
UI_Canvas::~UI_Canvas()
{ }


void UI_Canvas::resize(int X, int Y, int W, int H)
{
	Fl_Widget::resize(X, Y, W, H);
}


void UI_Canvas::draw()
{
	fl_push_clip(x(), y(), w(), h());

	fl_color(FL_WHITE);

	// default font (for showing object numbers)
	int font_size = (grid.Scale < 0.4) ? 10 :
	                (grid.Scale < 1.9) ? 14 : 18;
	fl_font(FL_COURIER, font_size);

	DrawEverything();

	fl_pop_clip();
}


int UI_Canvas::handle(int event)
{
	if (EV_HandleEvent(event))
		return 1;

	return Fl_Widget::handle(event);
}


void UI_Canvas::PointerPos(int *map_x, int *map_y, bool in_event)
{
	// NOTE: this fast method is disabled until behavior of the
	//       other method can be verified on all platforms....
#if 0
	if (in_event)
	{
		*map_x = MAPX(Fl::event_x());
		*map_y = MAPY(Fl::event_y());

		return;
	}
#endif

	// read current position outside of FLTK's event propagation.
	// this is a bit harder, and a bit slower in X-windows

	int raw_x, raw_y;

	Fl::get_mouse(raw_x, raw_y);

	raw_x -= main_win->x_root();
	raw_y -= main_win->y_root();

	*map_x = MAPX(raw_x);
	*map_y = MAPY(raw_y);
}


int UI_Canvas::ApproxBoxSize(int mx1, int my1, int mx2, int my2)
{
	if (mx2 < mx1) std::swap(mx1, mx2);
	if (my2 < my1) std::swap(my1, my2);

	int x1 = SCREENX(mx1);
	int x2 = SCREENX(mx2);

	int y1 = SCREENY(my2);
	int y2 = SCREENY(my1);

	if (x1 < 8 || x2 > w() - 8 ||
		y1 < 8 || y2 > h() - 8)
		return 1; // too big

	float x_ratio = MAX(4, x2 - x1) / (float) MAX(4, w());
	float y_ratio = MAX(4, y2 - y1) / (float) MAX(4, h());

	if (MAX(x_ratio, y_ratio) < 0.25)
		return -1;  // too small

	return 0;
}


//------------------------------------------------------------------------


void UI_Canvas::DrawEverything()
{
	map_lx = MAPX(x());
	map_ly = MAPY(y()+h());
	map_hx = MAPX(x()+w());
	map_hy = MAPY(y());

	// setup for drawing sector numbers
	if (edit.show_object_numbers && edit.mode == OBJ_SECTORS)
	{
		seen_sectors.clear_all();
	}

	DrawMap();

	DrawSelection(edit.Selected);

	if (edit.action == ACT_DRAG && edit.drag_single_obj < 0 && ! drag_lines.empty())
		DrawSelection(&drag_lines);
	else if (edit.action == ACT_TRANSFORM && ! trans_lines.empty())
		DrawSelection(&trans_lines);

	if (edit.action == ACT_DRAG && edit.drag_single_obj >= 0)
	{
		int dx = 0;
		int dy = 0;
		DragDelta(&dx, &dy);

		DrawHighlight(edit.mode, edit.drag_single_obj,
					  (edit.mode == OBJ_VERTICES) ? HI_AND_SEL_COL : HI_COL,
		              ! edit.error_mode /* do_tagged */, false /* skip_lines */,
					  dx, dy);

		if (edit.mode == OBJ_VERTICES && highlight.valid())
			DrawHighlight(highlight.type, highlight.num, HI_COL, false);
	}
	else if (highlight.valid())
	{
		Fl_Color hi_color = HI_COL;

		if (edit.Selected->get(highlight.num))
			hi_color = HI_AND_SEL_COL;

		DrawHighlight(highlight.type, highlight.num, hi_color,
		              ! edit.error_mode /* do_tagged */);
	}

	if (edit.action == ACT_SELBOX)
		SelboxDraw();

	if (edit.action == ACT_DRAW_LINE)
		DrawCurrentLine();
}


//
// draw the whole map, except for hilight/selection/selbox
//
void UI_Canvas::DrawMap()
{
	fl_color(FL_BLACK);
	fl_rectf(x(), y(), w(), h());

	if (edit.sector_render_mode)
	{
		for (int n = 0 ; n < NumSectors ; n++)
			RenderSector(n);
	}

	// draw the grid first since it's in the background
	if (grid.shown)
	{
		if (grid_style == 0)
			DrawGrid_Normal();
		else
			DrawGrid_Dotty();
	}

	if (Debugging)
		DrawMapBounds();

	DrawCamera();

	if (edit.mode != OBJ_THINGS)
		DrawThings();

	DrawLinedefs();

	if (edit.mode == OBJ_VERTICES)
		DrawVertices();

	if (edit.mode == OBJ_THINGS)
	{
		if (edit.thing_render_mode > 0)
		{
			DrawThings();
			DrawThingSprites();
		}
		else
		{
			DrawThingBodies();
			DrawThings();
		}
	}
}


//
//  draw the grid in the background of the edit window
//
void UI_Canvas::DrawGrid_Normal()
{
	float pixels_1 = grid.step * grid.Scale;


	if (pixels_1 < 1.6)
	{
		fl_color(DarkerColor(DarkerColor(normal_main_col)));
		fl_rectf(x(), y(), w(), h());

		DrawAxes(normal_axis_col);
		return;
	}


	int flat_step = 64;

	float pixels_2 = flat_step * grid.Scale;

	Fl_Color flat_col = (grid.step < 64) ? normal_main_col : normal_flat_col;

	if (pixels_2 < 2.2)
		flat_col = DarkerColor(flat_col);

	fl_color(flat_col);

	if (pixels_2 < 1.6)
	{
		fl_rectf(x(), y(), w(), h());
	}
	else
	{
		int gx = (map_lx / flat_step) * flat_step;

		for (; gx <= map_hx; gx += flat_step)
			DrawMapLine(gx, map_ly, gx, map_hy);

		int gy = (map_ly / flat_step) * flat_step;

		for (; gy <= map_hy; gy += flat_step)
			DrawMapLine(map_lx, gy, map_hx, gy);
	}


	Fl_Color main_col = (grid.step < 64) ? normal_small_col : normal_main_col;

	float pixels_3 = grid.step * grid.Scale;

	if (pixels_3 < 4.2)
		main_col = DarkerColor(main_col);

	fl_color(main_col);

	{
		int gx = (map_lx / grid.step) * grid.step;

		for (; gx <= map_hx; gx += grid.step)
			if ((grid.step >= 64 || (gx & 63) != 0) && (gx != 0))
				DrawMapLine(gx, map_ly, gx, map_hy);

		int gy = (map_ly / grid.step) * grid.step;

		for (; gy <= map_hy; gy += grid.step)
			if ((grid.step >= 64 || (gy & 63) != 0) && (gy != 0))
				DrawMapLine(map_lx, gy, map_hx, gy);
	}


	DrawAxes(normal_axis_col);
}


void UI_Canvas::DrawGrid_Dotty()
{
	int grid_step_1 = 1 * grid.step;    // Map units between dots
	int grid_step_2 = 8 * grid_step_1;  // Map units between dim lines
	int grid_step_3 = 8 * grid_step_2;  // Map units between bright lines

	float pixels_1 = grid.step * grid.Scale;


	if (pixels_1 < 1.6)
	{
		fl_color(DarkerColor(DarkerColor(dotty_point_col)));
		fl_rectf(x(), y(), w(), h());

		DrawAxes(dotty_axis_col);
		return;
	}


	fl_color(dotty_major_col);
	{
		int gx = (map_lx / grid_step_3) * grid_step_3;

		for (; gx <= map_hx; gx += grid_step_3)
			DrawMapLine(gx, map_ly-2, gx, map_hy+2);

		int gy = (map_ly / grid_step_3) * grid_step_3;

		for (; gy <=  map_hy; gy += grid_step_3)
			DrawMapLine(map_lx, gy, map_hx, gy);
	}


	DrawAxes(dotty_axis_col);


	fl_color(dotty_minor_col);
	{
		int gx = (map_lx / grid_step_2) * grid_step_2;

		for (; gx <= map_hx; gx += grid_step_2)
			if (gx % grid_step_3 != 0)
				DrawMapLine(gx, map_ly, gx, map_hy);

		int gy = (map_ly / grid_step_2) * grid_step_2;

		for (; gy <= map_hy; gy += grid_step_2)
			if (gy % grid_step_3 != 0)
				DrawMapLine(map_lx, gy, map_hx, gy);
	}


	if (pixels_1 < 4.02)
		fl_color(DarkerColor(dotty_point_col));
	else
		fl_color(dotty_point_col);

	{
		int gx = (map_lx / grid_step_1) * grid_step_1;
		int gy = (map_ly / grid_step_1) * grid_step_1;

		for (int ny = gy; ny <= map_hy; ny += grid_step_1)
		for (int nx = gx; nx <= map_hx; nx += grid_step_1)
		{
			int sx = SCREENX(nx);
			int sy = SCREENY(ny);

			if (pixels_1 < 24.1)
				fl_point(sx, sy);
			else
			{
				fl_rectf(sx, sy, 2, 2);

				// fl_line(sx-0, sy, sx+1, sy);
				// fl_line(sx, sy-0, sx, sy+1);
			}
		}
	}
}


void UI_Canvas::DrawAxes(Fl_Color col)
{
	fl_color(col);

	DrawMapLine(0, map_ly, 0, map_hy);

	DrawMapLine(map_lx, 0, map_hx, 0);
}


void UI_Canvas::DrawMapBounds()
{
	fl_color(FL_RED);

	DrawMapLine(Map_bound_x1, Map_bound_y1, Map_bound_x2, Map_bound_y1);
	DrawMapLine(Map_bound_x1, Map_bound_y2, Map_bound_x2, Map_bound_y2);

	DrawMapLine(Map_bound_x1, Map_bound_y1, Map_bound_x1, Map_bound_y2);
	DrawMapLine(Map_bound_x2, Map_bound_y1, Map_bound_x2, Map_bound_y2);
}


//
//  the apparent radius of a vertex, in pixels
//
int vertex_radius(double scale)
{
	int r = 6 * (0.26 + scale / 2);

	if (r > 12) r = 12;

	return r;
}



//
//  draw the vertices, and possibly their numbers
//
void UI_Canvas::DrawVertex(int map_x, int map_y, int r)
{
	int scrx = SCREENX(map_x);
	int scry = SCREENY(map_y);

// BLOBBY TEST
#if 0
	fl_line(scrx - 1, scry - 2, scrx + 1, scry - 2);
	fl_line(scrx - 2, scry - 1, scrx + 2, scry - 1);
	fl_line(scrx - 2, scry + 0, scrx + 2, scry + 0);
	fl_line(scrx - 2, scry + 1, scrx + 2, scry + 1);
	fl_line(scrx - 1, scry + 2, scrx + 1, scry + 2);
#else
	fl_line(scrx - r, scry - r, scrx + r, scry + r);
	fl_line(scrx + r, scry - r, scrx - r, scry + r);

	fl_line(scrx - 1, scry, scrx + 1, scry);
	fl_line(scrx, scry - 1, scrx, scry + 1);
#endif
}


void UI_Canvas::DrawVertices()
{
	const int r = vertex_radius(grid.Scale);

	fl_color(FL_GREEN);

	for (int n = 0 ; n < NumVertices ; n++)
	{
		int x = Vertices[n]->x;
		int y = Vertices[n]->y;

		if (Vis(x, y, r))
		{
			DrawVertex(x, y, r);
		}
	}

	if (edit.show_object_numbers)
	{
		for (int n = 0 ; n < NumVertices ; n++)
		{
			int x = Vertices[n]->x;
			int y = Vertices[n]->y;

			if (! Vis(x, y, r))
				continue;

			int sx = SCREENX(x) + r;
			int sy = SCREENY(y) - r - 2;

			DrawObjNum(sx, sy, n);
		}
	}
}


//
//  draw all the linedefs
//
void UI_Canvas::DrawLinedefs()
{
	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		int x1 = L->Start()->x;
		int y1 = L->Start()->y;
		int x2 = L->End  ()->x;
		int y2 = L->End  ()->y;

		if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
			continue;

		bool one_sided = (! L->Left());

		switch (edit.mode)
		{
			case OBJ_VERTICES:
			{
				if (n == split_ld)
					fl_color(HI_AND_SEL_COL);
				else if (edit.error_mode)
					fl_color(LIGHTGREY);
				else if (L->right < 0)
					fl_color(RED);
				else if (one_sided)
					fl_color(WHITE);
				else
					fl_color(LIGHTGREY);

				if (n == split_ld)
					DrawSplitLine(x1, y1, x2, y2);
				else
					DrawKnobbyLine(x1, y1, x2, y2);

				if (n != split_ld && n >= (NumLineDefs - 3) && ! edit.show_object_numbers)
				{
					DrawLineNumber(x1, y1, x2, y2, 0, I_ROUND(L->CalcLength()));
				}
			}
			break;

			case OBJ_LINEDEFS:
			{
				if (edit.error_mode)
					fl_color(LIGHTGREY);
				// no first sidedef?
				else if (! L->Right())
					fl_color(RED);
				else if (L->type != 0)
				{
					if (L->tag != 0)
						fl_color(LIGHTMAGENTA);
					else
						fl_color(LIGHTGREEN);
				}
				else if (one_sided)
					fl_color(WHITE);
				else if (L->flags & 1)
					fl_color(FL_CYAN);
				else
					fl_color(LIGHTGREY);

				DrawKnobbyLine(x1, y1, x2, y2);
			}
			break;

			case OBJ_SECTORS:
			{
				int sd1 = L->right;
				int sd2 = L->left;

				int s1  = (sd1 < 0) ? NIL_OBJ : SideDefs[sd1]->sector;
				int s2  = (sd2 < 0) ? NIL_OBJ : SideDefs[sd2]->sector;

				if (edit.error_mode)
					fl_color(LIGHTGREY);
				else if (sd1 < 0)
					fl_color(RED);
				else
				{
					bool have_tag  = false;
					bool have_type = false;

					if (Sectors[s1]->tag != 0)
						have_tag = true;
					if (Sectors[s1]->type != 0)
						have_type = true;

					if (s2 >= 0)
					{
						if (Sectors[s2]->tag != 0)
							have_tag = true;

						if (Sectors[s2]->type != 0)
							have_type = true;
					}

					if (have_tag && have_type)
						fl_color(SECTOR_TAGTYPE);
					else if (have_tag)
						fl_color(SECTOR_TAG);
					else if (have_type)
						fl_color(SECTOR_TYPE);
					else if (one_sided)
						fl_color(WHITE);
					else
						fl_color(LIGHTGREY);
				}

				DrawMapLine(x1, y1, x2, y2);

				if (edit.show_object_numbers)
				{
					if (s1 != NIL_OBJ)
						DrawSectorNum(x1, y1, x2, y2, SIDE_RIGHT, s1);

					if (s2 != NIL_OBJ)
						DrawSectorNum(x1, y1, x2, y2, SIDE_LEFT,  s2);
				}
			}
			break;

			// OBJ_THINGS
			default:
			{
				if (one_sided && ! edit.error_mode)
					fl_color(WHITE);
				else
					fl_color(LIGHTGREY);

				DrawMapLine(x1, y1, x2, y2);
			}
			break;
		}
	}

	// draw the linedef numbers
	if (edit.mode == OBJ_LINEDEFS && edit.show_object_numbers)
	{
		for (int n = 0 ; n < NumLineDefs ; n++)
		{
			int x1 = LineDefs[n]->Start()->x;
			int y1 = LineDefs[n]->Start()->y;
			int x2 = LineDefs[n]->End  ()->x;
			int y2 = LineDefs[n]->End  ()->y;

			if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
				continue;

			DrawLineNumber(x1, y1, x2, y2, 0, n);
		}
	}
}


void UI_Canvas::DrawThing(int x, int y, int r, int angle, bool big_arrow)
{
	DrawMapLine(x-r, y-r, x-r, y+r);
	DrawMapLine(x-r, y+r, x+r, y+r);
	DrawMapLine(x+r, y+r, x+r, y-r);
	DrawMapLine(x+r, y-r, x-r, y-r);

	if (big_arrow)
	{
		DrawMapArrow(x, y, r * 2, angle);
	}
	else
	{
		int dir = angle_to_direction(angle);

		static const short xsign[] = {  1,  1,  0, -1, -1, -1,  0,  1,  0 };
		static const short ysign[] = {  0,  1,  1,  1,  0, -1, -1, -1,  0 };

		int corner_x = r * xsign[dir];
		int corner_y = r * ysign[dir];

		DrawMapLine(x, y, x + corner_x, y + corner_y);
	}
}


//
//  draw things as squares (outlines)
//
void UI_Canvas::DrawThings()
{
	fl_color(DARKGREY);

	for (int n = 0 ; n < NumThings ; n++)
	{
		int x = Things[n]->x;
		int y = Things[n]->y;

		if (! Vis(x, y, MAX_RADIUS))
			continue;

		const thingtype_t *info = M_GetThingType(Things[n]->type);

		if (edit.mode == OBJ_THINGS)
		{
			if (edit.error_mode)
			{
				fl_color(LIGHTGREY);
			}
#if 0  // THIS DESIGNED TO SHOW SKILLS VIA COLOR, BUT DOESN'T WORK VERY WELL
			else if (true)
			{
				if (Things[n]->options & 1)
					fl_color (YELLOW);
				else if (Things[n]->options & 2)
					fl_color (LIGHTGREEN);
				else if (Things[n]->options & 4)
					fl_color (LIGHTRED);
				else
					fl_color (DARKGREY);
			}
#endif
			else
				fl_color((Fl_Color) info->color);
		}

		int r = info->radius;

		DrawThing(x, y, r, Things[n]->angle, false);
	}

	// draw the thing numbers
	if (edit.mode == OBJ_THINGS && edit.show_object_numbers)
	{
		for (int n = 0 ; n < NumThings ; n++)
		{
			int x = Things[n]->x;
			int y = Things[n]->y;

			if (! Vis(x, y, MAX_RADIUS))
				continue;

			const thingtype_t *info = M_GetThingType(Things[n]->type);

			x += info->radius;
			y += info->radius;

			DrawObjNum(SCREENX(x), SCREENY(y) - 2, n);
		}
	}
}


//
//  draw bodies of things (solid boxes, darker than the outline)
//
void UI_Canvas::DrawThingBodies()
{
	if (edit.error_mode)
		return;

	for (int n = 0 ; n < NumThings ; n++)
	{
		int x = Things[n]->x;
		int y = Things[n]->y;

		if (! Vis(x, y, MAX_RADIUS))
			continue;

		const thingtype_t *info = M_GetThingType(Things[n]->type);

		int r = info->radius;

		fl_color(DarkerColor(DarkerColor(info->color)));

		int sx1 = SCREENX(x - r);
		int sy1 = SCREENY(y + r);
		int sx2 = SCREENX(x + r);
		int sy2 = SCREENY(y - r);

		fl_rectf(sx1, sy1, sx2 - sx1 + 1, sy2 - sy1 + 1);
	}
}


void UI_Canvas::DrawThingSprites()
{
	for (int n = 0 ; n < NumThings ; n++)
	{
		int x = Things[n]->x;
		int y = Things[n]->y;

		if (! Vis(x, y, MAX_RADIUS))
			continue;

		const thingtype_t *info = M_GetThingType(Things[n]->type);

		Img_c *sprite = W_GetSprite(Things[n]->type);

		if (! sprite)
			sprite = IM_UnknownSprite();

		DrawSprite(x, y, sprite, info->scale);
	}
}


void UI_Canvas::DrawSprite(int map_x, int map_y, Img_c *img, float scale)
{
	int W = img->width();
	int H = img->height();

	scale = scale * 0.5;

	int bx1 = SCREENX(map_x - W * scale);
	int bx2 = SCREENX(map_x + W * scale);

	int by1 = SCREENY(map_y + H * scale);
	int by2 = SCREENY(map_y - H * scale);

	// prevent division by zero
	if (bx2 <= bx1) bx2 = bx1 + 1;
	if (by2 <= by1) by2 = by1 + 1;

	// clip to screen
	int sx1 = MAX(bx1, x());
	int sy1 = MAX(by1, y());

	int sx2 = MIN(bx2, x() + w());
	int sy2 = MIN(by2, y() + h());

	if (sy2 <= sy1 || sx2 <= sx1)
		return;

	// collect batches of pixels, it can greatly speed up rendering
	const int BATCH_MAX_LEN = 128;

	u8_t batch_rgb[BATCH_MAX_LEN * 3];
	u8_t *batch_dest = NULL;

	int  batch_len = 0;
	int  batch_sx  = 0;

	for (int sy = sy1 ; sy <= sy2 ; sy++)
	{
		batch_len = 0;

		for (int sx = sx1 ; sx <= sx2 ; sx++)
		{
			int ix = W * (sx - bx1) / (bx2 - bx1);
			int iy = H * (sy - by1) / (by2 - by1);

			ix = CLAMP(0, ix, W - 1);
			iy = CLAMP(0, iy, H - 1);

			img_pixel_t pix = img->buf()[iy * W + ix];

			if (pix == TRANS_PIXEL)
			{
				if (batch_len > 0)
				{
					fl_draw_image(batch_rgb, batch_sx, sy, batch_len, 1);
					batch_len = 0;
				}
				continue;
			}

			if (batch_len >= BATCH_MAX_LEN)
			{
				fl_draw_image(batch_rgb, batch_sx, sy, batch_len, 1);
				batch_len = 0;
			}

			if (batch_len == 0)
			{
				batch_sx = sx;
				batch_dest = batch_rgb;
			}

			IM_DecodePixel(pix, batch_dest[0], batch_dest[1], batch_dest[2]);

			batch_len++;
			batch_dest += 3;
		}

		if (batch_len > 0)
		{
			fl_draw_image(batch_rgb, batch_sx, sy, batch_len, 1);
		}
	}
}


void UI_Canvas::DrawSectorNum(int mx1, int my1, int mx2, int my2, int side, int n)
{
	// only draw a number for the first linedef actually visible
	if (seen_sectors.get(n))
		return;

	seen_sectors.set(n);

	DrawLineNumber(mx1, my1, mx2, my2, side, n);
}


void UI_Canvas::DrawLineNumber(int mx1, int my1, int mx2, int my2, int side, int n)
{
	int x1 = SCREENX(mx1);
	int y1 = SCREENY(my1);
	int x2 = SCREENX(mx2);
	int y2 = SCREENY(my2);

	int mx = (x1 + x2) / 2;
	int my = (y1 + y2) / 2 - 1;

	int dx = (y1 - y2);
	int dy = (x2 - x1);

	if (side == SIDE_LEFT)
	{
		dx = -dx;
		dy = -dy;
	}

	if (side)
	{
		int len = MAX(4, MAX(abs(dx), abs(dy)));
		int want_len = 4 + 10 * CLAMP(0.25, grid.Scale, 1.0);

		mx += dx * want_len / len;
		my += dy * want_len / len;

		if (abs(dx) > abs(dy))
		{
			want_len = 2 + want_len / 2;

			if (dx > 0)
				mx += want_len;
			else
				mx -= want_len;
		}
	}

	DrawObjNum(mx, my + fl_descent(), n, true /* center */);
}


//
//  draw a number at screen coordinates (x, y)
//
void UI_Canvas::DrawObjNum(int x, int y, int num, bool center)
{
	char buffer[64];
	sprintf(buffer, "%d", num);

	if (center)
	{
#if 0 /* DEBUG */
		fl_color(FL_RED);
		fl_rectf(x - 1, y - 1, 3, 3);
		return;
#endif
		x -= fl_width(buffer) / 2;
		y += fl_descent();
	}

	fl_color(FL_BLACK);

	fl_draw(buffer, x - 2, y);
	fl_draw(buffer, x - 1, y);
	fl_draw(buffer, x + 1, y);
	fl_draw(buffer, x + 2, y);
	fl_draw(buffer, x,     y + 1);
	fl_draw(buffer, x,     y - 1);

	fl_color(OBJECT_NUM_COL);

	fl_draw(buffer, x, y);
}


void UI_Canvas::HighlightSet(Objid& obj)
{
	if (highlight == obj)
		return;

	highlight = obj;
	redraw();
}


void UI_Canvas::HighlightForget()
{
	if (highlight.is_nil())
		return;

	highlight.clear();
	redraw();
}


void UI_Canvas::SplitLineSet(int ld, int new_x, int new_y)
{
	if (split_ld == ld && split_x == new_x && split_y == new_y)
		return;

	split_ld = ld;
	split_x  = new_x;
	split_y  = new_y;

	redraw();
}


void UI_Canvas::SplitLineForget()
{
	if (split_ld < 0)
		return;

	split_ld = -1;
	redraw();
}



//
//  draw the given object in highlight color
//
void UI_Canvas::DrawHighlight(int objtype, int objnum, Fl_Color col,
                              bool do_tagged, bool skip_lines, int dx, int dy)
{
	fl_color(col);

	// fprintf(stderr, "DrawHighlight: %d\n", objnum);

	int vert_r = vertex_radius(grid.Scale);

	switch (objtype)
	{
		case OBJ_THINGS:
		{
			int x = dx + Things[objnum]->x;
			int y = dy + Things[objnum]->y;

			if (! Vis(x, y, MAX_RADIUS))
				return;

			const thingtype_t *info = M_GetThingType(Things[objnum]->type);

			int r = info->radius;

			if (edit.error_mode)
				DrawThing(x, y, r, Things[objnum]->angle, false /* big_arrow */);

			r += r / 10 + 4;

			DrawThing(x, y, r, Things[objnum]->angle, true);
		}
		break;

		case OBJ_LINEDEFS:
		{
			// handle tagged linedefs : show matching sector(s)
			if (do_tagged && (dx==0 && dy==0) && LineDefs[objnum]->tag > 0)
			{
				for (int m = 0 ; m < NumSectors ; m++)
					if (Sectors[m]->tag == LineDefs[objnum]->tag)
						DrawHighlight(OBJ_SECTORS, m, LIGHTRED, false);

				fl_color(col);
			}

			int x1 = dx + LineDefs[objnum]->Start()->x;
			int y1 = dy + LineDefs[objnum]->Start()->y;
			int x2 = dx + LineDefs[objnum]->End  ()->x;
			int y2 = dy + LineDefs[objnum]->End  ()->y;

			if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
				return;

			DrawMapVector(x1, y1, x2, y2);
		}
		break;

		case OBJ_VERTICES:
		{
			int x = dx + Vertices[objnum]->x;
			int y = dy + Vertices[objnum]->y;

			if (! Vis(x, y, vert_r))
				return;

			DrawVertex(x, y, vert_r);

			int r = vert_r * 3 / 2;

			int sx1 = SCREENX(x) - r;
			int sy1 = SCREENY(y) - r;
			int sx2 = SCREENX(x) + r;
			int sy2 = SCREENY(y) + r;

			fl_line(sx1, sy1, sx2, sy1);
			fl_line(sx2, sy1, sx2, sy2);
			fl_line(sx2, sy2, sx1, sy2);
			fl_line(sx1, sy2, sx1, sy1);
		}
		break;

		case OBJ_SECTORS:
		{
			// handle tagged sectors : show matching line(s)
			if (do_tagged && (dx==0 && dy==0) && Sectors[objnum]->tag > 0)
			{
				for (int m = 0 ; m < NumLineDefs ; m++)
					if (LineDefs[m]->tag == Sectors[objnum]->tag)
						DrawHighlight(OBJ_LINEDEFS, m, LIGHTRED, false);

				fl_color(col);
			}

			fl_line_style(FL_SOLID, 2);

			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				const LineDef *L = LineDefs[n];

				if (! L->TouchesSector(objnum))
					continue;

				bool reverse = false;

				// skip lines if both sides are in the selection
				if (skip_lines && L->TwoSided())
				{
					int sec1 = L->Right()->sector;
					int sec2 = L->Left ()->sector;

					if ((sec1 == objnum || edit.Selected->get(sec1)) &&
					    (sec2 == objnum || edit.Selected->get(sec2)))
						continue;

					if (sec1 != objnum)
						reverse = true;
				}

				int x1 = dx + L->Start()->x;
				int y1 = dy + L->Start()->y;
				int x2 = dx + L->End  ()->x;
				int y2 = dy + L->End  ()->y;

				if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
					continue;

				if (skip_lines)
					DrawKnobbyLine(x1, y1, x2, y2, reverse);
				else
					DrawMapLine(x1, y1, x2, y2);
			}

			fl_line_style(FL_SOLID);
		}
		break;
	}
}


void UI_Canvas::DrawHighlightTransform(int objtype, int objnum, Fl_Color col)
{
	fl_color(col);

	int vert_r = vertex_radius(grid.Scale);

	switch (objtype)
	{
		case OBJ_THINGS:
		{
			int x = Things[objnum]->x;
			int y = Things[objnum]->y;

			trans_param.Apply(&x, &y);

			if (! Vis(x, y, MAX_RADIUS))
				return;

			const thingtype_t *info = M_GetThingType(Things[objnum]->type);

			int r = info->radius;

			DrawThing(x, y, r * 3 / 2, Things[objnum]->angle, true);
		}
		break;

		case OBJ_VERTICES:
		{
			int x = Vertices[objnum]->x;
			int y = Vertices[objnum]->y;

			trans_param.Apply(&x, &y);

			if (! Vis(x, y, vert_r))
				return;

			DrawVertex(x, y, vert_r);

			int r = vert_r * 3 / 2;

			int sx1 = SCREENX(x) - r;
			int sy1 = SCREENY(y) - r;
			int sx2 = SCREENX(x) + r;
			int sy2 = SCREENY(y) + r;

			fl_line(sx1, sy1, sx2, sy1);
			fl_line(sx2, sy1, sx2, sy2);
			fl_line(sx2, sy2, sx1, sy2);
			fl_line(sx1, sy2, sx1, sy1);
		}
		break;

		case OBJ_LINEDEFS:
		{
			int x1 = LineDefs[objnum]->Start()->x;
			int y1 = LineDefs[objnum]->Start()->y;
			int x2 = LineDefs[objnum]->End  ()->x;
			int y2 = LineDefs[objnum]->End  ()->y;

			trans_param.Apply(&x1, &y1);
			trans_param.Apply(&x2, &y2);

			if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
				return;

			DrawMapVector(x1, y1, x2, y2);
		}
		break;

		case OBJ_SECTORS:
		{
			fl_line_style(FL_SOLID, 2);

			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				if (! LineDefs[n]->TouchesSector(objnum))
					continue;

				int x1 = LineDefs[n]->Start()->x;
				int y1 = LineDefs[n]->Start()->y;
				int x2 = LineDefs[n]->End  ()->x;
				int y2 = LineDefs[n]->End  ()->y;

				trans_param.Apply(&x1, &y1);
				trans_param.Apply(&x2, &y2);

				if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
					continue;

				DrawMapLine(x1, y1, x2, y2);
			}

			fl_line_style(FL_SOLID);
		}
		break;
	}
}


//
//  draw selected objects in light blue
//
void UI_Canvas::DrawSelection(selection_c * list)
{
	if (! list || list->empty())
		return;

	selection_iterator_c it;

	if (edit.action == ACT_TRANSFORM)
	{
		for (list->begin(&it) ; !it.at_end() ; ++it)
		{
			DrawHighlightTransform(list->what_type(), *it, SEL_COL);
		}

		return;
	}

	int dx = 0;
	int dy = 0;

	if (edit.action == ACT_DRAG && edit.drag_single_obj < 0)
	{
		DragDelta(&dx, &dy);
	}

	for (list->begin(&it) ; !it.at_end() ; ++it)
	{
		DrawHighlight(list->what_type(), *it, edit.error_mode ? FL_RED : SEL_COL,
		              ! edit.error_mode /* do_tagged */,
					  true /* skip_lines */, dx, dy);
	}
}


//
//  draw a dot at map coordinates   [ NOT USED ATM ]
//
void UI_Canvas::DrawMapPoint(int map_x, int map_y)
{
    fl_point(SCREENX(map_x), SCREENY(map_y));
}


//
//  draw a plain line at the given map coords
//
void UI_Canvas::DrawMapLine(int map_x1, int map_y1, int map_x2, int map_y2)
{
    fl_line(SCREENX(map_x1), SCREENY(map_y1),
            SCREENX(map_x2), SCREENY(map_y2));
}


//
//  draw a line with a "knob" showing the right (front) side
//
void UI_Canvas::DrawKnobbyLine(int map_x1, int map_y1, int map_x2, int map_y2,
                               bool reverse)
{
	int x1 = SCREENX(map_x1);
	int y1 = SCREENY(map_y1);
	int x2 = SCREENX(map_x2);
	int y2 = SCREENY(map_y2);

	if (reverse)
	{
		std::swap(x1, x2);
		std::swap(y1, y2);
	}

    fl_line(x1, y1, x2, y2);

	// indicate direction of line
   	int mx = (x1 + x2) / 2;
   	int my = (y1 + y2) / 2;

	int dx = (y1 - y2);
	int dy = (x2 - x1);

	int len = MAX(4, MAX(abs(dx), abs(dy)));

	int want_len = MIN(12, len / 5);

	dx = dx * want_len / len;
	dy = dy * want_len / len;

	if (! (dx == 0 && dy == 0))
	{
		fl_line(mx, my, mx + dx, my + dy);
	}
}


void UI_Canvas::DrawSplitLine(int map_x1, int map_y1, int map_x2, int map_y2)
{
	// show how and where the line will be split

	int scr_x1 = SCREENX(map_x1);
	int scr_y1 = SCREENY(map_y1);
	int scr_x2 = SCREENX(map_x2);
	int scr_y2 = SCREENY(map_y2);

	int scr_mx = SCREENX(split_x);
	int scr_my = SCREENY(split_y);

	fl_line(scr_x1, scr_y1, scr_mx, scr_my);
	fl_line(scr_x2, scr_y2, scr_mx, scr_my);

	if (! edit.show_object_numbers)
	{
		int len1 = ComputeDist(map_x1 - split_x, map_y1 - split_y);
		int len2 = ComputeDist(map_x2 - split_x, map_y2 - split_y);

		DrawLineNumber(map_x1, map_y1, split_x, split_y, 0, len1);
		DrawLineNumber(map_x2, map_y2, split_x, split_y, 0, len2);
	}

	int size = (grid.Scale >= 5.0) ? 11 : (grid.Scale >= 1.0) ? 9 : 7;

	fl_color(HI_AND_SEL_COL);

	fl_pie(scr_mx - size/2, scr_my - size/2, size, size, 0, 360);
}


//
// draw a bolder linedef with an arrow on the end
// (used for highlighted / selected lines)
//
void UI_Canvas::DrawMapVector(int map_x1, int map_y1, int map_x2, int map_y2)
{
	int x1 = SCREENX(map_x1);
	int y1 = SCREENY(map_y1);
	int x2 = SCREENX(map_x2);
	int y2 = SCREENY(map_y2);

	int mx = (x1 + x2) / 2;
	int my = (y1 + y2) / 2;

	// knob
	fl_line(mx, my, mx + (y1 - y2) / 5, my + (x2 - x1) / 5);

	fl_line_style(FL_SOLID, 2);
	fl_line(x1, y1, x2, y2);

	double r2 = hypot((double) (x1 - x2), (double) (y1 - y2));

	if (r2 < 1.0)
		r2 = 1.0;

	double len = CLAMP(6.0, r2 / 10.0, 24.0);

	int dx = (int) (len * (x1 - x2) / r2);
	int dy = (int) (len * (y1 - y2) / r2);

	x1 = x2 + 2 * dx;
	y1 = y2 + 2 * dy;

	fl_line(x1 - dy, y1 + dx, x2, y2);
	fl_line(x1 + dy, y1 - dx, x2, y2);

	fl_line_style(FL_SOLID);
}


//
//  draw an arrow
//
void UI_Canvas::DrawMapArrow(int map_x1, int map_y1, int r, int angle)
{
	int map_x2 = map_x1 + r * cos(angle * M_PI / 180.0);
	int map_y2 = map_y1 + r * sin(angle * M_PI / 180.0);

	int x1 = SCREENX(map_x1);
	int y1 = SCREENY(map_y1);
	int x2 = SCREENX(map_x2);
	int y2 = SCREENY(map_y2);

	fl_line(x1, y1, x2, y2);

	double r2 = hypot((double) (x1 - x2), (double) (y1 - y2));

	if (r2 < 1.0)
		return;

	int dx = (int) ((x1 - x2) * 12.0 / (double)r2 * (grid.Scale / 2));
	int dy = (int) ((y1 - y2) * 12.0 / (double)r2 * (grid.Scale / 2));

	x1 = x2 + 2 * dx;
	y1 = y2 + 2 * dy;

	fl_line(x1 - dy, y1 + dx, x2, y2);
	fl_line(x1 + dy, y1 - dx, x2, y2);
}


void UI_Canvas::DrawCamera()
{
	int map_x, map_y;
	float angle;

	Render3D_GetCameraPos(&map_x, &map_y, &angle);

	int scr_x = SCREENX(map_x);
	int scr_y = SCREENY(map_y);

	float size = sqrt(grid.Scale) * 40;

	if (size < 8) size = 8;

	int dx = size *  cos(angle * M_PI / 180.0);
	int dy = size * -sin(angle * M_PI / 180.0);

	fl_color(CAMERA_COLOR);

	// arrow body

	fl_line(scr_x - dx, scr_y - dy, scr_x + dx, scr_y + dy);

	int ex =  dy/3;
	int ey = -dx/3;

	fl_line(scr_x + dx/8 + ex, scr_y + dy/8 + ey,
	        scr_x + dx/8 - ex, scr_y + dy/8 - ey);

	fl_line(scr_x - dx/8 + ex, scr_y - dy/8 + ey,
	        scr_x - dx/8 - ex, scr_y - dy/8 - ey);

	// arrow head

	scr_x += dx;
	scr_y += dy;

	int hx = dx/2;
	int hy = dy/2;

	fl_line(scr_x, scr_y, scr_x + hy - hx, scr_y - hx - hy);
	fl_line(scr_x, scr_y, scr_x - hy - hx, scr_y + hx - hy);
}


void UI_Canvas::DrawCurrentLine()
{
	if (edit.drawing_from < 0)
		return;

	int new_x = grid.SnapX(edit.map_x);
	int new_y = grid.SnapY(edit.map_y);

	// should draw a vertex?
	if (highlight.valid())
	{
		SYS_ASSERT(highlight.type == OBJ_VERTICES);

		new_x = Vertices[highlight.num]->x;
		new_y = Vertices[highlight.num]->y;
	}
	else if (split_ld < 0)
	{
		fl_color(FL_GREEN);

		DrawVertex(new_x, new_y, vertex_radius(grid.Scale));
	}

	fl_color(RED);

	const Vertex * v = Vertices[edit.drawing_from];

	DrawKnobbyLine(v->x, v->y, new_x, new_y);

	int dx = v->x - new_x;
	int dy = v->y - new_y;

	if (dx || dy)
	{
		float length = sqrt(dx * dx + dy * dy);

		DrawLineNumber(v->x, v->y, new_x, new_y, 0, I_ROUND(length));
	}
}


void UI_Canvas::SelboxBegin(int map_x, int map_y)
{
	selbox_x1 = selbox_x2 = map_x;
	selbox_y1 = selbox_y2 = map_y;
}

void UI_Canvas::SelboxUpdate(int map_x, int map_y)
{
	selbox_x2 = map_x;
	selbox_y2 = map_y;

	redraw();
}

void UI_Canvas::SelboxFinish(int *x1, int *y1, int *x2, int *y2)
{
	*x1 = MIN(selbox_x1, selbox_x2);
	*y1 = MIN(selbox_y1, selbox_y2);

	*x2 = MAX(selbox_x1, selbox_x2);
	*y2 = MAX(selbox_y1, selbox_y2);

	int scr_dx = SCREENX(*x2) - SCREENX(*x1);
	int scr_dy = SCREENY(*y1) - SCREENY(*y2);

	// small boxes should be treated as a click/release
	if (scr_dx < 10 && scr_dy < 10)  // TODO: CONFIG ITEM
	{
		*x2 = *x1;
		*y2 = *y1;
	}
}

void UI_Canvas::SelboxDraw()
{
	int x1 = MIN(selbox_x1, selbox_x2);
	int x2 = MAX(selbox_x1, selbox_x2);

	int y1 = MIN(selbox_y1, selbox_y2);
	int y2 = MAX(selbox_y1, selbox_y2);

	fl_color(FL_CYAN);

	DrawMapLine(x1, y1, x2, y1);
	DrawMapLine(x2, y1, x2, y2);
	DrawMapLine(x2, y2, x1, y2);
	DrawMapLine(x1, y2, x1, y1);
}


void UI_Canvas::DragBegin(int focus_x, int focus_y, int map_x, int map_y)
{
	drag_start_x = map_x;
	drag_start_y = map_y;

	drag_focus_x = focus_x;
	drag_focus_y = focus_y;

	drag_cur_x = drag_start_x;
	drag_cur_y = drag_start_y;

	if (edit.mode == OBJ_VERTICES)
	{
		drag_lines.change_type(OBJ_LINEDEFS);

		ConvertSelection(edit.Selected, &drag_lines);
	}
}

void UI_Canvas::DragFinish(int *dx, int *dy)
{
	drag_lines.clear_all();

	DragDelta(dx, dy);
}

void UI_Canvas::DragUpdate(int map_x, int map_y)
{
	drag_cur_x = map_x;
	drag_cur_y = map_y;

	redraw();
}

void UI_Canvas::DragDelta(int *dx, int *dy)
{
	*dx = drag_cur_x - drag_start_x;
	*dy = drag_cur_y - drag_start_y;

	if (grid.snap)
	{
		int focus_x = drag_focus_x + *dx;
		int focus_y = drag_focus_y + *dy;

		*dx = grid.SnapX(focus_x) - drag_focus_x;
		*dy = grid.SnapY(focus_y) - drag_focus_y;
	}
}


void UI_Canvas::TransformBegin(int map_x, int map_y, int middle_x, int middle_y,
							   transform_keyword_e mode)
{
	trans_start_x = map_x;
	trans_start_y = map_y;

	trans_mode = mode;

	trans_param.Clear();

	trans_param.mid_x = middle_x;
	trans_param.mid_y = middle_y;

	if (edit.mode == OBJ_VERTICES)
	{
		trans_lines.change_type(OBJ_LINEDEFS);

		ConvertSelection(edit.Selected, &trans_lines);
	}
}

void UI_Canvas::TransformFinish(transform_t& param)
{
	trans_lines.clear_all();

	param = trans_param;
}

void UI_Canvas::TransformUpdate(int map_x, int map_y)
{
	int dx1 = map_x - trans_param.mid_x;
	int dy1 = map_y - trans_param.mid_y;

	int dx0 = trans_start_x - trans_param.mid_x;
	int dy0 = trans_start_y - trans_param.mid_y;

	trans_param.scale_x = trans_param.scale_y = 1;
	trans_param.skew_x  = trans_param.skew_y  = 0;
	trans_param.rotate  = 0;

	if (trans_mode == TRANS_K_Rotate || trans_mode == TRANS_K_RotScale)
	{
		int angle1 = (int)ComputeAngle(dx1, dy1);
		int angle0 = (int)ComputeAngle(dx0, dy0);

		trans_param.rotate = angle1 - angle0;

//		fprintf(stderr, "angle diff : %1.2f\n", trans_rotate * 360.0 / 65536.0);
	}

	switch (trans_mode)
	{
		case TRANS_K_Scale:
		case TRANS_K_RotScale:
			dx1 = MAX(abs(dx1), abs(dy1));
			dx0 = MAX(abs(dx0), abs(dy0));

			if (dx0)
			{
				trans_param.scale_x = dx1 / (float)dx0;
				trans_param.scale_y = trans_param.scale_x;
			}
			break;

		case TRANS_K_Stretch:
			if (dx0) trans_param.scale_x = dx1 / (float)dx0;
			if (dy0) trans_param.scale_y = dy1 / (float)dy0;
			break;

		case TRANS_K_Rotate:
			// already done
			break;

		case TRANS_K_Skew:
			if (abs(dx0) >= abs(dy0))
			{
				if (dx0) trans_param.skew_y = (dy1 - dy0) / (float)dx0;
			}
			else
			{
				if (dy0) trans_param.skew_x = (dx1 - dx0) / (float)dy0;
			}
			break;
	}

	redraw();
}


//------------------------------------------------------------------------


// this represents a segment of a linedef bounding a sector.
struct sector_edge_t
{
	const LineDef * line;

	// coordinates mapped to screen space, not clipped
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

	void CalcX(short y)
	{
		x = scr_x1 + (scr_x2 - scr_x1) * (float)(y - scr_y1) / (float)(scr_y2 - scr_y1);
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


void UI_Canvas::RenderSector(int num)
{
///  fprintf(stderr, "RenderSector %d\n", num);

	rgb_color_t light_col = SectorLightColor(Sectors[num]->light);

	const char * tex_name = NULL;

	Img_c * img = NULL;

	if (edit.sector_render_mode == SREND_Lighting)
	{
		fl_color(light_col);
	}
	else
	{
		if (edit.sector_render_mode == SREND_Ceiling)
			tex_name = Sectors[num]->CeilTex();
		else
			tex_name = Sectors[num]->FloorTex();

		if (is_sky(tex_name))
		{
			fl_color(palette[game_info.sky_color]);
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

	const img_pixel_t *src_pix = img ? img->buf() : NULL;

	int tw = img ? img->width()  : 1;
	int th = img ? img->height() : 1;

	// verify size is at least 64x64
	if (img && (tw < 64 || th < 64))
	{
		fl_color(palette[game_info.missing_color]);

		img = NULL;
	}


	/*** Part 1 : visit linedefs and create edges ***/


	std::vector<sector_edge_t> edgelist;

	short min_y = 32767;
	short max_y = 0;


	for (int n = 0 ; n < NumLineDefs ; n++)
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
		if (MAX(edge.scr_y1, edge.scr_y2) < y())
			continue;

		if (MIN(edge.scr_y1, edge.scr_y2) >= y() + h())
			continue;

		// skip horizontal lines
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
		edge.y1 = MAX(edge.scr_y1, y());
		edge.y2 = MIN(edge.scr_y2, y() + h() - 1);

		// this probably cannot happen....
		if (edge.y2 < edge.y1)
			continue;

		min_y = MIN(min_y, edge.y1);
		max_y = MAX(max_y, edge.y2);

		// compute side
		bool is_right = (L->WhatSector(SIDE_LEFT) == num);

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


	/*** Part 2 : traverse edge list and render spans ***/


	unsigned int next_edge = 0;

	u8_t * line_rgb = new u8_t[3 * (w() + 4)];

	std::vector<sector_edge_t *> active_edges;

	unsigned int i;

	// visit each screen row
	for (short y = min_y ; y <= max_y ; y++)
	{
		// remove old edges from active list
		for (i = 0 ; i < active_edges.size() ; i++)
		{
			if (y > active_edges[i]->y2)
				active_edges[i] = NULL;
		}

		// add new edges from active list
		for ( ; next_edge < edgelist.size() && y == edgelist[next_edge].y1 ; next_edge++)
		{
			active_edges.push_back(&edgelist[next_edge]);
		}

///  fprintf(stderr, "  active @ y=%d --> %d\n", y, (int)active_edges.size());

		if (active_edges.empty())
			continue;

		// sort active edges by X value
		// [ also puts NULL entries at end, making easy to remove them ]

		for (i = 0 ; i < active_edges.size() ; i++)
		{
			if (active_edges[i])
				active_edges[i]->CalcX(y);
		}

		std::sort(active_edges.begin(), active_edges.end(), sector_edge_t::CMP_X());

		while (active_edges.size() > 0 && active_edges.back() == NULL)
			active_edges.pop_back();

		// compute spans

		for (unsigned int i = 1 ; i < active_edges.size() ; i++)
		{
			const sector_edge_t * E1 = active_edges[i - 1];
			const sector_edge_t * E2 = active_edges[i];
#if 1
			if (E1 == NULL || E2 == NULL)
				BugError("RenderSector: did not delete NULLs properly!\n");
#endif

///  fprintf(stderr, "E1 @ x=%1.2f side=%d  |  E2 @ x=%1.2f side=%d\n",
///  E1->x, E1->side, E2->x, E2->side);

			if (! (E1->side == SIDE_RIGHT && E2->side == SIDE_LEFT))
				continue;

			// treat lines without a right side as dead
			if (E1->line->right < 0) continue;
			if (E2->line->right < 0) continue;

			int x1 = floor(E1->x);
			int x2 = floor(E2->x);

			// completely off the screen?
			if (x2 < x() || x1 >= x() + w())
				continue;

			// clip span to screen
			x1 = MAX(x1, x());
			x2 = MIN(x2, x() + w() - 1);

			// this probably cannot happen....
			if (x2 < x1) continue;

///  fprintf(stderr, "  span : y=%d  x=%d..%d\n", y, x1, x2);

			// solid color?
			if (! img)
			{
				fl_rectf(x1, y, x2 - x1 + 1, 1);
				continue;
			}

			int x = x1;
			int span_w = x2 - x1 + 1;

			u8_t *dest = line_rgb;
			u8_t *dest_end = line_rgb + span_w * 3;

			int ty = (0 - MAPY(y)) & 63;

			for (; dest < dest_end ; dest += 3, x++)
			{
				// TODO : be nice to optimize the next line
				int tx = MAPX(x) & 63;

				img_pixel_t pix = src_pix[ty * tw + tx];

				IM_DecodePixel(pix, dest[0], dest[1], dest[2]);
			}

			fl_draw_image(line_rgb, x1, y, span_w, 1);
		}
	}

	delete[] line_rgb;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
