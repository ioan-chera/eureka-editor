//------------------------------------------------------------------------
//  LEVEL LOADING ETC
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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

#include "m_bitvec.h"
#include "m_dialog.h"
#include "m_game.h"
#include "editloop.h"
#include "levels.h"
#include "e_things.h"
#include "w_rawdef.h"

#include "ui_window.h"


int MapBound_hx = -32767;   /* maximum X value of map */
int MapBound_hy = -32767;   /* maximum Y value of map */
int MapBound_lx = 32767;    /* minimum X value of map */
int MapBound_ly = 32767;    /* minimum Y value of map */

int MadeChanges;   /* made changes? */

static bool recalc_map_bounds;
static int  new_vertex_minimum;
static int  moved_vertex_count;


void MarkChanges(int scope)
{
	MadeChanges |= scope;

	main_win->canvas->redraw();

	UpdateHighlight();
}


void UpdateLevelBounds(int start_vert)
{
	for (int i = start_vert; i < NumVertices; i++)
	{
		const Vertex * V = Vertices[i];

		if (V->x < MapBound_lx) MapBound_lx = V->x;
		if (V->y < MapBound_ly) MapBound_ly = V->y;

		if (V->x > MapBound_hx) MapBound_hx = V->x;
		if (V->y > MapBound_hy) MapBound_hy = V->y;
	}
}

void CalculateLevelBounds()
{
	if (NumVertices == 0)
	{
		MapBound_lx = MapBound_hx = 0;
		MapBound_ly = MapBound_hy = 0;
		return;
	}

	MapBound_lx = 999999; MapBound_hx = -999999;
	MapBound_ly = 999999; MapBound_hy = -999999;

	UpdateLevelBounds(0);
}


void MapStuff_NotifyBegin()
{
	recalc_map_bounds  = false;
	new_vertex_minimum = -1;
	moved_vertex_count =  0;
}

void MapStuff_NotifyInsert(obj_type_e type, int objnum)
{
	if (type == OBJ_VERTICES)
	{
		if (new_vertex_minimum < 0 || objnum < new_vertex_minimum)
			new_vertex_minimum = objnum;
	}
}

void MapStuff_NotifyDelete(obj_type_e type, int objnum)
{
	if (type == OBJ_VERTICES)
	{
		recalc_map_bounds = true;
	}
}

void MapStuff_NotifyChange(obj_type_e type, int objnum, int field)
{
	if (type == OBJ_VERTICES)
	{
		// NOTE: for performance reasons we don't recalculate the
		//       map bounds when only moving a few vertices.
		moved_vertex_count++;

		const Vertex * V = Vertices[objnum];

		if (V->x < MapBound_lx) MapBound_lx = V->x;
		if (V->y < MapBound_ly) MapBound_ly = V->y;

		if (V->x > MapBound_hx) MapBound_hx = V->x;
		if (V->y > MapBound_hy) MapBound_hy = V->y;
	}
}

void MapStuff_NotifyEnd()
{
	if (recalc_map_bounds || moved_vertex_count > 10)  // TODO: CONFIG
	{
		CalculateLevelBounds();
	}
	else if (new_vertex_minimum >= 0)
	{
		UpdateLevelBounds(new_vertex_minimum);
	}
}


bool is_sky(const char *flat)
{
	return (y_stricmp(g_sky_flat.c_str(), flat) == 0);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
