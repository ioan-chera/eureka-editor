//------------------------------------------------------------------------
//  EDITING CANVAS
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

#ifndef NO_OPENGL
#include "FL/gl.h"
#endif

#include "ui_window.h"

#include "m_events.h"
#include "e_main.h"
#include "e_hover.h"
#include "e_sector.h"
#include "e_things.h"
#include "e_path.h"	  // SoundPropagation
#include "m_config.h"
#include "m_game.h"
#include "r_grid.h"
#include "r_subdiv.h"
#include "im_color.h"
#include "im_img.h"
#include "r_render.h"
#include "w_rawdef.h"	// MLF_xxx
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


// compatibility defines for software rendering
#ifdef NO_OPENGL
#define gl_color    fl_color
#define gl_rectf    fl_rectf
#define gl_line     fl_line
#define gl_font     fl_font
#define gl_width    fl_width
#define gl_height   fl_height
#define gl_descent  fl_descent
#endif


int vertex_radius(double scale);


UI_Canvas::UI_Canvas(int X, int Y, int W, int H, const char *label) :
#ifdef NO_OPENGL
	Fl_Widget(X, Y, W, H, label),
#else
	Fl_Gl_Window(X, Y, W, H),
#endif
	highlight(), split_ld(-1),
	drag_lines(),
	trans_lines(),
	seen_sectors()
{ }


UI_Canvas::~UI_Canvas()
{ }


void UI_Canvas::DeleteContext()
{
#ifndef NO_OPENGL
	context(NULL, 0);

	// ensure W_UnloadAllTextures() gets called on next draw()
	invalidate();
#endif
}


void UI_Canvas::resize(int X, int Y, int W, int H)
{
#ifdef NO_OPENGL
	Fl_Widget::resize(X, Y, W, H);
#else
	Fl_Gl_Window::resize(X, Y, W, H);
#endif
}


void UI_Canvas::draw()
{
#ifdef NO_OPENGL
	xx = x();
	yy = y();

	fl_push_clip(x(), y(), w(), h());

	map_lx = floor(MAPX(xx));
	map_ly = floor(MAPY(yy + h()));

	map_hx = ceil(MAPX(xx + w()));
	map_hy = ceil(MAPY(yy));
#else
	xx = yy = 0;

	map_lx = floor(MAPX(0));
	map_ly = floor(MAPY(0));

	map_hx = ceil(MAPX(w()));
	map_hy = ceil(MAPY(h()));

	if (! valid())
	{
		// setup projection matrix for 2D drawing
		ortho();

		// reset the 'gl_tex' field of all loaded images, as the value
		// belongs to a context which was (probably) just deleted and
		// hence refer to textures which no longer exist.
		W_UnloadAllTextures();
	}
#endif

	gl_color(FL_WHITE);

	// must always set the line style *after* the color.
	// [ see FLTK docs for details ]
	gl_line_width(1);

	// default font (for showing object numbers)
	int font_size = (grid.Scale < 0.4) ? 10 :
	                (grid.Scale < 1.9) ? 14 : 18;
	gl_font(FL_COURIER, font_size);

	DrawEverything();

#ifdef NO_OPENGL
	fl_pop_clip();
#endif
}


int UI_Canvas::handle(int event)
{
	if (EV_HandleEvent(event))
		return 1;

	return Fl_Widget::handle(event);
}


#ifndef NO_OPENGL
void UI_Canvas::gl_line(int x1, int y1, int x2, int y2)
{
	glBegin(GL_LINE_STRIP);
	glVertex2i(x1, y1);
	glVertex2i(x2, y2);
	glEnd();
}
#endif


void UI_Canvas::gl_line_width(int w)
{
#ifdef NO_OPENGL
	// apparently 0 produces nicer results than 1
	if (w == 1)
		w = 0;

	fl_line_style(FL_SOLID, w);
#else
	glLineWidth(w);
#endif
}


void UI_Canvas::gl_draw_string(const char *s, int x, int y)
{
#ifdef NO_OPENGL
	fl_draw(s, x, y);
#else
	gl_draw(s, x, y);
#endif
}


int UI_Canvas::NORMALX(int len, int dx, int dy)
{
#ifdef NO_OPENGL
	float res = -dy;
#else
	float res = dy;
#endif

	float got_len = hypotf(dx, dy);
	if (got_len < 0.01)
		return 0;

	return I_ROUND(res * len / got_len);
}

int UI_Canvas::NORMALY(int len, int dx, int dy)
{
#ifdef NO_OPENGL
	float res = dx;
#else
	float res = -dx;
#endif

	float got_len = hypotf(dx, dy);
	if (got_len < 0.01)
		return 0;

	return I_ROUND(res * len / got_len);
}


void UI_Canvas::PointerPos(bool in_event)
{
	// read current position outside of FLTK's event propagation.
	// this is a bit harder, and a bit slower in X-windows

	int raw_x, raw_y;

	Fl::get_mouse(raw_x, raw_y);

#ifdef NO_OPENGL
	raw_x -= main_win->x_root();
	raw_y -= main_win->y_root();

	edit.map_x = MAPX(raw_x);
	edit.map_y = MAPY(raw_y);

#else // OpenGL
	raw_x -= x_root();
	raw_y -= y_root();

	edit.map_x = MAPX(raw_x);
	edit.map_y = MAPY(h() - raw_y);
#endif
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

		if (edit.mode == OBJ_VERTICES)
			gl_color(HI_AND_SEL_COL);
		else
			gl_color(HI_COL);

		if (edit.mode == OBJ_LINEDEFS || edit.mode == OBJ_SECTORS)
			gl_line_width(2);

		DrawHighlight(edit.mode, edit.drag_single_obj,
		              false /* skip_lines */, dx, dy);

		if (edit.mode == OBJ_VERTICES && highlight.valid())
		{
			gl_color(HI_COL);
			DrawHighlight(highlight.type, highlight.num);
		}

		gl_line_width(1);
	}
	else if (highlight.valid())
	{
		if (edit.Selected->get(highlight.num))
			gl_color(HI_AND_SEL_COL);
		else
			gl_color(HI_COL);

		if (highlight.type == OBJ_LINEDEFS || highlight.type == OBJ_SECTORS)
			gl_line_width(2);

		DrawHighlight(highlight.type, highlight.num);

		gl_color(LIGHTRED);

		if (! edit.error_mode)
			DrawTagged(highlight.type, highlight.num);

		gl_line_width(1);
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
	gl_color(FL_BLACK);
	gl_rectf(xx, yy, w(), h());

	if (edit.sector_render_mode && ! edit.error_mode)
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
		gl_color(DarkerColor(DarkerColor(normal_main_col)));
		gl_rectf(xx, yy, w(), h());

		DrawAxes(normal_axis_col);
		return;
	}


	int flat_step = 64;

	float pixels_2 = flat_step * grid.Scale;

	Fl_Color flat_col = (grid.step < 64) ? normal_main_col : normal_flat_col;

	if (pixels_2 < 2.2)
		flat_col = DarkerColor(flat_col);

	gl_color(flat_col);

	if (pixels_2 < 1.6)
	{
		gl_rectf(xx, yy, w(), h());
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

	gl_color(main_col);

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
		gl_color(DarkerColor(DarkerColor(dotty_point_col)));
		gl_rectf(xx, yy, w(), h());

		DrawAxes(dotty_axis_col);
		return;
	}


	gl_color(dotty_major_col);
	{
		int gx = (map_lx / grid_step_3) * grid_step_3;

		for (; gx <= map_hx; gx += grid_step_3)
			DrawMapLine(gx, map_ly-2, gx, map_hy+2);

		int gy = (map_ly / grid_step_3) * grid_step_3;

		for (; gy <=  map_hy; gy += grid_step_3)
			DrawMapLine(map_lx, gy, map_hx, gy);
	}


	DrawAxes(dotty_axis_col);


	gl_color(dotty_minor_col);
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
		gl_color(DarkerColor(dotty_point_col));
	else
		gl_color(dotty_point_col);

	{
		int gx = (map_lx / grid_step_1) * grid_step_1;
		int gy = (map_ly / grid_step_1) * grid_step_1;

		for (int ny = gy; ny <= map_hy; ny += grid_step_1)
		for (int nx = gx; nx <= map_hx; nx += grid_step_1)
		{
			int sx = SCREENX(nx);
			int sy = SCREENY(ny);

			if (pixels_1 < 24.1)
				gl_rectf(sx, sy, 1, 1);
			else
				gl_rectf(sx, sy, 2, 2);
		}
	}
}


void UI_Canvas::DrawAxes(Fl_Color col)
{
	gl_color(col);

	DrawMapLine(0, map_ly, 0, map_hy);

	DrawMapLine(map_lx, 0, map_hx, 0);
}


void UI_Canvas::DrawMapBounds()
{
	gl_color(FL_RED);

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
	gl_line(scrx - 1, scry - 2, scrx + 1, scry - 2);
	gl_line(scrx - 2, scry - 1, scrx + 2, scry - 1);
	gl_line(scrx - 2, scry + 0, scrx + 2, scry + 0);
	gl_line(scrx - 2, scry + 1, scrx + 2, scry + 1);
	gl_line(scrx - 1, scry + 2, scrx + 1, scry + 2);
#else
	gl_line(scrx - r, scry - r, scrx + r, scry + r);
	gl_line(scrx + r, scry - r, scrx - r, scry + r);

	gl_line(scrx - 1, scry, scrx + 1, scry);
	gl_line(scrx, scry - 1, scrx, scry + 1);
#endif
}


void UI_Canvas::DrawVertices()
{
	const int r = vertex_radius(grid.Scale);

	gl_color(FL_GREEN);

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

			int sx = SCREENX(x) + r * 3;
			int sy = SCREENY(y) + r * 3;

			DrawObjNum(sx, sy, n);
		}
	}
}


//
//  draw all the linedefs
//
void UI_Canvas::DrawLinedefs()
{
	// we perform 5 passes over the lines.  the first four use a single
	// color specific to the pass (stored in colors[] array).  the 5th
	// pass is for all other colors.
	//
	// the REASON for this complexity is because fl_color() under X windows
	// is very very slow, and was causing a massive slow down of this code.
	// even detecting the same color as last time and inhibiting the call
	// did not help.

	Fl_Color colors[4];

	colors[0] = LIGHTGREY;
	colors[1] = LIGHTMAGENTA;
	colors[2] = LIGHTGREEN;
	colors[3] = WHITE;

	if (edit.mode == OBJ_SECTORS)
	{
		colors[1] = SECTOR_TAG;
		colors[2] = SECTOR_TYPE;

		if (edit.sector_render_mode == SREND_SoundProp)
			colors[1] = FL_MAGENTA;
	}

	for (int pass = 0 ; pass < 5 ; pass++)
	{
		if (pass < 4)
			gl_color(colors[pass]);

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

			Fl_Color col = LIGHTGREY;

			// 'p' for plain, 'k' for knobbly, 's' for split
			char line_kind = 'p';

			switch (edit.mode)
			{
				case OBJ_VERTICES:
				{
					if (n == split_ld)
						col = HI_AND_SEL_COL;
					else if (edit.error_mode)
						col = LIGHTGREY;
					else if (L->right < 0)
						col = RED;
					else if (one_sided)
						col = WHITE;

					if (n == split_ld)
						line_kind = 's';
					else
						line_kind = 'k';

					if (pass==4 && n != split_ld && n >= (NumLineDefs - 3) && !edit.show_object_numbers)
					{
						DrawLineNumber(x1, y1, x2, y2, 0, I_ROUND(L->CalcLength()));
					}
				}
				break;

				case OBJ_LINEDEFS:
				{
					if (edit.error_mode)
						col = LIGHTGREY;
					else if (! L->Right()) // no first sidedef?
						col = RED;
					else if (L->type != 0)
					{
						if (L->tag != 0)
							col = LIGHTMAGENTA;
						else
							col = LIGHTGREEN;
					}
					else if (one_sided)
						col = WHITE;
					else if (L->flags & MLF_Blocking)
						col = FL_CYAN;

					line_kind = 'k';
				}
				break;

				case OBJ_SECTORS:
				{
					int sd1 = L->right;
					int sd2 = L->left;

					int s1  = (sd1 < 0) ? NIL_OBJ : SideDefs[sd1]->sector;
					int s2  = (sd2 < 0) ? NIL_OBJ : SideDefs[sd2]->sector;

					if (edit.error_mode)
						col = LIGHTGREY;
					else if (sd1 < 0)
						col = RED;
					else if (edit.sector_render_mode == SREND_SoundProp)
					{
						if (L->flags & MLF_SoundBlock)
							col = FL_MAGENTA;
						else if (one_sided)
							col = WHITE;
					}
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
							col = SECTOR_TAGTYPE;
						else if (have_tag)
							col = SECTOR_TAG;
						else if (have_type)
							col = SECTOR_TYPE;
						else if (one_sided)
							col = WHITE;
					}

					if (pass==4 && edit.show_object_numbers)
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
						col = WHITE;
				}
				break;
			}

			if (pass < 4)
			{
				if (col != colors[pass])
					continue;
			}
			else
			{
				if (col == colors[0] || col == colors[1] ||
					col == colors[2] || col == colors[3])
					continue;

				gl_color(col);
			}

			switch (line_kind)
			{
				case 'p':
					DrawMapLine(x1, y1, x2, y2);
					break;

				case 'k':
					DrawKnobbyLine(x1, y1, x2, y2);
					break;

				case 's':
					DrawSplitLine(x1, y1, x2, y2);
					break;
			}
		} // n
	} // pass

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
	if (edit.mode != OBJ_THINGS)
		gl_color(DARKGREY);
	else if (edit.error_mode)
		gl_color(LIGHTGREY);
	else
		gl_color((Fl_Color)0xFF000000);

	// see notes in DrawLinedefs() on why we perform multiple passes.
	// here first pass is bright red, second pass is everything else.

	for (int pass = 0 ; pass < 2 ; pass++)
	for (int n = 0 ; n < NumThings ; n++)
	{
		int x = Things[n]->x;
		int y = Things[n]->y;

		if (! Vis(x, y, MAX_RADIUS))
			continue;

		const thingtype_t *info = M_GetThingType(Things[n]->type);

		if (edit.mode == OBJ_THINGS && !edit.error_mode)
		{
			Fl_Color col = (Fl_Color)info->color;

			if (pass == 0)
			{
				if (col != 0xFF000000)
					continue;
			}
			else
			{
				if (col == 0xFF000000)
					continue;

				gl_color(col);
			}
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

			x += info->radius + 8;
			y += info->radius + 8;

			DrawObjNum(SCREENX(x), SCREENY(y), n);
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

	// see notes in DrawLinedefs() on why we perform multiple passes.
	// here first pass is dark red, second pass is everything else.

	gl_color(DarkerColor(DarkerColor((Fl_Color)0xFF000000)));

	for (int pass = 0 ; pass < 2 ; pass++)
	for (int n = 0 ; n < NumThings ; n++)
	{
		int x = Things[n]->x;
		int y = Things[n]->y;

		if (! Vis(x, y, MAX_RADIUS))
			continue;

		const thingtype_t *info = M_GetThingType(Things[n]->type);

		int r = info->radius;

		Fl_Color col = (Fl_Color)info->color;

		if (pass == 0)
		{
			if (col != 0xFF000000)
				continue;
		}
		else
		{
			if (col == 0xFF000000)
				continue;

			gl_color(DarkerColor(DarkerColor(col)));
		}

		int sx1 = SCREENX(x - r);
		int sy1 = SCREENY(y + r);
		int sx2 = SCREENX(x + r);
		int sy2 = SCREENY(y - r);

		gl_rectf(sx1, sy1, sx2 - sx1 + 1, sy2 - sy1 + 1);
	}
}


void UI_Canvas::DrawThingSprites()
{
#ifndef NO_OPENGL
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);

	glAlphaFunc(GL_GREATER, 0.5);
#endif

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

#ifndef NO_OPENGL
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
#endif
}


void UI_Canvas::DrawSprite(int map_x, int map_y, Img_c *img, float scale)
{
	int W = img->width();
	int H = img->height();

	scale = scale * 0.5;

#ifdef NO_OPENGL
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

#else // OpenGL
	int bx1 = SCREENX(map_x - W * scale);
	int bx2 = SCREENX(map_x + W * scale);

	int by1 = SCREENY(map_y - H * scale);
	int by2 = SCREENY(map_y + H * scale);

	// don't make too small
	if (bx2 <= bx1) bx2 = bx1 + 1;
	if (by2 <= by1) by2 = by1 + 1;

	// bind the sprite image (upload it to OpenGL if needed)
	img->bind_gl();

	// choose texture coords based on image size
	float tx1 = 0.0;
	float ty1 = 0.0;
	float tx2 = (float)img->width()  / (float)RoundPOW2(img->width());
	float ty2 = (float)img->height() / (float)RoundPOW2(img->height());

	glColor3f(1, 1, 1);

	glBegin(GL_QUADS);

	glTexCoord2f(tx1, ty1); glVertex2i(bx1, by1);
	glTexCoord2f(tx1, ty2); glVertex2i(bx1, by2);
	glTexCoord2f(tx2, ty2); glVertex2i(bx2, by2);
	glTexCoord2f(tx2, ty1); glVertex2i(bx2, by1);

	glEnd();
#endif
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

	int sx = (x1 + x2) / 2;
	int sy = (y1 + y2) / 2;

	// normally draw line numbers on back of line
	int want_len = -16 * CLAMP(0.25, grid.Scale, 1.0);

	// for sectors, draw closer and on sector side
	if (side != 0)
	{
		want_len = 2 + 12 * CLAMP(0.25, grid.Scale, 1.0);

		if (side == SIDE_LEFT)
			want_len = -want_len;
	}

	sx += NORMALX(want_len*2, x2 - x1, y2 - y1);
	sy += NORMALY(want_len,   x2 - x1, y2 - y1);

	DrawObjNum(sx, sy, n);
}


//
//  draw a number centered at screen coordinate (x, y)
//
void UI_Canvas::DrawObjNum(int x, int y, int num)
{
	char buffer[64];
	sprintf(buffer, "%d", num);

#if 0 /* DEBUG */
	gl_color(FL_RED);
	gl_rectf(x - 1, y - 1, 3, 3);
	return;
#endif
	x -= gl_width(buffer) / 2;

#ifdef NO_OPENGL
	y += gl_height() / 2;
#else
	y -= gl_height() / 2;
#endif

	gl_color(FL_BLACK);

	gl_draw_string(buffer, x - 2, y);
	gl_draw_string(buffer, x - 1, y);
	gl_draw_string(buffer, x + 1, y);
	gl_draw_string(buffer, x + 2, y);
	gl_draw_string(buffer, x,     y + 1);
	gl_draw_string(buffer, x,     y - 1);

	gl_color(OBJECT_NUM_COL);

	gl_draw_string(buffer, x, y);
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
void UI_Canvas::DrawHighlight(int objtype, int objnum,
                              bool skip_lines, int dx, int dy)
{
	// gl_color() and gl_line_width() has been done by caller

	// fprintf(stderr, "DrawHighlight: %d\n", objnum);

	switch (objtype)
	{
		case OBJ_THINGS:
		{
			int x = dx + Things[objnum]->x;
			int y = dy + Things[objnum]->y;

			if (! Vis(x, y, MAX_RADIUS))
				break;

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
			int x1 = dx + LineDefs[objnum]->Start()->x;
			int y1 = dy + LineDefs[objnum]->Start()->y;
			int x2 = dx + LineDefs[objnum]->End  ()->x;
			int y2 = dy + LineDefs[objnum]->End  ()->y;

			if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
				break;

			DrawMapVector(x1, y1, x2, y2);
		}
		break;

		case OBJ_VERTICES:
		{
			int x = dx + Vertices[objnum]->x;
			int y = dy + Vertices[objnum]->y;

			int vert_r = vertex_radius(grid.Scale);

			if (! Vis(x, y, vert_r))
				break;

			DrawVertex(x, y, vert_r);

			int r = vert_r * 3 / 2;

			int sx1 = SCREENX(x) - r;
			int sy1 = SCREENY(y) - r;
			int sx2 = SCREENX(x) + r;
			int sy2 = SCREENY(y) + r;

			gl_line(sx1, sy1, sx2, sy1);
			gl_line(sx2, sy1, sx2, sy2);
			gl_line(sx2, sy2, sx1, sy2);
			gl_line(sx1, sy2, sx1, sy1);
		}
		break;

		case OBJ_SECTORS:
		{
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
		}
		break;
	}
}


void UI_Canvas::DrawHighlightTransform(int objtype, int objnum)
{
	// gl_color() and gl_line_width() has been done by caller

	switch (objtype)
	{
		case OBJ_THINGS:
		{
			int x = Things[objnum]->x;
			int y = Things[objnum]->y;

			trans_param.Apply(&x, &y);

			if (! Vis(x, y, MAX_RADIUS))
				break;

			const thingtype_t *info = M_GetThingType(Things[objnum]->type);

			int r = info->radius;

			DrawThing(x, y, r * 3 / 2, Things[objnum]->angle, true);
		}
		break;

		case OBJ_VERTICES:
		{
			int x = Vertices[objnum]->x;
			int y = Vertices[objnum]->y;

			int vert_r = vertex_radius(grid.Scale);

			trans_param.Apply(&x, &y);

			if (! Vis(x, y, vert_r))
				break;

			DrawVertex(x, y, vert_r);

			int r = vert_r * 3 / 2;

			int sx1 = SCREENX(x) - r;
			int sy1 = SCREENY(y) - r;
			int sx2 = SCREENX(x) + r;
			int sy2 = SCREENY(y) + r;

			gl_line(sx1, sy1, sx2, sy1);
			gl_line(sx2, sy1, sx2, sy2);
			gl_line(sx2, sy2, sx1, sy2);
			gl_line(sx1, sy2, sx1, sy1);
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
				break;

			DrawMapVector(x1, y1, x2, y2);
		}
		break;

		case OBJ_SECTORS:
		{
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
		}
		break;
	}
}


void UI_Canvas::DrawTagged(int objtype, int objnum)
{
	// gl_color has been done by caller

	// handle tagged linedefs : show matching sector(s)
	if (objtype == OBJ_LINEDEFS && LineDefs[objnum]->tag > 0)
	{
		for (int m = 0 ; m < NumSectors ; m++)
			if (Sectors[m]->tag == LineDefs[objnum]->tag)
				DrawHighlight(OBJ_SECTORS, m);
	}

	// handle tagged sectors : show matching line(s)
	if (objtype == OBJ_SECTORS && Sectors[objnum]->tag > 0)
	{
		for (int m = 0 ; m < NumLineDefs ; m++)
			if (LineDefs[m]->tag == Sectors[objnum]->tag)
				DrawHighlight(OBJ_LINEDEFS, m);
	}
}


void UI_Canvas::DrawSectorSelection(selection_c *list, int dx, int dy)
{
	// gl_color() and gl_line_width() has been done by caller

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		int x1 = dx + L->Start()->x;
		int y1 = dy + L->Start()->y;
		int x2 = dx + L->End  ()->x;
		int y2 = dy + L->End  ()->y;

		if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
			continue;

		if (L->right < 0 && L->left < 0)
			continue;

		int sec1 = -1;
		int sec2 = -1;

		if (L->right >= 0) sec1 = L->Right()->sector;
		if (L->left  >= 0) sec2 = L->Left() ->sector;

		bool touches1 = (sec1 >= 0) && list->get(sec1);
		bool touches2 = (sec2 >= 0) && list->get(sec2);

		if (! (touches1 || touches2))
			continue;

		// skip lines if both sides are in the selection
		if (touches1 && touches2)
			continue;

		bool reverse = !touches1;

		DrawKnobbyLine(x1, y1, x2, y2, reverse);
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
		gl_color(SEL_COL);

		if (list->what_type() == OBJ_LINEDEFS || list->what_type() == OBJ_SECTORS)
			gl_line_width(2);

		for (list->begin(&it) ; !it.at_end() ; ++it)
		{
			DrawHighlightTransform(list->what_type(), *it);
		}

		gl_line_width(1);
		return;
	}

	int dx = 0;
	int dy = 0;

	if (edit.action == ACT_DRAG && edit.drag_single_obj < 0)
	{
		DragDelta(&dx, &dy);
	}

	gl_color(edit.error_mode ? FL_RED : SEL_COL);

	if (list->what_type() == OBJ_LINEDEFS || list->what_type() == OBJ_SECTORS)
		gl_line_width(2);

	// special case when we have many sectors
	if (list->what_type() == OBJ_SECTORS && list->count_obj() > MAX_STORE_SEL)
	{
		DrawSectorSelection(list, dx, dy);
	}
	else
	{
		for (list->begin(&it) ; !it.at_end() ; ++it)
		{
			DrawHighlight(list->what_type(), *it, true /* skip_lines */, dx, dy);
		}
	}

	if (! edit.error_mode && dx == 0 && dy == 0)
	{
		gl_color(LIGHTRED);

		for (list->begin(&it) ; !it.at_end() ; ++it)
		{
			DrawTagged(list->what_type(), *it);
		}
	}

	gl_line_width(1);
}


//
//  draw a plain line at the given map coords
//
void UI_Canvas::DrawMapLine(float map_x1, float map_y1, float map_x2, float map_y2)
{
    gl_line(SCREENX(map_x1), SCREENY(map_y1),
            SCREENX(map_x2), SCREENY(map_y2));
}


//
//  draw a line with a "knob" showing the right (front) side
//
void UI_Canvas::DrawKnobbyLine(int map_x1, int map_y1, int map_x2, int map_y2,
                               bool reverse)
{
	// gl_color() has been done by caller

	int x1 = SCREENX(map_x1);
	int y1 = SCREENY(map_y1);
	int x2 = SCREENX(map_x2);
	int y2 = SCREENY(map_y2);

	if (reverse)
	{
		std::swap(x1, x2);
		std::swap(y1, y2);
	}

    gl_line(x1, y1, x2, y2);

	// indicate direction of line
   	int mx = (x1 + x2) / 2;
   	int my = (y1 + y2) / 2;

	int len = MAX(abs(x2 - x1), abs(y2 - y1));
	int want_len = MIN(12, len / 5);

	int dx = NORMALX(want_len, x2 - x1, y2 - y1);
	int dy = NORMALY(want_len, x2 - x1, y2 - y1);

	if (! (dx == 0 && dy == 0))
	{
		gl_line(mx, my, mx + dx, my + dy);
	}
}


void UI_Canvas::DrawSplitPoint(int map_x, int map_y)
{
	int sx = SCREENX(map_x);
	int sy = SCREENY(map_y);

	int size = (grid.Scale >= 5.0) ? 9 : (grid.Scale >= 1.0) ? 7 : 5;

	gl_color(HI_AND_SEL_COL);

#ifdef NO_OPENGL
	fl_pie(sx - size/2, sy - size/2, size, size, 0, 360);
#else
	glPointSize(size);

	glBegin(GL_POINTS);
	glVertex2i(sx, sy);
	glEnd();

	glPointSize(1.0);
#endif
}


void UI_Canvas::DrawSplitLine(int map_x1, int map_y1, int map_x2, int map_y2)
{
	// show how and where the line will be split

	// gl_color() has been done by caller

	int scr_x1 = SCREENX(map_x1);
	int scr_y1 = SCREENY(map_y1);
	int scr_x2 = SCREENX(map_x2);
	int scr_y2 = SCREENY(map_y2);

	int scr_mx = SCREENX(split_x);
	int scr_my = SCREENY(split_y);

	gl_line(scr_x1, scr_y1, scr_mx, scr_my);
	gl_line(scr_x2, scr_y2, scr_mx, scr_my);

	if (! edit.show_object_numbers)
	{
		int len1 = ComputeDist(map_x1 - split_x, map_y1 - split_y);
		int len2 = ComputeDist(map_x2 - split_x, map_y2 - split_y);

		DrawLineNumber(map_x1, map_y1, split_x, split_y, 0, len1);
		DrawLineNumber(map_x2, map_y2, split_x, split_y, 0, len2);
	}

	DrawSplitPoint(split_x, split_y);
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

	gl_line(x1, y1, x2, y2);

	// knob
	int mx = (x1 + x2) / 2;
	int my = (y1 + y2) / 2;

	int klen = MAX(abs(x2 - x1), abs(y2 - y1));
	int want_len = CLAMP(12, klen / 4, 40);

	int kx = NORMALX(want_len, x2 - x1, y2 - y1);
	int ky = NORMALY(want_len, x2 - x1, y2 - y1);

	gl_line(mx, my, mx + kx, my + ky);

	// arrow
	double r2 = hypot((double) (x1 - x2), (double) (y1 - y2));

	if (r2 < 1.0)
		r2 = 1.0;

	double len = CLAMP(6.0, r2 / 10.0, 24.0);

	int dx = (int) (len * (x1 - x2) / r2);
	int dy = (int) (len * (y1 - y2) / r2);

	x1 = x2 + 2 * dx;
	y1 = y2 + 2 * dy;

	gl_line(x1 - dy, y1 + dx, x2, y2);
	gl_line(x1 + dy, y1 - dx, x2, y2);
}


//
//  draw an arrow
//
void UI_Canvas::DrawMapArrow(int map_x1, int map_y1, int r, int angle)
{
	float dx = r * cos(angle * M_PI / 180.0);
	float dy = r * sin(angle * M_PI / 180.0);

	float map_x2 = map_x1 + dx;
	float map_y2 = map_y1 + dy;

	DrawMapLine(map_x1, map_y1, map_x2, map_y2);

	// arrow head
	float x3 = map_x2 - dx * 0.3 + dy * 0.3;
	float y3 = map_y2 - dy * 0.3 - dx * 0.3;

	DrawMapLine(map_x2, map_y2, x3, y3);

	x3 = map_x2 - dx * 0.3 - dy * 0.3;
	y3 = map_y2 - dy * 0.3 + dx * 0.3;

	DrawMapLine(map_x2, map_y2, x3, y3);
}


void UI_Canvas::DrawCamera()
{
	int map_x, map_y;
	float angle;

	Render3D_GetCameraPos(&map_x, &map_y, &angle);

	float mx = map_x;
	float my = map_y;

	float r = 40.0 / sqrt(grid.Scale);

	float dx = r * cos(angle * M_PI / 180.0);
	float dy = r * sin(angle * M_PI / 180.0);

	// arrow body
	float x1 = mx - dx;
	float y1 = my - dy;

	float x2 = mx + dx;
	float y2 = my + dy;

	gl_color(CAMERA_COLOR);
	gl_line_width(1);

	DrawMapLine(x1, y1, x2, y2);

	// arrow head
	float x3 = x2 - dx * 0.6 + dy * 0.4;
	float y3 = y2 - dy * 0.6 - dx * 0.4;

	DrawMapLine(x2, y2, x3, y3);

	x3 = x2 - dx * 0.6 - dy * 0.4;
	y3 = y2 - dy * 0.6 + dx * 0.4;

	DrawMapLine(x2, y2, x3, y3);

	// notches on body
	DrawMapLine(mx - dy * 0.4, my + dx * 0.4,
				mx + dy * 0.4, my - dx * 0.4);

	mx = mx - dx * 0.2;
	my = my - dy * 0.2;

	DrawMapLine(mx - dy * 0.4, my + dx * 0.4,
				mx + dy * 0.4, my - dx * 0.4);

	gl_line_width(1);
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
	else if (split_ld >= 0)
	{
		new_x = split_x;
		new_y = split_y;
	}
	else
	{
		gl_color(FL_GREEN);

		DrawVertex(new_x, new_y, vertex_radius(grid.Scale));
	}

	gl_color(RED);

	const Vertex * v = Vertices[edit.drawing_from];

	DrawKnobbyLine(v->x, v->y, new_x, new_y);

	int dx = v->x - new_x;
	int dy = v->y - new_y;

	if (dx || dy)
	{
		float length = sqrt(dx * dx + dy * dy);

		DrawLineNumber(v->x, v->y, new_x, new_y, 0, I_ROUND(length));
	}

	// draw all the crossing points
	crossing_state_c cross;

	FindCrossingPoints(cross,
					   v->x, v->y, edit.drawing_from,
					   new_x, new_y, highlight.valid() ? highlight.num : -1);

	for (unsigned int k = 0 ; k < cross.points.size() ; k++)
	{
		cross_point_t& point = cross.points[k];

		if (point.ld >= 0 && point.ld == split_ld)
			return;

		DrawSplitPoint(point.x, point.y);
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

	gl_color(FL_CYAN);

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

void UI_Canvas::RenderSector(int num)
{
	if (! Subdiv_SectorOnScreen(num, map_lx, map_ly, map_hx, map_hy))
		return;

	sector_subdivision_c *subdiv = Subdiv_PolygonsForSector(num);

	if (! subdiv)
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

#ifdef NO_OPENGL
	const img_pixel_t *src_pix = img ? img->buf() : NULL;

	u8_t * line_rgb = new u8_t[3 * (w() + 4)];

	for (unsigned int i = 0 ; i < subdiv->polygons.size() ; i++)
	{
		sector_polygon_t *poly = &subdiv->polygons[i];

		float py1 = poly->my[1];  // north most
		float py2 = poly->my[0];

		int sy1 = SCREENY(py1);
		int sy2 = SCREENY(py2);

		// clip range to screen
		sy1 = MAX(sy1, y());
		sy2 = MIN(sy2, y() + h() - 1);

		// reject polygons vertically off the screen
		if (sy1 > sy2)
			continue;

		// get left and right edges, unpacking a triangle if necessary
		float lx1 = poly->mx[1];
		float lx2 = poly->mx[0];

		float rx1 = poly->mx[2];
		float rx2 = poly->mx[3];

		if (poly->count == 3)
		{
			if (poly->my[2] == poly->my[0])
			{
				rx1 = poly->mx[1];
				rx2 = poly->mx[2];
			}
			else // my[2] == my[1]
			{
				rx2 = poly->mx[0];
			}
		}

		// visit each screen row
		for (short y = (short)sy1 ; y <= (short)sy2 ; y++)
		{
			// compute horizontal span
			float map_y = MAPY(y);

			float lx = lx1 + (lx2 - lx1) * (map_y - py1) / (py2 - py1);
			float rx = rx1 + (rx2 - rx1) * (map_y - py1) / (py2 - py1);

			int sx1 = SCREENX(floor(lx));
			int sx2 = SCREENX(ceil(rx));

			// clip span to screen
			sx1 = MAX(sx1, x());
			sx2 = MIN(sx2, x() + w() - 1);

			// reject spans completely off the screen
			if (sx2 < sx1)
				continue;

///  fprintf(stderr, "  span : y=%d  x=%d..%d\n", y, x1, x2);

			// solid color?
			if (! img)
			{
				gl_rectf(sx1, y, sx2 - sx1 + 1, 1);
				continue;
			}

			int x = sx1;
			int span_w = sx2 - sx1 + 1;

			u8_t *dest = line_rgb;
			u8_t *dest_end = line_rgb + span_w * 3;

			// the logic here for non-64x64 textures matches the software
			// 3D renderer, but is different than ZDoom (which scales them).
			int ty = (0 - (int)MAPY(y)) & (th - 1);

			for (; dest < dest_end ; dest += 3, x++)
			{
				int tx = (int)MAPX(x) & (tw - 1);

				img_pixel_t pix = src_pix[ty * tw + tx];

				IM_DecodePixel(pix, dest[0], dest[1], dest[2]);
			}

			fl_draw_image(line_rgb, sx1, y, span_w, 1);
		}
	}

	delete[] line_rgb;

#else // OpenGL
	if (img)
	{
		glColor3f(1, 1, 1);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);

		glAlphaFunc(GL_GREATER, 0.5);

		img->bind_gl();
	}

	for (unsigned int i = 0 ; i < subdiv->polygons.size() ; i++)
	{
		sector_polygon_t *poly = &subdiv->polygons[i];

		// draw polygon
		glBegin(GL_POLYGON);

		for (int p = 0 ; p < poly->count ; p++)
		{
			int sx = SCREENX(poly->mx[p]);
			int sy = SCREENY(poly->my[p]);

			if (img)
			{
				// this logic follows ZDoom, which scales large flats to
				// occupy a 64x64 unit area.  I presume wall textures are
				// handled similarily....
				glTexCoord2f(poly->mx[p] / 64.0, poly->my[p] / 64.0);
			}

			glVertex2i(sx, sy);
		}

		glEnd();
	}

	if (img)
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
	}
#endif
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
