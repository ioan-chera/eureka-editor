//------------------------------------------------------------------------
//  BASIC OBJECT HANDLING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
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

#include <algorithm>
#include <list>
#include <string>

#include "e_basis.h"
#include "levels.h"
#include "m_strings.h"

#include "m_game.h"  // g_default_xxx

// need these for the XXX_Notify() prototypes
#include "e_cutpaste.h"
#include "objects.h"
#include "selectn.h"


std::vector<Thing *>   Things;
std::vector<Vertex *>  Vertices;
std::vector<Sector *>  Sectors;
std::vector<SideDef *> SideDefs;
std::vector<LineDef *> LineDefs;
std::vector<RadTrig *> RadTrigs;


int default_floor_h		=   0;
int default_ceil_h		= 128;
int default_light_level	= 176;
int default_thing		= 2001;

const char * default_floor_tex	= "FLAT1";
const char * default_ceil_tex	= "FLAT1";
const char * default_lower_tex	= "STARTAN3";
const char * default_mid_tex	= "STARTAN3";
const char * default_upper_tex	= "STARTAN3";


bool did_make_changes;
bool wrecked_the_nodes;


string_table_c basis_strtab;


int NumObjects(obj_type_e type)
{
	switch (type)
	{
		case OBJ_THINGS:   return NumThings;
		case OBJ_LINEDEFS: return NumLineDefs;
		case OBJ_SIDEDEFS: return NumSideDefs;
		case OBJ_VERTICES: return NumVertices;
		case OBJ_SECTORS:  return NumSectors;
		case OBJ_RADTRIGS: return NumRadTrigs;

		default:
			BugError("NumObjects: bad type: %d\n", (int)type);
			return 0; /* NOT REACHED */
	}
}


static void DoClearChangeStatus()
{
	did_make_changes  = false;
	wrecked_the_nodes = false;

	Clipboard_NotifyBegin();
	Selection_NotifyBegin();
	 MapStuff_NotifyBegin();
	ObjectBox_NotifyBegin();
}

static void DoProcessChangeStatus()
{
	if (wrecked_the_nodes)
		MarkChanges(2);
	else if (did_make_changes)
		MarkChanges(1);

	Clipboard_NotifyEnd();
	Selection_NotifyEnd();
	 MapStuff_NotifyEnd();
	ObjectBox_NotifyEnd();
}


int BA_InternaliseString(const char *str)
{
	return basis_strtab.add(str);
}

int BA_InternaliseShortStr(const char *str, int max_len)
{
	std::string goodie(str, max_len);

	int result = BA_InternaliseString(goodie.c_str());

	return result;
}

const char *BA_GetString(int offset)
{
	return basis_strtab.get(offset);
}


const char * Sector::FloorTex() const
{
	return basis_strtab.get(floor_tex);
}

const char * Sector::CeilTex() const
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


const char * SideDef::UpperTex() const
{
	return basis_strtab.get(upper_tex);
}

const char * SideDef::MidTex() const
{
	return basis_strtab.get(mid_tex);
}

const char * SideDef::LowerTex() const
{
	return basis_strtab.get(lower_tex);
}

void SideDef::SetDefaults(bool two_sided)
{
	lower_tex = BA_InternaliseString(default_lower_tex);
	upper_tex = BA_InternaliseString(default_upper_tex);

	if (two_sided)
		mid_tex = BA_InternaliseString("-");
	else
		mid_tex = BA_InternaliseString(default_mid_tex);
}


Sector * SideDef::SecRef() const
{
	return Sectors[sector];
}

Vertex * LineDef::Start() const
{
	return Vertices[start];
}

Vertex * LineDef::End() const
{
	return Vertices[end];
}

SideDef * LineDef::Right() const
{
	return (right >= 0) ? SideDefs[right] : NULL;
}

SideDef * LineDef::Left() const
{
	return (left >= 0) ? SideDefs[left] : NULL;
}


bool LineDef::TouchesSector(int sec_num) const
{
	if (right >= 0 && SideDefs[right]->sector == sec_num)
		return true;

	if (left >= 0 && SideDefs[left]->sector == sec_num)
		return true;

	return false;
}


int LineDef::WhatSector(int side) const
{
	switch (side)
	{
		case SIDE_LEFT:
			return Left() ? Left()->sector : -1;

		case SIDE_RIGHT:
			return Right() ? Right()->sector : -1;

		default:
			BugError("bad side : %d\n", side);
			return -1;
	}
}


double LineDef::CalcLength() const
{
	int dx = Start()->x - End()->x;
	int dy = Start()->y - End()->y;

	return sqrt(dx * dx + dy * dy);
}


//------------------------------------------------------------------------


static void RawInsertThing(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumThings);

	Things.push_back(NULL);

	for (int n = NumThings-1 ; n > objnum ; n--)
		Things[n] = Things[n - 1];

	Things[objnum] = (Thing*) ptr;
}

static void RawInsertLineDef(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumLineDefs);

	LineDefs.push_back(NULL);

	for (int n = NumLineDefs-1 ; n > objnum ; n--)
		LineDefs[n] = LineDefs[n - 1];

	LineDefs[objnum] = (LineDef*) ptr;
}

static void RawInsertVertex(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumVertices);

	Vertices.push_back(NULL);

	for (int n = NumVertices-1 ; n > objnum ; n--)
		Vertices[n] = Vertices[n - 1];

	Vertices[objnum] = (Vertex*) ptr;

	// fix references in linedefs

	if (objnum+1 < NumVertices)
	{
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = LineDefs[n];

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

	SideDefs.push_back(NULL);

	for (int n = NumSideDefs-1 ; n > objnum ; n--)
		SideDefs[n] = SideDefs[n - 1];

	SideDefs[objnum] = (SideDef*) ptr;

	// fix the linedefs references

	if (objnum+1 < NumSideDefs)
	{
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = LineDefs[n];

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

	Sectors.push_back(NULL);

	for (int n = NumSectors-1 ; n > objnum ; n--)
		Sectors[n] = Sectors[n - 1];

	Sectors[objnum] = (Sector*) ptr;

	// fix all sidedef references

	if (objnum+1 < NumSectors)
	{
		for (int n = NumSideDefs-1 ; n >= 0 ; n--)
		{
			SideDef *S = SideDefs[n];

			if (S->sector >= objnum)
				S->sector++;
		}
	}
}

static void RawInsertRadTrig(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumRadTrigs);

	RadTrigs.push_back(NULL);

	for (int n = NumRadTrigs-1 ; n > objnum ; n--)
		RadTrigs[n] = RadTrigs[n - 1];

	RadTrigs[objnum] = (RadTrig*) ptr;
}


static int * RawDeleteThing(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumThings);

	int * result = (int*) Things[objnum];

	for (int n = objnum ; n < NumThings-1 ; n++)
		Things[n] = Things[n + 1];

	Things.pop_back();

	return result;
}

static void RawInsert(obj_type_e objtype, int objnum, int *ptr)
{
	did_make_changes = true;

	Clipboard_NotifyInsert(objtype, objnum);
	Selection_NotifyInsert(objtype, objnum);
	 MapStuff_NotifyInsert(objtype, objnum);
	ObjectBox_NotifyInsert(objtype, objnum);

	switch (objtype)
	{
		case OBJ_THINGS:
			RawInsertThing(objnum, ptr);
			break;

		case OBJ_VERTICES:
			wrecked_the_nodes = true;
			RawInsertVertex(objnum, ptr);
			break;

		case OBJ_SIDEDEFS:
			RawInsertSideDef(objnum, ptr);
			break;

		case OBJ_SECTORS:
			RawInsertSector(objnum, ptr);
			break;

		case OBJ_LINEDEFS:
			wrecked_the_nodes = true;
			RawInsertLineDef(objnum, ptr);
			break;

		case OBJ_RADTRIGS:
			RawInsertRadTrig(objnum, ptr);
			break;

		default:
			BugError("RawInsert: bad objtype %d", (int) objtype);
	}
}


static int * RawDeleteLineDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumLineDefs);

	int * result = (int*) LineDefs[objnum];

	for (int n = objnum ; n < NumLineDefs-1 ; n++)
		LineDefs[n] = LineDefs[n + 1];
	
	LineDefs.pop_back();

	return result;
}

static int * RawDeleteVertex(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumVertices);

	int * result = (int*) Vertices[objnum];

	for (int n = objnum ; n < NumVertices-1 ; n++)
		Vertices[n] = Vertices[n + 1];

	Vertices.pop_back();

	// fix the linedef references

	if (objnum < NumVertices)
	{
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = LineDefs[n];

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

	int * result = (int*) SideDefs[objnum];

	for (int n = objnum ; n < NumSideDefs-1 ; n++)
		SideDefs[n] = SideDefs[n + 1];

	SideDefs.pop_back();

	// fix the linedefs references

	if (objnum < NumSideDefs)
	{
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = LineDefs[n];

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

	int * result = (int*) Sectors[objnum];

	for (int n = objnum ; n < NumSectors-1 ; n++)
		Sectors[n] = Sectors[n + 1];

	Sectors.pop_back();

	// fix sidedef references

	if (objnum < NumSectors)
	{
		for (int n = NumSideDefs-1 ; n >= 0 ; n--)
		{
			SideDef *S = SideDefs[n];

			if (S->sector > objnum)
				S->sector--;
		}
	}

	return result;
}

static int * RawDeleteRadTrig(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumRadTrigs);

	int * result = (int*) RadTrigs[objnum];

	for (int n = objnum ; n < NumRadTrigs-1 ; n++)
		RadTrigs[n] = RadTrigs[n + 1];

	RadTrigs.pop_back();

	return result;
}

static int * RawDelete(obj_type_e objtype, int objnum)
{
	did_make_changes = true;

	Clipboard_NotifyDelete(objtype, objnum);
	Selection_NotifyDelete(objtype, objnum);
	 MapStuff_NotifyDelete(objtype, objnum);
	ObjectBox_NotifyDelete(objtype, objnum);

	switch (objtype)
	{
		case OBJ_THINGS:
			return RawDeleteThing(objnum);

		case OBJ_VERTICES:
			wrecked_the_nodes = true;
			return RawDeleteVertex(objnum);

		case OBJ_SECTORS:
			return RawDeleteSector(objnum);

		case OBJ_SIDEDEFS:
			return RawDeleteSideDef(objnum);
		
		case OBJ_LINEDEFS:
			wrecked_the_nodes = true;
			return RawDeleteLineDef(objnum);

		case OBJ_RADTRIGS:
			return RawDeleteRadTrig(objnum);

		default:
			BugError("RawDelete: bad objtype %d", (int) objtype);
			return NULL; /* NOT REACHED */
	}
}


static void DeleteFinally(obj_type_e objtype, int *ptr)
{
// fprintf(stderr, "DeleteFinally: %p\n", ptr);
	switch (objtype)
	{
		case OBJ_THINGS:   delete (Thing *)   ptr; break;
		case OBJ_VERTICES: delete (Vertex *)  ptr; break;
		case OBJ_SECTORS:  delete (Sector *)  ptr; break;
		case OBJ_SIDEDEFS: delete (SideDef *) ptr; break;
		case OBJ_LINEDEFS: delete (LineDef *) ptr; break;
		case OBJ_RADTRIGS: delete (RadTrig *) ptr; break;

		default:
			BugError("DeleteFinally: bad objtype %d", (int) objtype);
	}
}


static void RawChange(obj_type_e objtype, int objnum, int field, int *value)
{
	int * pos = NULL;

	switch (objtype)
	{
		case OBJ_THINGS:
			wrecked_the_nodes = true;
			pos = (int*) Things[objnum];
			break;

		case OBJ_VERTICES:
			pos = (int*) Vertices[objnum];
			break;

		case OBJ_SECTORS:
			pos = (int*) Sectors[objnum];
			break;

		case OBJ_SIDEDEFS:
			pos = (int*) SideDefs[objnum];
			break;
		
		case OBJ_LINEDEFS:
			wrecked_the_nodes = true;
			pos = (int*) LineDefs[objnum];
			break;

		case OBJ_RADTRIGS:
			pos = (int*) RadTrigs[objnum];
			break;

		default:
			BugError("RawGetBase: bad objtype %d", (int) objtype);
			return; /* NOT REACHED */
	}

	std::swap(pos[field], *value);

	did_make_changes = true;

	Clipboard_NotifyChange(objtype, objnum, field);
	Selection_NotifyChange(objtype, objnum, field);
	 MapStuff_NotifyChange(objtype, objnum, field);
	ObjectBox_NotifyChange(objtype, objnum, field);
}


//------------------------------------------------------------------------
//  BASIS API IMPLEMENTATION
//------------------------------------------------------------------------


enum
{
	OP_CHANGE = 'c',
	OP_INSERT = 'i',
	OP_DELETE = 'd',
};


class edit_op_c
{
public:
	char action;
	byte objtype;
	byte field;
	byte _pad;

	int objnum;

	int *ptr;
	int value;

public:
	edit_op_c() : action(0), objtype(0), field(0), objnum(0), ptr(NULL), value(0)
	{ }

	~edit_op_c()
	{ }

	void Apply()
	{
		switch (action)
		{
			case OP_CHANGE:
				RawChange((obj_type_e)objtype, objnum, (int)field, &value);
				return;

			case OP_DELETE:
				ptr = RawDelete((obj_type_e)objtype, objnum);
				action = OP_INSERT;  // reverse the operation
				return;

			case OP_INSERT:
				RawInsert((obj_type_e)objtype, objnum, ptr);
				ptr = NULL;
				action = OP_DELETE;  // reverse the operation
				return;

			default:
				BugError("edit_op_c::Apply");
		}
	}

	void Destroy()
	{
// fprintf(stderr, "edit_op_c::Destroy %p action = '%c'\n", this, (action == 0) ? ' ' : action);
		if (action == OP_INSERT)
		{
			SYS_ASSERT(ptr);
			DeleteFinally((obj_type_e) objtype, ptr);
		}
		else if (action == OP_DELETE)
		{
			SYS_ASSERT(! ptr);
		}
	}
};


class undo_group_c
{
private:
	std::vector<edit_op_c> ops;

	int dir;

public:
	undo_group_c() : ops(), dir(+1)
	{ }

	~undo_group_c()
	{
		for (int i = (int)ops.size() - 1 ; i >= 0 ; i--)
		{
			ops[i].Destroy();
		}
	}

	bool Empty() const
	{
		return ops.empty();
	}

	void Add_Apply(edit_op_c& op)
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
			for (int i = 0; i < total; i++)
				ops[i].Apply();
		else
			for (int i = total-1; i >= 0; i--)
				ops[i].Apply();

		// reverse the order for next time
		dir = -dir;
	}
};


static undo_group_c *cur_group;

static std::list<undo_group_c *> undo_history;
static std::list<undo_group_c *> redo_future;


static void ClearUndoHistory()
{
	std::list<undo_group_c *>::iterator LI;

	for (LI = undo_history.begin(); LI != undo_history.end(); LI++)
		delete *LI;

	undo_history.clear();
}

static void ClearRedoFuture()
{
	std::list<undo_group_c *>::iterator LI;

	for (LI = redo_future.begin(); LI != redo_future.end(); LI++)
		delete *LI;

	redo_future.clear();
}


void BA_Begin()
{
	if (cur_group)
		BugError("BA_Begin called twice without BA_End\n");

	ClearRedoFuture();

	cur_group = new undo_group_c();

	DoClearChangeStatus();
}

void BA_End()
{
	if (! cur_group)
		BugError("BA_End called without a previous BA_Begin\n");

	cur_group->End();

	if (! cur_group->Empty())
	{
		undo_history.push_front(cur_group);
	}

	cur_group = NULL;

	DoProcessChangeStatus();
}


int BA_New(obj_type_e type)
{
	edit_op_c op;

	op.action  = OP_INSERT;
	op.objtype = type;

	switch (type)
	{
		case OBJ_THINGS:
			op.objnum = NumThings;
			op.ptr = (int*) new Thing;
			break;

		case OBJ_VERTICES:
			op.objnum = NumVertices;
			op.ptr = (int*) new Vertex;
			break;

		case OBJ_SIDEDEFS:
			op.objnum = NumSideDefs;
			op.ptr = (int*) new SideDef;
			break;

		case OBJ_LINEDEFS:
			op.objnum = NumLineDefs;
			op.ptr = (int*) new LineDef;
			break;

		case OBJ_SECTORS:
			op.objnum = NumSectors;
			op.ptr = (int*) new Sector;
			break;

		case OBJ_RADTRIGS:
			op.objnum = NumRadTrigs;
			op.ptr = (int*) new RadTrig;
			break;

		default: BugError("BA_New");
	}

	SYS_ASSERT(cur_group);

	cur_group->Add_Apply(op);

	return op.objnum;
}


void BA_Delete(obj_type_e type, int objnum)
{
	edit_op_c op;

	op.action  = OP_DELETE;
	op.objtype = type;
	op.objnum  = objnum;

	// this must happen  _before_ doing the deletion (otherwise
	// when we undo, the insertion will mess up the references).
	if (type == OBJ_SIDEDEFS)
	{
		// unbind sidedef from any linedefs using it
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->right == objnum)
				BA_ChangeLD(n, LineDef::F_RIGHT, -1);

			if (L->left == objnum)
				BA_ChangeLD(n, LineDef::F_LEFT, -1);
		}
	}
	else if (type == OBJ_VERTICES)
	{
		// delete any linedefs bound to this vertex
		for (int n = NumLineDefs-1 ; n >= 0 ; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->start == objnum || L->end == objnum)
			{
				BA_Delete(OBJ_LINEDEFS, n);
			}
		}
	}
	else if (type == OBJ_SECTORS)
	{
		// delete the sidedefs bound to this sector
		for (int n = NumSideDefs-1 ; n >= 0 ; n--)
		{
			if (SideDefs[n]->sector == objnum)
			{
				BA_Delete(OBJ_SIDEDEFS, n);
			}
		}
	}

	SYS_ASSERT(cur_group);

	cur_group->Add_Apply(op);
}


bool BA_Change(obj_type_e type, int objnum, byte field, int value)
{
	// TODO: optimise, check whether value actually changes

	edit_op_c op;

	op.action  = OP_CHANGE;
	op.objtype = type;
	op.field   = field;
	op.objnum  = objnum;
	op.value   = value;

	SYS_ASSERT(cur_group);

	cur_group->Add_Apply(op);
	return true;
}


bool BA_Undo()
{
	if (undo_history.empty())
		return false;

	DoClearChangeStatus();

	undo_group_c * grp = undo_history.front();
	undo_history.pop_front();

	grp->ReApply();

	redo_future.push_front(grp);

	DoProcessChangeStatus();
	return true;
}

bool BA_Redo()
{
	if (redo_future.empty())
		return false;
	
	DoClearChangeStatus();

	undo_group_c * grp = redo_future.front();
	redo_future.pop_front();

	grp->ReApply();

	undo_history.push_front(grp);

	DoProcessChangeStatus();
	return true;
}


void BA_ClearAll()
{
	int i;

	for (i = 0 ; i < NumThings   ; i++) delete Things[i];
	for (i = 0 ; i < NumVertices ; i++) delete Vertices[i];
	for (i = 0 ; i < NumSectors  ; i++) delete Sectors[i];
	for (i = 0 ; i < NumSideDefs ; i++) delete SideDefs[i];
	for (i = 0 ; i < NumLineDefs ; i++) delete LineDefs[i];
	for (i = 0 ; i < NumRadTrigs ; i++) delete RadTrigs[i];

	Things.clear();
	Vertices.clear();
	Sectors.clear();
	SideDefs.clear();
	LineDefs.clear();
	RadTrigs.clear();

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
	SYS_ASSERT(field <= Thing::F_OPTIONS);

	return BA_Change(OBJ_THINGS, thing, field, value);
}

bool BA_ChangeVT(int vert, byte field, int value)
{
	SYS_ASSERT(is_vertex(vert));
	SYS_ASSERT(field <= Vertex::F_Y);

	return BA_Change(OBJ_VERTICES, vert, field, value);
}

bool BA_ChangeSEC(int sec, byte field, int value)
{
	SYS_ASSERT(is_sector(sec));
	SYS_ASSERT(field <= Sector::F_TAG);

	return BA_Change(OBJ_SECTORS, sec, field, value);
}

bool BA_ChangeSD(int side, byte field, int value)
{
	SYS_ASSERT(is_sidedef(side));
	SYS_ASSERT(field <= SideDef::F_SECTOR);

	return BA_Change(OBJ_SIDEDEFS, side, field, value);
}

bool BA_ChangeLD(int line, byte field, int value)
{
	SYS_ASSERT(is_linedef(line));
	SYS_ASSERT(field <= LineDef::F_LEFT);

	return BA_Change(OBJ_LINEDEFS, line, field, value);
}

bool BA_ChangeRAD(int rad, byte field, int value)
{
	SYS_ASSERT(is_radtrig(rad));
	SYS_ASSERT(field <= RadTrig::F_CODE);

	return BA_Change(OBJ_RADTRIGS, rad, field, value);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
