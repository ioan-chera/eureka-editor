//------------------------------------------------------------------------
//  THING OPERATIONS
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

#include "Instance.h"
#include "main.h"

#include "m_game.h"
#include "e_things.h"
#include "e_main.h"
#include "m_bitvec.h"
#include "w_rawdef.h"

#include "ui_window.h"


int calc_new_angle(int angle, int diff)
{
	angle += diff;

	while (angle < 0)
		angle += 360000000;

	return angle % 360;
}


//
// spin_thing - change the angle of things
//
void CMD_TH_SpinThings(Instance &inst)
{
	int degrees = atoi(EXEC_Param[0]);

	if (! degrees)
		degrees = +45;

	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No things to spin");
		return;
	}

	inst.level.basis.begin();
	inst.level.basis.setMessageForSelection("spun", *edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		const Thing *T = inst.level.things[*it];

		inst.level.basis.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, degrees));
	}

	inst.level.basis.end();

	instance::main_win->thing_box->UpdateField(Thing::F_ANGLE);

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


bool ThingsOverlap(int th1, int th2)
{
	const Thing *T1 = gDocument.things[th1];
	const Thing *T2 = gDocument.things[th2];

	int r1 = M_GetThingType(T1->type).radius;
	int r2 = M_GetThingType(T2->type).radius;

	double dx = abs(T1->x() - T2->x());
	double dy = abs(T1->y() - T2->y());

	return (MAX(dx, dy) < r1 + r2);
}


bool ThingsAtSameLoc(int th1, int th2)
{
	const Thing *T1 = gDocument.things[th1];
	const Thing *T2 = gDocument.things[th2];

	double dx = abs(T1->x() - T2->x());
	double dy = abs(T1->y() - T2->y());

	return (dx < 8 && dy < 8);
}


static void CollectOverlapGroup(selection_c& list)
{
	int first = edit.Selected->find_first();

	list.set(first);

	for (int k = 0 ; k < gDocument.numThings() ; k++)
		if (k != first && ThingsAtSameLoc(k, first))
			list.set(k);
}


static void MoveOverlapThing(int th, int mid_x, int mid_y, int n, int total)
{
	float angle = static_cast<float>(n * 360 / total);

	float vec_x = static_cast<float>(cos(angle * M_PI / 180.0));
	float vec_y = static_cast<float>(sin(angle * M_PI / 180.0));

	float dist = static_cast<float>(8 + 6 * MIN(100, total));

	fixcoord_t fdx = MakeValidCoord(vec_x * dist);
	fixcoord_t fdy = MakeValidCoord(vec_y * dist);

	const Thing *T = gDocument.things[th];

	gDocument.basis.changeThing(th, Thing::F_X, T->raw_x + fdx);
	gDocument.basis.changeThing(th, Thing::F_Y, T->raw_y + fdy);
}


//
//  all things lying at same location (or very near) to the selected
//  things are moved so they are more distinct -- about 8 units away
//  from that location.
//
void CMD_TH_Disconnect(void)
{
	SelectHighlight unselect = SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No vertices to disconnect");
		return;
	}

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection("disconnected", *edit.Selected);

	while (! edit.Selected->empty())
	{
		selection_c overlaps(ObjType::things);

		CollectOverlapGroup(overlaps);

		// remove these from the selection
		edit.Selected->unmerge(overlaps);


		int total = overlaps.count_obj();
		if (total < 2)
			continue;

		double mid_x, mid_y;
		gDocument.objects.calcMiddle(&overlaps, &mid_x, &mid_y);

		int n = 0;
		for (sel_iter_c it(overlaps) ; !it.done() ; it.next(), n++)
		{
			MoveOverlapThing(*it, static_cast<int>(mid_x), static_cast<int>(mid_y), n, total);
		}
	}

	gDocument.basis.end();
}


//
// place all selected things at same location
//
void CMD_TH_Merge(void)
{
	if (edit.Selected->count_obj() == 1 && edit.highlight.valid())
	{
		Selection_Add(edit.highlight);
	}

	if (edit.Selected->count_obj() < 2)
	{
		Beep("Need 2 or more things to merge");
		return;
	}

	double mid_x, mid_y;
	gDocument.objects.calcMiddle(edit.Selected, &mid_x, &mid_y);

	gDocument.basis.begin();
	gDocument.basis.setMessageForSelection("merged", *edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		gDocument.basis.changeThing(*it, Thing::F_X, MakeValidCoord(mid_x));
		gDocument.basis.changeThing(*it, Thing::F_Y, MakeValidCoord(mid_y));
	}

	gDocument.basis.end();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
