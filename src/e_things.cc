//------------------------------------------------------------------------
//  THING OPERATIONS
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

#include "m_game.h"
#include "e_things.h"
#include "editloop.h"
#include "levels.h"
#include "selectn.h"
#include "m_bitvec.h"
#include "w_rawdef.h"

#include "ui_window.h"


/*
 *  GetAngleName
 *  Get the name of the angle
 */
const char *GetAngleName (int angle)
{
	static char buf[30];

	switch (angle)
	{
		case 0:
			return "East";
		case 45:
			return "North-east";
		case 90:
			return "North";
		case 135:
			return "North-west";
		case 180:
			return "West";
		case 225:
			return "South-west";
		case 270:
			return "South";
		case 315:
			return "South-east";
	}
	sprintf (buf, "ILLEGAL (%d)", angle);
	return buf;
}


/*
 *  GetWhenName
 *  get string of when something will appear
 */
const char *GetWhenName (int when)
{
	static char buf[16+3+1];
	// "N" is a Boom extension ("not in deathmatch")
	// "C" is a Boom extension ("not in cooperative")
	// "F" is an MBF extension ("friendly")
	const char *flag_chars = "????" "????" "FCNM" "D431";
	int n;

	char *b = buf;
	for (n = 0; n < 16; n++)
	{
		if (n != 0 && n % 4 == 0)
			*b++ = ' ';
		if (when & (0x8000u >> n))
			*b++ = flag_chars[n];
		else
			*b++ = '-';
	}
	*b = '\0';
	return buf;

#if 0
static char buf[30];
char *ptr = buf;
*ptr = '\0';
if (when & 0x01)
   {
   strcpy (ptr, "D12");
   ptr += 3;
   }
if (when & 0x02)
   {
   if (ptr != buf)
      *ptr++ = ' ';
   strcpy (ptr, "D3");
   ptr += 2;
   }
if (when & 0x04)
   {
   if (ptr != buf)
      *ptr++ = ' ';
   strcpy (ptr, "D45");
   ptr += 3;
   }
if (when & 0x08)
   {
   if (ptr != buf)
      *ptr++ = ' ';
   strcpy (ptr, "Deaf");
   ptr += 4;
   }
if (when & 0x10)
   {
   if (ptr != buf)
      *ptr++ = ' ';
   strcpy (ptr, "Multi");
   ptr += 5;
   }
return buf;
#endif
}


int calc_new_angle(int angle, int diff)
{
	angle += diff;

	while (angle < 0)
		angle += 360000000;

	return angle % 360;
}


/*
 *  spin_thing - change the angle of things
 */
void TH_SpinThings(void)
{
	int degrees = atoi(EXEC_Param[0]);

	if (! degrees)
		degrees = +45;

	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No things to spin");
		return;
	}

	BA_Begin();

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing *T = Things[*it];

		BA_ChangeTH(*it, Thing::F_ANGLE, calc_new_angle(T->angle, degrees));
	}

	BA_End();

	main_win->thing_box->UpdateField(Thing::F_ANGLE);
}


#if 0  // FIXME  frob_things_flags
/*
 *  frob_things_flags - set/reset/toggle things flags
 *
 *  For all the things in <list>, apply the operator <op>
 *  with the operand <operand> on the flags field.
 */
void frob_things_flags (SelPtr list, int op, int operand)
{
	SelPtr cur;
	s16_t mask;

	if (op == BOP_REMOVE || op == BOP_ADD || op == BOP_TOGGLE)
		mask = 1 << operand;
	else
		mask = operand;

	for (cur = list; cur; cur = cur->next)
	{
		if (op == BOP_REMOVE)
			Things[cur->objnum]->options &= ~mask;
		else if (op == BOP_ADD)
			Things[cur->objnum]->options |= mask;
		else if (op == BOP_TOGGLE)
			Things[cur->objnum]->options ^= mask;
		else
		{
			BugError("frob_things_flags: op=%02X", op);
			return;
		}
	}
	MarkChanges();
}
#endif


static void CollectOverlappingThings(selection_c& list)
{
	selection_c newbies(OBJ_THINGS);

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing *T1 = Things[*it];
		int r1 = M_GetThingType(T1->type)->radius;

		for (int k = 0 ; k < NumThings ; k++)
		{
			if (k == *it)
				continue;

			const Thing *T2 = Things[k];
			int r2 = M_GetThingType(T2->type)->radius;

			int dx = abs(T1->x - T2->x);
			int dy = abs(T1->y - T2->y);

			if (MAX(dx, dy) <= r1 + r2)
			{
				newbies.set(k);
			}
		}
	}

	list.merge(newbies);
}


static bool HasOverlappingThings(selection_c& list)
{
	selection_iterator_c it1;
	selection_iterator_c it2;

	for (list.begin(&it1) ; !it1.at_end() ; ++it1)
	{
		const Thing *T1 = Things[*it1];
		int r1 = M_GetThingType(T1->type)->radius;

		for (list.begin(&it2) ; !it2.at_end() ; ++it2)
		{
			if (*it1 >= *it2)
				continue;

			const Thing *T2 = Things[*it2];
			int r2 = M_GetThingType(T2->type)->radius;

			int dx = abs(T1->x - T2->x);
			int dy = abs(T1->y - T2->y);

			if (MAX(dx, dy) <= r1 + r2)
				return true;
		}
	}

	return false;  // nothing overlapping any more
}


static void MoveOverlappingThings(selection_c& list, int mid_x, int mid_y)
{
	int total = list.count_obj();
	int n;

	// determine distance to move (find largest thing)
	int dist = 8;

/*
	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		const Thing *T = Things[*it];
		const thingtype_t *info = M_GetThingType(T->type)
		
		dist = MAX(dist, info->radius);
	}
*/

	// move 'em!

	selection_iterator_c it;

	for (n = 0, list.begin(&it) ; !it.at_end() ; ++it, ++n)
	{
		const Thing *T = Things[*it];

		float vec_x, vec_y;

		vec_x = (T->x - mid_x) + ((rand() & 255) - 127) / 127.0;
		vec_y = (T->y - mid_y) + ((rand() & 255) - 127) / 127.0;

		float len = sqrt(vec_x*vec_x + vec_y*vec_y);

		if (len < 0.2) len = 0.2;

		float dist = 2 + (rand() & 255) / 60.0;

		int new_x = T->x + vec_x * dist / len;
		int new_y = T->y + vec_y * dist / len;

		BA_ChangeTH(*it, Thing::F_X, new_x);
		BA_ChangeTH(*it, Thing::F_Y, new_y);
	}
}


void TH_Disconnect(void)
{
	if (edit.Selected->empty())
	{
		if (! edit.highlighted())
		{
			Beep("No vertices to disconnect");
			return;
		}

		edit.Selected->set(edit.highlighted.num);
	}

	BA_Begin();

	while (! edit.Selected->empty())
	{
		int first = edit.Selected->find_first();

		// find all the things which overlap this one
		selection_c overlaps(OBJ_THINGS);

		overlaps.set(first);

		for (int pass = 0 ; pass < 3 ; pass++)
			CollectOverlappingThings(overlaps);

		// remove these from the selection
		edit.Selected->unmerge(overlaps);

		if (overlaps.count_obj() < 2)
			continue;

		int mid_x, mid_y;

		Objs_CalcMiddle(&overlaps, &mid_x, &mid_y);

		do
		{
			MoveOverlappingThings(overlaps, mid_x, mid_y);
		
		} while (HasOverlappingThings(overlaps));
	}

	BA_End();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
