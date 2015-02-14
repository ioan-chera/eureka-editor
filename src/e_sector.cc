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

#include "levels.h"
#include "e_linedef.h"
#include "e_sector.h"
#include "editloop.h"
#include "m_bitvec.h"
#include "objects.h"
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
}


void SEC_Light(void)
{
	int diff = atoi(EXEC_Param[0]);

	if (diff == 0)
	{
		Beep("SEC_Light: bad parameter '%s'", EXEC_Param[0]);
		return;
	}

	CMD_AdjustLight(diff);
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
	if (edit.Selected->count_obj() == 1 && edit.highlight.valid())
	{
		edit.Selected->set(edit.highlight.num);
	}

	if (edit.Selected->count_obj() < 2)
	{
		Beep("Need 2 or more sectors to merge");
		return;
	}

	bool keep_common_lines = Exec_HasFlag("/keep");

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

	// re-select the final sector
	edit.error_mode = false;
	edit.Selected->clear_all();
	edit.Selected->set(source);
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
