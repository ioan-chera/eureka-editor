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
#include "LineDef.h"
#include "main.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"

// need these for the XXX_Notify() prototypes
#include "r_render.h"

int global::default_floor_h		=   0;
int global::default_ceil_h		= 128;
int global::default_light_level	= 176;

static StringTable basis_strtab;

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

StringID BA_InternaliseString(const SString &str)
{
	return basis_strtab.add(str);
}

SString BA_GetString(StringID offset)
{
	return basis_strtab.get(offset);
}


FFixedPoint MakeValidCoord(MapFormat format, double x)
{
	if (format == MapFormat::udmf)
		return FFixedPoint(x);

	// in standard format, coordinates must be integral
	return FFixedPoint(round(x));
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
		op.thing = Thing();
		break;

	case ObjType::vertices:
		op.objnum = doc.numVertices();
		op.vertex = Vertex();
		break;

	case ObjType::sidedefs:
		op.objnum = doc.numSidedefs();
		op.sidedef = SideDef();
		break;

	case ObjType::linedefs:
		op.objnum = doc.numLinedefs();
		op.linedef = LineDef();
		break;

	case ObjType::sectors:
		op.objnum = doc.numSectors();
		op.sector = Sector();
		break;

	default:
		BugError("Basis::addNew: unknown type\n");
	}

	int objnum = op.objnum;
	mCurrentGroup.addApply(std::move(op), *this);

	return objnum;
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
			const auto &L = doc.getLinedef(n);

			if(L.right == objnum)
				changeLinedef(n, LineDef::F_RIGHT, -1);

			if(L.left == objnum)
				changeLinedef(n, LineDef::F_LEFT, -1);
		}
	}
	else if(type == ObjType::vertices)
	{
		// delete any linedefs bound to this vertex
		for(int n = doc.numLinedefs() - 1; n >= 0; n--)
		{
			const auto &L = doc.getLinedef(n);

			if(L.start == objnum || L.end == objnum)
				del(ObjType::linedefs, n);
		}
	}
	else if(type == ObjType::sectors)
	{
		// delete the sidedefs bound to this sector
		for(int n = doc.numSidedefs() - 1; n >= 0; n--)
			if(doc.sidedefs[n].sector == objnum)
				del(ObjType::sidedefs, n);
	}

	SYS_ASSERT(mCurrentGroup.isActive());

	mCurrentGroup.addApply(std::move(op), *this);
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

	mCurrentGroup.addApply(std::move(op), *this);
	return true;
}

//
// Change thing
//
bool Basis::changeThing(int thing, Thing::IntAddress field, int value)
{
	SYS_ASSERT(thing >= 0 && thing < doc.numThings());

	if(field == Thing::F_TYPE)
		inst.recent_things.insert_number(value);

	return change(ObjType::things, thing, field, value);
}
bool Basis::changeThing(int thing, Thing::FixedPointAddress field, FFixedPoint value)
{
	SYS_ASSERT(thing >= 0 && thing < doc.numThings());

	return change(ObjType::things, thing, field, value.raw());
}

//
// Change vertex
//
bool Basis::changeVertex(int vert, byte field, FFixedPoint value)
{
	SYS_ASSERT(vert >= 0 && vert < doc.numVertices());
	SYS_ASSERT(field <= Vertex::F_Y);

	return change(ObjType::vertices, vert, field, value.raw());
}

//
// Change sector
//
bool Basis::changeSector(int sec, Sector::IntAddress field, int value)
{
	SYS_ASSERT(sec >= 0 && sec < doc.numSectors());

	return change(ObjType::sectors, sec, field, value);
}
bool Basis::changeSector(int sec, Sector::StringIDAddress field, StringID value)
{
	SYS_ASSERT(sec >= 0 && sec < doc.numSectors());

	inst.recent_flats.insert(BA_GetString(value));

	return change(ObjType::sectors, sec, field, value.get());
}

//
// Change sidedef
//
bool Basis::changeSidedef(int side, SideDef::IntAddress field, int value)
{
	SYS_ASSERT(side >= 0 && side < doc.numSidedefs());

	return change(ObjType::sidedefs, side, field, value);
}
bool Basis::changeSidedef(int side, SideDef::StringIDAddress field, StringID value)
{
	SYS_ASSERT(side >= 0 && side < doc.numSidedefs());

	inst.recent_textures.insert(BA_GetString(value));

	return change(ObjType::sidedefs, side, field, value.get());
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
	doc.things.clear();
	doc.deleteAllVertices();
	doc.sectors.clear();
	doc.sidedefs.clear();
	doc.deleteAllLinedefs();

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
		rawDelete(basis);
		action = EditType::insert;	// reverse the operation
		return;
	case EditType::insert:
		rawInsert(basis);
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
		deleteFinally();
		break;
	case EditType::del:
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
		pos = reinterpret_cast<int *>(&basis.doc.things[objnum]);
		break;
	case ObjType::vertices:
		SYS_ASSERT(0 <= objnum && objnum < basis.doc.numVertices());
		pos = reinterpret_cast<int *>(&basis.doc.getMutableVertex(objnum));
		break;
	case ObjType::sectors:
		SYS_ASSERT(0 <= objnum && objnum < basis.doc.numSectors());
		pos = reinterpret_cast<int *>(&basis.doc.sectors[objnum]);
		break;
	case ObjType::sidedefs:
		SYS_ASSERT(0 <= objnum && objnum < basis.doc.numSidedefs());
		pos = reinterpret_cast<int *>(&basis.doc.sidedefs[objnum]);
		break;
	case ObjType::linedefs:
		SYS_ASSERT(0 <= objnum && objnum < basis.doc.numLinedefs());
		pos = reinterpret_cast<int *>(&basis.doc.getMutableLinedef(objnum));
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
void Basis::EditUnit::rawDelete(Basis &basis)
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
		thing = rawDeleteThing(basis.doc);
		return;

	case ObjType::vertices:
		vertex = basis.doc.removeVertex(objnum);
		return;

	case ObjType::sectors:
		sector = rawDeleteSector(basis.doc);
		return;

	case ObjType::sidedefs:
		sidedef = rawDeleteSidedef(basis.doc);
		return;

	case ObjType::linedefs:
		linedef = rawDeleteLinedef(basis.doc);
		return;

	default:
		BugError("Basis::EditOperation::rawDelete: bad objtype %u\n", (unsigned)objtype);
		return; /* NOT REACHED */
	}
}

//
// Thing deletion
//
Thing Basis::EditUnit::rawDeleteThing(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum < doc.numThings());

	auto result = doc.things[objnum];
	doc.things.erase(doc.things.begin() + objnum);

	return result;
}

//
// Raw delete sector (and update sidedef refs)
//
Sector Basis::EditUnit::rawDeleteSector(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum < doc.numSectors());

	auto result = doc.sectors[objnum];
	doc.sectors.erase(doc.sectors.begin() + objnum);

	// fix sidedef references

	if(objnum < doc.numSectors())
	{
		for(int n = doc.numSidedefs() - 1; n >= 0; n--)
		{
			auto &S = doc.sidedefs[n];

			if(S.sector > objnum)
				S.sector--;
		}
	}

	return result;
}

//
// Delete sidedef (and update linedef references)
//
SideDef Basis::EditUnit::rawDeleteSidedef(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum < doc.numSidedefs());

	auto result = doc.sidedefs[objnum];
	doc.sidedefs.erase(doc.sidedefs.begin() + objnum);

	// fix the linedefs references

	if(objnum < doc.numSidedefs())
	{
		for(int n = doc.numLinedefs() - 1; n >= 0; n--)
		{
			auto &L = doc.getMutableLinedef(n);

			if(L.right > objnum)
				L.right--;

			if(L.left > objnum)
				L.left--;
		}
	}

	return result;
}

//
// Raw delete linedef
//
LineDef Basis::EditUnit::rawDeleteLinedef(Document &doc) const
{
	SYS_ASSERT(0 <= objnum && objnum < doc.numLinedefs());

	return doc.removeLinedef(objnum);
}

//
// Insert operation
//
void Basis::EditUnit::rawInsert(Basis &basis)
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
		thing = {};	// normally already reset
		break;

	case ObjType::vertices:
		basis.doc.insertVertex(vertex, objnum);
		vertex = {};
		break;

	case ObjType::sidedefs:
		rawInsertSidedef(basis.doc);
		sidedef = {};
		break;

	case ObjType::sectors:
		rawInsertSector(basis.doc);
		sector = {};
		break;

	case ObjType::linedefs:
		rawInsertLinedef(basis.doc);
		linedef = {};
		break;

	default:
		BugError("Basis::EditOperation::rawInsert: bad objtype %u\n", (unsigned)objtype);
	}
}

//
// Thing insertion
//
void Basis::EditUnit::rawInsertThing(Document &doc)
{
	SYS_ASSERT(0 <= objnum && objnum <= doc.numThings());
	doc.things.insert(doc.things.begin() + objnum, thing);
}

//
// Sector insertion
//
void Basis::EditUnit::rawInsertSector(Document &doc)
{
	SYS_ASSERT(0 <= objnum && objnum <= doc.numSectors());
	doc.sectors.insert(doc.sectors.begin() + objnum, sector);

	// fix all sidedef references

	if(objnum + 1 < doc.numSectors())
	{
		for(int n = doc.numSidedefs() - 1; n >= 0; n--)
		{
			auto &S = doc.sidedefs[n];

			if(S.sector >= objnum)
				S.sector++;
		}
	}
}

//
// Sidedef insertion
//
void Basis::EditUnit::rawInsertSidedef(Document &doc)
{
	SYS_ASSERT(0 <= objnum && objnum <= doc.numSidedefs());
	doc.sidedefs.insert(doc.sidedefs.begin() + objnum, std::move(sidedef));

	// fix the linedefs references

	if(objnum + 1 < doc.numSidedefs())
	{
		for(int n = doc.numLinedefs() - 1; n >= 0; n--)
		{
			auto &L = doc.getMutableLinedef(n);

			if(L.right >= objnum)
				L.right++;

			if(L.left >= objnum)
				L.left++;
		}
	}
}

//
// Linedef insertion
//
void Basis::EditUnit::rawInsertLinedef(Document &doc)
{
	SYS_ASSERT(0 <= objnum && objnum <= doc.numLinedefs());
	doc.insertLinedef(linedef, objnum);
}

//
// Action to do on destruction of insert operation
//
void Basis::EditUnit::deleteFinally()
{
	switch(objtype)
	{
	case ObjType::things:   thing = {}; break;
	case ObjType::vertices: vertex = {}; break;
	case ObjType::sectors:  sector = {}; break;
	case ObjType::sidedefs: sidedef = {}; break;
	case ObjType::linedefs: linedef = {}; break;

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
void Basis::UndoGroup::addApply(EditUnit &&op, Basis &basis)
{
	mOps.push_back(std::move(op));
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
