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

#ifndef __EUREKA_E_BASIS_H__
#define __EUREKA_E_BASIS_H__

#include "DocumentModule.h"
#include "m_strings.h"
#include "objid.h"
#include <stack>

#define DEFAULT_UNDO_GROUP_MESSAGE "[something]"

class crc32_c;
class selection_c;
struct Document;

//
// DESIGN NOTES
//
// Every field in these structures are a plain 'int'.  This is a
// design decision aiming to simplify the logic and code for undo
// and redo.
//
// Strings are represented as offsets into a string table.
//
// These structures are always ensured to have valid fields, e.g.
// the LineDef vertex numbers are OK, the SideDef sector number is
// valid, etc.  For LineDefs, the left and right fields can contain
// -1 to mean "no sidedef", but note that a missing right sidedef
// can cause problems or crashes when playing the map in DOOM.
//


// a fixed-point coordinate with 12 bits of fractional part.
typedef int fixcoord_t;

#define  FROM_COORD(fx)  ((double)(fx) / 4096.0)
#define    TO_COORD(db)  ((fixcoord_t) I_ROUND((db) * 4096.0))

#define INT_TO_COORD(i)  ((fixcoord_t) ((i) * 4096))
#define COORD_TO_INT(i)  ((int) ((i) / 4096))

enum class Side
{
	right,
	left,
	neither
};
static const Side kSides[] = { Side::right, Side::left };
inline static Side operator - (Side side)
{
	if(side == Side::right)
		return Side::left;
	if(side == Side::left)
		return Side::right;
	return side;
}
inline static Side operator * (Side side1, Side side2)
{
	if(side1 == Side::neither || side2 == Side::neither)
		return Side::neither;
	if(side1 != side2)
		return Side::left;
	return Side::right;
}

// See objid.h for obj_type_e (OBJ_THINGS etc)


class Thing
{
public:
	fixcoord_t raw_x = 0;
	fixcoord_t raw_y = 0;

	int angle = 0;
	int type = 0;
	int options = 0;

	// Hexen stuff
	fixcoord_t raw_h = 0;

	int tid = 0;
	int special = 0;
	int arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0;

	enum { F_X, F_Y, F_ANGLE, F_TYPE, F_OPTIONS,
	       F_H, F_TID, F_SPECIAL,
		   F_ARG1, F_ARG2, F_ARG3, F_ARG4, F_ARG5 };

public:
	inline double x() const
	{
		return FROM_COORD(raw_x);
	}
	inline double y() const
	{
		return FROM_COORD(raw_y);
	}
	inline double h() const
	{
		return FROM_COORD(raw_h);
	}

	// these handle rounding to integer in non-UDMF mode
	void SetRawX(const Instance &inst, double x);
	void SetRawY(const Instance &inst, double y);
	void SetRawH(const Instance &inst, double h);

	void SetRawXY(const Instance &inst, double x, double y)
	{
		SetRawX(inst, x);
		SetRawY(inst, y);
	}

	int Arg(int which /* 1..5 */) const
	{
		if (which == 1) return arg1;
		if (which == 2) return arg2;
		if (which == 3) return arg3;
		if (which == 4) return arg4;
		if (which == 5) return arg5;

		return 0;
	}
};


class Vertex
{
public:
	fixcoord_t raw_x = 0;
	fixcoord_t raw_y = 0;

	enum { F_X, F_Y };

public:
	inline double x() const
	{
		return FROM_COORD(raw_x);
	}
	inline double y() const
	{
		return FROM_COORD(raw_y);
	}

	// these handle rounding to integer in non-UDMF mode
	void SetRawX(const Instance &inst, double x);
	void SetRawY(const Instance &inst, double y);

	void SetRawXY(const Instance &inst, double x, double y)
	{
		SetRawX(inst, x);
		SetRawY(inst, y);
	}

	bool Matches(fixcoord_t ox, fixcoord_t oy) const
	{
		return (raw_x == ox) && (raw_y == oy);
	}

	bool operator == (const Vertex &other) const
	{
		return raw_x == other.raw_x && raw_y == other.raw_y;
	}
	bool operator != (const Vertex &other) const
	{
		return raw_x != other.raw_x || raw_y != other.raw_y;
	}
};


class Sector
{
public:
	int floorh = 0;
	int ceilh = 0;
	int floor_tex = 0;
	int ceil_tex = 0;
	int light = 0;
	int type = 0;
	int tag = 0;

	enum { F_FLOORH, F_CEILH, F_FLOOR_TEX, F_CEIL_TEX, F_LIGHT, F_TYPE, F_TAG };

public:
	SString FloorTex() const;
	SString CeilTex() const;

	int HeadRoom() const
	{
		return ceilh - floorh;
	}

	void SetDefaults(const Instance &inst);
};


class SideDef
{
public:
	int x_offset = 0;
	int y_offset = 0;
	int upper_tex = 0;
	int mid_tex = 0;
	int lower_tex = 0;
	int sector = 0;

	enum { F_X_OFFSET, F_Y_OFFSET, F_UPPER_TEX, F_MID_TEX, F_LOWER_TEX, F_SECTOR };

public:

	SString UpperTex() const;
	SString MidTex()   const;
	SString LowerTex() const;

	Sector *SecRef(const Document &doc) const;

	// use new_tex when >= 0, otherwise use default_wall_tex
	void SetDefaults(const Instance &inst, bool two_sided, int new_tex = -1);
};


class LineDef
{
public:
	int start = 0;
	int end = 0;
	int right = -1;
	int left = -1;

	int flags = 0;
	int type = 0;
	int tag = 0;

	// Hexen stuff  [NOTE: tag is 'arg1']
	int arg2 = 0;
	int arg3 = 0;
	int arg4 = 0;
	int arg5 = 0;

	enum { F_START, F_END, F_RIGHT, F_LEFT,
	       F_FLAGS, F_TYPE, F_TAG,
		   F_ARG2, F_ARG3, F_ARG4, F_ARG5 };

public:
	Vertex *Start(const Document &doc) const;
	Vertex *End(const Document &doc)   const;

	// remember: these two can return NULL!
	SideDef *Right(const Document &doc) const;
	SideDef *Left(const Document &doc)  const;

	bool TouchesVertex(int v_num) const
	{
		return (start == v_num) || (end == v_num);
	}

	//
	// Assuming TouchesVertex(v_num), return the other one. Undefined otherwise.
	//
	int OtherVertex(int v_num) const
	{
		return start == v_num ? end : start;
	}

	bool TouchesCoord(fixcoord_t tx, fixcoord_t ty, const Document &doc) const
	{
		return Start(doc)->Matches(tx, ty) || End(doc)->Matches(tx, ty);
	}

	bool TouchesSector(int sec_num, const Document &doc) const;

	bool NoSided() const
	{
		return (right < 0) && (left < 0);
	}

	bool OneSided() const
	{
		return (right >= 0) && (left < 0);
	}

	bool TwoSided() const
	{
		return (right >= 0) && (left >= 0);
	}

	// side is either SIDE_LEFT or SIDE_RIGHT
	int WhatSector(Side side, const Document &doc) const;
	int WhatSideDef(Side side) const;

	double CalcLength(const Document &doc) const;

	bool IsZeroLength(const Document &doc) const
	{
		return (Start(doc)->raw_x == End(doc)->raw_x) && (Start(doc)->raw_y == End(doc)->raw_y);
	}

	bool IsSelfRef(const Document &doc) const;

	bool IsHorizontal(const Document &doc) const
	{
		return (Start(doc)->raw_y == End(doc)->raw_y);
	}

	bool IsVertical(const Document &doc) const
	{
		return (Start(doc)->raw_x == End(doc)->raw_x);
	}

	int Arg(int which /* 1..5 */) const
	{
		if (which == 1) return tag;
		if (which == 2) return arg2;
		if (which == 3) return arg3;
		if (which == 4) return arg4;
		if (which == 5) return arg5;

		return 0;
	}
};

//
// Editor command manager, handles undo/redo
//
class Basis : public DocumentModule
{
public:
	Basis(Document &doc) : DocumentModule(doc)
	{
	}

	void setMessage(EUR_FORMAT_STRING(const char *format), ...) EUR_PRINTF(2, 3);
	void setMessageForSelection(const char *verb, const selection_c &list, const char *suffix = "");
	int addNew(ObjType type);
	void del(ObjType type, int objnum);
	bool change(ObjType type, int objnum, byte field, int value);
	bool changeThing(int thing, byte field, int value);
	bool changeVertex(int vert, byte field, int value);
	bool changeSector(int sec, byte field, int value);
	bool changeSidedef(int side, byte field, int value);
	bool changeLinedef(int line, byte field, int value);
	bool undo();
	bool redo();
	void clearAll();

private:
	//
	// Edit change
	//
	enum class EditType : byte
	{
		none,	// initial state (invalid)
		change,
		insert,
		del
	};

	//
	// Edit operation
	//
	struct EditUnit
	{
		EditType action = EditType::none;
		ObjType objtype = ObjType::things;
		byte field = 0;
		int objnum = 0;
		union
		{
			int *ptr = nullptr;
			Thing *thing;
			Vertex *vertex;
			Sector *sector;
			SideDef *sidedef;
			LineDef *linedef;
		};
		int value = 0;

		void apply(Basis &basis);
		void destroy();

	private:
		void rawChange(Basis &basis);

		void *rawDelete(Basis &basis) const;
		Thing *rawDeleteThing(Document &doc) const;
		Vertex *rawDeleteVertex(Document &doc) const;
		Sector *rawDeleteSector(Document &doc) const;
		SideDef *rawDeleteSidedef(Document &doc) const;
		LineDef *rawDeleteLinedef(Document &doc) const;

		void rawInsert(Basis &basis) const;
		void rawInsertThing(Document &doc) const;
		void rawInsertVertex(Document &doc) const;
		void rawInsertSector(Document &doc) const;
		void rawInsertSidedef(Document &doc) const;
		void rawInsertLinedef(Document &doc) const;

		void deleteFinally();
	};

	friend class EditOperation;
	friend struct EditUnit;

	//
	// Undo operation group
	//
	class UndoGroup
	{
	public:
		UndoGroup() = default;
		~UndoGroup()
		{
			for(auto it = mOps.rbegin(); it != mOps.rend(); ++it)
				it->destroy();
		}
		
		// Ensure we only use move semantics
		UndoGroup(const UndoGroup &other) = delete;
		UndoGroup &operator = (const UndoGroup &other) = delete;

		//
		// Move constructor takes same semantics as operator
		//
		UndoGroup(UndoGroup &&other) noexcept
		{
			*this = std::move(other);
		}
		UndoGroup &operator = (UndoGroup &&other) noexcept;

		void reset();
		
		//
		// Mark if active and ready to use
		//
		bool isActive() const
		{
			return !!mDir;
		}

		//
		// Start it
		//
		void activate()
		{
			mDir = +1;
		}

		//
		// Whether it's empty of data
		//
		bool isEmpty() const
		{
			return mOps.empty();
		}

		void addApply(const EditUnit &op, Basis &basis);

		//
		// End current action
		//
		void end()
		{
			mDir = -1;
		}

		void reapply(Basis &basis);

		//
		// Get the message
		//
		const SString &getMessage() const
		{
			return mMessage;
		}

		//
		// Put the message 
		//
		void setMessage(const SString &message)
		{
			mMessage = message;
		}

	private:
		std::vector<EditUnit> mOps;
		SString mMessage = DEFAULT_UNDO_GROUP_MESSAGE;
		int mDir = 0;	// dir must be +1 or -1 if active
	};

	// Called exclusively from friend class
	void begin();
	void end();
	void abort(bool keepChanges);

	void doClearChangeStatus();
	void doProcessChangeStatus() const;

	UndoGroup mCurrentGroup;
	// FIXME: use a better data type here
	std::stack<UndoGroup> mUndoHistory;
	std::stack<UndoGroup> mRedoFuture;

	bool mDidMakeChanges = false;
};

//
// Undo/redo operation
//
class EditOperation
{
public:
	EditOperation(Basis &basis) : basis(basis)
	{
		basis.begin();
	}
	~EditOperation()
	{
		if(abort)
			basis.abort(abortKeepChanges);
		else
			basis.end();
	}

	void setMessage(EUR_FORMAT_STRING(const char *format), ...) EUR_PRINTF(2, 3);
	void setMessageForSelection(const char *verb, const selection_c &list, const char *suffix = "")
	{
		basis.setMessageForSelection(verb, list, suffix);
	}

	void setAbort(bool keepChanges)
	{
		abort = true;
		abortKeepChanges = keepChanges;
	}

private:
	Basis &basis;
	bool abort = false;
	bool abortKeepChanges = false;
};

const char *NameForObjectType(ObjType type, bool plural = false);

/* BASIS API */

// add this string to the basis string table (if it doesn't
// already exist) and return its integer offset.
int BA_InternaliseString(const SString &str);

// get the string from the basis string table.
SString BA_GetString(int offset);

#endif  /* __EUREKA_E_BASIS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
