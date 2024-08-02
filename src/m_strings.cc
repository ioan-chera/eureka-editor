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

#include "Errors.h"
#include "m_strings.h"
#include "sys_debug.h"
#include <assert.h>
#include <stdarg.h>
#ifdef _WIN32
#include <Windows.h>
#endif

//------------------------------------------------------------------------

//
// a case-insensitive strcmp()
//
int y_stricmp(const char *s1, const char *s2) noexcept
{
	for (;;)
	{
		if (tolower(*s1) != tolower(*s2))
			return (int)(unsigned char)(tolower(*s1)) - (int)(unsigned char)(tolower(*s2));

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
int y_strnicmp(const char *s1, const char *s2, size_t len) noexcept
{
	SYS_ASSERT(len != 0);

	while (len-- > 0)
	{
		if (tolower(*s1) != tolower(*s2))
			return (int)(unsigned char)(tolower(*s1)) - (int)(unsigned char)(tolower(*s2));

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
		*str = static_cast<char>(toupper(*str));
	}
}


//
// lower-case a string (in situ)
//
void y_strlowr(char *str)
{
	for ( ; *str ; str++)
	{
		*str = static_cast<char>(tolower(*str));
	}
}


char *StringNew(int length)
{
	// length does not include the trailing NUL.
	SYS_ASSERT(length >= 0);

	char *s = (char *) calloc(length + 1, 1);

	if (! s)
		ThrowException("Out of memory (%d bytes for string)\n", length);

	return s;
}


char *StringDup(const char *orig, int limit)
{
	if (! orig)
		return nullptr;

	if (limit < 0)
	{
		auto s = static_cast<char *>(malloc(strlen(orig) + 1));

		if (! s)
			ThrowException("Out of memory (copy string)\n");

		strcpy(s, orig);

		return s;
	}

	char * s = StringNew(limit+1);
	strncpy(s, orig, limit);
	s[limit] = 0;

	return s;
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
// Produces a string by joining multiple substrings by a separator
//
SString joined(const char *sep, const char **strlist, int listlen)
{
	assert(sep && strlist);
	SString result;
	for(int i = 0; i < listlen; ++i)
	{
		if(i >= 1)
			result += sep;
		assert(strlist[i]);
		result += strlist[i];
	}
	return result;
}

//
// Make a SString from printf format
//
SString SString::printf(EUR_FORMAT_STRING(const char *format), ...)
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

		va_start(args, format);
		out_len = vsnprintf(buf, buf_size, format, args);
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
SString SString::vprintf(const char *format, va_list ap)
{
	// Algorithm: keep doubling the allocated buffer size
	// until the output fits. Based on code by Darren Salt.

	std::vector<char> buf;
	int buf_size = 128;

	for (;;)
	{
		int out_len;

		buf_size *= 2;

		buf.resize(buf_size);

		out_len = vsnprintf(buf.data(), buf_size, format, ap);

		// old versions of vsnprintf() simply return -1 when
		// the output doesn't fit.
		if (out_len < 0 || out_len >= buf_size)
			continue;

		SString result(buf.data());
		return result;
	}
}

//
// Checks if a string ends with, no-case
//
bool SString::noCaseEndsWith(const SString &text) const
{
	if(text.empty())
		return true;
	size_t textLength = text.length();
	if(length() < textLength)
		return false;
	return !y_stricmp(c_str() + length() - textLength, text.c_str());
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
	erase(pos, npos);
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
// As title
//
SString SString::asTitle() const
{
	SString result(*this);
	if(result.good())
		result[0] = static_cast<char>(toupper(result[0]));
	return result;
}

//
// Lower case
//
SString SString::asLower() const
{
	SString result(*this);
	for(char &c : result)
		c = static_cast<char>(tolower(c));
	return result;
}

//
// Upper case
//
SString SString::asUpper() const
{
	SString result(*this);
	for(char &c : result)
		c = static_cast<char>(toupper(c));
	return result;
}

//
// Get the version with just the printable characters and without the included bad characters.
//
SString SString::getTidy(const char *badChars) const
{
	SString buf;
	buf.reserve(length() + 2);

	for(const char &c : *this)
		if(isprint(c) && (!badChars || !strchr(badChars, c)))
			buf.push_back(c);

	return buf;
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

//
// Add a text
//
StringID StringTable::add(const SString &text)
{
	int index = 0;
	for(const SString &string : mStrings)
	{
		if(string == text)	// this should also cover "" === 0
			return StringID(index);
		++index;
	}
	mStrings.push_back(text);
	return StringID((int)mStrings.size() - 1);
}

//
// Get a text (handle it robustly)
//
SString StringTable::get(StringID offset) const noexcept
{
	// this should never happen
	// [ but handle it gracefully, for the sake of robustness ]
	if(offset.isInvalid() || offset.get() >= (int)mStrings.size())
		return "???ERROR";
	return mStrings[offset.get()];
}

//
// If string has spaces, surrounds it in double quotes. If there are any quotes, it doubles them (MS-DOS convention)
//
SString SString::spaceEscape(bool backslash) const
{
	if(empty())
		return "\"\"";
	bool needsQuotes = false;
	for(char c : data)
		if(isspace(c) || c == '"' || c == '#')
		{
			needsQuotes = true;
			break;
		}

	SString result(*this);
	if(needsQuotes)
	{
		size_t pos = std::string::npos;
		while((pos = result.data.find('"', pos == std::string::npos ? 0 : pos + 2)) != std::string::npos)
			result.data.insert(result.data.begin() + pos, backslash ? '\\' : '"');
		return "\"" + result + "\"";
	}
	return result;
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

//
// Fail safe so we avoid failures
//
static std::wstring FailSafeUTF8ToWide(const char* text)
{
	size_t len = strlen(text);
	std::wstring result;
	result.reserve(len);
	for (size_t i = 0; i < len; ++i)
	{
		result.push_back(static_cast<wchar_t>(text[i]));
	}
	return result;
}

//
// Converts UTF8 characters to wide. Mainly for Windows
//
std::wstring UTF8ToWide(const char* text)
{
	wchar_t* buffer;
	int n = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
	if (!n)
	{
		return FailSafeUTF8ToWide(text);
	}
	buffer = new wchar_t[n];
	n = MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer, n);
	if (!n)
	{
		delete[] buffer;
		return FailSafeUTF8ToWide(text);
	}
	std::wstring result = buffer;
	delete[] buffer;
	return result;
}
#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
