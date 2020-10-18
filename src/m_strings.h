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

#include "lib_util.h"

//
// String storage table
//
class StringTable
{
public:
	int add(const SString &str);
	SString get(int offset) const;
private:
	// Must start with an empty string, so get(0) gets "".
	std::vector<SString> mStrings = { "" };	
};

#ifdef _WIN32
SString WideToUTF8(const wchar_t *text);
#endif

#endif  /* __EUREKA_M_STRINGS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
