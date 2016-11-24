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
	SREND_Lighting

} sector_rendering_mode_e;


/* this holds some important editor state */

typedef struct
{
	obj_type_e  mode;   // current mode (OBJ_LINEDEFS, OBJ_SECTORS, etc...)

	editor_action_e  action;  // an in-progress action, usually ACT_NOTHING

	bool is_scrolling;	// user is scrolling the map (or moving in 3D view)
	bool is_navigating;	// user is holding down a navigation key

	keycode_t sticky_mod;	// user pressed ';' -- waiting for next key

	bool render3d;    // 3D view is active
	bool error_mode;  // draw selection in red

	int  sector_render_mode;	// one of the SREND_XXX values

	bool show_object_numbers; // Whether the object numbers are shown
	bool show_things_squares; // Whether the things squares are shown
	bool show_things_sprites; // Whether the things sprites are shown

	int map_x;    // Map coordinates of pointer
	int map_y;
	int pointer_in_window;  // If false, pointer_[xy] are not meaningful.

	int button_down;  // mouse button 1 to 3, or 0 for none,
	keycode_t button_mod;  // modifier(s) used when button was pressed

	Objid clicked;		// The object that was under the pointer when
						// the left click occurred.

	selection_c *Selected;    // all selected objects (usually empty)

	Objid highlight;   // The highlighted object

	Objid split_line;  // linedef which would be split by a new vertex
	int split_x;
	int split_y;

	int drawing_from;	 // for ACT_DRAW_LINE, the vertex we are drawing a line from

	int drag_single_obj;  // -1, or object number we are dragging

	// private navigation stuff
	float nav_scroll_dx;
	float nav_scroll_dy;

} Editor_State_t;


extern Editor_State_t  edit;


void Editor_Init();
bool Editor_ParseUser(const char ** tokens, int num_tok);
void Editor_WriteUser(FILE *fp);

void Editor_ClearErrorMode();
void Editor_ChangeMode(char mode);
void Editor_ChangeMode_Raw(obj_type_e new_mode);

bool GetCurrentObjects(selection_c *list);
void UpdateHighlight();

void RedrawMap();
void ZoomWholeMap();


extern int Map_bound_x1;   /* minimum X value of map */
extern int Map_bound_y1;   /* minimum Y value of map */
extern int Map_bound_x2;   /* maximum X value of map */
extern int Map_bound_y2;   /* maximum Y value of map */

void MarkChanges();

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
void Selection_Push();
void Selection_InvalidateLast();


void CMD_LastSelection(void);


// handling of recently used textures, flats and things

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