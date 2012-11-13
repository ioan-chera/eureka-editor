//------------------------------------------------------------------------
//  SELECTION LISTS
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

#include "m_bitvec.h"
#include "m_select.h"
#include "editloop.h"
#include "levels.h"
#include "selectn.h"
#include "x_hover.h"


static bool invalidated_selection;


/*
 *  DumpSelection - list the contents of a selection
 *
 */
void DumpSelection (selection_c * list)
{
	SYS_ASSERT(list);

	printf("Selection:");

	selection_iterator_c it;

	for (list->begin(&it); ! it.at_end(); ++it)
		printf(" %d", *it);

	printf("\n");
}


void ConvertSelection(selection_c * src, selection_c * dest)
{
	if (src->what_type() == dest->what_type())
	{
		dest->merge(*src);
		return;
	}


	if (src->what_type() == OBJ_SECTORS && dest->what_type() == OBJ_THINGS)
	{
		// FIXME: get bbox of selection, skip things outside it

		for (int t = 0 ; t < NumThings ; t++)
		{
			const Thing *T = Things[t];

			// if (! thing_touches_bbox(T->x, T->y, 128, bbox))
			//    continue;

			Objid obj;  GetCurObject(obj, OBJ_SECTORS, T->x, T->y);

			if (! obj.is_nil() && src->get(obj.num))
			{
				dest->set(t);
			}
		}
		return;
	}


	if (src->what_type() == OBJ_SECTORS && dest->what_type() == OBJ_LINEDEFS)
	{
		for (int l = 0 ; l < NumLineDefs ; l++)
		{
			const LineDef *L = LineDefs[l];

			if ( (L->Right() && src->get(L->Right()->sector)) ||
				 (L->Left()  && src->get(L->Left()->sector)) )
			{
				dest->set(l);
			}
		}
		return;
	}


	if (src->what_type() == OBJ_SECTORS && dest->what_type() == OBJ_VERTICES)
	{
		for (int l = 0 ; l < NumLineDefs ; l++)
		{
			const LineDef *L = LineDefs[l];

			if ( (L->Right() && src->get(L->Right()->sector)) ||
				 (L->Left()  && src->get(L->Left()->sector)) )
			{
				dest->set(L->start);
				dest->set(L->end);
			}
		}
		return;
	}


	if (src->what_type() == OBJ_LINEDEFS && dest->what_type() == OBJ_SIDEDEFS)
	{
		selection_iterator_c it;

		for (src->begin(&it); ! it.at_end(); ++it)
		{
			const LineDef *L = LineDefs[*it];

			if (L->Right()) dest->set(L->right);
			if (L->Left())  dest->set(L->left);
		}
		return;
	}

	if (src->what_type() == OBJ_SECTORS && dest->what_type() == OBJ_SIDEDEFS)
	{
		for (int n = 0 ; n < NumSideDefs ; n++)
		{
			const SideDef * SD = SideDefs[n];

			if (src->get(SD->sector))
				dest->set(n);
		}
		return;
	}


	if (src->what_type() == OBJ_LINEDEFS && dest->what_type() == OBJ_VERTICES)
	{
		selection_iterator_c it;

		for (src->begin(&it); ! it.at_end(); ++it)
		{
			const LineDef *L = LineDefs[*it];

			dest->set(L->start);
			dest->set(L->end);
		}
		return;
	}


	if (src->what_type() == OBJ_VERTICES && dest->what_type() == OBJ_LINEDEFS)
	{
		// select all linedefs that have both ends selected
		for (int l = 0 ; l < NumLineDefs ; l++)
		{
			const LineDef *L = LineDefs[l];

			if (src->get(L->start) && src->get(L->end))
			{
				dest->set(l);
			}
		}
	}


	// remaining conversions are L->S and V->S

	if (dest->what_type() != OBJ_SECTORS)
		return;
	
	if (src->what_type() != OBJ_LINEDEFS && src->what_type() != OBJ_VERTICES)
		return;


	// step 1: select all sectors (except empty ones)
	int l;

	for (l = 0 ; l < NumLineDefs ; l++)
	{
		const LineDef *L = LineDefs[l];

		if (L->Right()) dest->set(L->Right()->sector);
		if (L->Left())  dest->set(L->Left()->sector);
	}

	// step 2: unselect any sectors if a component is not selected

	for (l = 0 ; l < NumLineDefs ; l++)
	{
		const LineDef *L = LineDefs[l];

		if (src->what_type() == OBJ_VERTICES)
		{
			if (src->get(L->start) && src->get(L->end))
				continue;
		}
		else
		{
			if (src->get(l))
				continue;
		}

		if (L->Right()) dest->clear(L->Right()->sector);
		if (L->Left())  dest->clear(L->Left()->sector);
	}
}


void Selection_NotifyBegin()
{
	invalidated_selection = false;
}


void Selection_NotifyInsert(obj_type_e type, int objnum)
{
	if (objnum <= edit.Selected->max_obj())
	{
		invalidated_selection = true;
	}
}


void Selection_NotifyDelete(obj_type_e type, int objnum)
{
	if (objnum <= edit.Selected->max_obj())
	{
		invalidated_selection = true;
	}
}


void Selection_NotifyChange(obj_type_e type, int objnum, int field)
{
	// field changes never affect the current selection
}


void Selection_NotifyEnd()
{
	if (invalidated_selection)
	{
		// this clears AND RESIZES the selection_c object
		edit.Selected->change_type(edit.obj_type);
	}
}


/*
   select all objects inside a given box
*/

void SelectObjectsInBox(selection_c *list, int objtype, int x1, int y1, int x2, int y2)
{
	if (x2 < x1)
	{
		int tmp = x1; x1 = x2; x2 = tmp;
	}

	if (y2 < y1)
	{
		int tmp = y1; y1 = y2; y2 = tmp;
	}

	switch (objtype)
	{
		case OBJ_THINGS:
			for (int n = 0 ; n < NumThings ; n++)
			{
				const Thing *T = Things[n];

				if (x1 <= T->x && T->x <= x2 &&
				    y1 <= T->y && T->y <= y2)
				{
					list->toggle(n);
				}
			}
			break;

		case OBJ_VERTICES:
			for (int n = 0 ; n < NumVertices ; n++)
			{
				const Vertex *V = Vertices[n];

				if (x1 <= V->x && V->x <= x2 &&
				    y1 <= V->y && V->y <= y2)
				{
					list->toggle(n);
				}
			}
			break;

		case OBJ_LINEDEFS:
			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				const LineDef *L = LineDefs[n];

				/* the two ends of the line must be in the box */
				if (x1 <= L->Start()->x && L->Start()->x <= x2 &&
				    y1 <= L->Start()->y && L->Start()->y <= y2 &&
				    x1 <= L->End()->x   && L->End()->x <= x2 &&
				    y1 <= L->End()->y   && L->End()->y <= y2)
				{
					list->toggle(n);
				}
			}
			break;

		case OBJ_SECTORS:
		{
			selection_c  in_sectors(OBJ_SECTORS);
			selection_c out_sectors(OBJ_SECTORS);

			for (int n = 0 ; n < NumLineDefs ; n++)
			{
				const LineDef *L = LineDefs[n];

				// Get the numbers of the sectors on both sides of the linedef
				int s1 = L->Right() ? L->Right()->sector : -1;
				int s2 = L->Left( ) ? L->Left() ->sector : -1;

				if (x1 <= L->Start()->x && L->Start()->x <= x2 &&
				    y1 <= L->Start()->y && L->Start()->y <= y2 &&
				    x1 <= L->End()->x   && L->End()->x <= x2 &&
				    y1 <= L->End()->y   && L->End()->y <= y2)
				{
					if (s1 >= 0) in_sectors.set(s1);
					if (s2 >= 0) in_sectors.set(s2);
				}
				else
				{
					if (s1 >= 0) out_sectors.set(s1);
					if (s2 >= 0) out_sectors.set(s2);
				}
			}

			for (int i = 0 ; i < NumSectors ; i++)
				if (in_sectors.get(i) && ! out_sectors.get(i))
					list->toggle(i);

			break;
		}
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
