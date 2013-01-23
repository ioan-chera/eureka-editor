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

#include "main.h"

#include "e_checks.h"
#include "m_dialog.h"
#include "editloop.h"
#include "e_cutpaste.h"
#include "r_misc.h"
#include "r_grid.h"
#include "e_linedef.h"
#include "e_sector.h"
#include "e_path.h"
#include "e_vertex.h"
#include "levels.h"
#include "objects.h"
#include "selectn.h"
#include "x_mirror.h"
#include "x_hover.h"
#include "r_render.h"
#include "ui_window.h"


Editor_State_c  edit;


int active_when = 0;  // FIXME MOVE THESE into Editor_State
int active_wmask = 0;



// config items
bool digits_set_zoom = false;
bool escape_key_quits = false;
bool mouse_wheel_scrolls_map = false;
bool same_mode_clears_selection = false; 

int multi_select_modifier = 0;


Editor_State_c::Editor_State_c()
    // FIXME !!!!
{
}

Editor_State_c::~Editor_State_c()
{
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

	if (MapBound_lx < MapBound_hx)
		xzoom = ScrMaxX / (double)(MapBound_hx - MapBound_lx);

	if (MapBound_ly < MapBound_hy)
		yzoom = ScrMaxY / (double)(MapBound_hy - MapBound_ly);

	grid.NearestScale(MIN(xzoom, yzoom));

	grid.CenterMapAt((MapBound_lx + MapBound_hx) / 2, (MapBound_ly + MapBound_hy) / 2);
}


static void UpdateSplitLine(int drag_vert = -1)
{
	edit.split_line.clear();

	// usually disabled while dragging stuff
	if (main_win->canvas->isDragActive() && edit.drag_single_vertex < 0)
		return;

	// in vertex mode, see if there is a linedef which would be split by
	// adding a new vertex

	if (edit.obj_type == OBJ_VERTICES && edit.pointer_in_window &&
	    edit.highlighted.is_nil())
	{
		GetSplitLineDef(edit.split_line, edit.map_x, edit.map_y, edit.drag_single_vertex);

		// NOTE: OK if the split line has one of its vertices selected
		//       (that case is handled by Insert_Vertex)
	}

	if (edit.split_line())
		main_win->canvas->SplitLineSet(edit.split_line.num);
	else
		main_win->canvas->SplitLineForget();
}


static void UpdatePanel()
{
	// choose object to show in right-hand Panel
	int obj_idx = edit.highlighted.num;
	int obj_count = (obj_idx < 0) ? 0 : 1;

	if (edit.highlighted.is_nil() && edit.Selected->notempty())
	{
		obj_idx = edit.Selected->find_first();
	}

	if (edit.Selected->notempty())
	{
		// handle object which is highlighted but NOT in the selection
		if (obj_idx >= 0 && ! edit.Selected->get(obj_idx))
			obj_count = 1;
		else
			obj_count = edit.Selected->count_obj();
	}


	// -AJA- I think the highlighted object is always the same type as
	//       the current editing mode.  But do this check for safety.
	if (edit.highlighted() && edit.highlighted.type != edit.obj_type)
		return;


	switch (edit.obj_type)
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


void UpdateHighlight()
{
	bool dragging = main_win->canvas->isDragActive();


	// find the object to highlight
	edit.highlighted.clear();

	if (edit.pointer_in_window &&
	    (!dragging || edit.drag_single_vertex >= 0))
	{
		GetCurObject(edit.highlighted, edit.obj_type, edit.map_x, edit.map_y, grid.snap);

		// guarantee that we cannot drag a vertex onto itself
		if (edit.drag_single_vertex >= 0 && edit.highlighted() &&
			edit.drag_single_vertex == edit.highlighted.num)
		{
			edit.highlighted.clear();
		}
	}


	if (edit.highlighted())
		main_win->canvas->HighlightSet(edit.highlighted);
	else
		main_win->canvas->HighlightForget();


	UpdateSplitLine();

	UpdatePanel();
}


bool GetCurrentObjects(selection_c *list)
{
	// returns false when there are no objects at all

	list->change_type(edit.obj_type);  // this also clears it

	if (edit.Selected->notempty())
	{
		list->merge(*edit.Selected);
		return true;
	}

	if (edit.highlighted())
	{
		list->set(edit.highlighted.num);
		return true;
	}

	return false;
}


void Editor_ChangeMode(char mode)
{
	obj_type_e prev_type = edit.obj_type;

	if (mode == 'R')
		mode = 'r';

	// Set the object type according to the new mode.
	switch (mode)
	{
		case 't': edit.obj_type = OBJ_THINGS;   break;
		case 'l': edit.obj_type = OBJ_LINEDEFS; break;
		case 's': edit.obj_type = OBJ_SECTORS;  break;
		case 'v': edit.obj_type = OBJ_VERTICES; break;
		case 'r': edit.obj_type = OBJ_RADTRIGS; break;

		default:
			LogPrintf("INTERNAL ERROR: unknown mode %d\n", mode);
			return;
	}

	edit.highlighted.clear();
	edit.split_line.clear();
	edit.did_a_move = false;

	if (prev_type != edit.obj_type)
	{
		main_win->NewEditMode(mode);

		// convert the selection
		selection_c *prev_sel = edit.Selected;
		edit.Selected = new selection_c(edit.obj_type);

		ConvertSelection(prev_sel, edit.Selected);
		delete prev_sel;
	}
	// -AJA- Yadex (DEU?) would clear the selection if the mode didn't
	//       change.  We optionally emulate that behavior here.
	else if (same_mode_clears_selection)
	{
		edit.Selected->clear_all();
	}

	UpdateHighlight();

	edit.RedrawMap = 1;
}


void CMD_Nothing(void)
{
	/* hey jude, don't make it bad */
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
	int total = NumObjects(edit.obj_type);

	edit.Selected->change_type(edit.obj_type);
	edit.Selected->frob_range(0, total-1, BOP_ADD);
	edit.RedrawMap = 1;

	UpdateHighlight();
}


void CMD_UnselectAll(void)
{
	edit.Selected->change_type(edit.obj_type);
	edit.Selected->clear_all();
	edit.RedrawMap = 1;

	UpdateHighlight();
}


void CMD_InvertSelection(void)
{
	int total = NumObjects(edit.obj_type);

	if (edit.Selected->what_type() != edit.obj_type)
	{
		// convert the selection
		selection_c *prev_sel = edit.Selected;
		edit.Selected = new selection_c(edit.obj_type);

		ConvertSelection(prev_sel, edit.Selected);
		delete prev_sel;
	}

	edit.Selected->frob_range(0, total-1, BOP_TOGGLE);
	edit.RedrawMap = 1;

	UpdateHighlight();
}


void CMD_Quit(void)
{
	want_quit = true;
}


void CMD_SetVar(void)
{
	if (! EXEC_Param[0][0])
	{
		Beep("Set: missing var name");
		return;
	}

	if (! EXEC_Param[0][1])
	{
		Beep("Set: missing value");
		return;
	}

	if (y_stricmp(EXEC_Param[0], "3d") == 0)
	{
		main_win->canvas->ChangeRenderMode(atoi(EXEC_Param[1]) > 0);
	}
	else if (y_stricmp(EXEC_Param[0], "browser") == 0)
	{
		int want_vis   = (atoi(EXEC_Param[0]) > 0) ? 1 : 0;
		int is_visible = main_win->browser->visible() ? 1 : 0;

		if (want_vis != is_visible)
			main_win->ShowBrowser('/');
	}
	else if (y_stricmp(EXEC_Param[0], "obj_nums") == 0)
	{
		edit.show_object_numbers = (atoi(EXEC_Param[1]) > 0);
		edit.RedrawMap = 1;
	}
	else   // TODO: "skills"
	{
		Beep("Set: unknown var '%s'", EXEC_Param[0]);
	}
}


void CMD_ToggleVar(void)
{
	if (! EXEC_Param[0][0])
	{
		Beep("Toggle: missing var name");
		return;
	}

	if (y_stricmp(EXEC_Param[0], "3d") == 0)
	{
		main_win->canvas->ToggleRenderMode();
	}
	else if (y_stricmp(EXEC_Param[0], "browser") == 0)
	{
		main_win->ShowBrowser('/');
	}
	else if (y_stricmp(EXEC_Param[0], "obj_nums") == 0)
	{
		edit.show_object_numbers = ! edit.show_object_numbers;
		edit.RedrawMap = 1;
	}
	else if (y_stricmp(EXEC_Param[0], "skills") == 0)
	{
		active_wmask ^= 1;
		active_when = active_wmask;
		edit.RedrawMap = 1;
	}
	else
	{
		Beep("Toggle: unknown var '%s'\n", EXEC_Param[0]);
	}
}


void CMD_BrowserMode(char kind)
{
	main_win->ShowBrowser(kind);
}


void CMD_CycleCategory(int dir)
{
	main_win->browser->CycleCategory(dir);
}

void CMD_ClearSearchBox(void)
{
	main_win->browser->ClearSearchBox();
}

void CMD_ScrollBrowser(int delta)
{
	main_win->browser->Scroll(delta);
}


void Editor_ScrollMap(int dx, int dy)
{
	grid.orig_x += dx;
	grid.orig_y += dy;

	edit.RedrawMap = 1;
}


void CMD_Scroll(void)
{
	// these are percentages
	int delta_x = atoi(EXEC_Param[0]);
	int delta_y = atoi(EXEC_Param[1]);

	if (delta_x == 0 && delta_y == 0)
	{
		Beep("Bad parameter to Scroll: '%s' %s'", EXEC_Param[0], EXEC_Param[1]);
		return;
	}

	delta_x = delta_x * main_win->canvas->w() / 100.0 / grid.Scale;
	delta_y = delta_y * main_win->canvas->h() / 100.0 / grid.Scale;

	Editor_ScrollMap(delta_x, delta_y);
}


void CMD_Merge(void)
{
	switch (edit.obj_type)
	{
		case OBJ_VERTICES:
			VERT_Merge();
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
	switch (edit.obj_type)
	{
		case OBJ_VERTICES:
			VERT_Disconnect();
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



bool Browser_Key(keycode_t key)
{
	// [\]: cycle through categories in the Browser
	if (key == '\\')
	{
		CMD_CycleCategory(+1);
	}

	// [CTRL-K]: clear the Browser search box
	else if (key == ('k' | MOD_COMMAND))
	{
		CMD_ClearSearchBox();
	}

	// [PGUP], [PGDN]: scroll the Browser
	else if (key == FL_Page_Up)
	{
		CMD_ScrollBrowser(-2);
	}
	else if (key == FL_Page_Down)
	{
		CMD_ScrollBrowser(+2);
	}

	else  // not a browser key
	{
		return false;
	}

	return true;
}


bool Global_Key(keycode_t key)
{
	// [T], [F] etc : open browser at specific kind
	if (key == 'T' || key == 'F' || key == 'O' || key == 'L' || key == 'S')
	{
		CMD_BrowserMode(key);
	}

	else
	{
		return false;
	}

	return true;
}


static bool Grid_Key(keycode_t key)
{
	keycode_t unshifted_key = key & ~MOD_SHIFT;

	// [+]: zooming in  (mouse wheel too)
	if (key == '+' || key == '=')
	{
		CMD_Zoom(+1, edit.map_x, edit.map_y);
	}

	// [-]: zooming out  (mouse wheel too)
	else if (key == '-' || key == '_')
	{
		CMD_Zoom(-1, edit.map_x, edit.map_y);
	}

	// [HOME], [0]: show the whole map in the window
	else if (key == FL_Home || key == '0')
	{
		CMD_ZoomWholeMap();
	}

	// [1] - [9]: set the grid size
	else if (unshifted_key >= '1' && unshifted_key <= '9')
	{
		int digit = unshifted_key - '0';

		bool do_zoom = digits_set_zoom;

		if (key & MOD_SHIFT)
			do_zoom = !do_zoom;

		if (do_zoom)
		{
			float S1 = grid.Scale;
			grid.ScaleFromDigit(digit);
			grid.RefocusZoom(edit.map_x, edit.map_y, S1);
		}
		else
		{
			grid.StepFromDigit(digit);
		}
	}

	// [G]: increase the grid step
	else if (key == 'G')
	{
		grid.AdjustStep(+1);
	}

	// [g]: decrease the grid step
	else if (key == 'g')
	{
		grid.AdjustStep(-1);
	}

	// [h]: display or hide the grid
	else if (key == 'h')
	{
		grid.ToggleShown();
	}

	//???  // [H]: reset the grid to grid_step_max
	//???  else if (key == 'H')
	//???  {
	//???    e.grid_step = e.grid_step_max;
	//???    edit.RedrawMap = 1;
	//???  }

	// [f]: toggle the snap_to_grid flag
	else if (key == 'f')
	{
		grid.ToggleSnap();

		UpdateHighlight();
	}

	else
	{
		// not a grid key
		return false;
	}

	return true;
}


void CMD_Zoom(int delta, int mid_x, int mid_y)
{
    float S1 = grid.Scale;

    grid.AdjustScale(delta);

    grid.RefocusZoom(mid_x, mid_y, S1);
}


void CMD_ZoomWholeMap(void)
{
	if (MadeChanges)
		CalculateLevelBounds();

	zoom_fit();

	edit.RedrawMap = 1;
}


void CMD_ZoomSelection(void)
{
	if (edit.Selected->empty())
	{
		Beep("Selection is empty.");
		return;
	}

	GoToSelection();

	edit.RedrawMap = 1;
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

	edit.RedrawMap = 1;
}


void CMD_PlaceCamera(void)
{
	if (main_win->canvas->isRenderActive())
	{
		Beep("not supported in 3D view");
		return;
	}

	if (! edit.pointer_in_window)
	{
		// IDEA: turn cursor into cross, wait for click in map window

		Beep("mouse is not over map");
		return;
	}

	int x = edit.map_x;
	int y = edit.map_y;

	Render3D_SetCameraPos(x, y);

	if (isalpha(EXEC_Param[0][0]))  // open3d
	{
		main_win->canvas->ChangeRenderMode(1);
	}

	edit.RedrawMap = 1;
}


void CMD_CopyAndPaste(void)
{
	if (! (edit.Selected || edit.highlighted()))
	{
		Beep("Nothing to copy and paste");
		return;
	}

	if (CMD_Copy())
	{
		CMD_Paste();
	}
}


void Editor_Wheel(int dx, int dy, keycode_t mod)
{
	if (mouse_wheel_scrolls_map && mod !=
#ifdef __APPLE__
		MOD_ALT)
#else
		MOD_COMMAND)
#endif
	{
		int speed = 12;  // FIXME: CONFIG OPTION

		if (mod == MOD_SHIFT)
			speed = MAX(1, speed / 3);

		Editor_ScrollMap(  dx * (double) speed / grid.Scale,
		                 - dy * (double) speed / grid.Scale);
	}
	else
	{
		dy = (dy > 0) ? +1 : -1;

		CMD_Zoom(- dy, edit.map_x, edit.map_y);
	}
}


static bool Thing_Key(keycode_t key)
{
	return false;
}


static bool LineDef_Key(keycode_t key)
{
	if (0)
	{
	}

	// [e]: Select/unselect all linedefs in non-forked path
	else if (key == 'e')
	{
		CMD_SelectLinesInPath(SLP_Normal);
	}
	else if (key == 'E')
	{
		CMD_SelectLinesInPath(SLP_SameTex);
	}


	else
	{
		return false;
	}

	return true;
}


static bool Sector_Key(keycode_t key)
{
	return false;
}


static bool Vertex_Key(keycode_t key)
{
	return false;
}


static bool RadTrig_Key(keycode_t key)
{
	return false;
}


bool Editor_Key(keycode_t key)
{
	keycode_t bare_key = key & FL_KEY_MASK;
	keycode_t unshifted_key = key & ~MOD_SHIFT;

	// [F10]: pop up the "Checks" menu
	if (key == FL_F+10)
	{
		CheckLevel (-1, -1);
		edit.RedrawMap = 1;
	}

	// [j]: jump to object by number
	else if (key == 'j')
	{
		CMD_JumpToObject();
	}

	// ???: find object by type
	// FIXME: CMD_FindObjectByType()

	// [Ctrl-x]: exchange objects numbers
//!!!	else if (key == 24)
//!!!	{
//!!!		if (! edit.Selected
//!!!				|| ! edit.Selected->next
//!!!				|| (edit.Selected->next)->next)
//!!!		{
//!!!			Beep ();
//!!!			Notify (-1, -1, "You must select exactly two objects", 0);
//!!!			edit.RedrawMap = 1;
//!!!		}
//!!!		else
//!!!		{
//!!!			exchange_objects_numbers (edit.obj_type, edit.Selected, true);
//!!!			edit.RedrawMap = 1;
//!!!		}
//!!!	}


	// [%] Show things sprites
	else if (key == '%')
	{
		edit.show_things_sprites = ! edit.show_things_sprites;
		edit.show_things_squares = ! edit.show_things_sprites;  // Not a typo !
		edit.RedrawMap = 1;
	}

	// [PRTSCR]: save a screen shot.   FIXME
	else if (key == FL_Print)
	{
		Beep();
	}

	else if (Grid_Key(key))
	{
		/* did grid stuff */
	}

	else
	{
		/* try a mode-specific function */

		switch (edit.obj_type)
		{
			case OBJ_THINGS:   return Thing_Key(key);
			case OBJ_LINEDEFS: return LineDef_Key(key);
			case OBJ_SECTORS:  return Sector_Key(key);
			case OBJ_VERTICES: return Vertex_Key(key);
			case OBJ_RADTRIGS: return RadTrig_Key(key);

			default:
				return false;
		}
	}

	return true;
}


void Editor_MousePress(keycode_t mod)
{
	if (edit.button_down >= 2)
		return;

	edit.button_down = 1;
	edit.button_mod  = mod;

	Objid object;      // The object under the pointer

	GetCurObject(object, edit.obj_type, edit.map_x, edit.map_y, grid.snap);

	edit.clicked = object;

	// clicking on an empty space starts a new selection box.

	if (object.is_nil())
	{
		main_win->canvas->SelboxBegin(edit.map_x, edit.map_y);
		return;
	}
}


void Editor_MouseRelease()
{
	edit.button_down = 0;

	Objid click_obj(edit.clicked);
	edit.clicked.clear();

	bool was_did_move = edit.did_a_move;
	edit.did_a_move = false;

	/* Releasing the button while dragging : drop the selection. */
	// FIXME : should call this automatically when switching tool
	if (main_win->canvas->isDragActive())
	{
		int dx, dy;
		main_win->canvas->DragFinish(&dx, &dy);

		if (! (dx==0 && dy==0))
		{
			CMD_MoveObjects(dx, dy);

			// next select action will clear the selection
			edit.did_a_move = true;
		}

		edit.drag_single_vertex = -1;
		edit.RedrawMap = 1;
		return;
	}

	// optional multi-select : require a certain modifier key
	if (multi_select_modifier &&
		edit.button_mod != (multi_select_modifier == 1 ? MOD_SHIFT : MOD_COMMAND))
	{
		was_did_move = true;
	}

	if (click_obj() && was_did_move)
	{
		edit.Selected->clear_all();
	}

	/* Releasing the button while there was a selection box
	   causes all the objects within the box to be selected.
	 */
	if (main_win->canvas->isSelboxActive())
	{
		int x1, y1, x2, y2;
		main_win->canvas->SelboxFinish(&x1, &y1, &x2, &y2);

		// a mere click and release will unselect everything
		if (x1 == x2 && y1 == y2)
			CMD_UnselectAll();
		else
			SelectObjectsInBox(edit.Selected, edit.obj_type, x1, y1, x2, y2);

		UpdateHighlight();

		edit.RedrawMap = 1;
		return;
	}


	if (! click_obj())
		return;

	Objid object;      // object under the pointer

	GetCurObject(object, edit.obj_type, edit.map_x, edit.map_y, grid.snap);

	/* select the object if unselected, and vice versa.
	 */
	if (object() && object.num == click_obj.num)
	{
		edit.Selected->toggle(object.num);

		edit.RedrawMap = 1;
		return;
	}
}


void Editor_MiddlePress(keycode_t mod)
{
	if (edit.button_down & 1)  // allow 0 or 2
		return;

	// ability to insert stuff via the mouse
	if (mod == 0)
	{
		EXEC_Param[0] = "";
		CMD_Insert();
		return;
	}

	if (edit.Selected->empty())
	{
		Beep();
		return;
	}

	edit.button_down = 2;
	edit.button_mod  = mod;

	int middle_x, middle_y;

	Objs_CalcMiddle(edit.Selected, &middle_x, &middle_y);

	main_win->canvas->ScaleBegin(edit.map_x, edit.map_y, middle_x, middle_y);
}


void Editor_MiddleRelease()
{
	edit.button_down = 0;

	if (main_win->canvas->isScaleActive())
	{
		scale_param_t param;

		main_win->canvas->ScaleFinish(param);

		CMD_ScaleObjects2(param);

		edit.RedrawMap = 1;
	}
}



void Editor_LeaveWindow()
{
	edit.pointer_in_window = false;
	
	UpdateHighlight();
}


void Editor_MouseMotion(int x, int y, keycode_t mod, int map_x, int map_y, bool drag)
{
	edit.map_x = map_x;
	edit.map_y = map_y;
	edit.pointer_in_window = true; // FIXME

	if (edit.pointer_in_window)
		main_win->info_bar->SetMouse(edit.map_x, edit.map_y);

	// fprintf(stderr, "MOUSE MOTION: %d,%d  map: %d,%d\n", x, y, edit.map_x, edit.map_y);

	if (edit.button_down == 2)
	{
		main_win->canvas->ScaleUpdate(edit.map_x, edit.map_y, mod);
		return;
	}

	if (! drag)
	{
		UpdateHighlight();
	}

	/* Moving the pointer with the left button pressed
	   and a selection box exists : move the second
	   corner of the selection box.
	*/
	else if (main_win->canvas->isSelboxActive())
	{
		main_win->canvas->SelboxUpdate(edit.map_x, edit.map_y);
		return;
	}

	/* Moving the pointer with the left button pressed
	   but no selection box exists and [Ctrl] was not
	   pressed when the button was pressed :
	   drag the selection.
	*/
	if (main_win->canvas->isDragActive())
	{
		main_win->canvas->DragUpdate(edit.map_x, edit.map_y);

		// if dragging a single vertex, update the possible split_line
		UpdateHighlight();
		return;
	}

	/*
	   begin dragging?
	   TODO: require pixel dist from click point to be >= THRESHHOLD
	 */
	if (edit.button_down == 1 && edit.clicked())
	{
		if (! edit.Selected->get(edit.clicked.num))
		{
			if (edit.did_a_move)
				edit.Selected->clear_all();

			edit.Selected->set(edit.clicked.num);
			edit.did_a_move = false;
		}

		int focus_x, focus_y;

		GetDragFocus(&focus_x, &focus_y, edit.map_x, edit.map_y);

		main_win->canvas->DragBegin(focus_x, focus_y, edit.map_x, edit.map_y);

		// check for a single vertex
		edit.drag_single_vertex = -1;

		if (edit.obj_type == OBJ_VERTICES && edit.Selected->find_second() < 0)
		{
			edit.drag_single_vertex = edit.Selected->find_first();
			SYS_ASSERT(edit.drag_single_vertex >= 0);
		}

		// forget the highlight
		edit.highlighted.clear();
		main_win->canvas->HighlightForget();
		return;
	}
}


void Editor_Resize(int is_width, int is_height)
{
	edit.RedrawMap = 1;
}


void Editor_RegisterCommands()
{
	/* global | interface stuff */

	M_RegisterCommand("Nothing", &CMD_Nothing);

	M_RegisterCommand("Quit", &CMD_Quit);
	M_RegisterCommand("EditMode", &CMD_EditMode);
// 	M_RegisterCommand("BrowserMode", &CMD_BrowserMode);

	M_RegisterCommand("Set",    &CMD_SetVar);
	M_RegisterCommand("Toggle", &CMD_ToggleVar);

	M_RegisterCommand("SelectAll", &CMD_SelectAll);
	M_RegisterCommand("UnselectAll", &CMD_UnselectAll);
	M_RegisterCommand("InvertSelection", &CMD_InvertSelection);

	M_RegisterCommand("Scroll", &CMD_Scroll);
	M_RegisterCommand("GoToCamera",  &CMD_GoToCamera);
	M_RegisterCommand("PlaceCamera", &CMD_PlaceCamera);

	/* global | map stuff */

	M_RegisterCommand("Insert", &CMD_Insert);
	M_RegisterCommand("Delete", &CMD_Delete);

	M_RegisterCommand("Mirror",   &CMD_Mirror);
	M_RegisterCommand("Rotate90", &CMD_Rotate90);
	M_RegisterCommand("Enlarge",  &CMD_Enlarge);
	M_RegisterCommand("Shrink",   &CMD_Shrink);

	M_RegisterCommand("Disconnect", &CMD_Disconnect);
	M_RegisterCommand("Merge", &CMD_Merge);
	M_RegisterCommand("Quantize", &CMD_Quantize);

	M_RegisterCommand("CopyAndPaste",   &CMD_CopyAndPaste);
	M_RegisterCommand("CopyProperties", &CMD_CopyProperties);

	/* line */

	M_RegisterCommand("LIN_Flip", &LIN_Flip, KCTX_Line);
	M_RegisterCommand("LIN_SplitHalf", &LIN_SplitHalf, KCTX_Line);
	M_RegisterCommand("LIN_AlignX", &LIN_AlignX, KCTX_Line);
	M_RegisterCommand("LIN_AlignY", &LIN_AlignY, KCTX_Line);

	/* sector */

	M_RegisterCommand("SEC_Floor", &SEC_Floor, KCTX_Sector);
	M_RegisterCommand("SEC_Ceil",  &SEC_Ceil,  KCTX_Sector);
	M_RegisterCommand("SEC_SelectGroup", &SEC_SelectGroup, KCTX_Sector);

	/* thing */

	M_RegisterCommand("TH_Spin", &TH_SpinThings, KCTX_Thing);

	/* vertex */

///---	M_RegisterCommand("VERT_Merge", &VERT_Merge, KCTX_Vertex);
}


void Editor_Init()
{
	memset(&edit, 0, sizeof(edit));  /* Catch-all */

	edit.move_speed = 20;
	edit.extra_zoom = 0;
	edit.obj_type   = OBJ_THINGS;

	edit.show_object_numbers = false;
	edit.show_things_squares = false;
	edit.show_things_sprites = true;

	edit.button_down = 0;
	edit.button_mod  = 0;
	edit.clicked.clear();
	edit.did_a_move  = false;

	edit.highlighted.clear();
	edit.split_line.clear();
	edit.drag_single_vertex = -1;

	edit.Selected = new selection_c(edit.obj_type);

	grid.Init();

	MadeChanges = 0;

	Editor_RegisterCommands();
/// TODO	Render_RegisterCommands();
}


bool Editor_ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "edit_mode") == 0 && num_tok >= 2)
	{
		Editor_ChangeMode(tokens[1][0]);
		return true;
	}

	if (strcmp(tokens[0], "render_mode") == 0)
	{
		if (main_win)
			main_win->canvas->ChangeRenderMode(1);
		return true;
	}

	if (strcmp(tokens[0], "show_object_numbers") == 0)
	{
		edit.show_object_numbers = 1;
		edit.RedrawMap = 1;
		return true;
	}

	return false;
}


void Editor_WriteUser(FILE *fp)
{
	switch (edit.obj_type)
	{
		case OBJ_THINGS:   fprintf(fp, "edit_mode t\n"); break;
		case OBJ_LINEDEFS: fprintf(fp, "edit_mode l\n"); break;
		case OBJ_SECTORS:  fprintf(fp, "edit_mode s\n"); break;
		case OBJ_VERTICES: fprintf(fp, "edit_mode v\n"); break;
		case OBJ_RADTRIGS: fprintf(fp, "edit_mode r\n"); break;

		default: break;
	}

	if (main_win && main_win->canvas->isRenderActive())
		fprintf(fp, "render_mode 1\n");

	if (edit.show_object_numbers)
		fprintf(fp, "show_object_numbers 1\n");
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
