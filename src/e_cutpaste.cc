//------------------------------------------------------------------------
//  LEVEL CUT 'N' PASTE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2009-2012 Andrew Apted
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

#include "main.h"

#include <map>

#include "e_basis.h"
#include "e_cutpaste.h"
#include "e_linedef.h"
#include "e_vertex.h"
#include "editloop.h"
#include "levels.h"
#include "objects.h"
#include "r_grid.h"
#include "selectn.h"
#include "w_rawdef.h"
#include "x_loop.h"


#define INVALID_SECTOR  (-999999)


class clipboard_data_c
{
public:
	// the mode used when the objects were Copied
	obj_type_e  mode;

	// NOTE:
	//
	// The references here normally refer to other objects in here
	// (as though this was a little self-contained map).  For example,
	// the LineDef::right field is an index into sides[].
	//
	// The exception is sector references in the SideDefs.  These
	// values refer to the real map when >= 0, and refer to local
	// sectors when < 0 (negate and add 1).
	//
	// That's because we generally want a copy'n'pasted piece of the
	// map (linedefs or sectors) to "fit in" with nearby sections of
	// the map.

	bool uses_real_sectors;

	std::vector<Thing *>   things;
	std::vector<Vertex *>  verts;
	std::vector<Sector *>  sectors;
	std::vector<SideDef *> sides;
	std::vector<LineDef *> lines;
	std::vector<RadTrig *> radtrigs;

public:
	clipboard_data_c(obj_type_e _mode) :
		mode(_mode), uses_real_sectors(false),
		things(), verts(), sectors(),
		sides(), lines(), radtrigs()
	{
		if (mode == OBJ_LINEDEFS || mode == OBJ_SECTORS)
			uses_real_sectors = true;
	}

	~clipboard_data_c()
	{
		unsigned int i;

		for (i = 0 ; i < things.size()   ; i++) delete things[i];
		for (i = 0 ; i < verts.size()    ; i++) delete verts[i];
		for (i = 0 ; i < sectors.size()  ; i++) delete sectors[i];
		for (i = 0 ; i < sides.size()    ; i++) delete sides[i];
		for (i = 0 ; i < lines.size()    ; i++) delete lines[i];
		for (i = 0 ; i < radtrigs.size() ; i++) delete radtrigs[i];
	}

	void CentreOfThings(int *cx, int *cy)
	{
		*cx = *cy = 0;

		if (things.empty())
			return;

		double sum_x = 0;
		double sum_y = 0;

		for (unsigned int i = 0 ; i < things.size() ; i++)
		{
			sum_x += things[i]->x;
			sum_y += things[i]->y;
		}

		sum_x /= (double)things.size();
		sum_y /= (double)things.size();

		// find closest vertex to this point
		double best_dist = 9e9;

		for (unsigned int k = 0 ; k < things.size() ; k++)
		{
			double dx = things[k]->x - sum_x;
			double dy = things[k]->y - sum_y;

			double dist = dx*dx + dy*dy;

			if (dist < best_dist)
			{
				*cx = things[k]->x;
				*cy = things[k]->y;

				best_dist = dist;
			}
		}
	}

	void CentreOfVertices(int *cx, int *cy)
	{
		*cx = *cy = 0;

		if (verts.empty())
			return;

		double sum_x = 0;
		double sum_y = 0;

		for (unsigned int i = 0 ; i < verts.size() ; i++)
		{
			sum_x += verts[i]->x;
			sum_y += verts[i]->y;
		}

		sum_x /= (double)verts.size();
		sum_y /= (double)verts.size();

		// find closest vertex to this point
		double best_dist = 9e9;

		for (unsigned int k = 0 ; k < verts.size() ; k++)
		{
			double dx = verts[k]->x - sum_x;
			double dy = verts[k]->y - sum_y;

			double dist = dx*dx + dy*dy;

			if (dist < best_dist)
			{
				*cx = verts[k]->x;
				*cy = verts[k]->y;

				best_dist = dist;
			}
		}
	}

	void CentreOfRadTrigs(int *cx, int *cy)
	{
		if (radtrigs.empty())
		{
			*cx = *cy = 0;
			return;
		}

		double sum_x = 0;
		double sum_y = 0;

		for (unsigned int i = 0 ; i < radtrigs.size() ; i++)
		{
			sum_x += radtrigs[i]->mx;
			sum_y += radtrigs[i]->my;
		}

		sum_x /= (double)radtrigs.size();
		sum_y /= (double)radtrigs.size();

		*cx = I_ROUND(sum_x);
		*cy = I_ROUND(sum_y);
	}

	bool HasSectorRefs(int s1, int s2)
	{
		if (! uses_real_sectors)
			return false;

		for (unsigned int i = 0 ; i < sides.size() ; i++)
		{
			if (s1 <= sides[i]->sector && sides[i]->sector <= s2)
				return true;
		}

		return false;
	}

	void InsertRealSector(int snum)
	{
		if (! uses_real_sectors)
			return;

		for (unsigned int i = 0 ; i < sides.size() ; i++)
		{
			if (sides[i]->sector >= snum)
				sides[i]->sector++;
		}
	}

	void DeleteRealSector(int snum)
	{
		if (! uses_real_sectors)
			return;

		for (unsigned int i = 0 ; i < sides.size() ; i++)
		{
			if (sides[i]->sector == snum)
				sides[i]->sector = INVALID_SECTOR;
			else if (sides[i]->sector > snum)
				sides[i]->sector--;
		}
	}

	void RemoveSectorRefs()
	{
		if (! uses_real_sectors)
			return;

		for (unsigned int i = 0 ; i < sides.size() ; i++)
		{
			if (sides[i]->sector >= 0)
				sides[i]->sector = INVALID_SECTOR;
		}
	}
};


static clipboard_data_c * clip_board;

static bool clip_doing_paste;


void Clipboard_Clear()
{
	if (clip_board)
	{
		delete clip_board;
		clip_board = NULL;
	}
}


/* this remove sidedefs which refer to local sectors, allowing the
   clipboard geometry to persist when changing maps.
 */
void Clipboard_ClearLocals()
{
	if (clip_board)
		clip_board->RemoveSectorRefs();
}


bool Clipboard_HasStuff()
{
	return clip_board ? true : false;
}


void Clipboard_NotifyBegin()
{ }

void Clipboard_NotifyEnd()
{ }


void Clipboard_NotifyInsert(obj_type_e type, int objnum)
{
	// this function notifies us that a new sector is about to be
	// inserted in the map (causing other sectors to be moved).

	if (type != OBJ_SECTORS)
		return;

	if (! clip_board)
		return;

	// paste operations should only insert new sectors at the end
	if (objnum < NumSectors)
	{
		SYS_ASSERT(! clip_doing_paste);
	}

#if 0  // OLD WAY
	if (clip_board->HasSectorRefs(objnum, NumSectors-1))
	{
		Clipboard_Clear();
	}
#else
	clip_board->InsertRealSector(objnum);
#endif
}


void Clipboard_NotifyDelete(obj_type_e type, int objnum)
{
	// this function notifies us that a sector is about to be deleted 
	// (causing other sectors to be moved).

	if (type != OBJ_SECTORS)
		return;

	if (! clip_board)
		return;

	SYS_ASSERT(! clip_doing_paste);

	clip_board->DeleteRealSector(objnum);
}


void Clipboard_NotifyChange(obj_type_e type, int objnum, int field)
{
	// field changes never affect the clipboard
}


//------------------------------------------------------------------------

static void CopyGroupOfObjects(selection_c *list)
{
	// this is used for LineDefs and Sectors, where we need to copy
	// groups of objects and create internal references between them.

	bool is_sectors = (list->what_type() == OBJ_SECTORS) ? true : false;

	selection_iterator_c it;

	selection_c vert_sel(OBJ_VERTICES);
	selection_c side_sel(OBJ_SIDEDEFS);
	selection_c line_sel(OBJ_LINEDEFS);

	ConvertSelection(list, &line_sel);
	ConvertSelection(&line_sel, &vert_sel);

	// determine needed sidedefs
	for (line_sel.begin(&it) ; !it.at_end() ; ++it)
	{
		LineDef *L = LineDefs[*it];

		if (L->right >= 0) side_sel.set(L->right);
		if (L->left  >= 0) side_sel.set(L->left);
	}


	// these hold the mapping from real index --> clipboard index
	std::map<int, int> vert_map;
	std::map<int, int> sector_map;
	std::map<int, int> side_map;


	for (vert_sel.begin(&it) ; !it.at_end() ; ++it)
	{
		vert_map[*it] = (int)clip_board->verts.size();

		Vertex * SD = new Vertex;
		SD->RawCopy(Vertices[*it]);
		clip_board->verts.push_back(SD);
	}

	if (is_sectors)
	{
		for (list->begin(&it) ; !it.at_end() ; ++it)
		{
			sector_map[*it] = (int)clip_board->sectors.size();

			Sector * S = new Sector;
			S->RawCopy(Sectors[*it]);
			clip_board->sectors.push_back(S);
		}
	}

	for (side_sel.begin(&it) ; !it.at_end() ; ++it)
	{
		side_map[*it] = (int)clip_board->sides.size();

		SideDef * SD = new SideDef;
		SD->RawCopy(SideDefs[*it]);
		clip_board->sides.push_back(SD);

		// adjust sector references, if needed
		if (is_sectors && list->get(SD->sector))
		{
			SYS_ASSERT(sector_map.find(SD->sector) != sector_map.end());
			SD->sector = -1 - sector_map[SD->sector];
		}
	}

	for (line_sel.begin(&it) ; !it.at_end() ; ++it)
	{
		LineDef * L = new LineDef;
		L->RawCopy(LineDefs[*it]);
		clip_board->lines.push_back(L);

		// adjust vertex references
		SYS_ASSERT(vert_map.find(L->start) != vert_map.end());
		SYS_ASSERT(vert_map.find(L->end)   != vert_map.end());

		L->start = vert_map[L->start];
		L->end   = vert_map[L->end  ];

		// adjust sidedef references
		if (L->right >= 0)
		{
			SYS_ASSERT(side_map.find(L->right) != side_map.end());
			L->right = side_map[L->right];
		}

		if (L->left >= 0)
		{
			SYS_ASSERT(side_map.find(L->left) != side_map.end());
			L->left = side_map[L->left];
		}
	}

	// in sectors mode, copy things too
	if (is_sectors)
	{
		selection_c thing_sel(OBJ_THINGS);

		ConvertSelection(list, &thing_sel);

		for (thing_sel.begin(&it) ; !it.at_end() ; ++it)
		{
			Thing * T = new Thing;
			T->RawCopy(Things[*it]);
			clip_board->things.push_back(T);
		}
	}
}


bool CMD_Copy()
{
	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
		return false;

	// create storage for the copied objects
	if (clip_board)
		delete clip_board;
	
	clip_board = new clipboard_data_c(edit.obj_type);

	switch (edit.obj_type)
	{
		case OBJ_THINGS:
			for (list.begin(&it) ; !it.at_end() ; ++it)
			{
				Thing * T = new Thing;
				T->RawCopy(Things[*it]);
				clip_board->things.push_back(T);
			}
			break;

		case OBJ_VERTICES:
			for (list.begin(&it) ; !it.at_end() ; ++it)
			{
				Vertex * V = new Vertex;
				V->RawCopy(Vertices[*it]);
				clip_board->verts.push_back(V);
			}
			break;

		case OBJ_RADTRIGS:
			for (list.begin(&it) ; !it.at_end() ; ++it)
			{
				RadTrig * R = new RadTrig;
				R->RawCopy(RadTrigs[*it]);
				clip_board->radtrigs.push_back(R);
			}
			break;

		case OBJ_LINEDEFS:
		case OBJ_SECTORS:
			CopyGroupOfObjects(&list);
			break;

		default:
			return false;
	}

	return true;
}


//------------------------------------------------------------------------

static void PasteGroupOfObjects(int pos_x, int pos_y)
{
	int cx, cy;
	clip_board->CentreOfVertices(&cx, &cy);

	// these hold the mapping from clipboard index --> real index
	std::map<int, int> vert_map;
	std::map<int, int> sector_map;
	std::map<int, int> side_map;

	unsigned int i;

	for (i = 0 ; i < clip_board->verts.size() ; i++)
	{
		int new_v = BA_New(OBJ_VERTICES);
		Vertex * V = Vertices[new_v];

		vert_map[i] = new_v;

		V->RawCopy(clip_board->verts[i]);

		V->x += pos_x - cx;
		V->y += pos_y - cy;
	}

	for (i = 0 ; i < clip_board->sectors.size() ; i++)
	{
		int new_s = BA_New(OBJ_SECTORS);
		Sector * S = Sectors[new_s];

		sector_map[i] = new_s;

		S->RawCopy(clip_board->sectors[i]);
	}

	for (i = 0 ; i < clip_board->sides.size() ; i++)
	{
		// handle invalidated sectors (as if sidedef had been deleted)
		if (clip_board->sides[i]->sector == INVALID_SECTOR)
		{
			side_map[i] = -1;
			continue;
		}

		int new_sd = BA_New(OBJ_SIDEDEFS);
		SideDef * SD = SideDefs[new_sd];

		side_map[i] = new_sd;

		SD->RawCopy(clip_board->sides[i]);

		if (SD->sector < 0)
		{
			int local = -1 - SD->sector;
			SYS_ASSERT(sector_map.find(local) != sector_map.end());
			SD->sector = sector_map[local];
		}
	}

	for (i = 0 ; i < clip_board->lines.size() ; i++)
	{
		int new_l = BA_New(OBJ_LINEDEFS);
		LineDef * L = LineDefs[new_l];

		L->RawCopy(clip_board->lines[i]);

		// adjust vertex references
		SYS_ASSERT(vert_map.find(L->start) != vert_map.end());
		SYS_ASSERT(vert_map.find(L->end)   != vert_map.end());

		L->start = vert_map[L->start];
		L->end   = vert_map[L->end  ];

		// adjust sidedef references
		if (L->Right())
		{
			SYS_ASSERT(side_map.find(L->right) != side_map.end());
			L->right = side_map[L->right];
		}

		if (L->Left())
		{
			SYS_ASSERT(side_map.find(L->left) != side_map.end());
			L->left = side_map[L->left];
		}

		// flip linedef if necessary
		if (L->Left() && ! L->Right())
		{
			FlipLineDef(new_l);
		}

		// if the linedef lost a side, fix texturing
		if (L->OneSided() && L->Right()->MidTex()[0] == '-')
		{
			int tex;

			if (L->Right()->LowerTex()[0] != '-')
				tex = L->Right()->lower_tex;
			else if (L->Right()->UpperTex()[0] != '-')
				tex = L->Right()->upper_tex;
			else
				tex = BA_InternaliseString(default_mid_tex);

			BA_ChangeSD(L->right, SideDef::F_MID_TEX, tex);
		}
	}

	for (i = 0 ; i < clip_board->things.size() ; i++)
	{
		int new_t = BA_New(OBJ_THINGS);
		Thing * T = Things[new_t];

		T->RawCopy(clip_board->things[i]);

		T->x += pos_x - cx;
		T->y += pos_y - cy;
	}
}


static void ReselectGroup()
{
	// this assumes all the new objects are at the end of their
	// array (currently true, but not a guarantee of BA_New).

	if (edit.obj_type == OBJ_THINGS)
	{
		if (clip_board->mode == OBJ_THINGS ||
		    clip_board->mode == OBJ_SECTORS)
		{
			int count = (int)clip_board->things.size();

			edit.Selected->clear_all();
			edit.Selected->frob_range(NumThings - count, NumThings-1, BOP_ADD);
		}
		return;
	}

	bool was_mappy = (clip_board->mode == OBJ_VERTICES ||
			    	  clip_board->mode == OBJ_LINEDEFS ||
					  clip_board->mode == OBJ_SECTORS);

	bool is_mappy =  (edit.obj_type == OBJ_VERTICES ||
			    	  edit.obj_type == OBJ_LINEDEFS ||
					  edit.obj_type == OBJ_SECTORS);

	// we don't bother to reselect RTS triggers
	if (! (was_mappy && is_mappy))
		return;

	selection_c new_sel(clip_board->mode);

	if (clip_board->mode == OBJ_VERTICES)
	{
		int count = (int)clip_board->verts.size();
		new_sel.frob_range(NumVertices - count, NumVertices-1, BOP_ADD);
	}
	else if (clip_board->mode == OBJ_LINEDEFS)
	{
		// Note: this doesn't behave as expected if the editing mode is
		// SECTORS, because the pasted lines do not completely surround
		// the sectors (non-pasted lines refer to them too).

		int count = (int)clip_board->lines.size();
		new_sel.frob_range(NumLineDefs - count, NumLineDefs-1, BOP_ADD);
	}
	else
	{
		SYS_ASSERT(clip_board->mode == OBJ_SECTORS);

		int count = (int)clip_board->sectors.size();
		new_sel.frob_range(NumSectors - count, NumSectors-1, BOP_ADD);
	}

	edit.Selected->clear_all();
	ConvertSelection(&new_sel, edit.Selected);
}


bool CMD_Paste()
{
	bool reselect = true;  // CONFIG TODO

	unsigned int i;

	if (! Clipboard_HasStuff())
		return false;

	// figure out where to put stuff
	int pos_x = edit.map_x;
	int pos_y = edit.map_y;

	if (! edit.pointer_in_window)
	{
		pos_x = I_ROUND(grid.orig_x);
		pos_y = I_ROUND(grid.orig_y);
	}

	// honor the grid snapping setting
	pos_x = grid.SnapX(pos_x);
	pos_y = grid.SnapY(pos_y);

	BA_Begin();

	clip_doing_paste = true;

	switch (clip_board->mode)
	{
		case OBJ_THINGS:
		{
			int cx, cy;
			clip_board->CentreOfThings(&cx, &cy);

			for (unsigned int i = 0 ; i < clip_board->things.size() ; i++)
			{
				int new_t = BA_New(OBJ_THINGS);
				Thing * T = Things[new_t];

				T->RawCopy(clip_board->things[i]);

				T->x += pos_x - cx;
				T->y += pos_y - cy;
			}
			break;
		}
		
		case OBJ_VERTICES:
		{
			int cx, cy;
			clip_board->CentreOfVertices(&cx, &cy);

			for (i = 0 ; i < clip_board->verts.size() ; i++)
			{
				int new_v = BA_New(OBJ_VERTICES);
				Vertex * V = Vertices[new_v];
				
				V->RawCopy(clip_board->verts[i]);

				V->x += pos_x - cx;
				V->y += pos_y - cy;
			}
			break;
		}
		
		case OBJ_RADTRIGS:
		{
			int cx, cy;
			clip_board->CentreOfRadTrigs(&cx, &cy);

			for (i = 0 ; i < clip_board->radtrigs.size() ; i++)
			{
				int new_r = BA_New(OBJ_RADTRIGS);
				RadTrig * R = RadTrigs[new_r];
				
				R->RawCopy(clip_board->radtrigs[i]);

				R->mx += pos_x - cx;
				R->my += pos_y - cy;

				// don't copy the RTS code string
				R->code = 0;
			}
			break;
		}
		
		case OBJ_LINEDEFS:
		case OBJ_SECTORS:
		{
			PasteGroupOfObjects(pos_x, pos_y);
			break;
		}

		default:
			break;
	}

	clip_doing_paste = false;

	BA_End();

	if (reselect)
		ReselectGroup();

	return true;
}


//------------------------------------------------------------------------

void UnusedVertices(selection_c *lines, selection_c *result)
{
	SYS_ASSERT(lines->what_type() == OBJ_LINEDEFS);

	ConvertSelection(lines, result);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		// we are interested in the lines we are NOT deleting
		if (lines->get(n))
			continue;
	
		const LineDef *L = LineDefs[n];

		result->clear(L->start);
		result->clear(L->end);
	}
}

void UnusedSideDefs(selection_c *lines, selection_c *result)
{
	SYS_ASSERT(lines->what_type() == OBJ_LINEDEFS);

	ConvertSelection(lines, result);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		// we are interested in the lines we are NOT deleting
		if (lines->get(n))
			continue;
	
		const LineDef *L = LineDefs[n];

		if (L->Right()) result->clear(L->right);
		if (L->Left())  result->clear(L->left);
	}
}

void UnusedLineDefs(selection_c *sectors, selection_c *result)
{
	SYS_ASSERT(sectors->what_type() == OBJ_SECTORS);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		// check if touches a to-be-deleted sector
		//    -1 : no side
		//     0 : deleted side
		//    +1 : kept side
		int right_m = (L->right < 0) ? -1 : sectors->get(L->Right()->sector) ? 0 : 1;
		int  left_m = (L->left  < 0) ? -1 : sectors->get(L->Left() ->sector) ? 0 : 1;

		if (MAX(right_m, left_m) == 0)
		{
			result->set(n);
		}
	}
}


static void FixupLineDefs(selection_c *lines, selection_c *sectors)
{
	selection_iterator_c it;

	for (lines->begin(&it) ; !it.at_end() ; ++it)
	{
		const LineDef *L = LineDefs[*it];

		// the logic is ugly here mainly to handle flipping (in particular,
		// not to flip the line when _both_ sides are unlinked).

		bool do_right = L->Right() ? sectors->get(L->Right()->sector) : false;
		bool do_left  = L->Left()  ? sectors->get(L->Left() ->sector) : false;

		// line shouldn't be in list unless it touches the sector
		SYS_ASSERT(do_right || do_left);

		if (do_right && do_left)
		{
			LD_RemoveSideDef(*it, SIDE_RIGHT);
			LD_RemoveSideDef(*it, SIDE_LEFT);
		}
		else if (do_right)
		{
			LD_RemoveSideDef(*it, SIDE_RIGHT);
			FlipLineDef(*it);
		}
		else // do_left
		{
			LD_RemoveSideDef(*it, SIDE_LEFT);
		}
	}
}


static void DeleteVertex_MergeLineDefs(int v_num)
{
	// find the linedefs
	int ld1 = -1;
	int ld2 = -1;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		if (L->start == v_num || L->end == v_num)
		{
			SYS_ASSERT(ld2 < 0);

			if (ld1 < 0)
				ld1 = n;
			else
				ld2 = n;
		}
	}

	SYS_ASSERT(ld1 >= 0);
	SYS_ASSERT(ld2 >= 0);

	LineDef *L1 = LineDefs[ld1];
	LineDef *L2 = LineDefs[ld2];

	// TODO: if (Length(ld1) < Length(ld2)) std::swap(ld1, ld2);

	// determine the other remaining vertex
	int v_other;

	if (L2->start == v_num)
		v_other = L2->end;
	else
		v_other = L2->start;

	if (L1->start == v_num)
		BA_ChangeLD(ld1, LineDef::F_START, v_other);
	else
		BA_ChangeLD(ld1, LineDef::F_END, v_other);

	// FIXME: update X offsets on existing linedef

	BA_Delete(OBJ_LINEDEFS, ld2);
	BA_Delete(OBJ_VERTICES, v_num);
}


void CMD_Delete(void)
{
	selection_c list;

	if (! GetCurrentObjects(&list))
	{
		Beep("Nothing to delete");
		return;
	}

	bool keep = (tolower(EXEC_Param[0][0]) == 'k');

	// TODO: decide how to differentiate these (if at all)
	bool keep_things = keep;
	bool keep_unused = keep;

	selection_c vert_sel(OBJ_VERTICES);
	selection_c side_sel(OBJ_SIDEDEFS);
	selection_c line_sel(OBJ_LINEDEFS);

	// special case for a single vertex connected to two linedef,
	// we delete the vertex but merge the two linedefs.
	if (list.what_type() == OBJ_VERTICES && list.count_obj() == 1)
	{
		int v_num = list.find_first();
		SYS_ASSERT(v_num >= 0);

		if (VertexHowManyLineDefs(v_num) == 2)
		{
			BA_Begin();

			DeleteVertex_MergeLineDefs(v_num);

			BA_End();

			goto success;
		}
	}

	BA_Begin();

	// delete things from each deleted sector
	if (!keep_things && list.what_type() == OBJ_SECTORS)
	{
		selection_c thing_sel(OBJ_THINGS);

		ConvertSelection(&list, &thing_sel);

		DeleteObjects(&thing_sel);
	}

	if (!keep_unused && list.what_type() == OBJ_SECTORS)
	{
		UnusedLineDefs(&list, &line_sel);

		if (line_sel.notempty())
		{
			UnusedVertices(&line_sel, &vert_sel);
			UnusedSideDefs(&line_sel, &side_sel);
		}
	}

	// perform linedef fixups here
	if (list.what_type() == OBJ_SECTORS)
	{
		selection_c fixups(OBJ_LINEDEFS);

		ConvertSelection(&list, &fixups);

		// skip lines which will get deleted
		fixups.unmerge(line_sel);

		FixupLineDefs(&fixups, &list);
	}

	if (!keep_unused && list.what_type() == OBJ_LINEDEFS)
	{
		UnusedVertices(&list, &vert_sel);
		UnusedSideDefs(&list, &side_sel);
	}

	DeleteObjects(&side_sel);

	DeleteObjects(&list);

	DeleteObjects(&line_sel);
	DeleteObjects(&vert_sel);

	BA_End();

success:
	edit.Selected->clear_all();
	edit.highlighted.clear();
	edit.split_line.clear();

	UpdateHighlight();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
