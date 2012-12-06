//------------------------------------------------------------------------
//  SECTOR OPS II
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

#include <map>

#include "r_misc.h"
#include "levels.h"
#include "selectn.h"
#include "objects.h"
#include "m_bitvec.h"
#include "m_dialog.h"
#include "e_sector.h"
#include "s_misc.h"
#include "w_rawdef.h"
#include "x_hover.h"
#include "x_mirror.h"


// #define DEBUG_PATH  1


/*
   Distribute sector floor heights
*/

#if 0  // FIXME: DistributeSectorFloors

void DistributeSectorFloors (SelPtr obj)
{
	SelPtr cur;
	int    n, num, floor1h, floor2h;

	num = 0;
	for (cur = obj; cur->next; cur = cur->next)
		num++;

	floor1h = Sectors[obj->objnum].floorh;
	floor2h = Sectors[cur->objnum].floorh;

	n = 0;
	for (cur = obj; cur; cur = cur->next)
	{
		Sectors[cur->objnum].floorh = floor1h + n * (floor2h - floor1h) / num;
		n++;
	}
	MarkChanges();
}
#endif



/*
   Distribute sector ceiling heights
*/

#if 0 // FIXME DistributeSectorCeilings

void DistributeSectorCeilings (SelPtr obj)
{
	SelPtr cur;
	int    n, num, ceil1h, ceil2h;

	num = 0;
	for (cur = obj; cur->next; cur = cur->next)
		num++;

	ceil1h = Sectors[obj->objnum].ceilh;
	ceil2h = Sectors[cur->objnum].ceilh;

	n = 0;
	for (cur = obj; cur; cur = cur->next)
	{
		Sectors[cur->objnum].ceilh = ceil1h + n * (ceil2h - ceil1h) / num;
		n++;
	}
	MarkChanges();
}
#endif


/*
   Raise or lower sectors
*/

#if 0 // TODO: RaiseOrLowerSectors

void RaiseOrLowerSectors (SelPtr obj)
{
	SelPtr cur;
	int  x0;          // left hand (x) window start
	int  y0;          // top (y) window start
	int  key;         // holds value returned by InputInteger
	int  delta = 0;   // user input for delta


	x0 = (ScrMaxX - 25 - 44 * FONTW) / 2;
	y0 = (ScrMaxY - 7 * FONTH) / 2;
	DrawScreenBox3D (x0, y0, x0 + 25 + 44 * FONTW, y0 + 7 * FONTH);
	set_colour (WHITE);
	DrawScreenText (x0+10, y0 + FONTH,     "Enter number of units to raise the ceilings");
	DrawScreenText (x0+10, y0 + 2 * FONTH, "and floors of selected sectors by.");
	DrawScreenText (x0+10, y0 + 3 * FONTH, "A negative number lowers them.");
	while (1)
	{
		key = InputInteger (x0+10, y0 + 5 * FONTH, &delta, -32768, 32767);
		if (key == YK_RETURN || key == YK_ESC)
			break;
		Beep ();
	}
	if (key == YK_ESC)
		return;

	for (cur = obj; cur != NULL; cur = cur->next)
	{
		Sectors[cur->objnum].ceilh += delta;
		Sectors[cur->objnum].floorh += delta;
	}
	MarkChanges();
}
#endif


/*
   Brighten or darken sectors
*/

#if 0  // TODO: BrightenOrDarkenSectors

void BrightenOrDarkenSectors (SelPtr obj)
{
	SelPtr cur;
	int  x0;          // left hand (x) window start
	int  y0;          // top (y) window start
	int  key;         // holds value returned by InputInteger
	int  delta = 0;   // user input for delta


	x0 = (ScrMaxX - 25 - 44 * FONTW) / 2;
	y0 = (ScrMaxY - 7 * FONTH) / 2;
	DrawScreenBox3D (x0, y0, x0 + 25 + 44 * FONTW, y0 + 7 * FONTH);
	set_colour (WHITE);
	DrawScreenText (x0+10, y0 + FONTH,     "Enter number of units to brighten");
	DrawScreenText (x0+10, y0 + 2 * FONTH, "the selected sectors by.");
	DrawScreenText (x0+10, y0 + 3 * FONTH, "A negative number darkens them.");
	while (1)
	{
		key = InputInteger (x0+10, y0 + 5 * FONTH, &delta, -255, 255);
		if (key == YK_RETURN || key == YK_ESC)
			break;
		Beep ();
	}
	if (key == YK_ESC)
		return;

	for (cur = obj; cur != NULL; cur = cur->next)
	{
		int light;
		light = Sectors[cur->objnum].light + delta;
		light = MAX(light, 0);
		light = MIN(light, 255);
		Sectors[cur->objnum].light = light;
	}
	MarkChanges();
}
#endif


static int find_linedef_for_area (int x, int y, int& side)
{
	int best_match = -1;

#if 0 // FIXME !!!!
	int n, m, curx;

	curx = 32767;  // Oh yes, one more hard-coded constant!

	for (n = 0 ; n < NumLineDefs ; n++)
		if ((Vertices[LineDefs[n].start].y > y)
				!= (Vertices[LineDefs[n].end].y > y))
		{
			int lx0 = Vertices[LineDefs[n].start].x;
			int ly0 = Vertices[LineDefs[n].start].y;
			int lx1 = Vertices[LineDefs[n].end].x;
			int ly1 = Vertices[LineDefs[n].end].y;
			m = lx0 + (int) ((long) (y - ly0) * (long) (lx1 - lx0)
					/ (long) (ly1 - ly0));
			if (m >= x && m < curx)
			{
				curx = m;
				best_match = n;
			}
		}

	/* now look if this linedef has a sidedef bound to one sector */
	if (best_match < 0)
		return OBJ_NO_NONE;

	if (Vertices[LineDefs[best_match].start].y
			> Vertices[LineDefs[best_match].end].y)
		side = 1;
	else
		side = 2;
#endif
	return best_match;
}


/*
   split a sector in two, adding a new linedef between the two vertices
*/

void SplitSector(int vertex1, int vertex2)
{
#if 0  // FIXME SplitSector

	SelPtr llist;
	int    curv, s, l, sd;
	char   msg1[80], msg2[80];

	/* AYM 1998-08-09 : FIXME : I'm afraid this test is not relevant
	   if the sector contains subsectors. I should ask Jim (Flynn),
	   he did something about that in DETH. */
	/* Check if there is a sector between the two vertices (in the middle) */
	Objid o;
	GetCurObject (o, OBJ_SECTORS,
			(Vertices[vertex1].x + Vertices[vertex2].x) / 2,
			(Vertices[vertex1].y + Vertices[vertex2].y) / 2);
	s = o.num;
	if (s < 0)
	{
		Beep ();
		sprintf (msg1, "There is no sector between vertex #%d and vertex #%d",
				vertex1, vertex2);
		Notify (-1, -1, msg1, NULL);
		return;
	}

	/* Check if there is a closed path from <vertex1> to <vertex2>,
	   along the edge of sector <s>. To make it faster, I scan only
	   the set of linedefs that face sector <s>. */

	obj_no_t *ld_numbers;
	int nlinedefs = linedefs_of_sector (s, ld_numbers);
	if (nlinedefs < 1)  // Can't happen
	{
		BugError("SplitSector: no linedef for sector %d\n", s);
		return;
	}
	llist = NULL;
	curv = vertex1;
	while (curv != vertex2)
	{
		printf ("%d\n", curv);
		int n;
		for (n = 0; n < nlinedefs; n++)
		{
			if (IsSelected (llist, ld_numbers[n]))
				continue;  // Already been there
			const LDPtr ld = LineDefs + ld_numbers[n];
			if (ld->start == curv
					&& is_sidedef (ld->side_R) && SideDefs[ld->side_R].sector == s)
			{
				curv = ld->end;
				SelectObject (&llist, ld_numbers[n]);
				break;
			}
			if (ld->end == curv
					&& is_sidedef (ld->side_L) && SideDefs[ld->side_L].sector == s)
			{
				curv = ld->start;
				SelectObject (&llist, ld_numbers[n]);
				break;
			}
		}
		if (n >= nlinedefs)
		{
			Beep ();
			sprintf (msg1, "Cannot find a closed path from vertex #%d to vertex #%d",
					vertex1, vertex2);
			if (curv == vertex1)
				sprintf (msg2, "There is no sidedef starting from vertex #%d"
						" on sector #%d", vertex1, s);
			else
				sprintf (msg2, "Check if sector #%d is closed"
						" (cannot go past vertex #%d)", s, curv);
			Notify (-1, -1, msg1, msg2);
			ForgetSelection (&llist);
			delete[] ld_numbers;
			return;
		}
		if (curv == vertex1)
		{
			Beep ();
			sprintf (msg1, "Vertex #%d is not on the same sector (#%d)"
					" as vertex #%d", vertex2, s, vertex1);
			Notify (-1, -1, msg1, NULL);
			ForgetSelection (&llist);
			delete[] ld_numbers;
			return;
		}
	}
	delete[] ld_numbers;
	/* now, the list of linedefs for the new sector is in llist */

	/* add the new sector, linedef and sidedefs */
	DoInsertObject (OBJ_SECTORS, s, 0, 0);
	DoInsertObject (OBJ_LINEDEFS, -1, 0, 0);
	LineDefs[NumLineDefs - 1].start = vertex1;
	LineDefs[NumLineDefs - 1].end = vertex2;
	LineDefs[NumLineDefs - 1].flags = 4;
	DoInsertObject (OBJ_SIDEDEFS, -1, 0, 0);
	SideDefs[NumSideDefs - 1].sector = s;
	strncpy (SideDefs[NumSideDefs - 1].mid_tex, "-", WAD_TEX_NAME);
	DoInsertObject (OBJ_SIDEDEFS, -1, 0, 0);
	strncpy (SideDefs[NumSideDefs - 1].mid_tex, "-", WAD_TEX_NAME);

	LineDefs[NumLineDefs - 1].side_R = NumSideDefs - 2;
	LineDefs[NumLineDefs - 1].side_L = NumSideDefs - 1;

	/* bind all linedefs in llist to the new sector */
	while (llist)
	{
		sd = LineDefs[llist->objnum].side_R;
		if (sd < 0 || SideDefs[sd].sector != s)
			sd = LineDefs[llist->objnum].side_L;
		SideDefs[sd].sector = NumSectors - 1;
		UnSelectObject (&llist, llist->objnum);
	}

	/* second check... useful for sectors within sectors */

	for (l = 0 ; l < NumLineDefs ; l++)
	{
		sd = LineDefs[l].side_R;
		if (sd >= 0 && SideDefs[sd].sector == s)
		{
			curv = GetOppositeSector (l, 1);

			if (curv == NumSectors - 1)
				SideDefs[sd].sector = NumSectors - 1;
		}
		sd = LineDefs[l].side_L;
		if (sd >= 0 && SideDefs[sd].sector == s)
		{
			curv = GetOppositeSector (l, 0);

			if (curv == NumSectors - 1)
				SideDefs[sd].sector = NumSectors - 1;
		}
	}

	MarkChanges(2);
#endif
}



/*
   split two linedefs, then split the sector and add a new linedef between the new vertices
*/

void SplitLineDefsAndSector(int linedef1, int linedef2)
{
#if 0  // FIXME SplitLineDefsAndSector
	SelPtr llist;
	char   msg[80];

	/* check if the two linedefs are adjacent to the same sector */

	int s1 = LineDefs[linedef1].side_R;
	int s2 = LineDefs[linedef1].side_L;
	int s3 = LineDefs[linedef2].side_R;
	int s4 = LineDefs[linedef2].side_L;

	if (s1 >= 0)
		s1 = SideDefs[s1].sector;
	if (s2 >= 0)
		s2 = SideDefs[s2].sector;
	if (s3 >= 0)
		s3 = SideDefs[s3].sector;
	if (s4 >= 0)
		s4 = SideDefs[s4].sector;

	if ((s1 < 0 || (s1 != s3 && s1 != s4)) && (s2 < 0 || (s2 != s3 && s2 != s4)))
	{
		Beep ();
		sprintf (msg, "Linedefs #%d and #%d are not adjacent to the same sector",
				linedef1, linedef2);
		Notify (-1, -1, msg, NULL);
		return;
	}
	/* split the two linedefs and create two new vertices */
	llist = NULL;
	SelectObject (&llist, linedef1);
	SelectObject (&llist, linedef2);
	SplitLineDefs (llist);
	ForgetSelection (&llist);

	/* split the sector and create a linedef between the two vertices */
	SplitSector (NumVertices - 1, NumVertices - 2);
#endif
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
