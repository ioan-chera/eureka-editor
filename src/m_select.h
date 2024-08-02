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

#ifndef __EUREKA_M_SELECT_H__
#define __EUREKA_M_SELECT_H__

#include "m_bitvec.h"
#include "objid.h"
#include <vector>
#include "tl/optional.hpp"

class sel_iter_c;


#define MAX_STORE_SEL  24

class selection_c
{
friend class sel_iter_c;

private:
	ObjType type;

	// number of objects in the selection
	int count = 0;

	int objs[MAX_STORE_SEL] = {};

	// use a bit vector when needed, NULL otherwise
	tl::optional<bitvec_c> bv;

	// an extended mode selection can access 8-bits per object
	tl::optional<std::vector<byte>> extended;

	// the highest object in the selection, or -1
	int maxobj = -1;

	// the very first object selected, or -1.
	// NOTE: this is only updated on a set() when selection is empty.
	int first_obj = -1;

public:
	 selection_c(ObjType type = ObjType::things, bool extended = false);
	selection_c(const selection_c &other) = default;
	selection_c(selection_c &&other) = default;
	selection_c &operator = (selection_c &&other) = default;

	ObjType what_type() const { return type; }

	// this also clears the selection
	void change_type(ObjType new_type) noexcept;

	void clear_all() noexcept;

	bool empty() const noexcept;
	bool notempty() const noexcept { return ! empty(); }

	// return the number of selected objects
	int count_obj() const;

	// return the highest selected object, or -1 if none
	int max_obj() const
	{
		return maxobj;
	}

	bool get(int n) const noexcept;

	void set(int n);
	void clear(int n) noexcept;
	void toggle(int n);

	// in extended mode, we can read and write 8-bits per object.
	// using set_ext() with zero value is equivalent to a clear().
	// NOTE: using these on normal selections is not guaranteed
	// to do anything useful.
	byte get_ext(int n) const noexcept;
	void set_ext(int n, byte value);

	void frob(int n, BitOp op);
	void frob_range(int n1, int n2, BitOp op);

	// set all the objects from the other selection
	void merge(const selection_c& other);

	// clear all the objects from the other selection
	void unmerge(const selection_c& other);

	// only keep values that are in both selections
	void intersect(const selection_c& other);

	bool test_equal(const selection_c& other);

	// these return -1 if there is no first or second
	int find_first()  const;
	int find_second() const;

	std::vector<int> asArray() const;

private:
	void ConvertToBitvec();
	void RecomputeMaxObj() noexcept;
	void ResizeExtended(int new_size);
};


class sel_iter_c
{
private:
	const selection_c *sel = nullptr;

	// this is position in the objs[] array when there is no
	// bit vector, otherwise it is the object number itself
	// (and the corresponding bit will be one).
	int pos = -777777;	// dummy values -- cannot use a bare iterator

public:

	// creates an iterator object for iterating over all the
	// object numbers contained in the given selection.
	// NOTE: modifying the selection is NOT ALLOWED during a traversal.
	explicit sel_iter_c(const selection_c *_sel);
	explicit sel_iter_c(const selection_c& _sel);

	bool done() const;
	void next();

	int operator* () const;
};


#endif  /* __EUREKA_M_SELECT_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
