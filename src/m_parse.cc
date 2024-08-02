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

#include "m_parse.h"

#include "Errors.h"
#include "m_strings.h"
#include "sys_debug.h"

//
// returns number of tokens, zero for comment, negative on error
//
int M_ParseLine(const SString &cline, std::vector<SString> &tokens,
				ParseOptions options)
{
	// when do_strings == 2, string tokens keep their quotes.

	SString tokenbuf;
	tokenbuf.reserve(256);
	bool nexttoken = true;

	bool in_string = false;

	tokens.clear();

	// skip leading whitespace
	SString line = cline;
	line.trimLeadingSpaces();
	line.trimTrailingSpaces();

	// blank line or comment line?
	if (line.empty() || line[0] == '#')
		return 0;

	for(size_t i = 0; i <= line.size(); ++i)
	{
		// This will also cover the null terminator
		char ch = line.c_str()[i];
		if (nexttoken)  // looking for a new token
		{
			SYS_ASSERT(!in_string);

			// end of line?  if yes, break out of for loop
			if (ch == 0 || ch == '\n')
				break;

			if (isspace(ch))
				continue;

			nexttoken = false;

			// begin a string?
			if (ch == '"' && options != ParseOptions::noStrings)
			{
				in_string = true;

				if (options != ParseOptions::haveStringsKeepQuotes)
					continue;
			}
			// begin a normal token
			tokenbuf.push_back(ch);
			continue;
		}

		if (in_string && ch == '"')
		{
			// end of string
			in_string = false;

			if (options == ParseOptions::haveStringsKeepQuotes)
				tokenbuf.push_back(ch);
		}
		else if (ch == 0 || ch == '\n')
		{
			// end of line
		}
		else if (! in_string && isspace(ch))
		{
			// end of token
		}
		else
		{
			tokenbuf.push_back(ch);
			continue;
		}

		if (in_string)  // ERROR: non-terminated string
			return ParseLine_stringError;

		nexttoken = true;

		tokens.push_back(tokenbuf);
		tokenbuf.clear();

		// end of line?  if yes, we are done
		if (ch == 0 || ch == '\n')
			break;
	}

	return static_cast<int>(tokens.size());
}

//
// Get the next word from a token list
//
bool TokenWordParse::getNext(SString &word)
{
	SString item;
	for(; mPos < (int)mLine.length(); ++mPos)
	{
		char c = mLine[mPos];
		switch(mState)
		{
		case State::open:
			if(isspace(c))
				continue;
			if(c == '"')
			{
				mState = State::multiWord;
				continue;
			}
			if(mHasComments && c == '#')
			{
				mState = State::comment;
				return false;
			}
			mState = State::singleWord;
			item.push_back(c);
			continue;
		case State::singleWord:
			if(isspace(c))
			{
				mState = State::open;
				++mPos;
				word = std::move(item);
				return true;
			}
			if(c == '"')
			{
				mState = State::multiWord;
				++mPos;
				word = std::move(item);
				return true;
			}
			if(mHasComments && c == '#')
			{
				mState = State::comment;
				word = std::move(item);
				return true;
			}
			item.push_back(c);
			continue;
		case State::multiWord:
			if(c == '"')
			{
				mState = State::firstQuoteInString;
				continue;
			}
			item.push_back(c);
			continue;
		case State::firstQuoteInString:
			if(c == '"')
			{
				item.push_back('"');
				mState = State::multiWord;
				continue;
			}
			if(mHasComments && c == '#')
			{
				mState = State::comment;
				word = std::move(item);
				return true;
			}
			
			mState = State::open;
			word = std::move(item);
			return true;
		case State::comment:
			return false;
		}
	}

	// Now past the end
	switch(mState)
	{
	case State::open:
	case State::comment:
		break;
	case State::singleWord:
	case State::multiWord:
	case State::firstQuoteInString:
		word = std::move(item);
		mState = State::open;
		return true;
	}

	return false;
}

bool TokenWordParse::getNext(fs::path &path)
{
	SString word;
	bool result = getNext(word);
	if(result)
		path = fs::u8path(word.get());
	return result;
}
