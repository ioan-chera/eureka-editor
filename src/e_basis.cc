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

#include "Errors.hpp"
#include "main.h"

#include <algorithm>
#include <list>
#include <string>

#include "lib_adler.h"

#include "e_basis.h"
#include "e_main.h"
#include "m_strings.h"

#include "m_game.h"  // g_default_xxx

// need these for the XXX_Notify() prototypes
#include "e_cutpaste.h"
#include "e_objects.h"
#include "r_render.h"

Document gDocument;	// currently a singleton.

std::vector<byte>  HeaderData;
std::vector<byte>  BehaviorData;
std::vector<byte>  ScriptsData;


int default_floor_h		=   0;
int default_ceil_h		= 128;
int default_light_level	= 176;
int default_thing		= 2001;

SString default_wall_tex	= "GRAY1";
SString default_floor_tex	= "FLAT1";
SString default_ceil_tex	= "FLAT1";


static bool did_make_changes;


StringTable basis_strtab;

int NumObjects(ObjType type)
{
	switch (type)
	{
	case ObjType::things:
		return NumThings;
	case ObjType::linedefs:
		return NumLineDefs;
	case ObjType::sidedefs:
		return NumSideDefs;
	case ObjType::vertices:
		return NumVertices;
	case ObjType::sectors:
		return NumSectors;
	default:
		return 0;
	}
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


static void DoClearChangeStatus()
{
	did_make_changes = false;

	Clipboard_NotifyBegin();
	Selection_NotifyBegin();
	 MapStuff_NotifyBegin();
	 Render3D_NotifyBegin();
	ObjectBox_NotifyBegin();
}

static void DoProcessChangeStatus()
{
	if (did_make_changes)
	{
		MadeChanges = 1;
		RedrawMap();
	}

	Clipboard_NotifyEnd();
	Selection_NotifyEnd();
	 MapStuff_NotifyEnd();
	 Render3D_NotifyEnd();
	ObjectBox_NotifyEnd();
}


int BA_InternaliseString(const SString &str)
{
	return basis_strtab.add(str);
}

int BA_InternaliseShortStr(const char *str, int max_len)
{
	SString goodie(str, max_len);

	int result = BA_InternaliseString(goodie);

	return result;
}

SString BA_GetString(int offset)
{
	return basis_strtab.get(offset);
}


fixcoord_t MakeValidCoord(double x)
{
	if (Level_format == MAPF_UDMF)
		return TO_COORD(x);

	// in standard format, coordinates must be integral
	return TO_COORD(I_ROUND(x));
}


SString Sector::FloorTex() const
{
	return basis_strtab.get(floor_tex);
}

SString Sector::CeilTex() const
{
	return basis_strtab.get(ceil_tex);
}

void Sector::SetDefaults()
{
	floorh = default_floor_h;
	 ceilh = default_ceil_h;

	floor_tex = BA_InternaliseString(default_floor_tex);
	 ceil_tex = BA_InternaliseString(default_ceil_tex);

	light = default_light_level;
}


SString SideDef::UpperTex() const
{
	return basis_strtab.get(upper_tex);
}

SString SideDef::MidTex() const
{
	return basis_strtab.get(mid_tex);
}

SString SideDef::LowerTex() const
{
	return basis_strtab.get(lower_tex);
}

void SideDef::SetDefaults(bool two_sided, int new_tex)
{
	if (new_tex < 0)
		new_tex = BA_InternaliseString(default_wall_tex);

	lower_tex = new_tex;
	upper_tex = new_tex;

	if (two_sided)
		mid_tex = BA_InternaliseString("-");
	else
		mid_tex = new_tex;
}


Sector * SideDef::SecRef() const
{
	return gDocument.sectors[sector];
}

Vertex * LineDef::Start() const
{
	return gDocument.vertices[start];
}

Vertex * LineDef::End() const
{
	return gDocument.vertices[end];
}

SideDef * LineDef::Right() const
{
	return (right >= 0) ? gDocument.sidedefs[right] : nullptr;
}

SideDef * LineDef::Left() const
{
	return (left >= 0) ? gDocument.sidedefs[left] : nullptr;
}


bool LineDef::TouchesSector(int sec_num) const
{
	if (right >= 0 && gDocument.sidedefs[right]->sector == sec_num)
		return true;

	if (left >= 0 && gDocument.sidedefs[left]->sector == sec_num)
		return true;

	return false;
}


int LineDef::WhatSector(Side side) const
{
	switch (side)
	{
		case Side::left:
			return Left() ? Left()->sector : -1;

		case Side::right:
			return Right() ? Right()->sector : -1;

		default:
			BugError("bad side : %d\n", side);
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
			BugError("bad side : %d\n", side);
			return -1;
	}
}


bool LineDef::IsSelfRef() const
{
	return (left >= 0) && (right >= 0) &&
		gDocument.sidedefs[left]->sector == gDocument.sidedefs[right]->sector;
}


double LineDef::CalcLength() const
{
	double dx = Start()->x() - End()->x();
	double dy = Start()->y() - End()->y();

	return hypot(dx, dy);
}


//------------------------------------------------------------------------


static void RawInsertThing(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumThings);

	gDocument.things.push_back(NULL);

	for (int n = NumThings-1 ; n > objnum ; n--)
		gDocument.things[n] = gDocument.things[n - 1];

	gDocument.things[objnum] = reinterpret_cast<Thing*>(ptr);
}

static void RawInsertLineDef(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumLineDefs);

	gDocument.linedefs.push_back(NULL);

	for (int n = NumLineDefs-1 ; n > objnum ; n--)
		gDocument.linedefs[n] = gDocument.linedefs[n - 1];

	gDocument.linedefs[objnum] = reinterpret_cast<LineDef*>(ptr);
}

static void RawInsertVertex(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumVertices);

	gDocument.vertices.push_back(NULL);

	for (int n = NumVertices-1 ; n > objnum ; n--)
		gDocument.vertices[n] = gDocument.vertices[n - 1];

	gDocument.vertices[objnum] = reinterpret_cast<Vertex*>(ptr);

	// fix references in linedefs

	if (objnum+1 < NumVertices)
	{
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = gDocument.linedefs[n];

			if (L->start >= objnum)
				L->start++;

			if (L->end >= objnum)
				L->end++;
		}
	}
}

static void RawInsertSideDef(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumSideDefs);

	gDocument.sidedefs.push_back(NULL);

	for (int n = NumSideDefs-1 ; n > objnum ; n--)
		gDocument.sidedefs[n] = gDocument.sidedefs[n - 1];

	gDocument.sidedefs[objnum] = reinterpret_cast<SideDef*>(ptr);

	// fix the linedefs references

	if (objnum+1 < NumSideDefs)
	{
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = gDocument.linedefs[n];

			if (L->right >= objnum)
				L->right++;

			if (L->left >= objnum)
				L->left++;
		}
	}
}

static void RawInsertSector(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumSectors);

	gDocument.sectors.push_back(NULL);

	for (int n = NumSectors-1 ; n > objnum ; n--)
		gDocument.sectors[n] = gDocument.sectors[n - 1];

	gDocument.sectors[objnum] = reinterpret_cast<Sector*>(ptr);

	// fix all sidedef references

	if (objnum+1 < NumSectors)
	{
		for (int n = NumSideDefs-1 ; n >= 0 ; n--)
		{
			SideDef *S = gDocument.sidedefs[n];

			if (S->sector >= objnum)
				S->sector++;
		}
	}
}

static int * RawDeleteThing(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumThings);

	int * result = reinterpret_cast<int*>(gDocument.things[objnum]);

	for (int n = objnum ; n < NumThings-1 ; n++)
		gDocument.things[n] = gDocument.things[n + 1];

	gDocument.things.pop_back();

	return result;
}

static void RawInsert(ObjType objtype, int objnum, int *ptr)
{
	did_make_changes = true;

	Clipboard_NotifyInsert(objtype, objnum);
	Selection_NotifyInsert(objtype, objnum);
	 MapStuff_NotifyInsert(objtype, objnum);
	 Render3D_NotifyInsert(objtype, objnum);
	ObjectBox_NotifyInsert(objtype, objnum);

	switch (objtype)
	{
		case ObjType::things:
			RawInsertThing(objnum, ptr);
			break;

		case ObjType::vertices:
			RawInsertVertex(objnum, ptr);
			break;

		case ObjType::sidedefs:
			RawInsertSideDef(objnum, ptr);
			break;

		case ObjType::sectors:
			RawInsertSector(objnum, ptr);
			break;

		case ObjType::linedefs:
			RawInsertLineDef(objnum, ptr);
			break;

		default:
			BugError("RawInsert: bad objtype %d\n", (int)objtype);
	}
}


static int * RawDeleteLineDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumLineDefs);

	int * result = reinterpret_cast<int*>(gDocument.linedefs[objnum]);

	for (int n = objnum ; n < NumLineDefs-1 ; n++)
		gDocument.linedefs[n] = gDocument.linedefs[n + 1];

	gDocument.linedefs.pop_back();

	return result;
}

static int * RawDeleteVertex(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumVertices);

	int * result = reinterpret_cast<int*>(gDocument.vertices[objnum]);

	for (int n = objnum ; n < NumVertices-1 ; n++)
		gDocument.vertices[n] = gDocument.vertices[n + 1];

	gDocument.vertices.pop_back();

	// fix the linedef references

	if (objnum < NumVertices)
	{
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = gDocument.linedefs[n];

			if (L->start > objnum)
				L->start--;

			if (L->end > objnum)
				L->end--;
		}
	}

	return result;
}

static int * RawDeleteSideDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSideDefs);

	int * result = reinterpret_cast<int*>(gDocument.sidedefs[objnum]);

	for (int n = objnum ; n < NumSideDefs-1 ; n++)
		gDocument.sidedefs[n] = gDocument.sidedefs[n + 1];

	gDocument.sidedefs.pop_back();

	// fix the linedefs references

	if (objnum < NumSideDefs)
	{
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = gDocument.linedefs[n];

			if (L->right > objnum)
				L->right--;

			if (L->left > objnum)
				L->left--;
		}
	}

	return result;
}

static int * RawDeleteSector(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSectors);

	int * result = reinterpret_cast<int*>(gDocument.sectors[objnum]);

	for (int n = objnum ; n < NumSectors-1 ; n++)
		gDocument.sectors[n] = gDocument.sectors[n + 1];

	gDocument.sectors.pop_back();

	// fix sidedef references

	if (objnum < NumSectors)
	{
		for (int n = NumSideDefs-1 ; n >= 0 ; n--)
		{
			SideDef *S = gDocument.sidedefs[n];

			if (S->sector > objnum)
				S->sector--;
		}
	}

	return result;
}

static int * RawDelete(ObjType objtype, int objnum)
{
	did_make_changes = true;

	Clipboard_NotifyDelete(objtype, objnum);
	Selection_NotifyDelete(objtype, objnum);
	 MapStuff_NotifyDelete(objtype, objnum);
	 Render3D_NotifyDelete(objtype, objnum);
	ObjectBox_NotifyDelete(objtype, objnum);

	switch (objtype)
	{
		case ObjType::things:
			return RawDeleteThing(objnum);

		case ObjType::vertices:
			return RawDeleteVertex(objnum);

		case ObjType::sectors:
			return RawDeleteSector(objnum);

		case ObjType::sidedefs:
			return RawDeleteSideDef(objnum);

		case ObjType::linedefs:
			return RawDeleteLineDef(objnum);

		default:
			BugError("RawDelete: bad objtype %d\n", (int)objtype);
			return NULL; /* NOT REACHED */
	}
}


static void DeleteFinally(ObjType objtype, int *ptr)
{
// fprintf(stderr, "DeleteFinally: %p\n", ptr);
	switch (objtype)
	{
		case ObjType::things:   delete reinterpret_cast<Thing   *>(ptr); break;
		case ObjType::vertices: delete reinterpret_cast<Vertex  *>(ptr); break;
		case ObjType::sectors:  delete reinterpret_cast<Sector  *>(ptr); break;
		case ObjType::sidedefs: delete reinterpret_cast<SideDef *>(ptr); break;
		case ObjType::linedefs: delete reinterpret_cast<LineDef *>(ptr); break;

		default:
			BugError("DeleteFinally: bad objtype %d\n", (int)objtype);
	}
}


static void RawChange(ObjType objtype, int objnum, int field, int *value)
{
	int * pos = NULL;

	switch (objtype)
	{
		case ObjType::things:
			SYS_ASSERT(0 <= objnum && objnum < NumThings);
			pos = reinterpret_cast<int*>(gDocument.things[objnum]);
			break;

		case ObjType::vertices:
			SYS_ASSERT(0 <= objnum && objnum < NumVertices);
			pos = reinterpret_cast<int*>(gDocument.vertices[objnum]);
			break;

		case ObjType::sectors:
			SYS_ASSERT(0 <= objnum && objnum < NumSectors);
			pos = reinterpret_cast<int*>(gDocument.sectors[objnum]);
			break;

		case ObjType::sidedefs:
			SYS_ASSERT(0 <= objnum && objnum < NumSideDefs);
			pos = reinterpret_cast<int*>(gDocument.sidedefs[objnum]);
			break;

		case ObjType::linedefs:
			SYS_ASSERT(0 <= objnum && objnum < NumLineDefs);
			pos = reinterpret_cast<int*>(gDocument.linedefs[objnum]);
			break;

		default:
			BugError("RawGetBase: bad objtype %d\n", (int)objtype);
			return; /* NOT REACHED */
	}

	std::swap(pos[field], *value);

	did_make_changes = true;

	Clipboard_NotifyChange(objtype, objnum, field);
	Selection_NotifyChange(objtype, objnum, field);
	 MapStuff_NotifyChange(objtype, objnum, field);
	 Render3D_NotifyChange(objtype, objnum, field);
	ObjectBox_NotifyChange(objtype, objnum, field);
}


//------------------------------------------------------------------------
//  BASIS API IMPLEMENTATION
//------------------------------------------------------------------------

enum class EditOp : byte
{
	none,
	change,
	insert,
	del
};


class edit_op_c
{
public:
	EditOp action = EditOp::none;

	ObjType objtype = ObjType::things;
	byte field = 0;

	int objnum = 0;

	int *ptr = nullptr;
	int value = 0;

public:
	void Apply()
	{
		switch (action)
		{
			case EditOp::change:
				RawChange(objtype, objnum, (int)field, &value);
				return;

			case EditOp::del:
				ptr = RawDelete(objtype, objnum);
				action = EditOp::insert;  // reverse the operation
				return;

			case EditOp::insert:
				RawInsert(objtype, objnum, ptr);
				ptr = NULL;
				action = EditOp::del;  // reverse the operation
				return;

			default:
				BugError("edit_op_c::Apply\n");
		}
	}

	void Destroy()
	{
// fprintf(stderr, "edit_op_c::Destroy %p action = '%c'\n", this, (action == 0) ? ' ' : action);
		if (action == EditOp::insert)
		{
			SYS_ASSERT(ptr);
			DeleteFinally(objtype, ptr);
		}
		else if (action == EditOp::del)
		{
			SYS_ASSERT(! ptr);
		}
	}
};


#define MAX_UNDO_MESSAGE  200

#define DEFAULT_UNDO_GROUP_MESSAGE "[something]"

class undo_group_c
{
private:
	std::vector<edit_op_c> ops;

	int dir = 0;	// dir must be +1 or -1 if active

	SString message = DEFAULT_UNDO_GROUP_MESSAGE;

public:
	undo_group_c() = default;
	~undo_group_c()
	{
		for(auto it = ops.rbegin(); it != ops.rend(); ++it)
			it->Destroy();
	}

	// Ensure we only use move semantics
	undo_group_c(const undo_group_c &other) = delete;
	undo_group_c &operator = (const undo_group_c &other) = delete;

	undo_group_c(undo_group_c &&other) noexcept
	{
		*this = std::move(other);
	}
	undo_group_c &operator = (undo_group_c &&other) noexcept
	{
		ops = std::move(other.ops);
		dir = other.dir;
		message = std::move(other.message);

		other.Reset();	// ensure the other goes into the default state
		return *this;
	}

	//
	// When it's "deleted".
	//
	void Reset()
	{
		ops.clear();
		dir = 0;
		message = DEFAULT_UNDO_GROUP_MESSAGE;
	}

	bool IsActive() const
	{
		return !!dir;
	}
	void Activate()
	{
		dir = +1;
	}

	bool Empty() const
	{
		return ops.empty();
	}

	void Add_Apply(const edit_op_c& op)
	{
		ops.push_back(op);

		ops.back().Apply();
	}

	void End()
	{
		dir = -1;
	}

	void ReApply()
	{
		int total = (int)ops.size();

		if (dir > 0)
			for (auto it = ops.begin(); it != ops.end(); ++it)
				it->Apply();
		else
			for (auto it = ops.rbegin(); it != ops.rend(); ++it)
				it->Apply();

		// reverse the order for next time
		dir = -dir;
	}

	void SetMsg(const char *buf)
	{
		message = buf;
	}

	const SString &GetMsg() const
	{
		return message;
	}
};

// TODO: move these to Document
// TODO: remove pointer usage, prefer move semantics
static undo_group_c cur_group;

static std::list<undo_group_c> undo_history;
static std::list<undo_group_c> redo_future;


inline static void ClearUndoHistory()
{
	undo_history.clear();
}

inline static void ClearRedoFuture()
{
	redo_future.clear();
}


void BA_Begin()
{
	if (cur_group.IsActive())
		BugError("BA_Begin called twice without BA_End\n");

	ClearRedoFuture();

	cur_group.Activate();

	DoClearChangeStatus();
}


void BA_End()
{
	if (! cur_group.IsActive())
		BugError("BA_End called without a previous BA_Begin\n");

	cur_group.End();

	if(cur_group.Empty())
		cur_group.Reset();
	else
	{
		undo_history.push_front(std::move(cur_group));
		Status_Set("%s", cur_group.GetMsg().c_str());
	}

	DoProcessChangeStatus();
}


void BA_Abort(bool keep_changes)
{
	if (! cur_group.IsActive())
		BugError("BA_Abort called without a previous BA_Begin\n");

	cur_group.End();

	if (! keep_changes && ! cur_group.Empty())
	{
		cur_group.ReApply();
	}

	cur_group.Reset();

	did_make_changes  = false;

	DoProcessChangeStatus();
}


void BA_Message(EUR_FORMAT_STRING(const char *msg), ...)
{
	SYS_ASSERT(msg);
	SYS_ASSERT(cur_group.IsActive());

	va_list arg_ptr;

	char buffer[MAX_UNDO_MESSAGE];

	va_start(arg_ptr, msg);
	vsnprintf(buffer, MAX_UNDO_MESSAGE, msg, arg_ptr);
	va_end(arg_ptr);

	buffer[MAX_UNDO_MESSAGE-1] = 0;

	cur_group.SetMsg(buffer);
}


void BA_MessageForSel(const char *verb, selection_c *list, const char *suffix)
{
	// utility for creating messages like "moved 3 things"

	int total = list->count_obj();

	if (total < 1)  // oops
		return;

	if (total == 1)
	{
		BA_Message("%s %s #%d%s", verb, NameForObjectType(list->what_type()), list->find_first(), suffix);
	}
	else
	{
		BA_Message("%s %d %s%s", verb, total, NameForObjectType(list->what_type(), true /* plural */), suffix);
	}
}


int BA_New(ObjType type)
{
	SYS_ASSERT(cur_group.IsActive());

	edit_op_c op;

	op.action  = EditOp::insert;
	op.objtype = type;

	switch (type)
	{
		case ObjType::things:
			op.objnum = NumThings;
			op.ptr = (int*) new Thing;
			break;

		case ObjType::vertices:
			op.objnum = NumVertices;
			op.ptr = (int*) new Vertex;
			break;

		case ObjType::sidedefs:
			op.objnum = NumSideDefs;
			op.ptr = (int*) new SideDef;
			break;

		case ObjType::linedefs:
			op.objnum = NumLineDefs;
			op.ptr = (int*) new LineDef;
			break;

		case ObjType::sectors:
			op.objnum = NumSectors;
			op.ptr = (int*) new Sector;
			break;

		default:
			BugError("BA_New: unknown type\n");
	}

	SYS_ASSERT(cur_group.IsActive());

	cur_group.Add_Apply(op);

	return op.objnum;
}


void BA_Delete(ObjType type, int objnum)
{
	SYS_ASSERT(cur_group.IsActive());

	edit_op_c op;

	op.action  = EditOp::del;
	op.objtype = type;
	op.objnum  = objnum;

	// this must happen _before_ doing the deletion (otherwise
	// when we undo, the insertion will mess up the references).
	if (type == ObjType::sidedefs)
	{
		// unbind sidedef from any linedefs using it
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = gDocument.linedefs[n];

			if (L->right == objnum)
				BA_ChangeLD(n, LineDef::F_RIGHT, -1);

			if (L->left == objnum)
				BA_ChangeLD(n, LineDef::F_LEFT, -1);
		}
	}
	else if (type == ObjType::vertices)
	{
		// delete any linedefs bound to this vertex
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = gDocument.linedefs[n];

			if (L->start == objnum || L->end == objnum)
			{
				BA_Delete(ObjType::linedefs, n);
			}
		}
	}
	else if (type == ObjType::sectors)
	{
		// delete the sidedefs bound to this sector
		for (int n = NumSideDefs-1 ; n >= 0 ; n--)
		{
			if (gDocument.sidedefs[n]->sector == objnum)
			{
				BA_Delete(ObjType::sidedefs, n);
			}
		}
	}

	SYS_ASSERT(cur_group.IsActive());

	cur_group.Add_Apply(op);
}


bool BA_Change(ObjType type, int objnum, byte field, int value)
{
	// TODO: optimise, check whether value actually changes

	edit_op_c op;

	op.action  = EditOp::change;
	op.objtype = type;
	op.field   = field;
	op.objnum  = objnum;
	op.value   = value;

	SYS_ASSERT(cur_group.IsActive());

	cur_group.Add_Apply(op);
	return true;
}


bool BA_Undo()
{
	if (undo_history.empty())
		return false;

	DoClearChangeStatus();

	undo_group_c grp = std::move(undo_history.front());
	undo_history.pop_front();

	Status_Set("UNDO: %s", grp.GetMsg().c_str());

	grp.ReApply();

	redo_future.push_front(std::move(grp));

	DoProcessChangeStatus();
	return true;
}

bool BA_Redo()
{
	if (redo_future.empty())
		return false;

	DoClearChangeStatus();

	undo_group_c grp = std::move(redo_future.front());
	redo_future.pop_front();

	Status_Set("Redo: %s", grp.GetMsg().c_str());

	grp.ReApply();

	undo_history.push_front(std::move(grp));

	DoProcessChangeStatus();
	return true;
}


void BA_ClearAll()
{
	int i;

	for (i = 0 ; i < NumThings   ; i++) 
		delete gDocument.things[i];
	for (i = 0 ; i < NumVertices ; i++) 
		delete gDocument.vertices[i];
	for (i = 0 ; i < NumSectors  ; i++) 
		delete gDocument.sectors[i];
	for (i = 0 ; i < NumSideDefs ; i++) 
		delete gDocument.sidedefs[i];
	for (i = 0 ; i < NumLineDefs ; i++) 
		delete gDocument.linedefs[i];

	gDocument.things.clear();
	gDocument.vertices.clear();
	gDocument.sectors.clear();
	gDocument.sidedefs.clear();
	gDocument.linedefs.clear();

	HeaderData.clear();
	BehaviorData.clear();
	ScriptsData.clear();

	ClearUndoHistory();
	ClearRedoFuture();

	// Note: we don't clear the string table, since there can be
	//       string references in the clipboard.

	Clipboard_ClearLocals();
}


/* HELPERS */

bool BA_ChangeTH(int thing, byte field, int value)
{
	SYS_ASSERT(is_thing(thing));
	SYS_ASSERT(field <= Thing::F_ARG5);

	if (field == Thing::F_TYPE)
		recent_things.insert_number(value);

	return BA_Change(ObjType::things, thing, field, value);
}

bool BA_ChangeVT(int vert, byte field, int value)
{
	SYS_ASSERT(is_vertex(vert));
	SYS_ASSERT(field <= Vertex::F_Y);

	return BA_Change(ObjType::vertices, vert, field, value);
}

bool BA_ChangeSEC(int sec, byte field, int value)
{
	SYS_ASSERT(is_sector(sec));
	SYS_ASSERT(field <= Sector::F_TAG);

	if (field == Sector::F_FLOOR_TEX ||
		field == Sector::F_CEIL_TEX)
	{
		recent_flats.insert(BA_GetString(value));
	}

	return BA_Change(ObjType::sectors, sec, field, value);
}

bool BA_ChangeSD(int side, byte field, int value)
{
	SYS_ASSERT(is_sidedef(side));
	SYS_ASSERT(field <= SideDef::F_SECTOR);

	if (field == SideDef::F_LOWER_TEX ||
		field == SideDef::F_UPPER_TEX ||
		field == SideDef::F_MID_TEX)
	{
		recent_textures.insert(BA_GetString(value));
	}

	return BA_Change(ObjType::sidedefs, side, field, value);
}

bool BA_ChangeLD(int line, byte field, int value)
{
	SYS_ASSERT(is_linedef(line));
	SYS_ASSERT(field <= LineDef::F_ARG5);

	return BA_Change(ObjType::linedefs, line, field, value);
}


//------------------------------------------------------------------------
//   CHECKSUM LOGIC
//------------------------------------------------------------------------

static void ChecksumThing(crc32_c& crc, const Thing * T)
{
	crc += T->raw_x;
	crc += T->raw_y;
	crc += T->angle;
	crc += T->type;
	crc += T->options;
}

static void ChecksumVertex(crc32_c& crc, const Vertex * V)
{
	crc += V->raw_x;
	crc += V->raw_y;
}

static void ChecksumSector(crc32_c& crc, const Sector * sector)
{
	crc += sector->floorh;
	crc += sector->ceilh;
	crc += sector->light;
	crc += sector->type;
	crc += sector->tag;

	crc += sector->FloorTex();
	crc += sector->CeilTex();
}

static void ChecksumSideDef(crc32_c& crc, const SideDef * S)
{
	crc += S->x_offset;
	crc += S->y_offset;

	crc += S->LowerTex();
	crc += S->MidTex();
	crc += S->UpperTex();

	ChecksumSector(crc, S->SecRef());
}

static void ChecksumLineDef(crc32_c& crc, const LineDef * L)
{
	crc += L->flags;
	crc += L->type;
	crc += L->tag;

	ChecksumVertex(crc, L->Start());
	ChecksumVertex(crc, L->End());

	if (L->Right())
		ChecksumSideDef(crc, L->Right());

	if (L->Left())
		ChecksumSideDef(crc, L->Left());
}


void BA_LevelChecksum(crc32_c& crc)
{
	// the following method conveniently skips any unused vertices,
	// sidedefs and sectors.  It also adds each sector umpteen times
	// (for each line in the sector), but that should not affect the
	// validity of the final checksum.

	int i;

	for (i = 0 ; i < NumThings ; i++)
		ChecksumThing(crc, gDocument.things[i]);

	for (i = 0 ; i < NumLineDefs ; i++)
		ChecksumLineDef(crc, gDocument.linedefs[i]);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
