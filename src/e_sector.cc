//------------------------------------------------------------------------
//  SECTOR STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2013 Andrew Apted
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
#include "levels.h"
#include "e_linedef.h"
#include "e_sector.h"
#include "editloop.h"
#include "m_bitvec.h"
#include "objects.h"
#include "selectn.h"
#include "x_loop.h"

#include "ui_window.h"


void SEC_Floor(void)
{
	int diff = atoi(EXEC_Param[0]);

	if (diff == 0)
	{
		Beep("SEC_Floor: bad parameter '%s'", EXEC_Param[0]);
		return;
	}


	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No sectors to move");
		return;
	}


	BA_Begin();
		
	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Sector *S = Sectors[*it];

		int new_h = CLAMP(-32767, S->floorh + diff, S->ceilh);

		BA_ChangeSEC(*it, Sector::F_FLOORH, new_h);
	}

	BA_End();

	main_win->sec_box->UpdateField(Sector::F_FLOORH);
	MarkChanges();
}


void SEC_Ceil(void)
{
	int diff = atoi(EXEC_Param[0]);

	if (diff == 0)
	{
		Beep("SEC_Ceil: bad parameter '%s'", EXEC_Param[0]);
		return;
	}


	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No sectors to move");
		return;
	}

	BA_Begin();
		
	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Sector *S = Sectors[*it];

		int new_h = CLAMP(S->floorh, S->ceilh + diff, 32767);

		BA_ChangeSEC(*it, Sector::F_CEILH, new_h);
	}

	BA_End();

	main_win->sec_box->UpdateField(Sector::F_CEILH);
	MarkChanges();
}


static int light_add_delta(int level, int delta)
{
	// NOTE: delta is assumed to be a power of two

	if (abs(delta) <= 1)
	{
		level += delta;
	}
	else if (delta > 0)
	{
		level = (level | (delta-1)) + 1;
	}
	else
	{
		level = (level - 1) & ~(abs(delta)-1);
	}

	return CLAMP(0, level, 255);
}

void CMD_AdjustLight(int delta)
{
	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No sectors to adjust light");
		return;
	}

	BA_Begin();
		
	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Sector *S = Sectors[*it];

		int new_lt = light_add_delta(S->light, delta);

		BA_ChangeSEC(*it, Sector::F_LIGHT, new_lt);
	}

	BA_End();

	main_win->sec_box->UpdateField(Sector::F_LIGHT);
	MarkChanges();
}


/*
   turn a sector into a door: change the linedefs and sidedefs
*/

void MakeDoorFromSector (int sector)
{
#if 0  // TODO: MakeDoorFromSector
	int    sd1, sd2;
	int    n, s;
	SelPtr ldok, ldflip, ld1s;

	ldok = NULL;
	ldflip = NULL;
	ld1s = NULL;
	s = 0;
	/* build lists of linedefs that border the sector */
	for (n = 0 ; n < NumLineDefs ; n++)
	{

		sd1 = LineDefs[n]->right;
		sd2 = LineDefs[n]->left;
		if (sd1 >= 0 && sd2 >= 0)
		{

			if (SideDefs[sd2]->sector == sector)
			{
				SelectObject (&ldok, n); /* already ok */
				s++;
			}
			if (SideDefs[sd1]->sector == sector)
			{
				SelectObject (&ldflip, n); /* must be flipped */
				s++;
			}
		}
		else if (sd1 >= 0 && sd2 < 0)
		{

			if (SideDefs[sd1]->sector == sector)
				SelectObject (&ld1s, n); /* wall (one-sided) */
		}
	}
	/* a normal door has two sides... */
	if (s < 2)
	{
		Beep ();
		Notify (-1, -1, "The door must be connected to two other Sectors.", NULL);
		ForgetSelection (&ldok);
		ForgetSelection (&ldflip);
		ForgetSelection (&ld1s);
		return;
	}
	if ((s > 2) && !(Expert || Confirm (-1, -1, "The door will have more than two sides.", "Do you still want to create it?")))
	{
		ForgetSelection (&ldok);
		ForgetSelection (&ldflip);
		ForgetSelection (&ld1s);
		return;
	}
	/* flip the linedefs that have the wrong orientation */
	if (ldflip != NULL)
		FlipLineDefs (ldflip, 1);
	/* merge the two selection lists */
	while (ldflip != NULL)
	{
		if (!IsSelected (ldok, ldflip->objnum))
			SelectObject (&ldok, ldflip->objnum);
		UnSelectObject (&ldflip, ldflip->objnum);
	}
	/* change the linedefs and sidedefs */
	while (ldok != NULL)
	{
		/* give the "normal door" type and flags to the linedef */

		n = ldok->objnum;
		LineDefs[n]->type = 1;
		LineDefs[n]->flags = 0x04;
		sd1 = LineDefs[n]->right; /* outside */
		sd2 = LineDefs[n]->left; /* inside */
		/* adjust the textures for the sidedefs */

		if (strncmp (SideDefs[sd1]->mid_tex, "-", WAD_TEX_NAME))
		{
			if (!strncmp (SideDefs[sd1]->upper_tex, "-", WAD_TEX_NAME))
				strncpy (SideDefs[sd1]->upper_tex, SideDefs[sd1]->mid_tex, WAD_TEX_NAME);
			strncpy (SideDefs[sd1]->mid_tex, "-", WAD_TEX_NAME);
		}
		if (!strncmp (SideDefs[sd1]->upper_tex, "-", WAD_TEX_NAME))
			strncpy (SideDefs[sd1]->upper_tex, "BIGDOOR2", WAD_TEX_NAME);
		strncpy (SideDefs[sd2]->mid_tex, "-", WAD_TEX_NAME);
		UnSelectObject (&ldok, n);
	}
	while (ld1s != NULL)
	{
		/* give the "door side" flags to the linedef */

		n = ld1s->objnum;
		LineDefs[n]->flags = 0x11;
		sd1 = LineDefs[n]->right;
		/* adjust the textures for the sidedef */

		if (!strncmp (SideDefs[sd1]->mid_tex, "-", WAD_TEX_NAME))
			strncpy (SideDefs[sd1]->mid_tex, "DOORTRAK", WAD_TEX_NAME);
		strncpy (SideDefs[sd1]->upper_tex, "-", WAD_TEX_NAME);
		strncpy (SideDefs[sd1]->lower_tex, "-", WAD_TEX_NAME);
		UnSelectObject (&ld1s, n);
	}
	/* adjust the ceiling height */

	Sectors[sector]->ceilh = Sectors[sector]->floorh;

#endif
}



/*
   turn a Sector into a lift: change the linedefs and sidedefs
   */

void MakeLiftFromSector (int sector)
{
#if 0  // TODO: MakeLiftFromSector
	int    sd1, sd2;
	int    n, s, tag;
	SelPtr ldok, ldflip, ld1s;
	SelPtr sect, curs;
	int    minh, maxh;

	ldok = NULL;
	ldflip = NULL;
	ld1s = NULL;
	sect = NULL;
	/* build lists of linedefs that border the Sector */
	for (n = 0 ; n < NumLineDefs ; n++)
	{

		sd1 = LineDefs[n]->right;
		sd2 = LineDefs[n]->left;
		if (sd1 >= 0 && sd2 >= 0)
		{

			if (SideDefs[sd2]->sector == sector)
			{
				SelectObject (&ldok, n); /* already ok */
				s = SideDefs[sd1]->sector;
				if (s != sector && !IsSelected (sect, s))
					SelectObject (&sect, s);
			}
			if (SideDefs[sd1]->sector == sector)
			{
				SelectObject (&ldflip, n); /* will be flipped */
				s = SideDefs[sd2]->sector;
				if (s != sector && !IsSelected (sect, s))
					SelectObject (&sect, s);
			}
		}
		else if (sd1 >= 0 && sd2 < 0)
		{

			if (SideDefs[sd1]->sector == sector)
				SelectObject (&ld1s, n); /* wall (one-sided) */
		}
	}
	/* there must be a way to go on the lift... */
	if (sect == NULL)
	{
		Beep ();
		Notify (-1, -1, "The lift must be connected to at least one other Sector.", NULL);
		ForgetSelection (&ldok);
		ForgetSelection (&ldflip);
		ForgetSelection (&ld1s);
		return;
	}
	/* flip the linedefs that have the wrong orientation */
	if (ldflip != NULL)
		FlipLineDefs (ldflip, 1);
	/* merge the two selection lists */
	while (ldflip != NULL)
	{
		if (!IsSelected (ldok, ldflip->objnum))
			SelectObject (&ldok, ldflip->objnum);
		UnSelectObject (&ldflip, ldflip->objnum);
	}

	/* find a free tag number */
	tag = FindFreeTag ();

	/* find the minimum and maximum altitudes */

	minh = 32767;
	maxh = -32767;
	for (curs = sect ; curs ; curs = curs->next)
	{
		if (Sectors[curs->objnum]->floorh < minh)
			minh = Sectors[curs->objnum]->floorh;
		if (Sectors[curs->objnum]->floorh > maxh)
			maxh = Sectors[curs->objnum]->floorh;
	}
	ForgetSelection (&sect);

	/* change the lift's floor height if necessary */
	if (Sectors[sector]->floorh < maxh)
		Sectors[sector]->floorh = maxh;

	/* change the lift's ceiling height if necessary */
	if (Sectors[sector]->ceilh < maxh + DOOM_PLAYER_HEIGHT)
		Sectors[sector]->ceilh = maxh + DOOM_PLAYER_HEIGHT;

	/* assign the new tag number to the lift */
	Sectors[sector]->tag = tag;

	/* change the linedefs and sidedefs */
	while (ldok != NULL)
	{
		/* give the "lower lift" type and flags to the linedef */

		n = ldok->objnum;
		LineDefs[n]->type = 62; /* lower lift (switch) */
		LineDefs[n]->flags = 0x04;
		LineDefs[n]->tag = tag;
		sd1 = LineDefs[n]->right; /* outside */
		sd2 = LineDefs[n]->left; /* inside */
		/* adjust the textures for the sidedef visible from the outside */

		if (strncmp (SideDefs[sd1]->mid_tex, "-", WAD_TEX_NAME))
		{
			if (!strncmp (SideDefs[sd1]->lower_tex, "-", WAD_TEX_NAME))
				strncpy (SideDefs[sd1]->lower_tex, SideDefs[sd1]->mid_tex, WAD_TEX_NAME);
			strncpy (SideDefs[sd1]->mid_tex, "-", WAD_TEX_NAME);
		}
		if (!strncmp (SideDefs[sd1]->lower_tex, "-", WAD_TEX_NAME))
			strncpy (SideDefs[sd1]->lower_tex, "SHAWN2", WAD_TEX_NAME);
		/* adjust the textures for the sidedef visible from the lift */
		strncpy (SideDefs[sd2]->mid_tex, "-", WAD_TEX_NAME);
		s = SideDefs[sd1]->sector;

		if (Sectors[s]->floorh > minh)
		{

			if (strncmp (SideDefs[sd2]->mid_tex, "-", WAD_TEX_NAME))
			{
				if (!strncmp (SideDefs[sd2]->lower_tex, "-", WAD_TEX_NAME))
					strncpy (SideDefs[sd2]->lower_tex, SideDefs[sd1]->mid_tex, WAD_TEX_NAME);
				strncpy (SideDefs[sd2]->mid_tex, "-", WAD_TEX_NAME);
			}
			if (!strncmp (SideDefs[sd2]->lower_tex, "-", WAD_TEX_NAME))
				strncpy (SideDefs[sd2]->lower_tex, "SHAWN2", WAD_TEX_NAME);
		}
		else
		{

			strncpy (SideDefs[sd2]->lower_tex, "-", WAD_TEX_NAME);
		}
		strncpy (SideDefs[sd2]->mid_tex, "-", WAD_TEX_NAME);

		/* if the ceiling of the sector is lower than that of the lift */
		if (Sectors[s]->ceilh < Sectors[sector]->ceilh)
		{

			if (strncmp (SideDefs[sd2]->upper_tex, "-", WAD_TEX_NAME))
				strncpy (SideDefs[sd2]->upper_tex, default_upper_texture, WAD_TEX_NAME);
		}

		/* if the floor of the sector is above the lift */
		if (Sectors[s]->floorh >= Sectors[sector]->floorh)
		{

			LineDefs[n]->type = 88; /* lower lift (walk through) */
			/* flip it, just for fun */
			curs = NULL;
			SelectObject (&curs, n);
			FlipLineDefs (curs, 1);
			ForgetSelection (&curs);
		}
		/* done with this linedef */
		UnSelectObject (&ldok, n);
	}

	while (ld1s != NULL)
	{
		/* these are the lift walls (one-sided) */

		n = ld1s->objnum;
		LineDefs[n]->flags = 0x01;
		sd1 = LineDefs[n]->right;
		/* adjust the textures for the sidedef */

		if (!strncmp (SideDefs[sd1]->mid_tex, "-", WAD_TEX_NAME))
			strncpy (SideDefs[sd1]->mid_tex, default_middle_texture, WAD_TEX_NAME);
		strncpy (SideDefs[sd1]->upper_tex, "-", WAD_TEX_NAME);
		strncpy (SideDefs[sd1]->lower_tex, "-", WAD_TEX_NAME);
		UnSelectObject (&ld1s, n);
	}
#endif
}



/*
 *  linedefs_of_sector
 *  Return a bit vector of all linedefs used by the sector.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *linedefs_of_sector (obj_no_t s)
{
	bitvec_c *linedefs = new bitvec_c (NumLineDefs);

	for (int n = 0 ; n < NumLineDefs ; n++)
		if (LineDefs[n]->TouchesSector(s))
			linedefs->set (n);

	return linedefs;
}


/*
 *  linedefs_of_sector
 *  Returns the number of linedefs that face sector <s>
 *  and, if that number is greater than zero, sets <array>
 *  to point on a newly allocated array filled with the
 *  numbers of those linedefs. It's up to the caller to
 *  delete[] the array after use.
 */
int linedefs_of_sector (obj_no_t s, obj_no_t *&array)
{
	int count = 0;
#if 0  // FIXME  linedefs_of_sector shit
	for (int n = 0 ; n < NumLineDefs ; n++)
		if (   is_sidedef (LineDefs[n]->right)
				&& SideDefs[LineDefs[n]->right].sector == s
				|| is_sidedef (LineDefs[n]->left)
				&& SideDefs[LineDefs[n]->left].sector == s)
			count++;
	if (count > 0)
	{
		array = new obj_no_t[count];
		count = 0;
		for (int n = 0 ; n < NumLineDefs ; n++)
			if (   is_sidedef (LineDefs[n]->right)
					&& SideDefs[LineDefs[n]->right].sector == s
					|| is_sidedef (LineDefs[n]->left)
					&& SideDefs[LineDefs[n]->left].sector == s)
				array[count++] = n;
	}
#endif
	return count;
}


void SEC_SwapFlats()
{
	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No sectors to swap");
		return;
	}

	BA_Begin();
		
	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Sector *S = Sectors[*it];

		int floor_tex = S->floor_tex;
		int  ceil_tex = S->ceil_tex;

		BA_ChangeSEC(*it, Sector::F_FLOOR_TEX, ceil_tex);
		BA_ChangeSEC(*it, Sector::F_CEIL_TEX, floor_tex);
	}

	BA_End();

	main_win->sec_box->UpdateField(Sector::F_FLOOR_TEX);
	main_win->sec_box->UpdateField(Sector::F_CEIL_TEX);

	MarkChanges();
}


static void LineDefsBetweenSectors(selection_c *list, int sec1, int sec2)
{
	for (int i = 0 ; i < NumLineDefs ; i++)
	{
		const LineDef * L = LineDefs[i];

		if (! (L->Left() && L->Right()))
			continue;

		if ((L->Left()->sector == sec1 && L->Right()->sector == sec2) ||
		    (L->Left()->sector == sec2 && L->Right()->sector == sec1))
		{
			list->set(i);
		}
	}
}


static void ReplaceSectorRefs(int old_sec, int new_sec)
{
	for (int i = 0 ; i < NumSideDefs ; i++)
	{
		SideDef * sd = SideDefs[i];

		if (sd->sector == old_sec)
		{
			BA_ChangeSD(i, SideDef::F_SECTOR, new_sec);
		}
	}
}


void SEC_Merge(void)
{
	// need a selection
	if (edit.Selected->count_obj() == 1 && edit.highlighted())
	{
		edit.Selected->set(edit.highlighted.num);
	}

	if (edit.Selected->count_obj() < 2)
	{
		Beep("Need 2 or more sectors to merge");
		return;
	}

	bool keep_common_lines = false;

	if (tolower(EXEC_Param[0][0]) == 'k')
		keep_common_lines = true;

	int source = edit.Selected->find_first();

	selection_c common_lines(OBJ_LINEDEFS);
	selection_iterator_c it;

	BA_Begin();

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		int target = *it;

		if (target == source)
			continue;

		LineDefsBetweenSectors(&common_lines, target, source);

		ReplaceSectorRefs(target, source);
	}

	if (! keep_common_lines)
	{
		// this deletes any unused vertices/sidedefs too
		DeleteLineDefs(&common_lines);
	}

	BA_End();

	MarkChanges();
}


/*
 *  bv_vertices_of_sector
 *  Return a bit vector of all vertices used by a sector.
 *  It's up to the caller to delete the bit vector after use.
 */
bitvec_c *bv_vertices_of_sector (obj_no_t s)
{
	bitvec_c *linedefs = linedefs_of_sector(s);

	bitvec_c *vertices = bv_vertices_of_linedefs(linedefs);
	delete linedefs;

	return vertices;
}


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
