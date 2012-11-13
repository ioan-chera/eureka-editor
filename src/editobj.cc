//------------------------------------------------------------------------
//  OBJECT EDITING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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

#include "main.h"

#include "m_dialog.h"
#include "editloop.h"
#include "editobj.h"
#include "m_game.h"
#include "r_misc.h"
#include "levels.h"
#include "objects.h"
#include "e_sector.h"
#include "s_misc.h"
#include "selectn.h"
#include "x_mirror.h"



#if (0 == 1)  // FIXME

/*
 *   TransferThingProperties
 *
 *   -AJA- 2001-05-27
 */
void TransferThingProperties (int src_thing, SelPtr things)
{
	SelPtr cur;

	for (cur=things; cur; cur=cur->next)
	{
		if (! is_obj(cur->objnum))
			continue;

		Things[cur->objnum].angle = Things[src_thing].angle;
		Things[cur->objnum].type  = Things[src_thing].type;
		Things[cur->objnum].options  = Things[src_thing].options;

		MarkChanges();
	}
}


/*
 *   TransferSectorProperties
 *
 *   -AJA- 2001-05-27
 */
void TransferSectorProperties (int src_sector, SelPtr sectors)
{
	SelPtr cur;

	for (cur=sectors; cur; cur=cur->next)
	{
		if (! is_obj(cur->objnum))
			continue;

		strncpy (Sectors[cur->objnum].floor_tex, Sectors[src_sector].floor_tex,
				WAD_FLAT_NAME);
		strncpy (Sectors[cur->objnum].ceil_tex, Sectors[src_sector].ceil_tex,
				WAD_FLAT_NAME);

		Sectors[cur->objnum].floorh  = Sectors[src_sector].floorh;
		Sectors[cur->objnum].ceilh   = Sectors[src_sector].ceilh;
		Sectors[cur->objnum].light   = Sectors[src_sector].light;
		Sectors[cur->objnum].type = Sectors[src_sector].type;
		Sectors[cur->objnum].tag     = Sectors[src_sector].tag;

		MarkChanges();
	}
}


/*
 *   TransferLinedefProperties
 *
 *   Note: right now nothing is done about sidedefs.  Being able to
 *   (intelligently) transfer sidedef properties from source line to
 *   destination linedefs could be a useful feature -- though it is
 *   unclear the best way to do it.  OTOH not touching sidedefs might
 *   be useful too.
 *
 *   -AJA- 2001-05-27
 */
#define LINEDEF_FLAG_KEEP  (1 + 4)

void TransferLinedefProperties (int src_linedef, SelPtr linedefs)
{
	SelPtr cur;
	int src_flags = LineDefs[src_linedef].flags & ~LINEDEF_FLAG_KEEP;

	for (cur=linedefs; cur; cur=cur->next)
	{
		if (! is_obj(cur->objnum))
			continue;

		// don't transfer certain flags
		LineDefs[cur->objnum].flags &= LINEDEF_FLAG_KEEP;
		LineDefs[cur->objnum].flags |= src_flags;

		LineDefs[cur->objnum].type = LineDefs[src_linedef].type;
		LineDefs[cur->objnum].tag  = LineDefs[src_linedef].tag;

		MarkChanges();
	}
}

#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
