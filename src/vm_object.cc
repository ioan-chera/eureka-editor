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
			delete ptr.str;  ptr.str = NULL;
			break;

		default:
			delete ptr.sel;  ptr.sel = NULL;
			break;
	}
}


object_ref_c::AddRef()
{
	if (count < PERMANENT_COUNT)
		count++;
}


object_ref_c::RemoveRef()
{
	SYS_ASSERT(count != 0);

	if (count < PERMANENT_COUNT)
		count--;
}


void VM_FreePotentiallyFreeables()
{
	// TODO
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
