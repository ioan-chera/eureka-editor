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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
