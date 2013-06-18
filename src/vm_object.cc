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
	// TODO
}


object_ref_c::RemoveRef()
{
	// TODO
}


void VM_FreePotentiallyFreeables()
{
	// TODO
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
