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

#include "m_strings.h"
#include <vector>

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

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

//
// Line of token words. A token word if it has spaces will be surrounded by 
// quotes. Any literal quotes within it will be doubled. A word without spaces
// but with a literal quote will have that quote doubled and the word itself 
// within quotes.
//
class TokenWordParse
{
public:
	TokenWordParse(const SString &line, bool hasComments) : mLine(line), mHasComments(hasComments)
	{
	}

	bool getNext(SString &word);
	bool getNext(fs::path &path);
private:
	enum class State
	{
		open,
		singleWord,
		multiWord,
		firstQuoteInString,
		comment,
	};

	const SString mLine;
	int mPos = 0;
	State mState = State::open;	// state can be left in other states
	bool mHasComments = false;
};
