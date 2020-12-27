//------------------------------------------------------------------------
//  EVENT HANDLING
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

#ifndef __EUREKA_M_EVENTS_H__
#define __EUREKA_M_EVENTS_H__

#include "m_keys.h"

typedef enum
{
	ACT_NOTHING = 0,

	ACT_CLICK,			// user has clicked on something
	ACT_SELBOX,			// user is outlining a selection box
	ACT_DRAG,			// user is dragging some objects
	ACT_TRANSFORM,		// user is scaling/rotating some objects
	ACT_ADJUST_OFS,		// user is adjusting the offsets on a sidedef
	ACT_DRAW_LINE,		// user is drawing a new line

} editor_action_e;


void Editor_ClearAction(Instance &inst);
void Editor_SetAction(Instance &inst, editor_action_e new_action);

void Editor_Zoom(int delta, int mid_x, int mid_y);

void Editor_UpdateFromScroll();
void Editor_ScrollMap(Instance &inst, int mode, int dx = 0, int dy = 0, keycode_t mod = 0);

/* raw input handling */

int EV_HandleEvent(Instance &inst, int event);
void EV_EscapeKey(Instance &inst);

void ClearStickyMod(Instance &inst);

keycode_t M_RawKeyForEvent(int event);
keycode_t M_CookedKeyForEvent(int event);

keycode_t M_ReadLaxModifiers();

extern int wheel_dx;
extern int wheel_dy;

typedef void (* nav_release_func_t)(Instance &inst);

void Nav_Navigate(Instance &inst);
bool Nav_SetKey(Instance &inst, keycode_t key, nav_release_func_t func);
bool Nav_ActionKey(Instance &inst, keycode_t key, nav_release_func_t func);

unsigned int Nav_TimeDiff(); /* milliseconds */

void M_LoadOperationMenus(Instance &inst);

void CMD_OperationMenu(Instance &inst);


#endif /* __EUREKA_M_EVENTS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
