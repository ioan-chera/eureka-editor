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

#ifndef __EUREKA_VM_OBJECT_H__
#define __EUREKA_VM_OBJECT_H__


class selection_c;


class object_ref_c
{
public:
	enum
	{
		K_INVALID = 0,

		K_STRING,

		K_LINEDEF_SEL,
		K_SIDEDEF_SEL,
		K_SECTOR_SEL,
		K_THING_SEL,
		K_VERTEX_SEL
	};

	short	kind;
	short	count;

	union
	{
		const char *str;

		selection_c *sel;
	
	} ptr;

private:
	object_ref_c();
	~object_ref_c();

public:
	static object_ref_c * NewString(const char *s);

	static object_ref_c * NewSelection(obj_type_e _type);

	void AddRef();
	void RemoveRef();

	void TryFree();
	void MakePermanent();
};


void VM_FreePotentiallyFreeables();


#endif  /* __EUREKA_VM_OBJECT_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
