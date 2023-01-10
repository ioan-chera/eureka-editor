//------------------------------------------------------------------------
//  SELECTION SET
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
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

#include "Errors.h"

#include "sys_debug.h"

#include "m_select.h"


//#define NEED_SLOW_CLEAR

#define INITIAL_BITVEC_SIZE    1024
#define INITIAL_EXTENDED_SIZE  256

selection_c::selection_c(ObjType type, bool extended) : type(type)
{
	if(extended)
	{
		this->extended.emplace(INITIAL_EXTENDED_SIZE);
	}
}

selection_c::~selection_c()
{
}


void selection_c::change_type(ObjType new_type)
{
	type = new_type;

	clear_all();
}


bool selection_c::empty() const
{
	return count == 0;
}


int selection_c::count_obj() const
{
	return count;
}


bool selection_c::get(int n) const
{
	if (extended)
		return get_ext(n) != 0;

	if (bv)
		return bv->get(n);

	for (int i = 0 ; i < count ; i++)
		if (objs[i] == n)
			return true;

	return false;
}


void selection_c::set(int n)
{
	if (extended)
	{
		set_ext(n, 1);
		return;
	}

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
		bv->set(n);
		count++;
		return;
	}

	objs[count++] = n;
}


void selection_c::clear(int n)
{
	if (extended)
	{
		if (get_ext(n) == 0)
			return;

		// n should be safe to access directly, due to above check
		(*extended)[n] = 0;
		count--;
	}
	else if (bv)
	{
		if (! get(n))
			return;

		bv->clear(n);
		count--;
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

	if (extended)
	{
		std::fill(extended->begin(), extended->end(), 0);
	}
	bv.reset();
}


byte selection_c::get_ext(int n) const
{
	if (! extended)
		return get(n) ? 255 : 0;

	if (n >= (int)extended->size())
		return 0;

	return (*extended)[n];
}


void selection_c::set_ext(int n, byte value)
{
	// set_ext() should not be used with plain selections
	if (! extended)
		return;

	if (value == 0)
	{
		clear(n);
		return;
	}

	if (maxobj < n)
		maxobj = n;

	if (first_obj < 0 && empty())
		first_obj = n;

	// need to resize the array?
	while (n >= extended->size())
	{
		ResizeExtended((int)extended->size() * 2);
	}

	if ((*extended)[n] == 0)
		count++;

	(*extended)[n] = value;
}


void selection_c::frob(int n, BitOp op)
{
	switch (op)
	{
		case BitOp::add: set(n); break;
		case BitOp::remove: clear(n); break;
		default: toggle(n); break;
	}
}


void selection_c::frob_range(int n1, int n2, BitOp op)
{
	for ( ; n1 <= n2 ; n1++)
	{
		frob(n1, op);
	}
}


void selection_c::merge(const selection_c& other)
{
	if (extended && other.extended)
	{
		for (int i = 0 ; i <= other.maxobj ; i++)
		{
			byte value = other.get_ext(i);

			set_ext(i, get_ext(i) | value);
		}
	}
	else if (other.bv || other.extended)
	{
		for (int i = 0 ; i <= other.maxobj ; i++)
			if (other.get(i) && !get(i))
				set(i);
	}
	else
	{
		for (int i = 0 ; i < other.count ; i++)
			if (!get(other.objs[i]))
				set(other.objs[i]);
	}
}


void selection_c::unmerge(const selection_c& other)
{
	if (other.bv || other.extended)
	{
		for (int i = 0 ; i <= other.maxobj ; i++)
			if (other.get(i))
				clear(i);
	}
	else
	{
		for (int i = 0 ; i < other.count ; i++)
			clear(other.objs[i]);
	}
}


void selection_c::intersect(const selection_c& other)
{
	for (int i = 0 ; i <= maxobj ; i++)
		if (get(i) && !other.get(i))
			clear(i);
}


bool selection_c::test_equal(const selection_c& other)
{
	if (type != other.type)
		return false;

	if (maxobj != other.maxobj)
		return false;

	if (count_obj() != other.count_obj())
		return false;

	// the quick tests have passed, now perform the expensive one

	for (sel_iter_c it(this) ; !it.done() ; it.next())
		if (! other.get(*it))
			return false;

	return true;
}


void selection_c::ConvertToBitvec()
{
	SYS_ASSERT(! bv);

	int size = INITIAL_BITVEC_SIZE;

	if (size < maxobj+1)
		size = maxobj+1;

	bv.emplace(size);

	for (int i = 0 ; i < count ; i++)
	{
		bv->set(objs[i]);
	}
}


void selection_c::RecomputeMaxObj()
{
	maxobj = -1;

	if (extended)
	{
		// search backwards so we can early out
		for (int i = (int)extended->size() - 1; i >= 0; i--)
		{
			if (get_ext(i))
			{
				maxobj = i;
				break;
			}
		}
	}
	else if (bv)
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
			maxobj = std::max(maxobj, objs[i]);
		}
	}
}


void selection_c::ResizeExtended(int new_size)
{
	SYS_ASSERT(new_size > 0);

	extended->resize(new_size);
}


int selection_c::find_first() const
{
	if (first_obj >= 0)
	{
		SYS_ASSERT(get(first_obj));

		return first_obj;
	}

	sel_iter_c it(this);

	return it.done() ? -1 : *it;
}


int selection_c::find_second() const
{
	sel_iter_c it(this);

	if (it.done())
		return -1;

	// the logic here is trickier than it looks.
	//
	// When first_obj exists AND is the same as the very first object
	// in the group, then this test fails and we move onto the next
	// object in the group.
	if (first_obj >= 0 && *it != first_obj)
		return *it;

	it.next();

	return it.done() ? -1 : *it;
}


//------------------------------------------------------------------------
//  ITERATOR STUFF
//------------------------------------------------------------------------

sel_iter_c::sel_iter_c(const selection_c *_sel) : sel(_sel), pos(0)
{
	if (sel->bv || sel->extended)
	{
		// for bit vector, need to find the first one bit.
		// Note: this logic is slightly hacky...

		pos = -1;
		next();
	}
}


sel_iter_c::sel_iter_c(const selection_c& _sel) : sel(&_sel), pos(0)
{
	if (sel->bv || sel->extended)
	{
		pos = -1;
		next();
	}
}


bool sel_iter_c::done() const
{
	SYS_ASSERT(sel);

	if (sel->extended)
		return (pos >= (int)sel->extended->size());
	else if (sel->bv)
		return (pos >= sel->bv->size());
	else
		return (pos >= sel->count);
}


int sel_iter_c::operator* () const
{
	SYS_ASSERT(sel);

	if (sel->bv || sel->extended)
		return pos;
	else
		return sel->objs[pos];
}


void sel_iter_c::next()
{
	SYS_ASSERT(sel);

	pos++;

	if (sel->extended)
	{
		while (pos < (int)sel->extended->size() && (*sel->extended)[pos] == 0)
			pos++;
	}
	else if (sel->bv)
	{
		// this could be optimised....
		while (pos < sel->bv->size() && !sel->bv->get(pos))
			pos++;
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
