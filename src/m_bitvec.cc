//------------------------------------------------------------------------
//  BIT VECTORS
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


bitvec_c::bitvec_c(int n_elements) : num_elem(n_elements)
{
	int total = (num_elem / 8) + 1;

	d = new byte[total];

	clear_all();
}

bitvec_c::~bitvec_c()
{
	delete[] d;
}

void bitvec_c::resize(int n_elements)
{
	int old_elem  = num_elem;
	int old_total = (num_elem / 8) + 1;
	int new_total = (n_elements / 8) + 1;

	// don't bother re-allocating when shrinking
	if (n_elements < num_elem)
	{
		num_elem = n_elements;
		return;
	}

	num_elem = n_elements;

	byte * old_d = d;

	d = new byte[new_total];
	memcpy(d, old_d, old_total);

	delete[] old_d;

	if (new_total > old_total)
		memset(d+old_total, 0, new_total - old_total);

	// make sure the bits near the old top are clear
	for (int i = 0; i < 8; i++)
	{
		if (old_elem + i < num_elem)
			clear(old_elem + i);
	}
}

bool bitvec_c::get(int n) const
{
	return (d[n >> 3] & (1 << (n & 7))) ? true : false;
}

void bitvec_c::set(int n)
{
	d[n >> 3] |= (1 << (n & 7));
}

void bitvec_c::clear(int n)
{
	d[n >> 3] &= ~(1 << (n & 7));
}

void bitvec_c::toggle(int n)
{
	d[n >> 3] ^= (1 << (n & 7));
}

void bitvec_c::set_all()
{
	int total = (num_elem / 8) + 1;

	memset(d, 0xFF, total);
}

void bitvec_c::clear_all()
{
	int total = (num_elem / 8) + 1;

	memset(d, 0, total);
}

void bitvec_c::toggle_all()
{
	int total = (num_elem / 8) + 1;

	byte *pos   = d;
	byte *p_end = d + total;

	while (pos < p_end)
		*pos++ ^= 0xFF;
}

void bitvec_c::frob(int n, sel_op_e op)
{
	switch (op)
	{
		case BOP_ADD: set(n); break;
		case BOP_REMOVE: clear(n); break;
		default: toggle(n); break;
	}
}

void bitvec_c::merge(const bitvec_c& other)
{
	SYS_ASSERT(other.size() == size());

	int total = (num_elem / 8) + 1;

	const byte *src   = other.d;
	const byte *s_end = other.d + total;

	byte *dest = d;

	while (src < s_end)
		*dest++ |= *src++;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
