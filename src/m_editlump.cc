//------------------------------------------------------------------------
//  TEXT LUMP EDITOR
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2018 Andrew Apted
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
#include "m_editlump.h"
#include "w_wad.h"

#include "ui_window.h"


void CMD_EditLump()
{
	const char *lump_name = EXEC_Param[0];

	if (lump_name[0] == 0 || lump_name[0] == '/')
	{
		// ask for the lump name
		// FIXME: custom dialog

		lump_name = fl_input("Lump name?");

		if (lump_name == NULL)
			return;
	}


	// FIXME

}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
