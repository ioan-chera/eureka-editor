//------------------------------------------------------------------------
//  COMMAND FUNCTIONS
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
#include "e_cutpaste.h"
#include "e_hover.h"
#include "r_grid.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_objects.h"
#include "e_sector.h"
#include "e_path.h"
#include "e_vertex.h"
#include "m_loadsave.h"
#include "r_render.h"
#include "ui_window.h"
#include "ui_misc.h"


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


void CMD_Select(void)
{
	if (edit.render3d)
		return;

	// FIXME : action in effect?

	// FIXME : split_line in effect?

	if (edit.highlight.is_nil())
	{
		Beep("Nothing under cursor");
		return;
	}

	int obj_num = edit.highlight.num;

	edit.Selected->toggle(obj_num);

	RedrawMap();
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
		edit.action == ACT_TRANSFORM)
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
	Main_Quit();
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
//FIXME!!!!
//		active_wmask ^= 1;
//		active_when = active_wmask;
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
		Beep("BrowserMode: missing mode");
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
	Editor_UpdateFromScroll();
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

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_X_release, MOD_COMMAND | MOD_SHIFT);
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

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Y_release, MOD_COMMAND | MOD_SHIFT);
}


static void NAV_MouseScroll_release(void)
{
	Editor_ScrollMap(+1);
}

void CMD_NAV_MouseScroll(void)
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	if (Nav_SetKey(EXEC_CurKey, &NAV_MouseScroll_release))
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
		ExecuteCommand("UnselectAll");
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

	if (dx || dy)
	{
		if (edit.drag_single_obj >= 0)
			DragSingleObject(edit.drag_single_obj, dx, dy);
		else
			MoveObjects(edit.Selected, dx, dy);
	}

	edit.drag_single_obj = -1;

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

		// check for a single vertex    FIXME FIXME
		edit.drag_single_obj = -1;

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


static void ACT_Transform_release(void)
{
	// check if cancelled or overridden
	if (edit.action != ACT_TRANSFORM)
		return;

	Editor_ClearAction();

	transform_t param;

	main_win->canvas->TransformFinish(param);

	TransformObjects(param);

	RedrawMap();
}

void CMD_ACT_Transform(void)
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

	const char *keyword = EXEC_Param[0];
	transform_keyword_e  mode;

	if (! keyword[0])
	{
		Beep("ACT_Transform: missing keyword");
		return;
	}
	else if (y_stricmp(keyword, "scale") == 0)
	{
		mode = TRANS_K_Scale;
	}
	else if (y_stricmp(keyword, "stretch") == 0)
	{
		mode = TRANS_K_Stretch;
	}
	else if (y_stricmp(keyword, "rotate") == 0)
	{
		mode = TRANS_K_Rotate;
	}
	else if (y_stricmp(keyword, "rotscale") == 0)
	{
		mode = TRANS_K_RotScale;
	}
	else if (y_stricmp(keyword, "skew") == 0)
	{
		mode = TRANS_K_Skew;
	}
	else
	{
		Beep("ACT_Transform: unknown keyword: %s", keyword);
		return;
	}


	if (! Nav_ActionKey(EXEC_CurKey, &ACT_Transform_release))
		return;


	int middle_x, middle_y;

	Objs_CalcMiddle(edit.Selected, &middle_x, &middle_y);

	main_win->canvas->TransformBegin(edit.map_x, edit.map_y, middle_x, middle_y, mode);

	Editor_SetAction(ACT_TRANSFORM);
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
			CMD_VT_Merge();
			break;

		case OBJ_LINEDEFS:
			CMD_LIN_MergeTwo();
			break;

		case OBJ_SECTORS:
			CMD_SEC_Merge();
			break;

		case OBJ_THINGS:
			CMD_TH_Merge();
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
			CMD_VT_Disconnect();
			break;

		case OBJ_LINEDEFS:
			CMD_LIN_Disconnect();
			break;

		case OBJ_SECTORS:
			CMD_SEC_Disconnect();
			break;

		case OBJ_THINGS:
			CMD_TH_Disconnect();
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
		Beep("Zoom: bad or missing value");
		return;
	}

	Editor_Zoom(delta, edit.map_x, edit.map_y);
}


void CMD_ZoomWholeMap(void)
{
	ZoomWholeMap();
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


void CMD_GRID_Bump(void)
{
	int delta = atoi(EXEC_Param[0]);

	delta = (delta >= 0) ? +1 : -1;

	grid.AdjustStep(delta);
}


void CMD_GRID_Set(void)
{
	int step = atoi(EXEC_Param[0]);

	if (step < 2 || step > 4096)
	{
		Beep("Bad grid step");
		return;
	}

	grid.SetStep(step);
}


void CMD_GRID_Zoom(void)
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


void CMD_BR_CycleCategory(void)
{
	if (! main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	int dir = (atoi(EXEC_Param[0]) >= 0) ? +1 : -1;

	main_win->browser->CycleCategory(dir);
}

void CMD_BR_ClearSearch(void)
{
	if (! main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	main_win->browser->ClearSearchBox();
}


void CMD_BR_Scroll(void)
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
	/* ----- FILE menu ----- */

	{	"Quit",  "File", 0,
		&CMD_Quit
	},

	{	"GivenFile",  "File", 0,
		&CMD_GivenFile,
		/* flags */ NULL,
		/* keywords */ "next prev first last current"
	},

	{	"FlipMap",  "File", 0,
		&CMD_FlipMap,
		/* flags */ NULL,
		/* keywords */ "next prev first last"
	},


	/* ----- EDIT menu ----- */

	{	"Insert",	"Edit", 0,
		&CMD_Insert,
		/* flags */ "/new /continue /nofill"
	},

	{	"Delete",	"Edit", 0,
		&CMD_Delete,
		/* flags */ "/keep_things /keep_unused"
	},

	{	"Undo",   "Edit", 0,
		&CMD_Undo
	},

	{	"Redo",   "Edit", 0,
		&CMD_Redo
	},

	{	"Clipboard_Cut",   "Edit", 0,
		&CMD_Clipboard_Cut
	},

	{	"Clipboard_Copy",   "Edit", 0,
		&CMD_Clipboard_Copy
	},

	{	"Clipboard_Paste",   "Edit", 0,
		&CMD_Clipboard_Paste
	},

	{	"Select",	"Edit", 0,
		&CMD_Select
	},

	{	"SelectAll",	"Edit", 0,
		&CMD_SelectAll
	},

	{	"UnselectAll",	"Edit", 0,
		&CMD_UnselectAll
	},

	{	"InvertSelection",	"Edit", 0,
		&CMD_InvertSelection
	},

	{	"LastSelection",	"Edit", 0,
		&CMD_LastSelection
	},

	{	"CopyAndPaste",   "Edit", 0,
		&CMD_CopyAndPaste
	},

	{	"CopyProperties",   "Edit", 0,
		&CMD_CopyProperties,
		/* flags */ "/reverse"
	},

	{	"PruneUnused",   "Edit", 0,
		&CMD_PruneUnused
	},

	{	"MoveObjectsDialog",   "Edit", 0,
		&CMD_MoveObjects_Dialog
	},

	{	"ScaleObjectsDialog",   "Edit", 0,
		&CMD_ScaleObjects_Dialog
	},

	{	"RotateObjectsDialog",   "Edit", 0,
		&CMD_RotateObjects_Dialog
	},


	/* ----- VIEW menu ----- */

	{	"GoToCamera",  "View", 0,
		&CMD_GoToCamera
	},

	{	"PlaceCamera",  "View", 0,
		&CMD_PlaceCamera,
		/* flags */ "/open3d"
	},

	{	"Zoom",  "View", 0,
		&CMD_Zoom
	},

	{	"ZoomWholeMap",  "View", 0,
		&CMD_ZoomWholeMap
	},

	{	"ZoomSelection",  "View", 0,
		&CMD_ZoomSelection
	},

	{	"JumpToObject",  "View", 0,
		&CMD_JumpToObject
	},


	/* ------ interface stuff ------ */

	{	"EditMode", "UI", 0,
		&CMD_EditMode,
		/* flags */ NULL,
		/* keywords */ "thing line sector vertex"
	},

	{	"MetaKey", "UI", 0,
		&CMD_MetaKey
	},

	{	"MapCheck", "UI", 0,
		&CMD_MapCheck,
		/* flags */ NULL,
		/* keywords */ "all major vertices sectors linedefs things textures tags current"
	},

	{	"Set", "UI", 0,
		&CMD_SetVar,
		/* flags */ NULL,
		/* keywords */ "3d browser grid obj_nums snap"
	},

	{	"Toggle", "UI", 0,
		&CMD_ToggleVar,
		/* flags */ NULL,
		/* keywords */ "3d browser grid obj_nums snap recent"
	},

	{	"Scroll",  "UI", 0,
		&CMD_Scroll
	},

	{	"GRID_Bump",  "UI", 0,
		&CMD_GRID_Bump
	},

	{	"GRID_Set",  "UI", 0,
		&CMD_GRID_Set
	},

	{	"GRID_Zoom",  "UI", 0,
		&CMD_GRID_Zoom
	},

	{	"WHEEL_Scroll",  "UI", 0,
		&CMD_WHEEL_Scroll
	},

	{	"NAV_Scroll_X",  "UI",  MOD_SHIFT | MOD_COMMAND,
		&CMD_NAV_Scroll_X
	},

	{	"NAV_Scroll_Y",  "UI",  MOD_SHIFT | MOD_COMMAND,
		&CMD_NAV_Scroll_Y
	},

	{	"NAV_MouseScroll", "UI", 0,
		&CMD_NAV_MouseScroll
	},


	/* ----- general operations ----- */

	{	"Nothing", "General", 0,
		&CMD_Nothing
	},

	{	"Mirror",	"General", 0,
		&CMD_Mirror,
		/* flags */ NULL,
		/* keywords */ "horiz vert"
	},

	{	"Rotate90",	"General", 0,
		&CMD_Rotate90,
		/* flags */ NULL,
		/* keywords */ "cw acw"
	},

	{	"Enlarge",	"General", 0,
		&CMD_Enlarge
	},

	{	"Shrink",	"General", 0,
		&CMD_Shrink
	},

	{	"Disconnect",	"General", 0,
		&CMD_Disconnect
	},

	{	"Merge",	"General", 0,
		&CMD_Merge,
		/* flags */ "/keep"
	},

	{	"Quantize",	"General", 0,
		&CMD_Quantize
	},

	{	"Gamma",	"General", 0,
		&CMD_Gamma
	},

	{	"ApplyTag",	"General", 0,
		&CMD_ApplyTag,
		/* flags */ NULL,
		/* keywords */ "fresh last"
	},

	{	"ACT_SelectBox", "General", 0,
		&CMD_ACT_SelectBox
	},

	{	"ACT_Drag", "General", 0,
		&CMD_ACT_Drag
	},

	{	"ACT_Transform", "General", 0,
		&CMD_ACT_Transform,
		/* flags */ NULL,
		/* keywords */ "scale stretch rotate rotscale skew"
	},


	/* -------- linedef -------- */

	{	"LIN_Flip", NULL, 0,
		&CMD_LIN_Flip,
		/* flags */ "/verts /sides"
	},

	{	"LIN_SplitHalf", NULL, 0,
		&CMD_LIN_SplitHalf
	},

	{	"LIN_SelectPath", NULL, 0,
		&CMD_LIN_SelectPath,
		/* flags */ "/add /onesided /sametex"
	},


	/* -------- sector -------- */

	{	"SEC_Floor", NULL, 0,
		&CMD_SEC_Floor
	},

	{	"SEC_Ceil", NULL, 0,
		&CMD_SEC_Ceil
	},

	{	"SEC_Light", NULL, 0,
		&CMD_SEC_Light
	},

	{	"SEC_SelectGroup", NULL, 0,
		&CMD_SEC_SelectGroup,
		/* flags */ "/add /can_walk /doors /floor_h /floor_tex /ceil_h /ceil_tex /light /tag /special"
	},

	{	"SEC_SwapFlats", NULL, 0,
		&CMD_SEC_SwapFlats
	},


	/* -------- thing -------- */

	{	"TH_Spin", NULL, 0,
		&CMD_TH_SpinThings
	},


	/* -------- vertex -------- */

	{	"VT_ShapeLine", NULL, 0,
		&CMD_VT_ShapeLine
	},

	{	"VT_ShapeArc", NULL, 0,
		&CMD_VT_ShapeArc
	},


	/* -------- browser -------- */

 	{	"BrowserMode", "Browser", 0,
		&CMD_BrowserMode,
		/* flags */ NULL,
		/* keywords */ "obj tex flat line sec genline"
	},

	{	"BR_CycleCategory", "Browser", 0,
		&CMD_BR_CycleCategory
	},

	{	"BR_ClearSearch", "Browser", 0,
		&CMD_BR_ClearSearch
	},

	{	"BR_Scroll", "Browser", 0,
		&CMD_BR_Scroll
	},

	// end of command list
	{	NULL, NULL, 0, NULL  }
};


void Editor_RegisterCommands()
{
	M_RegisterCommandList(command_table);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
