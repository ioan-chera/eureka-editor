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
void Instance::CMD_TH_SpinThings()
{
	int degrees = atoi(EXEC_Param[0]);

	if (! degrees)
		degrees = +45;

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("No things to spin");
		return;
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("spun", *edit.Selected);

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			const Thing *T = level.things[*it];

			op.changeThing(*it, Thing::F_ANGLE, calc_new_angle(T->angle, degrees));
		}
	}

	main_win->thing_box->UpdateField(Thing::F_ANGLE);

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


static bool ThingsAtSameLoc(const Document &doc, int th1, int th2)
{
	const Thing *T1 = doc.things[th1];
	const Thing *T2 = doc.things[th2];

	double dx = abs(T1->x() - T2->x());
	double dy = abs(T1->y() - T2->y());

	return (dx < 8 && dy < 8);
}


static void CollectOverlapGroup(const Instance &inst, selection_c& list)
{
	int first = inst.edit.Selected->find_first();

	list.set(first);

	for (int k = 0 ; k < inst.level.numThings() ; k++)
		if (k != first && ThingsAtSameLoc(inst.level, k, first))
			list.set(k);
}


static void MoveOverlapThing(EditOperation &op, Instance &inst, int th, int mid_x, int mid_y, int n, int total)
{
	float angle = static_cast<float>(n * 360 / total);

	float vec_x = static_cast<float>(cos(angle * M_PI / 180.0));
	float vec_y = static_cast<float>(sin(angle * M_PI / 180.0));

	float dist = static_cast<float>(8 + 6 * std::min(100, total));

	fixcoord_t fdx = inst.MakeValidCoord(vec_x * dist);
	fixcoord_t fdy = inst.MakeValidCoord(vec_y * dist);

	const Thing *T = inst.level.things[th];

	op.changeThing(th, Thing::F_X, T->raw_x + fdx);
	op.changeThing(th, Thing::F_Y, T->raw_y + fdy);
}


//
//  all things lying at same location (or very near) to the selected
//  things are moved so they are more distinct -- about 8 units away
//  from that location.
//
void CMD_TH_Disconnect(Instance &inst)
{
	SelectHighlight unselect = inst.edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("No vertices to disconnect");
		return;
	}

	EditOperation op(inst.level.basis);
	op.setMessageForSelection("disconnected", *inst.edit.Selected);

	while (!inst.edit.Selected->empty())
	{
		selection_c overlaps(ObjType::things);

		CollectOverlapGroup(inst, overlaps);

		// remove these from the selection
		inst.edit.Selected->unmerge(overlaps);


		int total = overlaps.count_obj();
		if (total < 2)
			continue;

		v2double_t mid = inst.level.objects.calcMiddle(overlaps);

		int n = 0;
		for (sel_iter_c it(overlaps) ; !it.done() ; it.next(), n++)
		{
			MoveOverlapThing(op, inst, *it, static_cast<int>(mid.x), static_cast<int>(mid.y), n, total);
		}
	}
}


//
// place all selected things at same location
//
void CMD_TH_Merge(Instance &inst)
{
	if (inst.edit.Selected->count_obj() == 1 && inst.edit.highlight.valid())
	{
		inst.edit.Selection_AddHighlighted();
	}

	if (inst.edit.Selected->count_obj() < 2)
	{
		inst.Beep("Need 2 or more things to merge");
		return;
	}

	v2double_t mid = inst.level.objects.calcMiddle(*inst.edit.Selected);

	EditOperation op(inst.level.basis);
	op.setMessageForSelection("merged", *inst.edit.Selected);

	for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
	{
		op.changeThing(*it, Thing::F_X, inst.MakeValidCoord(mid.x));
		op.changeThing(*it, Thing::F_Y, inst.MakeValidCoord(mid.y));
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
