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
// this holds some important editor state
//
struct Editor_State_t
{
	void Selection_AddHighlighted();
	void Selection_Toggle(Objid &obj);

	ObjType mode;  // current mode (OBJ_LINEDEFS, OBJ_SECTORS, etc...)

	bool render3d;     // 3D view is active

	editor_action_e  action;  // an in-progress action, usually ACT_NOTHING

	keycode_t sticky_mod;  // if != 0, waiting for next key  (fake meta)

	bool pointer_in_window;  // whether the mouse is over the 2D/3D view
	double map_x, map_y, map_z;  // map coordinates of pointer (no Z in 2D)

	selection_c *Selected;    // all selected objects (usually empty)

	Objid highlight;   // the highlighted object

	Objid split_line;  // linedef which would be split by a new vertex
	double split_x;
	double split_y;


	/* rendering stuff */

	bool error_mode;   // draw selection in red?

	int  sector_render_mode;   // one of the SREND_XXX values
	int   thing_render_mode;

	bool show_object_numbers;


	/* navigation stuff */

	bool is_navigating;  // user is holding down a navigation key
	bool is_panning;     // user is panning the map (turning in 3D) via RMB

	float nav_fwd,    nav_back;
	float nav_left,   nav_right;
	float nav_up,     nav_down;
	float nav_turn_L, nav_turn_R;
	bool  nav_lax;

	float panning_speed;
	bool  panning_lax;


	/* click stuff (ACT_CLICK) */

	Objid clicked;    // object under the pointer when ACT_Click occurred

	int click_screen_x, click_screen_y;  // screen coord of the click

	double click_map_x, click_map_y, click_map_z;  // location of the click

	bool click_check_drag;
	bool click_check_select;
	bool click_force_single;


	/* line drawing stuff (ACT_DRAW_LINE) */

	Objid draw_from;  // the vertex we are drawing a line from

	double draw_to_x, draw_to_y;  // target coordinate of current line


	/* selection-box stuff (ACT_SELBOX) */

	double selbox_x1, selbox_y1;  // map coords
	double selbox_x2, selbox_y2;


	/* transforming state (ACT_TRANSFORM) */

	double trans_start_x;
	double trans_start_y;

	transform_keyword_e trans_mode;
	transform_t trans_param;

	selection_c *trans_lines;


	/* dragging state (ACT_DRAG) */

	Objid dragged;    // the object we are dragging, or nil for whole selection

	int drag_screen_dx, drag_screen_dy;

	double drag_start_x, drag_start_y, drag_start_z;
	double drag_focus_x, drag_focus_y, drag_focus_z;
	double drag_cur_x,   drag_cur_y,   drag_cur_z;

	float drag_point_dist;
	float drag_sector_dz;
	int   drag_other_vert;  // used to ratio-lock a dragged vertex

	int   drag_thing_num;
	float drag_thing_floorh;
	bool  drag_thing_up_down;

	selection_c *drag_lines;


	/* adjusting state (ACT_ADJUST_OFS) */

	float adjust_dx, adjust_dy;
	bool  adjust_lax;

	SaveBucket_c * adjust_bucket;

	struct { float x1, y1, x2, y2; } adjust_bbox;
};

void Selection_NotifyChange(ObjType type, int objnum, int field);


void DumpSelection (selection_c * list);

void ConvertSelection(const Document &doc, const selection_c * src, selection_c * dest);

//
// When using editor
//
enum class SelectHighlight
{
	ok,			// using selection, nothing else needed
	unselect,	// using highlight, must unselect at end
	empty		// both selection or highlight are empty
};

void SelectObjectsInBox(const Document &doc, selection_c *list, ObjType objtype, double x1, double y1, double x2, double y2);

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
