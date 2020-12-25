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

#include "Document.h"
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
#include "r_subdiv.h"
#include "ui_window.h"
#include "ui_about.h"
#include "ui_misc.h"
#include "ui_prefs.h"


// config items
int config::minimum_drag_pixels = 5;


void CMD_Nothing(Document &doc)
{
	/* hey jude, don't make it bad */
}


void CMD_MetaKey(Document &doc)
{
	if (edit.sticky_mod)
	{
		ClearStickyMod();
		return;
	}

	Status_Set("META...");

	edit.sticky_mod = MOD_META;
}


void CMD_EditMode(Document &doc)
{
	char mode = tolower(EXEC_Param[0][0]);

	if (! mode || ! strchr("lstvr", mode))
	{
		Beep("Bad parameter for EditMode: '%s'", EXEC_Param[0].c_str());
		return;
	}

	Editor_ChangeMode(mode);
}


static void CMD_Select(Document &doc)
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


static void CMD_SelectAll(Document &doc)
{
	Editor_ClearErrorMode();

	int total = doc.numObjects(edit.mode);

	Selection_Push();

	edit.Selected->change_type(edit.mode);
	edit.Selected->frob_range(0, total-1, BitOp::add);

	RedrawMap();
}


static void CMD_UnselectAll(Document &doc)
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


static void CMD_InvertSelection(Document &doc)
{
	// do not clear selection when in error mode
	edit.error_mode = false;

	int total = doc.numObjects(edit.mode);

	if (edit.Selected->what_type() != edit.mode)
	{
		// convert the selection
		selection_c *prev_sel = edit.Selected;
		edit.Selected = new selection_c(edit.mode, true /* extended */);

		ConvertSelection(prev_sel, edit.Selected);
		delete prev_sel;
	}

	edit.Selected->frob_range(0, total-1, BitOp::toggle);

	RedrawMap();
}


static void CMD_Quit(Document &doc)
{
	Main_Quit();
}


static void CMD_Undo(Document &doc)
{
	if (! doc.basis.undo())
	{
		Beep("No operation to undo");
		return;
	}

	RedrawMap();
	instance::main_win->UpdatePanelObj();
}


static void CMD_Redo(Document &doc)
{
	if (! gDocument.basis.redo())
	{
		Beep("No operation to redo");
		return;
	}

	RedrawMap();
	instance::main_win->UpdatePanelObj();
}


static void SetGamma(int new_val)
{
	config::usegamma = CLAMP(0, new_val, 4);

	W_UpdateGamma();

	// for OpenGL, need to reload all images
	if (instance::main_win && instance::main_win->canvas)
		instance::main_win->canvas->DeleteContext();

	Status_Set("gamma level %d", config::usegamma);

	RedrawMap();
}


void CMD_SetVar(Document &doc)
{
	SString var_name = EXEC_Param[0];
	SString value    = EXEC_Param[1];

	if (var_name.empty())
	{
		Beep("Set: missing var name");
		return;
	}

	if (value.empty())
	{
		Beep("Set: missing value");
		return;
	}

	 int  int_val = atoi(value);
	bool bool_val = (int_val > 0);


	if (var_name.noCaseEqual("3d"))
	{
		Render3D_Enable(bool_val);
	}
	else if (var_name.noCaseEqual("browser"))
	{
		Editor_ClearAction();

		int want_vis   = bool_val ? 1 : 0;
		int is_visible = instance::main_win->browser->visible() ? 1 : 0;

		if (want_vis != is_visible)
			instance::main_win->BrowserMode('/');
	}
	else if (var_name.noCaseEqual("grid"))
	{
		grid.SetShown(bool_val);
	}
	else if (var_name.noCaseEqual("snap"))
	{
		grid.SetSnap(bool_val);
	}
	else if (var_name.noCaseEqual("sprites"))
	{
		edit.thing_render_mode = int_val;
		RedrawMap();
	}
	else if (var_name.noCaseEqual("obj_nums"))
	{
		edit.show_object_numbers = bool_val;
		RedrawMap();
	}
	else if (var_name.noCaseEqual("gamma"))
	{
		SetGamma(int_val);
	}
	else if (var_name.noCaseEqual("ratio"))
	{
		grid.ratio = CLAMP(0, int_val, 7);
		instance::main_win->info_bar->UpdateRatio();
		RedrawMap();
	}
	else if (var_name.noCaseEqual("sec_render"))
	{
		int_val = CLAMP(0, int_val, (int)SREND_SoundProp);
		edit.sector_render_mode = (sector_rendering_mode_e) int_val;

		if (edit.render3d)
			Render3D_Enable(false);

		// need sectors mode for sound propagation display
		if (edit.sector_render_mode == SREND_SoundProp && edit.mode != ObjType::sectors)
			Editor_ChangeMode('s');

		RedrawMap();
	}
	else
	{
		Beep("Set: unknown var: %s", var_name.c_str());
	}
}


void CMD_ToggleVar(Document &doc)
{
	SString var_name = EXEC_Param[0];

	if (var_name.empty())
	{
		Beep("Toggle: missing var name");
		return;
	}

	if (var_name.noCaseEqual("3d"))
	{
		Render3D_Enable(! edit.render3d);
	}
	else if (var_name.noCaseEqual("browser"))
	{
		Editor_ClearAction();

		instance::main_win->BrowserMode('/');
	}
	else if (var_name.noCaseEqual("recent"))
	{
		instance::main_win->browser->ToggleRecent();
	}
	else if (var_name.noCaseEqual("grid"))
	{
		grid.ToggleShown();
	}
	else if (var_name.noCaseEqual("snap"))
	{
		grid.ToggleSnap();
	}
	else if (var_name.noCaseEqual("sprites"))
	{
		edit.thing_render_mode = ! edit.thing_render_mode;
		RedrawMap();
	}
	else if (var_name.noCaseEqual("obj_nums"))
	{
		edit.show_object_numbers = ! edit.show_object_numbers;
		RedrawMap();
	}
	else if (var_name.noCaseEqual("gamma"))
	{
		SetGamma((config::usegamma >= 4) ? 0 : config::usegamma + 1);
	}
	else if (var_name.noCaseEqual("ratio"))
	{
		if (grid.ratio >= 7)
			grid.ratio = 0;
		else
			grid.ratio++;

		instance::main_win->info_bar->UpdateRatio();
		RedrawMap();
	}
	else if (var_name.noCaseEqual("sec_render"))
	{
		if (edit.sector_render_mode >= SREND_SoundProp)
			edit.sector_render_mode = SREND_Nothing;
		else
			edit.sector_render_mode = (sector_rendering_mode_e)(1 + (int)edit.sector_render_mode);
		RedrawMap();
	}
	else
	{
		Beep("Toggle: unknown var: %s", var_name.c_str());
	}
}


static void CMD_BrowserMode(Document &doc)
{
	if (EXEC_Param[0].empty())
	{
		Beep("BrowserMode: missing mode");
		return;
	}

	char mode = toupper(EXEC_Param[0][0]);

	if (! (mode == 'L' || mode == 'S' || mode == 'O' ||
	       mode == 'T' || mode == 'F' || mode == 'G'))
	{
		Beep("Unknown browser mode: %s", EXEC_Param[0].c_str());
		return;
	}

	// if that browser is already open, close it now
	if (instance::main_win->browser->visible() &&
		instance::main_win->browser->GetMode() == mode &&
		! Exec_HasFlag("/force") &&
		! Exec_HasFlag("/recent"))
	{
		instance::main_win->BrowserMode(0);
		return;
	}

	instance::main_win->BrowserMode(mode);

	if (Exec_HasFlag("/recent"))
	{
		instance::main_win->browser->ToggleRecent(true /* force */);
	}
}


void CMD_Scroll(Document &doc)
{
	// these are percentages
	float delta_x = static_cast<float>(atof(EXEC_Param[0]));
	float delta_y = static_cast<float>(atof(EXEC_Param[1]));

	if (delta_x == 0 && delta_y == 0)
	{
		Beep("Bad parameter to Scroll: '%s' %s'", EXEC_Param[0].c_str(), EXEC_Param[1].c_str());
		return;
	}

	int base_size = (instance::main_win->canvas->w() + instance::main_win->canvas->h()) / 2;

	delta_x = static_cast<float>(delta_x * base_size / 100.0 / grid.Scale);
	delta_y = static_cast<float>(delta_y * base_size / 100.0 / grid.Scale);

	grid.Scroll(delta_x, delta_y);
}


static void NAV_Scroll_Left_release(void)
{
	edit.nav_left = 0;
}

void CMD_NAV_Scroll_Left(Document &doc)
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = static_cast<float>(atof(EXEC_Param[0]));
	int base_size = (instance::main_win->canvas->w() + instance::main_win->canvas->h()) / 2;
	edit.nav_left = static_cast<float>(perc * base_size / 100.0 / grid.Scale);

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Left_release);
}


static void NAV_Scroll_Right_release(void)
{
	edit.nav_right = 0;
}

void CMD_NAV_Scroll_Right(Document &doc)
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = static_cast<float>(atof(EXEC_Param[0]));
	int base_size = (instance::main_win->canvas->w() + instance::main_win->canvas->h()) / 2;
	edit.nav_right = static_cast<float>(perc * base_size / 100.0 / grid.Scale);

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Right_release);
}


static void NAV_Scroll_Up_release(void)
{
	edit.nav_up = 0;
}

void CMD_NAV_Scroll_Up(Document &doc)
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = static_cast<float>(atof(EXEC_Param[0]));
	int base_size = (instance::main_win->canvas->w() + instance::main_win->canvas->h()) / 2;
	edit.nav_up = static_cast<float>(perc * base_size / 100.0 / grid.Scale);

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Up_release);
}


static void NAV_Scroll_Down_release(void)
{
	edit.nav_down = 0;
}

static void CMD_NAV_Scroll_Down(Document &doc)
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float perc = static_cast<float>(atof(EXEC_Param[0]));
	int base_size = (instance::main_win->canvas->w() + instance::main_win->canvas->h()) / 2;
	edit.nav_down = static_cast<float>(perc * base_size / 100.0 / grid.Scale);

	Nav_SetKey(EXEC_CurKey, &NAV_Scroll_Down_release);
}


static void NAV_MouseScroll_release(void)
{
	Editor_ScrollMap(+1);
}

static void CMD_NAV_MouseScroll(Document &doc)
{
	if (! EXEC_CurKey)
		return;

	edit.panning_speed = static_cast<float>(atof(EXEC_Param[0]));
	edit.panning_lax = Exec_HasFlag("/LAX");

	if (! edit.is_navigating)
		Editor_ClearNav();

	if (Nav_SetKey(EXEC_CurKey, &NAV_MouseScroll_release))
	{
		// begin
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
	if (edit.render3d && !(edit.mode == ObjType::things || edit.mode == ObjType::sectors))
		return;

	int pixel_dx = Fl::event_x() - edit.click_screen_x;
	int pixel_dy = Fl::event_y() - edit.click_screen_y;

	if (MAX(abs(pixel_dx), abs(pixel_dy)) < config::minimum_drag_pixels)
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
		if (edit.mode == ObjType::sectors)
			edit.drag_sector_dz = 0;

		if (edit.mode == ObjType::things)
		{
			edit.drag_thing_num = edit.clicked.num;
			edit.drag_thing_floorh = static_cast<float>(edit.drag_start_z);
			edit.drag_thing_up_down = (instance::Level_format != MapFormat::doom && !grid.snap);

			// get thing's floor
			if (edit.drag_thing_num >= 0)
			{
				const Thing *T = gDocument.things[edit.drag_thing_num];

				Objid sec = gDocument.hover.getNearbyObject(ObjType::sectors, T->x(), T->y());

				if (sec.valid())
					edit.drag_thing_floorh = static_cast<float>(gDocument.sectors[sec.num]->floorh);
			}
		}
	}

	// in vertex mode, show all the connected lines too
	if (edit.drag_lines)
	{
		delete edit.drag_lines;
		edit.drag_lines = NULL;
	}

	if (edit.mode == ObjType::vertices)
	{
		edit.drag_lines = new selection_c(ObjType::linedefs);
		ConvertSelection(edit.Selected, edit.drag_lines);

		// find opposite end-point when dragging a single vertex
		if (edit.dragged.valid())
			edit.drag_other_vert = gDocument.vertmod.findDragOther(edit.dragged.num);
	}

	edit.clicked.clear();

	Editor_SetAction(ACT_DRAG);

	instance::main_win->canvas->redraw();
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
	if (!instance::main_win->canvas->SelboxGet(x1, y1, x2, y2))
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
		if (edit.mode == ObjType::things)
			Render3D_DragThings();

		if (edit.mode == ObjType::sectors)
			Render3D_DragSectors();
	}

	// note: DragDelta needs edit.dragged
	double dx, dy;
	instance::main_win->canvas->DragDelta(&dx, &dy);

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
			near_obj = gDocument.hover.getNearbyObject(edit.mode, edit.map_x, edit.map_y);

		if (near_obj.num == click_obj.num)
			Selection_Toggle(click_obj);
	}

	Editor_ClearAction();
	Editor_ClearErrorMode();

	RedrawMap();
}

static void CMD_ACT_Click(Document &doc)
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
		if (edit.highlight.type == ObjType::things)
		{
			const Thing *T = doc.things[edit.highlight.num];
			edit.drag_point_dist = static_cast<float>(r_view.DistToViewPlane(T->x(), T->y()));
		}
		else
		{
			edit.drag_point_dist = static_cast<float>(r_view.DistToViewPlane(edit.map_x, edit.map_y));
		}

		edit.clicked = edit.highlight;
		Editor_SetAction(ACT_CLICK);
		return;
	}

	// check for splitting a line, and ensure we can drag the vertex
	if (! Exec_HasFlag("/nosplit") &&
		edit.mode == ObjType::vertices &&
		edit.split_line.valid() &&
		edit.action != ACT_DRAW_LINE)
	{
		int split_ld = edit.split_line.num;

		edit.click_force_single = true;   // if drag vertex, force single-obj mode
		edit.click_check_select = false;  // do NOT select the new vertex

		// check if both ends are in selection, if so (and only then)
		// shall we select the new vertex
		const LineDef *L = doc.linedefs[split_ld];

		bool want_select = edit.Selected->get(L->start) && edit.Selected->get(L->end);

		doc.basis.begin();
		doc.basis.setMessage("split linedef #%d", split_ld);

		int new_vert = doc.basis.addNew(ObjType::vertices);

		Vertex *V = doc.vertices[new_vert];

		V->SetRawXY(edit.split_x, edit.split_y);

		doc.linemod.splitLinedefAtVertex(split_ld, new_vert);

		doc.basis.end();

		if (want_select)
			edit.Selected->set(new_vert);

		edit.clicked = Objid(ObjType::vertices, new_vert);
		Editor_SetAction(ACT_CLICK);

		RedrawMap();
		return;
	}

	// find the object under the pointer.
	edit.clicked = doc.hover.getNearbyObject(edit.mode, edit.map_x, edit.map_y);

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


void CMD_ACT_SelectBox(Document &doc)
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


static void CMD_ACT_Drag(Document &doc)
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
		double angle[2] = { atan2(dy1, dx1), atan2(dy0, dx0) };

		edit.trans_param.rotate = angle[1] - angle[0];

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

	instance::main_win->canvas->redraw();
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

static void CMD_ACT_Transform(Document &doc)
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

	SString keyword = EXEC_Param[0];
	transform_keyword_e  mode;

	if (keyword.empty())
	{
		Beep("ACT_Transform: missing keyword");
		return;
	}
	else if (keyword.noCaseEqual("scale"))
	{
		mode = TRANS_K_Scale;
	}
	else if (keyword.noCaseEqual("stretch"))
	{
		mode = TRANS_K_Stretch;
	}
	else if (keyword.noCaseEqual("rotate"))
	{
		mode = TRANS_K_Rotate;
	}
	else if (keyword.noCaseEqual("rotscale"))
	{
		mode = TRANS_K_RotScale;
	}
	else if (keyword.noCaseEqual("skew"))
	{
		mode = TRANS_K_Skew;
	}
	else
	{
		Beep("ACT_Transform: unknown keyword: %s", keyword.c_str());
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

	if (edit.mode == ObjType::vertices)
	{
		edit.trans_lines = new selection_c(ObjType::linedefs);
		ConvertSelection(edit.Selected, edit.trans_lines);
	}

	Editor_SetAction(ACT_TRANSFORM);
}


void CMD_WHEEL_Scroll(Document &doc)
{
	float speed = static_cast<float>(atof(EXEC_Param[0]));

	if (Exec_HasFlag("/LAX"))
	{
		keycode_t mod = Fl::event_state() & MOD_ALL_MASK;

		if (mod & MOD_SHIFT)
			speed /= 3.0;
		else if (mod & MOD_COMMAND)
			speed *= 3.0;
	}

	float delta_x = static_cast<float>(wheel_dx);
	float delta_y = static_cast<float>(0 - wheel_dy);

	int base_size = (instance::main_win->canvas->w() + instance::main_win->canvas->h()) / 2;

	speed = static_cast<float>(speed * base_size / 100.0 / grid.Scale);

	grid.Scroll(delta_x * speed, delta_y * speed);
}


static void CMD_Merge(Document &doc)
{
	switch (edit.mode)
	{
		case ObjType::vertices:
			VertexModule::commandMerge();
			break;

		case ObjType::linedefs:
			LinedefModule::commandMergeTwo();
			break;

		case ObjType::sectors:
			SectorModule::commandMerge();
			break;

		case ObjType::things:
			CMD_TH_Merge();
			break;

		default:
			Beep("Cannot merge that");
			break;
	}
}


static void CMD_Disconnect(Document &doc)
{
	switch (edit.mode)
	{
		case ObjType::vertices:
			VertexModule::commandDisconnect();
			break;

		case ObjType::linedefs:
			VertexModule::commandLineDisconnect();
			break;

		case ObjType::sectors:
			VertexModule::commandSectorDisconnect();
			break;

		case ObjType::things:
			CMD_TH_Disconnect();
			break;

		default:
			Beep("Cannot disconnect that");
			break;
	}
}


static void CMD_Zoom(Document &doc)
{
	int delta = atoi(EXEC_Param[0]);

	if (delta == 0)
	{
		Beep("Zoom: bad or missing value");
		return;
	}

	int mid_x = static_cast<int>(edit.map_x);
	int mid_y = static_cast<int>(edit.map_y);

	if (Exec_HasFlag("/center"))
	{
		mid_x = I_ROUND(grid.orig_x);
		mid_y = I_ROUND(grid.orig_y);
	}

	Editor_Zoom(delta, mid_x, mid_y);
}


static void CMD_ZoomWholeMap(Document &doc)
{
	if (edit.render3d)
		Render3D_Enable(false);

	ZoomWholeMap();
}


static void CMD_ZoomSelection(Document &doc)
{
	if (edit.Selected->empty())
	{
		Beep("No selection to zoom");
		return;
	}

	GoToSelection();
}


static void CMD_GoToCamera(Document &doc)
{
	if (edit.render3d)
		Render3D_Enable(false);

	double x, y; float angle;
	Render3D_GetCameraPos(&x, &y, &angle);

	grid.MoveTo(x, y);

	RedrawMap();
}


static void CMD_PlaceCamera(Document &doc)
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


static void CMD_MoveObjects_Dialog(Document &doc)
{
	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("Nothing to move");
		return;
	}

	bool want_dz = (edit.mode == ObjType::sectors);
	// can move things vertically in Hexen/UDMF formats
	if (edit.mode == ObjType::things && instance::Level_format != MapFormat::doom)
		want_dz = true;

	UI_MoveDialog * dialog = new UI_MoveDialog(want_dz);

	dialog->Run();

	delete dialog;

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


static void CMD_ScaleObjects_Dialog(Document &doc)
{
	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("Nothing to scale");
		return;
	}

	UI_ScaleDialog * dialog = new UI_ScaleDialog();

	dialog->Run();

	delete dialog;

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


static void CMD_RotateObjects_Dialog(Document &doc)
{
	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("Nothing to rotate");
		return;
	}

	UI_RotateDialog * dialog = new UI_RotateDialog();

	dialog->Run();

	delete dialog;

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void CMD_GRID_Bump(Document &doc)
{
	int delta = atoi(EXEC_Param[0]);

	delta = (delta >= 0) ? +1 : -1;

	grid.AdjustStep(delta);
}


void CMD_GRID_Set(Document &doc)
{
	int step = atoi(EXEC_Param[0]);

	if (step < 2 || step > 4096)
	{
		Beep("Bad grid step");
		return;
	}

	grid.ForceStep(step);
}


void CMD_GRID_Zoom(Document &doc)
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

	float S1 = static_cast<float>(grid.Scale);

	grid.NearestScale(scale);

	grid.RefocusZoom(edit.map_x, edit.map_y, S1);
}


static void CMD_BR_CycleCategory(Document &doc)
{
	if (!instance::main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	int dir = (atoi(EXEC_Param[0]) >= 0) ? +1 : -1;

	instance::main_win->browser->CycleCategory(dir);
}


static void CMD_BR_ClearSearch(Document &doc)
{
	if (!instance::main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	instance::main_win->browser->ClearSearchBox();
}


static void CMD_BR_Scroll(Document &doc)
{
	if (!instance::main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	if (EXEC_Param[0].empty())
	{
		Beep("BR_Scroll: missing value");
		return;
	}

	int delta = atoi(EXEC_Param[0]);

	instance::main_win->browser->Scroll(delta);
}


static void CMD_DefaultProps(Document &doc)
{
	instance::main_win->ShowDefaultProps();
}


static void CMD_FindDialog(Document &doc)
{
	instance::main_win->ShowFindAndReplace();
}


static void CMD_FindNext(Document &doc)
{
	instance::main_win->find_box->FindNext();
}


static void CMD_RecalcSectors(Document &doc)
{
	Subdiv_InvalidateAll();
	RedrawMap();
}


static void CMD_LogViewer(Document &doc)
{
	LogViewer_Open();
}


static void CMD_OnlineDocs(Document &doc)
{
	int rv = fl_open_uri("http://eureka-editor.sourceforge.net/?n=Docs.Index");
	if (rv == 1)
		Status_Set("Opened web browser");
	else
		Beep("Failed to open web browser");
}


static void CMD_AboutDialog(Document &doc)
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

	{	"OpMenu",  "Misc",
		&CMD_OperationMenu
	},

	{	"MapCheck", "Misc",
		&ChecksModule::commandMapCheck,
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

	{	"RecalcSectors",  "Tools",
		&CMD_RecalcSectors
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
		&ChecksModule::commandApplyTag,
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
		&LinedefModule::commandAlign,
		/* flags */ "/x /y /right /clear"
	},

	{	"LIN_Flip", NULL,
		&LinedefModule::commandFlip,
		/* flags */ "/force"
	},

	{	"LIN_SwapSides", NULL,
		&LinedefModule::commandSwapSides
	},

	{	"LIN_SplitHalf", NULL,
		&LinedefModule::commandSplitHalf
	},

	{	"LIN_SelectPath", NULL,
		&CMD_LIN_SelectPath,
		/* flags */ "/fresh /onesided /sametex"
	},


	/* ------ Sector mode ------ */

	{	"SEC_Floor", NULL,
		&SectorModule::commandFloor
	},

	{	"SEC_Ceil", NULL,
		&SectorModule::commandCeiling
	},

	{	"SEC_Light", NULL,
		&SectorModule::commandLight
	},

	{	"SEC_SelectGroup", NULL,
		&CMD_SEC_SelectGroup,
		/* flags */ "/fresh /can_walk /doors /floor_h /floor_tex /ceil_h /ceil_tex /light /tag /special"
	},

	{	"SEC_SwapFlats", NULL,
		&SectorModule::commandSwapFlats
	},


	/* ------ Thing mode ------ */

	{	"TH_Spin", NULL,
		&CMD_TH_SpinThings
	},


	/* ------ Vertex mode ------ */

	{	"VT_ShapeLine", NULL,
		&VertexModule::commandShapeLine
	},

	{	"VT_ShapeArc", NULL,
		&VertexModule::commandShapeArc
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
