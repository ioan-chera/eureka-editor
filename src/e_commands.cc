//------------------------------------------------------------------------
//  COMMAND FUNCTIONS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2018 Andrew Apted
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
#include "m_config.h"
#include "m_editlump.h"
#include "m_events.h"
#include "m_loadsave.h"
#include "m_nodes.h"
#include "r_render.h"
#include "ui_window.h"
#include "ui_about.h"
#include "ui_misc.h"
#include "ui_prefs.h"


// config items
int minimum_drag_pixels = 5;


void CMD_Nothing()
{
	/* hey jude, don't make it bad */
}


void CMD_MetaKey()
{
	if (edit.sticky_mod)
	{
		ClearStickyMod();
		return;
	}

	Status_Set("META...");

	edit.sticky_mod = MOD_META;
}


void CMD_EditMode()
{
	char mode = tolower(EXEC_Param[0][0]);

	if (! mode || ! strchr("lstvr", mode))
	{
		Beep("Bad parameter for EditMode: '%s'", EXEC_Param[0]);
		return;
	}

	Editor_ChangeMode(mode);
}


void CMD_Select()
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

	Selection_Toggle(edit.highlight);
	RedrawMap();
}


void CMD_SelectAll()
{
	Editor_ClearErrorMode();

	int total = NumObjects(edit.mode);

	Selection_Push();

	edit.Selected->change_type(edit.mode);
	edit.Selected->frob_range(0, total-1, BOP_ADD);

	RedrawMap();
}


void CMD_UnselectAll()
{
	Editor_ClearErrorMode();

	if (edit.action == ACT_DRAW_LINE ||
		edit.action == ACT_TRANSFORM)
	{
		Editor_ClearAction();
	}

	Selection_Clear();

	RedrawMap();
}


void CMD_InvertSelection()
{
	// do not clear selection when in error mode
	edit.error_mode = false;

	int total = NumObjects(edit.mode);

	if (edit.Selected->what_type() != edit.mode)
	{
		// convert the selection
		selection_c *prev_sel = edit.Selected;
		edit.Selected = new selection_c(edit.mode, true /* extended */);

		ConvertSelection(prev_sel, edit.Selected);
		delete prev_sel;
	}

	edit.Selected->frob_range(0, total-1, BOP_TOGGLE);

	RedrawMap();
}


void CMD_Quit()
{
	Main_Quit();
}


void CMD_Undo()
{
	if (! BA_Undo())
	{
		Beep("No operation to undo");
		return;
	}

	RedrawMap();
	main_win->UpdatePanelObj();
}


void CMD_Redo()
{
	if (! BA_Redo())
	{
		Beep("No operation to redo");
		return;
	}

	RedrawMap();
	main_win->UpdatePanelObj();
}


static void SetGamma(int new_val)
{
	usegamma = CLAMP(0, new_val, 4);

	W_UpdateGamma();

	// for OpenGL, need to reload all images
	if (main_win && main_win->canvas)
		main_win->canvas->DeleteContext();

	Status_Set("gamma level %d", usegamma);

	RedrawMap();
}


void CMD_SetVar()
{
	const char *var_name = EXEC_Param[0];
	const char *value    = EXEC_Param[1];

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
			main_win->BrowserMode('/');
	}
	else if (y_stricmp(var_name, "grid") == 0)
	{
		grid.SetShown(bool_val);
	}
	else if (y_stricmp(var_name, "snap") == 0)
	{
		grid.SetSnap(bool_val);
	}
	else if (y_stricmp(var_name, "sprites") == 0)
	{
		edit.thing_render_mode = int_val;
		RedrawMap();
	}
	else if (y_stricmp(var_name, "obj_nums") == 0)
	{
		edit.show_object_numbers = bool_val;
		RedrawMap();
	}
	else if (y_stricmp(var_name, "gamma") == 0)
	{
		SetGamma(int_val);
	}
	else if (y_stricmp(var_name, "ratio") == 0)
	{
		grid.ratio = CLAMP(0, int_val, 8);
		main_win->info_bar->UpdateRatio();
		RedrawMap();
	}
	else if (y_stricmp(var_name, "sec_render") == 0)
	{
		int_val = CLAMP(0, int_val, (int)SREND_SoundProp);
		edit.sector_render_mode = (sector_rendering_mode_e) int_val;
		RedrawMap();
	}
	else
	{
		Beep("Set: unknown var: %s", var_name);
	}
}


void CMD_ToggleVar()
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

		main_win->BrowserMode('/');
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
	else if (y_stricmp(var_name, "sprites") == 0)
	{
		edit.thing_render_mode = ! edit.thing_render_mode;
		RedrawMap();
	}
	else if (y_stricmp(var_name, "obj_nums") == 0)
	{
		edit.show_object_numbers = ! edit.show_object_numbers;
		RedrawMap();
	}
	else if (y_stricmp(var_name, "gamma") == 0)
	{
		SetGamma((usegamma >= 4) ? 0 : usegamma + 1);
	}
	else if (y_stricmp(var_name, "ratio") == 0)
	{
		if (grid.ratio >= 8)
			grid.ratio = 0;
		else
			grid.ratio++;

		main_win->info_bar->UpdateRatio();
		RedrawMap();
	}
	else if (y_stricmp(var_name, "sec_render") == 0)
	{
		if (edit.sector_render_mode >= SREND_SoundProp)
			edit.sector_render_mode = SREND_Nothing;
		else
			edit.sector_render_mode = (sector_rendering_mode_e)(1 + (int)edit.sector_render_mode);
		RedrawMap();
	}
	else
	{
		Beep("Toggle: unknown var: %s", var_name);
	}
}


void CMD_BrowserMode()
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

	// if that browser is already open, close it now
	if (main_win->browser->visible() &&
		main_win->browser->GetMode() == mode &&
		! Exec_HasFlag("/force") &&
		! Exec_HasFlag("/recent"))
	{
		main_win->BrowserMode(0);
		return;
	}

	main_win->BrowserMode(mode);

	if (Exec_HasFlag("/recent"))
	{
		main_win->browser->ToggleRecent(true /* force */);
	}
}


void CMD_Scroll()
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
}


static void NAV_Scroll_Left_release(void)
{
	edit.nav_left = 0;
}

void CMD_NAV_Scroll_Left()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = atof(EXEC_Param[0]);
	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;
	edit.nav_left = perc * base_size / 100.0 / grid.Scale;

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Left_release);
}


static void NAV_Scroll_Right_release(void)
{
	edit.nav_right = 0;
}

void CMD_NAV_Scroll_Right()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = atof(EXEC_Param[0]);
	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;
	edit.nav_right = perc * base_size / 100.0 / grid.Scale;

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Right_release);
}


static void NAV_Scroll_Up_release(void)
{
	edit.nav_up = 0;
}

void CMD_NAV_Scroll_Up()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = atof(EXEC_Param[0]);
	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;
	edit.nav_up = perc * base_size / 100.0 / grid.Scale;

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Up_release);
}


static void NAV_Scroll_Down_release(void)
{
	edit.nav_down = 0;
}

void CMD_NAV_Scroll_Down()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = atof(EXEC_Param[0]);
	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;
	edit.nav_down = perc * base_size / 100.0 / grid.Scale;

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Down_release);
}


static void NAV_MouseScroll_release(void)
{
	Editor_ScrollMap(+1);
}

void CMD_NAV_MouseScroll()
{
	if (edit.render3d)
		return;

	if (! EXEC_CurKey)
		return;

	edit.panning_speed = atof(EXEC_Param[0]);
	edit.panning_lax = Exec_HasFlag("/LAX");

	if (! edit.is_navigating)
		Editor_ClearNav();

	if (Nav_SetKey(EXEC_CurKey, &NAV_MouseScroll_release))
	{
		Editor_ScrollMap(-1);
	}
}


static void DoBeginDrag();


void CheckBeginDrag()
{
	if (! edit.clicked.valid())
		return;

	if (! edit.click_check_drag)
		return;

	// can drag things and sector planes in 3D mode
	if (edit.render3d && !(edit.mode == OBJ_THINGS || edit.mode == OBJ_SECTORS))
		return;

	int pixel_dx = Fl::event_x() - edit.click_screen_x;
	int pixel_dy = Fl::event_y() - edit.click_screen_y;

	if (MAX(abs(pixel_dx), abs(pixel_dy)) < minimum_drag_pixels)
		return;

	// if highlighted object is in selection, we drag the selection,
	// otherwise we drag just this one object.

	if (edit.click_force_single || !edit.Selected->get(edit.clicked.num))
		edit.dragged = edit.clicked;
	else
		edit.dragged.clear();

	DoBeginDrag();
}

static void DoBeginDrag()
{
	edit.drag_start_x = edit.drag_cur_x = edit.click_map_x;
	edit.drag_start_y = edit.drag_cur_y = edit.click_map_y;
	edit.drag_start_z = edit.drag_cur_z = edit.click_map_z;

	edit.drag_screen_dx  = edit.drag_screen_dy = 0;
	edit.drag_thing_num  = -1;
	edit.drag_other_vert = -1;

	// the focus is only used when grid snapping is on
	GetDragFocus(&edit.drag_focus_x, &edit.drag_focus_y, edit.click_map_x, edit.click_map_y);

	if (edit.render3d)
	{
		if (edit.mode == OBJ_SECTORS)
			edit.drag_sector_dz = 0;

		if (edit.mode == OBJ_THINGS)
		{
			edit.drag_thing_num = edit.clicked.num;
			edit.drag_thing_floorh = edit.drag_start_z;
			edit.drag_thing_up_down = (Level_format != MAPF_Doom && !grid.snap);

			// get thing's floor
			if (edit.drag_thing_num >= 0)
			{
				const Thing *T = Things[edit.drag_thing_num];

				Objid sec;
				GetNearObject(sec, OBJ_SECTORS, T->x(), T->y());

				if (sec.valid())
					edit.drag_thing_floorh = Sectors[sec.num]->floorh;
			}
		}
	}

	// in vertex mode, show all the connected lines too
	if (edit.drag_lines)
	{
		delete edit.drag_lines;
		edit.drag_lines = NULL;
	}

	if (edit.mode == OBJ_VERTICES)
	{
		edit.drag_lines = new selection_c(OBJ_LINEDEFS);
		ConvertSelection(edit.Selected, edit.drag_lines);

		// find opposite end-point when dragging a single vertex
		if (edit.dragged.valid())
			edit.drag_other_vert = Vertex_FindDragOther(edit.dragged.num);
	}

	edit.clicked.clear();

	Editor_SetAction(ACT_DRAG);

	main_win->canvas->redraw();
}


static void ACT_SelectBox_release(void)
{
	// check if cancelled or overridden
	if (edit.action != ACT_SELBOX)
		return;

	Editor_ClearAction();
	Editor_ClearErrorMode();

	// a mere click and release will unselect everything
	double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	if (! main_win->canvas->SelboxGet(x1, y1, x2, y2))
	{
		ExecuteCommand("UnselectAll");
		return;
	}

	SelectObjectsInBox(edit.Selected, edit.mode, x1, y1, x2, y2);
	RedrawMap();
}


static void ACT_Drag_release(void)
{
	// check if cancelled or overridden
	if (edit.action != ACT_DRAG)
	{
		edit.dragged.clear();
		return;
	}

	if (edit.render3d)
	{
		if (edit.mode == OBJ_THINGS)
			Render3D_DragThings();

		if (edit.mode == OBJ_SECTORS)
			Render3D_DragSectors();
	}

	// note: DragDelta needs edit.dragged
	double dx, dy;
	main_win->canvas->DragDelta(&dx, &dy);

	Objid dragged(edit.dragged);
	edit.dragged.clear();

	if (edit.drag_lines)
		edit.drag_lines->clear_all();

	if (! edit.render3d && (dx || dy))
	{
		if (dragged.valid())
			DragSingleObject(dragged, dx, dy);
		else
			MoveObjects(edit.Selected, dx, dy);
	}

	Editor_ClearAction();

	RedrawMap();
}


static void ACT_Click_release(void)
{
	Objid click_obj(edit.clicked);
	edit.clicked.clear();

	edit.click_check_drag = false;


	if (edit.action == ACT_SELBOX)
	{
		ACT_SelectBox_release();
		return;
	}
	else if (edit.action == ACT_DRAG)
	{
		ACT_Drag_release();
		return;
	}

	// check if cancelled or overridden
	if (edit.action != ACT_CLICK)
		return;

	if (edit.click_check_select && click_obj.valid())
	{
		// only toggle selection if it's the same object as before
		Objid near_obj;
		if (edit.render3d)
			near_obj = edit.highlight;
		else
			GetNearObject(near_obj, edit.mode, edit.map_x, edit.map_y);

		if (near_obj.num == click_obj.num)
			Selection_Toggle(click_obj);
	}

	Editor_ClearAction();
	Editor_ClearErrorMode();

	RedrawMap();
}

void CMD_ACT_Click()
{
	if (! EXEC_CurKey)
		return;

	// require a highlighted object in 3D mode
	if (edit.render3d && edit.highlight.is_nil())
		return;

	if (! Nav_ActionKey(EXEC_CurKey, &ACT_Click_release))
		return;

	edit.click_check_select = ! Exec_HasFlag("/noselect");
	edit.click_check_drag   = ! Exec_HasFlag("/nodrag");
	edit.click_force_single = false;

	// remember some state (for drag detection)
	edit.click_screen_x = Fl::event_x();
	edit.click_screen_y = Fl::event_y();

	edit.click_map_x = edit.map_x;
	edit.click_map_y = edit.map_y;
	edit.click_map_z = edit.map_z;

	// handle 3D mode, skip stuff below which only makes sense in 2D
	if (edit.render3d)
	{
		if (edit.highlight.type == OBJ_THINGS)
		{
			const Thing *T = Things[edit.highlight.num];
			edit.drag_point_dist = r_view.DistToViewPlane(T->x(), T->y());
		}
		else
		{
			edit.drag_point_dist = r_view.DistToViewPlane(edit.map_x, edit.map_y);
		}

		edit.clicked = edit.highlight;
		Editor_SetAction(ACT_CLICK);
		return;
	}

	// check for splitting a line, and ensure we can drag the vertex
	if (! Exec_HasFlag("/nosplit") &&
		edit.mode == OBJ_VERTICES &&
		edit.split_line.valid() &&
		edit.action != ACT_DRAW_LINE)
	{
		int split_ld = edit.split_line.num;

		edit.click_force_single = true;   // if drag vertex, force single-obj mode
		edit.click_check_select = false;  // do NOT select the new vertex

		// check if both ends are in selection, if so (and only then)
		// shall we select the new vertex
		const LineDef *L = LineDefs[split_ld];

		bool want_select = edit.Selected->get(L->start) && edit.Selected->get(L->end);

		BA_Begin();
		BA_Message("split linedef #%d", split_ld);

		int new_vert = BA_New(OBJ_VERTICES);

		Vertex *V = Vertices[new_vert];

		V->SetRawXY(edit.split_x, edit.split_y);

		SplitLineDefAtVertex(split_ld, new_vert);

		BA_End();

		if (want_select)
			edit.Selected->set(new_vert);

		edit.clicked = Objid(OBJ_VERTICES, new_vert);
		Editor_SetAction(ACT_CLICK);

		RedrawMap();
		return;
	}

	// find the object under the pointer.
	GetNearObject(edit.clicked, edit.mode, edit.map_x, edit.map_y);

	// clicking on an empty space starts a new selection box
	if (edit.click_check_select && edit.clicked.is_nil())
	{
		edit.selbox_x1 = edit.selbox_x2 = edit.map_x;
		edit.selbox_y1 = edit.selbox_y2 = edit.map_y;

		Editor_SetAction(ACT_SELBOX);
		return;
	}

	Editor_SetAction(ACT_CLICK);
}


void CMD_ACT_SelectBox()
{
	if (edit.render3d)
		return;

	if (! EXEC_CurKey)
		return;

	if (! Nav_ActionKey(EXEC_CurKey, &ACT_SelectBox_release))
		return;

	edit.selbox_x1 = edit.selbox_x2 = edit.map_x;
	edit.selbox_y1 = edit.selbox_y2 = edit.map_y;

	Editor_SetAction(ACT_SELBOX);
}


void CMD_ACT_Drag()
{
	if (! EXEC_CurKey)
		return;

	if (edit.Selected->empty())
	{
		Beep("Nothing to drag");
		return;
	}

	if (! Nav_ActionKey(EXEC_CurKey, &ACT_Drag_release))
		return;

	// we only drag the selection, never a single object
	edit.dragged.clear();

	DoBeginDrag();
}


void Transform_Update()
{
	double dx1 = edit.map_x - edit.trans_param.mid_x;
	double dy1 = edit.map_y - edit.trans_param.mid_y;

	double dx0 = edit.trans_start_x - edit.trans_param.mid_x;
	double dy0 = edit.trans_start_y - edit.trans_param.mid_y;

	edit.trans_param.scale_x = edit.trans_param.scale_y = 1;
	edit.trans_param.skew_x  = edit.trans_param.skew_y  = 0;
	edit.trans_param.rotate  = 0;

	if (edit.trans_mode == TRANS_K_Rotate || edit.trans_mode == TRANS_K_RotScale)
	{
		int angle1 = (int)ComputeAngle(dx1, dy1);
		int angle0 = (int)ComputeAngle(dx0, dy0);

		edit.trans_param.rotate = angle1 - angle0;

//		fprintf(stderr, "angle diff : %1.2f\n", edit.trans_rotate * 360.0 / 65536.0);
	}

	switch (edit.trans_mode)
	{
		case TRANS_K_Scale:
		case TRANS_K_RotScale:
			dx1 = MAX(abs(dx1), abs(dy1));
			dx0 = MAX(abs(dx0), abs(dy0));

			if (dx0)
			{
				edit.trans_param.scale_x = dx1 / (float)dx0;
				edit.trans_param.scale_y = edit.trans_param.scale_x;
			}
			break;

		case TRANS_K_Stretch:
			if (dx0) edit.trans_param.scale_x = dx1 / (float)dx0;
			if (dy0) edit.trans_param.scale_y = dy1 / (float)dy0;
			break;

		case TRANS_K_Rotate:
			// already done
			break;

		case TRANS_K_Skew:
			if (abs(dx0) >= abs(dy0))
			{
				if (dx0) edit.trans_param.skew_y = (dy1 - dy0) / (float)dx0;
			}
			else
			{
				if (dy0) edit.trans_param.skew_x = (dx1 - dx0) / (float)dy0;
			}
			break;
	}

	main_win->canvas->redraw();
}


static void ACT_Transform_release(void)
{
	// check if cancelled or overridden
	if (edit.action != ACT_TRANSFORM)
		return;

	if (edit.trans_lines)
		edit.trans_lines->clear_all();

	TransformObjects(edit.trans_param);

	Editor_ClearAction();

	RedrawMap();
}

void CMD_ACT_Transform()
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


	double middle_x, middle_y;
	Objs_CalcMiddle(edit.Selected, &middle_x, &middle_y);

	edit.trans_mode = mode;
	edit.trans_start_x = edit.map_x;
	edit.trans_start_y = edit.map_y;

	edit.trans_param.Clear();
	edit.trans_param.mid_x = middle_x;
	edit.trans_param.mid_y = middle_y;

	if (edit.trans_lines)
	{
		delete edit.trans_lines;
		edit.trans_lines = NULL;
	}

	if (edit.mode == OBJ_VERTICES)
	{
		edit.trans_lines = new selection_c(OBJ_LINEDEFS);
		ConvertSelection(edit.Selected, edit.trans_lines);
	}

	Editor_SetAction(ACT_TRANSFORM);
}


void CMD_WHEEL_Scroll()
{
	float speed = atof(EXEC_Param[0]);

	if (Exec_HasFlag("/LAX"))
	{
		keycode_t mod = Fl::event_state() & MOD_ALL_MASK;

		if (mod & MOD_SHIFT)
			speed /= 3.0;
		else if (mod & MOD_COMMAND)
			speed *= 3.0;
	}

	float delta_x =     wheel_dx;
	float delta_y = 0 - wheel_dy;

	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;

	speed = speed * base_size / 100.0 / grid.Scale;

	grid.Scroll(delta_x * speed, delta_y * speed);
}


void CMD_Merge()
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


void CMD_Disconnect()
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


void CMD_Zoom()
{
	int delta = atoi(EXEC_Param[0]);

	if (delta == 0)
	{
		Beep("Zoom: bad or missing value");
		return;
	}

	int mid_x = edit.map_x;
	int mid_y = edit.map_y;

	if (Exec_HasFlag("/center"))
	{
		mid_x = I_ROUND(grid.orig_x);
		mid_y = I_ROUND(grid.orig_y);
	}

	Editor_Zoom(delta, mid_x, mid_y);
}


void CMD_ZoomWholeMap()
{
	if (edit.render3d)
		Render3D_Enable(false);

	ZoomWholeMap();
}


void CMD_ZoomSelection()
{
	if (edit.Selected->empty())
	{
		Beep("No selection to zoom");
		return;
	}

	GoToSelection();
}


void CMD_GoToCamera()
{
	if (edit.render3d)
		Render3D_Enable(false);

	double x, y; float angle;
	Render3D_GetCameraPos(&x, &y, &angle);

	grid.MoveTo(x, y);

	RedrawMap();
}


void CMD_PlaceCamera()
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

	double x = edit.map_x;
	double y = edit.map_y;

	Render3D_SetCameraPos(x, y);

	if (Exec_HasFlag("/open3d"))
	{
		Render3D_Enable(true);
	}

	RedrawMap();
}


void CMD_MoveObjects_Dialog()
{
	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("Nothing to move");
		return;
	}

	bool want_dz = (edit.mode == OBJ_SECTORS);
	// can move things vertically in Hexen/UDMF formats
	if (edit.mode == OBJ_THINGS && Level_format != MAPF_Doom)
		want_dz = true;

	UI_MoveDialog * dialog = new UI_MoveDialog(want_dz);

	dialog->Run();

	delete dialog;

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);
}


void CMD_ScaleObjects_Dialog()
{
	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("Nothing to scale");
		return;
	}

	UI_ScaleDialog * dialog = new UI_ScaleDialog();

	dialog->Run();

	delete dialog;

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);
}


void CMD_RotateObjects_Dialog()
{
	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("Nothing to rotate");
		return;
	}

	UI_RotateDialog * dialog = new UI_RotateDialog();

	dialog->Run();

	delete dialog;

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);
}


void CMD_GRID_Bump()
{
	int delta = atoi(EXEC_Param[0]);

	delta = (delta >= 0) ? +1 : -1;

	grid.AdjustStep(delta);
}


void CMD_GRID_Set()
{
	int step = atoi(EXEC_Param[0]);

	if (step < 2 || step > 4096)
	{
		Beep("Bad grid step");
		return;
	}

	grid.ForceStep(step);
}


void CMD_GRID_Zoom()
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


void CMD_BR_CycleCategory()
{
	if (! main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	int dir = (atoi(EXEC_Param[0]) >= 0) ? +1 : -1;

	main_win->browser->CycleCategory(dir);
}


void CMD_BR_ClearSearch()
{
	if (! main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	main_win->browser->ClearSearchBox();
}


void CMD_BR_Scroll()
{
	if (! main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	if (! EXEC_Param[0][0])
	{
		Beep("BR_Scroll: missing value");
		return;
	}

	int delta = atoi(EXEC_Param[0]);

	main_win->browser->Scroll(delta);
}


void CMD_DefaultProps()
{
	main_win->ShowDefaultProps();
}


void CMD_FindDialog()
{
	main_win->ShowFindAndReplace();
}


void CMD_FindNext()
{
	main_win->find_box->FindNext();
}


void CMD_LogViewer()
{
	LogViewer_Open();
}


void CMD_OnlineDocs()
{
	int rv = fl_open_uri("http://eureka-editor.sourceforge.net/?n=Docs.Index");
	if (rv == 1)
		Status_Set("Opened web browser");
	else
		Beep("Failed to open web browser");
}


void CMD_AboutDialog()
{
	DLG_AboutText();
}


//------------------------------------------------------------------------


static editor_command_t  command_table[] =
{
	/* ---- miscellaneous / UI stuff ---- */

	{	"Nothing", "Misc",
		&CMD_Nothing
	},

	{	"Set", "Misc",
		&CMD_SetVar,
		/* flags */ NULL,
		/* keywords */ "3d browser gamma grid obj_nums ratio sec_render snap sprites"
	},

	{	"Toggle", "Misc",
		&CMD_ToggleVar,
		/* flags */ NULL,
		/* keywords */ "3d browser gamma grid obj_nums ratio sec_render snap recent sprites"
	},

	{	"MetaKey", "Misc",
		&CMD_MetaKey
	},

	{	"EditMode", "Misc",
		&CMD_EditMode,
		/* flags */ NULL,
		/* keywords */ "thing line sector vertex"
	},

	{	"OperationMenu",  "Misc",
		&CMD_OperationMenu
	},

	{	"MapCheck", "Misc",
		&CMD_MapCheck,
		/* flags */ NULL,
		/* keywords */ "all major vertices sectors linedefs things textures tags current"
	},


	/* ----- 2D canvas ----- */

	{	"Scroll",  "2D View",
		&CMD_Scroll
	},

	{	"GRID_Bump",  "2D View",
		&CMD_GRID_Bump
	},

	{	"GRID_Set",  "2D View",
		&CMD_GRID_Set
	},

	{	"GRID_Zoom",  "2D View",
		&CMD_GRID_Zoom
	},

	{	"ACT_SelectBox", "2D View",
		&CMD_ACT_SelectBox
	},

	{	"WHEEL_Scroll",  "2D View",
		&CMD_WHEEL_Scroll
	},

	{	"NAV_Scroll_Left",  "2D View",
		&CMD_NAV_Scroll_Left
	},

	{	"NAV_Scroll_Right",  "2D View",
		&CMD_NAV_Scroll_Right
	},

	{	"NAV_Scroll_Up",  "2D View",
		&CMD_NAV_Scroll_Up
	},

	{	"NAV_Scroll_Down",  "2D View",
		&CMD_NAV_Scroll_Down
	},

	{	"NAV_MouseScroll", "2D View",
		&CMD_NAV_MouseScroll
	},


	/* ----- FILE menu ----- */

	{	"NewProject",  "File",
		&CMD_NewProject
	},

	{	"ManageProject",  "File",
		&CMD_ManageProject
	},

	{	"OpenMap",  "File",
		&CMD_OpenMap
	},

	{	"GivenFile",  "File",
		&CMD_GivenFile,
		/* flags */ NULL,
		/* keywords */ "next prev first last current"
	},

	{	"FlipMap",  "File",
		&CMD_FlipMap,
		/* flags */ NULL,
		/* keywords */ "next prev first last"
	},

	{	"SaveMap",  "File",
		&CMD_SaveMap
	},

	{	"ExportMap",  "File",
		&CMD_ExportMap
	},

	{	"FreshMap",  "File",
		&CMD_FreshMap
	},

	{	"CopyMap",  "File",
		&CMD_CopyMap
	},

	{	"RenameMap",  "File",
		&CMD_RenameMap
	},

	{	"DeleteMap",  "File",
		&CMD_DeleteMap
	},

	{	"Quit",  "File",
		&CMD_Quit
	},


	/* ----- EDIT menu ----- */

	{	"Undo",   "Edit",
		&CMD_Undo
	},

	{	"Redo",   "Edit",
		&CMD_Redo
	},

	{	"Insert",	"Edit",
		&CMD_Insert,
		/* flags */ "/continue /nofill"
	},

	{	"Delete",	"Edit",
		&CMD_Delete,
		/* flags */ "/keep"
	},

	{	"Clipboard_Cut",   "Edit",
		&CMD_Clipboard_Cut
	},

	{	"Clipboard_Copy",   "Edit",
		&CMD_Clipboard_Copy
	},

	{	"Clipboard_Paste",   "Edit",
		&CMD_Clipboard_Paste
	},

	{	"Select",	"Edit",
		&CMD_Select
	},

	{	"SelectAll",	"Edit",
		&CMD_SelectAll
	},

	{	"UnselectAll",	"Edit",
		&CMD_UnselectAll
	},

	{	"InvertSelection",	"Edit",
		&CMD_InvertSelection
	},

	{	"LastSelection",	"Edit",
		&CMD_LastSelection
	},

	{	"CopyAndPaste",   "Edit",
		&CMD_CopyAndPaste
	},

	{	"CopyProperties",   "Edit",
		&CMD_CopyProperties,
		/* flags */ "/reverse"
	},

	{	"PruneUnused",   "Edit",
		&CMD_PruneUnused
	},

	{	"MoveObjectsDialog",   "Edit",
		&CMD_MoveObjects_Dialog
	},

	{	"ScaleObjectsDialog",   "Edit",
		&CMD_ScaleObjects_Dialog
	},

	{	"RotateObjectsDialog",   "Edit",
		&CMD_RotateObjects_Dialog
	},


	/* ----- VIEW menu ----- */

	{	"Zoom",  "View",
		&CMD_Zoom,
		/* flags */ "/center"
	},

	{	"ZoomWholeMap",  "View",
		&CMD_ZoomWholeMap
	},

	{	"ZoomSelection",  "View",
		&CMD_ZoomSelection
	},

	{	"DefaultProps",  "View",
		&CMD_DefaultProps
	},

	{	"FindDialog",  "View",
		&CMD_FindDialog
	},

	{	"FindNext",  "View",
		&CMD_FindNext
	},

	{	"GoToCamera",  "View",
		&CMD_GoToCamera
	},

	{	"PlaceCamera",  "View",
		&CMD_PlaceCamera,
		/* flags */ "/open3d"
	},

	{	"JumpToObject",  "View",
		&CMD_JumpToObject
	},


	/* ------ TOOLS menu ------ */

	{	"PreferenceDialog",  "Tools",
		&CMD_Preferences
	},

	{	"TestMap",  "Tools",
		&CMD_TestMap
	},

	{	"BuildAllNodes",  "Tools",
		&CMD_BuildAllNodes
	},

	{	"EditLump",  "Tools",
		&CMD_EditLump,
		/* flags */ "/header /scripts"
	},

	{	"AddBehavior",  "Tools",
		&CMD_AddBehaviorLump
	},

	{	"LogViewer",  "Tools",
		&CMD_LogViewer
	},


	/* ------ HELP menu ------ */

	{	"OnlineDocs",  "Help",
		&CMD_OnlineDocs
	},

	{	"AboutDialog",  "Help",
		&CMD_AboutDialog
	},


	/* ----- general operations ----- */

	{	"Merge",	"General",
		&CMD_Merge,
		/* flags */ "/keep"
	},

	{	"Disconnect",	"General",
		&CMD_Disconnect
	},

	{	"Mirror",	"General",
		&CMD_Mirror,
		/* flags */ NULL,
		/* keywords */ "horiz vert"
	},

	{	"Rotate90",	"General",
		&CMD_Rotate90,
		/* flags */ NULL,
		/* keywords */ "cw acw"
	},

	{	"Enlarge",	"General",
		&CMD_Enlarge
	},

	{	"Shrink",	"General",
		&CMD_Shrink
	},

	{	"Quantize",	"General",
		&CMD_Quantize
	},

	{	"ApplyTag",	"General",
		&CMD_ApplyTag,
		/* flags */ NULL,
		/* keywords */ "fresh last"
	},

	{	"ACT_Click", "General",
		&CMD_ACT_Click,
		/* flags */ "/noselect /nodrag /nosplit"
	},

	{	"ACT_Drag", "General",
		&CMD_ACT_Drag
	},

	{	"ACT_Transform", "General",
		&CMD_ACT_Transform,
		/* flags */ NULL,
		/* keywords */ "scale stretch rotate rotscale skew"
	},


	/* ------ LineDef mode ------ */

	{	"LIN_Align", NULL,
		&CMD_LIN_Align,
		/* flags */ "/x /y /right /clear"
	},

	{	"LIN_Flip", NULL,
		&CMD_LIN_Flip,
		/* flags */ "/force"
	},

	{	"LIN_SwapSides", NULL,
		&CMD_LIN_SwapSides
	},

	{	"LIN_SplitHalf", NULL,
		&CMD_LIN_SplitHalf
	},

	{	"LIN_SelectPath", NULL,
		&CMD_LIN_SelectPath,
		/* flags */ "/fresh /onesided /sametex"
	},


	/* ------ Sector mode ------ */

	{	"SEC_Floor", NULL,
		&CMD_SEC_Floor
	},

	{	"SEC_Ceil", NULL,
		&CMD_SEC_Ceil
	},

	{	"SEC_Light", NULL,
		&CMD_SEC_Light
	},

	{	"SEC_SelectGroup", NULL,
		&CMD_SEC_SelectGroup,
		/* flags */ "/fresh /can_walk /doors /floor_h /floor_tex /ceil_h /ceil_tex /light /tag /special"
	},

	{	"SEC_SwapFlats", NULL,
		&CMD_SEC_SwapFlats
	},


	/* ------ Thing mode ------ */

	{	"TH_Spin", NULL,
		&CMD_TH_SpinThings
	},


	/* ------ Vertex mode ------ */

	{	"VT_ShapeLine", NULL,
		&CMD_VT_ShapeLine
	},

	{	"VT_ShapeArc", NULL,
		&CMD_VT_ShapeArc
	},


	/* -------- Browser -------- */

	{	"BrowserMode", "Browser",
		&CMD_BrowserMode,
		/* flags */ "/recent",
		/* keywords */ "obj tex flat line sec genline"
	},

	{	"BR_CycleCategory", "Browser",
		&CMD_BR_CycleCategory
	},

	{	"BR_ClearSearch", "Browser",
		&CMD_BR_ClearSearch
	},

	{	"BR_Scroll", "Browser",
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
