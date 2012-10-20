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

#ifndef __EUREKA_M_BITVEC_H__
#define __EUREKA_M_BITVEC_H__


typedef enum
{
	BOP_ADD = 0,   // Add to selection
	BOP_REMOVE,    // Remove from selection
	BOP_TOGGLE     // If not in selection, add it, else remove it
}
sel_op_e;


class bitvec_c
{
private:
	int num_elem;

	byte *d;

public:
	 bitvec_c(int n_elements);
	~bitvec_c();

	inline int size() const
	{
		return num_elem;
	}

	// this preserves existing elements
	void resize(int n_elements);

	bool get(int n) const;  // Get bit <n>

	void set(int n);    // Set bit <n> to 1
	void clear(int n);  // Set bit <n> to 0
	void toggle(int n); // Toggle bit <n>

	void set_all();
	void clear_all();
	void toggle_all();

	void frob(int n, sel_op_e op);

	void merge(const bitvec_c& other);
};


#endif  /* __EUREKA_M_BITVEC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
