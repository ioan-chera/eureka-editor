//------------------------------------------------------------------------
//  EDIT LOOP
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

#ifndef __EUREKA_EDITLOOP_H__
#define __EUREKA_EDITLOOP_H__

/* this holds some miscellaneous editor state */

typedef struct
{
	obj_type_e obj_type;   // current mode (OBJ_LINEDEFS, OBJ_SECTORS, etc...)

	int move_speed;   // Movement speed.
	int extra_zoom;   // Act like the zoom was 4 times what it is

	bool show_object_numbers; // Whether the object numbers are shown
	bool show_things_squares; // Whether the things squares are shown
	bool show_things_sprites; // Whether the things sprites are shown

	int map_x;    // Map coordinates of pointer
	int map_y;
	int pointer_in_window;  // If false, pointer_[xy] are not meaningful.

	int button_down;  // mouse button 1 to 3, or 0 for none,
	keycode_t button_mod;  // modifier(s) used when button was pressed

	Objid clicked;		// The object that was under the pointer when
						// the left click occurred. If clicked on
						// empty space, == CANVAS.
	bool did_a_move;   // just moved stuff, clear the next selection

	selection_c *Selected;    // all selected objects (usually empty)

	bool error_mode;  // draw selection in red

	Objid highlighted;   // The highlighted object

	Objid split_line;  // linedef which would be split by a new vertex

	int RedrawMap;   // set to 1 to force the map to be redrawn

	int drag_single_vertex;  // -1, or vertex number when dragging one vertex

} Editor_State_t;


extern Editor_State_t  edit;


void Editor_Init();

void Editor_DigitKey(keycode_t key);

void Editor_Wheel(int dx, int dy, keycode_t mod);
void Editor_MousePress(keycode_t mod);
void Editor_MouseRelease();
void Editor_MouseMotion(int x, int y, keycode_t mod, int map_x, int map_y, bool drag);
void Editor_MiddlePress(keycode_t mod);
void Editor_MiddleRelease();
void Editor_LeaveWindow();

bool Editor_ParseUser(const char ** tokens, int num_tok);
void Editor_WriteUser(FILE *fp);

bool GetCurrentObjects(selection_c *list);
void UpdateHighlight();

void Editor_ChangeMode(char mode);
void Editor_Zoom(int delta, int mid_x, int mid_y);

/* commands */

void CMD_SelectAll(void);
void CMD_UnselectAll(void);
void CMD_InvertSelection(void);

void CMD_Quit(void);

void CMD_ZoomWholeMap(void);
void CMD_ZoomSelection(void);
void CMD_GoToCamera(void);

void CMD_ToggleVar(void);

#endif /* __EUREKA_EDITLOOP_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
