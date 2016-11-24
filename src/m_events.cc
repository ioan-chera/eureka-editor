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

#include "main.h"

#include "m_events.h"
#include "e_misc.h"
#include "e_hover.h"
#include "r_render.h"

#include "ui_window.h"
#include "ui_misc.h"


int active_when = 0;  // FIXME MOVE THESE into Editor_State
int active_wmask = 0;


// config items
int multi_select_modifier = 0;
int minimum_drag_pixels = 5;


extern bool easier_drawing_mode;

void Editor_MouseMotion(int x, int y, keycode_t mod);


void ClearStickyMod()
{
	if (edit.sticky_mod)
		Status_Clear();

	edit.sticky_mod = 0;
}


static int mouse_last_x;
static int mouse_last_y;

// screen position when LMB was pressed
static int mouse_button1_x;
static int mouse_button1_y;

// map location when LMB was pressed
static int button1_map_x;
static int button1_map_y;


void Editor_ClearAction()
{
	switch (edit.action)
	{
		case ACT_NOTHING:
			return;

		case ACT_ADJUST_OFS:
			main_win->SetCursor(FL_CURSOR_DEFAULT);
			break;

		default:
			/* no special for the rest */
			break;
	}

	edit.action = ACT_NOTHING;
}


void Editor_SetAction(editor_action_e  new_action)
{
	Editor_ClearAction();

	edit.action = new_action;

	switch (edit.action)
	{
		case ACT_NOTHING:
			return;

		case ACT_ADJUST_OFS:
			mouse_last_x = Fl::event_x();
			mouse_last_y = Fl::event_y();

			main_win->SetCursor(FL_CURSOR_HAND);
			break;

		default:
			/* no special for the rest */
			break;
	}
}


void Editor_UpdateFromScroll()
{
	if (edit.action == ACT_SELBOX || edit.action == ACT_DRAW_LINE ||
		edit.action == ACT_DRAG)
	{
		int mod = Fl::event_state() & MOD_ALL_MASK;

		Editor_MouseMotion(Fl::event_x(), Fl::event_y(), mod);
	}
}


void Editor_Zoom(int delta, int mid_x, int mid_y)
{
    float S1 = grid.Scale;

    grid.AdjustScale(delta);

    grid.RefocusZoom(mid_x, mid_y, S1);
}


void Editor_ScrollMap(int mode, int dx, int dy)
{
	// started?
	if (mode < 0)
	{
		edit.is_scrolling = true;
		main_win->SetCursor(FL_CURSOR_HAND);
		return;
	}

	// finished?
	if (mode > 0)
	{
		edit.is_scrolling = false;
		main_win->SetCursor(FL_CURSOR_DEFAULT);
		return;
	}


	keycode_t mod = Fl::event_state() & MOD_ALL_MASK;

	if (edit.render3d)
	{
		Render3D_RBScroll(dx, dy, mod);
	}
	else
	{
		int speed = 8;  // FIXME: CONFIG OPTION

		if (mod == MOD_SHIFT)
			speed /= 2;
		else if (mod == MOD_COMMAND)
			speed *= 2;

		double delta_x = ((double) -dx * speed / 8.0 / grid.Scale);
		double delta_y = ((double)  dy * speed / 8.0 / grid.Scale);

		grid.Scroll(delta_x, delta_y);
	}
}


void Editor_ClearNav()
{
	edit.nav_scroll_dx = 0;
	edit.nav_scroll_dy = 0;
}


static void Editor_Navigate()
{
	float delay_ms = Nav_TimeDiff();

	delay_ms = delay_ms / 1000.0;

	if (edit.nav_scroll_dx || edit.nav_scroll_dy)
	{
		float delta_x = edit.nav_scroll_dx * delay_ms;
		float delta_y = edit.nav_scroll_dy * delay_ms;

		grid.Scroll(delta_x, delta_y);
	}

	RedrawMap();
}


/* navigation system */

#define MAX_NAV_ACTIVE_KEYS  20

typedef struct
{
	// key or button code, including any modifier.
	// zero when this slot is unused.
	keycode_t  key;

	// function to call when user releases the key or button.
	nav_release_func_t  release;

} nav_active_key_t;

static nav_active_key_t cur_action_key;

static nav_active_key_t nav_actives[MAX_NAV_ACTIVE_KEYS];

static unsigned int nav_time;


void Nav_Clear()
{
	  Editor_ClearNav();
	Render3D_ClearNav();

	memset(nav_actives, 0, sizeof(nav_actives));

	edit.is_navigating = false;
}


void Nav_Navigate()
{
	if (edit.render3d)
		Render3D_Navigate();
	else
		Editor_Navigate();
}


bool Nav_SetKey(keycode_t key, nav_release_func_t func)
{
	// when starting a navigation, grab the current time
	if (! edit.is_navigating)
		Nav_TimeDiff();

	edit.is_navigating = true;

	int free_slot = -1;

	for (int i = 0 ; i < MAX_NAV_ACTIVE_KEYS ; i++)
	{
		nav_active_key_t& N = nav_actives[i];

		if (! N.key)
		{
			if (free_slot < 0)
				free_slot = i;

			continue;
		}

		// already active?
		if (N.key == key && N.release == func)
			return false;

		// if it's the same physical key, release it now
		if ((N.key & FL_KEY_MASK) == (key & FL_KEY_MASK))
		{
			(N.release)();
			 N.key = 0;
		}
	}

	if (free_slot >= 0)
	{
		nav_actives[free_slot].key = key;
		nav_actives[free_slot].release = func;
	}

	return true;
}


bool Nav_ActionKey(keycode_t key, nav_release_func_t func)
{
	nav_active_key_t& N = cur_action_key;

	// already active?
	if (N.key == key && N.release == func)
		return false;

	// release existing action
	if (N.key != 0)
	{
		(N.release)();
		 N.key = 0;
	}

	N.key = key;
	N.release = func;

	return true;
}


static inline bool CheckKeyPressed(nav_active_key_t& N)
{
	keycode_t base = N.key & FL_KEY_MASK;
	keycode_t mod  = N.key & MOD_ALL_MASK;

	// grab current modifiers, but simplify to a single one
	keycode_t cur_mod = M_TranslateKey(0, Fl::event_state());

	if (is_mouse_button(base))
	{
		if (mod == cur_mod && (Fl::event_buttons() & FL_BUTTON(base - FL_Button)))
			return true;
	}
	else  // key on keyboard
	{
		if (mod == cur_mod && Fl::event_key(base))
			return true;
	}

	return false;
}


void Nav_UpdateActionKey()
{
	nav_active_key_t& N = cur_action_key;

	if (! N.key)
		return;

	if (! CheckKeyPressed(N))
	{
		(N.release)();
		 N.key = 0;
	}
}


void Nav_UpdateKeys()
{
	// ensure the currently active keys are still pressed
	// [ call this after getting each keyboard event from FLTK ]

	Nav_UpdateActionKey();

	if (! edit.is_navigating)
		return;

	// we rebuilt this value
	edit.is_navigating = false;

	for (int i = 0 ; i < MAX_NAV_ACTIVE_KEYS ; i++)
	{
		nav_active_key_t& N = nav_actives[i];

		if (! N.key)
			continue;

		if (! CheckKeyPressed(N))
		{
			// call release function, clear the slot
			(N.release)();

			N.key = 0;
			continue;
		}

		// at least one navigation key is still active
		edit.is_navigating = true;
	}
}


// returns number of milliseconds since the previous call
unsigned int Nav_TimeDiff()
{
	unsigned int old_time = nav_time;

	nav_time = TimeGetMillies();

	// handle overflow
	if (nav_time < old_time)
		return 10;

	unsigned int diff = (nav_time - old_time);

	// clamp large values
	if (diff > 250)
		diff = 250;

	return diff;
}


//------------------------------------------------------------------------
//   EVENT HANDLING
//------------------------------------------------------------------------

// these are grabbed from FL_MOUSEWHEEL events
int wheel_dx;
int wheel_dy;


void Editor_MousePress(keycode_t mod)
{
	if (edit.button_down >= 2)
		return;

	edit.button_down = 1;
	edit.button_mod  = mod;

	// remember some state (for dragging)
	mouse_button1_x = Fl::event_x();
	mouse_button1_y = Fl::event_y();

	button1_map_x = edit.map_x;
	button1_map_y = edit.map_y;

	// this is a special case, since we want to allow the new vertex
	// from a split-line (when in in drawing mode) to be draggable.
	// [ that is achieved by setting edit.clicked ]

	if (easier_drawing_mode && edit.split_line.valid() &&
		edit.action != ACT_DRAW_LINE)
	{
		Insert_Vertex_split(edit.split_line.num, edit.split_x, edit.split_y);
		return;
	}

	if (edit.action == ACT_DRAW_LINE)
	{
		bool force_cont = (mod == MOD_SHIFT);
		bool no_fill    = (mod == MOD_COMMAND);

		Insert_Vertex(force_cont, no_fill, true /* is_button */);
		return;
	}

	// find the object under the pointer.
	GetNearObject(edit.clicked, edit.mode, edit.map_x, edit.map_y);

	// inhibit in sector/linedef mode when SHIFT is pressed, to allow
	// opening a selection box in places which are otherwise impossible.
	if (mod == MOD_SHIFT && (edit.mode == OBJ_SECTORS || edit.mode == OBJ_LINEDEFS))
	{
		edit.clicked.clear();
	}

	// clicking on an empty space starts a new selection box.
	if (edit.clicked.is_nil())
	{
//!!!!		Editor_SetAction(ACT_SELBOX);
//!!!!		main_win->canvas->SelboxBegin(edit.map_x, edit.map_y);
		return;
	}

	// Note: drawing mode is activated on RELEASE...
	//       (as the user may be trying to drag the vertex)
}


void Editor_MouseRelease()
{
	edit.button_down = 0;

	Objid click_obj(edit.clicked);
	edit.clicked.clear();

	// releasing the button while dragging : drop the selection.
#if 1
	if (edit.action == ACT_DRAG)
	{
		Editor_ClearAction();

		int dx, dy;
		main_win->canvas->DragFinish(&dx, &dy);

		if (dx || dy)
		{
			if (edit.drag_single_obj >= 0)
				DragSingleObject(edit.drag_single_obj, dx, dy);
			else
				MoveObjects(edit.Selected, dx, dy);
		}

		edit.drag_single_obj = -1;
		RedrawMap();
		return;
	}
#endif

	// releasing the button while there was a selection box
	// causes all the objects within the box to be selected.



	// nothing needed while in drawing mode
	if (edit.action == ACT_DRAW_LINE)
		return;


	// optional multi-select : require a certain modifier key
	if (multi_select_modifier &&
		edit.button_mod != (multi_select_modifier == 1 ? MOD_SHIFT : MOD_COMMAND))
	{
//FIXME : REVIEW THIS
//		Selection_Clear();
	}


	// handle a clicked-on object
	// e.g. select the object if unselected, and vice versa.

	if (click_obj.valid())
	{
		bool was_empty = edit.Selected->empty();

		Editor_ClearErrorMode();

		// check if pointing at the same object as before
		Objid near_obj;

		GetNearObject(near_obj, edit.mode, edit.map_x, edit.map_y);

		if (near_obj != click_obj)
			return;

		// begin drawing mode (unless a modifier was pressed)
		if (easier_drawing_mode && edit.mode == OBJ_VERTICES &&
			was_empty && edit.button_mod == 0)
		{
			Editor_SetAction(ACT_DRAW_LINE);
			edit.drawing_from = click_obj.num;
			edit.Selected->set(click_obj.num);

			RedrawMap();
			return;
		}

		edit.Selected->toggle(click_obj.num);
		RedrawMap();
		return;
	}
}


void Editor_LeaveWindow()
{
	edit.pointer_in_window = false;

	// this offers a handy way to get out of drawing mode
	if (edit.action == ACT_DRAW_LINE)
		Editor_ClearAction();

	UpdateHighlight();
}


void Editor_MouseMotion(int x, int y, keycode_t mod)
{
	int map_x, map_y;

	main_win->canvas->PointerPos(&map_x, &map_y);

	edit.map_x = map_x;
	edit.map_y = map_y;
	edit.pointer_in_window = true; // FIXME

	if (edit.pointer_in_window)
		main_win->info_bar->SetMouse(edit.map_x, edit.map_y);

//  fprintf(stderr, "MOUSE MOTION: %d,%d  map: %d,%d\n", x, y, edit.map_x, edit.map_y);

	if (edit.action == ACT_TRANSFORM)
	{
		main_win->canvas->TransformUpdate(edit.map_x, edit.map_y);
		return;
	}

	if (edit.action == ACT_DRAW_LINE)
	{
		UpdateHighlight();
		main_win->canvas->redraw();
		return;
	}

	if (edit.action == ACT_SELBOX)
	{
		main_win->canvas->SelboxUpdate(edit.map_x, edit.map_y);
		return;
	}

	if (edit.action == ACT_DRAG)
	{
		main_win->canvas->DragUpdate(edit.map_x, edit.map_y);

		// if dragging a single vertex, update the possible split_line
		UpdateHighlight();
		return;
	}

#if 1
	//
	// begin dragging?
	//
	int pixel_dx = Fl::event_x() - mouse_button1_x;
	int pixel_dy = Fl::event_y() - mouse_button1_y;

	if (edit.button_down == 1 && edit.clicked.valid() &&
		MAX(abs(pixel_dx), abs(pixel_dy)) >= minimum_drag_pixels)
	{
		Editor_SetAction(ACT_DRAG);

		// if highlighted object is in selection, we drag the selection,
		// otherwise we drag just this one object

		if (! edit.Selected->get(edit.clicked.num))
			edit.drag_single_obj = edit.clicked.num;
		else
			edit.drag_single_obj = -1;

		int focus_x, focus_y;

		GetDragFocus(&focus_x, &focus_y, button1_map_x, button1_map_y);

		main_win->canvas->DragBegin(focus_x, focus_y, button1_map_x, button1_map_y);

///---		if (edit.mode == OBJ_VERTICES && edit.Selected->find_second() < 0)
///---		{
///---			edit.drag_single_vertex = edit.Selected->find_first();
///---			SYS_ASSERT(edit.drag_single_vertex >= 0);
///---		}

		UpdateHighlight();
		return;
	}
#endif


	// in general, just update the highlight, split-line (etc)

	UpdateHighlight();
}


//------------------------------------------------------------------------


int Editor_RawKey(int event)
{
	Nav_UpdateKeys();

	if (event == FL_KEYUP || event == FL_RELEASE)
		return 0;

	int raw_key = Fl::event_key();

	if (event == FL_PUSH)
	{
		// convert mouse button to a fake key
		raw_key = FL_Button + Fl::event_button();
	}
	else if (event == FL_MOUSEWHEEL)
	{
		// convert wheel direction to a fake key
		if (abs(wheel_dy) >= abs(wheel_dx))
			raw_key = wheel_dy < 0 ? FL_WheelUp : FL_WheelDown;
		else
			raw_key = wheel_dx < 0 ? FL_WheelLeft : FL_WheelRight;
	}

	int raw_state = Fl::event_state();

	int old_sticky_mod = edit.sticky_mod;

	if (edit.sticky_mod)
	{
		raw_state = edit.sticky_mod;
		ClearStickyMod();
	}

	keycode_t key = M_TranslateKey(raw_key, raw_state);

	if (key == 0)
		return 1;

#if 0  // DEBUG
	fprintf(stderr, "Key code: 0x%08x : %s\n", key, M_KeyToString(key));
#endif

	// keyboard propagation logic

	if (main_win->browser->visible() && ExecuteKey(key, KCTX_Browser))
		return 1;

	if (edit.render3d && ExecuteKey(key, KCTX_Render))
		return 1;

	if (ExecuteKey(key, M_ModeToKeyContext(edit.mode)))
		return 1;

	if (ExecuteKey(key, KCTX_General))
		return 1;


	// always eat mouse buttons
	if (event == FL_PUSH)
		return 1;


	// NOTE: the key may still get handled by something (e.g. Menus)
	// fprintf(stderr, "Unknown key %d (0x%04x)\n", key, key);


	// prevent a META-fied key from being sent elsewhere, because it
	// won't really be META-fied anywhere else -- including the case
	// of it being sent back to this function as a SHORTCUT event.
	return old_sticky_mod ? 1 : 0;
}


int Editor_RawWheel(int event)
{
	ClearStickyMod();

	// ensure we zoom from correct place
	main_win->canvas->PointerPos(&edit.map_x, &edit.map_y);

	wheel_dx = Fl::event_dx();
	wheel_dy = Fl::event_dy();

	if (wheel_dx == 0 && wheel_dy == 0)
		return 1;

	Editor_RawKey(FL_MOUSEWHEEL);

	return 1;
}


int Editor_RawButton(int event)
{
	ClearStickyMod();

	// Hack Alert : this is required to support pressing two buttons at the
	// same time.  Without this, FLTK does not send us the second button
	// release event, because when the first button is released the "pushed"
	// widget becomes NULL.

	if (Fl::event_buttons() != 0)
		Fl::pushed(main_win->canvas);


	int button = Fl::event_button();

	bool down = (event == FL_PUSH);

	if (button >= 2)
	{
		return Editor_RawKey(event);
	}


	Nav_UpdateKeys();


//---	// adjust offsets on a sidedef?
//---	if (edit.render3d && button == 2)
//---	{
//---		Render3D_AdjustOffsets(down ? -1 : +1);
//---		return 1;
//---	}

	if (! down)
	{
		if (! edit.render3d)
			Editor_MouseRelease();
		return 1;
	}

	int mod = Fl::event_state() & MOD_ALL_MASK;

	if (! edit.render3d)
	{
		Editor_MousePress(mod);
	}

	return 1;
}


int Editor_RawMouse(int event)
{
	int mod = Fl::event_state() & MOD_ALL_MASK;

	int dx = Fl::event_x() - mouse_last_x;
	int dy = Fl::event_y() - mouse_last_y;


	if (edit.is_scrolling)
	{
		Editor_ScrollMap(0, dx, dy);
	}
	else if (edit.action == ACT_ADJUST_OFS)
	{
		Render3D_AdjustOffsets(0, dx, dy);
	}
	else if (edit.render3d)
	{
		Render3D_MouseMotion(Fl::event_x(), Fl::event_y(), mod);
	}
	else
	{
		Editor_MouseMotion(Fl::event_x(), Fl::event_y(), mod);
	}

	mouse_last_x = Fl::event_x();
	mouse_last_y = Fl::event_y();

	return 1;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
