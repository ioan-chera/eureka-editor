//------------------------------------------------------------------------
//  BASIC OBJECT HANDLING
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

#include "e_basis.h"

#include "Errors.h"
#include "Instance.h"
#include "main.h"
#include "Thing.h"

// need these for the XXX_Notify() prototypes
#include "r_render.h"

int global::default_floor_h		=   0;
int global::default_ceil_h		= 128;
int global::default_light_level	= 176;

namespace global
{
	static StringTable basis_strtab;
}

const char *NameForObjectType(ObjType type, bool plural)
{
	switch (type)
	{
	case ObjType::things:   return plural ? "things" : "thing";
	case ObjType::linedefs: return plural ? "linedefs" : "linedef";
	case ObjType::sidedefs: return plural ? "sidedefs" : "sidedef";
	case ObjType::vertices: return plural ? "vertices" : "vertex";
	case ObjType::sectors:  return plural ? "sectors" : "sector";

	default:
		BugError("NameForObjectType: bad type: %d\n", (int)type);
		return "XXX"; /* NOT REACHED */
	}
}

int BA_InternaliseString(const SString &str)
{
	return global::basis_strtab.add(str);
}

SString BA_GetString(int offset)
{
	return global::basis_strtab.get(offset);
}


fixcoord_t MakeValidCoord(MapFormat format, double x)
{
	if (format == MapFormat::udmf)
		return toCoord(x);

	// in standard format, coordinates must be integral
	return toCoord(round(x));
}


SString Sector::FloorTex() const
{
	return global::basis_strtab.get(floor_tex);
}

SString Sector::CeilTex() const
{
	return global::basis_strtab.get(ceil_tex);
}

void Sector::SetDefaults(const ConfigData &config)
{
	floorh = global::default_floor_h;
	 ceilh = global::default_ceil_h;

	floor_tex = BA_InternaliseString(config.default_floor_tex);
	 ceil_tex = BA_InternaliseString(config.default_ceil_tex);

	light = global::default_light_level;
}


SString SideDef::UpperTex() const
{
	return global::basis_strtab.get(upper_tex);
}

SString SideDef::MidTex() const
{
	return global::basis_strtab.get(mid_tex);
}

SString SideDef::LowerTex() const
{
	return global::basis_strtab.get(lower_tex);
}

void SideDef::SetDefaults(const Instance &inst, bool two_sided, int new_tex)
{
	if (new_tex < 0)
		new_tex = BA_InternaliseString(inst.conf.default_wall_tex);

	lower_tex = new_tex;
	upper_tex = new_tex;

	if (two_sided)
		mid_tex = BA_InternaliseString("-");
	else
		mid_tex = new_tex;
}


Sector * SideDef::SecRef(const Document &doc) const
{
	return doc.sectors[sector];
}

Vertex * LineDef::Start(const Document &doc) const
{
	return doc.vertices[start];
}

Vertex * LineDef::End(const Document &doc) const
{
	return doc.vertices[end];
}

SideDef * LineDef::Right(const Document &doc) const
{
	return (right >= 0) ? doc.sidedefs[right] : nullptr;
}

SideDef * LineDef::Left(const Document &doc) const
{
	return (left >= 0) ? doc.sidedefs[left] : nullptr;
}


bool LineDef::TouchesSector(int sec_num, const Document &doc) const
{
	if (right >= 0 && doc.sidedefs[right]->sector == sec_num)
		return true;

	if (left >= 0 && doc.sidedefs[left]->sector == sec_num)
		return true;

	return false;
}


int LineDef::WhatSector(Side side, const Document &doc) const
{
	switch (side)
	{
		case Side::left:
			return Left(doc) ? Left(doc)->sector : -1;

		case Side::right:
			return Right(doc) ? Right(doc)->sector : -1;

		default:
			BugError("bad side : %d\n", (int)side);
			return -1;
	}
}


int LineDef::WhatSideDef(Side side) const
{
	switch (side)
	{
		case Side::left:
			return left;
		case Side::right:
			return right;

		default:
			BugError("bad side : %d\n", (int)side);
			return -1;
	}
}

bool LineDef::IsSelfRef(const Document &doc) const
{
	return (left >= 0) && (right >= 0) &&
		doc.sidedefs[left]->sector == doc.sidedefs[right]->sector;
}

double LineDef::CalcLength(const Document &doc) const
{
	double dx = Start(doc)->x() - End(doc)->x();
	double dy = Start(doc)->y() - End(doc)->y();

	return hypot(dx, dy);
}


//------------------------------------------------------------------------

//------------------------------------------------------------------------
//  BASIS API IMPLEMENTATION
//------------------------------------------------------------------------

//
// Begin a group of operations that will become a single undo/redo
// step.  Any stored _redo_ steps will be forgotten.  The BA_New,
// BA_Delete, BA_Change and BA_Message functions must only be called
// between BA_Begin() and BA_End() pairs.
//
void Basis::begin()
{
	if(mCurrentGroup.isActive())
		BugError("Basis::begin called twice without Basis::end\n");
	while(!mRedoFuture.empty())
		mRedoFuture.pop();
	mCurrentGroup.activate();
	doClearChangeStatus();
}

//
// finish a group of operations.
//
void Basis::end()
{
	if(!mCurrentGroup.isActive())
		BugError("Basis::end called without a previous Basis::begin\n");
	mCurrentGroup.end();

	if(mCurrentGroup.isEmpty())
		mCurrentGroup.reset();
	else
	{
		SString message = mCurrentGroup.getMessage();
		mUndoHistory.push(std::move(mCurrentGroup));
		inst.Status_Set("%s", message.c_str());
	}
	doProcessChangeStatus();
}

//
// abort the group of operations -- the undo/redo history is not
// modified and any changes since BA_Begin() are undone except
// when 'keep_changes' is true.
//
void Basis::abort(bool keepChanges)
{
	if(!mCurrentGroup.isActive())
		BugError("Basis::abort called without a previous Basis::begin\n");

	mCurrentGroup.end();

	if(!keepChanges && !mCurrentGroup.isEmpty())
		mCurrentGroup.reapply(*this);

	mCurrentGroup.reset();
	mDidMakeChanges = false;
	doProcessChangeStatus();
}

//
// assign a message to the current operation.
// this can be called multiple times.
//
void Basis::setMessage(EUR_FORMAT_STRING(const char *format), ...)
{
	SYS_ASSERT(format);
	SYS_ASSERT(mCurrentGroup.isActive());

	va_list arg_ptr;
	va_start(arg_ptr, format);
	mCurrentGroup.setMessage(SString::vprintf(format, arg_ptr));
	va_end(arg_ptr);
}

//
// Set a message for the selection
//
void Basis::setMessageForSelection(const char *verb, const selection_c &list, const char *suffix)
{
	// utility for creating messages like "moved 3 things"

	int total = list.count_obj();

	if(total < 1)  // oops
		return;

	if(total == 1)
	{
		setMessage("%s %s #%d%s", verb, NameForObjectType(list.what_type()), list.find_first(), suffix);
	}
	else
	{
		setMessage("%s %d %s%s", verb, total, NameForObjectType(list.what_type(), true /* plural */), suffix);
	}
}

//
// create a new object, returning its objnum.  It is safe to
// directly set the new object's fields after calling BA_New().
//
int Basis::addNew(ObjType type)
{
	SYS_ASSERT(mCurrentGroup.isActive());

	EditUnit op;

	op.action = EditType::insert;
	op.objtype = type;

	switch(type)
	{
	case ObjType::things:
		op.objnum = doc.numThings();
		op.thing = new Thing;
		break;

	case ObjType::vertices:
		op.objnum = doc.numVertices();
		op.vertex = new Vertex;
		break;

	case ObjType::sidedefs:
		op.objnum = doc.numSidedefs();
		op.sidedef = new SideDef;
		break;

	case ObjType::linedefs:
		op.objnum = doc.numLinedefs();
		op.linedef = new LineDef;
		break;

	case ObjType::sectors:
		op.objnum = doc.numSectors();
		op.sector = new Sector;
		break;

	default:
		BugError("Basis::addNew: unknown type\n");
	}

	mCurrentGroup.addApply(op, *this);

	return op.objnum;
}

//
// deletes the given object, and in certain cases other types of
// objects bound to it (e.g. deleting a vertex will cause all
// bound linedefs to also be deleted).
//
void Basis::del(ObjType type, int objnum)
{
	SYS_ASSERT(mCurrentGroup.isActive());

	EditUnit op;

	op.action = EditType::del;
	op.objtype = type;
	op.objnum = objnum;

	// this must happen _before_ doing the deletion (otherwise
	// when we undo, the insertion will mess up the references).
	if(type == ObjType::sidedefs)
	{
		// unbind sidedef from any linedefs using it
		for(int n = doc.numLinedefs() - 1; n >= 0; n--)
		{
			LineDef *L = doc.linedefs[n];

			if(L->right == objnum)
				changeLinedef(n, LineDef::F_RIGHT, -1);

			if(L->left == objnum)
				changeLinedef(n, LineDef::F_LEFT, -1);
		}
	}
	else if(type == ObjType::vertices)
	{
		// delete any linedefs bound to this vertex
		for(int n = doc.numLinedefs() - 1; n >= 0; n--)
		{
			LineDef *L = doc.linedefs[n];

			if(L->start == objnum || L->end == objnum)
				del(ObjType::linedefs, n);
		}
	}
	else if(type == ObjType::sectors)
	{
		// delete the sidedefs bound to this sector
		for(int n = doc.numSidedefs() - 1; n >= 0; n--)
			if(doc.sidedefs[n]->sector == objnum)
				del(ObjType::sidedefs, n);
	}

	SYS_ASSERT(mCurrentGroup.isActive());

	mCurrentGroup.addApply(op, *this);
}

//
// change a field of an existing object.  If the value was the
// same as before, nothing happens and false is returned.
// Otherwise returns true.
//
bool Basis::change(ObjType type, int objnum, byte field, int value)
{
	// TODO: optimise, check whether value actually changes

	EditUnit op;

	op.action = EditType::change;
	op.objtype = type;
	op.field = field;
	op.objnum = objnum;
	op.value = value;

	SYS_ASSERT(mCurrentGroup.isActive());

	mCurrentGroup.addApply(op, *this);
	return true;
}

//
// Change thing
//
bool Basis::changeThing(int thing, byte field, int value)
{
	SYS_ASSERT(thing >= 0 && thing < doc.numThings());
	SYS_ASSERT(field <= Thing::F_ARG5);

	if(field == Thing::F_TYPE)
		inst.recent_things.insert_number(value);

	return change(ObjType::things, thing, field, value);
}

//
// Change vertex
//
bool Basis::changeVertex(int vert, byte field, int value)
{
	SYS_ASSERT(vert >= 0 && vert < doc.numVertices());
	SYS_ASSERT(field <= Vertex::F_Y);

	return change(ObjType::vertices, vert, field, value);
}

//
// Change sector
//
bool Basis::changeSector(int sec, byte field, int value)
{
	SYS_ASSERT(sec >= 0 && sec < doc.numSectors());
	SYS_ASSERT(field <= Sector::F_TAG);

	if(field == Sector::F_FLOOR_TEX ||
		field == Sector::F_CEIL_TEX)
	{
		inst.recent_flats.insert(BA_GetString(value));
	}

	return change(ObjType::sectors, sec, field, value);
}

//
// Change sidedef
//
bool Basis::changeSidedef(int side, byte field, int value)
{
	SYS_ASSERT(side >= 0 && side < doc.numSidedefs());
	SYS_ASSERT(field <= SideDef::F_SECTOR);

	if(field == SideDef::F_LOWER_TEX ||
		field == SideDef::F_UPPER_TEX ||
		field == SideDef::F_MID_TEX)
	{
		inst.recent_textures.insert(BA_GetString(value));
	}

	return change(ObjType::sidedefs, side, field, value);
}

//
// Change linedef
//
bool Basis::changeLinedef(int line, byte field, int value)
{
	SYS_ASSERT(line >= 0 && line < doc.numLinedefs());
	SYS_ASSERT(field <= LineDef::F_ARG5);

	return change(ObjType::linedefs, line, field, value);
}

//
// attempt to undo the last normal or redo operation.  Returns
// false if the undo history is empty.
//
bool Basis::undo()
{
	if(mUndoHistory.empty())
		return false;

	doClearChangeStatus();

	UndoGroup grp = std::move(mUndoHistory.top());
	mUndoHistory.pop();

	inst.Status_Set("UNDO: %s", grp.getMessage().c_str());

	grp.reapply(*this);

	mRedoFuture.push(std::move(grp));

	doProcessChangeStatus();
	return true;
}

//
// attempt to re-do the last undo operation.  Returns false if
// there is no stored redo steps.
//
bool Basis::redo()
{
	if(mRedoFuture.empty())
		return false;

	doClearChangeStatus();

	UndoGroup grp = std::move(mRedoFuture.top());
	mRedoFuture.pop();

	inst.Status_Set("Redo: %s", grp.getMessage().c_str());

	grp.reapply(*this);

	mUndoHistory.push(std::move(grp));

	doProcessChangeStatus();
	return true;
}

//
// clear everything (before loading a new level).
//
void Basis::clearAll()
{
	for(Thing *thing : doc.things)
		delete thing;
	for(Vertex *vertex : doc.vertices)
		delete vertex;
	for(Sector *sector : doc.sectors)
		delete sector;
	for(SideDef *sidedef : doc.sidedefs)
		delete sidedef;
	for(LineDef *linedef : doc.linedefs)
		delete linedef;

	doc.things.clear();
	doc.vertices.clear();
	doc.sectors.clear();
	doc.sidedefs.clear();
	doc.linedefs.clear();

	doc.headerData.clear();
	doc.behaviorData.clear();
	doc.scriptsData.clear();

	while(!mUndoHistory.empty())
		mUndoHistory.pop();
	while(!mRedoFuture.empty())
		mRedoFuture.pop();

	// Note: we don't clear the string table, since there can be
	//       string references in the clipboard.

	// TODO: other modules
	Clipboard_ClearLocals();
}

//
// Execute the operation
//
void Basis::EditUnit::apply(Basis &basis)
{
	switch(action)
	{
	case EditType::change:
		rawChange(basis);
		return;
	case EditType::del:
		ptr = static_cast<int *>(rawDelete(basis));
		action = EditType::insert;	// reverse the operation
		return;
	case EditType::insert:
		rawInsert(basis);
		ptr = nullptr;
		action = EditType::del;	// reverse the operation
		return;
	default:
		BugError("Basis::EditOperation::apply\n");
	}
}

//
// Destroy an inst.inst.edit operation
//
void Basis::EditUnit::destroy()
{
	switch(action)
	{
	case EditType::insert:
		SYS_ASSERT(ptr);
		deleteFinally();
		break;
	case EditType::del:
		SYS_ASSERT(!ptr);
		break;
	default:
		break;
	}
}

//
// Execute the raw change
//
void Basis::EditUnit::rawChange(Basis &basis)
{
	int *pos = nullptr;
	switch(objtype)
	{
	case ObjType::things:
		SYS_ASSERT(0 <= objnum && objnum < basis.doc.numThings());
		pos = reinterpret_cast<int *>(basis.doc.things[objnum]);
		break;
	case ObjType::vertices:
		SYS_ASSERT(0 <= objnum && objnum < basis.doc.numVertices());
		pos = reinterpret_cast<int *>(basis.doc.vertices[objnum]);
		break;
	case ObjType::sectors:
		SYS_ASSERT(0 <= objnum && objnum < basis.doc.numSectors());
		pos = reinterpret_cast<int *>(basis.doc.sectors[objnum]);
		break;
	case ObjType::sidedefs:
		SYS_ASSERT(0 <= objnum && objnum < basis.doc.numSidedefs());
		pos = reinterpret_cast<int *>(basis.doc.sidedefs[objnum]);
		break;
	case ObjType::linedefs:
		SYS_ASSERT(0 <= objnum && objnum < basis.doc.numLinedefs());
		pos = reinterpret_cast<int *>(basis.doc.linedefs[objnum]);
		break;
	default:
		BugError("Basis::EditOperation::rawChange: bad objtype %u\n", (unsigned)objtype);
		return; /* NOT REACHED */
	}
	// TODO: CHANGE THIS TO A SAFER WAY!
	std::swap(pos[field], value);
	basis.mDidMakeChanges = true;

	// TODO: their modules
	Clipboard_NotifyChange(objtype, objnum, field);
	Selection_NotifyChange(objtype, objnum, field);
	basis.inst.MapStuff_NotifyChange(objtype, objnum, field);
	Render3D_NotifyChange(objtype, objnum, field);
	basis.inst.ObjectBox_NotifyChange(objtype, objnum, field);
}

//
// Deletion operation
//
void *Basis::EditUnit::rawDelete(Basis &basis) const
{
	basis.mDidMakeChanges = true;

	// TODO: their own modules
	Clipboard_NotifyDelete(objtype, objnum);
	basis.inst.Selection_NotifyDelete(objtype, objnum);
	basis.inst.MapStuff_NotifyDelete(objtype, objnum);
	Render3D_NotifyDelete(basis.doc, objtype, objnum);
	basis.inst.ObjectBox_NotifyDelete(objtype, objnum);

	switch(objtype)
	{
	case ObjType::things:
		return rawDeleteThing(basis.doc);

	case ObjType::vertices:
		return rawDeleteVertex(basis.doc);

	case ObjType::sectors:
		return rawDeleteSector(basis.doc);

	case ObjType::sidedefs:
		return rawDeleteSidedef(basis.doc);

	case ObjType::linedefs:
		return rawDeleteLinedef(basis.doc);

	default:
		BugError("Basis::EditOperation::rawDelete: bad objtype %u\n", (unsigned)objtype);
		return NULL; /* NOT REACHED */
	}
}

//
// Thing deletion
//
Thing *Basis::EditUnit::rawDeleteThing(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum < doc.numThings());

	Thing *result = doc.things[objnum];
	doc.things.erase(doc.things.begin() + objnum);

	return result;
}

//
// Vertex deletion (and update linedef refs)
//
Vertex *Basis::EditUnit::rawDeleteVertex(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum < doc.numVertices());

	Vertex *result = doc.vertices[objnum];
	doc.vertices.erase(doc.vertices.begin() + objnum);

	// fix the linedef references

	if(objnum < doc.numVertices())
	{
		for(int n = doc.numLinedefs() - 1; n >= 0; n--)
		{
			LineDef *L = doc.linedefs[n];

			if(L->start > objnum)
				L->start--;

			if(L->end > objnum)
				L->end--;
		}
	}

	return result;
}

//
// Raw delete sector (and update sidedef refs)
//
Sector *Basis::EditUnit::rawDeleteSector(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum < doc.numSectors());

	Sector *result = doc.sectors[objnum];
	doc.sectors.erase(doc.sectors.begin() + objnum);

	// fix sidedef references

	if(objnum < doc.numSectors())
	{
		for(int n = doc.numSidedefs() - 1; n >= 0; n--)
		{
			SideDef *S = doc.sidedefs[n];

			if(S->sector > objnum)
				S->sector--;
		}
	}

	return result;
}

//
// Delete sidedef (and update linedef references)
//
SideDef *Basis::EditUnit::rawDeleteSidedef(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum < doc.numSidedefs());

	SideDef *result = doc.sidedefs[objnum];
	doc.sidedefs.erase(doc.sidedefs.begin() + objnum);

	// fix the linedefs references

	if(objnum < doc.numSidedefs())
	{
		for(int n = doc.numLinedefs() - 1; n >= 0; n--)
		{
			LineDef *L = doc.linedefs[n];

			if(L->right > objnum)
				L->right--;

			if(L->left > objnum)
				L->left--;
		}
	}

	return result;
}

//
// Raw delete linedef
//
LineDef *Basis::EditUnit::rawDeleteLinedef(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum < doc.numLinedefs());

	LineDef *result = doc.linedefs[objnum];
	doc.linedefs.erase(doc.linedefs.begin() + objnum);

	return result;
}

//
// Insert operation
//
void Basis::EditUnit::rawInsert(Basis &basis) const
{
	basis.mDidMakeChanges = true;

	// TODO: their module
	Clipboard_NotifyInsert(basis.doc, objtype, objnum);
	basis.inst.Selection_NotifyInsert(objtype, objnum);
	basis.inst.MapStuff_NotifyInsert(objtype, objnum);
	Render3D_NotifyInsert(objtype, objnum);
	basis.inst.ObjectBox_NotifyInsert(objtype, objnum);

	switch(objtype)
	{
	case ObjType::things:
		rawInsertThing(basis.doc);
		break;

	case ObjType::vertices:
		rawInsertVertex(basis.doc);
		break;

	case ObjType::sidedefs:
		rawInsertSidedef(basis.doc);
		break;

	case ObjType::sectors:
		rawInsertSector(basis.doc);
		break;

	case ObjType::linedefs:
		rawInsertLinedef(basis.doc);
		break;

	default:
		BugError("Basis::EditOperation::rawInsert: bad objtype %u\n", (unsigned)objtype);
	}
}

//
// Thing insertion
//
void Basis::EditUnit::rawInsertThing(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum <= doc.numThings());
	doc.things.insert(doc.things.begin() + objnum, thing);
}

//
// Vertex insertion
//
void Basis::EditUnit::rawInsertVertex(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum <= doc.numVertices());
	doc.vertices.insert(doc.vertices.begin() + objnum, vertex);

	// fix references in linedefs

	if(objnum + 1 < doc.numVertices())
	{
		for(int n = doc.numLinedefs() - 1; n >= 0; n--)
		{
			LineDef *L = doc.linedefs[n];

			if(L->start >= objnum)
				L->start++;

			if(L->end >= objnum)
				L->end++;
		}
	}
}

//
// Sector insertion
//
void Basis::EditUnit::rawInsertSector(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum <= doc.numSectors());
	doc.sectors.insert(doc.sectors.begin() + objnum, sector);

	// fix all sidedef references

	if(objnum + 1 < doc.numSectors())
	{
		for(int n = doc.numSidedefs() - 1; n >= 0; n--)
		{
			SideDef *S = doc.sidedefs[n];

			if(S->sector >= objnum)
				S->sector++;
		}
	}
}

//
// Sidedef insertion
//
void Basis::EditUnit::rawInsertSidedef(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum <= doc.numSidedefs());
	doc.sidedefs.insert(doc.sidedefs.begin() + objnum, sidedef);

	// fix the linedefs references

	if(objnum + 1 < doc.numSidedefs())
	{
		for(int n = doc.numLinedefs() - 1; n >= 0; n--)
		{
			LineDef *L = doc.linedefs[n];

			if(L->right >= objnum)
				L->right++;

			if(L->left >= objnum)
				L->left++;
		}
	}
}

//
// Linedef insertion
//
void Basis::EditUnit::rawInsertLinedef(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum <= doc.numLinedefs());
	doc.linedefs.insert(doc.linedefs.begin() + objnum, linedef);
}

//
// Action to do on destruction of insert operation
//
void Basis::EditUnit::deleteFinally()
{
	switch(objtype)
	{
	case ObjType::things:   delete thing; break;
	case ObjType::vertices: delete vertex; break;
	case ObjType::sectors:  delete sector; break;
	case ObjType::sidedefs: delete sidedef; break;
	case ObjType::linedefs: delete linedef; break;

	default:
		BugError("DeleteFinally: bad objtype %d\n", (int)objtype);
	}
}

//
// Move operator
//
Basis::UndoGroup &Basis::UndoGroup::operator = (UndoGroup &&other) noexcept
{
	mOps = std::move(other.mOps);
	mDir = other.mDir;
	mMessage = std::move(other.mMessage);

	other.reset();	// ensure the other goes into the default state
	return *this;
}

//
// Reset to initial, inactive state
//
void Basis::UndoGroup::reset()
{
	mOps.clear();
	mDir = 0;
	mMessage = DEFAULT_UNDO_GROUP_MESSAGE;
}

//
// Add and apply
//
void Basis::UndoGroup::addApply(const EditUnit &op, Basis &basis)
{
	mOps.push_back(op);
	mOps.back().apply(basis);
}

//
// Reapply
//
void Basis::UndoGroup::reapply(Basis &basis)
{
	if(mDir > 0)
		for(auto it = mOps.begin(); it != mOps.end(); ++it)
			it->apply(basis);
	else if(mDir < 0)
		for(auto it = mOps.rbegin(); it != mOps.rend(); ++it)
			it->apply(basis);

	// reverse the order for next time
	mDir = -mDir;
}

//
// Clear change status
//
void Basis::doClearChangeStatus()
{
	mDidMakeChanges = false;

	// TODO: these shall go to other modules
	Clipboard_NotifyBegin();
	inst.Selection_NotifyBegin();
	inst.MapStuff_NotifyBegin();
	Render3D_NotifyBegin();
	inst.ObjectBox_NotifyBegin();
}

//
// If we made changes, notify the others
//
void Basis::doProcessChangeStatus() const
{
	if(mDidMakeChanges)
	{
		// TODO: the other modules
		inst.MadeChanges = true;
		inst.RedrawMap();
	}

	Clipboard_NotifyEnd();
	inst.Selection_NotifyEnd();
	inst.MapStuff_NotifyEnd();
	Render3D_NotifyEnd(inst);
	inst.ObjectBox_NotifyEnd();
}

//
// Set operation message
//
void EditOperation::setMessage(EUR_FORMAT_STRING(const char *format), ...)
{
	va_list ap;
	va_start(ap, format);
	basis.setMessage("%s", SString::vprintf(format, ap).c_str());
	va_end(ap);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
