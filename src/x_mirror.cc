//------------------------------------------------------------------------
//  MIRROR / ROTATE / ETC OPS
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include <map>

#include "m_bitvec.h"
#include "e_linedef.h"
#include "e_sector.h"
#include "e_things.h"
#include "e_vertex.h"
#include "editloop.h"
#include "levels.h"
#include "m_game.h"
#include "m_select.h"
#include "r_grid.h"
#include "selectn.h"
#include "x_mirror.h"


void scale_param_t::Clear()
{
	mid_x = mid_y = 0;

	scale_x = scale_y = 1;

	rotate = 0;
}


void scale_param_t::Apply(int *x, int *y) const
{
	*x = *x - mid_x;
	*y = *y - mid_y;

	if (rotate)
	{
		float s = sin(rotate * M_PI / 32768.0);
		float c = cos(rotate * M_PI / 32768.0);

		int x1 = *x;
		int y1 = *y;

		*x = x1 * c - y1 * s;
		*y = y1 * c + x1 * s;
	}

	*x = mid_x + I_ROUND( (*x) * scale_x );
	*y = mid_y + I_ROUND( (*y) * scale_y );
}


//
// Return the coordinate of the centre of a group of objects.
//
// This is computed using an average of all the coordinates, which can
// often give a different result than using the middle of the bounding
// box.
//
void Objs_CalcMiddle(selection_c * list, int *x, int *y)
{
	*x = *y = 0;

	if (list->empty())
		return;

	selection_iterator_c it;

	double sum_x = 0;
	double sum_y = 0;

	int count = 0;

	switch (list->what_type())
	{
		case OBJ_THINGS:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it, ++count)
			{
				sum_x += Things[*it]->x;
				sum_y += Things[*it]->y;
			}
			break;
		}

		case OBJ_VERTICES:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it, ++count)
			{
				sum_x += Vertices[*it]->x;
				sum_y += Vertices[*it]->y;
			}
			break;
		}

		case OBJ_RADTRIGS:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it, ++count)
			{
				sum_x += RadTrigs[*it]->mx;
				sum_y += RadTrigs[*it]->my;
			}
			break;
		}

		// everything else: just use the vertices
		default:
		{
			selection_c verts(OBJ_VERTICES);

			ConvertSelection(list, &verts);

			Objs_CalcMiddle(&verts, x, y);
			return;
		}
	}

	SYS_ASSERT(count > 0);

	*x = I_ROUND(sum_x / count);
	*y = I_ROUND(sum_y / count);
}


/*
 *  Returns a bounding box that completely includes a group of objects
 */
void Objs_CalcBBox(selection_c * list, int *x1, int *y1, int *x2, int *y2)
{
	if (list->empty())
	{
		*x1 = *y1 = 0;
		*x2 = *y2 = 0;
		return;
	}

	*x1 = *y1 = +777777;
	*x2 = *y2 = -777777;

	selection_iterator_c it;

	switch (list->what_type())
	{
		case OBJ_THINGS:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it)
			{
				const Thing *T = Things[*it];

				const thingtype_t *info = M_GetThingType(T->type);
				int r = info->radius;

				if (T->x - r < *x1) *x1 = T->x - r;
				if (T->y - r < *y1) *y1 = T->y - r;
				if (T->x + r > *x2) *x2 = T->x + r;
				if (T->y + r > *y2) *y2 = T->y + r;
			}
			break;
		}

		case OBJ_VERTICES:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it)
			{
				const Vertex *V = Vertices[*it];

				if (V->x < *x1) *x1 = V->x;
				if (V->y < *y1) *y1 = V->y;
				if (V->x > *x2) *x2 = V->x;
				if (V->y > *y2) *y2 = V->y;
			}
			break;
		}

		case OBJ_RADTRIGS:
		{
			for (list->begin(&it) ; !it.at_end() ; ++it)
			{
				const RadTrig *R = RadTrigs[*it];

				int rx = (R->rw+1) / 2;
				int ry = (R->rh+1) / 2;

				if (R->mx - rx < *x1) *x1 = R->mx - rx;
				if (R->my - ry < *y1) *y1 = R->my - ry;
				if (R->mx + rx > *x2) *x2 = R->mx + rx;
				if (R->my + ry > *y2) *y2 = R->my + ry;
			}
			break;
		}

		// everything else: just use the vertices
		default:
		{
			selection_c verts(OBJ_VERTICES);

			ConvertSelection(list, &verts);

			Objs_CalcBBox(&verts, x1, y1, x2, y2);
			return;
		}
	}

	SYS_ASSERT(*x1 <= *x2);
	SYS_ASSERT(*y1 <= *y2);
}


static void DoMirrorThings(selection_c& list, bool is_vert, int mid_x, int mid_y)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		if (is_vert)
		{
			BA_ChangeTH(*it, Thing::F_Y, 2*mid_y - T->y);

			if (T->angle != 0)
				BA_ChangeTH(*it, Thing::F_ANGLE, 360 - T->angle);
		}
		else
		{
			BA_ChangeTH(*it, Thing::F_X, 2*mid_x - T->x);

			if (T->angle > 180)
				BA_ChangeTH(*it, Thing::F_ANGLE, 540 - T->angle);
			else
				BA_ChangeTH(*it, Thing::F_ANGLE, 180 - T->angle);
		}
	}
}


static void DoMirrorVertices(selection_c& list, bool is_vert, int mid_x, int mid_y)
{
	selection_c verts(OBJ_VERTICES);

	ConvertSelection(&list, &verts);

	selection_iterator_c it;

	for (verts.begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex * V = Vertices[*it];

		if (is_vert)
			BA_ChangeVT(*it, Vertex::F_Y, 2*mid_y - V->y);
		else
			BA_ChangeVT(*it, Vertex::F_X, 2*mid_x - V->x);
	}

	// flip linedefs too !!
	selection_c lines(OBJ_LINEDEFS);

	ConvertSelection(&verts, &lines);

	for (lines.begin(&it) ; !it.at_end() ; ++it)
	{
		LineDef * L = LineDefs[*it];

		int start = L->start;
		int end   = L->end;

		BA_ChangeLD(*it, LineDef::F_START, end);
		BA_ChangeLD(*it, LineDef::F_END, start);
	}
}


static void DoMirrorStuff(selection_c& list, bool is_vert, int mid_x, int mid_y)
{
	if (edit.obj_type == OBJ_THINGS)
	{
		DoMirrorThings(list, is_vert, mid_x, mid_y);
		return;
	}

	// everything else just modifies the vertices

	if (edit.obj_type == OBJ_SECTORS)
	{
		// handle things in Sectors mode too
		selection_c things(OBJ_THINGS);

		ConvertSelection(&list, &things);

		DoMirrorThings(things, is_vert, mid_x, mid_y);
	}

	DoMirrorVertices(list, is_vert, mid_x, mid_y);
}


void CMD_Mirror(void)
{
	selection_c list;

	if (! GetCurrentObjects(&list) || edit.obj_type == OBJ_RADTRIGS)
	{
		Beep("No objects to mirror");
		return;
	}

	bool is_vert = false;

	if (tolower(EXEC_Param[0][0]) == 'v')
		is_vert = true;

	int mid_x, mid_y;

	Objs_CalcMiddle(&list, &mid_x, &mid_y);
	
	BA_Begin();

	DoMirrorStuff(list, is_vert, mid_x, mid_y);

	BA_End();
}


static void DoRotate90Things(selection_c& list, bool anti_clockwise, int mid_x, int mid_y)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		int old_x = T->x;
		int old_y = T->y;

		if (anti_clockwise)
		{
			BA_ChangeTH(*it, Thing::F_X, mid_x - old_y + mid_y);
			BA_ChangeTH(*it, Thing::F_Y, mid_y + old_x - mid_x);

			BA_ChangeTH(*it, Thing::F_ANGLE, calc_new_angle(T->angle, +90));
		}
		else
		{
			BA_ChangeTH(*it, Thing::F_X, mid_x + old_y - mid_y);
			BA_ChangeTH(*it, Thing::F_Y, mid_y - old_x + mid_x);

			BA_ChangeTH(*it, Thing::F_ANGLE, calc_new_angle(T->angle, -90));
		}
	}
}


void CMD_Rotate90(void)
{
	bool anti_clockwise = atoi(EXEC_Param[0]) >= 0;

	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list) || edit.obj_type == OBJ_RADTRIGS)
	{
		Beep("No objects to rotate");
		return;
	}

	int mid_x, mid_y;

	Objs_CalcMiddle(&list, &mid_x, &mid_y);
	
	BA_Begin();

	if (edit.obj_type == OBJ_THINGS)
	{
		DoRotate90Things(list, anti_clockwise, mid_x, mid_y);
	}
	else
	{
		// handle things inside sectors
		if (edit.obj_type == OBJ_SECTORS)
		{
			selection_c things(OBJ_THINGS);

			ConvertSelection(&list, &things);

			DoRotate90Things(things, anti_clockwise, mid_x, mid_y);
		}

		// everything else just rotates the vertices
		selection_c verts(OBJ_VERTICES);

		ConvertSelection(&list, &verts);

		for (verts.begin(&it) ; !it.at_end() ; ++it)
		{
			const Vertex * V = Vertices[*it];

			int old_x = V->x;
			int old_y = V->y;

			if (anti_clockwise)
			{
				BA_ChangeVT(*it, Vertex::F_X, mid_x - old_y + mid_y);
				BA_ChangeVT(*it, Vertex::F_Y, mid_y + old_x - mid_x);
			}
			else
			{
				BA_ChangeVT(*it, Vertex::F_X, mid_x + old_y - mid_y);
				BA_ChangeVT(*it, Vertex::F_Y, mid_y - old_x + mid_x);
			}
		}
	}

	BA_End();
}
 

static void DoEnlargeThings(selection_c& list, int mul, int mid_x, int mid_y)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		int dx = T->x - mid_x;
		int dy = T->y - mid_y;

		BA_ChangeTH(*it, Thing::F_X, mid_x + dx * mul);
		BA_ChangeTH(*it, Thing::F_Y, mid_y + dy * mul);
	}
}


void CMD_Enlarge(void)
{
	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No objects to enlarge");
		return;
	}

	int mul = 2;
	if (EXEC_Param[0][0])
		mul = atoi(EXEC_Param[0]);

	if (mul < 1 || mul > 64)
	{
		Beep("Bad parameter for enlarge: '%s'", EXEC_Param[0]);
		return;
	}

	int mid_x, mid_y, hx, hy;

	// TODO: CONFIG ITEM
	if (true)
		Objs_CalcMiddle(&list, &mid_x, &mid_y);
	else
	{
		Objs_CalcBBox(&list, &mid_x, &mid_y, &hx, &hy);

		mid_x = mid_x + (hx - mid_x) / 2;
		mid_y = mid_y + (hy - mid_y) / 2;
	}

	BA_Begin();

	if (edit.obj_type == OBJ_RADTRIGS)
	{
		// Note: the positions of the trigger(s) are not changed
		for (list.begin(&it) ; !it.at_end() ; ++it)
		{
			const RadTrig * R = RadTrigs[*it];

			BA_ChangeRAD(*it, RadTrig::F_RW, R->rw * mul);
			BA_ChangeRAD(*it, RadTrig::F_RH, R->rh * mul);
		}
	}
	else if (edit.obj_type == OBJ_THINGS)
	{
		DoEnlargeThings(list, mul, mid_x, mid_y);
	}
	else
	{
		// handle things inside sectors
		if (edit.obj_type == OBJ_SECTORS)
		{
			selection_c things(OBJ_THINGS);

			ConvertSelection(&list, &things);

			DoEnlargeThings(things, mul, mid_x, mid_y);
		}

		// everything else just scales the vertices
		selection_c verts(OBJ_VERTICES);

		ConvertSelection(&list, &verts);

		for (verts.begin(&it) ; !it.at_end() ; ++it)
		{
			const Vertex * V = Vertices[*it];

			int dx = V->x - mid_x;
			int dy = V->y - mid_y;

			BA_ChangeVT(*it, Vertex::F_X, mid_x + dx * mul);
			BA_ChangeVT(*it, Vertex::F_Y, mid_y + dy * mul);
		}
	}

	BA_End();
}


static void DoShrinkThings(selection_c& list, int div, int mid_x, int mid_y)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		int dx = T->x - mid_x;
		int dy = T->y - mid_y;

		BA_ChangeTH(*it, Thing::F_X, mid_x + dx / div);
		BA_ChangeTH(*it, Thing::F_Y, mid_y + dy / div);
	}
}


void CMD_Shrink(void)
{
	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No objects to shrink");
		return;
	}

	int div = 2;
	if (EXEC_Param[0][0])
		div = atoi(EXEC_Param[0]);

	if (div < 1 || div > 64)
	{
		Beep("Bad parameter for shrink: '%s'", EXEC_Param[0]);
		return;
	}

	int mid_x, mid_y, hx, hy;

	// TODO: CONFIG ITEM
	if (true)
		Objs_CalcMiddle(&list, &mid_x, &mid_y);
	else
	{
		Objs_CalcBBox(&list, &mid_x, &mid_y, &hx, &hy);

		mid_x = mid_x + (hx - mid_x) / 2;
		mid_y = mid_y + (hy - mid_y) / 2;
	}

	BA_Begin();

	if (edit.obj_type == OBJ_RADTRIGS)
	{
		// Note: the positions of the trigger(s) are not changed
		for (list.begin(&it) ; !it.at_end() ; ++it)
		{
			const RadTrig * R = RadTrigs[*it];

			BA_ChangeRAD(*it, RadTrig::F_RW, MAX(1, R->rw / div));
			BA_ChangeRAD(*it, RadTrig::F_RH, MAX(1, R->rh / div));
		}
	}
	else if (edit.obj_type == OBJ_THINGS)
	{
		DoShrinkThings(list, div, mid_x, mid_y);
	}
	else
	{
		// handle things inside sectors
		if (edit.obj_type == OBJ_SECTORS)
		{
			selection_c things(OBJ_THINGS);

			ConvertSelection(&list, &things);

			DoShrinkThings(things, div, mid_x, mid_y);
		}

		// everything else just scales the vertices
		selection_c verts(OBJ_VERTICES);

		ConvertSelection(&list, &verts);

		for (verts.begin(&it) ; !it.at_end() ; ++it)
		{
			const Vertex * V = Vertices[*it];

			int dx = V->x - mid_x;
			int dy = V->y - mid_y;

			BA_ChangeVT(*it, Vertex::F_X, mid_x + dx / div);
			BA_ChangeVT(*it, Vertex::F_Y, mid_y + dy / div);
		}
	}

	BA_End();
}


static void DoScaleTwoThings(selection_c& list, scale_param_t& param)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		int new_x = T->x;
		int new_y = T->y;

		param.Apply(&new_x, &new_y);

		BA_ChangeTH(*it, Thing::F_X, new_x);
		BA_ChangeTH(*it, Thing::F_Y, new_y);

		float rot1 = param.rotate / 8192.0;

		int ang_diff = round(rot1) * 45.0;

		if (ang_diff)
		{
			BA_ChangeTH(*it, Thing::F_ANGLE, calc_new_angle(T->angle, ang_diff));
		}
	}
}


static void DoScaleTwoVertices(selection_c& list, scale_param_t& param)
{
	selection_c verts(OBJ_VERTICES);

	ConvertSelection(&list, &verts);

	selection_iterator_c it;

	for (verts.begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex * V = Vertices[*it];

		int new_x = V->x;
		int new_y = V->y;

		param.Apply(&new_x, &new_y);

		BA_ChangeVT(*it, Vertex::F_X, new_x);
		BA_ChangeVT(*it, Vertex::F_Y, new_y);
	}
}


static void DoScaleTwoStuff(selection_c& list, scale_param_t& param)
{
	if (edit.obj_type == OBJ_THINGS)
	{
		DoScaleTwoThings(list, param);
		return;
	}

	// everything else just modifies the vertices

	if (edit.obj_type == OBJ_SECTORS)
	{
		// handle things in Sectors mode too
		selection_c things(OBJ_THINGS);

		ConvertSelection(&list, &things);

		DoScaleTwoThings(things, param);
	}

	DoScaleTwoVertices(list, param);
}


void CMD_ScaleObjects2(scale_param_t& param)
{
	// this is called by the MOUSE2 dynamic scaling code

	SYS_ASSERT(edit.Selected->notempty());

	BA_Begin();

	if (param.scale_x < 0)
	{
		param.scale_x = -param.scale_x;

		DoMirrorStuff(*edit.Selected, false /* is_vert */, param.mid_x, param.mid_y);
	}

	if (param.scale_y < 0)
	{
		param.scale_y = -param.scale_y;

		DoMirrorStuff(*edit.Selected, true /* is_vert */, param.mid_x, param.mid_y);
	}

	DoScaleTwoStuff(*edit.Selected, param);

	BA_End();
}


static void DetermineOrigin(scale_param_t& param, int pos_x, int pos_y)
{
	if (pos_x == 0 && pos_y == 0)
	{
		Objs_CalcMiddle(edit.Selected, &param.mid_x, &param.mid_y);
		return;
	}

	int lx, ly, hx, hy;

	Objs_CalcBBox(edit.Selected, &lx, &ly, &hx, &hy);

	if (pos_x < 0)
		param.mid_x = lx;
	else if (pos_x > 0)
		param.mid_x = hx;
	else
		param.mid_x = lx + (hx - lx) / 2;

	if (pos_y < 0)
		param.mid_y = ly;
	else if (pos_y > 0)
		param.mid_y = hy;
	else
		param.mid_y = ly + (hy - ly) / 2;
}


void CMD_ScaleObjects3(double scale_x, double scale_y, int pos_x, int pos_y)
{
	SYS_ASSERT(scale_x > 0);
	SYS_ASSERT(scale_y > 0);

	scale_param_t param;

	param.Clear();

	param.scale_x = scale_x;
	param.scale_y = scale_y;

	DetermineOrigin(param, pos_x, pos_y);

	BA_Begin();
	{
		DoScaleTwoStuff(*edit.Selected, param);
	}
	BA_End();
}


static void DoScaleSectorHeights(selection_c& list, double scale_z, int pos_z)
{
	SYS_ASSERT(! list.empty());

	selection_iterator_c it;

	// determine Z range and origin
	int lz = +99999;
	int hz = -99999;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Sector * S = Sectors[*it];

		lz = MIN(lz, S->floorh);
		hz = MAX(hz, S->ceilh);
	}

	int mid_z;

	if (pos_z < 0)
		mid_z = lz;
	else if (pos_z > 0)
		mid_z = hz;
	else
		mid_z = lz + (hz - lz) / 2;

	// apply the scaling

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Sector * S = Sectors[*it];
		
		int new_f = mid_z + I_ROUND((S->floorh - mid_z) * scale_z);
		int new_c = mid_z + I_ROUND((S-> ceilh - mid_z) * scale_z);

		BA_ChangeSEC(*it, Sector::F_FLOORH, new_f);
		BA_ChangeSEC(*it, Sector::F_CEILH,  new_c);
	}
}

void CMD_ScaleObjects3(double scale_x, double scale_y, double scale_z,
                       int pos_x, int pos_y, int pos_z)
{
	SYS_ASSERT(edit.obj_type == OBJ_SECTORS);

	scale_param_t param;

	param.Clear();

	param.scale_x = scale_x;
	param.scale_y = scale_y;

	DetermineOrigin(param, pos_x, pos_y);

	BA_Begin();
	{
		DoScaleTwoStuff(*edit.Selected, param);
		DoScaleSectorHeights(*edit.Selected, scale_z, pos_z);
	}
	BA_End();
}


void CMD_RotateObjects3(double deg, int pos_x, int pos_y)
{
	scale_param_t param;

	param.Clear();

	param.rotate = round(deg * 65536.0 / 360.0);

	DetermineOrigin(param, pos_x, pos_y);

	BA_Begin();
	{
		DoScaleTwoStuff(*edit.Selected, param);
	}
	BA_End();
}


bool SpotInUse(obj_type_e obj_type, int map_x, int map_y)
{
	// FIXME: FindObjectAt(obj_type, map_x, map_y)

	switch (obj_type)
	{
		case OBJ_THINGS:
			for (int n = 0 ; n < NumThings ; n++)
				if (Things[n]->x == map_x && Things[n]->y == map_y)
					return true;
			return false;

		case OBJ_VERTICES:
			for (int n = 0 ; n < NumVertices ; n++)
				if (Vertices[n]->x == map_x && Vertices[n]->y == map_y)
					return true;
			return false;

		default:
			BugError("IsSpotVacant: bad object type\n");
			return false;
	}
}


static void Quantize_Things(selection_c& list)
{
	// remember the things which we moved
	// (since we cannot modify the selection while we iterate over it)
	selection_c moved(list.what_type());

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing * T = Things[*it];

		if (grid.OnGrid(T->x, T->y))
		{
			moved.set(*it);
			continue;
		}

		for (int pass = 0 ; pass < 4 ; pass++)
		{
			int new_x = grid.QuantSnapX(T->x, pass & 1);
			int new_y = grid.QuantSnapY(T->y, pass & 2);

			if (! SpotInUse(OBJ_THINGS, new_x, new_y))
			{
				BA_ChangeTH(*it, Thing::F_X, new_x);
				BA_ChangeTH(*it, Thing::F_Y, new_y);

				moved.set(*it);
				break;
			}
		}
	}

	list.unmerge(moved);

	if (list.notempty())
		Beep("Quantize: could not move %d things", list.count_obj());
}


static void Quantize_Vertices(selection_c& list)
{
	// first : do an analysis pass, remember vertices that are part
	// of a horizontal or vertical line (and both in the selection)
	// and limit the movement of those vertices to ensure the lines
	// stay horizontal or vertical.

	enum
	{
		V_HORIZ   = (1 << 0),
		V_VERT    = (1 << 1),
		V_DIAG_NE = (1 << 2),
		V_DIAG_SE = (1 << 3)
	};

	byte * vert_modes = new byte[NumVertices];

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		// require both vertices of the linedef to be in the selection
		if (! (list.get(L->start) && list.get(L->end)))
			continue;

		// IDEA: make this a method of LineDef
		int x1 = L->Start()->x;
		int y1 = L->Start()->y;
		int x2 = L->End()->x;
		int y2 = L->End()->y;

		if (y1 == y2)
		{
			vert_modes[L->start] |= V_HORIZ;
			vert_modes[L->end]   |= V_HORIZ;
		}
		else if (x1 == x2)
		{
			vert_modes[L->start] |= V_VERT;
			vert_modes[L->end]   |= V_VERT;
		}
		else if ((x1 < x2 && y1 < y2) || (x1 > x2 && y1 > y2))
		{
			vert_modes[L->start] |= V_DIAG_NE;
			vert_modes[L->end]   |= V_DIAG_NE;
		}
		else
		{
			vert_modes[L->start] |= V_DIAG_SE;
			vert_modes[L->end]   |= V_DIAG_SE;
		}
	}

	// remember the vertices which we moved
	// (since we cannot modify the selection while we iterate over it)
	selection_c moved(list.what_type());

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Vertex * V = Vertices[*it];

		if (grid.OnGrid(V->x, V->y))
		{
			moved.set(*it);
			continue;
		}

		byte mode = vert_modes[*it];

		for (int pass = 0 ; pass < 4 ; pass++)
		{
			int x_dir, y_dir;

			int new_x = grid.QuantSnapX(V->x, pass & 1, &x_dir);
			int new_y = grid.QuantSnapY(V->y, pass & 2, &y_dir);

			// keep horizontal lines horizontal
			if ((mode & V_HORIZ) && (pass & 2))
				continue;

			// keep vertical lines vertical
			if ((mode & V_VERT) && (pass & 1))
				continue;

			// TODO: keep diagonal lines diagonal...

			if (! SpotInUse(OBJ_VERTICES, new_x, new_y))
			{
				BA_ChangeVT(*it, Vertex::F_X, new_x);
				BA_ChangeVT(*it, Vertex::F_Y, new_y);

				moved.set(*it);
				break;
			}
		}
	}

	delete[] vert_modes;

	list.unmerge(moved);

	if (list.notempty())
		Beep("Quantize: could not move %d vertices", list.count_obj());
}


static void Quantize_RadTrigs(selection_c& list)
{
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const RadTrig * R = RadTrigs[*it];

		BA_ChangeRAD(*it, RadTrig::F_MX, grid.ForceSnapX(R->mx));
		BA_ChangeRAD(*it, RadTrig::F_MY, grid.ForceSnapY(R->my));

		int new_rw = grid.step * ((R->rw + grid.step - 1) / grid.step);
		int new_rh = grid.step * ((R->rh + grid.step - 1) / grid.step);

		BA_ChangeRAD(*it, RadTrig::F_RW, new_rw);
		BA_ChangeRAD(*it, RadTrig::F_RH, new_rh);
	}
}


void CMD_Quantize(void)
{
	if (edit.Selected->empty())
	{
		if (edit.highlighted.is_nil())
		{
			Beep("Nothing to quantize");
			return;
		}

		edit.Selected->set(edit.highlighted.num);
	}

	BA_Begin();

	switch (edit.obj_type)
	{
		case OBJ_THINGS:
			Quantize_Things(*edit.Selected);
			break;

		case OBJ_RADTRIGS:
			Quantize_RadTrigs(*edit.Selected);
			break;

		case OBJ_VERTICES:
			Quantize_Vertices(*edit.Selected);
			break;

		// everything else merely quantizes vertices
		default:
		{
			selection_c verts(OBJ_VERTICES);

			ConvertSelection(edit.Selected, &verts);

			Quantize_Vertices(verts);

			edit.Selected->clear_all();
			break;
		}
	}

	BA_End();

	UpdateHighlight();

	edit.RedrawMap = 1;
}


#if 0  // FIXME exchange_objects_numbers
/*
 *  exchange_objects_numbers
 *  Exchange the numbers of two objects
 *
 *  Return 0 on success, non-zero on failure.
 */
int exchange_objects_numbers (int obj_type, SelPtr list, bool adjust)
{
	int n1, n2;

	// Must have exactly two objects in the selection
	if (list == 0 || list->next == 0 || (list->next)->next != 0)
	{
		BugError("exchange_object_numbers: wrong objects count.");
		return 1;
	}
	n1 = list->objnum;
	n2 = (list->next)->objnum;

	if (obj_type == OBJ_LINEDEFS)
	{
		struct LineDef swap_buf;
		swap_buf = LineDefs[n1];
		LineDefs[n1] = LineDefs[n2];
		LineDefs[n2] = swap_buf;
	}
	else if (obj_type == OBJ_SECTORS)
	{
		struct Sector swap_buf;
		swap_buf = Sectors[n1];
		Sectors[n1] = Sectors[n2];
		Sectors[n2] = swap_buf;
		if (adjust)
		{
			for (int n = 0 ; n < NumSideDefs ; n++)
			{
				if (SideDefs[n].sector == n1)
					SideDefs[n].sector = n2;
				else if (SideDefs[n].sector == n2)
					SideDefs[n].sector = n1;
			}
		}
	}
	else if (obj_type == OBJ_THINGS)
	{
		struct Thing swap_buf;
		swap_buf = Things[n1];
		Things[n1] = Things[n2];
		Things[n2] = swap_buf;
	}
	else if (obj_type == OBJ_VERTICES)
	{
		struct Vertex swap_buf;
		swap_buf = Vertices[n1];
		Vertices[n1] = Vertices[n2];
		Vertices[n2] = swap_buf;
		if (adjust)
		{
			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				if (LineDefs[n].start == n1)
					LineDefs[n].start = n2;
				else if (LineDefs[n].start == n2)
					LineDefs[n].start = n1;
				if (LineDefs[n].end == n1)
					LineDefs[n].end = n2;
				else if (LineDefs[n].end == n2)
					LineDefs[n].end = n1;
			}
		}
	}
	return 0;
}
#endif


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
