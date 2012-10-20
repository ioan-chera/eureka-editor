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



/*
   get the name of an object type
*/
const char *GetObjectTypeName (int objtype)
{
	switch (objtype)
	{
		case OBJ_THINGS:   return "thing";
		case OBJ_LINEDEFS: return "linedef";
		case OBJ_SIDEDEFS: return "sidedef";
		case OBJ_VERTICES: return "vertex";
		case OBJ_SECTORS:  return "sector";
		case OBJ_RADTRIGS: return "radtrig";
	}
	return "< Bug! >";
}



/*
   what are we editing?
   */
const char *GetEditModeName (int objtype)
{
	switch (objtype)
	{
		case OBJ_THINGS:   return "Things";
		case OBJ_LINEDEFS:
		case OBJ_SIDEDEFS: return "LD & SD";
		case OBJ_VERTICES: return "Vertices";
		case OBJ_SECTORS:  return "Sectors";
		case OBJ_RADTRIGS:  return "Hippies";
	}
	return "< Bug! >";
}



/*
   get a short description of the flags of a linedef
   */

const char *GetLineDefFlagsName (int flags)
{
	static char buf[20];
	// "P" is a Boom extension ("pass through")
	// "T" is for Strife ("translucent")
	const char *flag_chars = "???T??PANBSLU2MI";
	int n;

	char *p = buf;
	for (n = 0; n < 16; n++)
	{
		if (n != 0 && n % 4 == 0)
			*p++ = ' ';
		if (flags & (0x8000u >> n))
			*p++ = flag_chars[n];
		else
			*p++ = '-';
	}
	*p = '\0';
	return buf;

#if 0
	static char temp[20];
	if (flags & 0x0100)
		strcpy (temp, "A"); /* Already on the map (Ma) */
	else
		strcpy (temp, "-");
	if (flags & 0x80)
		strcat (temp, "V"); /* Invisible on the map (In) */
	else
		strcat (temp, "-");
	if (flags & 0x40)
		strcat (temp, "B"); /* Blocks sound (So) */
	else
		strcat (temp, "-");
	if (flags & 0x20)
		strcat (temp, "S"); /* Secret (normal on the map) (Se) */
	else
		strcat (temp, "-");
	if (flags & 0x10)
		strcat (temp, "L"); /* Lower texture offset changed (Lo) */
	else
		strcat (temp, "-");
	if (flags & 0x08)
		strcat (temp, "U"); /* Upper texture offset changed (Up) */
	else
		strcat (temp, "-");
	if (flags & 0x04)
		strcat (temp, "2"); /* Two-sided (2S) */
	else
		strcat (temp, "-");
	if (flags & 0x02)
		strcat (temp, "M"); /* Monsters can't cross this line (Mo) */
	else
		strcat (temp, "-");
	if (flags & 0x01)
		strcat (temp, "I"); /* Impassible (Im) */
	else
		strcat (temp, "-");
	if (strlen (temp) > 13)
	{
		temp[13] = '|';
		temp[14] = '\0';
	}
	return temp;
#endif
}



/*
   get a long description of one linedef flag
*/

const char *GetLineDefFlagsLongName (int flags)
{
	if (flags & 0x1000) return "Translucent [Strife]";
	if (flags & 0x200)  return "Pass-through [Boom]";
	if (flags & 0x100)  return "Always shown on the map";
	if (flags & 0x80)   return "Never shown on the map";
	if (flags & 0x40)   return "Blocks sound";
	if (flags & 0x20)   return "Secret (shown as normal on the map)";
	if (flags & 0x10)   return "Lower texture is \"unpegged\"";
	if (flags & 0x08)   return "Upper texture is \"unpegged\"";
	if (flags & 0x04)   return "Two-sided (may be transparent)";
	if (flags & 0x02)   return "Monsters cannot cross this line";
	if (flags & 0x01)   return "Impassible";
	return "UNKNOWN";
}



/*
   insert a standard object at given position
*/

void InsertStandardObject (int xpos, int ypos, int choice)
{
#if 0  // TODO !!! InsertStandardObject

	int sector;
	int n;
	int a, b;

	/* show where the object will be inserted */
	DrawPointer (1);
	/* are we inside a Sector? */
	Objid o;
	GetCurObject (o, OBJ_SECTORS, xpos, ypos);
	sector = o.num;

	/* !!!! Should also check for overlapping objects (sectors) !!!! */
	switch (choice)
	{
		case 1:
			a = 256;
			b = 128;
			if (Input2Numbers (-1, -1, "Width", "Height", 2000, 2000, &a, &b))
			{
				if (a < 8)
					a = 8;
				if (b < 8)
					b = 8;
				xpos = xpos - a / 2;
				ypos = ypos - b / 2;
				InsertObject (OBJ_VERTICES, -1, xpos, ypos);
				InsertObject (OBJ_VERTICES, -1, xpos + a, ypos);
				InsertObject (OBJ_VERTICES, -1, xpos + a, ypos + b);
				InsertObject (OBJ_VERTICES, -1, xpos, ypos + b);
				if (sector < 0)
					InsertObject (OBJ_SECTORS, -1, 0, 0);
				for (n = 0; n < 4; n++)
				{
					InsertObject (OBJ_LINEDEFS, -1, 0, 0);
					LineDefs[NumLineDefs - 1].side_R = NumSideDefs;
					InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
					if (sector >= 0)
						SideDefs[NumSideDefs - 1].sector = sector;
				}

				if (sector >= 0)
				{
					LineDefs[NumLineDefs - 4].start = NumVertices - 4;
					LineDefs[NumLineDefs - 4].end = NumVertices - 3;
					LineDefs[NumLineDefs - 3].start = NumVertices - 3;
					LineDefs[NumLineDefs - 3].end = NumVertices - 2;
					LineDefs[NumLineDefs - 2].start = NumVertices - 2;
					LineDefs[NumLineDefs - 2].end = NumVertices - 1;
					LineDefs[NumLineDefs - 1].start = NumVertices - 1;
					LineDefs[NumLineDefs - 1].end = NumVertices - 4;
				}
				else
				{
					LineDefs[NumLineDefs - 4].start = NumVertices - 1;
					LineDefs[NumLineDefs - 4].end = NumVertices - 2;
					LineDefs[NumLineDefs - 3].start = NumVertices - 2;
					LineDefs[NumLineDefs - 3].end = NumVertices - 3;
					LineDefs[NumLineDefs - 2].start = NumVertices - 3;
					LineDefs[NumLineDefs - 2].end = NumVertices - 4;
					LineDefs[NumLineDefs - 1].start = NumVertices - 4;
					LineDefs[NumLineDefs - 1].end = NumVertices - 1;
				}
			}
			break;
		case 2:
			a = 8;
			b = 128;
			if (Input2Numbers (-1, -1, "Number of sides", "Radius", 32, 2000, &a, &b))
			{
				if (a < 3)
					a = 3;
				if (b < 8)
					b = 8;
				InsertPolygonVertices (xpos, ypos, a, b);
				if (sector < 0)
					InsertObject (OBJ_SECTORS, -1, 0, 0);
				for (n = 0; n < a; n++)
				{
					InsertObject (OBJ_LINEDEFS, -1, 0, 0);
					LineDefs[NumLineDefs - 1].side_R = NumSideDefs;
					InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
					if (sector >= 0)
						SideDefs[NumSideDefs - 1].sector = sector;
				}

				if (sector >= 0)
				{
					LineDefs[NumLineDefs - 1].start = NumVertices - 1;
					LineDefs[NumLineDefs - 1].end = NumVertices - a;
					for (n = 2; n <= a; n++)
					{
						LineDefs[NumLineDefs - n].start = NumVertices - n;
						LineDefs[NumLineDefs - n].end = NumVertices - n + 1;
					}
				}
				else
				{
					LineDefs[NumLineDefs - 1].start = NumVertices - a;
					LineDefs[NumLineDefs - 1].end = NumVertices - 1;
					for (n = 2; n <= a; n++)
					{
						LineDefs[NumLineDefs - n].start = NumVertices - n + 1;
						LineDefs[NumLineDefs - n].end = NumVertices - n;
					}
				}
			}
			break;
		case 3:
			/*
			   a = 6;
			   b = 16;
			   if (Input2Numbers (-1, -1, "Number of steps", "Step height", 32, 48, &a, &b))
			   {
			   if (a < 2)
			   a = 2;

			   n = Sectors[sector].ceilh;
			   h = Sectors[sector].floorh;
			   if (a * b < n - h)
			   {
			   Beep ();
			   Notify (-1, -1, "The stairs are too high for this Sector", 0);
			   return;
			   }
			   xpos = xpos - 32;
			   ypos = ypos - 32 * a;
			   for (n = 0; n < a; n++)
			   {
			   InsertObject (OBJ_VERTICES, -1, xpos, ypos);
			   InsertObject (OBJ_VERTICES, -1, xpos + 64, ypos);
			   InsertObject (OBJ_VERTICES, -1, xpos + 64, ypos + 64);
			   InsertObject (OBJ_VERTICES, -1, xpos, ypos + 64);
			   ypos += 64;
			   InsertObject (OBJ_SECTORS, sector, 0, 0);
			   h += b;
			   Sectors[NumSectors - 1].floorh = h;

			   InsertObject (OBJ_LINEDEFS, -1, 0, 0);
			   LineDefs[NumLineDefs - 1].side_R = NumSideDefs;
			   LineDefs[NumLineDefs - 1].side_L = NumSideDefs + 1;
			   InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
			   SideDefs[NumSideDefs - 1].sector = sector;
			   InsertObject (OBJ_SIDEDEFS, -1, 0, 0);


			   LineDefs[NumLineDefs - 4].start = NumVertices - 4;
			   LineDefs[NumLineDefs - 4].end = NumVertices - 3;
			   LineDefs[NumLineDefs - 3].start = NumVertices - 3;
			   LineDefs[NumLineDefs - 3].end = NumVertices - 2;
			   LineDefs[NumLineDefs - 2].start = NumVertices - 2;
			   LineDefs[NumLineDefs - 2].end = NumVertices - 1;
			   LineDefs[NumLineDefs - 1].start = NumVertices - 1;
			   LineDefs[NumLineDefs - 1].end = NumVertices - 4;
			   }
			   }
			   break;
			   */
		case 4:
			NotImplemented ();
			break;
	}
#endif
}



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
