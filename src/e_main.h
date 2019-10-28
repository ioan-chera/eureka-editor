//------------------------------------------------------------------------
//  LEVEL MISC STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
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
#include "e_things.h"


typedef enum
{
	SREND_Nothing = 0,
	SREND_Floor,
	SREND_Ceiling,
	SREND_Lighting,
	SREND_SoundProp

} sector_rendering_mode_e;


//
// this holds some important editor state
//
struct Editor_State_t
{
	obj_type_e  mode;  // current mode (OBJ_LINEDEFS, OBJ_SECTORS, etc...)

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

	float panning_speed;


	/* click stuff (ACT_CLICK) */

	Objid clicked;    // object under the pointer when ACT_Click occurred

	int click_screen_x, click_screen_y;  // screen coord of the click

	double click_map_x, click_map_y, click_map_z;  // location of the click

	bool click_check_drag;
	bool click_check_select;
	bool click_force_single;


	/* line drawing stuff (ACT_DRAW_LINE) */

	Objid from_vert;  // the vertex we are drawing a line from


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
	int   drag_thing_num;
	float drag_thing_floorh;

	selection_c *drag_lines;


	/* adjusting state (ACT_ADJUST_OFS) */

	float adjust_dx, adjust_dy;
	float adjust_dz;  // FIXME remove this one
};


extern Editor_State_t  edit;


void Editor_Init();
void Editor_DefaultState();
bool Editor_ParseUser(const char ** tokens, int num_tok);
void Editor_WriteUser(FILE *fp);

void Editor_ClearErrorMode();
void Editor_ChangeMode(char mode);
void Editor_ChangeMode_Raw(obj_type_e new_mode);

void UpdateHighlight();

void RedrawMap();
void ZoomWholeMap();


extern double Map_bound_x1;   /* minimum X value of map */
extern double Map_bound_y1;   /* minimum Y value of map */
extern double Map_bound_x2;   /* maximum X value of map */
extern double Map_bound_y2;   /* maximum Y value of map */

void CalculateLevelBounds();


void MapStuff_NotifyBegin();
void MapStuff_NotifyInsert(obj_type_e type, int objnum);
void MapStuff_NotifyDelete(obj_type_e type, int objnum);
void MapStuff_NotifyChange(obj_type_e type, int objnum, int field);
void MapStuff_NotifyEnd();


void ObjectBox_NotifyBegin();
void ObjectBox_NotifyInsert(obj_type_e type, int objnum);
void ObjectBox_NotifyDelete(obj_type_e type, int objnum);
void ObjectBox_NotifyChange(obj_type_e type, int objnum, int field);
void ObjectBox_NotifyEnd();


void Selection_NotifyBegin();
void Selection_NotifyInsert(obj_type_e type, int objnum);
void Selection_NotifyDelete(obj_type_e type, int objnum);
void Selection_NotifyChange(obj_type_e type, int objnum, int field);
void Selection_NotifyEnd();


void DumpSelection (selection_c * list);

void ConvertSelection(selection_c * src, selection_c * dest);

int Selection_FirstLine(selection_c *list);

void Selection_Add(Objid& obj);
void Selection_Remove(Objid& obj);
void Selection_Toggle(Objid& obj);

void Selection_Clear(bool no_save = false);
void Selection_Push();
void Selection_InvalidateLast();

typedef enum
{
	SOH_OK = 0,       // using selection, nothing else needed
	SOH_Unselect = 1, // using highlight, must unselect at end
	SOH_Empty = 2     // both selection or highlight are empty
} soh_type_e;

soh_type_e Selection_Or_Highlight();

void SelectObjectsInBox(selection_c *list, int objtype, double x1, double y1, double x2, double y2);


/* commands */

void CMD_LastSelection();


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
	int size;
	int keep_num;

	const char *name_set[RECENTLY_USED_MAX];

public:
	 Recently_used();
	~Recently_used();

	int find(const char *name);
	int find_number(int val);

	void insert(const char *name);
	void insert_number(int val);

	void clear();

	void WriteUser(FILE *fp, char letter);

private:
	void erase(int index);
	void push_front(const char *name);
};

extern Recently_used  recent_textures;
extern Recently_used  recent_flats;
extern Recently_used  recent_things;

void RecUsed_ClearAll();
void RecUsed_WriteUser(FILE *fp);
bool RecUsed_ParseUser(const char ** tokens, int num_tok);


#endif  /* __EUREKA_LEVELS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
