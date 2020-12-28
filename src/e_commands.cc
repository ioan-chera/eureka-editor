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

#include "Instance.h"
#include "main.h"

#include "e_main.h"
#include "e_path.h"
#include "m_config.h"
#include "m_editlump.h"
#include "m_loadsave.h"
#include "m_nodes.h"
#include "r_render.h"
#include "r_subdiv.h"
#include "ui_about.h"
#include "ui_misc.h"
#include "ui_prefs.h"


// config items
int config::minimum_drag_pixels = 5;


static void CMD_Nothing(Instance &inst)
{
	/* hey jude, don't make it bad */
}


static void CMD_MetaKey(Instance &inst)
{
	if (inst.edit.sticky_mod)
	{
		ClearStickyMod(inst);
		return;
	}

	inst.Status_Set("META...");

	inst.edit.sticky_mod = EMOD_META;
}


static void CMD_EditMode(Instance &inst)
{
	char mode = tolower(EXEC_Param[0][0]);

	if (! mode || ! strchr("lstvr", mode))
	{
		inst.Beep("Bad parameter for EditMode: '%s'", EXEC_Param[0].c_str());
		return;
	}

	inst.Editor_ChangeMode(mode);
}


static void CMD_Select(Instance &inst)
{
	if (inst.edit.render3d)
		return;

	// FIXME : action in effect?

	// FIXME : split_line in effect?

	if (inst.edit.highlight.is_nil())
	{
		inst.Beep("Nothing under cursor");
		return;
	}

	inst.Selection_Toggle(inst.edit.highlight);
	inst.RedrawMap();
}


static void CMD_SelectAll(Instance &inst)
{
	Editor_ClearErrorMode(inst);

	int total = inst.level.numObjects(inst.edit.mode);

	inst.Selection_Push();

	inst.edit.Selected->change_type(inst.edit.mode);
	inst.edit.Selected->frob_range(0, total-1, BitOp::add);

	inst.RedrawMap();
}


static void CMD_UnselectAll(Instance &inst)
{
	Editor_ClearErrorMode(inst);

	if (inst.edit.action == ACT_DRAW_LINE ||
		inst.edit.action == ACT_TRANSFORM)
	{
		inst.Editor_ClearAction();
	}

	Selection_Clear(inst);

	inst.RedrawMap();
}


static void CMD_InvertSelection(Instance &inst)
{
	// do not clear selection when in error mode
	inst.edit.error_mode = false;

	int total = inst.level.numObjects(inst.edit.mode);

	if (inst.edit.Selected->what_type() != inst.edit.mode)
	{
		// convert the selection
		selection_c *prev_sel = inst.edit.Selected;
		inst.edit.Selected = new selection_c(inst.edit.mode, true /* extended */);

		ConvertSelection(inst.level, prev_sel, inst.edit.Selected);
		delete prev_sel;
	}

	inst.edit.Selected->frob_range(0, total-1, BitOp::toggle);

	inst.RedrawMap();
}


static void CMD_Quit(Instance &inst)
{
	Main_Quit();
}


static void CMD_Undo(Instance &inst)
{
	if (! inst.level.basis.undo())
	{
		inst.Beep("No operation to undo");
		return;
	}

	inst.RedrawMap();
	inst.main_win->UpdatePanelObj();
}


static void CMD_Redo(Instance &inst)
{
	if (! inst.level.basis.redo())
	{
		inst.Beep("No operation to redo");
		return;
	}

	inst.RedrawMap();
	inst.main_win->UpdatePanelObj();
}


static void SetGamma(Instance &inst, int new_val)
{
	config::usegamma = CLAMP(0, new_val, 4);

	inst.W_UpdateGamma();

	// for OpenGL, need to reload all images
	if (inst.main_win && inst.main_win->canvas)
		inst.main_win->canvas->DeleteContext();

	inst.Status_Set("gamma level %d", config::usegamma);

	inst.RedrawMap();
}


static void CMD_SetVar(Instance &inst)
{
	SString var_name = EXEC_Param[0];
	SString value    = EXEC_Param[1];

	if (var_name.empty())
	{
		inst.Beep("Set: missing var name");
		return;
	}

	if (value.empty())
	{
		inst.Beep("Set: missing value");
		return;
	}

	 int  int_val = atoi(value);
	bool bool_val = (int_val > 0);


	if (var_name.noCaseEqual("3d"))
	{
		Render3D_Enable(inst, bool_val);
	}
	else if (var_name.noCaseEqual("browser"))
	{
		inst.Editor_ClearAction();

		int want_vis   = bool_val ? 1 : 0;
		int is_visible = inst.main_win->browser->visible() ? 1 : 0;

		if (want_vis != is_visible)
			inst.main_win->BrowserMode('/');
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
		inst.edit.thing_render_mode = int_val;
		inst.RedrawMap();
	}
	else if (var_name.noCaseEqual("obj_nums"))
	{
		inst.edit.show_object_numbers = bool_val;
		inst.RedrawMap();
	}
	else if (var_name.noCaseEqual("gamma"))
	{
		SetGamma(inst, int_val);
	}
	else if (var_name.noCaseEqual("ratio"))
	{
		grid.ratio = CLAMP(0, int_val, 7);
		inst.main_win->info_bar->UpdateRatio();
		inst.RedrawMap();
	}
	else if (var_name.noCaseEqual("sec_render"))
	{
		int_val = CLAMP(0, int_val, (int)SREND_SoundProp);
		inst.edit.sector_render_mode = (sector_rendering_mode_e) int_val;

		if (inst.edit.render3d)
			Render3D_Enable(inst, false);

		// need sectors mode for sound propagation display
		if (inst.edit.sector_render_mode == SREND_SoundProp && inst.edit.mode != ObjType::sectors)
			inst.Editor_ChangeMode('s');

		inst.RedrawMap();
	}
	else
	{
		inst.Beep("Set: unknown var: %s", var_name.c_str());
	}
}


static void CMD_ToggleVar(Instance &inst)
{
	SString var_name = EXEC_Param[0];

	if (var_name.empty())
	{
		inst.Beep("Toggle: missing var name");
		return;
	}

	if (var_name.noCaseEqual("3d"))
	{
		Render3D_Enable(inst, ! inst.edit.render3d);
	}
	else if (var_name.noCaseEqual("browser"))
	{
		inst.Editor_ClearAction();

		inst.main_win->BrowserMode('/');
	}
	else if (var_name.noCaseEqual("recent"))
	{
		inst.main_win->browser->ToggleRecent();
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
		inst.edit.thing_render_mode = ! inst.edit.thing_render_mode;
		inst.RedrawMap();
	}
	else if (var_name.noCaseEqual("obj_nums"))
	{
		inst.edit.show_object_numbers = ! inst.edit.show_object_numbers;
		inst.RedrawMap();
	}
	else if (var_name.noCaseEqual("gamma"))
	{
		SetGamma(inst, (config::usegamma >= 4) ? 0 : config::usegamma + 1);
	}
	else if (var_name.noCaseEqual("ratio"))
	{
		if (grid.ratio >= 7)
			grid.ratio = 0;
		else
			grid.ratio++;

		inst.main_win->info_bar->UpdateRatio();
		inst.RedrawMap();
	}
	else if (var_name.noCaseEqual("sec_render"))
	{
		if (inst.edit.sector_render_mode >= SREND_SoundProp)
			inst.edit.sector_render_mode = SREND_Nothing;
		else
			inst.edit.sector_render_mode = (sector_rendering_mode_e)(1 + (int)inst.edit.sector_render_mode);
		inst.RedrawMap();
	}
	else
	{
		inst.Beep("Toggle: unknown var: %s", var_name.c_str());
	}
}


static void CMD_BrowserMode(Instance &inst)
{
	if (EXEC_Param[0].empty())
	{
		inst.Beep("BrowserMode: missing mode");
		return;
	}

	char mode = toupper(EXEC_Param[0][0]);

	if (! (mode == 'L' || mode == 'S' || mode == 'O' ||
	       mode == 'T' || mode == 'F' || mode == 'G'))
	{
		inst.Beep("Unknown browser mode: %s", EXEC_Param[0].c_str());
		return;
	}

	// if that browser is already open, close it now
	if (inst.main_win->browser->visible() &&
		inst.main_win->browser->GetMode() == mode &&
		! Exec_HasFlag("/force") &&
		! Exec_HasFlag("/recent"))
	{
		inst.main_win->BrowserMode(0);
		return;
	}

	inst.main_win->BrowserMode(mode);

	if (Exec_HasFlag("/recent"))
	{
		inst.main_win->browser->ToggleRecent(true /* force */);
	}
}


static void CMD_Scroll(Instance &inst)
{
	// these are percentages
	float delta_x = static_cast<float>(atof(EXEC_Param[0]));
	float delta_y = static_cast<float>(atof(EXEC_Param[1]));

	if (delta_x == 0 && delta_y == 0)
	{
		inst.Beep("Bad parameter to Scroll: '%s' %s'", EXEC_Param[0].c_str(), EXEC_Param[1].c_str());
		return;
	}

	int base_size = (inst.main_win->canvas->w() + inst.main_win->canvas->h()) / 2;

	delta_x = static_cast<float>(delta_x * base_size / 100.0 / grid.Scale);
	delta_y = static_cast<float>(delta_y * base_size / 100.0 / grid.Scale);

	grid.Scroll(delta_x, delta_y);
}


void Instance::NAV_Scroll_Left_release()
{
	edit.nav_left = 0;
}

static void CMD_NAV_Scroll_Left(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	float perc = static_cast<float>(atof(EXEC_Param[0]));
	int base_size = (inst.main_win->canvas->w() + inst.main_win->canvas->h()) / 2;
	inst.edit.nav_left = static_cast<float>(perc * base_size / 100.0 / grid.Scale);

	Nav_SetKey(inst, EXEC_CurKey, &Instance::NAV_Scroll_Left_release);
}


void Instance::NAV_Scroll_Right_release()
{
	edit.nav_right = 0;
}

static void CMD_NAV_Scroll_Right(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	float perc = static_cast<float>(atof(EXEC_Param[0]));
	int base_size = (inst.main_win->canvas->w() + inst.main_win->canvas->h()) / 2;
	inst.edit.nav_right = static_cast<float>(perc * base_size / 100.0 / grid.Scale);

	Nav_SetKey(inst, EXEC_CurKey, &Instance::NAV_Scroll_Right_release);
}


void Instance::NAV_Scroll_Up_release()
{
	edit.nav_up = 0;
}

static void CMD_NAV_Scroll_Up(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	float perc = static_cast<float>(atof(EXEC_Param[0]));
	int base_size = (inst.main_win->canvas->w() + inst.main_win->canvas->h()) / 2;
	inst.edit.nav_up = static_cast<float>(perc * base_size / 100.0 / grid.Scale);

	Nav_SetKey(inst, EXEC_CurKey, &Instance::NAV_Scroll_Up_release);
}


void Instance::NAV_Scroll_Down_release()
{
	edit.nav_down = 0;
}

static void CMD_NAV_Scroll_Down(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	float perc = static_cast<float>(atof(EXEC_Param[0]));
	int base_size = (inst.main_win->canvas->w() + inst.main_win->canvas->h()) / 2;
	inst.edit.nav_down = static_cast<float>(perc * base_size / 100.0 / grid.Scale);

	Nav_SetKey(inst, EXEC_CurKey, &Instance::NAV_Scroll_Down_release);
}


void Instance::NAV_MouseScroll_release()
{
	Editor_ScrollMap(+1);
}

static void CMD_NAV_MouseScroll(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	inst.edit.panning_speed = static_cast<float>(atof(EXEC_Param[0]));
	inst.edit.panning_lax = Exec_HasFlag("/LAX");

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	if (Nav_SetKey(inst, EXEC_CurKey, &Instance::NAV_MouseScroll_release))
	{
		// begin
		inst.Editor_ScrollMap(-1);
	}
}


static void DoBeginDrag(Instance &inst);


void CheckBeginDrag(Instance &inst)
{
	if (! inst.edit.clicked.valid())
		return;

	if (! inst.edit.click_check_drag)
		return;

	// can drag things and sector planes in 3D mode
	if (inst.edit.render3d && !(inst.edit.mode == ObjType::things || inst.edit.mode == ObjType::sectors))
		return;

	int pixel_dx = Fl::event_x() - inst.edit.click_screen_x;
	int pixel_dy = Fl::event_y() - inst.edit.click_screen_y;

	if (MAX(abs(pixel_dx), abs(pixel_dy)) < config::minimum_drag_pixels)
		return;

	// if highlighted object is in selection, we drag the selection,
	// otherwise we drag just this one object.

	if (inst.edit.click_force_single || !inst.edit.Selected->get(inst.edit.clicked.num))
		inst.edit.dragged = inst.edit.clicked;
	else
		inst.edit.dragged.clear();

	DoBeginDrag(inst);
}

static void DoBeginDrag(Instance &inst)
{
	inst.edit.drag_start_x = inst.edit.drag_cur_x = inst.edit.click_map_x;
	inst.edit.drag_start_y = inst.edit.drag_cur_y = inst.edit.click_map_y;
	inst.edit.drag_start_z = inst.edit.drag_cur_z = inst.edit.click_map_z;

	inst.edit.drag_screen_dx  = inst.edit.drag_screen_dy = 0;
	inst.edit.drag_thing_num  = -1;
	inst.edit.drag_other_vert = -1;

	// the focus is only used when grid snapping is on
	inst.level.objects.getDragFocus(&inst.edit.drag_focus_x, &inst.edit.drag_focus_y, inst.edit.click_map_x, inst.edit.click_map_y);

	if (inst.edit.render3d)
	{
		if (inst.edit.mode == ObjType::sectors)
			inst.edit.drag_sector_dz = 0;

		if (inst.edit.mode == ObjType::things)
		{
			inst.edit.drag_thing_num = inst.edit.clicked.num;
			inst.edit.drag_thing_floorh = static_cast<float>(inst.edit.drag_start_z);
			inst.edit.drag_thing_up_down = (inst.Level_format != MapFormat::doom && !grid.snap);

			// get thing's floor
			if (inst.edit.drag_thing_num >= 0)
			{
				const Thing *T = inst.level.things[inst.edit.drag_thing_num];

				Objid sec = inst.level.hover.getNearbyObject(ObjType::sectors, T->x(), T->y());

				if (sec.valid())
					inst.edit.drag_thing_floorh = static_cast<float>(inst.level.sectors[sec.num]->floorh);
			}
		}
	}

	// in vertex mode, show all the connected lines too
	if (inst.edit.drag_lines)
	{
		delete inst.edit.drag_lines;
		inst.edit.drag_lines = NULL;
	}

	if (inst.edit.mode == ObjType::vertices)
	{
		inst.edit.drag_lines = new selection_c(ObjType::linedefs);
		ConvertSelection(inst.level, inst.edit.Selected, inst.edit.drag_lines);

		// find opposite end-point when dragging a single vertex
		if (inst.edit.dragged.valid())
			inst.edit.drag_other_vert = inst.level.vertmod.findDragOther(inst.edit.dragged.num);
	}

	inst.edit.clicked.clear();

	Editor_SetAction(inst, ACT_DRAG);

	inst.main_win->canvas->redraw();
}


void Instance::ACT_SelectBox_release()
{
	// check if cancelled or overridden
	if (edit.action != ACT_SELBOX)
		return;

	Editor_ClearAction();
	Editor_ClearErrorMode(*this);

	// a mere click and release will unselect everything
	double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	if (!main_win->canvas->SelboxGet(x1, y1, x2, y2))
	{
		ExecuteCommand("UnselectAll");
		return;
	}

	SelectObjectsInBox(level, edit.Selected, edit.mode, x1, y1, x2, y2);
	RedrawMap();
}


void Instance::ACT_Drag_release()
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
			Render3D_DragThings(*this);

		if (edit.mode == ObjType::sectors)
			Render3D_DragSectors(*this);
	}

	// note: DragDelta needs inst.edit.dragged
	double dx, dy;
	main_win->canvas->DragDelta(&dx, &dy);

	Objid dragged(edit.dragged);
	edit.dragged.clear();

	if (edit.drag_lines)
		edit.drag_lines->clear_all();

	if (! edit.render3d && (dx || dy))
	{
		if (dragged.valid())
			level.objects.singleDrag(dragged, dx, dy, 0);
		else
			level.objects.move(edit.Selected, dx, dy, 0);
	}

	Editor_ClearAction();

	RedrawMap();
}


void Instance::ACT_Click_release()
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
			near_obj = level.hover.getNearbyObject(edit.mode, edit.map_x, edit.map_y);

		if (near_obj.num == click_obj.num)
			Selection_Toggle(click_obj);
	}

	Editor_ClearAction();
	Editor_ClearErrorMode(*this);

	RedrawMap();
}

static void CMD_ACT_Click(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	// require a highlighted object in 3D mode
	if (inst.edit.render3d && inst.edit.highlight.is_nil())
		return;

	if (! inst.Nav_ActionKey(EXEC_CurKey, &Instance::ACT_Click_release))
		return;

	inst.edit.click_check_select = ! Exec_HasFlag("/noselect");
	inst.edit.click_check_drag   = ! Exec_HasFlag("/nodrag");
	inst.edit.click_force_single = false;

	// remember some state (for drag detection)
	inst.edit.click_screen_x = Fl::event_x();
	inst.edit.click_screen_y = Fl::event_y();

	inst.edit.click_map_x = inst.edit.map_x;
	inst.edit.click_map_y = inst.edit.map_y;
	inst.edit.click_map_z = inst.edit.map_z;

	// handle 3D mode, skip stuff below which only makes sense in 2D
	if (inst.edit.render3d)
	{
		if (inst.edit.highlight.type == ObjType::things)
		{
			const Thing *T = inst.level.things[inst.edit.highlight.num];
			inst.edit.drag_point_dist = static_cast<float>(r_view.DistToViewPlane(T->x(), T->y()));
		}
		else
		{
			inst.edit.drag_point_dist = static_cast<float>(r_view.DistToViewPlane(inst.edit.map_x, inst.edit.map_y));
		}

		inst.edit.clicked = inst.edit.highlight;
		Editor_SetAction(inst, ACT_CLICK);
		return;
	}

	// check for splitting a line, and ensure we can drag the vertex
	if (! Exec_HasFlag("/nosplit") &&
		inst.edit.mode == ObjType::vertices &&
		inst.edit.split_line.valid() &&
		inst.edit.action != ACT_DRAW_LINE)
	{
		int split_ld = inst.edit.split_line.num;

		inst.edit.click_force_single = true;   // if drag vertex, force single-obj mode
		inst.edit.click_check_select = false;  // do NOT select the new vertex

		// check if both ends are in selection, if so (and only then)
		// shall we select the new vertex
		const LineDef *L = inst.level.linedefs[split_ld];

		bool want_select = inst.edit.Selected->get(L->start) && inst.edit.Selected->get(L->end);

		inst.level.basis.begin();
		inst.level.basis.setMessage("split linedef #%d", split_ld);

		int new_vert = inst.level.basis.addNew(ObjType::vertices);

		Vertex *V = inst.level.vertices[new_vert];

		V->SetRawXY(inst, inst.edit.split_x, inst.edit.split_y);

		inst.level.linemod.splitLinedefAtVertex(split_ld, new_vert);

		inst.level.basis.end();

		if (want_select)
			inst.edit.Selected->set(new_vert);

		inst.edit.clicked = Objid(ObjType::vertices, new_vert);
		Editor_SetAction(inst, ACT_CLICK);

		inst.RedrawMap();
		return;
	}

	// find the object under the pointer.
	inst.edit.clicked = inst.level.hover.getNearbyObject(inst.edit.mode, inst.edit.map_x, inst.edit.map_y);

	// clicking on an empty space starts a new selection box
	if (inst.edit.click_check_select && inst.edit.clicked.is_nil())
	{
		inst.edit.selbox_x1 = inst.edit.selbox_x2 = inst.edit.map_x;
		inst.edit.selbox_y1 = inst.edit.selbox_y2 = inst.edit.map_y;

		Editor_SetAction(inst, ACT_SELBOX);
		return;
	}

	Editor_SetAction(inst, ACT_CLICK);
}


static void CMD_ACT_SelectBox(Instance &inst)
{
	if (inst.edit.render3d)
		return;

	if (! EXEC_CurKey)
		return;

	if (! inst.Nav_ActionKey(EXEC_CurKey, &Instance::ACT_SelectBox_release))
		return;

	inst.edit.selbox_x1 = inst.edit.selbox_x2 = inst.edit.map_x;
	inst.edit.selbox_y1 = inst.edit.selbox_y2 = inst.edit.map_y;

	Editor_SetAction(inst, ACT_SELBOX);
}


static void CMD_ACT_Drag(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (inst.edit.Selected->empty())
	{
		inst.Beep("Nothing to drag");
		return;
	}

	if (! inst.Nav_ActionKey(EXEC_CurKey, &Instance::ACT_Drag_release))
		return;

	// we only drag the selection, never a single object
	inst.edit.dragged.clear();

	DoBeginDrag(inst);
}


void Transform_Update(Instance &inst)
{
	double dx1 = inst.edit.map_x - inst.edit.trans_param.mid_x;
	double dy1 = inst.edit.map_y - inst.edit.trans_param.mid_y;

	double dx0 = inst.edit.trans_start_x - inst.edit.trans_param.mid_x;
	double dy0 = inst.edit.trans_start_y - inst.edit.trans_param.mid_y;

	inst.edit.trans_param.scale_x = inst.edit.trans_param.scale_y = 1;
	inst.edit.trans_param.skew_x  = inst.edit.trans_param.skew_y  = 0;
	inst.edit.trans_param.rotate  = 0;

	if (inst.edit.trans_mode == TRANS_K_Rotate || inst.edit.trans_mode == TRANS_K_RotScale)
	{
		double angle[2] = { atan2(dy1, dx1), atan2(dy0, dx0) };

		inst.edit.trans_param.rotate = angle[1] - angle[0];

//		fprintf(stderr, "angle diff : %1.2f\n", inst.edit.trans_rotate * 360.0 / 65536.0);
	}

	switch (inst.edit.trans_mode)
	{
		case TRANS_K_Scale:
		case TRANS_K_RotScale:
			dx1 = MAX(abs(dx1), abs(dy1));
			dx0 = MAX(abs(dx0), abs(dy0));

			if (dx0)
			{
				inst.edit.trans_param.scale_x = dx1 / (float)dx0;
				inst.edit.trans_param.scale_y = inst.edit.trans_param.scale_x;
			}
			break;

		case TRANS_K_Stretch:
			if (dx0) inst.edit.trans_param.scale_x = dx1 / (float)dx0;
			if (dy0) inst.edit.trans_param.scale_y = dy1 / (float)dy0;
			break;

		case TRANS_K_Rotate:
			// already done
			break;

		case TRANS_K_Skew:
			if (abs(dx0) >= abs(dy0))
			{
				if (dx0) inst.edit.trans_param.skew_y = (dy1 - dy0) / (float)dx0;
			}
			else
			{
				if (dy0) inst.edit.trans_param.skew_x = (dx1 - dx0) / (float)dy0;
			}
			break;
	}

	inst.main_win->canvas->redraw();
}


void Instance::ACT_Transform_release()
{
	// check if cancelled or overridden
	if (edit.action != ACT_TRANSFORM)
		return;

	if (edit.trans_lines)
		edit.trans_lines->clear_all();

	level.objects.transform(edit.trans_param);

	Editor_ClearAction();

	RedrawMap();
}

static void CMD_ACT_Transform(Instance &inst)
{
	if (inst.edit.render3d)
		return;

	if (! EXEC_CurKey)
		return;

	if (inst.edit.Selected->empty())
	{
		inst.Beep("Nothing to scale");
		return;
	}

	SString keyword = EXEC_Param[0];
	transform_keyword_e  mode;

	if (keyword.empty())
	{
		inst.Beep("ACT_Transform: missing keyword");
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
		inst.Beep("ACT_Transform: unknown keyword: %s", keyword.c_str());
		return;
	}


	if (! inst.Nav_ActionKey(EXEC_CurKey, &Instance::ACT_Transform_release))
		return;


	double middle_x, middle_y;
	inst.level.objects.calcMiddle(inst.edit.Selected, &middle_x, &middle_y);

	inst.edit.trans_mode = mode;
	inst.edit.trans_start_x = inst.edit.map_x;
	inst.edit.trans_start_y = inst.edit.map_y;

	inst.edit.trans_param.Clear();
	inst.edit.trans_param.mid_x = middle_x;
	inst.edit.trans_param.mid_y = middle_y;

	if (inst.edit.trans_lines)
	{
		delete inst.edit.trans_lines;
		inst.edit.trans_lines = NULL;
	}

	if (inst.edit.mode == ObjType::vertices)
	{
		inst.edit.trans_lines = new selection_c(ObjType::linedefs);
		ConvertSelection(inst.level, inst.edit.Selected, inst.edit.trans_lines);
	}

	Editor_SetAction(inst, ACT_TRANSFORM);
}


static void CMD_WHEEL_Scroll(Instance &inst)
{
	float speed = static_cast<float>(atof(EXEC_Param[0]));

	if (Exec_HasFlag("/LAX"))
	{
		keycode_t mod = Fl::event_state() & EMOD_ALL_MASK;

		if (mod & EMOD_SHIFT)
			speed /= 3.0;
		else if (mod & EMOD_COMMAND)
			speed *= 3.0;
	}

	float delta_x = static_cast<float>(wheel_dx);
	float delta_y = static_cast<float>(0 - wheel_dy);

	int base_size = (inst.main_win->canvas->w() + inst.main_win->canvas->h()) / 2;

	speed = static_cast<float>(speed * base_size / 100.0 / grid.Scale);

	grid.Scroll(delta_x * speed, delta_y * speed);
}


static void CMD_Merge(Instance &inst)
{
	switch (inst.edit.mode)
	{
		case ObjType::vertices:
			VertexModule::commandMerge(inst);
			break;

		case ObjType::linedefs:
			LinedefModule::commandMergeTwo(inst);
			break;

		case ObjType::sectors:
			SectorModule::commandMerge(inst);
			break;

		case ObjType::things:
			CMD_TH_Merge(inst);
			break;

		default:
			inst.Beep("Cannot merge that");
			break;
	}
}


static void CMD_Disconnect(Instance &inst)
{
	switch (inst.edit.mode)
	{
		case ObjType::vertices:
			VertexModule::commandDisconnect(inst);
			break;

		case ObjType::linedefs:
			VertexModule::commandLineDisconnect(inst);
			break;

		case ObjType::sectors:
			VertexModule::commandSectorDisconnect(inst);
			break;

		case ObjType::things:
			CMD_TH_Disconnect(inst);
			break;

		default:
			inst.Beep("Cannot disconnect that");
			break;
	}
}


static void CMD_Zoom(Instance &inst)
{
	int delta = atoi(EXEC_Param[0]);

	if (delta == 0)
	{
		inst.Beep("Zoom: bad or missing value");
		return;
	}

	int mid_x = static_cast<int>(inst.edit.map_x);
	int mid_y = static_cast<int>(inst.edit.map_y);

	if (Exec_HasFlag("/center"))
	{
		mid_x = I_ROUND(grid.orig_x);
		mid_y = I_ROUND(grid.orig_y);
	}

	Editor_Zoom(delta, mid_x, mid_y);
}


static void CMD_ZoomWholeMap(Instance &inst)
{
	if (inst.edit.render3d)
		Render3D_Enable(inst, false);

	inst.ZoomWholeMap();
}


static void CMD_ZoomSelection(Instance &inst)
{
	if (inst.edit.Selected->empty())
	{
		inst.Beep("No selection to zoom");
		return;
	}

	GoToSelection(inst);
}


static void CMD_GoToCamera(Instance &inst)
{
	if (inst.edit.render3d)
		Render3D_Enable(inst, false);

	double x, y; float angle;
	Render3D_GetCameraPos(&x, &y, &angle);

	grid.MoveTo(x, y);

	inst.RedrawMap();
}


static void CMD_PlaceCamera(Instance &inst)
{
	if (inst.edit.render3d)
	{
		inst.Beep("Not supported in 3D view");
		return;
	}

	if (! inst.edit.pointer_in_window)
	{
		// IDEA: turn cursor into cross, wait for click in map window

		inst.Beep("Mouse is not over map");
		return;
	}

	double x = inst.edit.map_x;
	double y = inst.edit.map_y;

	Render3D_SetCameraPos(x, y);

	if (Exec_HasFlag("/open3d"))
	{
		Render3D_Enable(inst, true);
	}

	inst.RedrawMap();
}


static void CMD_MoveObjects_Dialog(Instance &inst)
{
	SelectHighlight unselect = inst.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("Nothing to move");
		return;
	}

	bool want_dz = (inst.edit.mode == ObjType::sectors);
	// can move things vertically in Hexen/UDMF formats
	if (inst.edit.mode == ObjType::things && inst.Level_format != MapFormat::doom)
		want_dz = true;

	UI_MoveDialog * dialog = new UI_MoveDialog(inst, want_dz);

	dialog->Run();

	delete dialog;

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(inst, true /* nosave */);
}


static void CMD_ScaleObjects_Dialog(Instance &inst)
{
	SelectHighlight unselect = inst.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("Nothing to scale");
		return;
	}

	UI_ScaleDialog * dialog = new UI_ScaleDialog(inst);

	dialog->Run();

	delete dialog;

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(inst, true /* nosave */);
}


static void CMD_RotateObjects_Dialog(Instance &inst)
{
	SelectHighlight unselect = inst.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("Nothing to rotate");
		return;
	}

	UI_RotateDialog * dialog = new UI_RotateDialog(inst);

	dialog->Run();

	delete dialog;

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(inst, true /* nosave */);
}


static void CMD_GRID_Bump(Instance &inst)
{
	int delta = atoi(EXEC_Param[0]);

	delta = (delta >= 0) ? +1 : -1;

	grid.AdjustStep(delta);
}


static void CMD_GRID_Set(Instance &inst)
{
	int step = atoi(EXEC_Param[0]);

	if (step < 2 || step > 4096)
	{
		inst.Beep("Bad grid step");
		return;
	}

	grid.ForceStep(step);
}


static void CMD_GRID_Zoom(Instance &inst)
{
	// target scale is positive for NN:1 and negative for 1:NN

	double scale = atof(EXEC_Param[0]);

	if (scale == 0)
	{
		inst.Beep("Bad scale");
		return;
	}

	if (scale < 0)
		scale = -1.0 / scale;

	float S1 = static_cast<float>(grid.Scale);

	grid.NearestScale(scale);

	grid.RefocusZoom(inst.edit.map_x, inst.edit.map_y, S1);
}


static void CMD_BR_CycleCategory(Instance &inst)
{
	if (!inst.main_win->browser->visible())
	{
		inst.Beep("Browser not open");
		return;
	}

	int dir = (atoi(EXEC_Param[0]) >= 0) ? +1 : -1;

	inst.main_win->browser->CycleCategory(dir);
}


static void CMD_BR_ClearSearch(Instance &inst)
{
	if (!inst.main_win->browser->visible())
	{
		inst.Beep("Browser not open");
		return;
	}

	inst.main_win->browser->ClearSearchBox();
}


static void CMD_BR_Scroll(Instance &inst)
{
	if (!inst.main_win->browser->visible())
	{
		inst.Beep("Browser not open");
		return;
	}

	if (EXEC_Param[0].empty())
	{
		inst.Beep("BR_Scroll: missing value");
		return;
	}

	int delta = atoi(EXEC_Param[0]);

	inst.main_win->browser->Scroll(delta);
}


static void CMD_DefaultProps(Instance &inst)
{
	inst.main_win->ShowDefaultProps();
}


static void CMD_FindDialog(Instance &inst)
{
	inst.main_win->ShowFindAndReplace();
}


static void CMD_FindNext(Instance &inst)
{
	inst.main_win->find_box->FindNext();
}


static void CMD_RecalcSectors(Instance &inst)
{
	Subdiv_InvalidateAll();
	inst.RedrawMap();
}


static void CMD_LogViewer(Instance &inst)
{
	LogViewer_Open();
}


static void CMD_OnlineDocs(Instance &inst)
{
	int rv = fl_open_uri("http://eureka-editor.sourceforge.net/?n=Docs.Index");
	if (rv == 1)
		inst.Status_Set("Opened web browser");
	else
		inst.Beep("Failed to open web browser");
}


static void CMD_AboutDialog(Instance &inst)
{
	DLG_AboutText();
}


//------------------------------------------------------------------------

namespace global
{
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
		&ObjectsModule::commandInsert,
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
		&ObjectsModule::commandCopyProperties,
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
		&ObjectsModule::commandMirror,
		/* flags */ NULL,
		/* keywords */ "horiz vert"
	},

	{	"Rotate90",	"General",
		&ObjectsModule::commandRotate90,
		/* flags */ NULL,
		/* keywords */ "cw acw"
	},

	{	"Enlarge",	"General",
		&ObjectsModule::commandEnlarge
	},

	{	"Shrink",	"General",
		&ObjectsModule::commandShrink
	},

	{	"Quantize",	"General",
		&ObjectsModule::commandQuantize
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
}	// namespace global

void Editor_RegisterCommands()
{
	M_RegisterCommandList(global::command_table);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
