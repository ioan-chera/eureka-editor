//------------------------------------------------------------------------
//  SELECTION SET
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
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

#include "m_select.h"
#include "objects.h"


//#define NEED_SLOW_CLEAR

#define INITIAL_BITVEC_SIZE  1024


selection_c::selection_c(obj_type_e _type) :
	type(_type),
	count(0), bv(NULL), b_count(0),
	maxobj(-1), first_obj(-1)
{ }

selection_c::~selection_c()
{
	if (bv)
		delete bv;
}


void selection_c::change_type(obj_type_e new_type)
{
	type = new_type;

	clear_all();
}


bool selection_c::empty() const
{
	if (bv)
		return b_count == 0;
	
	return count == 0;
}


int selection_c::count_obj() const
{
	if (! bv)
		return count;
	
	return b_count;  // hmmm, not so slow after all
}


bool selection_c::get(int n) const
{
	if (bv)
	{
		if (n >= bv->size())
			return false;

		return bv->get(n);
	}

	for (int i = 0 ; i < count ; i++)
		if (objs[i] == n)
			return true;

	return false;
}


void selection_c::set(int n)
{
	if (get(n))
		return;

	if (maxobj < n)
		maxobj = n;

	if (first_obj < 0 && empty())
		first_obj = n;

	if (!bv && count >= MAX_STORE_SEL)
	{
		ConvertToBitvec();
	}

	if (bv)
	{
		// the bitvector is too small, grow it
		while (n >= bv->size())
		{
			bv->resize(bv->size() * 2);
		}

		bv->set(n);
		b_count++;
		return;
	}

	objs[count++] = n;
}


void selection_c::clear(int n)
{
	if (bv)
	{
		if (n >= bv->size())
			return;

		if (! get(n))
			return;

		bv->clear(n);
		b_count--;
	}
	else
	{
		int i;

		for (i = 0 ; i < count ; i++)
			if (objs[i] == n)
				break;
		
		if (i >= count)
			return;  // not present

		count--;

#ifdef NEED_SLOW_CLEAR
		for ( ; i < count ; i++)
			objs[i] = objs[i+1];
#else
		if (i < count)
			objs[i] = objs[count];
#endif
	}

	if (maxobj == n)
		RecomputeMaxObj();

	if (first_obj == n)
		first_obj = -1;
}


void selection_c::toggle(int n)
{
	if (get(n))
		clear(n);
	else
		set(n);
}


void selection_c::clear_all()
{
	count = 0;
	maxobj = -1;
	first_obj = -1;

	if (bv)
	{
		delete bv;
	}

	bv = NULL;
	b_count = 0;
}


void selection_c::frob(int n, sel_op_e op)
{
	switch (op)
	{
		case BOP_ADD: set(n); break;
		case BOP_REMOVE: clear(n); break;
		default: toggle(n); break;
	}
}


void selection_c::frob_range(int n1, int n2, sel_op_e op)
{
	for ( ; n1 <= n2 ; n1++)
	{
		frob(n1, op);
	}
}


void selection_c::merge(const selection_c& other)
{
	if (! other.bv)
	{
		for (int i = 0 ; i < other.count ; i++)
			set(other.objs[i]);
		return;
	}

	if (! bv)
		ConvertToBitvec();

	for (int i = 0 ; i < other.bv->size() ; i++)
		if (other.bv->get(i))
			set(i);
}


void selection_c::unmerge(const selection_c& other)
{
	if (! other.bv)
	{
		for (int i = 0 ; i < other.count ; i++)
			clear(other.objs[i]);
	}
	else
	{
		for (int i = 0 ; i < other.bv->size() ; i++)
			if (other.bv->get(i))
				clear(i);
	}
}


void selection_c::intersect(const selection_c& other)
{
	if (! bv)
		ConvertToBitvec();

	int cur_size = bv->size();

	for (int i = 0 ; i < cur_size ; i++)
		if (get(i) != other.get(i))
			clear(i);
}


void selection_c::ConvertToBitvec()
{
	SYS_ASSERT(! bv);

	int size = INITIAL_BITVEC_SIZE;

	if (size < maxobj+1)
		size = maxobj+1;

	bv = new bitvec_c(size);

	for (int i = 0 ; i < count ; i++)
	{
		bv->set(objs[i]);
	}

	b_count = count;
	count = 0;
}


void selection_c::RecomputeMaxObj()
{
	maxobj = -1;

	if (bv)
	{
		// search backwards so we can early out
		for (int i = bv->size()-1 ; i >= 0 ; i--)
		{
			if (bv->get(i))
			{
				maxobj = i;
				break;
			}
		}
	}
	else
	{
		// cannot optimise here, values are not sorted
		for (int i = 0 ; i < count ; i++)
		{
			maxobj = MAX(maxobj, objs[i]);
		}
	}
}


int selection_c::find_first() const
{
	if (first_obj >= 0)
	{
		SYS_ASSERT(get(first_obj));

		return first_obj;
	}

	selection_iterator_c it;
	begin(&it);

	return it.at_end() ? -1 : *it;
}


int selection_c::find_second() const
{
	selection_iterator_c it;
	begin(&it);

	if (it.at_end())
		return -1;

	// the logic here is trickier than it looks.
	//
	// When first_obj exists AND is the same as the very first object
	// in the group, then this test fails and we move onto the next
	// object in the group.
	if (first_obj >= 0 && *it != first_obj)
		return *it;

	++it;

	return it.at_end() ? -1 : *it;
}


//------------------------------------------------------------------------
//  ITERATOR STUFF
//------------------------------------------------------------------------

void selection_c::begin(selection_iterator_c *it) const
{
	SYS_ASSERT(type != OBJ_NONE);

	it->sel = this;
	it->pos = 0;
	
	if (bv)
	{
		// for bit vector, need to find the first one bit
		// FIXME: this is rather hacky
		it->pos = -1;

		++ (*it);
	}
}


bool selection_iterator_c::at_end() const
{
	if (sel->bv)
		return (pos >= sel->bv->size());
	else
		return (pos >= sel->count);
}


int selection_iterator_c::operator* () const
{
	if (sel->bv)
		return pos;
	else
		return sel->objs[pos];
}


selection_iterator_c& selection_iterator_c::operator++ ()
{
	pos++;

	if (sel->bv)
	{
		// FIXME: OPTIMISE THIS
		while (pos < sel->bv->size() && ! sel->bv->get(pos))
			pos++;
	}

	return *this;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
