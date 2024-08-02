//------------------------------------------------------------------------
//  BIT VECTORS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2015 Andrew Apted
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

#include "m_bitvec.h"
#include "sys_debug.h"


bitvec_c::bitvec_c(int n_elements) : num_elem(n_elements)
{
	int total = (num_elem / 8) + 1;
	data.resize(total);
	clear_all();
}

void bitvec_c::resize(int n_elements)
{
	SYS_ASSERT(n_elements > 0);

	int old_elem  = num_elem;
	int new_total = (n_elements / 8) + 1;

	// don't bother re-allocating unless shrinking by a large amount
	if (num_elem / 2 < n_elements && n_elements < num_elem)
	{
		num_elem = n_elements;
		return;
	}

	num_elem = n_elements;
	data.resize(new_total);

	// make sure the bits near the old top are clear
	for (int i = 0 ; i < 8 ; i++)
	{
		if (old_elem + i < num_elem)
			raw_clear(old_elem + i);
	}
}


bool bitvec_c::get(int n) const noexcept
{
	SYS_ASSERT(n >= 0);

	if (n >= num_elem)
		return 0;

	return raw_get(n);
}


void bitvec_c::set(int n)
{
	SYS_ASSERT(n >= 0);

	while (n >= num_elem)
	{
		resize(num_elem * 3 / 2 + 16);
	}

	raw_set(n);
}


void bitvec_c::clear(int n) noexcept
{
	SYS_ASSERT(n >= 0);

	if (n >= num_elem)
		return;

	raw_clear(n);
}


void bitvec_c::toggle(int n)
{
	if (get(n))
		clear(n);
	else
		set(n);
}


void bitvec_c::frob(int n, BitOp op)
{
	switch (op)
	{
		case BitOp::add: set(n); break;
		case BitOp::remove: clear(n); break;
		default: toggle(n); break;
	}
}


void bitvec_c::set_all()
{
	// Note: this will set some extra bits (over num_elem).
	//       the get() functions (etc) make sure to never use them.

	memset(data.data(), 0xff, data.size());
}


void bitvec_c::clear_all()
{
	memset(data.data(), 0, data.size());
}


void bitvec_c::toggle_all()
{
	for(byte &b : data)
		b ^= 0xff;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
