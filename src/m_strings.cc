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

#include "Errors.hpp"
#include "m_strings.h"
#include "sys_debug.h"

//------------------------------------------------------------------------

//
// a case-insensitive strcmp()
//
int y_stricmp(const char *s1, const char *s2)
{
	for (;;)
	{
		if (tolower(*s1) != tolower(*s2))
			return (int)(unsigned char)(*s1) - (int)(unsigned char)(*s2);

		if (*s1 && *s2)
		{
			s1++;
			s2++;
			continue;
		}

		// both *s1 and *s2 must be zero
		return 0;
	}
}

//
// a case-insensitive strncmp()
//
int y_strnicmp(const char *s1, const char *s2, size_t len)
{
	SYS_ASSERT(len != 0);

	while (len-- > 0)
	{
		if (tolower(*s1) != tolower(*s2))
			return (int)(unsigned char)(*s1) - (int)(unsigned char)(*s2);

		if (*s1 && *s2)
		{
			s1++;
			s2++;
			continue;
		}

		// both *s1 and *s2 must be zero
		return 0;
	}

	return 0;
}

//
// upper-case a string (in situ)
//
void y_strupr(char *str)
{
	for ( ; *str ; str++)
	{
		*str = toupper(*str);
	}
}


//
// lower-case a string (in situ)
//
void y_strlowr(char *str)
{
	for ( ; *str ; str++)
	{
		*str = tolower(*str);
	}
}


char *StringNew(int length)
{
	// length does not include the trailing NUL.

	char *s = (char *) calloc(length + 1, 1);

	if (! s)
		ThrowException("Out of memory (%d bytes for string)\n", length);

	return s;
}


char *StringDup(const char *orig, int limit)
{
	if (! orig)
		return NULL;

	if (limit < 0)
	{
		char *s = strdup(orig);

		if (! s)
			ThrowException("Out of memory (copy string)\n");

		return s;
	}

	char * s = StringNew(limit+1);
	strncpy(s, orig, limit);
	s[limit] = 0;

	return s;
}


SString StringUpper(const SString &name)
{
	SString copy(name);
	for(char &c : copy)
		c = toupper(c);
	return copy;
}


SString StringLower(const SString &name)
{
	SString copy(name);
	for(char &c : copy)
		c = tolower(c);
	return copy;
}

//
// Non-leaking version
//
SString StringPrintf(EUR_FORMAT_STRING(const char *str), ...)
{
	// Algorithm: keep doubling the allocated buffer size
	// until the output fits. Based on code by Darren Salt.

	char *buf = NULL;
	int buf_size = 128;

	for (;;)
	{
		va_list args;
		int out_len;

		buf_size *= 2;

		buf = (char*)realloc(buf, buf_size);
		if (!buf)
			ThrowException("Out of memory (formatting string)\n");

		va_start(args, str);
		out_len = vsnprintf(buf, buf_size, str, args);
		va_end(args);

		// old versions of vsnprintf() simply return -1 when
		// the output doesn't fit.
		if (out_len < 0 || out_len >= buf_size)
			continue;

		SString result(buf);
		free(buf);
		return result;
	}
}
SString StringVPrintf(const char *str, va_list ap)
{
	// Algorithm: keep doubling the allocated buffer size
	// until the output fits. Based on code by Darren Salt.

	char *buf = NULL;
	int buf_size = 128;

	for (;;)
	{
		int out_len;

		buf_size *= 2;

		buf = (char*)realloc(buf, buf_size);
		if (!buf)
			ThrowException("Out of memory (formatting string)\n");

		out_len = vsnprintf(buf, buf_size, str, ap);

		// old versions of vsnprintf() simply return -1 when
		// the output doesn't fit.
		if (out_len < 0 || out_len >= buf_size)
			continue;

		SString result(buf);
		free(buf);
		return result;
	}

}

//
// Safe, cross-platform version of strncpy
//
void StringCopy(char *buffer, size_t size, const SString &source)
{
	if(!size)
		return;	// trivial
	strncpy(buffer, source.c_str(), size);
	buffer[size - 1] = 0;
}

//
// Constructs a string from a raw buffer with limited length
// Unlike std::string, this one trims the string at any potential NUL
//
SString::SString(const char *buffer, int length)
{
    if(!buffer || length <= 0)
    {
        data = "";
        return;
    }
    data.reserve(length);
    for(int i = 0; i < length; ++i)
    {
        if(!buffer[i])
            break;
        data.push_back(buffer[i]);
    }
}

//
// Removes the endline character or sequence from the end
//
void SString::removeCRLF()
{
	if(good() && back() == '\n')
		pop_back();
	if(good() && back() == '\r')
		pop_back();
}

//
// Cuts a string at position "pos", removing the respective character too
//
void SString::getCutWithSpace(size_t pos, SString *word0, SString *word1) const
{
	SYS_ASSERT(pos < length());
	if(word0)
		*word0 = SString(data.substr(0, pos));
	if(word1)
		*word1 = SString(data.substr(pos + 1));
}

//
// This one cuts self
//
void SString::cutWithSpace(size_t pos, SString *second)
{
	SYS_ASSERT(pos < length());
	if(second)
		*second = SString(data.substr(pos + 1));
	erase(pos);
}

//
// Trim leading spaces
//
void SString::trimLeadingSpaces()
{
	size_t i = 0;
	for(i = 0; i < length(); ++i)
		if(!isspace(data[i]))
			break;
	if(i)
		erase(0, i);
}

//
// Trim trailing spaces
//
void SString::trimTrailingSpaces()
{
	while(good() && isspace(data.back()))
		data.pop_back();
}

//
// Trim trailing characters from set
//
void SString::trimTrailingSet(const char *set)
{
	while(good() && strchr(set, data.back()))
		data.pop_back();
}

//
// Trim null termination. Common to happen if data was written as with C strings
//
SString &SString::trimNullTermination()
{
	size_t clen = strlen(data.c_str());
	if(clen != length())
		resize(clen);
	return *this;
}

//
// Finds the first space
//
size_t SString::findSpace() const
{
	for(size_t i = 0; i < length(); ++i)
	{
		if(isspace(data[i]))
			return i;
	}
	return std::string::npos;
}

size_t SString::findDigit() const
{
	for(size_t i = 0; i < length(); ++i)
	{
		if(isdigit(data[i]))
			return i;
	}
	return std::string::npos;
}

SString StringTidy(const SString &str, const SString &bad_chars)
{
	SString buf;
	buf.reserve(str.length() + 2);

	for(const char &c : str)
		if(isprint(c) && bad_chars.find(c) == std::string::npos)
			buf.push_back(c);

	return buf;
}

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
