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


/*
 *  levelname2levelno
 *  Used to know if directory entry is ExMy or MAPxy
 *  For "ExMy" (case-insensitive),  returns 10x + y
 *  For "ExMyz" (case-insensitive), returns 100*x + 10y + z
 *  For "MAPxy" (case-insensitive), returns 1000 + 10x + y
 *  E0My, ExM0, E0Myz, ExM0z are not considered valid names.
 *  MAP00 is considered a valid name.
 *  For other names, returns 0.
 */
int levelname2levelno (const char *name)
{
	const unsigned char *s = (const unsigned char *) name;
	if (toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& s[4] == '\0')
		return 10 * dectoi (s[1]) + dectoi (s[3]);
	if (yg_level_name == YGLN_E1M10
			&& toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 100 * dectoi (s[1]) + 10 * dectoi (s[3]) + dectoi (s[4]);
	if (toupper (s[0]) == 'M'
			&& toupper (s[1]) == 'A'
			&& toupper (s[2]) == 'P'
			&& isdigit (s[3])
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 1000 + 10 * dectoi (s[3]) + dectoi (s[4]);
	return 0;
}


/*
 *  levelname2rank
 *  Used to sort level names.
 *  Identical to levelname2levelno except that, for "ExMy",
 *  it returns 100x + y, so that
 *  - f("E1M10") = f("E1M9") + 1
 *  - f("E2M1")  > f("E1M99")
 *  - f("E2M1")  > f("E1M99") + 1
 *  - f("MAPxy") > f("ExMy")
 *  - f("MAPxy") > f("ExMyz")
 */
int levelname2rank (const char *name)
{
	const unsigned char *s = (const unsigned char *) name;
	if (toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& s[4] == '\0')
		return 100 * dectoi (s[1]) + dectoi (s[3]);
	if (yg_level_name == YGLN_E1M10
			&& toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 100 * dectoi (s[1]) + 10 * dectoi (s[3]) + dectoi (s[4]);
	if (toupper (s[0]) == 'M'
			&& toupper (s[1]) == 'A'
			&& toupper (s[2]) == 'P'
			&& isdigit (s[3])
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 1000 + 10 * dectoi (s[3]) + dectoi (s[4]);
	return 0;
}


bool is_sky(const char *flat)
{
	return (y_stricmp(g_sky_flat.c_str(), flat) == 0);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
