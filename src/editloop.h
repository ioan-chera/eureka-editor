//------------------------------------------------------------------------
//  EDIT LOOP
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

#ifndef __EUREKA_EDITLOOP_H__
#define __EUREKA_EDITLOOP_H__


typedef enum
{
	ACT_NOTHING = 0,

	ACT_WAIT_META,		// user pressed ';' -- waiting for next key

	ACT_SELBOX,			// user is outlining a selection box
	ACT_DRAG,			// user is dragging some objects
	ACT_SCALE,			// user is scaling (etc) some objects

	ACT_ADJUST_OFS,		// user is adjusting the offsets on a sidedef
	ACT_DRAW_LINE,		// user is drawing a new line

} editor_action_e;


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

	bool render3d;    // 3D preview is active
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

	bool did_a_move;   // just moved stuff, clear the next selection

	selection_c *Selected;    // all selected objects (usually empty)

	Objid highlight;   // The highlighted object

	Objid split_line;  // linedef which would be split by a new vertex
	int split_x;
	int split_y;

	int drawing_from;	 // for ACT_DRAW_LINE, the vertex we are drawing a line from

	int drag_single_vertex;  // -1, or vertex number when dragging one vertex

	// private navigation stuff
	float nav_scroll_dx;
	float nav_scroll_dy;

} Editor_State_t;


extern Editor_State_t  edit;


void Editor_Init();

void Editor_DigitKey(keycode_t key);

void Editor_Wheel(int dx, int dy, keycode_t mod);
void Editor_MousePress(keycode_t mod);
void Editor_MouseRelease();
void Editor_MiddlePress(keycode_t mod);
void Editor_MiddleRelease();
void Editor_LeaveWindow();

void Editor_ClearAction();
void Editor_SetAction(editor_action_e new_action);

bool Editor_ParseUser(const char ** tokens, int num_tok);
void Editor_WriteUser(FILE *fp);

void Editor_ClearErrorMode();
void Editor_ChangeMode(char mode);
void Editor_ChangeMode_Raw(obj_type_e new_mode);
void Editor_Zoom(int delta, int mid_x, int mid_y);

bool GetCurrentObjects(selection_c *list);
void UpdateHighlight();

void RedrawMap();

/* raw input handling */

int  Editor_RawKey(int event);
int  Editor_RawButton(int event);
int  Editor_RawWheel(int event);
int  Editor_RawMouse(int event);

typedef void (* nav_release_func_t)(void);

void Nav_Clear();
void Nav_Navigate();
bool Nav_SetKey(keycode_t key, nav_release_func_t func);
bool Nav_ActionKey(keycode_t key, nav_release_func_t func);
void Nav_UpdateKeys();

unsigned int Nav_TimeDiff(); /* milliseconds */

/* commands */

void CMD_SelectAll(void);
void CMD_UnselectAll(void);
void CMD_InvertSelection(void);
void CMD_LastSelection(void);

void CMD_Quit(void);

void CMD_Undo(void);
void CMD_Redo(void);

void CMD_ZoomWholeMap(void);
void CMD_ZoomSelection(void);
void CMD_GoToCamera(void);

void CMD_ToggleVar(void);

void CMD_MoveObjects_Dialog();
void CMD_ScaleObjects_Dialog();
void CMD_RotateObjects_Dialog();

#endif /* __EUREKA_EDITLOOP_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
