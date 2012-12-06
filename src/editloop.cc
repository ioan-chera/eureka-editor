//------------------------------------------------------------------------
//  EDIT LOOP
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
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
#include "s_misc.h"
#include "selectn.h"
#include "x_mirror.h"
#include "x_hover.h"
#include "r_render.h"
#include "ui_window.h"


Editor_State_c  edit;


int active_when = 0;  // MOVE THESE
int active_wmask = 0;



// config items
bool escape_key_quits = false;
bool mouse_wheel_scrolls_map = false;
bool same_mode_clears_selection = false; 

int multi_select_modifier = KM_none;


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
static int zoom_fit()
{
	if (NumVertices == 0)
		return 0;

	double xzoom = 1;
	double yzoom = 1;

	if (MapBound_lx < MapBound_hx)
		xzoom = ScrMaxX / (double)(MapBound_hx - MapBound_lx);

	if (MapBound_ly < MapBound_hy)
		yzoom = ScrMaxY / (double)(MapBound_hy - MapBound_ly);

	grid.NearestScale(MIN(xzoom, yzoom));

	grid.CenterMapAt((MapBound_lx + MapBound_hx) / 2, (MapBound_ly + MapBound_hy) / 2);
	return 0;
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


void CMD_ChangeEditMode(char mode)
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


void CMD_SelectAll()
{
	int total = NumObjects(edit.obj_type);

	edit.Selected->change_type(edit.obj_type);
	edit.Selected->frob_range(0, total-1, BOP_ADD);
	edit.RedrawMap = 1;

	UpdateHighlight();
}


void CMD_UnselectAll()
{
	edit.Selected->change_type(edit.obj_type);
	edit.Selected->clear_all();
	edit.RedrawMap = 1;

	UpdateHighlight();
}


void CMD_InvertSelection()
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


void CMD_Quit()
{
	want_quit = true;
}


void CMD_Toggle3Dview()
{
	main_win->canvas->ToggleRenderMode();
}

void CMD_ToggleBrowser()
{
	main_win->ShowBrowser('/');
}

void CMD_SetBrowser(char kind)
{
	main_win->ShowBrowser(kind);
}


void CMD_CycleCategory(int dir)
{
	main_win->browser->CycleCategory(dir);
}

void CMD_ClearSearchBox()
{
	main_win->browser->ClearSearchBox();
}

void CMD_ScrollBrowser(int delta)
{
	main_win->browser->Scroll(delta);
}



bool Browser_Key(int key, keymod_e mod)
{
	// [C]: cycle through categories in the Browser
	if (key == 'C')
	{
		CMD_CycleCategory(+1);
	}

	// [CTRL-K]: clear the Browser search box
	else if (key == 11)
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


bool Global_Key(int key, keymod_e mod)
{
	// [ESC]: quit
	if (key == FL_Escape && escape_key_quits)
	{
		CMD_Quit();
	}

	// [CTRL-L]: force redraw
	else if (key == '\f')
	{
		edit.RedrawMap = 1;

		main_win->redraw();
	}

	// [TAB]: toggle the 3D view on/off
	else if (key == FL_Tab)
	{
		CMD_Toggle3Dview();
	}

	// [b]: toggle the Browser panel on/off
	else if (key == 'b')
	{
		CMD_ToggleBrowser();
	}

	// [T], [F] etc : open browser at specific kind
	else if (key == 'T' || key == 'F' || key == 'O' || key == 'L' || key == 'S')
	{
		CMD_SetBrowser(key);
	}

	else
	{
		return false;
	}

	return true;
}


static bool Grid_Key(int key, keymod_e mod)
{
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

	// [END]: go to camera position
	else if (key == FL_End)
	{
		CMD_GoToCamera();
	}

	// [END]: jump to camera position

	// TODO: CONFIG ITEM : digits set the zoom factor (as in Yadex)
#if 0
	// [1] - [9]: set the zoom factor
	else if (key >= '1' && key <= '9' && mod == KM_SHIFT)
	{
		float S1 = grid.Scale;

		grid.ScaleFromDigit(key - '0');
		grid.RefocusZoom(edit.map_x, edit.map_y, S1);
	}
#endif

	// [1] - [9]: set the grid size
	else if (key >= '1' && key <= '9')
	{
		grid.StepFromDigit(key - '0');
	}

	// [Left], [Right], [Up], [Down]:
	// scroll <scroll_less> percents of a screenful.
	else if (key == FL_Left && grid.orig_x > -30000)
	{
		grid.orig_x -= (int) ((double) ScrMaxX * scroll_less / 100 / grid.Scale);
		edit.RedrawMap = 1;
	}
	else if (key == FL_Right && grid.orig_x < 30000)
	{
		grid.orig_x += (int) ((double) ScrMaxX * scroll_less / 100 / grid.Scale);
		edit.RedrawMap = 1;
	}
	else if (key == FL_Up && grid.orig_y < 30000)
	{
		grid.orig_y += (int) ((double) ScrMaxY * scroll_less / 100 / grid.Scale);
		edit.RedrawMap = 1;
	}
	else if (key == FL_Down && grid.orig_y > -30000)
	{
		grid.orig_y -= (int) ((double) ScrMaxY * scroll_less / 100 / grid.Scale);
		edit.RedrawMap = 1;
	}

#if 0
	// [PGUP], [PGDN]
	// scroll <scroll_more> percents of a screenful.
	else if (key == FL_Page_Up && (grid.orig_y) < /*MapBound_hy*/ 20000)
	{
		grid.orig_y += (int) ((double) ScrMaxY * scroll_more / 100 / grid.Scale);
		edit.RedrawMap = 1;
	}
	else if (key == FL_Page_Down && (grid.orig_y) > /*MapBound_ly*/ -20000)
	{
		grid.orig_y -= (int) ((double) ScrMaxY * scroll_more / 100 / grid.Scale);
		edit.RedrawMap = 1;
	}
#endif

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

///---	// [P]: toggle the alternate grid mode
///---	else if (key == 'P')
///---	{
///---		main_win->canvas->ToggleAltGrid();
///---	}

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


void CMD_ScrollMap(int dx, int dy)
{
	grid.orig_x += dx;
	grid.orig_y += dy;

	edit.RedrawMap = 1;
}


void CMD_Zoom(int delta, int mid_x, int mid_y)
{
    float S1 = grid.Scale;

    grid.AdjustScale(delta);

    grid.RefocusZoom(mid_x, mid_y, S1);
}


void CMD_ZoomWholeMap()
{
	if (MadeChanges)
		CalculateLevelBounds();

	zoom_fit();

	edit.RedrawMap = 1;
}


void CMD_GoToCamera()
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


void Editor_Wheel(int dx, int dy, keymod_e mod)
{
	if (mouse_wheel_scrolls_map && mod !=
#ifdef __APPLE__
		KM_ALT)
#else
		KM_CTRL)
#endif
	{
		int speed = 12;  // FIXME: CONFIG OPTION

		if (mod == KM_SHIFT)
			speed = MAX(1, speed / 3);

		CMD_ScrollMap(  dx * (double) speed / grid.Scale,
		              - dy * (double) speed / grid.Scale);
	}
	else
	{
		dy = (dy > 0) ? +1 : -1;

		CMD_Zoom(- dy, edit.map_x, edit.map_y);
	}
}


bool Editor_Key(int key, keymod_e mod)
{
	// in general, ignore ALT key
	if (mod == KM_ALT)
		return false;

#if 0
	// No (do it via menu) --> [F5]: pop up the "Preferences" menu

	// Ditto: [M]: pop up the "Misc. operations" menu
	if (key == 9876)
	{
		///---    edit.modpopup->set (edit.menubar->get_menu (MBI_MISC), 1);
	}

	// Ditto: [F9]: pop up the "Insert a standard object" menu
	else if (key == FL_F+9)
	{
		///---    edit.modpopup->set (edit.menubar->get_menu (MBI_OBJECTS), 1);
	}
#endif

	// [l], [s], [t], [v], [r]: switch mode
	if (key == 't' || key == 'v' || key == 'l' ||
		key == 's' || key == 'R')
	{
		CMD_ChangeEditMode(key);
	}

	// [F10]: pop up the "Checks" menu
	else if (key == FL_F+10)
	{
		CheckLevel (-1, -1);
		edit.RedrawMap = 1;
	}

	// [e]: Select/unselect all linedefs in non-forked path
	else if ((key == 'e') && edit.obj_type == OBJ_LINEDEFS)
	{
		CMD_SelectLinesInPath(SLP_Normal);
	}
	else if (key == 'E' && edit.obj_type == OBJ_LINEDEFS)
	{
		CMD_SelectLinesInPath(SLP_SameTex);
	}

	// [e]: Select/unselect contiguous sectors with same floor height
	else if (key == 'e' && edit.obj_type == OBJ_SECTORS)
	{
		CMD_SelectContiguousSectors(SCS_Floor_H);
	}
	else if (key == 'E' && edit.obj_type == OBJ_SECTORS)
	{
		CMD_SelectContiguousSectors(SCS_FloorTex);
	}

	// [q]: quantize objects to the grid
	else if (key == 'q')
	{
		CMD_QuantizeObjects();
	}

	// [j]: jump to object by number
	else if (key == 'j')
	{
		CMD_JumpToObject();
	}

	// [n]: highlight the next object
	else if (key == 'n')
	{
		CMD_NextObject();
	}

	// [p]: highlight the previous object
	else if (key == 'p' )
	{
		CMD_PrevObject();
	}

	// [f]: find object by type
	// FIXME: CMD_FindObjectByType()

	// [`]: clear selection and redraw the map (CTRL-U too, done via menu)
	else if (key == '`')
	{
		CMD_UnselectAll();
	}

	// [']: move camera to spot under cursor
	else if (key == '\'')
	{
		// IDEA: activate 3D mode (CONFIG ITEM)

		if (edit.pointer_in_window)
		{
			Render3D_SetCameraPos(edit.map_x, edit.map_y);

			edit.RedrawMap = 1;
		}
	}

	// [o]: copy a group of objects
	else if (key == 'o'
			&& (edit.Selected || edit.highlighted ()))
	{
		if (CMD_Copy())
		{
			CMD_Paste();
		}
	}

	else if (key == 'W')
	{
		CMD_RotateObjects(true);
	}
	else if (key == 'R')
	{
		CMD_RotateObjects(false);
	}

	else if (key == 'H')
	{
		CMD_MirrorObjects(false);
	}
	else if (key == 'V')
	{
		CMD_MirrorObjects(true);
	}

/*
	else if (key == 'p')
	{
		CMD_ScaleObjects(true);
	}
	else if (key == 'P')
	{
		CMD_ScaleObjects(false);
	}
*/

	// 'w': spin things 1/8 turn counter-clockwise
	else if (key == 'w' && edit.obj_type == OBJ_THINGS
			&& (edit.Selected || edit.highlighted ()))
	{
		CMD_SpinThings(+45);
	}

	// 'x': spin things 1/8 turn clockwise
	else if (key == 'x' && edit.obj_type == OBJ_THINGS)
	{
		CMD_SpinThings(-45);
	}

	// [.] and [,]: adjust floor height
	else if ((key == ',' || key == '<') && edit.obj_type == OBJ_SECTORS)
	{
		CMD_MoveFloors((mod == KM_CTRL) ? -64 : (mod == KM_SHIFT) ? -1 : -8);
	}
	else if ((key == '.' || key == '>') && edit.obj_type == OBJ_SECTORS)
	{
		CMD_MoveFloors((mod == KM_CTRL) ? +64 : (mod == KM_SHIFT) ? +1 : +8);
	}

	// '[' and ']': adjust ceiling height
	else if ((key == '[' || key == '{') && edit.obj_type == OBJ_SECTORS)
	{
		CMD_MoveCeilings((mod == KM_CTRL) ? -64 : (mod == KM_SHIFT) ? -1 : -8);
	}
	else if ((key == ']' || key == '}') && edit.obj_type == OBJ_SECTORS)
	{
		CMD_MoveCeilings((mod == KM_CTRL) ? +64 : (mod == KM_SHIFT) ? +1 : +8);
	}


	// [w]: split linedefs and sectors
//!!!	else if (key == 'w' && edit.obj_type == OBJ_LINEDEFS
//!!!			&& edit.Selected && edit.Selected->next && ! edit.Selected->next->next)
//!!!	{
//!!!		SplitLineDefsAndSector (edit.Selected->next->objnum, edit.Selected->objnum);
//!!!		edit.Selected->clear_all();
//!!!		edit.RedrawMap = 1;
//!!!	}

//!!!	// [w]: split sector between vertices
//!!!	else if (key == 'w' && edit.obj_type == OBJ_VERTICES
//!!!			&& edit.Selected && edit.Selected->next && ! edit.Selected->next->next)
//!!!	{
//!!!		SplitSector (edit.Selected->next->objnum, edit.Selected->objnum);
//!!!		edit.Selected->clear_all();
//!!!		edit.RedrawMap = 1;
//!!!	}


	// [x]: split linedefs
	else if (key == 'x' && edit.obj_type == OBJ_LINEDEFS)
	{
		CMD_SplitLineDefs();
	}

	// [w]: flip linedefs
	else if (key == 'w' && edit.obj_type == OBJ_LINEDEFS)
	{
		CMD_FlipLineDefs();
	}

	// [w]: swap flats in sectors
	else if (key == 'w' && edit.obj_type == OBJ_SECTORS)
	{
		CMD_SwapFlats();
	}

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

	// [Ctrl][k]: cut a slice out of a sector
//!!!	else if (key == 11 && edit.obj_type == OBJ_LINEDEFS
//!!!			&& edit.Selected && edit.Selected->next && ! edit.Selected->next->next)
//!!!	{
//!!!		sector_slice (edit.Selected->next->objnum, edit.Selected->objnum);
//!!!		edit.Selected->clear_all();
//!!!		edit.RedrawMap = 1;
//!!!	}

	// [DEL]: delete the current object(s)
	else if ((key == '\b' || key == FL_BackSpace || key == FL_Delete))
	{
		bool keep_unused = (mod == KM_SHIFT) ? true : false;

		CMD_Delete(keep_unused, keep_unused);
	}

	// [INS], [SPACE]: insert a new object
	else if (key == ' ' || key == FL_Insert)
	{
		CMD_InsertNewObject(mod);

		UpdateHighlight();

		edit.RedrawMap = 1;
	}

/*
	// [c]: correct sector at mouse pointer
	else if (key == 'c' && edit.obj_type == OBJ_SECTORS &&
	         edit.pointer_in_window)
	{
		CMD_CorrectSector();
	}
*/
	// [m]: merge sectors  (with SHIFT : keep common linedefs)
	else if (key == 'm' && edit.obj_type == OBJ_SECTORS)
	{
		CMD_MergeSectors(false);
	}
	else if (key == 'M' && edit.obj_type == OBJ_SECTORS)
	{
		CMD_MergeSectors(true);
	}

	// [d]: disconnect linedefs
	else if (key == 'd' && edit.obj_type == OBJ_VERTICES)
	{
		CMD_DisconnectVertices();
	}
	else if (key == 'd' && edit.obj_type == OBJ_LINEDEFS)
	{
		CMD_DisconnectLineDefs();
	}

	// [X]: align textures horizontally
	else if (key == 'X' and edit.obj_type == OBJ_LINEDEFS)
	{
		CMD_AlignTexturesX();
	}
	// [Y]: align textures vertically
	else if (key == 'Y' and edit.obj_type == OBJ_LINEDEFS)
	{
		CMD_AlignTexturesY();
	}

	// [K] limit shown things to specific skill (AJA)
	else if (key == 'K' && edit.obj_type == OBJ_THINGS)
	{
		active_wmask ^= 1;
		active_when = active_wmask;
		edit.RedrawMap = 1;
	}

	// [c] Copy properties to the highlighted object
	else if (key == 'c' && edit.Selected->notempty() &&
			 edit.highlighted())
	{
		CMD_CopyProperties();

	}

	// [&] Show object numbers
	else if (key == '&')
	{
		edit.show_object_numbers = ! edit.show_object_numbers;
		edit.RedrawMap = 1;
	}

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

	else if (Grid_Key(key, mod))
	{
		/* did grid stuff */
	}

	else
	{
		return false;
	}

	return true;
}


void EditorMousePress(keymod_e mod)
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


void EditorMouseRelease()
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
	if (multi_select_modifier != KM_none &&
		edit.button_mod != multi_select_modifier)
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


void EditorMiddlePress(keymod_e mod)
{
	if (edit.button_down & 1)  // allow 0 or 2
		return;

	// ability to insert stuff via the mouse
	if (mod == KM_none)
	{
		Editor_Key(' ', mod);
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


void EditorMiddleRelease()
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



void EditorLeaveWindow()
{
	edit.pointer_in_window = false;
	
	UpdateHighlight();
}


void EditorMouseMotion(int x, int y, keymod_e mod, int map_x, int map_y, bool drag)
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


void EditorResize(int is_width, int is_height)
{
	edit.RedrawMap = 1;
}


/*
  the editor main loop
*/

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
	edit.button_mod  = KM_none;
	edit.clicked.clear();
	edit.did_a_move  = false;

	edit.highlighted.clear();
	edit.split_line.clear();
	edit.drag_single_vertex = -1;

	edit.Selected = new selection_c(edit.obj_type);

	grid.Init();

	MadeChanges = 0;
}


bool Editor_ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "edit_mode") == 0 && num_tok >= 2)
	{
		CMD_ChangeEditMode(tokens[1][0]);
		return true;
	}

	if (strcmp(tokens[0], "render_mode") == 0)
	{
		if (main_win)
			main_win->canvas->ChangeRenderMode(1);
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
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
