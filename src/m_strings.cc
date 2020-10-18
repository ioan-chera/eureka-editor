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

#include "main.h"

#include "m_strings.h"

//------------------------------------------------------------------------

//
// Add a text
//
int StringTable::add(const SString &text)
{
	int index = 0;
	for(const SString &string : mStrings)
	{
		if(string == text)	// this should also cover "" === 0
			return index;
		++index;
	}
	mStrings.push_back(text);
	return (int)mStrings.size() - 1;
}

//
// Get a text (handle it robustly)
//
SString StringTable::get(int offset) const
{
	// this should never happen
	// [ but handle it gracefully, for the sake of robustness ]
	if(offset < 0 || offset >= (int)mStrings.size())
		return "???ERROR";
	return mStrings[offset];
}

#ifdef _WIN32
//
// Fail safe so we avoid failures
//
static SString FailSafeWideToUTF8(const wchar_t *text)
{
	size_t len = wcslen(text);
	SString result;
	result.reserve(len);
	for(size_t i = 0; i < len; ++i)
	{
		result.push_back(static_cast<char>(text[i]));
	}
	return result;
}

//
// Converts wide characters to UTF8. Mainly for Windows
//
SString WideToUTF8(const wchar_t *text)
{
	char *buffer;
	int n = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
	if(!n)
	{
		return FailSafeWideToUTF8(text);
	}
	buffer = new char[n];
	n = WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer, n, nullptr, nullptr);
	if(!n)
	{
		delete[] buffer;
		return FailSafeWideToUTF8(text);
	}
	SString result = buffer;
	delete[] buffer;
	return result;
}
#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
