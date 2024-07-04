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

#include "Instance.h"
#include "main.h"

#include <algorithm>

#ifndef NO_OPENGL
#include "FL/gl.h"
#include "FL/glu.h"
#endif

#include "ui_window.h"

#include "m_events.h"
#include "e_main.h"
#include "e_hover.h"
#include "e_linedef.h"
#include "e_sector.h"
#include "e_things.h"
#include "e_path.h"	  // SoundPropagation
#include "im_color.h"
#include "im_img.h"
#include "LineDef.h"
#include "m_config.h"
#include "m_game.h"
#include "m_vector.h"
#include "r_grid.h"
#include "r_subdiv.h"
#include "r_render.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"
#include "w_rawdef.h"	// MLF_xxx
#include "w_texture.h"

#include <assert.h>


#define CAMERA_COLOR  fl_rgb_color(255, 192, 255)


typedef enum
{
	LINFO_Nothing = 0,
	LINFO_Length,
	LINFO_Angle,
	LINFO_Ratio,
	LINFO_Length_Angle,
	LINFO_Length_Ratio

} line_info_mode_e;


// config items
rgb_color_t config::dotty_axis_col  = rgbMake(0, 128, 255);
rgb_color_t config::dotty_major_col = rgbMake(0, 0, 238);
rgb_color_t config::dotty_minor_col = rgbMake(0, 0, 187);
rgb_color_t config::dotty_point_col = rgbMake(0, 0, 255);

rgb_color_t config::normal_axis_col  = rgbMake(0, 128, 255);
rgb_color_t config::normal_main_col  = rgbMake(0, 0, 238);
rgb_color_t config::normal_flat_col  = rgbMake(60, 60, 120);
rgb_color_t config::normal_small_col = rgbMake(60, 60, 120);

int config::highlight_line_info = (int)LINFO_Length;


int vertex_radius(double scale);


UI_Canvas::UI_Canvas(Instance &inst, int X, int Y, int W, int H, const char *label) :
#ifdef NO_OPENGL
	Fl_Widget(X, Y, W, H, label),
#else
	Fl_Gl_Window(X, Y, W, H),
#endif
	last_highlight(),
	last_splitter(-1),
	last_split_x(), last_split_y(),
	snap_x(-1), snap_y(-1),
	seen_sectors(),
	inst(inst)
{
#ifdef NO_OPENGL
	rgb_buf = NULL;
#endif
}


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
#ifndef NO_OPENGL
	if (! valid())
	{
		// reset the 'gl_tex' field of all loaded images, as the value
		// belongs to a context which was (probably) just deleted and
		// hence refer to textures which no longer exist.
		inst.wad.images.W_UnloadAllTextures();
	}

#ifndef _WIN32	// TODO: #56: reenable this for Windows
	static bool tried;
	if(!tried)
	{
		tried = true;
		const GLubyte *strExt = glGetString(GL_EXTENSIONS);

		if(strExt)
			global::use_npot_textures = gluCheckExtension((const GLubyte *)"GL_ARB_texture_non_power_of_two", strExt) == GLU_TRUE;
	}
#endif
#endif

	if (inst.edit.render3d)
	{
		Render3D_Draw(inst, x(), y(), w(), h());
		return;
	}

#ifdef NO_OPENGL
	xx = x();
	yy = y();

	map_lx = floor(MAPX(xx));
	map_ly = floor(MAPY(yy + h()));

	map_hx = ceil(MAPX(xx + w()));
	map_hy = ceil(MAPY(yy));

#else // OpenGL
	xx = yy = 0;

	map_lx = floor(MAPX(0));
	map_ly = floor(MAPY(0));

	map_hx = ceil(MAPX(w()));
	map_hy = ceil(MAPY(h()));

	// setup projection matrix for 2D drawing

	// Note: this crud is a workaround for retina displays on MacOS
	Fl::use_high_res_GL(true);
	int pix = iround(inst.main_win->canvas->pixels_per_unit());
	Fl::use_high_res_GL(false);

	glLoadIdentity();
	glViewport(0, 0, w() * pix, h() * pix);
	glOrtho(0, w(), 0, h(), -1, 1);
#endif

	PrepareToDraw();

	RenderColor(FL_WHITE);
	RenderThickness(1);

	// default font (for showing object numbers)
	int font_size = (inst.grid.getScale() < 0.9) ? 14 : 19;
	RenderFontSize(font_size);

	DrawEverything();

	Blit();
}


int UI_Canvas::handle(int event)
{
	if (inst.EV_HandleEvent(event))
		return 1;

	return Fl_Widget::handle(event);
}


int UI_Canvas::NORMALX(int len, double dx, double dy)
{
#ifdef NO_OPENGL
	double res = -dy;
#else
	double res = dy;
#endif

	double got_len = hypot(dx, dy);
	if (got_len < 0.01)
		return 0;

	return iround(res * len / got_len);
}

int UI_Canvas::NORMALY(int len, double dx, double dy)
{
#ifdef NO_OPENGL
	double res = dx;
#else
	double res = -dx;
#endif

	double got_len = hypot(dx, dy);
	if (got_len < 0.01)
		return 0;

	return iround(res * len / got_len);
}

#ifdef NO_OPENGL
// convert screen coordinates to map coordinates
inline double UI_Canvas::MAPX(int sx) const { return inst.grid.getOrig().x + (sx - w() / 2 - x()) / inst.grid.getScale(); }
inline double UI_Canvas::MAPY(int sy) const { return inst.grid.getOrig().y + (h() / 2 - sy + y()) / inst.grid.getScale(); }

// convert map coordinates to screen coordinates
inline int UI_Canvas::SCREENX(double mx) const { return (x() + w() / 2 + iround((mx - inst.grid.getOrig().x) * inst.grid.getScale())); }
inline int UI_Canvas::SCREENY(double my) const { return (y() + h() / 2 + iround((inst.grid.getOrig().y - my) * inst.grid.getScale())); }
#else
// convert GL coordinates to map coordinates
inline double UI_Canvas::MAPX(int sx) const { return inst.grid.getOrig().x + (sx - w() / 2) / inst.grid.getScale(); }
inline double UI_Canvas::MAPY(int sy) const { return inst.grid.getOrig().y + (sy - h() / 2) / inst.grid.getScale(); }

// convert map coordinates to GL coordinates
inline int UI_Canvas::SCREENX(double mx) const { return (w() / 2 + iround((mx - inst.grid.getOrig().x) * inst.grid.getScale())); }
inline int UI_Canvas::SCREENY(double my) const { return (h() / 2 + iround((my - inst.grid.getOrig().y) * inst.grid.getScale())); }
#endif

void UI_Canvas::PointerPos(bool in_event)
{
	if (inst.edit.render3d)
		return;

	// we read current position outside of FLTK's event propagation.
	int raw_x, raw_y;
	Fl::get_mouse(raw_x, raw_y);

#ifdef NO_OPENGL
	raw_x -= inst.main_win->x_root();
	raw_y -= inst.main_win->y_root();

	inst.edit.map.x = MAPX(raw_x);
	inst.edit.map.y = MAPY(raw_y);

#else // OpenGL
	raw_x -= x_root();
	raw_y -= y_root();

	inst.edit.map.x = MAPX(raw_x);
	inst.edit.map.y = MAPY(h() - 1 - raw_y);
#endif

	inst.grid.NaturalSnapXY(inst.edit.map.x, inst.edit.map.y);

	// no Z coord with the 2D map view
	inst.edit.map.z = -1;
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

	float x_ratio = std::max(4, x2 - x1) / (float)std::max(4, w());
	float y_ratio = std::max(4, y2 - y1) / (float)std::max(4, h());

	if (std::max(x_ratio, y_ratio) < 0.25)
		return -1;  // too small

	return 0;
}


//------------------------------------------------------------------------


void UI_Canvas::DrawEverything()
{
	// setup for drawing sector numbers
	if (inst.edit.show_object_numbers && inst.edit.mode == ObjType::sectors)
	{
		seen_sectors.clear_all();
	}

	DrawMap();

	DrawSelection(&*inst.edit.Selected);

	if (inst.edit.action == EditorAction::drag && !inst.edit.dragged.valid() && inst.edit.drag_lines != NULL)
		DrawSelection(inst.edit.drag_lines);
	else if (inst.edit.action == EditorAction::transform && inst.edit.trans_lines != NULL)
		DrawSelection(inst.edit.trans_lines);

	if (inst.edit.action == EditorAction::drag && inst.edit.dragged.valid())
	{
		v2double_t delta = DragDelta();

		if (inst.edit.mode == ObjType::vertices)
			RenderColor(HI_AND_SEL_COL);
		else
			RenderColor(HI_COL);

		if (inst.edit.mode == ObjType::linedefs || inst.edit.mode == ObjType::sectors)
			RenderThickness(2);

		DrawHighlight(inst.edit.mode, inst.edit.dragged.num, false /* skip_lines */, delta.x, delta.y);

		if (inst.edit.mode == ObjType::vertices && inst.edit.highlight.valid())
		{
			RenderColor(HI_COL);
			DrawHighlight(inst.edit.highlight.type, inst.edit.highlight.num);
		}

		RenderThickness(1);

		// when ratio lock is on, want to see the new line
		if (inst.edit.mode == ObjType::vertices && inst.grid.getRatio() > 0 && inst.edit.drag_other_vert >= 0)
		{
			const auto v0 = inst.level.vertices[inst.edit.drag_other_vert];
			const auto v1 = inst.level.vertices[inst.edit.dragged.num];

			RenderColor(RED);
			DrawKnobbyLine(v0->x(), v0->y(), v1->x() + delta.x, v1->y() + delta.y);

			DrawLineInfo(v0->x(), v0->y(), v1->x() + delta.x, v1->y() + delta.y, true);
		}
	}
	else if (inst.edit.highlight.valid())
	{
		if (inst.edit.action != EditorAction::drawLine && inst.edit.Selected->get(inst.edit.highlight.num))
			RenderColor(HI_AND_SEL_COL);
		else
			RenderColor(HI_COL);

		if (inst.edit.highlight.type == ObjType::linedefs || inst.edit.highlight.type == ObjType::sectors)
			RenderThickness(2);

		DrawHighlight(inst.edit.highlight.type, inst.edit.highlight.num);

		if (! inst.edit.error_mode)
		{
			RenderColor(LIGHTRED);
			DrawTagged(inst.edit.highlight.type, inst.edit.highlight.num);
		}

		if (inst.edit.mode == ObjType::linedefs && !inst.edit.show_object_numbers)
		{
			const auto L = inst.level.linedefs[inst.edit.highlight.num];
			DrawLineInfo(inst.level.getStart(*L).x(), inst.level.getStart(*L).y(), inst.level.getEnd(*L).x(), inst.level.getEnd(*L).y(), false);
		}

		RenderThickness(1);
	}

	if (inst.edit.action == EditorAction::selbox)
		SelboxDraw();

	if (inst.edit.action == EditorAction::drawLine)
		DrawCurrentLine();
}


//
// draw the whole map, except for hilight/selection/selbox
//
void UI_Canvas::DrawMap()
{
	RenderColor(FL_BLACK);
	RenderRect(xx, yy, w(), h());

	if (inst.edit.sector_render_mode && ! inst.edit.error_mode)
	{
		for (int n = 0 ; n < inst.level.numSectors(); n++)
			RenderSector(n);
	}

	// draw the grid first since it's in the background
	if (inst.grid.isShown())
	{
		if (config::grid_style == 0)
			DrawGrid_Normal();
		else
			DrawGrid_Dotty();
	}

	if (global::Debugging)
		DrawMapBounds();

	DrawCamera();

	if (inst.edit.mode != ObjType::things)
		DrawThings();

	if (inst.grid.snaps() && config::grid_snap_indicator)
		DrawSnapPoint();

	DrawLinedefs();

	if (inst.edit.mode == ObjType::vertices)
		DrawVertices();

	if (inst.edit.mode == ObjType::things)
	{
		if (inst.edit.thing_render_mode > 0)
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
//  draw the grid in the background of the inst.edit window
//
void UI_Canvas::DrawGrid_Normal()
{
	float pixels_1 = static_cast<float>(inst.grid.getStep() * inst.grid.getScale());

	if (pixels_1 < 1.6)
	{
		RenderColor(DarkerColor(DarkerColor(config::normal_main_col)));
		RenderRect(xx, yy, w(), h());

		DrawAxes(config::normal_axis_col);
		return;
	}


	int flat_step = 64;

	float pixels_2 = static_cast<float>(flat_step * inst.grid.getScale());

	Fl_Color flat_col = (inst.grid.getStep() < 64) ? config::normal_main_col : config::normal_flat_col;

	if (pixels_2 < 2.2)
		flat_col = DarkerColor(flat_col);

	RenderColor(flat_col);

	if (pixels_2 < 1.6)
	{
		RenderRect(xx, yy, w(), h());
	}
	else
	{
		int gx = static_cast<int>(floor(map_lx / flat_step) * flat_step);

		for (; gx <= map_hx; gx += flat_step)
			DrawMapLine(gx, map_ly, gx, map_hy);

		int gy = static_cast<int>(floor(map_ly / flat_step) * flat_step);

		for (; gy <= map_hy; gy += flat_step)
			DrawMapLine(map_lx, gy, map_hx, gy);
	}


	Fl_Color main_col = (inst.grid.getStep() < 64) ? config::normal_small_col : config::normal_main_col;

	float pixels_3 = static_cast<float>(inst.grid.getStep() * inst.grid.getScale());

	if (pixels_3 < 4.2)
		main_col = DarkerColor(main_col);

	RenderColor(main_col);

	{
		int gx = static_cast<int>(floor(map_lx / inst.grid.getStep()) * inst.grid.getStep());

		for (; gx <= map_hx; gx += inst.grid.getStep())
			if ((inst.grid.getStep() >= 64 || (gx & 63) != 0) && (gx != 0))
				DrawMapLine(gx, map_ly, gx, map_hy);

		int gy = static_cast<int>(floor(map_ly / inst.grid.getStep()) * inst.grid.getStep());

		for (; gy <= map_hy; gy += inst.grid.getStep())
			if ((inst.grid.getStep() >= 64 || (gy & 63) != 0) && (gy != 0))
				DrawMapLine(map_lx, gy, map_hx, gy);
	}


	DrawAxes(config::normal_axis_col);
}


void UI_Canvas::DrawGrid_Dotty()
{
	int grid_step_1 = 1 * inst.grid.getStep();    // Map units between dots
	int grid_step_2 = 8 * grid_step_1;  // Map units between dim lines
	int grid_step_3 = 8 * grid_step_2;  // Map units between bright lines

	float pixels_1 = static_cast<float>(inst.grid.getStep() * inst.grid.getScale());


	if (pixels_1 < 1.6)
	{
		RenderColor(DarkerColor(DarkerColor(config::dotty_point_col)));
		RenderRect(xx, yy, w(), h());

		DrawAxes(config::dotty_axis_col);
		return;
	}


	RenderColor(config::dotty_major_col);
	{
		int gx = static_cast<int>(floor(map_lx / grid_step_3) * grid_step_3);

		for (; gx <= map_hx; gx += grid_step_3)
			DrawMapLine(gx, map_ly-2, gx, map_hy+2);

		int gy = static_cast<int>(floor(map_ly / grid_step_3) * grid_step_3);

		for (; gy <= map_hy; gy += grid_step_3)
			DrawMapLine(map_lx, gy, map_hx, gy);
	}


	DrawAxes(config::dotty_axis_col);


	RenderColor(config::dotty_minor_col);
	{
		int gx = static_cast<int>(floor(map_lx / grid_step_2) * grid_step_2);

		for (; gx <= map_hx; gx += grid_step_2)
			if (gx % grid_step_3 != 0)
				DrawMapLine(gx, map_ly, gx, map_hy);

		int gy = static_cast<int>(floor(map_ly / grid_step_2) * grid_step_2);

		for (; gy <= map_hy; gy += grid_step_2)
			if (gy % grid_step_3 != 0)
				DrawMapLine(map_lx, gy, map_hx, gy);
	}


	if (pixels_1 < 4.02)
		RenderColor(DarkerColor(config::dotty_point_col));
	else
		RenderColor(config::dotty_point_col);

	{
		int gx = static_cast<int>(floor(map_lx / grid_step_1) * grid_step_1);
		int gy = static_cast<int>(floor(map_ly / grid_step_1) * grid_step_1);

		for (int ny = gy; ny <= map_hy; ny += grid_step_1)
		for (int nx = gx; nx <= map_hx; nx += grid_step_1)
		{
			int sx = SCREENX(nx);
			int sy = SCREENY(ny);

			if (pixels_1 < 24.1)
				RenderRect(sx, sy, 1, 1);
			else
				RenderRect(sx, sy, 2, 2);
		}
	}
}


void UI_Canvas::DrawAxes(Fl_Color col)
{
	RenderColor(col);

	DrawMapLine(0, map_ly, 0, map_hy);
	DrawMapLine(map_lx, 0, map_hx, 0);
}


void UI_Canvas::DrawMapBounds()
{
	RenderColor(FL_RED);

	DrawMapLine(inst.level.Map_bound1.x, inst.level.Map_bound1.y, inst.level.Map_bound2.x, inst.level.Map_bound1.y);
	DrawMapLine(inst.level.Map_bound1.x, inst.level.Map_bound2.y, inst.level.Map_bound2.x, inst.level.Map_bound2.y);

	DrawMapLine(inst.level.Map_bound1.x, inst.level.Map_bound1.y, inst.level.Map_bound1.x, inst.level.Map_bound2.y);
	DrawMapLine(inst.level.Map_bound2.x, inst.level.Map_bound1.y, inst.level.Map_bound2.x, inst.level.Map_bound2.y);
}


//
//  the apparent radius of a vertex, in pixels
//
int vertex_radius(double scale)
{
	int r = static_cast<int>(6 * (0.26 + scale / 2));

	if (r > 12) r = 12;

	return r;
}



//
//  draw the vertices, and possibly their numbers
//
void UI_Canvas::DrawVertex(double map_x, double map_y, int r)
{
	int scrx = SCREENX(map_x);
	int scry = SCREENY(map_y);

// BLOBBY TEST
#if 0
	RenderLine(scrx - 1, scry - 2, scrx + 1, scry - 2);
	RenderLine(scrx - 2, scry - 1, scrx + 2, scry - 1);
	RenderLine(scrx - 2, scry + 0, scrx + 2, scry + 0);
	RenderLine(scrx - 2, scry + 1, scrx + 2, scry + 1);
	RenderLine(scrx - 1, scry + 2, scrx + 1, scry + 2);
#else
	RenderLine(scrx - r, scry - r, scrx + r, scry + r);
	RenderLine(scrx + r, scry - r, scrx - r, scry + r);

	RenderLine(scrx - 1, scry, scrx + 1, scry);
	RenderLine(scrx, scry - 1, scrx, scry + 1);
#endif
}


void UI_Canvas::DrawVertices()
{
	const int r = vertex_radius(inst.grid.getScale());

	RenderColor(FL_GREEN);

	for (const auto &vertex : inst.level.vertices)
	{
		double x = vertex->x();
		double y = vertex->y();

		if (Vis(x, y, r))
		{
			DrawVertex(x, y, r);
		}
	}

	if (inst.edit.show_object_numbers)
	{
		for (int n = 0 ; n < inst.level.numVertices(); n++)
		{
			double x = inst.level.vertices[n]->x();
			double y = inst.level.vertices[n]->y();

			if (! Vis(x, y, r))
				continue;

			int sx = SCREENX(x) + r * 3;
			int sy = SCREENY(y) + r * 3;

			DrawNumber(sx, sy, n);
		}
	}
}


//
//  draw all the linedefs
//
void UI_Canvas::DrawLinedefs()
{
	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		const auto L = inst.level.linedefs[n];

		double x1 = inst.level.getStart(*L).x();
		double y1 = inst.level.getStart(*L).y();
		double x2 = inst.level.getEnd(*L).x();
		double y2 = inst.level.getEnd(*L).y();

		if (! Vis(std::min(x1,x2), std::min(y1,y2), std::max(x1,x2), std::max(y1,y2)))
			continue;

		bool one_sided = (! inst.level.getLeft(*L));

		Fl_Color col = LIGHTGREY;

		// 'p' for plain, 'k' for knobbly, 's' for split
		char line_kind = 'p';

		switch (inst.edit.mode)
		{
			case ObjType::vertices:
			{
				if (n == inst.edit.split_line.num)
					col = HI_AND_SEL_COL;
				else if (inst.edit.error_mode)
					col = LIGHTGREY;
				else if (L->right < 0)
					col = RED;
				else if (one_sided)
					col = WHITE;

				if (n == inst.edit.split_line.num)
					line_kind = 's';
				else
					line_kind = 'k';

				// show info of last four added lines
				if (n != inst.edit.split_line.num && n >= (inst.level.numLinedefs() - 4) &&
					!inst.edit.show_object_numbers)
				{
					DrawLineInfo(x1, y1, x2, y2, false);
				}
			}
			break;

			case ObjType::linedefs:
			{
				if (inst.edit.error_mode)
					col = LIGHTGREY;
				else if (! inst.level.getRight(*L)) // no first sidedef?
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

			case ObjType::sectors:
			{
				int sd1 = L->right;
				int sd2 = L->left;

				int s1  = (sd1 < 0) ? NIL_OBJ : inst.level.sidedefs[sd1]->sector;
				int s2  = (sd2 < 0) ? NIL_OBJ : inst.level.sidedefs[sd2]->sector;

				if (inst.edit.error_mode)
					col = LIGHTGREY;
				else if (sd1 < 0)
					col = RED;
				else if (inst.edit.sector_render_mode == SREND_SoundProp)
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

					if (inst.level.sectors[s1]->tag != 0)
						have_tag = true;
					if (inst.level.sectors[s1]->type != 0)
						have_type = true;

					if (s2 >= 0)
					{
						if (inst.level.sectors[s2]->tag != 0)
							have_tag = true;

						if (inst.level.sectors[s2]->type != 0)
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

				if (inst.edit.show_object_numbers)
				{
					if (s1 != NIL_OBJ)
						DrawSectorNum(static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2), Side::right, s1);

					if (s2 != NIL_OBJ)
						DrawSectorNum(static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2), Side::left,  s2);
				}
			}
			break;

			// OBJ_THINGS
			default:
			{
				if (one_sided && ! inst.edit.error_mode)
					col = WHITE;
			}
			break;
		}

		RenderColor(col);

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
	}

	// draw the linedef numbers
	if (inst.edit.mode == ObjType::linedefs && inst.edit.show_object_numbers)
	{
		for (int n = 0 ; n < inst.level.numLinedefs(); n++)
		{
			double x1 = inst.level.getStart(*inst.level.linedefs[n]).x();
			double y1 = inst.level.getStart(*inst.level.linedefs[n]).y();
			double x2 = inst.level.getEnd(*inst.level.linedefs[n]).x();
			double y2 = inst.level.getEnd(*inst.level.linedefs[n]).y();

			if (! Vis(std::min(x1,x2), std::min(y1,y2), std::max(x1,x2), std::max(y1,y2)))
				continue;

			DrawLineNumber(static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2), Side::neither, n);
		}
	}
}


void UI_Canvas::DrawThing(double x, double y, int r, int angle, bool big_arrow)
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
	if (inst.edit.mode != ObjType::things)
		RenderColor(DARKGREY);
	else if (inst.edit.error_mode)
		RenderColor(LIGHTGREY);

	for (const auto &thing : inst.level.things)
	{
		double x = thing->x();
		double y = thing->y();

		if (! Vis(x, y, MAX_RADIUS))
			continue;

		const thingtype_t &info = inst.conf.getThingType(thing->type);

		if (inst.edit.mode == ObjType::things && !inst.edit.error_mode)
		{
			Fl_Color col = (Fl_Color)info.color;
			RenderColor(col);
		}

		int r = info.radius;

		DrawThing(x, y, r, thing->angle, false);
	}

	// draw the thing numbers
	if (inst.edit.mode == ObjType::things && inst.edit.show_object_numbers)
	{
		for (int n = 0 ; n < inst.level.numThings(); n++)
		{
			double x = inst.level.things[n]->x();
			double y = inst.level.things[n]->y();

			if (! Vis(x, y, MAX_RADIUS))
				continue;

			const thingtype_t &info = inst.conf.getThingType(inst.level.things[n]->type);

			x += info.radius + 8;
			y += info.radius + 8;

			DrawNumber(SCREENX(x), SCREENY(y), n);
		}
	}
}


//
//  draw bodies of things (solid boxes, darker than the outline)
//
void UI_Canvas::DrawThingBodies()
{
	if (inst.edit.error_mode)
		return;

	for (const auto &thing : inst.level.things)
	{
		double x = thing->x();
		double y = thing->y();

		if (! Vis(x, y, MAX_RADIUS))
			continue;

		const thingtype_t &info = inst.conf.getThingType(thing->type);

		Fl_Color col = (Fl_Color)info.color;
		RenderColor(DarkerColor(DarkerColor(col)));

		int r = info.radius;

		int sx1 = SCREENX(x - r);
		int sy1 = SCREENY(y + r);
		int sx2 = SCREENX(x + r);
		int sy2 = SCREENY(y - r);

		RenderRect(sx1, sy1, sx2 - sx1 + 1, sy2 - sy1 + 1);
	}
}

static int calcThingRotation(int angle)
{
	// 1: south,     247.5:292.5
	// 2: southwest, 202.5:247.5
	// 3: west,      157.5:202.5
	// 4: northwest, 112.5:157.5
	// 5: north,      67.5:112.5
	// 6: northeast,  22.5: 67.5
	// 7: east,      -22.5: 22.5
	// 8: southeast, -67.5:-22.5 or rather 292.5 : 337.5
	while(angle < 0)
		angle += 360;
	while(angle > 360)
		angle -= 360;
	if(angle >= 338 || angle < 23)
		return 7;
	if(angle < 68)
		return 6;
	if(angle < 113)
		return 5;
	if(angle < 158)
		return 4;
	if(angle < 203)
		return 3;
	if(angle < 248)
		return 2;
	if(angle < 293)
		return 1;
	// if(angle < 338)
	return 8;
}

void UI_Canvas::DrawThingSprites()
{
#ifndef NO_OPENGL
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);

	glAlphaFunc(GL_GREATER, 0.5);
#endif

	for (const auto &thing : inst.level.things)
	{
		double x = thing->x();
		double y = thing->y();

		if (! Vis(x, y, MAX_RADIUS))
			continue;

		const thingtype_t &info = inst.conf.getThingType(thing->type);
		float scale = info.scale;

		Img_c *sprite = inst.wad.getMutableSprite(inst.conf, thing->type, inst.loaded, calcThingRotation(thing->angle));

		if (! sprite)
		{
			sprite = &inst.wad.images.IM_UnknownSprite(inst.conf);
			scale = 0.66f;
		}

		int sx = SCREENX(x);
		int sy = SCREENY(y);

		RenderSprite(sx, sy, static_cast<float>(scale * inst.grid.getScale()), sprite);
	}

#ifndef NO_OPENGL
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
#endif
}


void UI_Canvas::RenderSprite(int sx, int sy, float scale, Img_c *img)
{
	int W = img->width();
	int H = img->height();

	scale = scale * 0.5f;

#ifdef NO_OPENGL
	// software rendering

	int bx1 = sx + (int)floor(-W * scale) - rgb_x;
	int bx2 = sx + (int)ceil ( W * scale) - rgb_x;

	int by1 = sy + (int)floor(-H * scale) - rgb_y;
	int by2 = sy + (int)ceil ( H * scale) - rgb_y;

	// prevent division by zero
	if (bx2 <= bx1) bx2 = bx1 + 1;
	if (by2 <= by1) by2 = by1 + 1;

	// clip to screen
	int rx1 = std::max(bx1, 0);
	int ry1 = std::max(by1, 0);

	int rx2 = std::min(bx2, rgb_w) - 1;
	int ry2 = std::min(by2, rgb_h) - 1;

	if (rx1 >= rx2 || ry1 >= ry2)
		return;

	for (int ry = ry1 ; ry <= ry2 ; ry++)
	{
		byte *dest = rgb_buf + 3 * (rx1 + ry * rgb_w);

		for (int rx = rx1 ; rx <= rx2 ; rx++, dest += 3)
		{
			int ix = W * (rx - bx1) / (bx2 - bx1);
			int iy = H * (ry - by1) / (by2 - by1);

			ix = clamp(0, ix, W - 1);
			iy = clamp(0, iy, H - 1);

			img_pixel_t pix = img->buf()[iy * W + ix];

			if (pix != TRANS_PIXEL)
			{
				inst.wad.palette.decodePixel(pix, dest[0], dest[1], dest[2]);
			}
		}
	}

#else // OpenGL
	int bx1 = sx + (int)floor(-W * scale);
	int bx2 = sx + (int)ceil ( W * scale);

	int by1 = sy + (int)floor(-H * scale);
	int by2 = sy + (int)ceil ( H * scale);

	// don't make too small
	if (bx2 <= bx1) bx2 = bx1 + 1;
	if (by2 <= by1) by2 = by1 + 1;

	// bind the sprite image (upload it to OpenGL if needed)
	img->bind_gl(inst.wad);

	// choose texture coords based on image size
	float tx1 = 0.0;
	float ty1 = 0.0;
	float tx2, ty2;

	if (global::use_npot_textures)
	{
		tx2 = 1.0;
		ty2 = 1.0;
	}
	else
	{
		tx2 = (float)img->width()  / (float)RoundPOW2(img->width());
		ty2 = (float)img->height() / (float)RoundPOW2(img->height());
	}

	glColor3f(1, 1, 1);

	glBegin(GL_QUADS);

	glTexCoord2f(tx1, ty1); glVertex2i(bx1, by1);
	glTexCoord2f(tx1, ty2); glVertex2i(bx1, by2);
	glTexCoord2f(tx2, ty2); glVertex2i(bx2, by2);
	glTexCoord2f(tx2, ty1); glVertex2i(bx2, by1);

	glEnd();
#endif
}


void UI_Canvas::DrawSectorNum(int mx1, int my1, int mx2, int my2, Side side, int n)
{
	// only draw a number for the first linedef actually visible
	if (seen_sectors.get(n))
		return;

	seen_sectors.set(n);

	DrawLineNumber(mx1, my1, mx2, my2, side, n);
}


void UI_Canvas::DrawLineNumber(int mx1, int my1, int mx2, int my2, Side side, int n)
{
	int x1 = SCREENX(mx1);
	int y1 = SCREENY(my1);
	int x2 = SCREENX(mx2);
	int y2 = SCREENY(my2);

	int sx = (x1 + x2) / 2;
	int sy = (y1 + y2) / 2;

	// normally draw line numbers on back of line
	int want_len = static_cast<int>(-16 * clamp(0.25, inst.grid.getScale(), 1.0));

	// for sectors, draw closer and on sector side
	if (side != Side::neither)
	{
		want_len = static_cast<int>(2 + 12 * clamp(0.25, inst.grid.getScale(), 1.0));

		if (side == Side::left)
			want_len = -want_len;
	}

	sx += NORMALX(want_len*2, x2 - x1, y2 - y1);
	sy += NORMALY(want_len,   x2 - x1, y2 - y1);

	DrawNumber(sx, sy, n);
}


void UI_Canvas::DrawLineInfo(double map_x1, double map_y1, double map_x2, double map_y2,
							 bool force_ratio)
{
	line_info_mode_e info = (line_info_mode_e)config::highlight_line_info;

	if (info == LINFO_Nothing)
		return;

	int x1 = SCREENX(map_x1);
	int y1 = SCREENY(map_y1);
	int x2 = SCREENX(map_x2);
	int y2 = SCREENY(map_y2);

	int sx = (x1 + x2) / 2;
	int sy = (y1 + y2) / 2;

	// if midpoint is off the screen, try to find a better one
	double mx = (map_x1 + map_x2) / 2.0;
	double my = (map_y1 + map_y2) / 2.0;

	if (mx < map_lx + 4 || mx > map_hx - 4 ||
		my < map_ly + 4 || my > map_hy - 4)
	{
		double best_dist = 9e9;

		for (double p = 0.1 ; p < 0.91 ; p += 0.1)
		{
			mx = map_x1 + (map_x2 - map_x1) * p;
			my = map_y1 + (map_y2 - map_y1) * p;

			double dist_x = mx * 2.0 - (map_lx + map_hx);
			double dist_y = my * 2.0 - (map_ly + map_hy);
			double dist = hypot(dist_x, dist_y);

			if (dist < best_dist)
			{
				sx = static_cast<int>(x1 + (x2 - x1) * p);
				sy = static_cast<int>(y1 + (y2 - y1) * p);
				best_dist = dist;
			}
		}
	}

	// back of line is best place, no knob getting in the way
	int want_len = static_cast<int>(-16 * clamp(0.25, inst.grid.getScale(), 1.0));

	sx += NORMALX(want_len*2, x2 - x1, y2 - y1);
	sy += NORMALY(want_len,   x2 - x1, y2 - y1);

	/* length */

	FFixedPoint idx = MakeValidCoord(inst.loaded.levelFormat, map_x2) - MakeValidCoord(inst.loaded.levelFormat, map_x1);
	FFixedPoint idy = MakeValidCoord(inst.loaded.levelFormat, map_y2) - MakeValidCoord(inst.loaded.levelFormat, map_y1);

	if (info == LINFO_Length || info >= LINFO_Length_Angle)
	{
		double length = hypot(static_cast<double>(idx), static_cast<double>(idy));

		if (length > 0.1)
		{
			char buffer[64];
			snprintf(buffer, sizeof(buffer), "%1.1f", length);

			RenderNumString(sx, sy, buffer);

			sy = sy - cur_font;
		}
	}

	/* angle */

	if (info == LINFO_Angle || info == LINFO_Length_Angle)
	{
		double dx = static_cast<double>(idx);
		double dy = static_cast<double>(idy);

		int degrees = (int)round(atan2(dy, dx) * 180.0 / M_PI);
		if (degrees < 0)
			degrees += 360;

		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%d^", degrees);

		RenderNumString(sx, sy, buffer);
	}

	/* ratio */

	if (info == LINFO_Ratio || info == LINFO_Length_Ratio)
	{
		if (idx != FFixedPoint{} && idy != FFixedPoint{})
		{
			SString ratio_name = LD_RatioName(idx, idy, true);

			RenderNumString(sx, sy, ratio_name.c_str());
		}
	}
}


//
//  draw a number centered at screen coordinate (x, y)
//
void UI_Canvas::DrawNumber(int x, int y, int num)
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%d", num);

#if 0 /* DEBUG */
	RenderColor(FL_RED);
	RenderRect(x - 1, y - 1, 3, 3);
	return;
#endif

	RenderNumString(x, y, buffer);
}


void UI_Canvas::CheckGridSnap()
{
	if (!inst.grid.snaps() || !config::grid_snap_indicator)
		return;

	double new_snap_x = inst.grid.SnapX(inst.edit.map.x);
	double new_snap_y = inst.grid.SnapY(inst.edit.map.y);

	if (snap_x == new_snap_x && snap_y == new_snap_y)
		return;

	snap_x = new_snap_x;
	snap_y = new_snap_y;

	redraw();
}


void UI_Canvas::UpdateHighlight()
{
	bool changes = false;

	if (! (last_highlight == inst.edit.highlight))
	{
		last_highlight = inst.edit.highlight;
		changes = true;
	}

	int new_ld = inst.edit.split_line.valid() ? inst.edit.split_line.num : -1;

	if (! (last_splitter == new_ld && last_split_x == inst.edit.split.x && last_split_y == inst.edit.split.y))
	{
		last_splitter = new_ld;
		last_split_x  = inst.edit.split.x;
		last_split_y  = inst.edit.split.y;
		changes = true;
	}

	if (changes)
		redraw();
}


//
//  draw the given object in highlight color
//
void UI_Canvas::DrawHighlight(ObjType objtype, int objnum, bool skip_lines,
							  double dx, double dy)
{
	// color and line thickness have been set by caller

	// fprintf(stderr, "DrawHighlight: %d\n", objnum);

	switch (objtype)
	{
		case ObjType::things:
		{
			double x = dx + inst.level.things[objnum]->x();
			double y = dy + inst.level.things[objnum]->y();

			if (! Vis(x, y, MAX_RADIUS))
				break;

			const thingtype_t &info = inst.conf.getThingType(inst.level.things[objnum]->type);

			int r = info.radius;

			if (inst.edit.error_mode)
				DrawThing(x, y, r, inst.level.things[objnum]->angle, false /* big_arrow */);

			r += r / 10 + 4;

			DrawThing(x, y, r, inst.level.things[objnum]->angle, true);
		}
		break;

		case ObjType::linedefs:
		{
			double x1 = dx + inst.level.getStart(*inst.level.linedefs[objnum]).x();
			double y1 = dy + inst.level.getStart(*inst.level.linedefs[objnum]).y();
			double x2 = dx + inst.level.getEnd(*inst.level.linedefs[objnum]).x();
			double y2 = dy + inst.level.getEnd(*inst.level.linedefs[objnum]).y();

			if (! Vis(std::min(x1,x2), std::min(y1,y2), std::max(x1,x2), std::max(y1,y2)))
				break;

			DrawMapVector(x1, y1, x2, y2);
		}
		break;

		case ObjType::vertices:
		{
			double x = dx + inst.level.vertices[objnum]->x();
			double y = dy + inst.level.vertices[objnum]->y();

			int vert_r = vertex_radius(inst.grid.getScale());

			if (! Vis(x, y, vert_r))
				break;

			DrawVertex(x, y, vert_r);

			int r = vert_r * 3 / 2;

			int sx1 = SCREENX(x) - r;
			int sy1 = SCREENY(y) - r;
			int sx2 = SCREENX(x) + r;
			int sy2 = SCREENY(y) + r;

			RenderLine(sx1, sy1, sx2, sy1);
			RenderLine(sx2, sy1, sx2, sy2);
			RenderLine(sx2, sy2, sx1, sy2);
			RenderLine(sx1, sy2, sx1, sy1);
		}
		break;

		case ObjType::sectors:
		{
			for (const auto &L : inst.level.linedefs)
			{
				if (! inst.level.touchesSector(*L, objnum))
					continue;

				bool reverse = false;

				// skip lines if both sides are in the selection
				if (skip_lines && L->TwoSided())
				{
					int sec1 = inst.level.getRight(*L)->sector;
					int sec2 = inst.level.getLeft(*L)->sector;

					if ((sec1 == objnum || inst.edit.Selected->get(sec1)) &&
					    (sec2 == objnum || inst.edit.Selected->get(sec2)))
						continue;

					if (sec1 != objnum)
						reverse = true;
				}

				double x1 = dx + inst.level.getStart(*L).x();
				double y1 = dy + inst.level.getStart(*L).y();
				double x2 = dx + inst.level.getEnd(*L).x();
				double y2 = dy + inst.level.getEnd(*L).y();

				if (! Vis(std::min(x1,x2), std::min(y1,y2), std::max(x1,x2), std::max(y1,y2)))
					continue;

				if (skip_lines)
					DrawKnobbyLine(x1, y1, x2, y2, reverse);
				else
					DrawMapLine(x1, y1, x2, y2);
			}
		}
		break;

		default:
			break;
	}
}


void UI_Canvas::DrawHighlightTransform(ObjType objtype, int objnum)
{
	// color and line thickness have been set by caller

	switch (objtype)
	{
		case ObjType::things:
		{
			double x = inst.level.things[objnum]->x();
			double y = inst.level.things[objnum]->y();

			inst.edit.trans_param.Apply(&x, &y);

			if (! Vis(x, y, MAX_RADIUS))
				break;

			const thingtype_t &info = inst.conf.getThingType(inst.level.things[objnum]->type);

			int r = info.radius;

			DrawThing(x, y, r * 3 / 2, inst.level.things[objnum]->angle, true);
		}
		break;

		case ObjType::vertices:
		{
			double x = inst.level.vertices[objnum]->x();
			double y = inst.level.vertices[objnum]->y();

			int vert_r = vertex_radius(inst.grid.getScale());

			inst.edit.trans_param.Apply(&x, &y);

			if (! Vis(x, y, vert_r))
				break;

			DrawVertex(x, y, vert_r);

			int r = vert_r * 3 / 2;

			int sx1 = SCREENX(x) - r;
			int sy1 = SCREENY(y) - r;
			int sx2 = SCREENX(x) + r;
			int sy2 = SCREENY(y) + r;

			RenderLine(sx1, sy1, sx2, sy1);
			RenderLine(sx2, sy1, sx2, sy2);
			RenderLine(sx2, sy2, sx1, sy2);
			RenderLine(sx1, sy2, sx1, sy1);
		}
		break;

		case ObjType::linedefs:
		{
			double x1 = inst.level.getStart(*inst.level.linedefs[objnum]).x();
			double y1 = inst.level.getStart(*inst.level.linedefs[objnum]).y();
			double x2 = inst.level.getEnd(*inst.level.linedefs[objnum]).x();
			double y2 = inst.level.getEnd(*inst.level.linedefs[objnum]).y();

			inst.edit.trans_param.Apply(&x1, &y1);
			inst.edit.trans_param.Apply(&x2, &y2);

			if (! Vis(std::min(x1,x2), std::min(y1,y2), std::max(x1,x2), std::max(y1,y2)))
				break;

			DrawMapVector(x1, y1, x2, y2);
		}
		break;

		case ObjType::sectors:
		{
			for (const auto &linedef : inst.level.linedefs)
			{
				if (! inst.level.touchesSector(*linedef, objnum))
					continue;

				double x1 = inst.level.getStart(*linedef).x();
				double y1 = inst.level.getStart(*linedef).y();
				double x2 = inst.level.getEnd(*linedef).x();
				double y2 = inst.level.getEnd(*linedef).y();

				inst.edit.trans_param.Apply(&x1, &y1);
				inst.edit.trans_param.Apply(&x2, &y2);

				if (! Vis(std::min(x1,x2), std::min(y1,y2), std::max(x1,x2), std::max(y1,y2)))
					continue;

				DrawMapLine(x1, y1, x2, y2);
			}
		}
		break;

		default:
			break;
	}
}

void UI_Canvas::DrawTagged(ObjType objtype, int objnum)
{
	// color has been set by caller

	// handle tagged linedefs : show matching sector(s)

    //
    // Highlight tagged items now
    //
    auto highlightTaggedItems = [this](const SpecialTagInfo &info)
    {
        if(info.numtags)
            for (int m = 0 ; m < inst.level.numSectors(); m++)
                if(inst.level.sectors[m]->tag > 0)
                    for(int i = 0; i < info.numtags; ++i)
                        if (inst.level.sectors[m]->tag == info.tags[i])
                            DrawHighlight(ObjType::sectors, m);
        if(info.numtids)
            for(int m = 0; m < inst.level.numThings(); m++)
                if(inst.level.things[m]->tid > 0)
                {
                    if(info.type == ObjType::things && info.objnum == m)
                        continue;   // don't highlight the trigger again
                    for(int i = 0; i < info.numtids; ++i)
                        if(inst.level.things[m]->tid == info.tids[i])
                            DrawHighlight(ObjType::things, m);
                }

        if(info.numlineids)
        {
            for(int m = 0; m < inst.level.numLinedefs(); ++m)
            {
                if(info.type == ObjType::linedefs && info.objnum == m)
                    continue;   // don't highlight the trigger again
                const LineDef &line = *inst.level.linedefs[m];
                if(inst.loaded.levelFormat == MapFormat::doom)
                {
                    if(line.tag > 0)
                    {
                        for(int i = 0; i < info.numlineids; ++i)
                            if(line.tag == info.lineids[i])
                                DrawHighlight(ObjType::linedefs, m);
                    }
                }
                else if(inst.loaded.levelFormat == MapFormat::hexen)
                {
                    SpecialTagInfo linfo;
                    if(!getSpecialTagInfo(ObjType::linedefs, m, line.type, &line, inst.conf, linfo)
                       || linfo.selflineid <= 0)
                    {
                        continue;
                    }
                    for(int i = 0; i < info.numlineids; ++i)
                        if(linfo.selflineid == info.lineids[i])
                            DrawHighlight(ObjType::linedefs, m);
                }
                // TODO: also handle UDMF.
            }
        }

        if(info.numpo)
            for(int m = 0; m < inst.level.numThings(); ++m)
            {
                const Thing &thing = *inst.level.things[m];
                const thingtype_t *type = get(inst.conf.thing_types, thing.type);
                if(!type || !(type->flags & THINGDEF_POLYSPOT))
                    continue;
                for(int i = 0; i < info.numpo; ++i)
                    if(info.po[i] == thing.angle)
                        DrawHighlight(ObjType::things, m);
            }
    };

    //
    // Look for all the tagging things
    //
    auto highlightTaggingTriggers = [this, objnum, objtype](int tag, int (SpecialTagInfo::*tags)[5],
                                                            int SpecialTagInfo::*numtags)
    {
        if(tag <= 0)
            return;
        for (int m = 0 ; m < inst.level.numLinedefs(); m++)
        {
            if(objtype == ObjType::linedefs && m == objnum)
                continue;
            const auto line = inst.level.linedefs[m];
            assert(line);
            SpecialTagInfo info;
            if(!getSpecialTagInfo(ObjType::linedefs, m, line->type, line.get(), inst.conf, info))
                continue;

            for(int i = 0; i < info.*numtags; ++i)
                if((info.*tags)[i] == tag)
                {
                    DrawHighlight(ObjType::linedefs, m);
                    break;
                }
        }
        if(inst.loaded.levelFormat == MapFormat::doom)
            return;
        for (int m = 0 ; m < inst.level.numThings(); m++)
        {
            if(objtype == ObjType::things && m == objnum)
                continue;
            const auto thing = inst.level.things[m];
            assert(thing);
            SpecialTagInfo info;
            if(!getSpecialTagInfo(ObjType::things, m, thing->special, thing.get(), inst.conf, info))
                continue;

            for(int i = 0; i < info.*numtags; ++i)
                if((info.*tags)[i] == tag)
                {
                    DrawHighlight(ObjType::things, m);
                    break;
                }
        }
    };

	if (objtype == ObjType::linedefs)
    {
        const auto line = inst.level.linedefs[objnum];
        assert(line);
        SpecialTagInfo info;
        if(getSpecialTagInfo(objtype, objnum, line->type, line.get(), inst.conf, info))
            highlightTaggedItems(info);
        if(inst.loaded.levelFormat == MapFormat::doom)
        {
            highlightTaggingTriggers(line->tag, &SpecialTagInfo::lineids,
                                     &SpecialTagInfo::numlineids);
        }
        else
        {
            SpecialTagInfo linfo;
            if(!getSpecialTagInfo(objtype, objnum, line->type, line.get(), inst.conf, linfo))
                return;
            // TODO: also UDMF line ID
            if(inst.loaded.levelFormat == MapFormat::hexen && linfo.selflineid > 0)
            {
                highlightTaggingTriggers(linfo.selflineid, &SpecialTagInfo::lineids,
                                         &SpecialTagInfo::numlineids);
            }
        }
    }
    else if(inst.loaded.levelFormat != MapFormat::doom && objtype == ObjType::things)
    {
        const auto thing = inst.level.things[objnum];
        assert(thing);
        SpecialTagInfo info;
        if(getSpecialTagInfo(objtype, objnum, thing->special, thing.get(), inst.conf, info))
            highlightTaggedItems(info);
        highlightTaggingTriggers(thing->tid, &SpecialTagInfo::tids, &SpecialTagInfo::numtids);
        const thingtype_t *type = get(inst.conf.thing_types, thing->type);
        if(type && type->flags & THINGDEF_POLYSPOT)
            highlightTaggingTriggers(thing->angle, &SpecialTagInfo::po, &SpecialTagInfo::numpo);
    }
	else if (objtype == ObjType::sectors)
    {
        highlightTaggingTriggers(inst.level.sectors[objnum]->tag, &SpecialTagInfo::tags,
                                 &SpecialTagInfo::numtags);
    }
}


void UI_Canvas::DrawSectorSelection(selection_c *list, double dx, double dy)
{
	// color and line thickness have been set by caller

	for (const auto &L : inst.level.linedefs)
	{
		double x1 = dx + inst.level.getStart(*L).x();
		double y1 = dy + inst.level.getStart(*L).y();
		double x2 = dx + inst.level.getEnd(*L).x();
		double y2 = dy + inst.level.getEnd(*L).y();

		if (! Vis(std::min(x1,x2), std::min(y1,y2), std::max(x1,x2), std::max(y1,y2)))
			continue;

		if (L->right < 0 && L->left < 0)
			continue;

		int sec1 = -1;
		int sec2 = -1;

		if (L->right >= 0) sec1 = inst.level.getRight(*L)->sector;
		if (L->left  >= 0) sec2 = inst.level.getLeft(*L) ->sector;

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

	if (inst.edit.action == EditorAction::transform)
	{
		RenderColor(SEL_COL);

		if (list->what_type() == ObjType::linedefs || list->what_type() == ObjType::sectors)
			RenderThickness(2);

		for (sel_iter_c it(list) ; !it.done() ; it.next())
		{
			DrawHighlightTransform(list->what_type(), *it);
		}

		RenderThickness(1);
		return;
	}

	v2double_t delta = {};

	if (inst.edit.action == EditorAction::drag && inst.edit.dragged.is_nil())
	{
		delta = DragDelta();
	}

	RenderColor(inst.edit.error_mode ? FL_RED : SEL_COL);

	if (list->what_type() == ObjType::linedefs || list->what_type() == ObjType::sectors)
		RenderThickness(2);

	// special case when we have many sectors
	if (list->what_type() == ObjType::sectors && list->count_obj() > MAX_STORE_SEL)
	{
		DrawSectorSelection(list, delta.x, delta.y);
	}
	else
	{
		for (sel_iter_c it(list) ; !it.done() ; it.next())
		{
			DrawHighlight(list->what_type(), *it, true /* skip_lines */, delta.x, delta.y);
		}
	}

	if (! inst.edit.error_mode && delta.x == 0 && delta.y == 0)
	{
		RenderColor(LIGHTRED);

		for (sel_iter_c it(list) ; !it.done() ; it.next())
		{
			DrawTagged(list->what_type(), *it);
		}
	}

	RenderThickness(1);
}


//
//  draw a plain line at the given map coords
//
void UI_Canvas::DrawMapLine(double map_x1, double map_y1, double map_x2, double map_y2)
{
    RenderLine(SCREENX(map_x1), SCREENY(map_y1),
            SCREENX(map_x2), SCREENY(map_y2));
}


//
//  draw a line with a "knob" showing the right (front) side
//
void UI_Canvas::DrawKnobbyLine(double map_x1, double map_y1, double map_x2, double map_y2,
                               bool reverse)
{
	// color and thickness has been set by caller

	int x1 = SCREENX(map_x1);
	int y1 = SCREENY(map_y1);
	int x2 = SCREENX(map_x2);
	int y2 = SCREENY(map_y2);

	if (reverse)
	{
		std::swap(x1, x2);
		std::swap(y1, y2);
	}

    RenderLine(x1, y1, x2, y2);

	// indicate direction of line
   	int mx = (x1 + x2) / 2;
   	int my = (y1 + y2) / 2;

	int len = std::max(abs(x2 - x1), abs(y2 - y1));
	int want_len = std::min(12, len / 5);

	int dx = NORMALX(want_len, x2 - x1, y2 - y1);
	int dy = NORMALY(want_len, x2 - x1, y2 - y1);

	if (! (dx == 0 && dy == 0))
	{
		RenderLine(mx, my, mx + dx, my + dy);
	}
}


void UI_Canvas::DrawSplitPoint(const v2double_t &map_pos)
{
	int sx = SCREENX(map_pos.x);
	int sy = SCREENY(map_pos.y);

	int size = (inst.grid.getScale() >= 5.0) ? 9 : (inst.grid.getScale() >= 1.0) ? 7 : 5;

	// color set by caller

#ifdef NO_OPENGL
	RenderRect(sx - size/2, sy - size/2, size, size);
#else
	glPointSize(static_cast<GLfloat>(size));

	glBegin(GL_POINTS);
	glVertex2i(sx, sy);
	glEnd();

	glPointSize(1.0);
#endif
}


void UI_Canvas::DrawSplitLine(double map_x1, double map_y1, double map_x2, double map_y2)
{
	// show how and where the line will be split

	// color has been set by caller

	int scr_x1 = SCREENX(map_x1);
	int scr_y1 = SCREENY(map_y1);
	int scr_x2 = SCREENX(map_x2);
	int scr_y2 = SCREENY(map_y2);

	int scr_mx = SCREENX(inst.edit.split.x);
	int scr_my = SCREENY(inst.edit.split.y);

	RenderLine(scr_x1, scr_y1, scr_mx, scr_my);
	RenderLine(scr_x2, scr_y2, scr_mx, scr_my);

	if (! inst.edit.show_object_numbers)
	{
		double len1 = hypot(map_x1 - inst.edit.split.x, map_y1 - inst.edit.split.y);
		double len2 = hypot(map_x2 - inst.edit.split.x, map_y2 - inst.edit.split.y);

		DrawLineNumber(static_cast<int>(map_x1), static_cast<int>(map_y1), static_cast<int>(inst.edit.split.x), static_cast<int>(inst.edit.split.y), Side::neither, iround(len1));
		DrawLineNumber(static_cast<int>(map_x2), static_cast<int>(map_y2), static_cast<int>(inst.edit.split.x), static_cast<int>(inst.edit.split.y), Side::neither, iround(len2));
	}

	RenderColor(HI_AND_SEL_COL);

	DrawSplitPoint(inst.edit.split);
}


//
// draw a bolder linedef with an arrow on the end
// (used for highlighted / selected lines)
//
void UI_Canvas::DrawMapVector(double map_x1, double map_y1, double map_x2, double map_y2)
{
	int x1 = SCREENX(map_x1);
	int y1 = SCREENY(map_y1);
	int x2 = SCREENX(map_x2);
	int y2 = SCREENY(map_y2);

	RenderLine(x1, y1, x2, y2);

	// knob
	int mx = (x1 + x2) / 2;
	int my = (y1 + y2) / 2;

	int klen = std::max(abs(x2 - x1), abs(y2 - y1));
	int want_len = clamp(12, klen / 4, 40);

	int kx = NORMALX(want_len, x2 - x1, y2 - y1);
	int ky = NORMALY(want_len, x2 - x1, y2 - y1);

	RenderLine(mx, my, mx + kx, my + ky);

	// arrow
	double r2 = hypot((double) (x1 - x2), (double) (y1 - y2));

	if (r2 < 1.0)
		r2 = 1.0;

	double len = clamp(6.0, r2 / 10.0, 24.0);

	int dx = (int) (len * (x1 - x2) / r2);
	int dy = (int) (len * (y1 - y2) / r2);

	x1 = x2 + 2 * dx;
	y1 = y2 + 2 * dy;

	RenderLine(x1 - dy, y1 + dx, x2, y2);
	RenderLine(x1 + dy, y1 - dx, x2, y2);
}


//
//  draw an arrow
//
void UI_Canvas::DrawMapArrow(double map_x1, double map_y1, int r, int angle)
{
	float dx = static_cast<float>(r * cos(angle * M_PI / 180.0));
	float dy = static_cast<float>(r * sin(angle * M_PI / 180.0));

	float map_x2 = static_cast<float>(map_x1 + dx);
	float map_y2 = static_cast<float>(map_y1 + dy);

	DrawMapLine(map_x1, map_y1, map_x2, map_y2);

	// arrow head
	float x3 = static_cast<float>(map_x2 - dx * 0.3 + dy * 0.3);
	float y3 = static_cast<float>(map_y2 - dy * 0.3 - dx * 0.3);

	DrawMapLine(map_x2, map_y2, x3, y3);

	x3 = map_x2 - dx * 0.3f - dy * 0.3f;
	y3 = map_y2 - dy * 0.3f + dx * 0.3f;

	DrawMapLine(map_x2, map_y2, x3, y3);
}


void UI_Canvas::DrawCamera()
{
	v2double_t map_pos;
	float angle;

	inst.Render3D_GetCameraPos(map_pos, &angle);

	float mx = static_cast<float>(map_pos.x);
	float my = static_cast<float>(map_pos.y);

	float r = static_cast<float>(40.0 / sqrt(inst.grid.getScale()));

	float dx = static_cast<float>(r * cos(angle * M_PI / 180.0));
	float dy = static_cast<float>(r * sin(angle * M_PI / 180.0));

	// arrow body
	float x1 = mx - dx;
	float y1 = my - dy;

	float x2 = mx + dx;
	float y2 = my + dy;

	RenderColor(CAMERA_COLOR);
	RenderThickness(1);

	DrawMapLine(x1, y1, x2, y2);

	// arrow head
	float x3 = static_cast<float>(x2 - dx * 0.6 + dy * 0.4);
	float y3 = static_cast<float>(y2 - dy * 0.6 - dx * 0.4);

	DrawMapLine(x2, y2, x3, y3);

	x3 = static_cast<float>(x2 - dx * 0.6 - dy * 0.4);
	y3 = static_cast<float>(y2 - dy * 0.6 + dx * 0.4);

	DrawMapLine(x2, y2, x3, y3);

	// notches on body
	DrawMapLine(mx - dy * 0.4, my + dx * 0.4,
				mx + dy * 0.4, my - dx * 0.4);

	mx = static_cast<float>(mx - dx * 0.2);
	my = static_cast<float>(my - dy * 0.2);

	DrawMapLine(mx - dy * 0.4, my + dx * 0.4,
				mx + dy * 0.4, my - dx * 0.4);

	RenderThickness(1);
}


void UI_Canvas::DrawSnapPoint()
{
	// don't draw if an action is occurring
	if (inst.edit.action != EditorAction::nothing)
		return;

	if (inst.edit.split_line.valid())
		return;

	if (! Vis(snap_x, snap_y, 10))
		return;

	RenderColor(FL_CYAN);

	int sx = SCREENX(snap_x);
	int sy = SCREENY(snap_y);

	RenderRect(sx,   sy-2, 2, 6);
	RenderRect(sx-2, sy,   6, 2);
}


void UI_Canvas::DrawCurrentLine()
{
	if (inst.edit.drawLine.from.is_nil())
		return;

	const auto V = inst.level.vertices[inst.edit.drawLine.from.num];

	v2double_t newpos = inst.edit.drawLine.to;

	// should draw a vertex?
	if (! (inst.edit.highlight.valid() || inst.edit.split_line.valid()))
	{
		RenderColor(FL_GREEN);
		DrawVertex(newpos.x, newpos.y, vertex_radius(inst.grid.getScale()));
	}

	RenderColor(RED);
	DrawKnobbyLine(V->x(), V->y(), newpos.x, newpos.y);

	DrawLineInfo(V->x(), V->y(), newpos.x, newpos.y, inst.grid.getRatio() > 0);

	// draw all the crossing points
	crossing_state_c cross(inst);

	inst.level.hover.findCrossingPoints(cross,
					   V->xy(), inst.edit.drawLine.from.num,
		newpos, inst.edit.highlight.valid() ? inst.edit.highlight.num : -1);

	for (unsigned int k = 0 ; k < cross.points.size() ; k++)
	{
		cross_point_t& point = cross.points[k];

		// ignore current split line (what new vertex is sitting on)
		if (point.ld >= 0 && point.ld == inst.edit.split_line.num)
			continue;

		if (point.vert >= 0)
			RenderColor(FL_GREEN);
		else
			RenderColor(HI_AND_SEL_COL);

		DrawSplitPoint(point.pos);
	}
}


bool UI_Canvas::SelboxGet(v2double_t &pos1, v2double_t &pos2)
{
	pos1.x = std::min(inst.edit.selbox1.x, inst.edit.selbox2.x);
	pos1.y = std::min(inst.edit.selbox1.y, inst.edit.selbox2.y);
	pos2.x = std::max(inst.edit.selbox1.x, inst.edit.selbox2.x);
	pos2.y = std::max(inst.edit.selbox1.y, inst.edit.selbox2.y);

	int scr_dx = abs(SCREENX(pos2.x) - SCREENX(pos1.x));
	int scr_dy = abs(SCREENY(pos2.y) - SCREENY(pos1.y));

	// small boxes should be ignored (treated as a click + release)
	if (scr_dx < 5 && scr_dy < 5)
		return false;

	return true; // Ok
}


void UI_Canvas::SelboxDraw()
{
	double x1 = std::min(inst.edit.selbox1.x, inst.edit.selbox2.x);
	double x2 = std::max(inst.edit.selbox1.x, inst.edit.selbox2.x);
	double y1 = std::min(inst.edit.selbox1.y, inst.edit.selbox2.y);
	double y2 = std::max(inst.edit.selbox1.y, inst.edit.selbox2.y);

	RenderColor(FL_CYAN);

	DrawMapLine(x1, y1, x2, y1);
	DrawMapLine(x2, y1, x2, y2);
	DrawMapLine(x2, y2, x1, y2);
	DrawMapLine(x1, y2, x1, y1);
}


v2double_t UI_Canvas::DragDelta()
{
	v2double_t result = inst.edit.drag_cur.xy - inst.edit.drag_start.xy;

	v2double_t pixel_dpos = result * inst.grid.getScale();

	// check that we have moved far enough from the start position,
	// giving the user the option to select the original place.
	if (std::max(fabs(pixel_dpos.x), fabs(pixel_dpos.y)) < config::minimum_drag_pixels*2)
	{
		return {};
	}

	// handle ratio-lock of a single dragged vertex
	if (inst.edit.mode == ObjType::vertices && inst.grid.getRatio() > 0 &&
		inst.edit.dragged.num >= 0 && inst.edit.drag_other_vert >= 0)
	{
		const auto v0 = inst.level.vertices[inst.edit.drag_other_vert];
		const auto v1 = inst.level.vertices[inst.edit.dragged.num];

		v2double_t newpos = inst.edit.drag_cur.xy;

		inst.grid.RatioSnapXY(newpos, v0->xy());

		return newpos - v1->xy();
	}

	if (inst.grid.getRatio() > 0)
	{
		v2double_t newpos = inst.edit.drag_cur.xy;

		inst.grid.RatioSnapXY(newpos, inst.edit.drag_start.xy);

		return newpos - inst.edit.drag_start.xy;
	}

	if (inst.grid.snaps())
	{
		v2double_t focus = inst.edit.drag_focus.xy + result;

		result = inst.grid.Snap(focus) - inst.edit.drag_focus.xy;
	}
	return result;
}


//------------------------------------------------------------------------

void UI_Canvas::RenderSector(int num)
{
	if (! inst.Subdiv_SectorOnScreen(num, map_lx, map_ly, map_hx, map_hy))
		return;

	sector_subdivision_c *subdiv = inst.Subdiv_PolygonsForSector(num);

	if (! subdiv)
		return;


///  fprintf(stderr, "RenderSector %d\n", num);

	rgb_color_t light_col = SectorLightColor(inst.level.sectors[num]->light);
	bool light_and_tex = false;

	SString tex_name;

	Img_c * img = NULL;

	if (inst.edit.sector_render_mode == SREND_Lighting)
	{
		RenderColor(light_col);
	}
	else if (inst.edit.sector_render_mode == SREND_SoundProp)
	{
		if (inst.edit.mode != ObjType::sectors || !inst.edit.highlight.valid())
			return;

		const byte * prop = inst.SoundPropagation(inst.edit.highlight.num);

		switch ((propagate_level_e) prop[num])
		{
			case PGL_Never:   return;
			case PGL_Maybe:   RenderColor(fl_rgb_color(64,64,192)); break;
			case PGL_Level_1: RenderColor(fl_rgb_color(192,32,32)); break;
			case PGL_Level_2: RenderColor(fl_rgb_color(192,96,32)); break;
		}
	}
	else
	{
		if (inst.edit.sector_render_mode <= SREND_Ceiling)
			light_and_tex = true;

		if (inst.edit.sector_render_mode == SREND_Ceiling ||
			inst.edit.sector_render_mode == SREND_CeilBright)
			tex_name = inst.level.sectors[num]->CeilTex();
		else
			tex_name = inst.level.sectors[num]->FloorTex();

		if (inst.is_sky(tex_name))
		{
			RenderColor(inst.wad.palette.getPaletteColor(inst.conf.miscInfo.sky_color));
		}
		else
		{
			img = inst.wad.images.getMutableFlat(inst.conf, tex_name);

			if (! img)
			{
				img = &inst.wad.images.getMutableUnknownTexture(inst.conf);
			}
		}
	}

#ifdef NO_OPENGL
	int tw = img ? img->width()  : 1;
	int th = img ? img->height() : 1;

	const img_pixel_t *src_pix = img ? img->buf() : NULL;

	for (unsigned int i = 0 ; i < subdiv->polygons.size() ; i++)
	{
		sector_polygon_t *poly = &subdiv->polygons[i];

		float py1 = poly->my[1];  // north most
		float py2 = poly->my[0];

		int sy1 = SCREENY(py1);
		int sy2 = SCREENY(py2);

		// clip range to screen
		sy1 = std::max(sy1, y());
		sy2 = std::min(sy2, y() + h() - 1);

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
			float map_y = (float)MAPY(y);

			float lx = lx1 + (lx2 - lx1) * (map_y - py1) / (py2 - py1);
			float rx = rx1 + (rx2 - rx1) * (map_y - py1) / (py2 - py1);

			int sx1 = SCREENX(lx);
			int sx2 = SCREENX(rx);

			// clip span to screen
			sx1 = std::max(sx1, x());
			sx2 = std::min(sx2, x() + w() - 1);

			// reject spans completely off the screen
			if (sx2 < sx1)
				continue;

///  fprintf(stderr, "  span : y=%d  x=%d..%d\n", y, x1, x2);

			// solid color?
			if (! img)
			{
				RenderRect(sx1, y, sx2 - sx1 + 1, 1);
				continue;
			}

			int x = sx1;
			int span_w = sx2 - sx1 + 1;

			uint8_t *dest = rgb_buf + ((x - rgb_x) + (y - rgb_y) * rgb_w) * 3;
			uint8_t *dest_end = dest + span_w * 3;

			// the logic here for non-64x64 textures matches the software
			// 3D renderer, but is different than ZDoom (which scales them).
			int ty = (0 - (int)MAPY(y)) & (th - 1);

			if (light_and_tex)
			{
				int r = RGB_RED(light_col)   * 0x101;
				int g = RGB_GREEN(light_col) * 0x101;
				int b = RGB_BLUE(light_col)  * 0x101;

				for (; dest < dest_end ; dest += 3, x++)
				{
					int tx = (int)MAPX(x) & (tw - 1);

					img_pixel_t pix = src_pix[ty * tw + tx];

					inst.wad.palette.decodePixel(pix, dest[0], dest[1], dest[2]);

					dest[0] = (uint8_t)(((int)dest[0] * r) >> 16);
					dest[1] = (uint8_t)(((int)dest[1] * g) >> 16);
					dest[2] = (uint8_t)(((int)dest[2] * b) >> 16);
				}
			}
			else  // fullbright version
			{
				for (; dest < dest_end ; dest += 3, x++)
				{
					int tx = (int)MAPX(x) & (tw - 1);

					img_pixel_t pix = src_pix[ty * tw + tx];

					inst.wad.palette.decodePixel(pix, dest[0], dest[1], dest[2]);
				}
			}
		}
	}

#else // OpenGL
	if (img)
	{
		if (light_and_tex)
			RenderColor(light_col);
		else
			glColor3f(1, 1, 1);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);

		glAlphaFunc(GL_GREATER, 0.5);

		img->bind_gl(inst.wad);
	}
	else
	{
		// color was set above, set texture to solid white
		glBindTexture(GL_TEXTURE_2D, 0);
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
				glTexCoord2f(poly->mx[p] / 64.0f, poly->my[p] / 64.0f);
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


//------------------------------------------------------------------------
//  CUSTOM S/W DRAWING CODE
//------------------------------------------------------------------------

void UI_Canvas::PrepareToDraw()
{
#ifdef NO_OPENGL
	rgb_x = x();
	rgb_y = y();

	if (rgb_w != w() || rgb_h != h())
	{
		if (rgb_buf)
			delete[] rgb_buf;

		rgb_w = w();
		rgb_h = h();

		rgb_buf = new byte[rgb_w * rgb_h * 3];
	}
#endif
}


void UI_Canvas::Blit()
{
#ifdef NO_OPENGL
	fl_draw_image(rgb_buf, x(), y(), w(), h());
#endif
}


void UI_Canvas::RenderColor(Fl_Color c)
{
#ifdef NO_OPENGL
	Fl::get_color(c, cur_col.r, cur_col.g, cur_col.b);
#else
	gl_color(c);
#endif
}

void UI_Canvas::RenderFontSize(int size)
{
	cur_font = size;
}


void UI_Canvas::RenderThickness(int w)
{
#ifdef NO_OPENGL
	thickness = (w < 2) ? 1 : 2;
#else
	glLineWidth(static_cast<GLfloat>(w));
#endif
}


void UI_Canvas::RenderRect(int rx, int ry, int rw, int rh)
{
#ifndef NO_OPENGL
	gl_rectf(rx, ry, rw, rh);

#else
	// software version
	rx -= rgb_x;
	ry -= rgb_y;

	// clip to screen
	if (rx + rw > rgb_w)
	{
		rw = rgb_w - rx;
	}
	if (rx < 0)
	{
		rw += rx;
		rx = 0;
	}
	if (rw <= 0)
		return;

	if (ry + rh > rgb_h)
	{
		rh = rgb_h - ry;
	}
	if (ry < 0)
	{
		rh += ry;
		ry = 0;
	}
	if (rh <= 0)
		return;

	// fast method for greyscale (especially BLACK)
	if (cur_col.r == cur_col.g && cur_col.g == cur_col.b)
	{
		byte *dest = rgb_buf + (ry * rgb_w * 3) + (rx * 3);

		for ( ; rh > 0 ; rh--, dest += (rgb_w * 3))
			memset(dest, cur_col.r, rw * 3);

		return;
	}

	// slower method for all other colors
	byte *base = rgb_buf + (ry * rgb_w * 3) + (rx * 3);

	for ( ; rh > 0 ; rh--, base += (rgb_w * 3))
	{
		byte *dest = base;

		for (int w2 = rw ; w2 > 0 ; w2--)
		{
			*dest++ = cur_col.r;
			*dest++ = cur_col.g;
			*dest++ = cur_col.b;
		}
	}
#endif
}


#ifdef NO_OPENGL
enum outcode_flags_e
{
	O_TOP    = 1,
	O_BOTTOM = 2,
	O_LEFT   = 4,
	O_RIGHT  = 8,
};

int UI_Canvas::Calc_Outcode(int x, int y)
{
	return
		((y < 0)      ? O_TOP    : 0) |
		((y >= rgb_h) ? O_BOTTOM : 0) |
		((x < 0)      ? O_LEFT   : 0) |
		((x >= rgb_w) ? O_RIGHT  : 0);
}
#endif // NO_OPENGL


void UI_Canvas::RenderLine(int x1, int y1, int x2, int y2)
{
#ifndef NO_OPENGL
	glBegin(GL_LINE_STRIP);
	glVertex2i(x1, y1);
	glVertex2i(x2, y2);
	glEnd();
#else
	// software line drawing
	if (x1 == x2)
	{
		if (y1 > y2)
			std::swap(y1, y2);

		RenderRect(x1, y1, thickness, y2 - y1 + thickness);
		return;
	}
	if (y1 == y2)
	{
		if (x1 > x2)
			std::swap(x1, x2);

		RenderRect(x1, y1, x2 - x1 + thickness, thickness);
		return;
	}


	// completely off the screen?
	x1 -= rgb_x; y1 -= rgb_y;
	x2 -= rgb_x; y2 -= rgb_y;

	int out1 = Calc_Outcode(x1, y1);
	int out2 = Calc_Outcode(x2, y2);

	if (out1 & out2)
		return;


	// clip diagonal line to the map
	// (this is the Cohen-Sutherland clipping algorithm)

	while (out1 | out2)
	{
		// may be partially inside box, find an outside point
		int outside = (out1 ? out1 : out2);

		int dx = x2 - x1;
		int dy = y2 - y1;

		// this almost certainly cannot happen, but for the sake of
		// robustness we check anyway (just in case)
		if (dx == 0 && dy == 0)
			return;

		int new_x, new_y;

		// clip to each side
		if (outside & O_TOP)
		{
			new_y = 0;
			new_x = x1 + dx * (new_y - y1) / dy;
		}
		else if (outside & O_BOTTOM)
		{
			new_y = rgb_h-1;
			new_x = x1 + dx * (new_y - y1) / dy;
		}
		else if (outside & O_LEFT)
		{
			new_x = 0;
			new_y = y1 + dy * (new_x - x1) / dx;
		}
		else
		{
			SYS_ASSERT(outside & O_RIGHT);

			new_x = rgb_w-1;
			new_y = y1 + dy * (new_x - x1) / dx;
		}

		if (out1)
		{
			x1 = new_x;
			y1 = new_y;

			out1 = Calc_Outcode(x1, y1);
		}
		else
		{
			SYS_ASSERT(out2);

			x2 = new_x;
			y2 = new_y;

			out2 = Calc_Outcode(x2, y2);
		}

		if (out1 & out2)
			return;
	}


	// this is the Bresenham line drawing algorithm
	// (based on code from am_map.c in the GPL DOOM source)

	int dx = x2 - x1;
	int dy = y2 - y1;

	int ax = 2 * (dx < 0 ? -dx : dx);
	int ay = 2 * (dy < 0 ? -dy : dy);

	int sx = dx < 0 ? -1 : 1;
	int sy = dy < 0 ? -1 : 1;

	int x = x1;
	int y = y1;

	if (ax > ay)  // horizontal stepping
	{
		int d = ay - ax/2;

		raw_pixel(x, y);
		if (thickness == 2 && y+1 < rgb_h) raw_pixel(x, y+1);

		while (x != x2)
		{
			if (d>=0)
			{
				y += sy;
				d -= ax;
			}

			x += sx;
			d += ay;

			raw_pixel(x, y);
			if (thickness == 2 && y+1 < rgb_h) raw_pixel(x, y+1);
		}
	}
	else   // vertical stepping
	{
		int d = ax - ay/2;

		raw_pixel(x, y);
		if (thickness == 2 && x+1 < rgb_w) raw_pixel(x+1, y);

		while (y != y2)
		{
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}

			y += sy;
			d += ax;

			raw_pixel(x, y);
			if (thickness == 2 && x+1 < rgb_w) raw_pixel(x+1, y);
		}
	}
#endif
}


void UI_Canvas::RenderNumString(int x, int y, const char *s)
{
	// NOTE: string is limited to the digits '0' to '9', spaces,
	//       and the characters '-', '.' and ':'.

	Img_c *font_img;
	int font_cw;
	int font_ch;
	int font_step;

	if (cur_font < 17)
	{
		font_img  = &inst.wad.images.IM_DigitFont_11x14();
		font_cw   = 11;
		font_ch   = 14;
		font_step = font_cw - 2;
	}
	else
	{
		font_img  = &inst.wad.images.IM_DigitFont_14x19();
		font_cw   = 14;
		font_ch   = 19;
		font_step = font_cw - 2;
	}

#ifndef NO_OPENGL
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);

	glAlphaFunc(GL_GREATER, 0.5);

	// bind the sprite image (upload it to OpenGL if needed)
	font_img->bind_gl(inst.wad);
#endif

	// compute total size
	int total_w = static_cast<int>(strlen(s) * font_step + 2);

	// center the string at the given coordinate
	x -= total_w / 2;
	y -= font_ch / 2;

	for ( ; *s ; s++, x += font_step)
	{
		int ch = (*s & 0x7f);
		if (ch == ' ')
			continue;

		if ('0' <= ch && ch <= '9')
			ch -= '0';
		else if (ch == ':')
			ch = 10;
		else if (ch == '.')
			ch = 11;
		else if (ch == '^')
			ch = 13;
		else // '-'
			ch = 12;

		RenderFontChar(x, y, font_img, ch * font_cw, 0, font_cw, font_ch);
	}

#ifndef NO_OPENGL
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
#endif
}


void UI_Canvas::RenderFontChar(int rx, int ry, Img_c *img, int ix, int iy, int iw, int ih)
{
#ifdef NO_OPENGL
	// software rendering

	rx -= rgb_x;
	ry -= rgb_y;

	// clip to screen
	int sx1 = std::max(rx, 0);
	int sy1 = std::max(ry, 0);

	int sx2 = std::min(rx + iw, rgb_w) - 1;
	int sy2 = std::min(ry + ih, rgb_h) - 1;

	if (sx1 >= sx2 || sy1 >= sy2)
		return;

	for (int sy = sy1 ; sy <= sy2 ; sy++, iy++)
	{
		const img_pixel_t *src = img->buf() + (ix + iy * img->width());

		byte *dest = rgb_buf + 3 * (sx1 + sy * rgb_w);

		for (int sx = sx1 ; sx <= sx2 ; sx++, dest += 3)
		{
			img_pixel_t pix = *src++;

			if (pix != TRANS_PIXEL)
			{
				inst.wad.palette.decodePixel(pix, dest[0], dest[1], dest[2]);
			}
		}
	}

#else // OpenGL
	int rx2 = rx + iw;
	int ry2 = ry + ih;

	int img_w, img_h;

	if (global::use_npot_textures)
	{
		img_w = img->width();
		img_h = img->height();
	}
	else
	{
		img_w = RoundPOW2(img->width());
		img_h = RoundPOW2(img->height());
	}

	float tx1 = (float)ix / (float)img_w;
	float ty1 = (float)iy / (float)img_h;
	float tx2 = (float)(ix + iw) / (float)img_w;
	float ty2 = (float)(iy + ih) / (float)img_h;

	glColor3f(1, 1, 1);

	glBegin(GL_QUADS);

	glTexCoord2f(tx1, ty1); glVertex2i(rx,  ry);
	glTexCoord2f(tx1, ty2); glVertex2i(rx,  ry2);
	glTexCoord2f(tx2, ty2); glVertex2i(rx2, ry2);
	glTexCoord2f(tx2, ty1); glVertex2i(rx2, ry);

	glEnd();
#endif
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
