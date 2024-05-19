//------------------------------------------------------------------------
//  LEVEL MISC STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include "LineDef.h"
#include "m_bitvec.h"
#include "m_config.h"
#include "m_game.h"
#include "e_hover.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_path.h"
#include "e_things.h"
#include "e_vertex.h"
#include "r_render.h"
#include "r_subdiv.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"
#include "w_rawdef.h"
#include "WadData.h"

#include "ui_window.h"

#include <algorithm>

// config items
int config::default_edit_mode = 3;  // Vertices

bool config::same_mode_clears_selection = false;

int config::sector_render_default = (int)SREND_Floor;
int  config::thing_render_default = 1;


//
// adjust zoom factor to make whole map fit in window
//
void Instance::zoom_fit()
{
	if (level.numVertices() == 0)
		return;

	v2double_t zoom = { 1, 1 };
	v2int_t ScrMax = { main_win->canvas->w(), main_win->canvas->h() };

	if (level.Map_bound1.x < level.Map_bound2.x)
		zoom.x = ScrMax.x / (level.Map_bound2.x - level.Map_bound1.x);

	if (level.Map_bound1.y < level.Map_bound2.y)
		zoom.y = ScrMax.y / (level.Map_bound2.y - level.Map_bound1.y);

	grid.NearestScale(std::min(zoom.x, zoom.y));

	grid.MoveTo((level.Map_bound1 + level.Map_bound2) / 2);
}


void Instance::ZoomWholeMap()
{
	if (level.MadeChanges)
		level.CalculateLevelBounds();

	zoom_fit();

	RedrawMap();
}


void Instance::RedrawMap()
{
	if (!main_win)
		return;

	UpdateHighlight();

	main_win->scroll->UpdateRenderMode();
	main_win->info_bar->UpdateSecRend();
	main_win->status_bar->redraw();
	main_win->canvas->redraw();
}

static int Selection_FirstLine(const Document &doc, const selection_c &list);

static void UpdatePanel(const Instance &inst)
{
	// -AJA- I think the highlighted object is always the same type as
	//       the current editing mode.  But do this check for safety.
	if (inst.edit.highlight.valid() &&
		inst.edit.highlight.type != inst.edit.mode)
		return;


	// Choose object to show in right-hand panel:
	//   - the highlighted object takes priority
	//   - otherwise show the selection (first + count)
	//
	// It's a little more complicated since highlight may or may not
	// be part of the selection.

	int obj_idx   = inst.edit.highlight.num;
	int obj_count = inst.edit.Selected->count_obj();

	// the highlight is usually turned off when dragging, so compensate
	if (obj_idx < 0 && inst.edit.action == EditorAction::drag)
		obj_idx = inst.edit.dragged.num;

	if (obj_idx >= 0)
	{
		if (! inst.edit.Selected->get(obj_idx))
			obj_count = 0;
	}
	else if (obj_count > 0)
	{
		// in linedef mode, we prefer showing a two-sided linedef
		if (inst.edit.mode == ObjType::linedefs && obj_count > 1)
			obj_idx = Selection_FirstLine(inst.level, *inst.edit.Selected);
		else
			obj_idx = inst.edit.Selected->find_first();
	}

	switch (inst.edit.mode)
	{
		case ObjType::things:
			inst.main_win->thing_box->SetObj(obj_idx, obj_count);
			break;

		case ObjType::linedefs:
			inst.main_win->line_box->SetObj(obj_idx, obj_count);
			break;

		case ObjType::sectors:
			inst.main_win->sec_box->SetObj(obj_idx, obj_count);
			break;

		case ObjType::vertices:
			inst.main_win->vert_box->SetObj(obj_idx, obj_count);
			break;

		default: break;
	}
}


void Instance::UpdateDrawLine()
{
	if (edit.action != EditorAction::drawLine || edit.drawLine.from.is_nil())
		return;

	const auto V = level.vertices[edit.drawLine.from.num];

	v2double_t newpos = edit.map.xy;

	if (grid.getRatio() > 0)
	{
		grid.RatioSnapXY(newpos, V->xy());
	}
	else if (edit.highlight.valid())
	{
		newpos = level.vertices[edit.highlight.num]->xy();
	}
	else if (edit.split_line.valid())
	{
		newpos = edit.split;
	}
	else
	{
		newpos = grid.Snap(newpos);
	}

	edit.drawLine.to = newpos;

	// when drawing mode, highlight a vertex at the snap position
	if (grid.snaps() && edit.highlight.is_nil() && edit.split_line.is_nil())
	{
		int near_vert = level.vertmod.findExact(FFixedPoint(newpos.x), FFixedPoint(newpos.y));
		if (near_vert >= 0)
		{
			edit.highlight = Objid(ObjType::vertices, near_vert);
		}
	}

	// never highlight the vertex we are drawing from
	if (edit.drawLine.from.valid() &&
		edit.drawLine.from == edit.highlight)
	{
		edit.highlight.clear();
	}
}


static void UpdateSplitLine(Instance &inst, const v2double_t &map)
{
	inst.edit.split_line.clear();

	// splitting usually disabled while dragging stuff, EXCEPT a single vertex
	if (inst.edit.action == EditorAction::drag && inst.edit.dragged.is_nil())
		goto done;

	// in vertex mode, see if there is a linedef which would be split by
	// adding a new vertex.

	if (inst.edit.mode == ObjType::vertices &&
		inst.edit.pointer_in_window &&
	    inst.edit.highlight.is_nil())
	{
		inst.edit.split_line = hover::findSplitLine(inst.level, inst.loaded.levelFormat, inst.edit,
													inst.grid, inst.edit.split, map,
													inst.edit.dragged.num);

		// NOTE: OK if the split line has one of its vertices selected
		//       (that case is handled by Insert_Vertex)
	}

done:
	inst.main_win->canvas->UpdateHighlight();
}


void Instance::UpdateHighlight()
{
	if (edit.render3d)
	{
		Render3D_UpdateHighlight();
		UpdatePanel(*this);
		return;
	}

	// find the object to highlight
	edit.highlight.clear();

	// don't highlight when dragging, EXCEPT when dragging a single vertex
	if (edit.pointer_in_window &&
	    (edit.action != EditorAction::drag || (edit.mode == ObjType::vertices && edit.dragged.valid()) ))
	{
		edit.highlight = hover::getNearbyObject(edit.mode, level, conf, grid, edit.map.xy);

		// guarantee that we cannot drag a vertex onto itself
		if (edit.action == EditorAction::drag && edit.dragged.valid() &&
			edit.highlight.valid() && edit.dragged.num == edit.highlight.num)
		{
			edit.highlight.clear();
		}

		// if drawing a line and ratio lock is ON, only highlight a
		// vertex if it is *exactly* the right ratio.
		if (grid.getRatio() > 0 && edit.action == EditorAction::drawLine &&
			edit.mode == ObjType::vertices && edit.highlight.valid())
		{
			const auto V = level.vertices[edit.highlight.num];
			const auto S = level.vertices[edit.drawLine.from.num];

			v2double_t vpos = V->xy();

			grid.RatioSnapXY(vpos, S->xy());

			if (MakeValidCoord(loaded.levelFormat, vpos.x) != V->raw_x ||
				MakeValidCoord(loaded.levelFormat, vpos.y) != V->raw_y)
			{
				edit.highlight.clear();
			}
		}
	}

	UpdateSplitLine(*this, edit.map.xy);
	UpdateDrawLine();

	main_win->canvas->UpdateHighlight();
	main_win->canvas->CheckGridSnap();

	UpdatePanel(*this);
}


void Instance::Editor_ClearErrorMode()
{
	if (edit.error_mode)
	{
		Selection_Clear();
	}
}


void Instance::Editor_ChangeMode_Raw(ObjType new_mode)
{
	// keep selection after a "Find All" and user dismisses panel
	if (new_mode == edit.mode && main_win->isSpecialPanelShown())
		edit.error_mode = false;

	edit.mode = new_mode;

	Editor_ClearAction();
	Editor_ClearErrorMode();

	edit.highlight.clear();
	edit.split_line.clear();
}


void Instance::Editor_ChangeMode(char mode_char)
{
	ObjType  prev_type = edit.mode;

	// Set the object type according to the new mode.
	switch (mode_char)
	{
		case 't': Editor_ChangeMode_Raw(ObjType::things);   break;
		case 'l': Editor_ChangeMode_Raw(ObjType::linedefs); break;
		case 's': Editor_ChangeMode_Raw(ObjType::sectors);  break;
		case 'v': Editor_ChangeMode_Raw(ObjType::vertices); break;

		default:
			Beep("Unknown mode: %c\n", mode_char);
			return;
	}

	if (prev_type != edit.mode)
	{
		Selection_Push();

		main_win->NewEditMode(edit.mode);

		// convert the selection
		selection_c prev_sel = *edit.Selected;
		edit.Selected.emplace(edit.mode, true /* extended */);

		ConvertSelection(level, prev_sel, *edit.Selected);
	}
	else if (main_win->isSpecialPanelShown())
	{
		// same mode, but this removes the special panel
		main_win->NewEditMode(edit.mode);
	}
	// -AJA- Yadex (DEU?) would clear the selection if the mode didn't
	//       change.  We optionally emulate that behavior here.
	else if (config::same_mode_clears_selection)
	{
		Selection_Clear();
	}

	RedrawMap();
}


//------------------------------------------------------------------------


void Document::UpdateLevelBounds(int start_vert) noexcept
{
	for(int i = start_vert; i < numVertices(); i++)
	{
		const auto V = vertices[i];

		if (V->x() < Map_bound1.x) Map_bound1.x = V->x();
		if (V->y() < Map_bound1.y) Map_bound1.y = V->y();

		if (V->x() > Map_bound2.x) Map_bound2.x = V->x();
		if (V->y() > Map_bound2.y) Map_bound2.y = V->y();
	}
}

void Document::CalculateLevelBounds() noexcept
{
	if (numVertices() == 0)
	{
		Map_bound1.x = Map_bound2.x = 0;
		Map_bound1.y = Map_bound2.y = 0;
		return;
	}

	Map_bound1.x = 32767; Map_bound2.x = -32767;
	Map_bound1.y = 32767; Map_bound2.y = -32767;

	UpdateLevelBounds(0);
}


void Instance::MapStuff_NotifyBegin()
{
	recalc_map_bounds  = false;
	new_vertex_minimum = -1;
	moved_vertex_count =  0;

	sound_propagation_invalid = true;
}

void Instance::MapStuff_NotifyInsert(ObjType type, int objnum)
{
	if (type == ObjType::vertices)
	{
		if (new_vertex_minimum < 0 || objnum < new_vertex_minimum)
			new_vertex_minimum = objnum;
	}
}

void Instance::MapStuff_NotifyDelete(ObjType type, int objnum)
{
	if (type == ObjType::vertices)
	{
		recalc_map_bounds = true;

		if (edit.action == EditorAction::drawLine &&
			edit.drawLine.from.num == objnum)
		{
			Editor_ClearAction();
		}
	}
}

void Instance::MapStuff_NotifyChange(ObjType type, int objnum, int field)
{
	if (type == ObjType::vertices)
	{
		// NOTE: for performance reasons we don't recalculate the
		//       map bounds when only moving a few vertices.
		moved_vertex_count++;

		const auto V = level.vertices[objnum];

		if (V->x() < level.Map_bound1.x) level.Map_bound1.x = V->x();
		if (V->y() < level.Map_bound1.y) level.Map_bound1.y = V->y();

		if (V->x() > level.Map_bound2.x) level.Map_bound2.x = V->x();
		if (V->y() > level.Map_bound2.y) level.Map_bound2.y = V->y();

		// TODO: only invalidate sectors touching vertex
		Subdiv_InvalidateAll();
	}

	if (type == ObjType::sidedefs && field == SideDef::F_SECTOR)
		Subdiv_InvalidateAll();

	if (type == ObjType::linedefs && (field == LineDef::F_LEFT || field == LineDef::F_RIGHT || field == LineDef::F_START || field == LineDef::F_END))
		Subdiv_InvalidateAll();

	if (type == ObjType::sectors && (field == Sector::F_FLOORH || field == Sector::F_CEILH))
		Subdiv_InvalidateAll();
}

void Instance::MapStuff_NotifyEnd()
{
	if (recalc_map_bounds || moved_vertex_count > 10)  // TODO: CONFIG
	{
		level.CalculateLevelBounds();
	}
	else if (new_vertex_minimum >= 0)
	{
		level.UpdateLevelBounds(new_vertex_minimum);
	}
}


//------------------------------------------------------------------------
//  ObjectBox Notification handling
//------------------------------------------------------------------------


void Instance::ObjectBox_NotifyBegin()
{
	invalidated_totals = false;
	invalidated_panel_obj = false;
	changed_panel_obj = false;
	changed_recent_list = false;
}


void Instance::ObjectBox_NotifyInsert(ObjType type, int objnum)
{
	invalidated_totals = true;

	if (type != edit.mode)
		return;

	if (objnum > main_win->GetPanelObjNum())
		return;

	invalidated_panel_obj = true;
}


void Instance::ObjectBox_NotifyDelete(ObjType type, int objnum)
{
	invalidated_totals = true;

	if (type != edit.mode)
		return;

	if (objnum > main_win->GetPanelObjNum())
		return;

	invalidated_panel_obj = true;
}


void Instance::ObjectBox_NotifyChange(ObjType type, int objnum, int field)
{
	if (type != edit.mode || !main_win)
		return;

	if (objnum != main_win->GetPanelObjNum())
		return;

	changed_panel_obj = true;
}


void Instance::ObjectBox_NotifyEnd() const
{
	if(!main_win)
		return;
	if (invalidated_totals)
		main_win->UpdateTotals(level);

	if (invalidated_panel_obj)
	{
		main_win->InvalidatePanelObj();
	}
	else if (changed_panel_obj)
	{
		main_win->UpdatePanelObj();
	}

	if (changed_recent_list)
		main_win->browser->RecentUpdate();
}


//------------------------------------------------------------------------
//  Selection Notification, ETC
//------------------------------------------------------------------------


void Instance::Selection_NotifyBegin()
{
	invalidated_selection = false;
	invalidated_last_sel  = false;
}

void Instance::Selection_NotifyInsert(ObjType type, int objnum)
{
	if (edit.Selected && type == edit.Selected->what_type() &&
		objnum <= edit.Selected->max_obj())
	{
		invalidated_selection = true;
	}

	if (last_Sel &&
		type == last_Sel->what_type() &&
		objnum <= last_Sel->max_obj())
	{
		invalidated_last_sel = true;
	}
}

void Instance::Selection_NotifyDelete(ObjType type, int objnum)
{
	if (edit.Selected && objnum <= edit.Selected->max_obj())
	{
		invalidated_selection = true;
	}

	if (last_Sel &&
		type == last_Sel->what_type() &&
		objnum <= last_Sel->max_obj())
	{
		invalidated_last_sel = true;
	}
}


void Selection_NotifyChange(ObjType type, int objnum, int field)
{
	// field changes never affect the current selection
}


void Instance::Selection_NotifyEnd()
{
	if (invalidated_selection)
	{
		// this clears AND RESIZES the selection_c object
		edit.Selected->change_type(edit.mode);
	}

	if (invalidated_last_sel)
		Selection_InvalidateLast();
}


//
//  list the contents of a selection (for debugging)
//
void DumpSelection(selection_c * list)
{
	SYS_ASSERT(list);

	printf("Selection:");

	for (sel_iter_c it(list); ! it.done(); it.next())
		printf(" %d", *it);

	printf("\n");
}


void ConvertSelection(const Document &doc, const selection_c & src, selection_c & dest)
{
	if (src.what_type() == dest.what_type())
	{
		dest.merge(src);
		return;
	}


	if (src.what_type() == ObjType::sectors && dest.what_type() == ObjType::things)
	{
		for (int t = 0 ; t < doc.numThings() ; t++)
		{
			const auto T = doc.things[t];

			Objid obj = hover::getNearestSector(doc, T->xy());

			if (! obj.is_nil() && src.get(obj.num))
			{
				dest.set(t);
			}
		}
		return;
	}


	if (src.what_type() == ObjType::sectors && dest.what_type() == ObjType::linedefs)
	{
		for (int l = 0 ; l < doc.numLinedefs(); l++)
		{
			const auto L = doc.linedefs[l];

			if ( (doc.getRight(*L) && src.get(doc.getRight(*L)->sector)) ||
				 (doc.getLeft(*L)  && src.get(doc.getLeft(*L)->sector)) )
			{
				dest.set(l);
			}
		}
		return;
	}


	if (src.what_type() == ObjType::sectors && dest.what_type() == ObjType::vertices)
	{
		for (const auto &L : doc.linedefs)
		{
			if ( (doc.getRight(*L) && src.get(doc.getRight(*L)->sector)) ||
				 (doc.getLeft(*L)  && src.get(doc.getLeft(*L)->sector)) )
			{
				dest.set(L->start);
				dest.set(L->end);
			}
		}
		return;
	}


	if (src.what_type() == ObjType::linedefs && dest.what_type() == ObjType::sidedefs)
	{
		for (sel_iter_c it(src); ! it.done(); it.next())
		{
			const auto L = doc.linedefs[*it];

			if (doc.getRight(*L)) dest.set(L->right);
			if (doc.getLeft(*L))  dest.set(L->left);
		}
		return;
	}

	if (src.what_type() == ObjType::sectors && dest.what_type() == ObjType::sidedefs)
	{
		for (int n = 0 ; n < doc.numSidedefs(); n++)
		{
			const auto SD = doc.sidedefs[n];

			if (src.get(SD->sector))
				dest.set(n);
		}
		return;
	}


	if (src.what_type() == ObjType::linedefs && dest.what_type() == ObjType::vertices)
	{
		for (sel_iter_c it(src); ! it.done(); it.next())
		{
			const auto L = doc.linedefs[*it];

			dest.set(L->start);
			dest.set(L->end);
		}
		return;
	}


	if (src.what_type() == ObjType::vertices && dest.what_type() == ObjType::linedefs)
	{
		// select all linedefs that have both ends selected
		for (int l = 0 ; l < doc.numLinedefs(); l++)
		{
			const auto L = doc.linedefs[l];

			if (src.get(L->start) && src.get(L->end))
			{
				dest.set(l);
			}
		}
	}


	// remaining conversions are L->S and V->S

	if (dest.what_type() != ObjType::sectors)
		return;

	if (src.what_type() != ObjType::linedefs && src.what_type() != ObjType::vertices)
		return;


	// step 1: select all sectors (except empty ones)
	int l;

	for (l = 0 ; l < doc.numLinedefs() ; l++)
	{
		const auto L = doc.linedefs[l];

		if (doc.getRight(*L)) dest.set(doc.getRight(*L)->sector);
		if (doc.getLeft(*L))  dest.set(doc.getLeft(*L)->sector);
	}

	// step 2: unselect any sectors if a component is not selected

	for (l = 0 ; l < doc.numLinedefs(); l++)
	{
		const auto L = doc.linedefs[l];

		if (src.what_type() == ObjType::vertices)
		{
			if (src.get(L->start) && src.get(L->end))
				continue;
		}
		else
		{
			if (src.get(l))
				continue;
		}

		if (doc.getRight(*L)) dest.clear(doc.getRight(*L)->sector);
		if (doc.getLeft(*L))  dest.clear(doc.getLeft(*L)->sector);
	}
}


//
// Return the line to show in the LineDef panel from the selection.
// When the selection is a mix of one-sided and two-sided lines, then
// we want the first TWO-SIDED line.
//
// NOTE: this is slow, as it may need to search the whole list.
//
static int Selection_FirstLine(const Document &doc, const selection_c &list)
{
	for (sel_iter_c it(list); ! it.done(); it.next())
	{
		const auto L = doc.linedefs[*it];

		if (L->TwoSided())
			return *it;
	}

	// return first entry (a one-sided line)
	return list.find_first();
}


//
// This is a helper to handle performing an operation on the
// selection if it is non-empty, otherwise the highlight.
// Returns false if both selection and highlight are empty.
//
SelectHighlight Editor_State_t::SelectionOrHighlight()
{
	if(!Selected->empty())
		return SelectHighlight::ok;

	if (highlight.valid())
	{
		Selection_AddHighlighted();
		return SelectHighlight::unselect;
	}

	return SelectHighlight::empty;
}


//
// select all objects inside a given box
//
void SelectObjectsInBox(const Document &doc, selection_c *list, ObjType objtype, v2double_t pos1, v2double_t pos2)
{
	if (pos2.x < pos1.x)
		std::swap(pos1.x, pos2.x);

	if (pos2.y < pos1.y)
		std::swap(pos1.y, pos2.y);

	switch (objtype)
	{
		case ObjType::things:
			for (int n = 0 ; n < doc.numThings() ; n++)
			{
				const auto T = doc.things[n];

				v2double_t tpos = T->xy();

				if(tpos.inbounds(pos1, pos2))
					list->toggle(n);
			}
			break;

		case ObjType::vertices:
			for (int n = 0 ; n < doc.numVertices(); n++)
			{
				const auto V = doc.vertices[n];

				v2double_t vpos = V->xy();

				if(vpos.inbounds(pos1, pos2))
					list->toggle(n);
			}
			break;

		case ObjType::linedefs:
			for (int n = 0 ; n < doc.numLinedefs(); n++)
			{
				const auto L = doc.linedefs[n];

				/* the two ends of the line must be in the box */
				if(doc.getStart(*L).xy().inbounds(pos1, pos2) &&
				   doc.getEnd(*L).xy().inbounds(pos1, pos2))
				{
					list->toggle(n);
				}
			}
			break;

		case ObjType::sectors:
		{
			selection_c  in_sectors(ObjType::sectors);
			selection_c out_sectors(ObjType::sectors);

			for (int n = 0 ; n < doc.numLinedefs(); n++)
			{
				const auto L = doc.linedefs[n];

				// Get the numbers of the sectors on both sides of the linedef
				int s1 = doc.getRight(*L) ? doc.getRight(*L)->sector : -1;
				int s2 = doc.getLeft(*L) ? doc.getLeft(*L) ->sector : -1;

				if(doc.getStart(*L).xy().inbounds(pos1, pos2) &&
				   doc.getEnd(*L).xy().inbounds(pos1, pos2))
				{
					if (s1 >= 0) in_sectors.set(s1);
					if (s2 >= 0) in_sectors.set(s2);
				}
				else
				{
					if (s1 >= 0) out_sectors.set(s1);
					if (s2 >= 0) out_sectors.set(s2);
				}
			}

			for (int i = 0 ; i < doc.numSectors(); i++)
				if (in_sectors.get(i) && ! out_sectors.get(i))
					list->toggle(i);

			break;
		}
		default:
			break;
	}
}



void Instance::Selection_InvalidateLast() noexcept
{
	last_Sel.reset();
}


void Instance::Selection_Push()
{
	if (edit.Selected->empty())
		return;

	if (last_Sel && last_Sel->test_equal(*edit.Selected))
		return;

	// OK copy it

	
	last_Sel.emplace(edit.Selected->what_type(), true);

	last_Sel->merge(*edit.Selected);
}


void Instance::Selection_Clear(bool no_save)
{
	if (! no_save)
		Selection_Push();

	// this always clears it
	edit.Selected->change_type(edit.mode);

	edit.error_mode = false;

	if (main_win)
		main_win->UnselectPics();

	RedrawMap();
}

void Editor_State_t::Selection_AddHighlighted()
{
	Objid &obj = highlight;
	// validate the mode is correct
	if (obj.type != mode)
		return;

	if (obj.parts == 0)
	{
		// ignore the add if object is already set.
		// [ since the selection may have parts, and we don't want to
		//   forget those parts ]
		if (! Selected->get(obj.num))
			Selected->set(obj.num);
		return;
	}

	byte cur = Selected->get_ext(obj.num);

	cur = static_cast<byte>(1 | obj.parts);

	Selected->set_ext(obj.num, cur);
}

void Editor_State_t::Selection_Toggle(Objid& obj)
{
	if (obj.type != mode)
		return;

	if (obj.parts == 0)
	{
		Selected->toggle(obj.num);
		return;
	}

	byte cur = Selected->get_ext(obj.num);

	// if object was simply selected, then just clear it
	if (cur == 1)
	{
		Selected->clear(obj.num);
		return;
	}

	cur = static_cast<byte>(1 | (cur ^ obj.parts));

	// if we toggled off all the parts, then unset the object itself
	if (cur == 1)
		cur = 0;

	Selected->set_ext(obj.num, cur);
}


static void Selection_Validate(Instance &inst)
{
	int num_obj = inst.level.numObjects(inst.edit.mode);

	if (inst.edit.Selected->max_obj() >= num_obj)
	{
		inst.edit.Selected->frob_range(num_obj, inst.edit.Selected->max_obj(), BitOp::remove);

		inst.Beep("BUG: invalid selection");
	}
}


void Instance::CMD_LastSelection()
{
	if (! last_Sel)
	{
		Beep("No last selection (or was invalidated)");
		return;
	}

	bool changed_mode = false;

	if (last_Sel->what_type() != edit.mode)
	{
		changed_mode = true;
		Editor_ChangeMode_Raw(last_Sel->what_type());
		main_win->NewEditMode(edit.mode);
	}

	last_Sel.swap(edit.Selected);

	// ensure everything is kosher
	Selection_Validate(*this);

	if (changed_mode)
		GoToSelection();

	RedrawMap();
}


//------------------------------------------------------------------------
//  RECENTLY USED TEXTURES (etc)
//------------------------------------------------------------------------


int Recently_used::find(const SString &name)
{
	for (int k = 0 ; k < size ; k++)
		if (name_set[k].noCaseEqual(name))
			return k;

	return -1;	// not found
}

int Recently_used::find_number(int val)
{
	return find(SString::printf("%d", val));
}


void Recently_used::insert(const SString &name)
{
	// ignore '-' texture
	if (is_null_tex(name))
		return;

	// ignore empty strings to prevent potential problems
	if (name.empty())
		return;

	int idx = find(name);

	// optimisation
	if (idx >= 0 && idx < 4)
		return;

	if (idx >= 0)
		erase(idx);

	push_front(name);

	// mark browser for later update
	// [ this method may be called very often by basis, too expensive to
	//   update the browser here ]
	inst.changed_recent_list = true;
}

void Recently_used::insert_number(int val)
{
	insert(SString::printf("%d", val));
}


void Recently_used::erase(int index)
{
	SYS_ASSERT(0 <= index && index < size);

	size--;

	for ( ; index < size ; index++)
	{
		name_set[index] = name_set[index + 1];
	}

	name_set[index].clear();
}


void Recently_used::push_front(const SString &name)
{
	if (size >= keep_num)
	{
		erase(keep_num - 1);
	}

	// shift elements up
	for (int k = size - 1 ; k >= 0 ; k--)
	{
		name_set[k + 1] = name_set[k];
	}

	name_set[0] = name;

	size++;
}


void Recently_used::clear()
{
	size = 0;

	for(SString &name : name_set)
		name.clear();
}


static void RecUsed_ClearAll(Instance &inst)
{
	inst.recent_textures.clear();
	inst.recent_flats   .clear();
	inst.recent_things  .clear();

	if (inst.main_win)
		inst.main_win->browser->RecentUpdate();
}


/* --- Save and Restore --- */

void Recently_used::WriteUser(std::ostream &os, char letter) const
{
	for (int i = 0 ; i < size ; i++)
	{
		os << "recent_used " << letter << " \"" << name_set[i].getTidy() << "\"\n";
	}
}


void Instance::RecUsed_WriteUser(std::ostream &os) const
{
	os << "\nrecent_used clear\n";

	recent_textures.WriteUser(os, 'T');
	recent_flats.WriteUser(os, 'F');
	recent_things.WriteUser(os, 'O');
}


bool Instance::RecUsed_ParseUser(const std::vector<SString> &tokens)
{
	if (tokens[0] != "recent_used" || tokens.size() < 2)
		return false;

	if (tokens[1] == "clear")
	{
		RecUsed_ClearAll(*this);
		return true;
	}

	// syntax is:  recent_used  <kind>  <name>
	if (tokens.size() < 3)
		return false;

	switch (tokens[1][0])
	{
		case 'T':
			recent_textures.insert(tokens[2]);
			break;

		case 'F':
			recent_flats.insert(tokens[2]);
			break;

		case 'O':
			recent_things.insert(tokens[2]);
			break;

		default:
			// ignore it
			break;
	}

	if (main_win)
		main_win->browser->RecentUpdate();

	return true;
}


//------------------------------------------------------------------------


// this in e_commands.cc
void Editor_RegisterCommands();


void Instance::Editor_Init()
{
	switch (config::default_edit_mode)
	{
		case 1:  edit.mode = ObjType::linedefs; break;
		case 2:  edit.mode = ObjType::sectors;  break;
		case 3:  edit.mode = ObjType::vertices; break;
		default: edit.mode = ObjType::things;   break;
	}

	edit.render3d = false;

	edit.defaultState();

	Nav_Clear();

	edit.pointer_in_window = false;
	edit.map = { 0, 0, -1 };

	edit.Selected.emplace(edit.mode, true /* extended */);

	edit.highlight.clear();
	edit.split_line.clear();
	edit.clicked.clear();

	grid.Init();

	level.MadeChanges = false;

	  Editor_RegisterCommands();
	Render3D_RegisterCommands();
}


void Editor_State_t::defaultState()
{
	action = EditorAction::nothing;
	sticky_mod = 0;
	is_panning = false;

	dragged.clear();
	drawLine.from.clear();

	error_mode = false;
	show_object_numbers = false;

	sector_render_mode = config::sector_render_default;
	 thing_render_mode =  config::thing_render_default;
}


bool Instance::Editor_ParseUser(const std::vector<SString> &tokens)
{
	if (tokens[0] == "edit_mode" && tokens.size() >= 2)
	{
		Editor_ChangeMode(tokens[1][0]);
		return true;
	}

	if (tokens[0] == "render_mode" && tokens.size() >= 2)
	{
		edit.render3d = atoi(tokens[1]);
		RedrawMap();
		return true;
	}

	if (tokens[0] == "sector_render_mode" && tokens.size() >= 2)
	{
		edit.sector_render_mode = atoi(tokens[1]);
		RedrawMap();
		return true;
	}

	if (tokens[0] == "thing_render_mode" && tokens.size() >= 2)
	{
		edit.thing_render_mode = atoi(tokens[1]);
		RedrawMap();
		return true;
	}

	if (tokens[0] == "show_object_numbers" && tokens.size() >= 2)
	{
		edit.show_object_numbers = atoi(tokens[1]);
		RedrawMap();
		return true;
	}

	return false;
}

//
// Write editor state
//
void Instance::Editor_WriteUser(std::ostream &os) const
{
	switch (edit.mode)
	{
	case ObjType::things:   os << "edit_mode t\n"; break;
	case ObjType::linedefs: os << "edit_mode l\n"; break;
	case ObjType::sectors:  os << "edit_mode s\n"; break;
	case ObjType::vertices: os << "edit_mode v\n"; break;

		default: break;
	}
	os << "render_mode " << (edit.render3d ? 1 : 0) << '\n';
	os << "sector_render_mode " << edit.sector_render_mode << '\n';
	os << "thing_render_mode " << edit.thing_render_mode << '\n';
	os << "show_object_numbers " << (edit.show_object_numbers ? 1 : 0) << '\n';
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
