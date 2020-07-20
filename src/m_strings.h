//------------------------------------------------------------------------
//  STRING TABLE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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

#ifndef __EUREKA_M_STRINGS_H__
#define __EUREKA_M_STRINGS_H__

#include <string>
#include "lib_util.h"

class string_block_c;


class string_table_c
{
private:
	std::vector<string_block_c *> blocks;

	std::vector<const char *> huge_ones;

public:
	 string_table_c();
	~string_table_c();

	int add(const char *str);
	// find the given string in the string table, returning the
	// offset if it exists, otherwise adds the string and returns
	// the new offset.  The empty string and NULL are always 0.

	const char * get(int offset);
	// get the string from an offset.  Zero always returns an
	// empty string.  NULL is never returned.

	void clear();
	// remove all strings.

private:
	int find_normal(const char *str, int len);
	// returns an offset value, zero when not found.

	int find_huge(const char *str, int len);
	// returns an offset value, zero when not found.

	int add_normal(const char *str, int len);
	int add_huge(const char *str, int len);
};

#ifdef _WIN32
SString WideToUTF8(const wchar_t *text);
#endif

#endif  /* __EUREKA_M_STRINGS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
