//------------------------------------------------------------------------
//  LEVEL MISC STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
//  Copyright (C) 1997-2003 Andr� Majorel et al
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
//  in the public domain in 1994 by Rapha�l Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "Document.h"
#include "Errors.h"
#include "main.h"

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
#include "w_rawdef.h"

#include "ui_window.h"


Editor_State_t  edit;


double Map_bound_x1 =  32767;   /* minimum X value of map */
double Map_bound_y1 =  32767;   /* minimum Y value of map */
double Map_bound_x2 = -32767;   /* maximum X value of map */
double Map_bound_y2 = -32767;   /* maximum Y value of map */

int MadeChanges;

static bool recalc_map_bounds;
static int  new_vertex_minimum;
static int  moved_vertex_count;

static selection_c * last_Sel;

extern bool sound_propagation_invalid;


// config items
int config::default_edit_mode = 3;  // Vertices

bool config::same_mode_clears_selection = false;

int config::sector_render_default = (int)SREND_Floor;
int  config::thing_render_default = 1;


//
// adjust zoom factor to make whole map fit in window
//
static void zoom_fit()
{
	if (NumVertices == 0)
		return;

	double xzoom = 1;
	double yzoom = 1;

	int ScrMaxX = main_win->canvas->w();
	int ScrMaxY = main_win->canvas->h();

	if (Map_bound_x1 < Map_bound_x2)
		xzoom = ScrMaxX / (Map_bound_x2 - Map_bound_x1);

	if (Map_bound_y1 < Map_bound_y2)
		yzoom = ScrMaxY / (Map_bound_y2 - Map_bound_y1);

	grid.NearestScale(MIN(xzoom, yzoom));

	grid.MoveTo((Map_bound_x1 + Map_bound_x2) / 2, (Map_bound_y1 + Map_bound_y2) / 2);
}


void ZoomWholeMap()
{
	if (MadeChanges)
		CalculateLevelBounds();

	zoom_fit();

	RedrawMap();
}


void RedrawMap()
{
	if (! main_win)
		return;

	UpdateHighlight();

	main_win->scroll->UpdateRenderMode();
	main_win->info_bar->UpdateSecRend();
	main_win->status_bar->redraw();
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

	// the highlight is usually turned off when dragging, so compensate
	if (obj_idx < 0 && edit.action == ACT_DRAG)
		obj_idx = edit.dragged.num;

	if (obj_idx >= 0)
	{
		if (! edit.Selected->get(obj_idx))
			obj_count = 0;
	}
	else if (obj_count > 0)
	{
		// in linedef mode, we prefer showing a two-sided linedef
		if (edit.mode == ObjType::linedefs && obj_count > 1)
			obj_idx = Selection_FirstLine(edit.Selected);
		else
			obj_idx = edit.Selected->find_first();
	}

	switch (edit.mode)
	{
		case ObjType::things:
			main_win->thing_box->SetObj(obj_idx, obj_count);
			break;

		case ObjType::linedefs:
			main_win->line_box->SetObj(obj_idx, obj_count);
			break;

		case ObjType::sectors:
			main_win->sec_box->SetObj(obj_idx, obj_count);
			break;

		case ObjType::vertices:
			main_win->vert_box->SetObj(obj_idx, obj_count);
			break;

		default: break;
	}
}


void UpdateDrawLine()
{
	if (edit.action != ACT_DRAW_LINE || edit.draw_from.is_nil())
		return;

	const Vertex *V = gDocument.vertices[edit.draw_from.num];

	double new_x = edit.map_x;
	double new_y = edit.map_y;

	if (grid.ratio > 0)
	{
		grid.RatioSnapXY(new_x, new_y, V->x(), V->y());
	}
	else if (edit.highlight.valid())
	{
		new_x = gDocument.vertices[edit.highlight.num]->x();
		new_y = gDocument.vertices[edit.highlight.num]->y();
	}
	else if (edit.split_line.valid())
	{
		new_x = edit.split_x;
		new_y = edit.split_y;
	}
	else
	{
		new_x = grid.SnapX(new_x);
		new_y = grid.SnapY(new_y);
	}

	edit.draw_to_x = new_x;
	edit.draw_to_y = new_y;

	// when drawing mode, highlight a vertex at the snap position
	if (grid.snap && edit.highlight.is_nil() && edit.split_line.is_nil())
	{
		int near_vert = Vertex_FindExact(TO_COORD(new_x), TO_COORD(new_y));
		if (near_vert >= 0)
		{
			edit.highlight = Objid(ObjType::vertices, near_vert);
		}
	}

	// never highlight the vertex we are drawing from
	if (edit.draw_from.valid() &&
		edit.draw_from == edit.highlight)
	{
		edit.highlight.clear();
	}
}


static void UpdateSplitLine(double map_x, double map_y)
{
	edit.split_line.clear();

	// splitting usually disabled while dragging stuff, EXCEPT a single vertex
	if (edit.action == ACT_DRAG && edit.dragged.is_nil())
		goto done;

	// in vertex mode, see if there is a linedef which would be split by
	// adding a new vertex.

	if (edit.mode == ObjType::vertices &&
		edit.pointer_in_window &&
	    edit.highlight.is_nil())
	{
		edit.split_line = gDocument.hover.findSplitLine(edit.split_x, edit.split_y,
					  map_x, map_y, edit.dragged.num);

		// NOTE: OK if the split line has one of its vertices selected
		//       (that case is handled by Insert_Vertex)
	}

done:
	main_win->canvas->UpdateHighlight();
}


void UpdateHighlight()
{
	if (edit.render3d)
	{
		Render3D_UpdateHighlight();
		UpdatePanel();
		return;
	}

	// find the object to highlight
	edit.highlight.clear();

	// don't highlight when dragging, EXCEPT when dragging a single vertex
	if (edit.pointer_in_window &&
	    (edit.action != ACT_DRAG || (edit.mode == ObjType::vertices && edit.dragged.valid()) ))
	{
		edit.highlight = gDocument.hover.getNearbyObject(edit.mode, edit.map_x, edit.map_y);

		// guarantee that we cannot drag a vertex onto itself
		if (edit.action == ACT_DRAG && edit.dragged.valid() &&
			edit.highlight.valid() && edit.dragged.num == edit.highlight.num)
		{
			edit.highlight.clear();
		}

		// if drawing a line and ratio lock is ON, only highlight a
		// vertex if it is *exactly* the right ratio.
		if (grid.ratio > 0 && edit.action == ACT_DRAW_LINE &&
			edit.mode == ObjType::vertices && edit.highlight.valid())
		{
			const Vertex *V = gDocument.vertices[edit.highlight.num];
			const Vertex *S = gDocument.vertices[edit.draw_from.num];

			double vx = V->x();
			double vy = V->y();

			grid.RatioSnapXY(vx, vy, S->x(), S->y());

			if (MakeValidCoord(vx) != V->raw_x ||
				MakeValidCoord(vy) != V->raw_y)
			{
				edit.highlight.clear();
			}
		}
	}

	UpdateSplitLine(edit.map_x, edit.map_y);
	UpdateDrawLine();

	main_win->canvas->UpdateHighlight();
	main_win->canvas->CheckGridSnap();

	UpdatePanel();
}


void Editor_ClearErrorMode()
{
	if (edit.error_mode)
	{
		Selection_Clear();
	}
}


void Editor_ChangeMode_Raw(ObjType new_mode)
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


void Editor_ChangeMode(char mode_char)
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
		selection_c *prev_sel = edit.Selected;
		edit.Selected = new selection_c(edit.mode, true /* extended */);

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
	else if (config::same_mode_clears_selection)
	{
		Selection_Clear();
	}

	RedrawMap();
}


//------------------------------------------------------------------------


void UpdateLevelBounds(int start_vert)
{
	for (int i = start_vert ; i < NumVertices ; i++)
	{
		const Vertex * V = gDocument.vertices[i];

		if (V->x() < Map_bound_x1) Map_bound_x1 = V->x();
		if (V->y() < Map_bound_y1) Map_bound_y1 = V->y();

		if (V->x() > Map_bound_x2) Map_bound_x2 = V->x();
		if (V->y() > Map_bound_y2) Map_bound_y2 = V->y();
	}
}

void CalculateLevelBounds()
{
	if (NumVertices == 0)
	{
		Map_bound_x1 = Map_bound_x2 = 0;
		Map_bound_y1 = Map_bound_y2 = 0;
		return;
	}

	Map_bound_x1 = 32767; Map_bound_x2 = -32767;
	Map_bound_y1 = 32767; Map_bound_y2 = -32767;

	UpdateLevelBounds(0);
}


void MapStuff_NotifyBegin()
{
	recalc_map_bounds  = false;
	new_vertex_minimum = -1;
	moved_vertex_count =  0;

	sound_propagation_invalid = true;
}

void MapStuff_NotifyInsert(ObjType type, int objnum)
{
	if (type == ObjType::vertices)
	{
		if (new_vertex_minimum < 0 || objnum < new_vertex_minimum)
			new_vertex_minimum = objnum;
	}
}

void MapStuff_NotifyDelete(ObjType type, int objnum)
{
	if (type == ObjType::vertices)
	{
		recalc_map_bounds = true;

		if (edit.action == ACT_DRAW_LINE &&
			edit.draw_from.num == objnum)
		{
			Editor_ClearAction();
		}
	}
}

void MapStuff_NotifyChange(ObjType type, int objnum, int field)
{
	if (type == ObjType::vertices)
	{
		// NOTE: for performance reasons we don't recalculate the
		//       map bounds when only moving a few vertices.
		moved_vertex_count++;

		const Vertex * V = gDocument.vertices[objnum];

		if (V->x() < Map_bound_x1) Map_bound_x1 = V->x();
		if (V->y() < Map_bound_y1) Map_bound_y1 = V->y();

		if (V->x() > Map_bound_x2) Map_bound_x2 = V->x();
		if (V->y() > Map_bound_y2) Map_bound_y2 = V->y();

		// TODO: only invalidate sectors touching vertex
		Subdiv_InvalidateAll();
	}

	if (type == ObjType::sidedefs && field == SideDef::F_SECTOR)
		Subdiv_InvalidateAll();

	if (type == ObjType::linedefs && (field == LineDef::F_LEFT || field == LineDef::F_RIGHT))
		Subdiv_InvalidateAll();

	if (type == ObjType::sectors && (field == Sector::F_FLOORH || field == Sector::F_CEILH))
		Subdiv_InvalidateAll();
}

void MapStuff_NotifyEnd()
{
	if (recalc_map_bounds || moved_vertex_count > 10)  // TODO: CONFIG
	{
		CalculateLevelBounds();
	}
	else if (new_vertex_minimum >= 0)
	{
		UpdateLevelBounds(new_vertex_minimum);
	}
}


//------------------------------------------------------------------------
//  ObjectBox Notification handling
//------------------------------------------------------------------------

static bool invalidated_totals;
static bool invalidated_panel_obj;
static bool changed_panel_obj;
static bool changed_recent_list;


void ObjectBox_NotifyBegin()
{
	invalidated_totals = false;
	invalidated_panel_obj = false;
	changed_panel_obj = false;
	changed_recent_list = false;
}


void ObjectBox_NotifyInsert(ObjType type, int objnum)
{
	invalidated_totals = true;

	if (type != edit.mode)
		return;

	if (objnum > main_win->GetPanelObjNum())
		return;

	invalidated_panel_obj = true;
}


void ObjectBox_NotifyDelete(ObjType type, int objnum)
{
	invalidated_totals = true;

	if (type != edit.mode)
		return;

	if (objnum > main_win->GetPanelObjNum())
		return;

	invalidated_panel_obj = true;
}


void ObjectBox_NotifyChange(ObjType type, int objnum, int field)
{
	if (type != edit.mode)
		return;

	if (objnum != main_win->GetPanelObjNum())
		return;

	changed_panel_obj = true;
}


void ObjectBox_NotifyEnd()
{
	if (invalidated_totals)
		main_win->UpdateTotals();

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

static bool invalidated_selection;
static bool invalidated_last_sel;


void Selection_NotifyBegin()
{
	invalidated_selection = false;
	invalidated_last_sel  = false;
}

void Selection_NotifyInsert(ObjType type, int objnum)
{
	if (type == edit.Selected->what_type() &&
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

void Selection_NotifyDelete(ObjType type, int objnum)
{
	if (objnum <= edit.Selected->max_obj())
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


void Selection_NotifyEnd()
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


void ConvertSelection(const selection_c * src, selection_c * dest)
{
	if (src->what_type() == dest->what_type())
	{
		dest->merge(*src);
		return;
	}


	if (src->what_type() == ObjType::sectors && dest->what_type() == ObjType::things)
	{
		for (int t = 0 ; t < NumThings ; t++)
		{
			const Thing *T = gDocument.things[t];

			Objid obj = gDocument.hover.getNearbyObject(ObjType::sectors, T->x(), T->y());

			if (! obj.is_nil() && src->get(obj.num))
			{
				dest->set(t);
			}
		}
		return;
	}


	if (src->what_type() == ObjType::sectors && dest->what_type() == ObjType::linedefs)
	{
		for (int l = 0 ; l < NumLineDefs ; l++)
		{
			const LineDef *L = gDocument.linedefs[l];

			if ( (L->Right(gDocument) && src->get(L->Right(gDocument)->sector)) ||
				 (L->Left(gDocument)  && src->get(L->Left(gDocument)->sector)) )
			{
				dest->set(l);
			}
		}
		return;
	}


	if (src->what_type() == ObjType::sectors && dest->what_type() == ObjType::vertices)
	{
		for (int l = 0 ; l < NumLineDefs ; l++)
		{
			const LineDef *L = gDocument.linedefs[l];

			if ( (L->Right(gDocument) && src->get(L->Right(gDocument)->sector)) ||
				 (L->Left(gDocument)  && src->get(L->Left(gDocument)->sector)) )
			{
				dest->set(L->start);
				dest->set(L->end);
			}
		}
		return;
	}


	if (src->what_type() == ObjType::linedefs && dest->what_type() == ObjType::sidedefs)
	{
		for (sel_iter_c it(src); ! it.done(); it.next())
		{
			const LineDef *L = gDocument.linedefs[*it];

			if (L->Right(gDocument)) dest->set(L->right);
			if (L->Left(gDocument))  dest->set(L->left);
		}
		return;
	}

	if (src->what_type() == ObjType::sectors && dest->what_type() == ObjType::sidedefs)
	{
		for (int n = 0 ; n < NumSideDefs ; n++)
		{
			const SideDef * SD = gDocument.sidedefs[n];

			if (src->get(SD->sector))
				dest->set(n);
		}
		return;
	}


	if (src->what_type() == ObjType::linedefs && dest->what_type() == ObjType::vertices)
	{
		for (sel_iter_c it(src); ! it.done(); it.next())
		{
			const LineDef *L = gDocument.linedefs[*it];

			dest->set(L->start);
			dest->set(L->end);
		}
		return;
	}


	if (src->what_type() == ObjType::vertices && dest->what_type() == ObjType::linedefs)
	{
		// select all linedefs that have both ends selected
		for (int l = 0 ; l < NumLineDefs ; l++)
		{
			const LineDef *L = gDocument.linedefs[l];

			if (src->get(L->start) && src->get(L->end))
			{
				dest->set(l);
			}
		}
	}


	// remaining conversions are L->S and V->S

	if (dest->what_type() != ObjType::sectors)
		return;

	if (src->what_type() != ObjType::linedefs && src->what_type() != ObjType::vertices)
		return;


	// step 1: select all sectors (except empty ones)
	int l;

	for (l = 0 ; l < NumLineDefs ; l++)
	{
		const LineDef *L = gDocument.linedefs[l];

		if (L->Right(gDocument)) dest->set(L->Right(gDocument)->sector);
		if (L->Left(gDocument))  dest->set(L->Left(gDocument)->sector);
	}

	// step 2: unselect any sectors if a component is not selected

	for (l = 0 ; l < NumLineDefs ; l++)
	{
		const LineDef *L = gDocument.linedefs[l];

		if (src->what_type() == ObjType::vertices)
		{
			if (src->get(L->start) && src->get(L->end))
				continue;
		}
		else
		{
			if (src->get(l))
				continue;
		}

		if (L->Right(gDocument)) dest->clear(L->Right(gDocument)->sector);
		if (L->Left(gDocument))  dest->clear(L->Left(gDocument)->sector);
	}
}


//
// Return the line to show in the LineDef panel from the selection.
// When the selection is a mix of one-sided and two-sided lines, then
// we want the first TWO-SIDED line.
//
// NOTE: this is slow, as it may need to search the whole list.
//
int Selection_FirstLine(selection_c *list)
{
	for (sel_iter_c it(list); ! it.done(); it.next())
	{
		const LineDef *L = gDocument.linedefs[*it];

		if (L->TwoSided())
			return *it;
	}

	// return first entry (a one-sided line)
	return list->find_first();
}


//
// This is a helper to handle performing an operation on the
// selection if it is non-empty, otherwise the highlight.
// Returns false if both selection and highlight are empty.
//
SelectHighlight SelectionOrHighlight()
{
	if(!edit.Selected->empty())
		return SelectHighlight::ok;

	if (edit.highlight.valid())
	{
		Selection_Add(edit.highlight);
		return SelectHighlight::unselect;
	}

	return SelectHighlight::empty;
}


//
// select all objects inside a given box
//
void SelectObjectsInBox(selection_c *list, ObjType objtype, double x1, double y1, double x2, double y2)
{
	if (x2 < x1)
		std::swap(x1, x2);

	if (y2 < y1)
		std::swap(y1, y2);

	switch (objtype)
	{
		case ObjType::things:
			for (int n = 0 ; n < NumThings ; n++)
			{
				const Thing *T = gDocument.things[n];

				double tx = T->x();
				double ty = T->y();

				if (x1 <= tx && tx <= x2 && y1 <= ty && ty <= y2)
				{
					list->toggle(n);
				}
			}
			break;

		case ObjType::vertices:
			for (int n = 0 ; n < NumVertices ; n++)
			{
				const Vertex *V = gDocument.vertices[n];

				double vx = V->x();
				double vy = V->y();

				if (x1 <= vx && vx <= x2 && y1 <= vy && vy <= y2)
				{
					list->toggle(n);
				}
			}
			break;

		case ObjType::linedefs:
			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				const LineDef *L = gDocument.linedefs[n];

				/* the two ends of the line must be in the box */
				if (x1 <= L->Start(gDocument)->x() && L->Start(gDocument)->x() <= x2 &&
				    y1 <= L->Start(gDocument)->y() && L->Start(gDocument)->y() <= y2 &&
				    x1 <= L->End(gDocument)->x()   && L->End(gDocument)->x() <= x2 &&
				    y1 <= L->End(gDocument)->y()   && L->End(gDocument)->y() <= y2)
				{
					list->toggle(n);
				}
			}
			break;

		case ObjType::sectors:
		{
			selection_c  in_sectors(ObjType::sectors);
			selection_c out_sectors(ObjType::sectors);

			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				const LineDef *L = gDocument.linedefs[n];

				// Get the numbers of the sectors on both sides of the linedef
				int s1 = L->Right(gDocument) ? L->Right(gDocument)->sector : -1;
				int s2 = L->Left(gDocument) ? L->Left(gDocument) ->sector : -1;

				if (x1 <= L->Start(gDocument)->x() && L->Start(gDocument)->x() <= x2 &&
				    y1 <= L->Start(gDocument)->y() && L->Start(gDocument)->y() <= y2 &&
				    x1 <= L->End(gDocument)->x()   && L->End(gDocument)->x() <= x2 &&
				    y1 <= L->End(gDocument)->y()   && L->End(gDocument)->y() <= y2)
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

			for (int i = 0 ; i < NumSectors ; i++)
				if (in_sectors.get(i) && ! out_sectors.get(i))
					list->toggle(i);

			break;
		}
		default:
			break;
	}
}



void Selection_InvalidateLast()
{
	delete last_Sel;
	last_Sel = NULL;
}


void Selection_Push()
{
	if (edit.Selected->empty())
		return;

	if (last_Sel && last_Sel->test_equal(*edit.Selected))
		return;

	// OK copy it

	if (last_Sel)
		delete last_Sel;

	last_Sel = new selection_c(edit.Selected->what_type(), true);

	last_Sel->merge(*edit.Selected);
}


void Selection_Clear(bool no_save)
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


void Selection_Add(Objid& obj)
{
	// validate the mode is correct
	if (obj.type != edit.mode)
		return;

	if (obj.parts == 0)
	{
		// ignore the add if object is already set.
		// [ since the selection may have parts, and we don't want to
		//   forget those parts ]
		if (! edit.Selected->get(obj.num))
			edit.Selected->set(obj.num);
		return;
	}

	byte cur = edit.Selected->get_ext(obj.num);

	cur = 1 | obj.parts;

	edit.Selected->set_ext(obj.num, cur);
}


void Selection_Remove(Objid& obj)
{
	if (obj.type != edit.mode)
		return;

	if (obj.parts == 0)
	{
		edit.Selected->clear(obj.num);
		return;
	}

	byte cur = edit.Selected->get_ext(obj.num);
	if (cur == 0)
		return;

	cur = 1 | (cur & ~obj.parts);

	// if we unset all the parts, then unset the object itself
	if (cur == 1)
		cur = 0;

	edit.Selected->set_ext(obj.num, cur);
}


void Selection_Toggle(Objid& obj)
{
	if (obj.type != edit.mode)
		return;

	if (obj.parts == 0)
	{
		edit.Selected->toggle(obj.num);
		return;
	}

	byte cur = edit.Selected->get_ext(obj.num);

	// if object was simply selected, then just clear it
	if (cur == 1)
	{
		edit.Selected->clear(obj.num);
		return;
	}

	cur = 1 | (cur ^ obj.parts);

	// if we toggled off all the parts, then unset the object itself
	if (cur == 1)
		cur = 0;

	edit.Selected->set_ext(obj.num, cur);
}


void Selection_Validate()
{
	int num_obj = gDocument.numObjects(edit.mode);

	if (edit.Selected->max_obj() >= num_obj)
	{
		edit.Selected->frob_range(num_obj, edit.Selected->max_obj(), BitOp::remove);

		Beep("BUG: invalid selection");
	}
}


void CMD_LastSelection()
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

	std::swap(last_Sel, edit.Selected);

	// ensure everything is kosher
	Selection_Validate();

	if (changed_mode)
		GoToSelection();

	RedrawMap();
}


//------------------------------------------------------------------------
//  RECENTLY USED TEXTURES (etc)
//------------------------------------------------------------------------


// the containers for the textures (etc)
Recently_used  recent_textures;
Recently_used  recent_flats;
Recently_used  recent_things;


Recently_used::Recently_used() :
	size(0),
	keep_num(RECENTLY_USED_MAX - 2)
{
}


Recently_used::~Recently_used()
{
	for (int i = 0 ; i < size ; i++)
	{
		name_set[i].clear();
	}
}


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
	changed_recent_list = true;
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


void RecUsed_ClearAll()
{
	recent_textures.clear();
	recent_flats   .clear();
	recent_things  .clear();

	if (main_win)
		main_win->browser->RecentUpdate();
}


/* --- Save and Restore --- */

void Recently_used::WriteUser(std::ostream &os, char letter)
{
	for (int i = 0 ; i < size ; i++)
	{
		os << "recent_used " << letter << " \"" << name_set[i].getTidy() << "\"\n";
	}
}


void RecUsed_WriteUser(std::ostream &os)
{
	os << "\nrecent_used clear\n";
	
	recent_textures.WriteUser(os, 'T');
	recent_flats.WriteUser(os, 'F');
	recent_things.WriteUser(os, 'O');
}


bool RecUsed_ParseUser(const std::vector<SString> &tokens)
{
	if (tokens[0] != "recent_used" || tokens.size() < 2)
		return false;

	if (tokens[1] == "clear")
	{
		RecUsed_ClearAll();
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


void Editor_Init()
{
	switch (config::default_edit_mode)
	{
		case 1:  edit.mode = ObjType::linedefs; break;
		case 2:  edit.mode = ObjType::sectors;  break;
		case 3:  edit.mode = ObjType::vertices; break;
		default: edit.mode = ObjType::things;   break;
	}

	edit.render3d = false;

	Editor_DefaultState();

	Nav_Clear();

	edit.pointer_in_window = false;
	edit.map_x = 0;
	edit.map_y = 0;
	edit.map_z = -1;

	edit.Selected = new selection_c(edit.mode, true /* extended */);

	edit.highlight.clear();
	edit.split_line.clear();
	edit.clicked.clear();

	grid.Init();

	MadeChanges = 0;

	  Editor_RegisterCommands();
	Render3D_RegisterCommands();
}


void Editor_DefaultState()
{
	edit.action = ACT_NOTHING;
	edit.sticky_mod = 0;
	edit.is_panning = false;

	edit.dragged.clear();
	edit.draw_from.clear();

	edit.error_mode = false;
	edit.show_object_numbers = false;

	edit.sector_render_mode = config::sector_render_default;
	edit. thing_render_mode =  config::thing_render_default;
}


bool Editor_ParseUser(const std::vector<SString> &tokens)
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
void Editor_WriteUser(std::ostream &os)
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
