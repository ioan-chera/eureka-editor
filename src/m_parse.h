//------------------------------------------------------------------------
//  STRING PARSING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2021 Ioan Chera
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

#include <vector>

class SString;

//
// Options for M_ParseLine
//
enum class ParseOptions
{
	noStrings,
	haveStrings,
	haveStringsKeepQuotes,
};

enum
{
	ParseLine_stringError = -3,	// return value in case of unterminated string
};

int M_ParseLine(const SString &cline, std::vector<SString> &tokens,
				ParseOptions options);
