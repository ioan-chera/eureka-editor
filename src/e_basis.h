//------------------------------------------------------------------------
//  BASIC OBJECT HANDLING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025-2026 Ioan Chera
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
#include "LineDef.h"
#include "m_strings.h"
#include "objid.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"
#include <memory>
#include <stack>

#define DEFAULT_UNDO_GROUP_MESSAGE "[something]"

class selection_c;
class LineDef;
struct Vertex;

namespace global
{
	extern int	default_floor_h;
	extern int	default_ceil_h;
	extern int	default_light_level;
}

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

// See objid.h for obj_type_e (OBJ_THINGS etc)

enum class LumpType : byte
{
	header,
	behavior,
	scripts
};

// E_BASIS

// Geometric epsilon divider (used for BSP building, coordinate matching, etc.)
constexpr int GEOM_EPSILON_DIVIDER = 1024;

// Distance epsilon for geometric comparisons
constexpr double GEOM_EPSILON = 1.0 / GEOM_EPSILON_DIVIDER;

FFixedPoint MakeValidCoord(MapFormat format, double x);
bool CoordsMatch(MapFormat format, const v2double_t &v1, const v2double_t &v2);

//
// Editor command manager, handles undo/redo
//
class Basis : public DocumentModule
{
public:
	enum class EditFormat
	{
		field,
		linedefDouble,
		linedefIntSet,
		sidedefDouble,
		sectorDouble,
		sectorIntSet,
	};

	struct EditField
	{
		EditField() = default;
		explicit EditField(byte field) : format(EditFormat::field), rawField(field)
		{
		}

		explicit EditField(double LineDef::*field) : format(EditFormat::linedefDouble), doubleLineField(field)
		{
		}

		explicit EditField(double SideDef::*field) : format(EditFormat::sidedefDouble), doubleSideField(field)
		{
		}

		explicit EditField(double Sector::*field) : format(EditFormat::sectorDouble), doubleSectorField(field)
		{
		}

		bool isRaw(byte rawField) const
		{
			return format == EditFormat::field && this->rawField == rawField;
		}

		int getRaw() const
		{
			return format == EditFormat::field ? rawValue : -1;
		}

		bool operator==(const EditField &other) const
		{
			if(format != other.format)
				return false;
			if(format == EditFormat::field)
				return rawField == other.rawField && rawValue == other.rawValue;
			if(format == EditFormat::sidedefDouble)
				return doubleSideField == other.doubleSideField && doubleValue == other.doubleValue;
			if(format == EditFormat::sectorDouble)
				return doubleSectorField == other.doubleSectorField && doubleValue == other.doubleValue;
			if(format == EditFormat::sectorIntSet)
				return intSetSectorField == other.intSetSectorField && intSetValue == other.intSetValue;
			return doubleLineField == other.doubleLineField && doubleValue == other.doubleValue;
		}

		EditFormat format = EditFormat::field;

		union
		{
			byte rawField = 0;
			double LineDef::*doubleLineField;
			std::set<int> LineDef::*intSetLineField;
			double SideDef::*doubleSideField;
			double Sector::*doubleSectorField;
			std::set<int> Sector::*intSetSectorField;
		};

		union
		{
			int rawValue = 0;
			double doubleValue;
		};
		std::set<int> intSetValue;
	};

	Basis(Document &doc) : DocumentModule(doc)
	{
	}

	bool undo();
	bool redo();
	void clear();
	void setSavedStack()
	{
		mSavedStack = mUndoHistory;
	}


	Basis &operator = (Basis &&other) noexcept
	{
		mCurrentGroup = std::move(other.mCurrentGroup);
		mUndoHistory = std::move(other.mUndoHistory);
		mRedoFuture = std::move(other.mRedoFuture);
		mSavedStack = std::move(other.mSavedStack);
		mDidMakeChanges = other.mDidMakeChanges;
		return *this;
	}

private:
	//
	// Edit change
	//
	enum class EditType : byte
	{
		none,	// initial state (invalid)
		change,
		insert,
		del,
		lump_change
	};

	//
	// Edit operation
	//
	struct EditUnit
	{
		EditType action = EditType::none;
		ObjType objtype = ObjType::things;

		EditField efield = {};

		int objnum = 0;
		std::shared_ptr<Thing> thing;
		std::shared_ptr<Vertex> vertex;
		std::shared_ptr<Sector> sector;
		std::shared_ptr<SideDef> sidedef;
		std::shared_ptr<LineDef> linedef;

		// For lump changes
		LumpType lumptype = LumpType::header;
		std::vector<byte> lumpData;

		void apply(Basis &basis);
		void destroy();

		bool operator == (const EditUnit &other) const
		{
			return action == other.action
				&& objtype == other.objtype
				&& efield == other.efield
				&& objnum == other.objnum
				&& thing == other.thing
				&& vertex == other.vertex
				&& sector == other.sector
				&& sidedef == other.sidedef
				&& linedef == other.linedef
				&& lumptype == other.lumptype
				&& lumpData == other.lumpData;
		}

		bool operator != (const EditUnit &other) const
		{
			return !(*this == other);
		}

	private:
		void rawChange(Basis &basis);

		void rawDelete(Basis &basis);
		std::shared_ptr<Thing> rawDeleteThing(Document &doc) const;
		std::shared_ptr<Vertex> rawDeleteVertex(Document &doc) const;
		std::shared_ptr<Sector> rawDeleteSector(Document &doc) const;
		std::shared_ptr<SideDef> rawDeleteSidedef(Document &doc) const;
		std::shared_ptr<LineDef> rawDeleteLinedef(Document &doc) const;

		void rawInsert(Basis &basis);
		void rawInsertThing(Document &doc);
		void rawInsertVertex(Document &doc);
		void rawInsertSector(Document &doc);
		void rawInsertSidedef(Document &doc);
		void rawInsertLinedef(Document &doc);

		void rawChangeLump(Basis &basis);

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

		UndoGroup(const UndoGroup &other) = default;
		UndoGroup &operator = (const UndoGroup &other) = default;

		//
		// Move constructor takes same semantics as operator
		//
		UndoGroup(UndoGroup &&other) noexcept
		{
			*this = std::move(other);
		}
		UndoGroup &operator = (UndoGroup &&other) noexcept;

		bool operator != (const UndoGroup &other) const
		{
			return mOps != other.mOps
				|| mMessage != other.mMessage
				|| mMenuName != other.mMenuName
				|| mDir != other.mDir;
		}

		bool operator == (const UndoGroup &other) const
		{
			return !(*this != other);
		}

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

		void addApply(EditUnit &&op, Basis &basis);

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

		void setMenuName(const SString &menuName)
		{
			mMenuName = menuName;
		}

		const SString &getMenuName() const
		{
			return mMenuName;
		}

	private:
		std::vector<EditUnit> mOps;
		SString mMessage = DEFAULT_UNDO_GROUP_MESSAGE;
		SString mMenuName;
		int mDir = 0;	// dir must be +1 or -1 if active
	};

	// Called exclusively from friend class
	void begin();
	void setMessage(EUR_FORMAT_STRING(const char *format), ...) EUR_PRINTF(2, 3);
	void setMessageForSelection(const char *verb, const selection_c &list, const char *suffix = "");
	int addNew(ObjType type);
	bool change(ObjType type, int objnum, byte field, int value);
	bool changeThing(int thing, Thing::IntAddress field, int value);
	bool changeThing(int thing, Thing::FixedPointAddress field, FFixedPoint value);
	bool changeVertex(int vert, byte field, FFixedPoint value);
	bool changeSector(int sec, Sector::IntAddress field, int value);
	bool changeSector(int sec, Sector::StringIDAddress field, StringID value);
	bool changeSector(int sec, double Sector::*field, double value);
	bool changeSector(int sec, std::set<int> Sector::*field, std::set<int> &&value);
	bool changeSidedef(int side, SideDef::IntAddress field, int value);
	bool changeSidedef(int side, SideDef::StringIDAddress field, StringID value);
	bool changeSidedef(int side, double SideDef::*field, double value);
	bool changeLinedef(int line, byte field, int value);
	bool changeLinedef(int line, double LineDef::*field, double value);
	bool changeLinedef(int line, std::set<int> LineDef::*field, std::set<int> &&value);
	void changeLump(LumpType lumpType, std::vector<byte> &&newData);
	void del(ObjType type, int objnum);
	void end();
	void abort(bool keepChanges);

	void doClearChangeStatus();
	void doProcessChangeStatus() const;

	UndoGroup mCurrentGroup;
	// FIXME: use a better data type here
	std::vector<UndoGroup> mUndoHistory;
	std::stack<UndoGroup> mRedoFuture;

	std::vector<UndoGroup> mSavedStack;

	bool mDidMakeChanges = false;
};

//
// Undo/redo operation
//
class EditOperation
{
public:
	EditOperation(Basis &basis) : doc(basis.doc), basis(basis)
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

	int addNew(ObjType type)
	{
		return basis.addNew(type);
	}

	bool change(ObjType type, int objnum, byte field, int value)
	{
		return basis.change(type, objnum, field, value);
	}

	bool changeThing(int thing, Thing::IntAddress field, int value)
	{
		return basis.changeThing(thing, field, value);
	}

	bool changeThing(int thing, Thing::FixedPointAddress field, FFixedPoint value)
	{
		return basis.changeThing(thing, field, value);
	}

	bool changeVertex(int vert, byte field, FFixedPoint value)
	{
		return basis.changeVertex(vert, field, value);
	}

	bool changeSector(int sec, Sector::IntAddress field, int value)
	{
		return basis.changeSector(sec, field, value);
	}
	bool changeSector(int sec, Sector::StringIDAddress field, StringID value)
	{
		return basis.changeSector(sec, field, value);
	}
	bool changeSector(int sec, double Sector::*field, double value)
	{
		return basis.changeSector(sec, field, value);
	}
	bool changeSector(int sec, std::set<int> Sector::*field, std::set<int> &&value)
	{
		return basis.changeSector(sec, field, std::move(value));
	}

	bool changeSidedef(int side, SideDef::IntAddress field, int value)
	{
		return basis.changeSidedef(side, field, value);
	}
	bool changeSidedef(int side, SideDef::StringIDAddress field, StringID value)
	{
		return basis.changeSidedef(side, field, value);
	}
	bool changeSidedef(int side, double SideDef::*field, double value)
	{
		return basis.changeSidedef(side, field, value);
	}

	bool changeLinedef(int line, byte field, int value)
	{
		return basis.changeLinedef(line, field, value);
	}

	bool changeLinedef(int line, double LineDef::*field, double value)
	{
		return basis.changeLinedef(line, field, value);
	}

	bool changeLinedef(int line, std::set<int> LineDef::*field, std::set<int> &&value)
	{
		return basis.changeLinedef(line, field, std::move(value));
	}

	void changeLump(LumpType lumpType, std::vector<byte> &&newData)
	{
		basis.changeLump(lumpType, std::move(newData));
	}

	void del(ObjType type, int objnum)
	{
		basis.del(type, objnum);
	}

	void setAbort(bool keepChanges)
	{
		abort = true;
		abortKeepChanges = keepChanges;
	}

	Document &doc;	// convenience reference to doc

private:
	Basis &basis;
	bool abort = false;
	bool abortKeepChanges = false;
};

const char *NameForObjectType(ObjType type, bool plural = false);

/* BASIS API */

// add this string to the basis string table (if it doesn't
// already exist) and return its integer offset.
StringID BA_InternaliseString(const SString &str);

// get the string from the basis string table.
SString BA_GetString(StringID offset) noexcept;

#endif  /* __EUREKA_E_BASIS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
