//------------------------------------------------------------------------
//  SECTOR STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
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
#include "e_objects.h"
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

	BA_MessageForSel(diff < 0 ? "lowered floor of" : "raised floor of", &list);

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

	BA_MessageForSel(diff < 0 ? "lowered ceil of" : "raised ceil of", &list);

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


void SectorsAdjustLight(int delta)
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

	BA_MessageForSel(delta < 0 ? "darkened" : "brightened", &list);

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

	SectorsAdjustLight(diff);
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

	BA_MessageForSel("swapped flats in", &list);

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

	BA_MessageForSel("merged", edit.Selected);

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
	Selection_Clear(true);

	edit.Selected->set(source);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
