//------------------------------------------------------------------------
//  Editing Canvas
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2013 Andrew Apted
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

#include "ui_window.h"

#include "editloop.h"
#include "e_sector.h"
#include "e_things.h"
#include "m_game.h"
#include "r_grid.h"
#include "im_color.h"
#include "levels.h"
#include "r_render.h"
#include "selectn.h"


#define CAMERA_COLOR  fl_rgb_color(255, 192, 255)


extern int active_when;
extern int active_wmask;


//
// UI_Canvas Constructor
//
UI_Canvas::UI_Canvas(int X, int Y, int W, int H, const char *label) : 
    Fl_Widget(X, Y, W, H, label),
    render3d(false),
	highlight(), split_ld(-1),
	selbox_active(false),
	drag_active(false), drag_lines(),
	scale_active(false), scale_lines(),
	seen_sectors(8)
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

	if (render3d)
		Render3D_Draw(x(), y(), w(), h());
	else
		DrawEverything();

	fl_pop_clip();
}


int UI_Canvas::handle(int event)
{
	//// fprintf(stderr, "HANDLE EVENT %d\n", event);

	switch (event)
	{
		case FL_FOCUS:
			return 1;

		case FL_ENTER:
			// we greedily grab the focus
			if (Fl::focus() != this)
				take_focus(); 

			return 1;

		case FL_LEAVE:
			Editor_LeaveWindow();
			redraw();
			return 1;

		case FL_KEYDOWN:
		case FL_SHORTCUT:
			return handle_key();

		case FL_DRAG:
			if (Fl::event_button3())
			{
				RightButtonScroll(2);
				return 1;
			}
			/* FALL THROUGH... */

		case FL_MOVE:
			return handle_move(event == FL_DRAG);

		case FL_PUSH:
			return handle_push();

		case FL_RELEASE:
			return handle_release();

		case FL_MOUSEWHEEL:
			return handle_wheel();

		default:
			break;
	}

	return 0;  // unused
}


int UI_Canvas::handle_key()
{
	keycode_t key = M_TranslateKey(Fl::event_key(), Fl::event_state());

	if (key == 0)
		return 0;

#if 0  // DEBUG
	fprintf(stderr, "Key code: 0x%08x : %s\n", key, M_KeyToString(key));
#endif

	// keyboard propagation logic

	// handle digits specially
	if ('1' <= (key & FL_KEY_MASK) && (key & FL_KEY_MASK) <= '9')
	{
		Editor_DigitKey(key);
		return 1;
	}

	if (main_win->browser->visible() && ExecuteKey(key, KCTX_Browser))
		return 1;

	if (render3d && ExecuteKey(key, KCTX_Render))
		return 1;

	if (ExecuteKey(key, M_ModeToKeyContext(edit.obj_type)))
		return 1;
	
	if (ExecuteKey(key, KCTX_General))
		return 1;


	// NOTE: the key may still get handled by something (e.g. Menus)
	// fprintf(stderr, "Unknown key %d (0x%04x)\n", key, key);

	return 0;
}


int UI_Canvas::handle_move(bool drag)
{
	int mod = Fl::event_state() & MOD_ALL_MASK;

	if (render3d)
	{ /* TODO */ }
	else
	{
		Editor_MouseMotion(Fl::event_x(), Fl::event_y(), mod,
				MAPX(Fl::event_x()), MAPY(Fl::event_y()), drag);
	}

	return 1;
}


int UI_Canvas::handle_push()
{
	// FIXME: THIS IS REALLY SHIT
	if (Fl::event_button() == 3)
	{
		RightButtonScroll(1);
		return 1;
	}

	int mod = Fl::event_state() & MOD_ALL_MASK;

	if (Fl::event_button() == 2)
	{
		Editor_MiddlePress(mod);
	}
	else if (! render3d)
	{
		Editor_MousePress(mod);
	}

	return 1;
}


int UI_Canvas::handle_release()
{
	if (Fl::event_button() == 3)
	{
		RightButtonScroll(0);
		return 1;
	}

	if (Fl::event_button() == 2)
		Editor_MiddleRelease();
	else if (! render3d)
		Editor_MouseRelease();
	return 1;
}


int UI_Canvas::handle_wheel()
{
	int dx = Fl::event_dx();
	int dy = Fl::event_dy();

	keycode_t mod = Fl::event_state() & MOD_ALL_MASK;

	if (render3d)
		Render3D_Wheel(0 - dy, mod);
	else
		Editor_Wheel(dx, dy, mod);

	return 1;
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
	if (edit.show_object_numbers && edit.obj_type == OBJ_SECTORS)
	{
		if (seen_sectors.size() < NumSectors)
			seen_sectors.resize(NumSectors);

		seen_sectors.clear_all();
	}

	DrawMap(); 

	DrawSelection(edit.Selected);

	if (drag_active && ! drag_lines.empty())
		DrawSelection(&drag_lines);
	else if (scale_active && ! scale_lines.empty())
		DrawSelection(&scale_lines);

	if (highlight())
	{
		Fl_Color hi_color = HI_COL;

		if (edit.Selected->get(highlight.num))
			hi_color = HI_AND_SEL_COL;

		DrawHighlight(highlight.type, highlight.num, hi_color);
	}

	if (selbox_active)
		SelboxDraw();
}


/*
  draw the actual game map
*/
void UI_Canvas::DrawMap()
{
	fl_color(FL_BLACK);
	fl_rectf(x(), y(), w(), h());

	// draw the grid first since it's in the background
	if (grid.shown)
		DrawGrid();

	if (false) // FIXME !!! DEBUG OPTION
		DrawMapBounds();

	DrawCamera();

	if (edit.obj_type != OBJ_THINGS)
		DrawThings();

	DrawLinedefs();

	if (edit.obj_type == OBJ_VERTICES)
		DrawVertices();

	if (edit.obj_type == OBJ_THINGS)
		DrawThings();

	if (edit.obj_type == OBJ_RADTRIGS)
		DrawRTS();
}


/*
 *  draw_grid - draw the grid in the background of the edit window
 */
void UI_Canvas::DrawGrid()
{
	int grid_step_1 = 1 * grid.step;    // Map units between dots
	int grid_step_2 = 8 * grid_step_1;  // Map units between dim lines
	int grid_step_3 = 8 * grid_step_2;  // Map units between bright lines

	float pixels_1 = grid.step * grid.Scale;


	if (pixels_1 < 1.99)
	{
		fl_color(GRID_DARK);
		fl_rectf(x(), y(), w(), h());
		return;
	}


	// alternate grid mode : simple squares
	if (grid.mode == 1)
	{
		fl_color(GRID_BRIGHT);

		DrawMapLine(0, map_ly, 0, map_hy);
		DrawMapLine(map_lx, 0, map_hx, 0);

		fl_color(GRID_MEDIUM);

		int gx = (map_lx / grid_step_1) * grid_step_1;

		for (; gx <= map_hx; gx += grid_step_1)
			if (gx != 0)
				DrawMapLine(gx, map_ly, gx, map_hy);

		int gy = (map_ly / grid_step_1) * grid_step_1;

		for (; gy <= map_hy; gy += grid_step_1)
			if (gy != 0)
				DrawMapLine(map_lx, gy, map_hx, gy);

		return;
	}


	fl_color(GRID_BRIGHT);
	{
		int gx = (map_lx / grid_step_3) * grid_step_3;

		for (; gx <= map_hx; gx += grid_step_3)
			DrawMapLine(gx, map_ly-2, gx, map_hy+2);

		int gy = (map_ly / grid_step_3) * grid_step_3;

		for (; gy <=  map_hy; gy += grid_step_3)
			DrawMapLine(map_lx, gy, map_hx, gy);
	}


	fl_color(GRID_MEDIUM);
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


	// POINTS

	if (pixels_1 < 3.99)
		fl_color(GRID_MEDIUM);
	//??  else if (pixels_1 > 30.88)
	//??    fl_color(GRID_BRIGHT);
	else
		fl_color(GRID_POINT);

	{
		int gx = (map_lx / grid_step_1) * grid_step_1;
		int gy = (map_ly / grid_step_1) * grid_step_1;

		for (int ny = gy; ny <= map_hy; ny += grid_step_1)
		for (int nx = gx; nx <= map_hx; nx += grid_step_1)
		{
			int sx = SCREENX(nx);
			int sy = SCREENY(ny);

			if (pixels_1 < 30.99)
				fl_point(sx, sy);
			else
			{
				fl_line(sx-0, sy, sx+1, sy);
				fl_line(sx, sy-0, sx, sy+1);
			}
		}
	}
}


void UI_Canvas::DrawMapBounds()
{
	fl_color(FL_RED);

	DrawMapLine(MapBound_lx, MapBound_ly, MapBound_hx, MapBound_ly);
	DrawMapLine(MapBound_lx, MapBound_hy, MapBound_hx, MapBound_hy);

	DrawMapLine(MapBound_lx, MapBound_ly, MapBound_lx, MapBound_hy);
	DrawMapLine(MapBound_hx, MapBound_ly, MapBound_hx, MapBound_hy);
}


/*
 *  vertex_radius - apparent radius of a vertex, in pixels
 */
int vertex_radius(double scale)
{
	int r = 6 * (0.26 + scale / 2);

	if (r > 12) r = 12;

	return r;
}



/*
 *  draw_vertices - draw the vertices, and possibly their numbers
 */
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


/*
 *  draw_linedefs - draw the linedefs
 */
void UI_Canvas::DrawLinedefs()
{
	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		int x1 = LineDefs[n]->Start()->x;
		int y1 = LineDefs[n]->Start()->y;
		int x2 = LineDefs[n]->End  ()->x;
		int y2 = LineDefs[n]->End  ()->y;

		if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
			continue;

		bool one_sided = (! LineDefs[n]->Left());
		// was this:
		// one_sided = (LineDefs[n]->flags & 1) ? true : false;

		switch (edit.obj_type)
		{
			case OBJ_VERTICES:
			{
				if (n == split_ld)
					fl_color(HI_AND_SEL_COL);
				else if (! LineDefs[n]->Right())
					fl_color(RED);
				else if (one_sided)
					fl_color(WHITE);
				else
					fl_color(LIGHTGREY);

				DrawKnobbyLine(x1, y1, x2, y2);
			}
			break;

			case OBJ_LINEDEFS:
			{
				if (LineDefs[n]->type != 0)
				{
					if (LineDefs[n]->tag != 0)
						fl_color(LIGHTMAGENTA);
					else
						fl_color(LIGHTGREEN);
				}
				else if (one_sided)
					fl_color(WHITE);
				else if (LineDefs[n]->flags & 1)
					fl_color(FL_CYAN);
				else
					fl_color(LIGHTGREY);

				// Signal errors by drawing the linedef in red. Needs work.
				// Tag on a typeless linedef
				if (LineDefs[n]->type == 0 && LineDefs[n]->tag != 0)
					fl_color(RED);
				// No first sidedef
				else if (! LineDefs[n]->Right())
					fl_color(RED);

				DrawKnobbyLine(x1, y1, x2, y2);
			}
			break;

			case OBJ_SECTORS:
			{
				int sd1 = LineDefs[n]->right;
				int sd2 = LineDefs[n]->left;
				
				int s1  = (sd1 < 0) ? OBJ_NO_NONE : SideDefs[sd1]->sector;
				int s2  = (sd2 < 0) ? OBJ_NO_NONE : SideDefs[sd2]->sector;

				if (sd1 < 0)
				{
					fl_color(RED);
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
					if (s1 != OBJ_NO_NONE)
						DrawSectorNum(x1, y1, x2, y2, SIDE_RIGHT, s1);

					if (s2 != OBJ_NO_NONE)
						DrawSectorNum(x1, y1, x2, y2, SIDE_LEFT,  s2);
				}
			}
			break;

			// OBJ_THINGS
			default:
			{
				if (one_sided)
					fl_color(WHITE);
				else
					fl_color(LIGHTGREY);

				DrawMapLine(x1, y1, x2, y2);
			}
			break;
		}
	}

	// draw the linedef numbers
	if (edit.obj_type == OBJ_LINEDEFS && edit.show_object_numbers)
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
		DrawMapArrow(x, y, angle * 182);
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


/*
 *  draw_things_squares - the obvious
 */
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

		if (edit.obj_type == OBJ_THINGS)
		{
			if (edit.error_mode)
			{
				fl_color(LIGHTGREY);
			}
			else if (active_wmask)
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
			else
				fl_color((Fl_Color) info->color);
		}

		int r = info->radius;

		DrawThing(x, y, r, Things[n]->angle, false);
	}

	// draw the thing numbers
	if (edit.obj_type == OBJ_THINGS && edit.show_object_numbers)
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


void UI_Canvas::DrawRTS()
{
  // TODO: DrawRTS
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


/*
 *  draw_obj_no - draw a number at screen coordinates (x, y)
 */
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

	fl_color(OBJ_NUM_COL);

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


void UI_Canvas::SplitLineSet(int ld)
{
	if (split_ld == ld)
		return;
	
	split_ld = ld;
	redraw();
}


void UI_Canvas::SplitLineForget()
{
	if (split_ld < 0)
		return;
	
	split_ld = -1;
	redraw();
}



/*
   highlight the selected object
*/
void UI_Canvas::DrawHighlight(int objtype, int objnum, Fl_Color col,
                              bool do_tagged, int dx, int dy)
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

			DrawThing(x, y, r * 3 / 2, Things[objnum]->angle, true);
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

			int mx = (x1 + x2) / 2;
			int my = (y1 + y2) / 2;

			DrawMapLine(mx, my, mx + (y2 - y1) / 5, my + (x1 - x2) / 5);

			fl_line_style(FL_SOLID, 2);

			DrawMapVector(x1, y1, x2, y2);

			fl_line_style(FL_SOLID);
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
				if (! LineDefs[n]->TouchesSector(objnum))
					continue;

				int x1 = dx + LineDefs[n]->Start()->x;
				int y1 = dy + LineDefs[n]->Start()->y;
				int x2 = dx + LineDefs[n]->End  ()->x;
				int y2 = dy + LineDefs[n]->End  ()->y;

				if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
					continue;

				DrawMapLine(x1, y1, x2, y2);
			}

			fl_line_style(FL_SOLID);
		}
		break;
	}
}


void UI_Canvas::DrawHighlightScaled(int objtype, int objnum, Fl_Color col)
{
	fl_color(col);

	int vert_r = vertex_radius(grid.Scale);

	switch (objtype)
	{
		case OBJ_THINGS:
		{
			int x = Things[objnum]->x;
			int y = Things[objnum]->y;

			scale_param.Apply(&x, &y);

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

			scale_param.Apply(&x, &y);

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

			scale_param.Apply(&x1, &y1);
			scale_param.Apply(&x2, &y2);

			if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
				return;

			int mx = (x1 + x2) / 2;
			int my = (y1 + y2) / 2;

			DrawMapLine(mx, my, mx + (y2 - y1) / 5, my + (x1 - x2) / 5);

			fl_line_style(FL_SOLID, 2);

			DrawMapVector(x1, y1, x2, y2);

			fl_line_style(FL_SOLID);
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

				scale_param.Apply(&x1, &y1);
				scale_param.Apply(&x2, &y2);

				if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
					continue;

				DrawMapLine(x1, y1, x2, y2);
			}

			fl_line_style(FL_SOLID);
		}
		break;
	}
}


/*
   highlight the selected objects
*/
void UI_Canvas::DrawSelection(selection_c * list)
{
	if (! list)
		return;

	selection_iterator_c it;

	if (scale_active)
	{
		for (list->begin(&it) ; !it.at_end() ; ++it)
		{
			DrawHighlightScaled(list->what_type(), *it, SEL_COL);
		}

		return;
	}

	int dx = 0;
	int dy = 0;

	if (drag_active)
	{
		DragDelta(&dx, &dy);
	}

	for (list->begin(&it) ; !it.at_end() ; ++it)
	{
		DrawHighlight(list->what_type(), *it, edit.error_mode ? FL_RED : SEL_COL,
		              ! edit.error_mode /* do_tagged */, dx, dy);
	}
}


/*
 *  draw_map_point - draw a point at map coordinates
 *
 *  The point is drawn at map coordinates (<map_x>, <map_y>)
 */
void UI_Canvas::DrawMapPoint(int map_x, int map_y)
{
    fl_point(SCREENX(map_x), SCREENY(map_y));
}


/*
 *  DrawMapLine - draw a line on the screen from map coords
 */
void UI_Canvas::DrawMapLine(int map_x1, int map_y1, int map_x2, int map_y2)
{
    fl_line(SCREENX(map_x1), SCREENY(map_y1),
            SCREENX(map_x2), SCREENY(map_y2));
}


void UI_Canvas::DrawKnobbyLine(int map_x1, int map_y1, int map_x2, int map_y2)
{
	int x1 = SCREENX(map_x1);
	int y1 = SCREENY(map_y1);
	int x2 = SCREENX(map_x2);
	int y2 = SCREENY(map_y2);

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


/*
 *  DrawMapVector - draw an arrow on the screen from map coords
 */
void UI_Canvas::DrawMapVector (int map_x1, int map_y1, int map_x2, int map_y2)
{
	int scrx1 = SCREENX (map_x1);
	int scry1 = SCREENY (map_y1);
	int scrx2 = SCREENX (map_x2);
	int scry2 = SCREENY (map_y2);

	double r  = hypot((double) (scrx1 - scrx2), (double) (scry1 - scry2));
#if 0
	/* AYM 19980216 to avoid getting huge arrowheads when zooming in */
	int    scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
	int    scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
#else
	int scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (grid.Scale / 2)) : 0;
	int scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (grid.Scale / 2)) : 0;
#endif

	fl_line(scrx1, scry1, scrx2, scry2);

	scrx1 = scrx2 + 2 * scrXoff;
	scry1 = scry2 + 2 * scrYoff;

	fl_line(scrx1 - scrYoff, scry1 + scrXoff, scrx2, scry2);
	fl_line(scrx1 + scrYoff, scry1 - scrXoff, scrx2, scry2);
}


/*
 *  DrawMapArrow - draw an arrow on the screen from map coords and angle (0 - 65535)
 */
void UI_Canvas::DrawMapArrow(int map_x1, int map_y1, unsigned angle)
{
	int map_x2 = map_x1 + (int) (50 * cos(angle / 10430.37835));
	int map_y2 = map_y1 + (int) (50 * sin(angle / 10430.37835));
	int scrx1 = SCREENX(map_x1);
	int scry1 = SCREENY(map_y1);
	int scrx2 = SCREENX(map_x2);
	int scry2 = SCREENY(map_y2);

	double r = hypot(scrx1 - scrx2, scry1 - scry2);
#if 0
	int    scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
	int    scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
#else
	int scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (grid.Scale / 2)) : 0;
	int scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (grid.Scale / 2)) : 0;
#endif

	fl_line(scrx1, scry1, scrx2, scry2);

	scrx1 = scrx2 + 2 * scrXoff;
	scry1 = scry2 + 2 * scrYoff;

	fl_line(scrx1 - scrYoff, scry1 + scrXoff, scrx2, scry2);
	fl_line(scrx1 + scrYoff, scry1 - scrXoff, scrx2, scry2);
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


void UI_Canvas::SelboxBegin(int map_x, int map_y)
{
	selbox_active = true;
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
	selbox_active = false;

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
	drag_active = true;

	drag_start_x = map_x;
	drag_start_y = map_y;

	drag_focus_x = focus_x;
	drag_focus_y = focus_y;

	drag_cur_x = drag_start_x;
	drag_cur_y = drag_start_y;

	if (edit.obj_type == OBJ_VERTICES)
	{
		drag_lines.change_type(OBJ_LINEDEFS);

		ConvertSelection(edit.Selected, &drag_lines);
	}
}

void UI_Canvas::DragFinish(int *dx, int *dy)
{
	drag_active = false;

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

	// no real dragging until mouse distance has moved
	int scr_dx = SCREENX(drag_cur_x) - SCREENX(drag_start_x);
	int scr_dy = SCREENY(drag_cur_y) - SCREENY(drag_start_y);

	if (abs(scr_dx) < 6 && abs(scr_dy) < 6)  // TODO: CONFIG ITEM
	{
		*dx = *dy = 0;
		return;
	}

	if (grid.snap)
	{
		int focus_x = drag_focus_x + *dx;
		int focus_y = drag_focus_y + *dy;

		*dx = grid.SnapX(focus_x) - drag_focus_x;
		*dy = grid.SnapX(focus_y) - drag_focus_y;
	}
}


void UI_Canvas::ScaleBegin(int map_x, int map_y, int middle_x, int middle_y)
{
	scale_active = true;

	scale_start_x = map_x;
	scale_start_y = map_y;

	scale_param.Clear();

	scale_param.mid_x = middle_x;
	scale_param.mid_y = middle_y;

	if (edit.obj_type == OBJ_VERTICES)
	{
		scale_lines.change_type(OBJ_LINEDEFS);

		ConvertSelection(edit.Selected, &scale_lines);
	}
}

void UI_Canvas::ScaleFinish(scale_param_t& param)
{
	scale_active = false;

	scale_lines.clear_all();

	param = scale_param;
}

void UI_Canvas::ScaleUpdate(int map_x, int map_y, keycode_t mod)
{
	int dx1 = map_x - scale_param.mid_x;
	int dy1 = map_y - scale_param.mid_y;

	int dx2 = scale_start_x - scale_param.mid_x;
	int dy2 = scale_start_y - scale_param.mid_y;

	bool any_aspect = (mod & MOD_ALT)     ? true : false;
	bool rotate     = (mod & MOD_COMMAND) ? true : false;

	scale_param.rotate = 0;

	if (rotate)
	{
		int angle1 = (int)ComputeAngle(dx1, dy1);
		int angle2 = (int)ComputeAngle(dx2, dy2);

		scale_param.rotate = angle1 - angle2;

//		fprintf(stderr, "angle diff : %1.2f\n", scale_rotate * 360.0 / 65536.0);
	}

	if (rotate)  // TODO: CONFIG ITEM: rotate_with_scale
	{
		// no scaling
		dx1 = dx2 = 10;
		dy1 = dy2 = 10;
	}
	else if (rotate || !any_aspect)
	{
		dx1 = MAX(abs(dx1), abs(dy1));
		dx2 = MAX(abs(dx2), abs(dy2));

		dy1 = dx1;
		dy2 = dx2;
	}

	scale_param.scale_x = dx2 ? (dx1 / (float)dx2) : 1.0;
	scale_param.scale_y = dy2 ? (dy1 / (float)dy2) : 1.0;

	redraw();
}


void UI_Canvas::ChangeRenderMode(int mode)
{
	render3d = mode ? true : false;

	redraw();
}


void UI_Canvas::RightButtonScroll(int mode)
{
	keycode_t mod = Fl::event_state() & MOD_ALL_MASK;

	if (mode == 0)
		main_win->SetCursor(FL_CURSOR_DEFAULT);

	else if (mode == 1)
		main_win->SetCursor(FL_CURSOR_HAND);

	else if (mode == 2)
	{
		int dx = Fl::event_x() - rbscroll_x;
		int dy = Fl::event_y() - rbscroll_y;

		if (render3d)
		{
			Render3D_RBScroll(dx, dy, mod);
		}
		else
		{
			int speed = 8;  // FIXME: CONFIG OPTION

			if (mod == MOD_SHIFT)
				speed /= 2;
			else if (mod == MOD_COMMAND)
				speed *= 2;

			grid.orig_x -= ((double) dx * speed / 8.0 / grid.Scale);
			grid.orig_y += ((double) dy * speed / 8.0 / grid.Scale);

			redraw();
		}
	}

	rbscroll_x = Fl::event_x();
	rbscroll_y = Fl::event_y();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
