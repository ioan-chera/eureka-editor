//------------------------------------------------------------------------
//  LEVEL CUT 'N' PASTE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2009-2019 Andrew Apted
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

#include "Instance.h"

#include "LineDef.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"

#define INVALID_SECTOR  (-999999)


class clipboard_data_c
{
public:
	// the mode used when the objects were Copied
	const ObjType mode;

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

	const bool uses_real_sectors;

	std::vector<Thing>   things;
	std::vector<Vertex>  verts;
	std::vector<Sector>  sectors;
	std::vector<SideDef> sides;
	std::vector<LineDef> lines;

public:
	explicit clipboard_data_c(ObjType mode) : mode(mode),
			uses_real_sectors(mode == ObjType::linedefs || mode == ObjType::sectors)
	{
	}

	int TotalSize() const
	{
		size_t num = things.size() + verts.size() + sectors.size() + sides.size() + lines.size();

		return (int)num;
	}

	void Paste_BA_Message(EditOperation &op) const
	{
		size_t t = things.size();
		size_t v = verts.size();
		size_t s = sectors.size();
		size_t l = lines.size();

		if (s > 0)
		{
			const char *name = NameForObjectType(ObjType::sectors, s != 1);
			op.setMessage("pasted %zu %s", s, name);
		}
		else if (l > 0)
		{
			const char *name = NameForObjectType(ObjType::linedefs, l != 1);
			op.setMessage("pasted %zu %s", l, name);
		}
		else if (t > 0)
		{
			const char *name = NameForObjectType(ObjType::things, t != 1);
			op.setMessage("pasted %zu %s", t, name);
		}
		else if (v > 0)
		{
			const char *name = NameForObjectType(ObjType::vertices, v != 1);
			op.setMessage("pasted %zu %s", v, name);
		}
		else
		{
			op.setMessage("pasted something");
		}
	}

	void InsertRealSector(int snum)
	{
		if (! uses_real_sectors)
			return;

		for (SideDef &side : sides)
		{
			if (side.sector >= snum)
				side.sector++;
		}
	}

	void DeleteRealSector(int snum)
	{
		if (! uses_real_sectors)
			return;

		for (SideDef &side : sides)
		{
			if (side.sector == snum)
				side.sector = INVALID_SECTOR;
			else if (side.sector > snum)
				side.sector--;
		}
	}

	void RemoveSectorRefs()
	{
		if (! uses_real_sectors)
			return;

		for (SideDef &side : sides)
		{
			if (side.sector >= 0)
				side.sector = INVALID_SECTOR;
		}
	}
};


static clipboard_data_c * clip_board;

static bool clip_doing_paste;


//
// this remove sidedefs which refer to local sectors, allowing the
// clipboard geometry to persist when changing maps.
//
void Clipboard_ClearLocals()
{
	if (clip_board)
		clip_board->RemoveSectorRefs();
}


static bool Clipboard_HasStuff()
{
	return clip_board ? true : false;
}


void Clipboard_NotifyBegin()
{ }

void Clipboard_NotifyEnd()
{ }


void Clipboard_NotifyInsert(const Document &doc, ObjType type, int objnum)
{
	// this function notifies us that a new sector is about to be
	// inserted in the map (causing other sectors to be moved).

	if (type != ObjType::sectors)
		return;

	if (! clip_board)
		return;

	// paste operations should only insert new sectors at the end
	if (objnum < doc.numSectors())
	{
		SYS_ASSERT(! clip_doing_paste);
	}

	clip_board->InsertRealSector(objnum);
}


void Clipboard_NotifyDelete(ObjType type, int objnum)
{
	// this function notifies us that a sector is about to be deleted
	// (causing other sectors to be moved).

	if (type != ObjType::sectors)
		return;

	if (! clip_board)
		return;

	SYS_ASSERT(! clip_doing_paste);

	clip_board->DeleteRealSector(objnum);
}


void Clipboard_NotifyChange(ObjType type, int objnum, int field)
{
	// field changes never affect the clipboard
}


//----------------------------------------------------------------------
//  Texture Clipboard
//----------------------------------------------------------------------

namespace tex_clipboard
{
	SString tex;
	SString flat;

	int thing;
};


void Texboard_Clear()
{
	tex_clipboard::tex.clear();
	tex_clipboard::flat.clear();
	tex_clipboard::thing = 0;
}

StringID Texboard_GetTexNum(const ConfigData &config)
{
	if (tex_clipboard::tex.empty())
		tex_clipboard::tex = config.default_wall_tex;

	return BA_InternaliseString(tex_clipboard::tex);
}

StringID Texboard_GetFlatNum(const ConfigData &config)
{
	if (tex_clipboard::flat.empty())
		tex_clipboard::flat = config.default_floor_tex;

	return BA_InternaliseString(tex_clipboard::flat);
}

int Texboard_GetThing(const ConfigData &config)
{
	if (tex_clipboard::thing == 0)
		return config.default_thing;

	return tex_clipboard::thing;
}

void Texboard_SetTex(const SString &new_tex, const ConfigData &config)
{
	tex_clipboard::tex = new_tex;

	if (config.features.mix_textures_flats)
		tex_clipboard::flat = new_tex;
}

void Texboard_SetFlat(const SString &new_flat, const ConfigData &config)
{
	tex_clipboard::flat = new_flat;

	if (config.features.mix_textures_flats)
		tex_clipboard::tex = new_flat;
}

void Texboard_SetThing(int new_id)
{
	tex_clipboard::thing = new_id;
}


//------------------------------------------------------------------------

static void CopyGroupOfObjects(const Document &doc, const selection_c &list)
{
	// this is used for LineDefs and Sectors, where we need to copy
	// groups of objects and create internal references between them.

	bool is_sectors = list.what_type() == ObjType::sectors;

	selection_c vert_sel(ObjType::vertices);
	selection_c side_sel(ObjType::sidedefs);
	selection_c line_sel(ObjType::linedefs);

	ConvertSelection(doc, list, line_sel);
	ConvertSelection(doc, line_sel, vert_sel);

	// determine needed sidedefs
	for (sel_iter_c it(line_sel) ; !it.done() ; it.next())
	{
		const auto L = doc.linedefs[*it];

		if (L->right >= 0) side_sel.set(L->right);
		if (L->left  >= 0) side_sel.set(L->left);
	}


	// these hold the mapping from real index --> clipboard index
	std::map<int, int> vert_map;
	std::map<int, int> sector_map;
	std::map<int, int> side_map;


	for (sel_iter_c it(vert_sel) ; !it.done() ; it.next())
	{
		vert_map[*it] = (int)clip_board->verts.size();

		clip_board->verts.push_back(*doc.vertices[*it]);
	}

	if (is_sectors)
	{
		for (sel_iter_c it(list) ; !it.done() ; it.next())
		{
			sector_map[*it] = (int)clip_board->sectors.size();

			clip_board->sectors.push_back(*doc.sectors[*it]);
		}
	}

	for (sel_iter_c it(side_sel) ; !it.done() ; it.next())
	{
		side_map[*it] = (int)clip_board->sides.size();

		clip_board->sides.push_back(*doc.sidedefs[*it]);
		SideDef &SD = clip_board->sides.back();

		// adjust sector references, if needed
		if (is_sectors && list.get(SD.sector))
		{
			SYS_ASSERT(sector_map.find(SD.sector) != sector_map.end());
			SD.sector = -1 - sector_map[SD.sector];
		}
	}

	for (sel_iter_c it(line_sel) ; !it.done() ; it.next())
	{
		clip_board->lines.push_back(*doc.linedefs[*it]);
		LineDef &L = clip_board->lines.back();

		// adjust vertex references
		SYS_ASSERT(vert_map.find(L.start) != vert_map.end());
		SYS_ASSERT(vert_map.find(L.end)   != vert_map.end());

		L.start = vert_map[L.start];
		L.end   = vert_map[L.end  ];

		// adjust sidedef references
		if (L.right >= 0)
		{
			SYS_ASSERT(side_map.find(L.right) != side_map.end());
			L.right = side_map[L.right];
		}

		if (L.left >= 0)
		{
			SYS_ASSERT(side_map.find(L.left) != side_map.end());
			L.left = side_map[L.left];
		}
	}

	// in sectors mode, copy things too
	if (is_sectors)
	{
		selection_c thing_sel(ObjType::things);

		ConvertSelection(doc, list, thing_sel);

		for (sel_iter_c it(thing_sel) ; !it.done() ; it.next())
			clip_board->things.push_back(*doc.things[*it]);
	}
}


bool Instance::Clipboard_DoCopy()
{
	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
		return false;

	// create storage for the copied objects
	if (clip_board)
		delete clip_board;

	bool result = true;

	clip_board = new clipboard_data_c(edit.mode);

	switch (edit.mode)
	{
		case ObjType::things:
			for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
				clip_board->things.push_back(*level.things[*it]);
			break;

		case ObjType::vertices:
			for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
				clip_board->verts.push_back(*level.vertices[*it]);
			break;

		case ObjType::linedefs:
		case ObjType::sectors:
			CopyGroupOfObjects(level, *edit.Selected);
			break;

		default:
			result = false;
			break;
	}

	int total = edit.Selected->count_obj();
	if (total == 1)
		Status_Set("copied %s #%d", NameForObjectType(edit.Selected->what_type()), edit.Selected->find_first());
	else
		Status_Set("copied %d %s", total, NameForObjectType(edit.Selected->what_type(), true /* plural */));

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);

	return result;
}


//------------------------------------------------------------------------

template<typename T>
static v2double_t CentreOfPointObjects(const std::vector<T> &list)
{
	if (list.empty())
		return {};

	v2double_t sum = {};

	for (const T &object : list)
		sum += object.xy();

	sum /= (double)list.size();

	return sum;
}


static void PasteGroupOfObjects(EditOperation &op, MapFormat format, const v2double_t &pos)
{
	v2double_t cpos = CentreOfPointObjects(clip_board->verts);

	// these hold the mapping from clipboard index --> real index
	std::map<int, int> vert_map;
	std::map<int, int> sector_map;
	std::map<int, int> side_map;

	unsigned int i;

	for (i = 0 ; i < clip_board->verts.size() ; i++)
	{
		int new_v = op.addNew(ObjType::vertices);
		auto V = op.doc.vertices[new_v];

		vert_map[i] = new_v;

		*V = clip_board->verts[i];

		V->SetRawXY(format, V->xy() + pos - cpos);
	}

	for (i = 0 ; i < clip_board->sectors.size() ; i++)
	{
		int new_s = op.addNew(ObjType::sectors);
		auto S = op.doc.sectors[new_s];

		sector_map[i] = new_s;

		*S = clip_board->sectors[i];
	}

	for (i = 0 ; i < clip_board->sides.size() ; i++)
	{
		// handle invalidated sectors (as if sidedef had been deleted)
		if (clip_board->sides[i].sector == INVALID_SECTOR)
		{
			side_map[i] = -1;
			continue;
		}

		int new_sd = op.addNew(ObjType::sidedefs);
		auto SD = op.doc.sidedefs[new_sd];

		side_map[i] = new_sd;

		*SD = clip_board->sides[i];

		if (SD->sector < 0)
		{
			int local = -1 - SD->sector;
			SYS_ASSERT(sector_map.find(local) != sector_map.end());
			SD->sector = sector_map[local];
		}
	}

	for (i = 0 ; i < clip_board->lines.size() ; i++)
	{
		int new_l = op.addNew(ObjType::linedefs);
		auto L = op.doc.linedefs[new_l];

		*L = clip_board->lines[i];

		// adjust vertex references
		SYS_ASSERT(vert_map.find(L->start) != vert_map.end());
		SYS_ASSERT(vert_map.find(L->end)   != vert_map.end());

		L->start = vert_map[L->start];
		L->end   = vert_map[L->end  ];

		// adjust sidedef references
		if (op.doc.getRight(*L))
		{
			SYS_ASSERT(side_map.find(L->right) != side_map.end());
			L->right = side_map[L->right];
		}

		if (op.doc.getLeft(*L))
		{
			SYS_ASSERT(side_map.find(L->left) != side_map.end());
			L->left = side_map[L->left];
		}

		// flip linedef if necessary
		if (op.doc.getLeft(*L) && ! op.doc.getRight(*L))
		{
			op.doc.linemod.flipLinedef(op, new_l);
		}

		// if the linedef lost a side, fix texturing
		if (L->OneSided() && is_null_tex(op.doc.getRight(*L)->MidTex()))
			op.doc.linemod.fixForLostSide(op, new_l);
	}

	for (i = 0 ; i < clip_board->things.size() ; i++)
	{
		int new_t = op.addNew(ObjType::things);
		auto T = op.doc.things[new_t];

		*T = clip_board->things[i];

		T->SetRawXY(format, T->xy() + pos - cpos);
	}
}


void Instance::ReselectGroup()
{
	// this assumes all the new objects are at the end of their
	// array (currently true, but not a guarantee of BA_New).

	if (edit.mode == ObjType::things)
	{
		if (clip_board->mode == ObjType::things ||
		    clip_board->mode == ObjType::sectors)
		{
			int count = (int)clip_board->things.size();

			Selection_Clear();

			edit.Selected->frob_range(level.numThings() - count, level.numThings()-1, BitOp::add);
		}
		return;
	}

	bool was_mappy = (clip_board->mode == ObjType::vertices ||
			    	  clip_board->mode == ObjType::linedefs ||
					  clip_board->mode == ObjType::sectors);

	bool is_mappy =  (edit.mode == ObjType::vertices ||
			    	  edit.mode == ObjType::linedefs ||
					  edit.mode == ObjType::sectors);

	if (! (was_mappy && is_mappy))
		return;

	selection_c new_sel(clip_board->mode);

	if (clip_board->mode == ObjType::vertices)
	{
		int count = (int)clip_board->verts.size();
		new_sel.frob_range(level.numVertices() - count, level.numVertices() -1, BitOp::add);
	}
	else if (clip_board->mode == ObjType::linedefs)
	{
		// Note: this doesn't behave as expected if the editing mode is
		// SECTORS, because the pasted lines do not completely surround
		// the sectors (non-pasted lines refer to them too).

		int count = (int)clip_board->lines.size();
		new_sel.frob_range(level.numLinedefs() - count, level.numLinedefs() -1, BitOp::add);
	}
	else
	{
		SYS_ASSERT(clip_board->mode == ObjType::sectors);

		int count = (int)clip_board->sectors.size();
		new_sel.frob_range(level.numSectors() - count, level.numSectors() -1, BitOp::add);
	}

	Selection_Clear();

	ConvertSelection(level, new_sel, *edit.Selected);
}


bool Instance::Clipboard_DoPaste()
{
	bool reselect = true;  // CONFIG TODO

	unsigned int i;

	if (! Clipboard_HasStuff())
		return false;

	// figure out where to put stuff
	v2double_t pos = edit.map.xy;

	if (! edit.pointer_in_window)
	{
		pos = grid.getOrig();
	}

	// honor the grid snapping setting
	pos = grid.Snap(pos);

	{
		EditOperation op(level.basis);
		clip_board->Paste_BA_Message(op);

		clip_doing_paste = true;

		switch (clip_board->mode)
		{
			case ObjType::things:
			{
				v2double_t cpos = CentreOfPointObjects(clip_board->things);

				for (unsigned int i = 0 ; i < clip_board->things.size() ; i++)
				{
					int new_t = op.addNew(ObjType::things);
					auto T = level.things[new_t];

					*T = clip_board->things[i];

					T->SetRawXY(loaded.levelFormat, T->xy() + pos - cpos);

					recent_things.insert_number(T->type);
				}
				break;
			}

			case ObjType::vertices:
			{
				v2double_t cpos = CentreOfPointObjects(clip_board->verts);

				for (i = 0 ; i < clip_board->verts.size() ; i++)
				{
					int new_v = op.addNew(ObjType::vertices);
					auto V = level.vertices[new_v];

					*V = clip_board->verts[i];

					V->SetRawXY(loaded.levelFormat, V->xy() + pos - cpos);
				}
				break;
			}

			case ObjType::linedefs:
			case ObjType::sectors:
			{
				PasteGroupOfObjects(op, loaded.levelFormat, pos);
				break;
			}

			default:
				break;
		}

		clip_doing_paste = false;

	}

	edit.error_mode = false;

	if (reselect)
		ReselectGroup();

	return true;
}


//------------------------------------------------------------------------

void Instance::CMD_CopyAndPaste()
{
	if (edit.Selected->empty() && edit.highlight.is_nil())
	{
		Beep("Nothing to copy and paste");
		return;
	}

	if (Clipboard_DoCopy())
	{
		Clipboard_DoPaste();
	}
}


void Instance::CMD_Clipboard_Cut()
{
	if (main_win->ClipboardOp(EditCommand::cut))
		return;

	if (edit.render3d && edit.mode != ObjType::things)
	{
		Render3D_CB_Cut();
		return;
	}

	if (! Clipboard_DoCopy())
	{
		Beep("Nothing to cut");
		return;
	}

	ExecuteCommand("Delete");
}


void Instance::CMD_Clipboard_Copy()
{
	if (main_win->ClipboardOp(EditCommand::copy))
		return;

	if (edit.render3d && edit.mode != ObjType::things)
	{
		Render3D_CB_Copy();
		return;
	}

	if (! Clipboard_DoCopy())
	{
		Beep("Nothing to copy");
		return;
	}
}


void Instance::CMD_Clipboard_Paste()
{
	if (main_win->ClipboardOp(EditCommand::paste))
		return;

	if (edit.render3d && edit.mode != ObjType::things)
	{
		Render3D_CB_Paste();
		return;
	}

	if (! Clipboard_DoPaste())
	{
		Beep("Clipboard is empty");
		return;
	}
}


//------------------------------------------------------------------------

void UnusedVertices(const Document &doc, const selection_c &lines, selection_c &result)
{
	SYS_ASSERT(lines.what_type() == ObjType::linedefs);

	ConvertSelection(doc, lines, result);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		// we are interested in the lines we are NOT deleting
		if (lines.get(n))
			continue;

		const auto L = doc.linedefs[n];

		result.clear(L->start);
		result.clear(L->end);
	}
}


void UnusedSideDefs(const Document &doc, const selection_c &lines, const selection_c *secs, selection_c &result)
{
	SYS_ASSERT(lines.what_type() == ObjType::linedefs);

	ConvertSelection(doc, lines, result);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		// we are interested in the lines we are NOT deleting
		if (lines.get(n))
			continue;

		const auto L = doc.linedefs[n];

		if (doc.getRight(*L)) result.clear(L->right);
		if (doc.getLeft(*L))  result.clear(L->left);
	}

	for (int i = 0 ; i < doc.numSidedefs(); i++)
	{
		const auto SD = doc.sidedefs[i];

		if (secs && secs->get(SD->sector))
			result.set(i);
	}
}


static void UnusedLineDefs(const Document &doc, const selection_c &sectors, selection_c &result)
{
	SYS_ASSERT(sectors.what_type() == ObjType::sectors);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		// check if touches a to-be-deleted sector
		//    -1 : no side
		//     0 : deleted side
		//    +1 : kept side
		int right_m = (L->right < 0) ? -1 : sectors.get(doc.getRight(*L)->sector) ? 0 : 1;
		int  left_m = (L->left  < 0) ? -1 : sectors.get(doc.getLeft(*L) ->sector) ? 0 : 1;

		if (std::max(right_m, left_m) == 0)
		{
			result.set(n);
		}
	}
}


static void DuddedSectors(const Document &doc, const selection_c &verts, const selection_c &lines, selection_c &result)
{
	SYS_ASSERT(verts.what_type() == ObjType::vertices);
	SYS_ASSERT(lines.what_type() == ObjType::linedefs);

	// collect all the sectors that touch a linedef being removed.

	bitvec_c del_lines(doc.numLinedefs());

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto linedef = doc.linedefs[n];

		if (lines.get(n) || verts.get(linedef->start) || verts.get(linedef->end))
		{
			del_lines.set(n);

			if (doc.getSectorID(*linedef, Side::left) >= 0)
				result.set(doc.getSectorID(*linedef, Side::left));
			if (doc.getSectorID(*linedef, Side::right) >= 0)
				result.set(doc.getSectorID(*linedef, Side::right));
		}
	}

	// visit all linedefs NOT being removed, and see if the sector(s)
	// on it will actually be OK after the delete.

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		if(result.empty())	// stop looking if there's nothing else to remove
			return;

		const auto linedef = doc.linedefs[n];

		if (lines.get(n) || verts.get(linedef->start) || verts.get(linedef->end))
			continue;

		for (Side what_side : kSides)
		{
			int sec_num = doc.getSectorID(*linedef, what_side);

			if (sec_num < 0)
				continue;

			// skip sectors that are not potentials for removal,
			// and prevent the expensive tests below...
			if (! result.get(sec_num))
				continue;

			// check if the linedef opposite faces this sector (BUT
			// IGNORING any lines being deleted).  when found, we
			// know that this sector should be kept.

			Side opp_side;
			int opp_ld = doc.hover.getOppositeLinedef(n, what_side, &opp_side, &del_lines, nullptr);

			if (opp_ld < 0)
				continue;

			const auto oppositeLinedef = doc.linedefs[opp_ld];

			if (doc.getSectorID(*oppositeLinedef, opp_side) == sec_num)
				result.clear(sec_num);
		}
	}
}


static void FixupLineDefs(EditOperation &op, const Document &doc, selection_c *lines, selection_c *sectors)
{
	for (sel_iter_c it(lines) ; !it.done() ; it.next())
	{
		const auto L = doc.linedefs[*it];

		// the logic is ugly here mainly to handle flipping (in particular,
		// not to flip the line when _both_ sides are unlinked).

		bool do_right = doc.getRight(*L) ? sectors->get(doc.getRight(*L)->sector) : false;
		bool do_left  = doc.getLeft(*L)  ? sectors->get(doc.getLeft(*L) ->sector) : false;

		// line shouldn't be in list unless it touches the sector
		SYS_ASSERT(do_right || do_left);

		if (do_right && do_left)
		{
			doc.linemod.removeSidedef(op, *it, Side::right);
			doc.linemod.removeSidedef(op, *it, Side::left);
		}
		else if (do_right)
		{
			doc.linemod.removeSidedef(op, *it, Side::right);
			doc.linemod.flipLinedef(op, *it);
		}
		else // do_left
		{
			doc.linemod.removeSidedef(op, *it, Side::left);
		}
	}
}


static bool DeleteVertex_MergeLineDefs(Document &doc, int v_num)
{
	// returns true if OK, false if would create an overlapping linedef
	// [ meaning the vertex should be deleted normally ]

	// find the linedefs
	int ld1 = -1;
	int ld2 = -1;

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

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

	const LineDef *L1 = doc.linedefs[ld1].get();
	const LineDef *L2 = doc.linedefs[ld2].get();

	// we merge L2 into L1, unless L1 is significantly shorter
	if (doc.calcLength(*L1) < doc.calcLength(*L2) * 0.7)
	{
		std::swap(ld1, ld2);
		std::swap(L1,  L2);
	}

	// determine the remaining vertices
	int v1 = (L1->start == v_num) ? L1->end : L1->start;
	int v2 = (L2->start == v_num) ? L2->end : L2->start;

	// ensure we don't create two directly overlapping linedefs
	if (doc.linemod.linedefAlreadyExists(v1, v2))
		return false;

	// see what sidedefs would become unused
	selection_c line_sel(ObjType::linedefs);
	selection_c side_sel(ObjType::sidedefs);

	line_sel.set(ld2);

	UnusedSideDefs(doc, line_sel, NULL /* sec_sel */, side_sel);


	EditOperation op(doc.basis);
	op.setMessage("deleted vertex #%d\n", v_num);

	if (L1->start == v_num)
		op.changeLinedef(ld1, LineDef::F_START, v2);
	else
		op.changeLinedef(ld1, LineDef::F_END, v2);

	// NOT-DO: update X offsets on existing linedef

	op.del(ObjType::linedefs, ld2);
	op.del(ObjType::vertices, v_num);

	doc.objects.del(op, side_sel);

	return true;
}


void DeleteObjects_WithUnused(EditOperation &op, const Document &doc, const selection_c &list, bool keep_things,
							  bool keep_verts, bool keep_lines)
{
	selection_c vert_sel(ObjType::vertices);
	selection_c side_sel(ObjType::sidedefs);
	selection_c line_sel(ObjType::linedefs);
	selection_c  sec_sel(ObjType::sectors);

	switch (list.what_type())
	{
		case ObjType::vertices:
			vert_sel.merge(list);
			break;

		case ObjType::linedefs:
			line_sel.merge(list);
			break;

		case ObjType::sectors:
			sec_sel.merge(list);
			break;

		default: /* OBJ_THINGS or OBJ_SIDEDEFS */
			doc.objects.del(op, list);
			return;
	}

	if (list.what_type() == ObjType::vertices)
	{
		for (int n = 0 ; n < doc.numLinedefs(); n++)
		{
			const auto L = doc.linedefs[n];

			if (list.get(L->start) || list.get(L->end))
				line_sel.set(n);
		}
	}

	if (!keep_lines && list.what_type() == ObjType::sectors)
	{
		UnusedLineDefs(doc, sec_sel, line_sel);

		if (line_sel.notempty())
		{
			UnusedSideDefs(doc, line_sel, &sec_sel, side_sel);

			if (!keep_verts)
				UnusedVertices(doc, line_sel, vert_sel);
		}
	}

	if (!keep_verts && list.what_type() == ObjType::linedefs)
	{
		UnusedVertices(doc, line_sel, vert_sel);
	}

	// try to detect sectors that become "dudded", where all the
	// remaining linedefs of the sector face into the void.
	if (list.what_type() == ObjType::vertices || list.what_type() == ObjType::linedefs)
	{
		DuddedSectors(doc, vert_sel, line_sel, sec_sel);
		UnusedSideDefs(doc, line_sel, &sec_sel, side_sel);
	}

	// delete things from each deleted sector
	if (!keep_things && sec_sel.notempty())
	{
		selection_c thing_sel(ObjType::things);

		ConvertSelection(doc, sec_sel, thing_sel);
		doc.objects.del(op, thing_sel);
	}

	// perform linedef fixups here (when sectors get removed)
	if (sec_sel.notempty())
	{
		selection_c fixups(ObjType::linedefs);

		ConvertSelection(doc, sec_sel, fixups);

		// skip lines which will get deleted
		fixups.unmerge(line_sel);

		FixupLineDefs(op, doc, &fixups, &sec_sel);
	}

	// actually delete stuff, in the correct order
	doc.objects.del(op, line_sel);
	doc.objects.del(op, side_sel);
	doc.objects.del(op, vert_sel);
	doc.objects.del(op,  sec_sel);
}


void Instance::CMD_Delete()
{
	if (main_win->ClipboardOp(EditCommand::del))
		return;

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("Nothing to delete");
		return;
	}

	bool keep = Exec_HasFlag("/keep");

	// special case for a single vertex connected to two linedefs,
	// we delete the vertex but merge the two linedefs.
	if (edit.mode == ObjType::vertices && edit.Selected->count_obj() == 1)
	{
		int v_num = edit.Selected->find_first();
		SYS_ASSERT(v_num >= 0);

		if (level.vertmod.howManyLinedefs(v_num) == 2)
		{
			if (DeleteVertex_MergeLineDefs(level, v_num))
				goto success;
		}

		// delete vertex normally
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("deleted", *edit.Selected);

		DeleteObjects_WithUnused(op, level, *edit.Selected, keep, false /* keep_verts */, keep);
	}

success:
	Editor_ClearAction();

	// always clear the selection (deleting objects invalidates it)
	Selection_Clear();

	edit.highlight.clear();
	edit.split_line.clear();

	RedrawMap();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
