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
#include "m_game.h"
#include "editloop.h"
#include "levels.h"
#include "e_things.h"
#include "w_rawdef.h"

#include "ui_window.h"


int Map_bound_x1 =  32767;   /* minimum X value of map */
int Map_bound_y1 =  32767;   /* minimum Y value of map */
int Map_bound_x2 = -32767;   /* maximum X value of map */
int Map_bound_y2 = -32767;   /* maximum Y value of map */

int MadeChanges;

static bool recalc_map_bounds;
static int  new_vertex_minimum;
static int  moved_vertex_count;


void MarkChanges()
{
	MadeChanges = 1;

	UpdateHighlight();

	edit.RedrawMap = 1;
}


void UpdateLevelBounds(int start_vert)
{
	for (int i = start_vert ; i < NumVertices ; i++)
	{
		const Vertex * V = Vertices[i];

		if (V->x < Map_bound_x1) Map_bound_x1 = V->x;
		if (V->y < Map_bound_y1) Map_bound_y1 = V->y;

		if (V->x > Map_bound_x2) Map_bound_x2 = V->x;
		if (V->y > Map_bound_y2) Map_bound_y2 = V->y;
	}
}

void CalculateLevelBounds()
{
	if (NumVertices == 0)
	{
		Map_bound_x1 = Map_bound_x2 = 0;
		Map_bound_y1 = Map_bound_y2 = 0;
		return;
	}

	Map_bound_x1 = 999999; Map_bound_x2 = -999999;
	Map_bound_y1 = 999999; Map_bound_y2 = -999999;

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

		if (V->x < Map_bound_x1) Map_bound_x1 = V->x;
		if (V->y < Map_bound_y1) Map_bound_y1 = V->y;

		if (V->x > Map_bound_x2) Map_bound_x2 = V->x;
		if (V->y > Map_bound_y2) Map_bound_y2 = V->y;
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


//------------------------------------------------------------------------
//  RECENTLY USED TEXTURES (etc)
//------------------------------------------------------------------------


// the containers for the textures (etc)
Recently_used  recent_textures;
Recently_used  recent_flats;
Recently_used  recent_things;


Recently_used::Recently_used(int _keepnum) :
	size(0),
	keep_num(_keepnum)
{
	memset(&name_set, 0, sizeof(name_set));
}


Recently_used::~Recently_used()
{
	for (int i = 0 ; i < size ; i++)
	{
		StringFree(name_set[i]);
		name_set[i] = NULL;
	}
}


int Recently_used::find(const char *name)
{
	for (int k = 0 ; k < size ; k++)
		if (y_stricmp(name_set[k], name) == 0)
			return k;

	return -1;	// not found
}

int Recently_used::find_number(int val)
{
	char buffer[64];

	sprintf(buffer, "%d", val);

	return find(buffer);
}


void Recently_used::insert(const char *name)
{
	// ignore '-' texture
	if (name[0] == '-')
		return;

	int idx = find(name);

	// small optimisation
	if (idx >= 0 && idx < keep_num/3)
		return;

	if (idx >= 0)
		erase(idx);

	push_front(name);
}

void Recently_used::insert_number(int val)
{
	char buffer[64];

	sprintf(buffer, "%d", val);

	insert(buffer);
}


void Recently_used::erase(int index)
{
	SYS_ASSERT(0 <= index && index < size);

	StringFree(name_set[index]);

	size--;

	for ( ; index < size ; index++)
	{
		name_set[index] = name_set[index + 1];
	}

	name_set[index] = NULL;
}


void Recently_used::push_front(const char *name)
{
	if (size >= keep_num)
	{
		erase(keep_num - 1);
	}

	// shift elements up
	for (int k = size - 1 ; k >= 0 ; k--)
	{
		name_set[k + 1] = name_set[k];
	}

	name_set[0] = StringDup(name);

	size++;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
