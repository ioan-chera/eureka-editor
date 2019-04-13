//------------------------------------------------------------------------
//  LEVEL MISC STUFF
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

#include "m_bitvec.h"
#include "m_game.h"
#include "e_hover.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_path.h"
#include "e_things.h"
#include "e_vertex.h"
#include "r_render.h"
#include "w_rawdef.h"

#include "ui_window.h"


Editor_State_t  edit;


int Map_bound_x1 =  32767;   /* minimum X value of map */
int Map_bound_y1 =  32767;   /* minimum Y value of map */
int Map_bound_x2 = -32767;   /* maximum X value of map */
int Map_bound_y2 = -32767;   /* maximum Y value of map */

int MadeChanges;

static bool recalc_map_bounds;
static int  new_vertex_minimum;
static int  moved_vertex_count;

static selection_c * last_Sel;

extern bool sound_propagation_invalid;


// config items
int default_edit_mode = 3;  // Vertices

bool same_mode_clears_selection = false;

int sector_render_default = (int)SREND_Floor;
int  thing_render_default = 1;


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
		xzoom = ScrMaxX / (double)(Map_bound_x2 - Map_bound_x1);

	if (Map_bound_y1 < Map_bound_y2)
		yzoom = ScrMaxY / (double)(Map_bound_y2 - Map_bound_y1);

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
	if (edit.action == ACT_DRAG && edit.drag_single_obj < 0)
		return;

	// in vertex mode, see if there is a linedef which would be split by
	// adding a new vertex

	if (edit.mode == OBJ_VERTICES &&
		edit.pointer_in_window && !edit.render3d &&
	    edit.highlight.is_nil())
	{
		GetSplitLineDef(edit.split_line, map_x, map_y, edit.drag_single_obj);

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
	    (!dragging || (edit.mode == OBJ_VERTICES && edit.drag_single_obj >= 0)))
	{
		GetNearObject(edit.highlight, edit.mode, edit.map_x, edit.map_y);

		// guarantee that we cannot drag a vertex onto itself
		if (edit.drag_single_obj >= 0 && edit.highlight.valid() &&
			edit.drag_single_obj == edit.highlight.num)
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


	Render3D_UpdateHighlight();

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

	SectorCache_Invalidate();
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
			Beep("Unknown mode: %c\n", mode_char);
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

	RedrawMap();
}


//------------------------------------------------------------------------


void UpdateLevelBounds(int start_vert)
{
	for (int i = start_vert ; i < NumVertices ; i++)
	{
		const Vertex * V = Vertices[i];

		if (V->x < Map_bound_x1) Map_bound_x1 = V->x;
		if (V->y < Map_bound_y1) Map_bound_y1 = V->y;

		if (V->x > Map_bound_x2) Map_bound_x2 = V->x;
		if (V->y > Map_bound_y2) Map_bound_y2 = V->y;
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

	Map_bound_x1 = 999999; Map_bound_x2 = -999999;
	Map_bound_y1 = 999999; Map_bound_y2 = -999999;

	UpdateLevelBounds(0);
}


void MapStuff_NotifyBegin()
{
	recalc_map_bounds  = false;
	new_vertex_minimum = -1;
	moved_vertex_count =  0;

	sound_propagation_invalid = true;
}

void MapStuff_NotifyInsert(obj_type_e type, int objnum)
{
	if (type == OBJ_VERTICES)
	{
		if (new_vertex_minimum < 0 || objnum < new_vertex_minimum)
			new_vertex_minimum = objnum;
	}
}

void MapStuff_NotifyDelete(obj_type_e type, int objnum)
{
	if (type == OBJ_VERTICES)
	{
		recalc_map_bounds = true;

		if (edit.action == ACT_DRAW_LINE &&
			edit.drawing_from == objnum)
		{
			Editor_ClearAction();
		}
	}
}

void MapStuff_NotifyChange(obj_type_e type, int objnum, int field)
{
	if (type == OBJ_VERTICES)
	{
		// NOTE: for performance reasons we don't recalculate the
		//       map bounds when only moving a few vertices.
		moved_vertex_count++;

		const Vertex * V = Vertices[objnum];

		if (V->x < Map_bound_x1) Map_bound_x1 = V->x;
		if (V->y < Map_bound_y1) Map_bound_y1 = V->y;

		if (V->x > Map_bound_x2) Map_bound_x2 = V->x;
		if (V->y > Map_bound_y2) Map_bound_y2 = V->y;

		SectorCache_Invalidate();
	}

	if (type == OBJ_SIDEDEFS && field == SideDef::F_SECTOR)
		SectorCache_Invalidate();

	if (type == OBJ_LINEDEFS && (field == LineDef::F_LEFT || field == LineDef::F_RIGHT))
		SectorCache_Invalidate();
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


void ObjectBox_NotifyInsert(obj_type_e type, int objnum)
{
	invalidated_totals = true;

	if (type != edit.mode)
		return;

	if (objnum > main_win->GetPanelObjNum())
		return;

	invalidated_panel_obj = true;
}


void ObjectBox_NotifyDelete(obj_type_e type, int objnum)
{
	invalidated_totals = true;

	if (type != edit.mode)
		return;

	if (objnum > main_win->GetPanelObjNum())
		return;

	invalidated_panel_obj = true;
}


void ObjectBox_NotifyChange(obj_type_e type, int objnum, int field)
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
static bool invalidated_3d_selection;
static bool invalidated_last_sel;


void Selection_NotifyBegin()
{
	invalidated_selection = false;
	invalidated_3d_selection = false;
	invalidated_last_sel  = false;
}


void Selection_NotifyInsert(obj_type_e type, int objnum)
{
	invalidated_3d_selection = true;

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


void Selection_NotifyDelete(obj_type_e type, int objnum)
{
	invalidated_3d_selection = true;

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


void Selection_NotifyChange(obj_type_e type, int objnum, int field)
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

	if (invalidated_3d_selection)
		Render3D_ClearSelection();

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

	selection_iterator_c it;

	for (list->begin(&it); ! it.at_end(); ++it)
		printf(" %d", *it);

	printf("\n");
}


void ConvertSelection(selection_c * src, selection_c * dest)
{
	if (src->what_type() == dest->what_type())
	{
		dest->merge(*src);
		return;
	}


	if (src->what_type() == OBJ_SECTORS && dest->what_type() == OBJ_THINGS)
	{
		// FIXME: get bbox of selection, skip things outside it

		for (int t = 0 ; t < NumThings ; t++)
		{
			const Thing *T = Things[t];

			// if (! thing_touches_bbox(T->x, T->y, 128, bbox))
			//    continue;

			Objid obj;  GetNearObject(obj, OBJ_SECTORS, T->x, T->y);

			if (! obj.is_nil() && src->get(obj.num))
			{
				dest->set(t);
			}
		}
		return;
	}


	if (src->what_type() == OBJ_SECTORS && dest->what_type() == OBJ_LINEDEFS)
	{
		for (int l = 0 ; l < NumLineDefs ; l++)
		{
			const LineDef *L = LineDefs[l];

			if ( (L->Right() && src->get(L->Right()->sector)) ||
				 (L->Left()  && src->get(L->Left()->sector)) )
			{
				dest->set(l);
			}
		}
		return;
	}


	if (src->what_type() == OBJ_SECTORS && dest->what_type() == OBJ_VERTICES)
	{
		for (int l = 0 ; l < NumLineDefs ; l++)
		{
			const LineDef *L = LineDefs[l];

			if ( (L->Right() && src->get(L->Right()->sector)) ||
				 (L->Left()  && src->get(L->Left()->sector)) )
			{
				dest->set(L->start);
				dest->set(L->end);
			}
		}
		return;
	}


	if (src->what_type() == OBJ_LINEDEFS && dest->what_type() == OBJ_SIDEDEFS)
	{
		selection_iterator_c it;

		for (src->begin(&it); ! it.at_end(); ++it)
		{
			const LineDef *L = LineDefs[*it];

			if (L->Right()) dest->set(L->right);
			if (L->Left())  dest->set(L->left);
		}
		return;
	}

	if (src->what_type() == OBJ_SECTORS && dest->what_type() == OBJ_SIDEDEFS)
	{
		for (int n = 0 ; n < NumSideDefs ; n++)
		{
			const SideDef * SD = SideDefs[n];

			if (src->get(SD->sector))
				dest->set(n);
		}
		return;
	}


	if (src->what_type() == OBJ_LINEDEFS && dest->what_type() == OBJ_VERTICES)
	{
		selection_iterator_c it;

		for (src->begin(&it); ! it.at_end(); ++it)
		{
			const LineDef *L = LineDefs[*it];

			dest->set(L->start);
			dest->set(L->end);
		}
		return;
	}


	if (src->what_type() == OBJ_VERTICES && dest->what_type() == OBJ_LINEDEFS)
	{
		// select all linedefs that have both ends selected
		for (int l = 0 ; l < NumLineDefs ; l++)
		{
			const LineDef *L = LineDefs[l];

			if (src->get(L->start) && src->get(L->end))
			{
				dest->set(l);
			}
		}
	}


	// remaining conversions are L->S and V->S

	if (dest->what_type() != OBJ_SECTORS)
		return;

	if (src->what_type() != OBJ_LINEDEFS && src->what_type() != OBJ_VERTICES)
		return;


	// step 1: select all sectors (except empty ones)
	int l;

	for (l = 0 ; l < NumLineDefs ; l++)
	{
		const LineDef *L = LineDefs[l];

		if (L->Right()) dest->set(L->Right()->sector);
		if (L->Left())  dest->set(L->Left()->sector);
	}

	// step 2: unselect any sectors if a component is not selected

	for (l = 0 ; l < NumLineDefs ; l++)
	{
		const LineDef *L = LineDefs[l];

		if (src->what_type() == OBJ_VERTICES)
		{
			if (src->get(L->start) && src->get(L->end))
				continue;
		}
		else
		{
			if (src->get(l))
				continue;
		}

		if (L->Right()) dest->clear(L->Right()->sector);
		if (L->Left())  dest->clear(L->Left()->sector);
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
	selection_iterator_c it;

	for (list->begin(&it); ! it.at_end(); ++it)
	{
		const LineDef *L = LineDefs[*it];

		if (L->TwoSided())
			return *it;
	}

	// return first entry (a one-sided line)
	return list->find_first();
}


//
// select all objects inside a given box
//
void SelectObjectsInBox(selection_c *list, int objtype, int x1, int y1, int x2, int y2)
{
	if (x2 < x1)
	{
		int tmp = x1; x1 = x2; x2 = tmp;
	}

	if (y2 < y1)
	{
		int tmp = y1; y1 = y2; y2 = tmp;
	}

	switch (objtype)
	{
		case OBJ_THINGS:
			for (int n = 0 ; n < NumThings ; n++)
			{
				const Thing *T = Things[n];

				if (x1 <= T->x && T->x <= x2 &&
				    y1 <= T->y && T->y <= y2)
				{
					list->toggle(n);
				}
			}
			break;

		case OBJ_VERTICES:
			for (int n = 0 ; n < NumVertices ; n++)
			{
				const Vertex *V = Vertices[n];

				if (x1 <= V->x && V->x <= x2 &&
				    y1 <= V->y && V->y <= y2)
				{
					list->toggle(n);
				}
			}
			break;

		case OBJ_LINEDEFS:
			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				const LineDef *L = LineDefs[n];

				/* the two ends of the line must be in the box */
				if (x1 <= L->Start()->x && L->Start()->x <= x2 &&
				    y1 <= L->Start()->y && L->Start()->y <= y2 &&
				    x1 <= L->End()->x   && L->End()->x <= x2 &&
				    y1 <= L->End()->y   && L->End()->y <= y2)
				{
					list->toggle(n);
				}
			}
			break;

		case OBJ_SECTORS:
		{
			selection_c  in_sectors(OBJ_SECTORS);
			selection_c out_sectors(OBJ_SECTORS);

			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				const LineDef *L = LineDefs[n];

				// Get the numbers of the sectors on both sides of the linedef
				int s1 = L->Right() ? L->Right()->sector : -1;
				int s2 = L->Left( ) ? L->Left() ->sector : -1;

				if (x1 <= L->Start()->x && L->Start()->x <= x2 &&
				    y1 <= L->Start()->y && L->Start()->y <= y2 &&
				    x1 <= L->End()->x   && L->End()->x <= x2 &&
				    y1 <= L->End()->y   && L->End()->y <= y2)
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
	}
}



void Selection_InvalidateLast()
{
	delete last_Sel; last_Sel = NULL;
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

	last_Sel = new selection_c(edit.Selected->what_type());

	last_Sel->merge(*edit.Selected);
}


void Selection_Clear(bool no_save)
{
	if (! no_save)
		Selection_Push();

	// this always clears it
	edit.Selected->change_type(edit.mode);

	edit.error_mode = false;

	Render3D_ClearSelection();

	RedrawMap();
}


void Selection_Validate()
{
	int num_obj = NumObjects(edit.mode);

	if (edit.Selected->max_obj() >= num_obj)
	{
		edit.Selected->frob_range(num_obj, edit.Selected->max_obj(), BOP_REMOVE);

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


//----------------------------------------------------------------------
//  3D CLIPBOARD
//----------------------------------------------------------------------

render_clipboard_c  r_clipboard;


render_clipboard_c::render_clipboard_c() :
	thing(0)
{
	tex [0] = 0;
	flat[0] = 0;
}

render_clipboard_c::~render_clipboard_c()
{ }


int render_clipboard_c::GetTexNum()
{
	if (tex[0] == 0)
		SetTex(default_wall_tex);

	return BA_InternaliseString(tex);
}

int render_clipboard_c::GetFlatNum()
{
	if (flat[0] == 0)
		SetFlat(default_floor_tex);

	return BA_InternaliseString(flat);
}

int render_clipboard_c::GetThing()
{
	if (thing == 0)
		thing = default_thing;

	return thing;
}


void render_clipboard_c::SetTex(const char *new_tex)
{
	snprintf(tex, sizeof(tex), "%s", new_tex);
}

void render_clipboard_c::SetFlat(const char *new_flat)
{
	snprintf(flat, sizeof(flat), "%s", new_flat);
}

void render_clipboard_c::SetThing(int new_id)
{
	thing = new_id;
}


bool render_clipboard_c::ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "r_clipboard") != 0)
		return false;

	if (strcmp(tokens[1], "tex") == 0)
		SetTex(tokens[2]);

	if (strcmp(tokens[1], "flat") == 0)
		SetFlat(tokens[2]);

	if (strcmp(tokens[1], "thing") == 0)
		thing = atoi(tokens[2]);

	return true;
}

void render_clipboard_c::WriteUser(FILE *fp)
{
	fprintf(fp, "r_clipboard tex \"%s\"\n",  StringTidy(tex,  "\""));
	fprintf(fp, "r_clipboard flat \"%s\"\n", StringTidy(flat, "\""));
	fprintf(fp, "r_clipboard thing %d\n",    thing);
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
	memset(name_set, 0, sizeof(name_set));
}


Recently_used::~Recently_used()
{
	for (int i = 0 ; i < size ; i++)
	{
		StringFree(name_set[i]);
		name_set[i] = NULL;
	}
}


int Recently_used::find(const char *name)
{
	for (int k = 0 ; k < size ; k++)
		if (y_stricmp(name_set[k], name) == 0)
			return k;

	return -1;	// not found
}

int Recently_used::find_number(int val)
{
	char buffer[64];

	snprintf(buffer, sizeof(buffer), "%d", val);

	return find(buffer);
}


void Recently_used::insert(const char *name)
{
	// ignore '-' texture
	if (is_null_tex(name))
		return;

	// ignore empty strings to prevent potential problems
	if (strlen(name) == 0)
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
	char buffer[64];

	snprintf(buffer, sizeof(buffer), "%d", val);

	insert(buffer);
}


void Recently_used::erase(int index)
{
	SYS_ASSERT(0 <= index && index < size);

	StringFree(name_set[index]);

	size--;

	for ( ; index < size ; index++)
	{
		name_set[index] = name_set[index + 1];
	}

	name_set[index] = NULL;
}


void Recently_used::push_front(const char *name)
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

	name_set[0] = StringDup(name);

	size++;
}


void Recently_used::clear()
{
	size = 0;

	memset(name_set, 0, sizeof(name_set));
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

void Recently_used::WriteUser(FILE *fp, char letter)
{
	for (int i = 0 ; i < size ; i++)
	{
		fprintf(fp, "recent_used %c \"%s\"\n", letter, StringTidy(name_set[i]));
	}
}


void RecUsed_WriteUser(FILE *fp)
{
	fprintf(fp, "\n");
	fprintf(fp, "recent_used clear\n");

	recent_textures.WriteUser(fp, 'T');
	recent_flats   .WriteUser(fp, 'F');
	recent_things  .WriteUser(fp, 'O');
}


bool RecUsed_ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "recent_used") != 0 || num_tok < 2)
		return false;

	if (strcmp(tokens[1], "clear") == 0)
	{
		RecUsed_ClearAll();
		return true;
	}

	// syntax is:  recent_used  <kind>  <name>
	if (num_tok < 3)
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
	switch (default_edit_mode)
	{
		case 1:  edit.mode = OBJ_LINEDEFS; break;
		case 2:  edit.mode = OBJ_SECTORS;  break;
		case 3:  edit.mode = OBJ_VERTICES; break;
		default: edit.mode = OBJ_THINGS;   break;
	}

	edit.render3d = false;

	Editor_DefaultState();

	Nav_Clear();

	edit.pointer_in_window = false;
	edit.map_x = 0;
	edit.map_y = 0;

	edit.Selected = new selection_c(edit.mode);

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
	edit.is_scrolling = false;

	edit.drawing_from = -1;
	edit.drag_single_obj = -1;

	edit.error_mode = false;
	edit.show_object_numbers = false;

	edit.sector_render_mode = sector_render_default;
	edit. thing_render_mode =  thing_render_default;
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
		RedrawMap();
		return true;
	}

	if (strcmp(tokens[0], "sector_render_mode") == 0 && num_tok >= 2)
	{
		edit.sector_render_mode = atoi(tokens[1]);
		RedrawMap();
		return true;
	}

	if (strcmp(tokens[0], "thing_render_mode") == 0 && num_tok >= 2)
	{
		edit.thing_render_mode = atoi(tokens[1]);
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
	fprintf(fp, "thing_render_mode %d\n",  edit.thing_render_mode);
	fprintf(fp, "show_object_numbers %d\n", edit.show_object_numbers ? 1 : 0);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
