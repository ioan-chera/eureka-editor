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
			
			mState = State::open;
			word = std::move(item);
			return true;
		}
	}

	// Now past the end
	switch(mState)
	{
	case State::open:
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

//
// Standardized way to get words from a line. All tokens are space separated. A token is either a single word or a quoted
// text. A single word can't contain quotes or spaces. A quoted text is always surrounded by quotes and can contain spaces.
// Any literal quotes within it appear doubled.
//
std::vector<SString> getTokenWords(const SString &line)
{
	enum class State
	{
		open,
		singleWord,
		multiWord,
		firstQuoteInString,
	};
	auto state = State::open;
	SString item;
	std::vector<SString> list;
	for(char c : line)
	{
		switch(state)
		{
		case State::open:
			if(isspace(c))
				continue;
			if(c == '"')
			{
				state = State::multiWord;
				continue;
			}
			state = State::singleWord;
			item.push_back(c);
			continue;
		case State::singleWord:
			if(isspace(c))
			{
				list.push_back(item);
				item.clear();
				state = State::open;
				continue;
			}
			if(c == '"')
			{
				list.push_back(item);
				item.clear();
				state = State::multiWord;
				continue;
			}
			item.push_back(c);
			continue;
		case State::multiWord:
			if(c == '"')
			{
				state = State::firstQuoteInString;
				continue;
			}
			item.push_back(c);
			continue;
		case State::firstQuoteInString:
			if(c == '"')
			{
				item.push_back('"');
				state = State::multiWord;
				continue;
			}
			list.push_back(item);
			item.clear();
			if(isspace(c))
			{
				state = State::open;
				continue;
			}
			item.push_back(c);
			state = State::singleWord;
			continue;
		}
	}
	// Now past the end
	switch(state)
	{
	case State::open:
		break;
	case State::singleWord:
	case State::multiWord:
	case State::firstQuoteInString:
		list.push_back(item);
		break;
	}

	return list;
}
