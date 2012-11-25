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

#ifndef __EUREKA_E_BASIS_H__
#define __EUREKA_E_BASIS_H__


//
// DESIGN NOTES
//
// Every field in these structures are a plain 'int'.  This is a
// design decision aiming to simplify the logic and code for undo
// and redo.
//
// Strings are represented as offsets into a string table, where
// fetching the actual (read-only) string is fast, but adding new
// strings is slow (with the current code).
//
// These structures are always ensured to have valid fields, e.g.
// the LineDef vertex numbers are OK, the SideDef sector number is
// valid, etc.  For LineDefs, either/both of side_L and side_R can
// contain -1 to mean "no sidedef", but note that a missing right
// sidedef can cause problems when playing the map in DOOM.
//


// Object types
typedef enum
{
	OBJ_NONE,
	OBJ_THINGS,
	OBJ_LINEDEFS,
	OBJ_SIDEDEFS,
	OBJ_VERTICES,
	OBJ_SECTORS,
	OBJ_RADTRIGS,
}
obj_type_e;


class Thing
{
public:
	int x;
	int y;
	int angle;
	int type;
	int options;

	enum { F_X, F_Y, F_ANGLE, F_TYPE, F_OPTIONS };

public:
	Thing() : x(0), y(0), angle(0), type(0), options(7)
	{ }

	void RawCopy(const Thing *other)
	{
		x = other->x;
		y = other->y;
		angle = other->angle;
		type = other->type;
		options = other->options;
	}
};


class Vertex
{
public:
	int x;
	int y;

	enum { F_X, F_Y };

public:
	Vertex() : x(0), y(0)
	{ }

	void RawCopy(const Vertex *other)
	{
		x = other->x;
		y = other->y;
	}

	bool Matches(int ox, int oy) const
	{
		return (x == ox) && (y == oy);
	}

	bool Matches(const Vertex *other) const
	{
		return (x == other->x) && (y == other->y);
	}
};


class Sector
{
public:
	int floorh;
	int ceilh;
	int floor_tex;
	int ceil_tex;
	int light;
	int type;
	int tag;

	enum { F_FLOORH, F_CEILH, F_FLOOR_TEX, F_CEIL_TEX, F_LIGHT, F_TYPE, F_TAG };

public:
	Sector() : floorh(0), ceilh(0), floor_tex(0), ceil_tex(0),
	           light(0), type(0), tag(0)
	{ }

	void RawCopy(const Sector *other)
	{
		floorh = other->floorh;
		ceilh  = other->ceilh;
		floor_tex = other->floor_tex;
		ceil_tex  = other->ceil_tex;
		light  = other->light;
		type   = other->type;
		tag    = other->tag;
	}

	const char *FloorTex() const;
	const char *CeilTex() const;

	int HeadRoom() const
	{
		return ceilh - floorh;
	}

	void SetDefaults();
};


class SideDef
{
public:
	int x_offset;
	int y_offset;
	int upper_tex;
	int mid_tex;
	int lower_tex;
	int sector;

	enum { F_X_OFFSET, F_Y_OFFSET, F_UPPER_TEX, F_MID_TEX, F_LOWER_TEX, F_SECTOR };

public:
	SideDef() : x_offset(0), y_offset(0), upper_tex(0), mid_tex(0),
				lower_tex(0), sector(0)
	{ }

	void RawCopy(const SideDef *other)
	{
		x_offset  = other->x_offset;
		y_offset  = other->y_offset;
		upper_tex = other->upper_tex;
		mid_tex   = other->mid_tex;
		lower_tex = other->lower_tex;
		sector    = other->sector;
	}

	const char *UpperTex() const;
	const char *MidTex()   const;
	const char *LowerTex() const;

	Sector *SecRef() const;

	void SetDefaults(bool two_sided);
};


class LineDef
{
public:
	int start;
	int end;
	int flags;
	int type;
	int tag;
	int right;
	int left;

	enum { F_START, F_END, F_FLAGS, F_TYPE, F_TAG, F_RIGHT, F_LEFT };

public:
	LineDef() : start(0), end(0), flags(0), type(0), tag(0),
				right(-1), left(-1)
	{ }

	void RawCopy(const LineDef *other)
	{
		start = other->start;
		end   = other->end;
		flags = other->flags;
		type  = other->type;
		tag   = other->tag;
		right = other->right;
		left  = other->left;
	}

	Vertex *Start() const;
	Vertex *End()   const;

	// remember: these two can return NULL!
	SideDef *Right() const;
	SideDef *Left()  const;

	bool TouchesVertex(int v_num) const
	{
		return (start == v_num) || (end == v_num);
	}

	bool TouchesSector(int sec_num) const;

	bool OneSided() const
	{
		return (right >= 0) && (left < 0);
	}

	bool TwoSided() const
	{
		return (right >= 0) && (left >= 0);
	}

	// side is either SIDE_LEFT or SIDE_RIGHT
	int WhatSector(int side) const;

	double CalcLength() const;

	bool isZeroLength() const
	{
		return (Start()->x == End()->x) && (Start()->y == End()->y);
	}
};


#define RADF_Square  (1 << 20)

class RadTrig
{
public:
	int mx;  // mid-point
	int my;
	int rw;  // radius (width/2 and height/2)
	int rh;

	int z1;
	int z2;

	int name;
	int tag;
	int options;
	int code;

	enum { F_MX, F_MY, F_RW, F_RH, F_Z1, F_Z2, F_NAME, F_TAG, F_FLAGS, F_CODE };

public:
	RadTrig() : mx(0), my(0), rw(128), rh(128), z1(0), z2(0),
				name(0), tag(0), options(0), code(0)
	{ }

	void RawCopy(const RadTrig *other)
	{
		mx = other->mx; my = other->my;
		rw = other->rw; rh = other->rh;
		z1 = other->z1; z2 = other->z2;

		name = other->name;
		tag  = other->tag;
		options = other->options;
		code = other->code;
	}

	const char *Name() const;
	const char *Code() const;

	bool isSquare() const
	{
		return (options & RADF_Square) ? true : false;
	}
};


typedef struct Vertex  *VPtr;
typedef struct Thing   *TPtr;
typedef struct LineDef *LDPtr;
typedef struct SideDef *SDPtr;
typedef struct Sector  *SPtr;


extern std::vector<Thing *>   Things;
extern std::vector<Vertex *>  Vertices;
extern std::vector<Sector *>  Sectors;
extern std::vector<SideDef *> SideDefs;
extern std::vector<LineDef *> LineDefs;
extern std::vector<RadTrig *> RadTrigs;

#define NumThings     ((int)Things.size())
#define NumVertices   ((int)Vertices.size())
#define NumSectors    ((int)Sectors.size())
#define NumSideDefs   ((int)SideDefs.size())
#define NumLineDefs   ((int)LineDefs.size())
#define NumRadTrigs   ((int)RadTrigs.size())

extern int NumObjects(obj_type_e type);

#define is_thing(n)    ((n) >= 0 && (n) < NumThings  )
#define is_vertex(n)   ((n) >= 0 && (n) < NumVertices)
#define is_sector(n)   ((n) >= 0 && (n) < NumSectors )
#define is_sidedef(n)  ((n) >= 0 && (n) < NumSideDefs)
#define is_linedef(n)  ((n) >= 0 && (n) < NumLineDefs)
#define is_radtrig(n)  ((n) >= 0 && (n) < NumRadTrigs)


/* BASIS API */

// begin a group of operations that will become a single undo/redo
// step.  All stored _redo_ steps will be removed.  The BA_New,
// BA_Delete and BA_Change calls must only be called between
// BA_Begin() and BA_End() pairs.
void BA_Begin();

// finish a group of operations.
void BA_End();

// create a new object, returning its objnum.  It is safe to
// directly set the new object's fields after calling BA_New().
int BA_New(obj_type_e type);

// deletes the given object, and in certain cases other types of
// objects bound to it (e.g. deleting a vertex will cause all
// bound linedefs to also be deleted).
void BA_Delete(obj_type_e type, int objnum);

// change a field of an existing object.  If the value was the
// same as before, nothing happens and false is returned.
// Otherwise returns true.
bool BA_Change(obj_type_e type, int objnum, byte field, int value);

// attempt to undo the last normal or redo operation.  Returns
// false if the undo history is empty.
bool BA_Undo();

// attempt to re-do the last undo operation.  Returns false if
// there is no stored redo steps.
bool BA_Redo();

// add this string to the basis string table (if it doesn't
// already exist) and return its integer offset.
int BA_InternaliseString(const char *str);
int BA_InternaliseShortStr(const char *str, int max_len);

// get the string from the basis string table.
const char *BA_GetString(int offset);

// clear everything (before loading a new level).
void BA_ClearAll();


/* HELPERS */

bool BA_ChangeTH(int thing, byte field, int value);
bool BA_ChangeVT(int vert,  byte field, int value);
bool BA_ChangeSEC(int sec,  byte field, int value);
bool BA_ChangeSD(int side,  byte field, int value);
bool BA_ChangeLD(int line,  byte field, int value);
bool BA_ChangeRAD(int rad,  byte field, int value);


#endif  /* __EUREKA_E_BASIS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
