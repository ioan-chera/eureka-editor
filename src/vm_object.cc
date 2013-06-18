//------------------------------------------------------------------------
//  VM : OBJECT MANAGEMENT
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2013 Andrew Apted
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

#include "m_select.h"
#include "vm_object.h"

#include <map>


/*
	MEMORY MODEL

	These objects have a reference count.  Whenever one is stored
	in a global variable, the count is increased and the previous
	object's count is decreased.

	When the reference count reaches zero, the object is placed in
	a "potentially freeable" list.  Once the script has finished
	(from the VM_Call), then this list is scanned and objects which
	still have a zero reference count can be freed.

	Objects can exist on the stack, and may have a zero reference
	count [the count does _not_ include the stack].  The logic above
	is adequate for this VM, as the stack no longer exists after the
	VM_Call() is complete.  But the above logic is not able to free
	objects during script execution.

	And of course reference counting is not adequate for objects
	which can create cyclic references.  The current kind of objects
	types do not enable creating cyclic structures.
*/


static std::map< object_ref_c *, int > potentially_freeables;


// once the count reaches this, an object is never freed
#define PERMANENT_COUNT  65535


object_ref_c::object_ref_c() : kind(K_INVALID), count(0)
{
	ptr.str = NULL;
}


object_ref_c::~object_ref_c()
{
	switch (kind)
	{
		case K_INVALID: /* nothing to free */
			break;

		case K_STRING:
			StringFree(ptr.str);  ptr.str = NULL;
			break;

		case K_LINEDEF_SEL:
		case K_SIDEDEF_SEL:
		case K_SECTOR_SEL:
		case K_THING_SEL:
		case K_VERTEX_SEL:
			delete ptr.sel;  ptr.sel = NULL;
			break;
	}
}


void object_ref_c::AddRef()
{
	if (count < PERMANENT_COUNT)
		count++;
}


void object_ref_c::RemoveRef()
{
	SYS_ASSERT(count != 0);

	if (count == PERMANENT_COUNT)
		return;

	count--;

	if (count == 0)
		potentially_freeables[this] = 1;
}


void object_ref_c::TryFree()
{
	if (count == 0)
		delete this;
}


object_ref_c * object_ref_c::NewString(const char *s)
{
	object_ref_c * ref = new object_ref_c();

	ref->kind = K_STRING;
	ref->ptr.str = StringDup(s);

	return ref;
}


object_ref_c * object_ref_c::NewSelection(obj_type_e _type)
{
	object_ref_c * ref = new object_ref_c();

	switch (_type)
	{
		case OBJ_LINEDEFS:
			ref->kind = K_LINEDEF_SEL;
			break;

		case OBJ_SIDEDEFS:
			ref->kind = K_SIDEDEF_SEL;
			break;

		case OBJ_SECTORS:
			ref->kind = K_SECTOR_SEL;
			break;

		case OBJ_THINGS:
			ref->kind = K_THING_SEL;
			break;

		case OBJ_VERTICES:
			ref->kind = K_VERTEX_SEL;
			break;

		default:
			BugError("INTERNAL ERROR: bad sel_type for object_ref_c::NewSelection\n");
			break;
	}

	ref->ptr.sel = new selection_c(_type);

	return ref;
}


void VM_FreePotentiallyFreeables()
{
	std::map< object_ref_c *, int >::iterator IT;

	for (IT = potentially_freeables.begin() ; IT != potentially_freeables.end() ; IT++)
	{
		IT->first->TryFree();
	}

	potentially_freeables.clear();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
