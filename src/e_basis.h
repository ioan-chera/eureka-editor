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

// E_BASIS

FFixedPoint MakeValidCoord(MapFormat format, double x);

//
// Editor command manager, handles undo/redo
//
class Basis : public DocumentModule
{
public:
	Basis(Document &doc) : DocumentModule(doc)
	{
	}

	bool undo();
	bool redo();
	void clear();
	
	Basis &operator = (Basis &&other) noexcept
	{
		mCurrentGroup = std::move(other.mCurrentGroup);
		mUndoHistory = std::move(other.mUndoHistory);
		mRedoFuture = std::move(other.mRedoFuture);
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
		std::shared_ptr<Thing> thing;
		std::shared_ptr<Vertex> vertex;
		std::shared_ptr<Sector> sector;
		std::shared_ptr<SideDef> sidedef;
		std::shared_ptr<LineDef> linedef;
		int value = 0;

		void apply(Basis &basis);
		void destroy();

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
	bool changeSidedef(int side, SideDef::IntAddress field, int value);
	bool changeSidedef(int side, SideDef::StringIDAddress field, StringID value);
	bool changeLinedef(int line, byte field, int value);
	void del(ObjType type, int objnum);
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

	bool changeSidedef(int side, SideDef::IntAddress field, int value)
	{
		return basis.changeSidedef(side, field, value);
	}
	bool changeSidedef(int side, SideDef::StringIDAddress field, StringID value)
	{
		return basis.changeSidedef(side, field, value);
	}

	bool changeLinedef(int line, byte field, int value)
	{
		return basis.changeLinedef(line, field, value);
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
