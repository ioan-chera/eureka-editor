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

#define MAX_NAV_ACTIVE_KEYS  20

enum editor_action_e
{
	ACT_NOTHING = 0,

	ACT_CLICK,			// user has clicked on something
	ACT_SELBOX,			// user is outlining a selection box
	ACT_DRAG,			// user is dragging some objects
	ACT_TRANSFORM,		// user is scaling/rotating some objects
	ACT_ADJUST_OFS,		// user is adjusting the offsets on a sidedef
	ACT_DRAW_LINE,		// user is drawing a new line
};

typedef void (Instance:: *nav_release_func_t)();

struct nav_active_key_t
{
	// key or button code, including any modifier.
	// zero when this slot is unused.
	keycode_t  key;

	// function to call when user releases the key or button.
	nav_release_func_t  release;

	// modifiers that can change state without a keypress
	// being considered as a new command.
	keycode_t  lax_mod;

};

void Editor_SetAction(Instance &inst, editor_action_e new_action);

void Editor_UpdateFromScroll();

/* raw input handling */

keycode_t M_RawKeyForEvent(int event);
keycode_t M_CookedKeyForEvent(int event);

keycode_t M_ReadLaxModifiers();

#endif /* __EUREKA_M_EVENTS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
