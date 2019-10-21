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

	bool is_scrolling;   // user is scrolling the map (or moving in 3D view)
	bool is_navigating;  // user is holding down a navigation key

	bool pointer_in_window;  // whether the mouse is over the 2D/3D view
	double map_x, map_y;     // map coordinates of pointer


	selection_c *Selected;    // all selected objects (usually empty)

	Objid highlight;   // the highlighted object

	Objid split_line;  // linedef which would be split by a new vertex
	double split_x;
	double split_y;

	// the object that was under the pointer when ACT_Click occurred
	Objid clicked;

	int drawing_from;     // for ACT_DRAW_LINE: vertex we are drawing a line from
	int drag_single_obj;  // for ACT_DRAG: an object number we are dragging, or -1 for selection


	/* rendering stuff */

	bool error_mode;   // draw selection in red?

	int  sector_render_mode;   // one of the SREND_XXX values
	int   thing_render_mode;

	bool show_object_numbers;

	/* private navigation stuff */

	float nav_scroll_left;
	float nav_scroll_right;
	float nav_scroll_up;
	float nav_scroll_down;

	float scroll_speed;
};


extern Editor_State_t  edit;


void Editor_Init();
void Editor_DefaultState();
bool Editor_ParseUser(const char ** tokens, int num_tok);
void Editor_WriteUser(FILE *fp);

void Editor_ClearErrorMode();
void Editor_ChangeMode(char mode);
void Editor_ChangeMode_Raw(obj_type_e new_mode);

bool GetCurrentObjects(selection_c *list);
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

void Selection_Clear(bool no_save = false);
void Selection_Add(Objid& obj);
void Selection_Toggle(Objid& obj);
void Selection_Push();
void Selection_InvalidateLast();

void SelectObjectsInBox(selection_c *list, int objtype, double x1, double y1, double x2, double y2);


/* commands */

void CMD_LastSelection();


//----------------------------------------------------------------------
//  3D Clipboard
//----------------------------------------------------------------------

class render_clipboard_c
{
private:
	char tex [16];
	char flat[16];

	int  thing;

public:
	 render_clipboard_c();
	~render_clipboard_c();

	// when current one is unset, these return default_wall_tex (etc)
	int GetTexNum();
	int GetFlatNum();
	int GetThing();

	void SetTex(const char *new_tex);
	void SetFlat(const char *new_flat);
	void SetThing(int new_id);

	bool ParseUser(const char ** tokens, int num_tok);
	void WriteUser(FILE *fp);
};

extern render_clipboard_c  r_clipboard;


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
