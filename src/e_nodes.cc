//------------------------------------------------------------------------
//  BUILDING NODES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012 Andrew Apted
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
#include "levels.h"
#include "w_wad.h"


void CMD_BuildNodes()
{
	if (MadeChanges)
	{
		// FIXME: dialog "your changes are not saved"
		Beep();
		return;
	}

	if (! edit_wad)
	{
		// FIXME: dialog "require a PWAD to build nodes for"
		Beep();
		return;
	}

	// TODO
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
