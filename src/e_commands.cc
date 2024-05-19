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
#include "LineDef.h"
#include "m_config.h"
#include "m_loadsave.h"
#include "m_vector.h"
#include "r_render.h"
#include "r_subdiv.h"
#include "Sector.h"
#include "Thing.h"
#include "ui_about.h"
#include "ui_misc.h"
#include "ui_prefs.h"
#include "Vertex.h"


// config items
int config::minimum_drag_pixels = 5;


void Instance::CMD_Nothing()
{
	/* hey jude, don't make it bad */
}


void Instance::CMD_MetaKey()
{
	if (edit.sticky_mod)
	{
		ClearStickyMod();
		return;
	}

	Status_Set("META...");

	edit.sticky_mod = EMOD_META;
}


void Instance::CMD_EditMode()
{
	char mode = static_cast<char>(tolower(EXEC_Param[0][0]));

	if (! mode || ! strchr("lstvr", mode))
	{
		Beep("Bad parameter for EditMode: '%s'", EXEC_Param[0].c_str());
		return;
	}

	Editor_ChangeMode(mode);
}


void Instance::CMD_Select()
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

	edit.Selection_Toggle(edit.highlight);
	RedrawMap();
}


void Instance::CMD_SelectAll()
{
	Editor_ClearErrorMode();

	int total = level.numObjects(edit.mode);

	Selection_Push();

	edit.Selected->change_type(edit.mode);
	edit.Selected->frob_range(0, total-1, BitOp::add);

	RedrawMap();
}


void Instance::CMD_UnselectAll()
{
	Editor_ClearErrorMode();

	if (edit.action == EditorAction::drawLine ||
		edit.action == EditorAction::transform)
	{
		Editor_ClearAction();
	}

	Selection_Clear();

	RedrawMap();
}

void Instance::CMD_InvertSelection()
{
	// do not clear selection when in error mode
	edit.error_mode = false;

	int total = level.numObjects(edit.mode);

	if (edit.Selected->what_type() != edit.mode)
	{
		// convert the selection
		selection_c prev_sel = *edit.Selected;
		edit.Selected.emplace(edit.mode, true /* extended */);

		ConvertSelection(level, prev_sel, *edit.Selected);
	}

	edit.Selected->frob_range(0, total-1, BitOp::toggle);

	RedrawMap();
}


void Instance::CMD_Quit()
{
	Main_Quit();
}


void Instance::CMD_Undo()
{
	if (! level.basis.undo())
	{
		Beep("No operation to undo");
		return;
	}

	RedrawMap();
	main_win->UpdatePanelObj();
}


void Instance::CMD_Redo()
{
	if (! level.basis.redo())
	{
		Beep("No operation to redo");
		return;
	}

	RedrawMap();
	main_win->UpdatePanelObj();
}


static void SetGamma(Instance &inst, int new_val)
{
	config::usegamma = clamp(0, new_val, 4);

	inst.wad.palette.updateGamma(config::usegamma, config::panel_gamma);

	// for OpenGL, need to reload all images
	if (inst.main_win && inst.main_win->canvas)
		inst.main_win->canvas->DeleteContext();

	inst.Status_Set("gamma level %d", config::usegamma);

	inst.RedrawMap();
}


void Instance::CMD_SetVar()
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
		Render3D_Enable(*this, bool_val);
	}
	else if (var_name.noCaseEqual("browser"))
	{
		Editor_ClearAction();

		int want_vis   = bool_val ? 1 : 0;
		int is_visible = main_win->browser->visible() ? 1 : 0;

		if (want_vis != is_visible)
			main_win->BrowserMode(BrowserMode::toggle);
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
		SetGamma(*this, int_val);
	}
	else if (var_name.noCaseEqual("ratio"))
		grid.configureRatio(clamp(0, int_val, 7), true);
	else if (var_name.noCaseEqual("sec_render"))
	{
		int_val = clamp(0, int_val, (int)SREND_SoundProp);
		edit.sector_render_mode = (sector_rendering_mode_e) int_val;

		if (edit.render3d)
			Render3D_Enable(*this, false);

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


void Instance::CMD_ToggleVar()
{
	SString var_name = EXEC_Param[0];

	if (var_name.empty())
	{
		Beep("Toggle: missing var name");
		return;
	}

	if (var_name.noCaseEqual("3d"))
	{
		Render3D_Enable(*this, ! edit.render3d);
	}
	else if (var_name.noCaseEqual("browser"))
	{
		Editor_ClearAction();

		main_win->BrowserMode(BrowserMode::toggle);
	}
	else if (var_name.noCaseEqual("recent"))
	{
		main_win->browser->ToggleRecent();
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
		SetGamma(*this, (config::usegamma >= 4) ? 0 : config::usegamma + 1);
	}
	else if (var_name.noCaseEqual("ratio"))
		grid.configureRatio(grid.getRatio() >= 7 ? 0 : grid.getRatio() + 1, true);
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


void Instance::CMD_BrowserMode()
{
	if (EXEC_Param[0].empty())
	{
		Beep("BrowserMode: missing mode");
		return;
	}

	char modeChar = static_cast<char>(toupper(EXEC_Param[0][0]));

	if (!charMapsToSpecificBrowserMode(modeChar))
	{
		Beep("Unknown browser mode: %s", EXEC_Param[0].c_str());
		return;
	}
	BrowserMode mode = charToBrowserMode(modeChar);

	// if that browser is already open, close it now
	if (main_win->browser->visible() &&
		main_win->browser->GetMode() == mode &&
		! Exec_HasFlag("/force") &&
		! Exec_HasFlag("/recent"))
	{
		main_win->BrowserMode(BrowserMode::hide);
		return;
	}

	main_win->BrowserMode(mode);

	if (Exec_HasFlag("/recent"))
	{
		main_win->browser->ToggleRecent(true /* force */);
	}
}


void Instance::CMD_Scroll()
{
	// these are percentages
	v2double_t delta = { atof(EXEC_Param[0]), atof(EXEC_Param[1]) };

	if (!delta)
	{
		Beep("Bad parameter to Scroll: '%s' %s'", EXEC_Param[0].c_str(), EXEC_Param[1].c_str());
		return;
	}

	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;

	delta *= base_size / 100.0 / grid.getScale();

	grid.Scroll(delta);
}


void Instance::NAV_Scroll_Left_release()
{
	edit.nav.left = 0;
}

//
// Scroll in any direction as dictated by CMD_Nav_Scroll_*
//
void Instance::navigationScroll(float *editNav, nav_release_func_t func)
{
	if(!EXEC_CurKey)
		return;

	if(!edit.is_navigating)
		edit.clearNav();

	float perc = static_cast<float>(atof(EXEC_Param[0]));
	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;
	*editNav = static_cast<float>(perc * base_size / 100.0 / grid.getScale());

	Nav_SetKey(EXEC_CurKey, func);
}

void Instance::CMD_NAV_Scroll_Left()
{
	navigationScroll(&edit.nav.left, &Instance::NAV_Scroll_Left_release);
}

void Instance::NAV_Scroll_Right_release()
{
	edit.nav.right = 0;
}

void Instance::CMD_NAV_Scroll_Right()
{
	navigationScroll(&edit.nav.right, &Instance::NAV_Scroll_Right_release);
}


void Instance::NAV_Scroll_Up_release()
{
	edit.nav.up = 0;
}

void Instance::CMD_NAV_Scroll_Up()
{
	navigationScroll(&edit.nav.up, &Instance::NAV_Scroll_Up_release);
}


void Instance::NAV_Scroll_Down_release()
{
	edit.nav.down = 0;
}

void Instance::CMD_NAV_Scroll_Down()
{
	navigationScroll(&edit.nav.down, &Instance::NAV_Scroll_Down_release);
}


void Instance::NAV_MouseScroll_release()
{
	Editor_ScrollMap(+1);
}

void Instance::CMD_NAV_MouseScroll()
{
	if (! EXEC_CurKey)
		return;

	edit.panning_speed = static_cast<float>(atof(EXEC_Param[0]));
	edit.panning_lax = Exec_HasFlag("/LAX");

	if (! edit.is_navigating)
		edit.clearNav();

	if (Nav_SetKey(EXEC_CurKey, &Instance::NAV_MouseScroll_release))
	{
		// begin
		Editor_ScrollMap(-1);
	}
}

void Instance::CheckBeginDrag()
{
	if (! edit.clicked.valid())
		return;

	if (! edit.click_check_drag)
		return;

	// can drag things and sector planes in 3D mode
	if (edit.render3d && !(edit.mode == ObjType::things || edit.mode == ObjType::sectors))
		return;

	v2int_t pixel_dpos = {
		Fl::event_x() - edit.click_screen_pos.x,
		Fl::event_y() - edit.click_screen_pos.y
	};

	if (pixel_dpos.chebyshev() < config::minimum_drag_pixels)
		return;

	// if highlighted object is in selection, we drag the selection,
	// otherwise we drag just this one object.

	if (edit.click_force_single || !edit.Selected->get(edit.clicked.num))
		edit.dragged = edit.clicked;
	else
		edit.dragged.clear();

	DoBeginDrag();
}

void Instance::DoBeginDrag()
{
	edit.drag_start = edit.drag_cur = edit.click_map;

	edit.drag_screen_dpos = {};
	edit.drag_thing_num  = -1;
	edit.drag_other_vert = -1;

	// the focus is only used when grid snapping is on
	edit.drag_focus.xy = level.objects.getDragFocus(edit.click_map.xy);

	if (edit.render3d)
	{
		if (edit.mode == ObjType::sectors)
			edit.drag_sector_dz = 0;

		if (edit.mode == ObjType::things)
		{
			edit.drag_thing_num = edit.clicked.num;
			edit.drag_thing_floorh = static_cast<float>(edit.drag_start.z);
			edit.drag_thing_up_down = (loaded.levelFormat != MapFormat::doom && !grid.snaps());

			// get thing's floor
			if (edit.drag_thing_num >= 0)
			{
				const auto T = level.things[edit.drag_thing_num];

				Objid sec = hover::getNearestSector(level, T->xy());

				if (sec.valid())
					edit.drag_thing_floorh = static_cast<float>(level.sectors[sec.num]->floorh);
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
		ConvertSelection(level, *edit.Selected, *edit.drag_lines);

		// find opposite end-point when dragging a single vertex
		if (edit.dragged.valid())
			edit.drag_other_vert = level.vertmod.findDragOther(edit.dragged.num);
	}

	edit.clicked.clear();

	Editor_SetAction(EditorAction::drag);

	main_win->canvas->redraw();
}


void Instance::ACT_SelectBox_release()
{
	// check if cancelled or overridden
	if (edit.action != EditorAction::selbox)
		return;

	Editor_ClearAction();
	Editor_ClearErrorMode();

	// a mere click and release will unselect everything
	v2double_t pos1 = {};
	v2double_t pos2 = {};
	if (!main_win->canvas->SelboxGet(pos1, pos2))
	{
		ExecuteCommand("UnselectAll");
		return;
	}

	SelectObjectsInBox(level, &*edit.Selected, edit.mode, pos1, pos2);
	RedrawMap();
}


void Instance::ACT_Drag_release()
{
	// check if cancelled or overridden
	if (edit.action != EditorAction::drag)
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
	v2double_t delta = main_win->canvas->DragDelta();

	Objid dragged(edit.dragged);
	edit.dragged.clear();

	if (edit.drag_lines)
		edit.drag_lines->clear_all();

	if (! edit.render3d && delta.nonzero())
	{
		if (dragged.valid())
			level.objects.singleDrag(dragged, v3double_t(delta));
		else
			level.objects.move(*edit.Selected, v3double_t(delta));
	}

	Editor_ClearAction();

	RedrawMap();
}


void Instance::ACT_Click_release()
{
	Objid click_obj(edit.clicked);
	edit.clicked.clear();

	edit.click_check_drag = false;


	if (edit.action == EditorAction::selbox)
	{
		ACT_SelectBox_release();
		return;
	}
	else if (edit.action == EditorAction::drag)
	{
		ACT_Drag_release();
		return;
	}

	// check if cancelled or overridden
	if (edit.action != EditorAction::click)
		return;

	if (edit.click_check_select && click_obj.valid())
	{
		// only toggle selection if it's the same object as before
		Objid near_obj;
		if (edit.render3d)
			near_obj = edit.highlight;
		else
			near_obj = hover::getNearbyObject(edit.mode, level, conf, grid, edit.map.xy);

		if (near_obj.num == click_obj.num)
			edit.Selection_Toggle(click_obj);
	}

	Editor_ClearAction();
	Editor_ClearErrorMode();

	RedrawMap();
}

void Instance::CMD_ACT_Click()
{
	if (! EXEC_CurKey)
		return;

	// require a highlighted object in 3D mode
	if (edit.render3d && edit.highlight.is_nil())
		return;

	if (! Nav_ActionKey(EXEC_CurKey, &Instance::ACT_Click_release))
		return;

	edit.click_check_select = ! Exec_HasFlag("/noselect");
	edit.click_check_drag   = ! Exec_HasFlag("/nodrag");
	edit.click_force_single = false;

	// remember some state (for drag detection)
	edit.click_screen_pos.x = Fl::event_x();
	edit.click_screen_pos.y = Fl::event_y();

	edit.click_map = edit.map;

	// handle 3D mode, skip stuff below which only makes sense in 2D
	if (edit.render3d)
	{
		if (edit.highlight.type == ObjType::things)
		{
			const auto T = level.things[edit.highlight.num];
			edit.drag_point_dist = static_cast<float>(r_view.DistToViewPlane(T->xy()));
		}
		else
		{
			edit.drag_point_dist = static_cast<float>(r_view.DistToViewPlane(edit.map.xy));
		}

		edit.clicked = edit.highlight;
		Editor_SetAction(EditorAction::click);
		return;
	}

	// check for splitting a line, and ensure we can drag the vertex
	if (! Exec_HasFlag("/nosplit") &&
		edit.mode == ObjType::vertices &&
		edit.split_line.valid() &&
		edit.action != EditorAction::drawLine)
	{
		int split_ld = edit.split_line.num;

		edit.click_force_single = true;   // if drag vertex, force single-obj mode
		edit.click_check_select = false;  // do NOT select the new vertex

		// check if both ends are in selection, if so (and only then)
		// shall we select the new vertex
		const auto L = level.linedefs[split_ld];

		bool want_select = edit.Selected->get(L->start) && edit.Selected->get(L->end);
		int new_vert;
		{
			EditOperation op(level.basis);
			op.setMessage("split linedef #%d", split_ld);

			new_vert = op.addNew(ObjType::vertices);

			auto V = level.vertices[new_vert];

			V->SetRawXY(loaded.levelFormat, edit.split);

			level.linemod.splitLinedefAtVertex(op, split_ld, new_vert);
		}

		if (want_select)
			edit.Selected->set(new_vert);

		edit.clicked = Objid(ObjType::vertices, new_vert);
		Editor_SetAction(EditorAction::click);

		RedrawMap();
		return;
	}

	// find the object under the pointer.
	edit.clicked = hover::getNearbyObject(edit.mode, level, conf, grid, edit.map.xy);

	// clicking on an empty space starts a new selection box
	if (edit.click_check_select && edit.clicked.is_nil())
	{
		edit.selbox1 = edit.selbox2 = edit.map.xy;

		Editor_SetAction(EditorAction::selbox);
		return;
	}

	Editor_SetAction(EditorAction::click);
}


void Instance::CMD_ACT_SelectBox()
{
	if (edit.render3d)
		return;

	if (! EXEC_CurKey)
		return;

	if (! Nav_ActionKey(EXEC_CurKey, &Instance::ACT_SelectBox_release))
		return;

	edit.selbox1 = edit.selbox2 = edit.map.xy;

	Editor_SetAction(EditorAction::selbox);
}


void Instance::CMD_ACT_Drag()
{
	if (! EXEC_CurKey)
		return;

	if (edit.Selected->empty())
	{
		Beep("Nothing to drag");
		return;
	}

	if (! Nav_ActionKey(EXEC_CurKey, &Instance::ACT_Drag_release))
		return;

	// we only drag the selection, never a single object
	edit.dragged.clear();

	DoBeginDrag();
}


void Instance::Transform_Update()
{
	v2double_t dv1 = edit.map.xy - edit.trans_param.mid;
	v2double_t dv0 = edit.trans_start - edit.trans_param.mid;

	edit.trans_param.scale = { 1.0, 1.0 };
	edit.trans_param.skew = {};
	edit.trans_param.rotate  = 0;

	if (edit.trans_mode == TRANS_K_Rotate || edit.trans_mode == TRANS_K_RotScale)
	{
		double angle[2] = { dv1.atan2(), dv0.atan2() };

		edit.trans_param.rotate = angle[1] - angle[0];

//		fprintf(stderr, "angle diff : %1.2f\n", inst.edit.trans_rotate * 360.0 / 65536.0);
	}

	switch (edit.trans_mode)
	{
		case TRANS_K_Scale:
		case TRANS_K_RotScale:
			dv1.x = std::max(fabs(dv1.x), fabs(dv1.y));
			dv0.x = std::max(fabs(dv0.x), fabs(dv0.y));

			if (dv0.x)
			{
				edit.trans_param.scale.x = dv1.x / dv0.x;
				edit.trans_param.scale.y = edit.trans_param.scale.x;
			}
			break;

		case TRANS_K_Stretch:
			if (dv0.x)
				edit.trans_param.scale.x = dv1.x / dv0.x;
			if (dv0.y)
				edit.trans_param.scale.y = dv1.y / dv0.y;
			break;

		case TRANS_K_Rotate:
			// already done
			break;

		case TRANS_K_Skew:
			if (fabs(dv0.x) >= fabs(dv0.y))
			{
				if (dv0.x)
					edit.trans_param.skew.y = (dv1.y - dv0.y) / (float)dv0.x;
			}
			else
			{
				if (dv0.y)
					edit.trans_param.skew.x = (dv1.x - dv0.x) / (float)dv0.y;
			}
			break;
	}

	main_win->canvas->redraw();
}


void Instance::ACT_Transform_release()
{
	// check if cancelled or overridden
	if (edit.action != EditorAction::transform)
		return;

	if (edit.trans_lines)
		edit.trans_lines->clear_all();

	level.objects.transform(edit.trans_param);

	Editor_ClearAction();

	RedrawMap();
}

void Instance::CMD_ACT_Transform()
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


	if (! Nav_ActionKey(EXEC_CurKey, &Instance::ACT_Transform_release))
		return;


	v2double_t middle = level.objects.calcMiddle(*edit.Selected);

	edit.trans_mode = mode;
	edit.trans_start = edit.map.xy;

	edit.trans_param.Clear();
	edit.trans_param.mid = middle;

	if (edit.trans_lines)
	{
		delete edit.trans_lines;
		edit.trans_lines = NULL;
	}

	if (edit.mode == ObjType::vertices)
	{
		edit.trans_lines = new selection_c(ObjType::linedefs);
		ConvertSelection(level, *edit.Selected, *edit.trans_lines);
	}

	Editor_SetAction(EditorAction::transform);
}


void Instance::CMD_WHEEL_Scroll()
{
	float speed = static_cast<float>(atof(EXEC_Param[0]));

	if (Exec_HasFlag("/LAX"))
	{
		keycode_t mod = Fl::event_state() & EMOD_ALL_MASK;

		if (mod & EMOD_SHIFT)
			speed /= 3.0f;
		else if (mod & EMOD_COMMAND)
			speed *= 3.0f;
	}

	v2double_t delta = {
		static_cast<double>(wheel_dpos.x),
		static_cast<double>(-wheel_dpos.y)
	};

	int base_size = (main_win->canvas->w() + main_win->canvas->h()) / 2;

	speed = static_cast<float>(speed * base_size / 100.0 / grid.getScale());

	grid.Scroll(delta * speed);
}


void Instance::CMD_Merge()
{
	switch (edit.mode)
	{
		case ObjType::vertices:
			commandVertexMerge();
			break;

		case ObjType::linedefs:
			commandLinedefMergeTwo();
			break;

		case ObjType::sectors:
			commandSectorMerge();
			break;

		case ObjType::things:
			CMD_TH_Merge(*this);
			break;

		default:
			Beep("Cannot merge that");
			break;
	}
}


void Instance::CMD_Disconnect()
{
	switch (edit.mode)
	{
		case ObjType::vertices:
			commandVertexDisconnect();
			break;

		case ObjType::linedefs:
			commandLineDisconnect();
			break;

		case ObjType::sectors:
			commandSectorDisconnect();
			break;

		case ObjType::things:
			CMD_TH_Disconnect(*this);
			break;

		default:
			Beep("Cannot disconnect that");
			break;
	}
}


void Instance::CMD_Zoom()
{
	int delta = atoi(EXEC_Param[0]);

	if (delta == 0)
	{
		Beep("Zoom: bad or missing value");
		return;
	}

	auto mid = v2int_t(edit.map.xy);

	if (Exec_HasFlag("/center"))
		mid = grid.getOrig().iround();

	Editor_Zoom(delta, mid);
}


void Instance::CMD_ZoomWholeMap()
{
	if (edit.render3d)
		Render3D_Enable(*this, false);

	ZoomWholeMap();
}


void Instance::CMD_ZoomSelection()
{
	if (edit.Selected->empty())
	{
		Beep("No selection to zoom");
		return;
	}

	GoToSelection();
}


void Instance::CMD_GoToCamera()
{
	if (edit.render3d)
		Render3D_Enable(*this, false);

	v2double_t pos;
	float angle;
	Render3D_GetCameraPos(pos, &angle);

	grid.MoveTo(pos);

	RedrawMap();
}


void Instance::CMD_PlaceCamera()
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

	Render3D_SetCameraPos(edit.map.xy);

	if (Exec_HasFlag("/open3d"))
	{
		Render3D_Enable(*this, true);
	}

	RedrawMap();
}


void Instance::CMD_MoveObjects_Dialog()
{
	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("Nothing to move");
		return;
	}

	bool want_dz = (edit.mode == ObjType::sectors);
	// can move things vertically in Hexen/UDMF formats
	if (edit.mode == ObjType::things && loaded.levelFormat != MapFormat::doom)
		want_dz = true;

	UI_MoveDialog * dialog = new UI_MoveDialog(*this, want_dz);

	dialog->Run();

	delete dialog;

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void Instance::CMD_ScaleObjects_Dialog()
{
	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("Nothing to scale");
		return;
	}

	UI_ScaleDialog * dialog = new UI_ScaleDialog(*this);

	dialog->Run();

	delete dialog;

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void Instance::CMD_RotateObjects_Dialog()
{
	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("Nothing to rotate");
		return;
	}

	UI_RotateDialog * dialog = new UI_RotateDialog(*this);

	dialog->Run();

	delete dialog;

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


void Instance::CMD_GRID_Bump()
{
	int delta = atoi(EXEC_Param[0]);

	delta = (delta >= 0) ? +1 : -1;

	grid.AdjustStep(delta);
}


void Instance::CMD_GRID_Set()
{
	int step = atoi(EXEC_Param[0]);

	if (step < 2 || step > 4096)
	{
		Beep("Bad grid step");
		return;
	}

	grid.ForceStep(step);
}


void Instance::CMD_GRID_Zoom()
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

	float S1 = static_cast<float>(grid.getScale());

	grid.NearestScale(scale);

	grid.RefocusZoom(edit.map.xy, S1);
}


void Instance::CMD_BR_CycleCategory()
{
	if (!main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	int dir = (atoi(EXEC_Param[0]) >= 0) ? +1 : -1;

	main_win->browser->CycleCategory(dir);
}


void Instance::CMD_BR_ClearSearch()
{
	if (!main_win->browser->visible())
	{
		Beep("Browser not open");
		return;
	}

	main_win->browser->ClearSearchBox();
}


void Instance::CMD_BR_Scroll()
{
	if (!main_win->browser->visible())
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

	main_win->browser->Scroll(delta);
}


void Instance::CMD_DefaultProps()
{
	main_win->ShowDefaultProps();
}


void Instance::CMD_FindDialog()
{
	main_win->ShowFindAndReplace();
}


void Instance::CMD_FindNext()
{
	main_win->find_box->FindNext();
}


void Instance::CMD_RecalcSectors()
{
	Subdiv_InvalidateAll();
	RedrawMap();
}


void Instance::CMD_LogViewer()
{
	LogViewer_Open();
}


void Instance::CMD_OnlineDocs()
{
	int rv = fl_open_uri("http://eureka-editor.sourceforge.net/?n=Docs.Index");
	if (rv == 1)
		Status_Set("Opened web browser");
	else
		Beep("Failed to open web browser");
}


void Instance::CMD_AboutDialog()
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
		&Instance::CMD_Nothing
	},

	{	"Set", "Misc",
		&Instance::CMD_SetVar,
		/* flags */ NULL,
		/* keywords */ "3d browser gamma grid obj_nums ratio sec_render snap sprites"
	},

	{	"Toggle", "Misc",
		&Instance::CMD_ToggleVar,
		/* flags */ NULL,
		/* keywords */ "3d browser gamma grid obj_nums ratio sec_render snap recent sprites"
	},

	{	"MetaKey", "Misc",
		&Instance::CMD_MetaKey
	},

	{	"EditMode", "Misc",
		&Instance::CMD_EditMode,
		/* flags */ NULL,
		/* keywords */ "thing line sector vertex"
	},

	{	"OpMenu",  "Misc",
		&Instance::CMD_OperationMenu
	},

	{	"MapCheck", "Misc",
		&Instance::CMD_MapCheck,
		/* flags */ NULL,
		/* keywords */ "all major vertices sectors linedefs things textures tags current"
	},


	/* ----- 2D canvas ----- */

	{	"Scroll",  "2D View",
		&Instance::CMD_Scroll
	},

	{	"GRID_Bump",  "2D View",
		&Instance::CMD_GRID_Bump
	},

	{	"GRID_Set",  "2D View",
		&Instance::CMD_GRID_Set
	},

	{	"GRID_Zoom",  "2D View",
		&Instance::CMD_GRID_Zoom
	},

	{	"ACT_SelectBox", "2D View",
		&Instance::CMD_ACT_SelectBox
	},

	{	"WHEEL_Scroll",  "2D View",
		&Instance::CMD_WHEEL_Scroll
	},

	{	"NAV_Scroll_Left",  "2D View",
		&Instance::CMD_NAV_Scroll_Left
	},

	{	"NAV_Scroll_Right",  "2D View",
		&Instance::CMD_NAV_Scroll_Right
	},

	{	"NAV_Scroll_Up",  "2D View",
		&Instance::CMD_NAV_Scroll_Up
	},

	{	"NAV_Scroll_Down",  "2D View",
		&Instance::CMD_NAV_Scroll_Down
	},

	{	"NAV_MouseScroll", "2D View",
		&Instance::CMD_NAV_MouseScroll
	},


	/* ----- FILE menu ----- */

	{	"NewProject",  "File",
		&Instance::CMD_NewProject
	},

	{	"ManageProject",  "File",
		&Instance::CMD_ManageProject
	},

	{	"OpenMap",  "File",
		&Instance::CMD_OpenMap
	},

	{	"GivenFile",  "File",
		&Instance::CMD_GivenFile,
		/* flags */ NULL,
		/* keywords */ "next prev first last current"
	},

	{	"FlipMap",  "File",
		&Instance::CMD_FlipMap,
		/* flags */ NULL,
		/* keywords */ "next prev first last"
	},

	{	"SaveMap",  "File",
		&Instance::CMD_SaveMap
	},

	{	"ExportMap",  "File",
		&Instance::CMD_ExportMap
	},

	{	"FreshMap",  "File",
		&Instance::CMD_FreshMap
	},

	{	"CopyMap",  "File",
		&Instance::CMD_CopyMap
	},

	{	"RenameMap",  "File",
		&Instance::CMD_RenameMap
	},

	{	"DeleteMap",  "File",
		&Instance::CMD_DeleteMap
	},

	{	"Quit",  "File",
		&Instance::CMD_Quit
	},


	/* ----- EDIT menu ----- */

	{	"Undo",   "Edit",
		&Instance::CMD_Undo
	},

	{	"Redo",   "Edit",
		&Instance::CMD_Redo
	},

	{	"Insert",	"Edit",
		&Instance::CMD_ObjectInsert,
		/* flags */ "/continue /nofill"
	},

	{	"Delete",	"Edit",
		&Instance::CMD_Delete,
		/* flags */ "/keep"
	},

	{	"Clipboard_Cut",   "Edit",
		&Instance::CMD_Clipboard_Cut
	},

	{	"Clipboard_Copy",   "Edit",
		&Instance::CMD_Clipboard_Copy
	},

	{	"Clipboard_Paste",   "Edit",
		&Instance::CMD_Clipboard_Paste
	},

	{	"Select",	"Edit",
		&Instance::CMD_Select
	},

	{	"SelectAll",	"Edit",
		&Instance::CMD_SelectAll
	},

	{	"UnselectAll",	"Edit",
		&Instance::CMD_UnselectAll
	},
	
	{
		"SelectNeighbors", "Edit",
		&Instance::CMD_SelectNeighbors,
		/* flags */ NULL,
		/* keywords */ "height texture"
	},

	{	"InvertSelection",	"Edit",
		&Instance::CMD_InvertSelection
	},

	{	"LastSelection",	"Edit",
		&Instance::CMD_LastSelection
	},

	{	"CopyAndPaste",   "Edit",
		&Instance::CMD_CopyAndPaste
	},

	{	"CopyProperties",   "Edit",
		&Instance::CMD_CopyProperties,
		/* flags */ "/reverse"
	},

	{	"PruneUnused",   "Edit",
		&Instance::CMD_PruneUnused
	},

	{	"MoveObjectsDialog",   "Edit",
		&Instance::CMD_MoveObjects_Dialog
	},

	{	"ScaleObjectsDialog",   "Edit",
		&Instance::CMD_ScaleObjects_Dialog
	},

	{	"RotateObjectsDialog",   "Edit",
		&Instance::CMD_RotateObjects_Dialog
	},


	/* ----- VIEW menu ----- */

	{	"Zoom",  "View",
		&Instance::CMD_Zoom,
		/* flags */ "/center"
	},

	{	"ZoomWholeMap",  "View",
		&Instance::CMD_ZoomWholeMap
	},

	{	"ZoomSelection",  "View",
		&Instance::CMD_ZoomSelection
	},

	{	"DefaultProps",  "View",
		&Instance::CMD_DefaultProps
	},

	{	"FindDialog",  "View",
		&Instance::CMD_FindDialog
	},

	{	"FindNext",  "View",
		&Instance::CMD_FindNext
	},

	{	"GoToCamera",  "View",
		&Instance::CMD_GoToCamera
	},

	{	"PlaceCamera",  "View",
		&Instance::CMD_PlaceCamera,
		/* flags */ "/open3d"
	},

	{	"JumpToObject",  "View",
		&Instance::CMD_JumpToObject
	},


	/* ------ TOOLS menu ------ */

	{	"PreferenceDialog",  "Tools",
		&Instance::CMD_Preferences
	},

	{	"TestMap",  "Tools",
		&Instance::CMD_TestMap
	},

	{	"ChangeTestSettings", "Tools",
		&Instance::CMD_ChangeTestSettings
	},

	{	"RecalcSectors",  "Tools",
		&Instance::CMD_RecalcSectors
	},

	{	"BuildAllNodes",  "Tools",
		&Instance::CMD_BuildAllNodes
	},

	{	"EditLump",  "Tools",
		&Instance::CMD_EditLump,
		/* flags */ "/header /scripts"
	},

	{	"AddBehavior",  "Tools",
		&Instance::CMD_AddBehaviorLump
	},

	{	"LogViewer",  "Tools",
		&Instance::CMD_LogViewer
	},


	/* ------ HELP menu ------ */

	{	"OnlineDocs",  "Help",
		&Instance::CMD_OnlineDocs
	},

	{	"AboutDialog",  "Help",
		&Instance::CMD_AboutDialog
	},


	/* ----- general operations ----- */

	{	"Merge",	"General",
		&Instance::CMD_Merge,
		/* flags */ "/keep"
	},

	{	"Disconnect",	"General",
		&Instance::CMD_Disconnect
	},

	{	"Mirror",	"General",
		&Instance::CMD_Mirror,
		/* flags */ NULL,
		/* keywords */ "horiz vert"
	},

	{	"Rotate90",	"General",
		&Instance::CMD_Rotate90,
		/* flags */ NULL,
		/* keywords */ "cw acw"
	},

	{	"Enlarge",	"General",
		&Instance::CMD_Enlarge
	},

	{	"Shrink",	"General",
		&Instance::CMD_Shrink
	},

	{	"Quantize",	"General",
		&Instance::CMD_Quantize
	},

	{	"ApplyTag",	"General",
		&Instance::CMD_ApplyTag,
		/* flags */ NULL,
		/* keywords */ "fresh last"
	},

	{	"ACT_Click", "General",
		&Instance::CMD_ACT_Click,
		/* flags */ "/noselect /nodrag /nosplit"
	},

	{	"ACT_Drag", "General",
		&Instance::CMD_ACT_Drag
	},

	{	"ACT_Transform", "General",
		&Instance::CMD_ACT_Transform,
		/* flags */ NULL,
		/* keywords */ "scale stretch rotate rotscale skew"
	},


	/* ------ LineDef mode ------ */

	{	"LIN_Align", NULL,
		&Instance::CMD_LIN_Align,
		/* flags */ "/x /y /right /clear"
	},

	{	"LIN_Flip", NULL,
		&Instance::CMD_LIN_Flip,
		/* flags */ "/force"
	},

	{	"LIN_SwapSides", NULL,
		&Instance::CMD_LIN_SwapSides
	},

	{	"LIN_SplitHalf", NULL,
		&Instance::CMD_LIN_SplitHalf
	},

	{	"LIN_SelectPath", NULL,
		&Instance::CMD_LIN_SelectPath,
		/* flags */ "/fresh /onesided /sametex"
	},


	/* ------ Sector mode ------ */

	{	"SEC_Floor", NULL,
		&Instance::CMD_SEC_Floor
	},

	{	"SEC_Ceil", NULL,
		&Instance::CMD_SEC_Ceil
	},

	{	"SEC_Light", NULL,
		&Instance::CMD_SEC_Light
	},

	{	"SEC_SelectGroup", NULL,
		&Instance::CMD_SEC_SelectGroup,
		/* flags */ "/fresh /can_walk /doors /floor_h /floor_tex /ceil_h /ceil_tex /light /tag /special"
	},

	{	"SEC_SwapFlats", NULL,
		&Instance::CMD_SEC_SwapFlats
	},


	/* ------ Thing mode ------ */

	{	"TH_Spin", NULL,
		&Instance::CMD_TH_SpinThings
	},


	/* ------ Vertex mode ------ */

	{	"VT_ShapeLine", NULL,
		&Instance::CMD_VT_ShapeLine
	},

	{	"VT_ShapeArc", NULL,
		&Instance::CMD_VT_ShapeArc
	},


	/* -------- Browser -------- */

	{	"BrowserMode", "Browser",
		&Instance::CMD_BrowserMode,
		/* flags */ "/recent",
		/* keywords */ "obj tex flat line sec genline"
	},

	{	"BR_CycleCategory", "Browser",
		&Instance::CMD_BR_CycleCategory
	},

	{	"BR_ClearSearch", "Browser",
		&Instance::CMD_BR_ClearSearch
	},

	{	"BR_Scroll", "Browser",
		&Instance::CMD_BR_Scroll
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
