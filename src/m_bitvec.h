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

#ifndef __EUREKA_M_BITVEC_H__
#define __EUREKA_M_BITVEC_H__

#include "sys_type.h"
#include <vector>

enum class BitOp
{
	add,		// Add to selection
	remove,		// Remove from selection
	toggle		// If not in selection, add it, else remove it
};


class bitvec_c
{
	//
	// Although this bit-vector has a current size, it acts as though it
	// was infinitely sized and all bits past the end are zero.  When
	// setting a bit past the end, it will automatically resize itself.
	//

private:
	std::vector<byte> data;
	int num_elem;

public:
	explicit bitvec_c(int n_elements = 64);

	bitvec_c(const bitvec_c &other) = default;
	bitvec_c(bitvec_c &&other) : data(std::move(other.data)), num_elem(other.num_elem)
	{
		other.num_elem = 0;
	}
	bitvec_c &operator = (bitvec_c &&other) = default;

	inline int size() const
	{
		return num_elem;
	}

	bool get(int n) const noexcept;	// Get bit <n>

	void set(int n);		// Set bit <n> to 1
	void clear(int n) noexcept;		// Set bit <n> to 0
	void toggle(int n);		// Toggle bit <n>

	void frob(int n, BitOp op);

	void set_all();
	void clear_all();
	void toggle_all();

private:
	/* NOTE : these functions do no range checking! */

	inline bool raw_get(int n) const noexcept
	{
		return !!(data[n >> 3] & (1 << (n & 7)));
	}

	inline void raw_set(int n)
	{
		data[n >> 3] |= (1 << (n & 7));
	}

	inline void raw_clear(int n) noexcept
	{
		data[n >> 3] &= ~(1 << (n & 7));
	}

	inline void raw_toggle(int n)
	{
		data[n >> 3] ^= (1 << (n & 7));
	}

	inline int getByteCount() const
	{
		return num_elem / 8 + 1;
	}

	// this preserves existing elements
	void resize(int n_elements);
};


#endif  /* __EUREKA_M_BITVEC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
