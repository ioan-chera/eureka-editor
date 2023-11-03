//------------------------------------------------------------------------
//  LEVEL MISC STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
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

#ifndef __EUREKA_LEVELS_H__
#define __EUREKA_LEVELS_H__

#include <string>

#include "m_events.h"
#include "e_objects.h"
#include "e_things.h"
#include "m_vector.h"

class SaveBucket_c;
class selection_c;

typedef enum
{
	SREND_Nothing = 0,
	SREND_Floor,
	SREND_Ceiling,
	SREND_Lighting,
	SREND_FloorBright,
	SREND_CeilBright,
	SREND_SoundProp

} sector_rendering_mode_e;

//
// When using editor
//
enum class SelectHighlight
{
	ok,			// using selection, nothing else needed
	unselect,	// using highlight, must unselect at end
	empty		// both selection or highlight are empty
};

//
// line drawing stuff (EditorAction::drawLine)
//
struct DrawLineState
{
	Objid from;	// the vertex we are drawing a line from
	v2double_t to;	// target coordinate of current line
};

//
// Navigation state
//
struct Navigation
{
	float fwd,    back;
	float left,   right;
	float up,     down;
	float turn_L, turn_R;
	bool  lax;
};

//
// this holds some important editor state
//
struct Editor_State_t
{
	SelectHighlight SelectionOrHighlight();
	void Selection_AddHighlighted();
	void Selection_Toggle(Objid &obj);
	void defaultState();
	void clearNav()
	{
		nav = {};
	}

	ObjType mode;  // current mode (OBJ_LINEDEFS, OBJ_SECTORS, etc...)

	bool render3d;     // 3D view is active

	EditorAction  action;  // an in-progress action, usually EditorAction::nothing

	keycode_t sticky_mod;  // if != 0, waiting for next key  (fake meta)

	bool pointer_in_window;  // whether the mouse is over the 2D/3D view
	v3double_t map;  // map coordinates of pointer (no Z in 2D)

	selection_c *Selected;    // all selected objects (usually empty)

	Objid highlight;   // the highlighted object

	Objid split_line;  // linedef which would be split by a new vertex
	v2double_t split;


	/* rendering stuff */

	bool error_mode;   // draw selection in red?

	int  sector_render_mode;   // one of the SREND_XXX values
	int   thing_render_mode;

	bool show_object_numbers;


	/* navigation stuff */

	bool is_navigating;  // user is holding down a navigation key
	bool is_panning;     // user is panning the map (turning in 3D) via RMB

	Navigation nav;

	float panning_speed;
	bool  panning_lax;


	/* click stuff (EditorAction::click) */

	Objid clicked;    // object under the pointer when ACT_Click occurred

	v2int_t click_screen_pos;	// screen coord of the click

	v3double_t click_map;	// location of the click

	bool click_check_drag;
	bool click_check_select;
	bool click_force_single;


	/* line drawing stuff (EditorAction::drawLine) */
	DrawLineState drawLine;

	/* selection-box stuff (EditorAction::selbox) */

	v2double_t selbox1;  // map coords
	v2double_t selbox2;


	/* transforming state (EditorAction::transform) */

	v2double_t trans_start;

	transform_keyword_e trans_mode;
	transform_t trans_param;

	selection_c *trans_lines;


	/* dragging state (EditorAction::drag) */

	Objid dragged;    // the object we are dragging, or nil for whole selection

	v2int_t drag_screen_dpos;

	v3double_t drag_start;
	v3double_t drag_focus;
	v3double_t drag_cur;

	float drag_point_dist;
	float drag_sector_dz;
	int   drag_other_vert;  // used to ratio-lock a dragged vertex

	int   drag_thing_num;
	float drag_thing_floorh;
	bool  drag_thing_up_down;

	selection_c *drag_lines;


	/* adjusting state (EditorAction::adjustOfs) */

	float adjust_dx, adjust_dy;
	bool  adjust_lax;

	SaveBucket_c * adjust_bucket;

	struct { float x1, y1, x2, y2; } adjust_bbox;
};

void Selection_NotifyChange(ObjType type, int objnum, int field);


void DumpSelection (selection_c * list);

void ConvertSelection(const Document &doc, const selection_c & src, selection_c & dest);

void SelectObjectsInBox(const Document &doc, selection_c *list, ObjType objtype, v2double_t pos1, v2double_t pos2);

//----------------------------------------------------------------------
//  Helper for handling either the highlight or selection
//----------------------------------------------------------------------



//----------------------------------------------------------------------
//  Recently used textures, flats and things
//----------------------------------------------------------------------

#define RECENTLY_USED_MAX	32

class Recently_used
{
private:
	int size = 0;
	int keep_num = RECENTLY_USED_MAX - 2;

	SString name_set[RECENTLY_USED_MAX];

	Instance &inst;

public:
	explicit Recently_used(Instance &inst) : inst(inst)
	{
	}

	int find(const SString &name);
	int find_number(int val);

	void insert(const SString &name);
	void insert_number(int val);

	void clear();

	void WriteUser(std::ostream &os, char letter) const;

private:
	void erase(int index);
	void push_front(const SString &name);
};

#endif  /* __EUREKA_LEVELS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
