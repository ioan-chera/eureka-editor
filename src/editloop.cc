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

#include "main.h"

#include "e_checks.h"
#include "editloop.h"
#include "e_cutpaste.h"
#include "r_grid.h"
#include "e_linedef.h"
#include "e_loadsave.h"
#include "e_sector.h"
#include "e_path.h"
#include "e_vertex.h"
#include "levels.h"
#include "objects.h"
#include "x_mirror.h"
#include "x_hover.h"
#include "r_render.h"
#include "ui_window.h"
#include "ui_misc.h"


Editor_State_t  edit;


int active_when = 0;  // FIXME MOVE THESE into Editor_State
int active_wmask = 0;



// config items
int default_edit_mode = 0;  // Things

bool same_mode_clears_selection = false;

int multi_select_modifier = 0;
int minimum_drag_pixels = 5;

int sector_render_default = (int)SREND_Floor;


extern bool easier_drawing_mode;

void Editor_MouseMotion(int x, int y, keycode_t mod);


void ClearStickyMod()
{
	if (edit.sticky_mod)
		Status_Clear();

	edit.sticky_mod = 0;
}


/*
 *  zoom_fit - adjust zoom factor to make level fit in window
 *
 *  Return 0 on success, non-zero on failure.
 */
static void zoom_fit()
{
	if (NumVertices == 0)
		return;

	double xzoom = 1;
	double yzoom = 1;

	int ScrMaxX = main_win->canvas->w();
	int ScrMaxY = main_win->canvas->h();

	if (Map_bound_x1 < Map_bound_x2)
		xzoom = ScrMaxX / (double)(Map_bound_x2 - Map_bound_x1);

	if (Map_bound_y1 < Map_bound_y2)
		yzoom = ScrMaxY / (double)(Map_bound_y2 - Map_bound_y1);

	grid.NearestScale(MIN(xzoom, yzoom));

	grid.CenterMapAt((Map_bound_x1 + Map_bound_x2) / 2, (Map_bound_y1 + Map_bound_y2) / 2);
}


void RedrawMap()
{
	if (! main_win)
		return;

	if (edit.render3d)
		main_win->render->redraw();
	else
		main_win->canvas->redraw();
}


static void UpdatePanel()
{
	// -AJA- I think the highlighted object is always the same type as
	//       the current editing mode.  But do this check for safety.
	if (edit.highlight.valid() &&
		edit.highlight.type != edit.mode)
		return;


	// Choose object to show in right-hand panel:
	//   - the highlighted object takes priority
	//   - otherwise show the selection (first + count)
	//
	// It's a little more complicated since highlight may or may not
	// be part of the selection.

	int obj_idx   = edit.highlight.num;
	int obj_count = edit.Selected->count_obj();

	if (obj_idx >= 0)
	{
		if (! edit.Selected->get(obj_idx))
			obj_count = 0;
	}
	else if (obj_count > 0)
	{
		// in linedef mode, we want a two-sided linedef to show
		if (edit.mode == OBJ_LINEDEFS && obj_count > 1)
			obj_idx = Selection_FirstLine(edit.Selected);
		else
			obj_idx = edit.Selected->find_first();
	}


	switch (edit.mode)
	{
		case OBJ_THINGS:
			main_win->thing_box->SetObj(obj_idx, obj_count);
			break;

		case OBJ_LINEDEFS:
			main_win->line_box->SetObj(obj_idx, obj_count);
			break;

		case OBJ_SECTORS:
			main_win->sec_box->SetObj(obj_idx, obj_count);
			break;

		case OBJ_VERTICES:
			main_win->vert_box->SetObj(obj_idx, obj_count);
			break;

		default: break;
	}
}


static void UpdateSplitLine(int map_x, int map_y)
{
	edit.split_line.clear();

	// usually disabled while dragging stuff
	if (edit.action == ACT_DRAG && edit.drag_single_vertex < 0)
		return;

	// in vertex mode, see if there is a linedef which would be split by
	// adding a new vertex

	if (edit.mode == OBJ_VERTICES &&
		edit.pointer_in_window && !edit.render3d &&
	    edit.highlight.is_nil())
	{
		GetSplitLineDef(edit.split_line, map_x, map_y, edit.drag_single_vertex);

		// NOTE: OK if the split line has one of its vertices selected
		//       (that case is handled by Insert_Vertex)
	}

	if (edit.split_line.valid())
	{
		edit.split_x = grid.SnapX(map_x);
		edit.split_y = grid.SnapY(map_y);

		// in FREE mode, ensure the new vertex is directly on the linedef
		if (! grid.snap)
			MoveCoordOntoLineDef(edit.split_line.num, &edit.split_x, &edit.split_y);

		main_win->canvas->SplitLineSet(edit.split_line.num, edit.split_x, edit.split_y);
	}
	else
		main_win->canvas->SplitLineForget();
}


void UpdateHighlight()
{
	bool dragging = (edit.action == ACT_DRAG);


	// find the object to highlight
	edit.highlight.clear();

	if (edit.pointer_in_window && !edit.render3d &&
	    (!dragging || edit.drag_single_vertex >= 0))
	{
		GetNearObject(edit.highlight, edit.mode, edit.map_x, edit.map_y);

		// guarantee that we cannot drag a vertex onto itself
		if (edit.drag_single_vertex >= 0 && edit.highlight.valid() &&
			edit.drag_single_vertex == edit.highlight.num)
		{
			edit.highlight.clear();
		}
	}


	UpdateSplitLine(edit.map_x, edit.map_y);


	// in drawing mode, highlight extends to a vertex at the snap position
	if (edit.mode == OBJ_VERTICES && grid.snap &&
		edit.action == ACT_DRAW_LINE &&
		edit.highlight.is_nil() && edit.split_line.is_nil())
	{
		int new_x = grid.SnapX(edit.map_x);
		int new_y = grid.SnapY(edit.map_y);

		int near_vert = Vertex_FindExact(new_x, new_y);

		if (near_vert >= 0)
		{
			edit.highlight = Objid(OBJ_VERTICES, near_vert);
		}
		else
		{
			UpdateSplitLine(new_x, new_y);
		}
	}


	if (edit.highlight.valid())
		main_win->canvas->HighlightSet(edit.highlight);
	else
		main_win->canvas->HighlightForget();


	UpdatePanel();
}


bool GetCurrentObjects(selection_c *list)
{
	// returns false when there are no objects at all

	list->change_type(edit.mode);  // this also clears it

	if (edit.Selected->notempty())
	{
		list->merge(*edit.Selected);
		return true;
	}

	if (edit.highlight.valid())
	{
		list->set(edit.highlight.num);
		return true;
	}

	return false;
}


void Editor_ClearErrorMode()
{
	if (edit.error_mode)
	{
		Selection_Clear();
	}
}


void Editor_ChangeMode_Raw(obj_type_e new_mode)
{
	// keep selection after a "Find All" and user dismisses panel
	if (new_mode == edit.mode && main_win->isSpecialPanelShown())
		edit.error_mode = false;

	edit.mode = new_mode;

	Editor_ClearAction();
	Editor_ClearErrorMode();

	edit.highlight.clear();
	edit.split_line.clear();
	edit.did_a_move = false;
}


void Editor_ChangeMode(char mode_char)
{
	obj_type_e  prev_type = edit.mode;

	// Set the object type according to the new mode.
	switch (mode_char)
	{
		case 't': Editor_ChangeMode_Raw(OBJ_THINGS);   break;
		case 'l': Editor_ChangeMode_Raw(OBJ_LINEDEFS); break;
		case 's': Editor_ChangeMode_Raw(OBJ_SECTORS);  break;
		case 'v': Editor_ChangeMode_Raw(OBJ_VERTICES); break;

		default:
			LogPrintf("INTERNAL ERROR: unknown mode %d\n", mode_char);
			return;
	}

	if (prev_type != edit.mode)
	{
		Selection_Push();

		main_win->NewEditMode(edit.mode);

		// convert the selection
		selection_c *prev_sel = edit.Selected;
		edit.Selected = new selection_c(edit.mode);

		ConvertSelection(prev_sel, edit.Selected);
		delete prev_sel;
	}
	else if (main_win->isSpecialPanelShown())
	{
		// same mode, but this removes the special panel
		main_win->NewEditMode(edit.mode);
	}
	// -AJA- Yadex (DEU?) would clear the selection if the mode didn't
	//       change.  We optionally emulate that behavior here.
	else if (same_mode_clears_selection)
	{
		Selection_Clear();
	}

	UpdateHighlight();
	RedrawMap();
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


void Editor_Zoom(int delta, int mid_x, int mid_y)
{
    float S1 = grid.Scale;

    grid.AdjustScale(delta);

    grid.RefocusZoom(mid_x, mid_y, S1);
}


static void Editor_ScrollMap(int mode, int dx = 0, int dy = 0)
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


static void Editor_ClearNav()
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
		if (edit.did_a_move)
			Selection_Clear();

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

	bool was_did_move = edit.did_a_move;
	edit.did_a_move = false;

	// releasing the button while dragging : drop the selection.
#if 1
	if (edit.action == ACT_DRAG)
	{
		Editor_ClearAction();

		int dx, dy;
		main_win->canvas->DragFinish(&dx, &dy);

		if (! (dx==0 && dy==0))
		{
			CMD_MoveObjects(dx, dy);

			// next select action will clear the selection
			edit.did_a_move = true;
		}

		edit.drag_single_vertex = -1;
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
		was_did_move = true;
	}


	// handle a clicked-on object
	// e.g. select the object if unselected, and vice versa.

	if (click_obj.valid())
	{
		bool was_empty = edit.Selected->empty();

		Editor_ClearErrorMode();

		if (was_did_move)
			Selection_Clear();

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

	if (edit.action == ACT_SCALE)
	{
		main_win->canvas->ScaleUpdate(edit.map_x, edit.map_y, mod);
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

#if 0
	//
	// begin dragging?
	//
	int pixel_dx = Fl::event_x() - mouse_button1_x;
	int pixel_dy = Fl::event_y() - mouse_button1_y;

	if (edit.button_down == 1 && edit.clicked.valid() &&
		MAX(abs(pixel_dx), abs(pixel_dy)) >= minimum_drag_pixels)
	{
		if (! edit.Selected->get(edit.clicked.num))
		{
			if (edit.did_a_move)
				Selection_Clear();

			edit.Selected->set(edit.clicked.num);
			edit.did_a_move = false;
		}

		int focus_x, focus_y;

		GetDragFocus(&focus_x, &focus_y, button1_map_x, button1_map_y);

		Editor_SetAction(ACT_DRAG);
		main_win->canvas->DragBegin(focus_x, focus_y, button1_map_x, button1_map_y);

		// check for a single vertex
		edit.drag_single_vertex = -1;

		if (edit.mode == OBJ_VERTICES && edit.Selected->find_second() < 0)
		{
			edit.drag_single_vertex = edit.Selected->find_first();
			SYS_ASSERT(edit.drag_single_vertex >= 0);
		}

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


//------------------------------------------------------------------------
//  COMMAND FUNCTIONS
//------------------------------------------------------------------------


void CMD_Nothing(void)
{
	/* hey jude, don't make it bad */
}


void CMD_MetaKey(void)
{
	if (edit.sticky_mod)
	{
		ClearStickyMod();
		return;
	}

	Status_Set("META...");

	edit.sticky_mod = MOD_META;
}


void CMD_EditMode(void)
{
	char mode = tolower(EXEC_Param[0][0]);

	if (! mode || ! strchr("lstvr", mode))
	{
		Beep("Bad parameter for EditMode: '%s'", EXEC_Param[0]);
		return;
	}

	Editor_ChangeMode(mode);
}


void CMD_SelectAll(void)
{
	Editor_ClearErrorMode();

	int total = NumObjects(edit.mode);

	Selection_Push();

	edit.Selected->change_type(edit.mode);
	edit.Selected->frob_range(0, total-1, BOP_ADD);

	UpdateHighlight();
	RedrawMap();
}


void CMD_UnselectAll(void)
{
	Editor_ClearErrorMode();

	if (edit.action == ACT_DRAW_LINE ||
		edit.action == ACT_SCALE)
	{
		Editor_ClearAction();
	}

	Selection_Clear();

	UpdateHighlight();
	RedrawMap();
}


void CMD_InvertSelection(void)
{
	// do not clear selection when in error mode
	edit.error_mode = false;

	int total = NumObjects(edit.mode);

	if (edit.Selected->what_type() != edit.mode)
	{
		// convert the selection
		selection_c *prev_sel = edit.Selected;
		edit.Selected = new selection_c(edit.mode);

		ConvertSelection(prev_sel, edit.Selected);
		delete prev_sel;
	}

	edit.Selected->frob_range(0, total-1, BOP_TOGGLE);

	UpdateHighlight();
	RedrawMap();
}


void CMD_Quit(void)
{
	want_quit = true;
}


void CMD_Undo(void)
{
	if (! BA_Undo())
	{
		Beep("No operation to undo");
		return;
	}

	RedrawMap();
	main_win->UpdatePanelObj();
}


void CMD_Redo(void)
{
	if (! BA_Redo())
	{
		Beep("No operation to redo");
		return;
	}

	RedrawMap();
	main_win->UpdatePanelObj();
}


void CMD_SetVar(void)
{
	const char *var_name = EXEC_Param[0];
	const char *value    = EXEC_Param[0];

	if (! var_name[0])
	{
		Beep("Set: missing var name");
		return;
	}

	if (! value[0])
	{
		Beep("Set: missing value");
		return;
	}

	 int  int_val = atoi(value);
	bool bool_val = (int_val > 0);


	if (y_stricmp(var_name, "3d") == 0)
	{
		Render3D_Enable(bool_val);
	}
	else if (y_stricmp(var_name, "browser") == 0)
	{
		Editor_ClearAction();

		int want_vis   = bool_val ? 1 : 0;
		int is_visible = main_win->browser->visible() ? 1 : 0;

		if (want_vis != is_visible)
			main_win->ShowBrowser('/');
	}
	else if (y_stricmp(var_name, "grid") == 0)
	{
		grid.SetShown(bool_val);
	}
	else if (y_stricmp(var_name, "snap") == 0)
	{
		grid.SetSnap(bool_val);
	}
	else if (y_stricmp(var_name, "obj_nums") == 0)
	{
		edit.show_object_numbers = bool_val;
		RedrawMap();
	}
	else   // TODO: "skills"
	{
		Beep("Set: unknown var: %s", var_name);
	}
}


void CMD_ToggleVar(void)
{
	const char *var_name = EXEC_Param[0];

	if (! var_name[0])
	{
		Beep("Toggle: missing var name");
		return;
	}

	if (y_stricmp(var_name, "3d") == 0)
	{
		Render3D_Enable(! edit.render3d);
	}
	else if (y_stricmp(var_name, "browser") == 0)
	{
		Editor_ClearAction();

		main_win->ShowBrowser('/');
	}
	else if (y_stricmp(var_name, "recent") == 0)
	{
		main_win->browser->ToggleRecent();
	}
	else if (y_stricmp(var_name, "grid") == 0)
	{
		grid.ToggleShown();
	}
	else if (y_stricmp(var_name, "snap") == 0)
	{
		grid.ToggleSnap();
	}
	else if (y_stricmp(var_name, "obj_nums") == 0)
	{
		edit.show_object_numbers = ! edit.show_object_numbers;
		RedrawMap();
	}
	else if (y_stricmp(var_name, "skills") == 0)
	{
		active_wmask ^= 1;
		active_when = active_wmask;
		RedrawMap();
	}
	else
	{
		Beep("Toggle: unknown var: %s", var_name);
	}
}


void CMD_BrowserMode(void)
{
	if (! EXEC_Param[0][0])
	{
		Beep("Missing parameter to CMD_BrowserMode");
		return;
	}

	char mode = toupper(EXEC_Param[0][0]);

	if (! (mode == 'L' || mode == 'S' || mode == 'O' ||
	       mode == 'T' || mode == 'F' || mode == 'G'))
	{
		Beep("Unknown browser mode: %s", EXEC_Param[0]);
		return;
	}

	main_win->ShowBrowser(mode);
}


void CMD_Scroll(void)
{
	// these are percentages
	float delta_x = atof(EXEC_Param[0]);
	float delta_y = atof(EXEC_Param[1]);

	if (delta_x == 0 && delta_y == 0)
	{
		Beep("Bad parameter to Scroll: '%s' %s'", EXEC_Param[0], EXEC_Param[1]);
		return;
	}

	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;

	delta_x = delta_x * base_size / 100.0 / grid.Scale;
	delta_y = delta_y * base_size / 100.0 / grid.Scale;

	grid.Scroll(delta_x, delta_y);

	// certain actions need to be updated
	if (edit.action == ACT_SELBOX || edit.action == ACT_DRAW_LINE ||
		edit.action == ACT_DRAG)
	{
		int mod = Fl::event_state() & MOD_ALL_MASK;

		Editor_MouseMotion(Fl::event_x(), Fl::event_y(), mod);
	}
}


static void NAV_Scroll_X_release(void)
{
	edit.nav_scroll_dx = 0;
}

void CMD_NAV_Scroll_X(void)
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = atof(EXEC_Param[0]);

	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;

	edit.nav_scroll_dx = perc * base_size / 100.0 / grid.Scale;

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_X_release);
}


static void NAV_Scroll_Y_release(void)
{
	edit.nav_scroll_dy = 0;
}

void CMD_NAV_Scroll_Y(void)
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = atof(EXEC_Param[0]);

	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;

	edit.nav_scroll_dy = perc * base_size / 100.0 / grid.Scale;

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Y_release);
}


static void NAV_Scroll_Mouse_release(void)
{
	Editor_ScrollMap(+1);
}

void CMD_NAV_Scroll_Mouse(void)
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	if (Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Mouse_release))
	{
		Editor_ScrollMap(-1);
	}
}


static void ACT_SelectBox_release(void)
{
	// check if cancelled or overridden
	if (edit.action != ACT_SELBOX)
		return;

	Editor_ClearAction();
	Editor_ClearErrorMode();

	int x1, y1, x2, y2;

	main_win->canvas->SelboxFinish(&x1, &y1, &x2, &y2);

	// a mere click and release will unselect everything
	// FIXME : REVIEW THIS
	if (x1 == x2 && y1 == y2)
		CMD_UnselectAll();
	else
		SelectObjectsInBox(edit.Selected, edit.mode, x1, y1, x2, y2);

	UpdateHighlight();
	RedrawMap();
}

void CMD_ACT_SelectBox(void)
{
	if (edit.render3d)
		return;

	if (! EXEC_CurKey)
		return;

	if (Nav_ActionKey(EXEC_CurKey, &ACT_SelectBox_release))
	{
		Editor_SetAction(ACT_SELBOX);

		main_win->canvas->SelboxBegin(edit.map_x, edit.map_y);
	}
}


static void ACT_Drag_release(void)
{
	// check if cancelled or overridden
	if (edit.action != ACT_DRAG)
		return;

	Editor_ClearAction();

	int dx, dy;
	main_win->canvas->DragFinish(&dx, &dy);

	if (! (dx==0 && dy==0))
	{
		CMD_MoveObjects(dx, dy);

//???	// next select action will clear the selection
//???	edit.did_a_move = true;
	}

	edit.drag_single_vertex = -1;

	UpdateHighlight();
	RedrawMap();
}

void CMD_ACT_Drag(void)
{
	if (edit.render3d)
		return;

	if (! EXEC_CurKey)
		return;

	if (edit.Selected->empty())
	{
		Beep("Nothing to drag");
		return;
	}

	if (Nav_ActionKey(EXEC_CurKey, &ACT_Drag_release))
	{
		int focus_x, focus_y;

		GetDragFocus(&focus_x, &focus_y, edit.map_x, edit.map_y);

		Editor_SetAction(ACT_DRAG);
		main_win->canvas->DragBegin(focus_x, focus_y, edit.map_x, edit.map_y);

		// check for a single vertex
		edit.drag_single_vertex = -1;

#if 0  //????
		if (edit.mode == OBJ_VERTICES && edit.Selected->find_second() < 0)
		{
			edit.drag_single_vertex = edit.Selected->find_first();
			SYS_ASSERT(edit.drag_single_vertex >= 0);
		}
#endif

		UpdateHighlight();
	}
}


static void ACT_Scale_release(void)
{
	// check if cancelled or overridden
	if (edit.action != ACT_SCALE)
		return;

	Editor_ClearAction();

	scale_param_t param;

	main_win->canvas->ScaleFinish(param);

	CMD_ScaleObjects2(param);

	RedrawMap();
}

void CMD_ACT_Scale(void)
{
	if (edit.render3d)
		return;

	if (! EXEC_CurKey)
		return;

	if (edit.Selected->empty())
	{
		Beep("Nothing to scale");
		return;
	}


	if (! Nav_ActionKey(EXEC_CurKey, &ACT_Scale_release))
		return;


	// FIXME : button_mod is probably obsolete
	edit.button_mod = Fl::event_state() & MOD_ALL_MASK;

	int middle_x, middle_y;

	Objs_CalcMiddle(edit.Selected, &middle_x, &middle_y);

	main_win->canvas->ScaleBegin(edit.map_x, edit.map_y, middle_x, middle_y);

	Editor_SetAction(ACT_SCALE);
}


void CMD_WHEEL_Scroll()
{
	float speed = atof(EXEC_Param[0]);

//???	if (mod == MOD_SHIFT)
//???		speed /= 3.0;

	float delta_x =     wheel_dx;
	float delta_y = 0 - wheel_dy;

	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;

	speed = speed * base_size / 100.0 / grid.Scale;

	grid.Scroll(delta_x * speed, delta_y * speed);
}


void CMD_Merge(void)
{
	switch (edit.mode)
	{
		case OBJ_VERTICES:
			VT_Merge();
			break;

		case OBJ_LINEDEFS:
			LIN_MergeTwo();
			break;

		case OBJ_SECTORS:
			SEC_Merge();
			break;

		case OBJ_THINGS:
			TH_Merge();
			break;

		default:
			Beep("Cannot merge that");
			break;
	}
}


void CMD_Disconnect(void)
{
	switch (edit.mode)
	{
		case OBJ_VERTICES:
			VT_Disconnect();
			break;

		case OBJ_LINEDEFS:
			LIN_Disconnect();
			break;

		case OBJ_SECTORS:
			SEC_Disconnect();
			break;

		case OBJ_THINGS:
			TH_Disconnect();
			break;

		default:
			Beep("Cannot disconnect that");
			break;
	}
}


void CMD_Zoom(void)
{
	int delta = atoi(EXEC_Param[0]);

	if (delta == 0)
	{
		Beep("Bad parameter to CMD_Zoom");
		return;
	}

	Editor_Zoom(delta, edit.map_x, edit.map_y);
}


void CMD_ZoomWholeMap(void)
{
	if (MadeChanges)
		CalculateLevelBounds();

	zoom_fit();

	RedrawMap();
}


void CMD_ZoomSelection(void)
{
	if (edit.Selected->empty())
	{
		Beep("No selection to zoom");
		return;
	}

	GoToSelection();
}


void CMD_GoToCamera(void)
{
	int x, y;
	float angle;

	Render3D_GetCameraPos(&x, &y, &angle);

	grid.CenterMapAt(x, y);

	// FIXME: this is not right, we want to recompute where mouse pointer is
	edit.map_x = x;
	edit.map_y = y;

	RedrawMap();
}


void CMD_PlaceCamera(void)
{
	if (edit.render3d)
	{
		Beep("Not supported in 3D view");
		return;
	}

	if (! edit.pointer_in_window)
	{
		// IDEA: turn cursor into cross, wait for click in map window

		Beep("Mouse is not over map");
		return;
	}

	int x = edit.map_x;
	int y = edit.map_y;

	Render3D_SetCameraPos(x, y);

	if (Exec_HasFlag("/open3d"))
	{
		Render3D_Enable(true);
	}

	RedrawMap();
}


void CMD_Gamma(void)
{
	int delta = (atoi(EXEC_Param[0]) >= 0) ? +1 : -1;

	usegamma = (usegamma + delta + 5) % 5;

	W_UpdateGamma();

	Status_Set("Gamma level %d", usegamma);

	RedrawMap();
}


void CMD_CopyAndPaste(void)
{
	if (edit.Selected->empty() && edit.highlight.is_nil())
	{
		Beep("Nothing to copy and paste");
		return;
	}

	if (CMD_Copy())
	{
		CMD_Paste();
	}
}


void CMD_MoveObjects_Dialog()
{
	if (edit.Selected->empty())
	{
		Beep("Nothing to move");
		return;
	}

	UI_MoveDialog * dialog = new UI_MoveDialog();

	dialog->Run();

	delete dialog;
}


void CMD_ScaleObjects_Dialog()
{
	if (edit.Selected->empty())
	{
		Beep("Nothing to scale");
		return;
	}

	UI_ScaleDialog * dialog = new UI_ScaleDialog();

	dialog->Run();

	delete dialog;
}


void CMD_RotateObjects_Dialog()
{
	if (edit.Selected->empty())
	{
		Beep("Nothing to rotate");
		return;
	}

	UI_RotateDialog * dialog = new UI_RotateDialog();

	dialog->Run();

	delete dialog;
}


void GRID_Bump(void)
{
	int delta = atoi(EXEC_Param[0]);

	delta = (delta >= 0) ? +1 : -1;

	grid.AdjustStep(delta);
}


void GRID_Set(void)
{
	int step = atoi(EXEC_Param[0]);

	if (step < 2 || step > 4096)
	{
		Beep("Bad grid step");
		return;
	}

	grid.SetStep(step);
}


void GRID_Zoom(void)
{
	// target scale is positive for NN:1 and negative for 1:NN

	double scale = atof(EXEC_Param[0]);

	if (scale == 0)
	{
		Beep("Bad scale");
		return;
	}

	if (scale < 0)
		scale = -1.0 / scale;

	float S1 = grid.Scale;

	grid.NearestScale(scale);

	grid.RefocusZoom(edit.map_x, edit.map_y, S1);
}


void BR_CycleCategory(void)
{
	if (! main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	int dir = (atoi(EXEC_Param[0]) >= 0) ? +1 : -1;

	main_win->browser->CycleCategory(dir);
}

void BR_ClearSearch(void)
{
	if (! main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	main_win->browser->ClearSearchBox();
}


void BR_Scroll(void)
{
	if (! main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	if (! EXEC_Param[0][0])
	{
		Beep("Missing parameter to BR_Scroll");
		return;
	}

	int delta = atoi(EXEC_Param[0]);

	main_win->browser->Scroll(delta);
}


//------------------------------------------------------------------------


static editor_command_t  command_table[] =
{
	/* ------ interface stuff ------ */

	{	"Nothing",
		&CMD_Nothing
	},

	{	"Quit",
		&CMD_Quit
	},

	{	"EditMode",
		&CMD_EditMode,
		/* flags */ NULL,
		/* keywords */ "thing line sector vertex"
	},

 	{	"BrowserMode",
		&CMD_BrowserMode,
		/* flags */ NULL,
		/* keywords */ "obj tex flat line sec genline"
	},

	{	"Set",
		&CMD_SetVar,
		/* flags */ NULL,
		/* keywords */ "3d browser grid obj_nums snap"
	},

	{	"Toggle",
		&CMD_ToggleVar,
		/* flags */ NULL,
		/* keywords */ "3d browser grid obj_nums snap recent"
	},

	{	"Check",
		&CMD_CheckMap,
		/* flags */ NULL,
		/* keywords */ "all major vertices sectors linedefs things textures tags current"
	},

	{	"MetaKey",
		&CMD_MetaKey
	},

	{	"GivenFile",
		&CMD_GivenFile,
		/* flags */ NULL,
		/* keywords */ "next prev first last current"
	},

	{	"FlipMap",
		&CMD_FlipMap,
		/* flags */ NULL,
		/* keywords */ "next prev first last"
	},

	{	"SelectAll",
		&CMD_SelectAll
	},

	{	"UnselectAll",
		&CMD_UnselectAll
	},

	{	"InvertSelection",
		&CMD_InvertSelection
	},

	{	"LastSelection",
		&CMD_LastSelection
	},

	{	"Scroll",
		&CMD_Scroll
	},

	{	"NAV_Scroll_X",
		&CMD_NAV_Scroll_X
	},

	{	"NAV_Scroll_Y",
		&CMD_NAV_Scroll_Y
	},

	{	"NAV_Scroll_Mouse",
		&CMD_NAV_Scroll_Mouse
	},

	{	"ACT_SelectBox",
		&CMD_ACT_SelectBox
	},

	{	"ACT_Drag",
		&CMD_ACT_Drag
	},

	{	"ACT_Scale",
		&CMD_ACT_Scale,
	},

	{	"WHEEL_Scroll",
		&CMD_WHEEL_Scroll
	},

	{	"GoToCamera",
		&CMD_GoToCamera
	},

	{	"PlaceCamera",
		&CMD_PlaceCamera,
		/* flags */ "/open3d"
	},

	{	"JumpToObject",
		&CMD_JumpToObject
	},

	{	"Zoom",
		&CMD_Zoom
	},

	{	"ZoomWholeMap",
		&CMD_ZoomWholeMap
	},

	{	"ZoomSelection",
		&CMD_ZoomSelection
	},

	{	"GRID_Bump",
		&GRID_Bump
	},

	{	"GRID_Set",
		&GRID_Set
	},

	{	"GRID_Zoom",
		&GRID_Zoom
	},


	/* ----- general map stuff ----- */

	{	"Insert",
		&CMD_Insert,
		/* flags */ "/new /continue /nofill"
	},

	{	"Delete",
		&CMD_Delete,
		/* flags */ "/keep_things /keep_unused"
	},

	{	"Mirror",
		&CMD_Mirror,
		/* flags */ NULL,
		/* keywords */ "horiz vert"
	},

	{	"Rotate90",
		&CMD_Rotate90,
		/* flags */ NULL,
		/* keywords */ "cw acw"
	},

	{	"Enlarge",
		&CMD_Enlarge
	},

	{	"Shrink",
		&CMD_Shrink
	},

	{	"Disconnect",
		&CMD_Disconnect
	},

	{	"Merge",
		&CMD_Merge,
		/* flags */ "/keep"
	},

	{	"Quantize",
		&CMD_Quantize
	},

	{	"Gamma",
		&CMD_Gamma
	},

	{	"CopyAndPaste",
		&CMD_CopyAndPaste
	},

	{	"CopyProperties",
		&CMD_CopyProperties,
		/* flags */ "/reverse"
	},

	{	"ApplyTag",
		&CMD_ApplyTag,
		/* flags */ NULL,
		/* keywords */ "fresh last"
	},

	{	"PruneUnused",
		&CMD_PruneUnused
	},

	{	"MoveObjectsDialog",
		&CMD_MoveObjects_Dialog
	},

	{	"ScaleObjectsDialog",
		&CMD_ScaleObjects_Dialog
	},

	{	"RotateObjectsDialog",
		&CMD_RotateObjects_Dialog
	},


	/* -------- linedef -------- */

	{	"LIN_Flip",
		&LIN_Flip,
		/* flags */ "/verts /sides"
	},

	{	"LIN_SplitHalf",
		&LIN_SplitHalf
	},

	{	"LIN_SelectPath",
		&LIN_SelectPath,
		/* flags */ "/add /onesided /sametex"
	},

	/* -------- sector -------- */

	{	"SEC_Floor",
		&SEC_Floor
	},

	{	"SEC_Ceil",
		&SEC_Ceil
	},

	{	"SEC_Light",
		&SEC_Light
	},

	{	"SEC_SelectGroup",
		&SEC_SelectGroup,
		/* flags */ "/add /can_walk /doors /floor_h /floor_tex /ceil_h /ceil_tex /light /tag /special"
	},

	{	"SEC_SwapFlats",
		&SEC_SwapFlats
	},

	/* -------- thing -------- */

	{	"TH_Spin",
		&TH_SpinThings
	},

	/* -------- vertex -------- */

	{	"VT_ShapeLine",
		&VT_ShapeLine
	},

	{	"VT_ShapeArc",
		&VT_ShapeArc
	},

	/* -------- browser -------- */

	{	"BR_CycleCategory",
		&BR_CycleCategory
	},

	{	"BR_ClearSearch",
		&BR_ClearSearch
	},

	{	"BR_Scroll",
		&BR_Scroll
	},

	// end of command list
	{	NULL, NULL	}
};


void Editor_RegisterCommands()
{
	M_RegisterCommandList(command_table);
}


void Editor_Init()
{
	memset(&edit, 0, sizeof(edit));  /* Catch-all */

	switch (default_edit_mode)
	{
		case 1:  edit.mode = OBJ_LINEDEFS; break;
		case 2:  edit.mode = OBJ_SECTORS;  break;
		case 3:  edit.mode = OBJ_VERTICES; break;
		default: edit.mode = OBJ_THINGS;   break;
	}

	edit.action = ACT_NOTHING;
	edit.is_scrolling  = false;

	Nav_Clear();

	edit.render3d = false;
	edit.error_mode = false;

	edit.sector_render_mode = sector_render_default;

	edit.show_object_numbers = false;
	edit.show_things_squares = false;
	edit.show_things_sprites = true;

	edit.button_down = 0;
	edit.button_mod  = 0;
	edit.clicked.clear();

	edit.highlight.clear();
	edit.split_line.clear();
	edit.drawing_from = -1;
	edit.drag_single_vertex = -1;

	edit.Selected = new selection_c(edit.mode);

	edit.did_a_move = false;

	grid.Init();

	MadeChanges = 0;

	Editor_RegisterCommands();
	Render3D_RegisterCommands();
}


bool Editor_ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "edit_mode") == 0 && num_tok >= 2)
	{
		Editor_ChangeMode(tokens[1][0]);
		return true;
	}

	if (strcmp(tokens[0], "render_mode") == 0 && num_tok >= 2)
	{
		edit.render3d = atoi(tokens[1]);
		UpdateHighlight();
		RedrawMap();
		return true;
	}

	if (strcmp(tokens[0], "sector_render_mode") == 0 && num_tok >= 2)
	{
		edit.sector_render_mode = atoi(tokens[1]);
		RedrawMap();
		return true;
	}

	if (strcmp(tokens[0], "show_object_numbers") == 0 && num_tok >= 2)
	{
		edit.show_object_numbers = atoi(tokens[1]);
		RedrawMap();
		return true;
	}

	return false;
}


void Editor_WriteUser(FILE *fp)
{
	switch (edit.mode)
	{
		case OBJ_THINGS:   fprintf(fp, "edit_mode t\n"); break;
		case OBJ_LINEDEFS: fprintf(fp, "edit_mode l\n"); break;
		case OBJ_SECTORS:  fprintf(fp, "edit_mode s\n"); break;
		case OBJ_VERTICES: fprintf(fp, "edit_mode v\n"); break;

		default: break;
	}

	fprintf(fp, "render_mode %d\n", edit.render3d ? 1 : 0);
	fprintf(fp, "sector_render_mode %d\n", edit.sector_render_mode);
	fprintf(fp, "show_object_numbers %d\n", edit.show_object_numbers ? 1 : 0);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
