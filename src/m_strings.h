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

#include "PrintfMacros.h"

#include <string.h>

#include <ostream>
#include <string>
#include <vector>

// Helper to treat nullptr char* the same as ""
#define CSTRING_EMPTY(string) (!(string) || !*(string))

class SString;

int y_stricmp (const char *s1, const char *s2) noexcept;
int y_strnicmp (const char *s1, const char *s2, size_t len) noexcept;

void y_strupr (char *str);
void y_strlowr (char *str);

char *StringNew(int length);
char *StringDup(const char *orig, int limit = -1);
void StringCopy(char *buffer, size_t size, const SString &source);

//
// Safe string: doesn't crash if given NULL (can happen due to refactoring old "char *" code.
// Also enforces null-terminated strings: can't contain NUL inside the string.
//
class SString
{
public:
	SString() = default;

	SString(const char *cstring) : data(cstring ? cstring : "")
	{
	}

	SString(const std::string &cppstring) : data(cppstring)
	{
	}

    SString(const char *buffer, int length);

	explicit SString(char c)
	{
		data.push_back(c);
	}

	SString(std::string &&consume) : data(std::move(consume))
	{
	}

	explicit SString(int number) : data(std::to_string(number))
	{
	}

	static SString printf(EUR_FORMAT_STRING(const char *format), ...) EUR_PRINTF(1, 2);
	static SString vprintf(const char *format, va_list ap);

	bool empty() const
	{
		return data.empty();
	}

	bool noCaseEqual(const SString &c) const noexcept
	{
		return noCaseEqual(c.c_str());
	}

	bool noCaseEqual(const char *c) const noexcept
	{
		return !y_stricmp(data.c_str(), c ? c : "");
	}

	int noCaseCompare(const SString &other) const noexcept
	{
		return y_stricmp(data.c_str(), other.c_str());
	}

	bool noCaseStartsWith(const char *c, int pos = 0) const noexcept
	{
		return !y_strnicmp(data.c_str() + pos, c ? c : "", c ? strlen(c) : 0);
	}

	bool noCaseEndsWith(const SString &text) const;

	size_t findNoCase(const char *c) const
	{
		SString upperme = asUpper();
		SString upperthem = SString(c).asUpper();
		return upperme.find(upperthem);
	}

	bool startsWith(const char *c) const
	{
		return !strncmp(data.c_str(), c ? c : "", c ? strlen(c) : 0);
	}

	bool operator != (const char *c) const
	{
		return data != (c ? c : "");
	}

	bool operator == (const char *c) const
	{
		return data == (c ? c : "");
	}

    bool operator == (const SString &other) const
    {
        return data == other.data;
    }

	bool operator != (const SString &other) const
	{
		return data != other.data;
	}

	bool operator < (const SString &other) const
	{
		return data < other.data;
	}

	char operator[](int n) const
	{
		return n >= 0 ? data[n] : data[data.length() - abs(n)];
	}

	char &operator[](int n)
	{
		return n >= 0 ? data[n] : data[data.length() - abs(n)];
	}

	char operator[](size_t n) const
	{
		return data[n];
	}

	char &operator[](size_t n)
	{
		return data[n];
	}


	char back() const
	{
		return data.back();
	}

	char &back()
	{
		return data.back();
	}

	const char *c_str() const noexcept
	{
		return data.c_str();
	}

	char *ptr() const
	{
		return const_cast<char *>(data.data());
	}

	size_t find(const char *s) const
	{
		return data.find(s ? s : "");
	}

	size_t find(const SString &s) const
	{
		return data.find(s.data);
	}

	size_t find(char c, size_t pos = 0) const
	{
		return data.find(c, pos);
	}

	size_t find_first_of(const char *s, size_t pos = 0) const
	{
		return data.find_first_of(s ? s : "", pos);
	}

	size_t find_first_not_of(const char *s, size_t pos = 0) const
	{
		return data.find_first_not_of(s ? s : "", pos);
	}

	size_t find_last_of(const char *s) const
	{
		return data.find_last_of(s ? s : "");
	}

	size_t length() const noexcept
	{
		return data.length();
	}

	size_t size() const noexcept
	{
		return data.size();
	}

	SString operator + (const char *s) const
	{
		SString result;
		result.data = data + (s ? s : "");
		return result;
	}

	SString operator + (const SString &other) const
	{
		SString result;
		result.data = data + other.data;
		return result;
	}

	SString operator + (char c) const
	{
		SString result;
		result.data = data + c;
		return result;
	}

	SString &operator += (char c)
	{
		if(c)	// disallow NULL addition!
			data += c;
		return *this;
	}

	SString &operator += (const char *c)
	{
		data += (c ? c : "");
		return *this;
	}

	SString &operator += (const SString &other)
	{
		data += other.data;
		return *this;
	}

	SString &insert(size_t pos, const char *s)
	{
		data.insert(pos, s ? s : "");
		return *this;
	}

	SString substr(size_t pos = 0, size_t len = std::string::npos) const
	{
		return data.substr(pos, len);
	}

	void clear() noexcept
	{
		data.clear();
	}

	void erase(size_t n, size_t len)
	{
		data.erase(n, len);
	}

	void pop_back()
	{
		data.pop_back();
	}

	void push_back(char c)
	{
		data.push_back(c);
	}

	void reserve(size_t n)
	{
		data.reserve(n);
	}

	size_t rfind(char c) const
	{
		return data.rfind(c);
	}

	std::string::iterator begin() noexcept
	{
		return data.begin();
	}
	std::string::const_iterator begin() const noexcept
	{
		return data.begin();
	}
	std::string::iterator end() noexcept
	{
		return data.end();
	}
	std::string::const_iterator end() const noexcept
	{
		return data.end();
	}

	//
	// Convenience to avoid double negation
	//
	bool good() const
	{
		return !empty();
	}

	enum
	{
		npos = std::string::npos
	};

	std::string &get()
	{
		return data;
	}
	const std::string &get() const
	{
		return data;
	}

	void getCutWithSpace(size_t pos, SString *word0, SString *word1) const;
	void cutWithSpace(size_t pos, SString *second);

	void trimLeadingSpaces();
	void trimTrailingSpaces();
	void trimTrailingSet(const char *set);

	//
	// Capitalizes first letter
	//
	SString asTitle() const;
	SString asLower() const;
	SString asUpper() const;

	SString getTidy(const char *badChars = nullptr) const;

	size_t findSpace() const;
	size_t findDigit() const;

	SString spaceEscape(bool backslash = false) const;

private:
	std::string data;
};

//
// Convenience stuff
//
inline static SString operator + (const char *cstring, const SString &sstring)
{
	return SString(cstring) + sstring;
}

inline static std::ostream &operator<<(std::ostream &os, const SString &str)
{
	os << str.get();
	return os;
}

SString joined(const char *sep, const char **strlist, int listlen);

//
// Variant of joined which accepts a lambda for each item
//
template<typename T, typename C>
SString joined(const char *sep, const T *items, int listlen, C &&func)
{
	SString result;
	for(int i = 0; i < listlen; ++i)
	{
		if(i >= 1)
			result += sep;
		result += func(items[i]);
	}
	return result;
}

namespace std
{
	template <> struct hash<SString>
	{
		size_t operator()(const SString & x) const
		{
			return hash<std::string>()(x.get());
		}
	};
}

//
// Convenience
//
inline static int atoi(const SString &string)
{
	return atoi(string.c_str());
}
inline static double atof(const SString &string)
{
	return atof(string.c_str());
}
inline static long strtol(const SString &string, char **endptr, int radix)
{
	return strtol(string.c_str(), endptr, radix);
}

//
// Opaque identifier for string. Maps to an int.
//
class StringID
{
public:
	StringID() = default;
	explicit StringID(int num) : num(num)
	{
	}
	int get() const noexcept
	{
		return num;
	}
	bool operator == (StringID other) const
	{
		return num == other.num;
	}
	bool operator != (StringID other) const
	{
		return num != other.num;
	}
	bool operator ! () const
	{
		return !num;
	}
	bool isValid() const
	{
		return num >= 0;
	}
	bool isInvalid() const noexcept
	{
		return num < 0;
	}
	bool hasContent() const
	{
		return num > 0;
	}

private:
	int num = 0;
};
static_assert(sizeof(StringID) == sizeof(int), "StringID must be size of int");

//
// String storage table
//
class StringTable
{
public:
	StringID add(const SString &str);
	SString get(StringID offset) const noexcept;
private:
	// Must start with an empty string, so get(0) gets "".
	std::vector<SString> mStrings = { "" };	
};

#ifdef _WIN32
SString WideToUTF8(const wchar_t *text);
std::wstring UTF8ToWide(const char* text);
#endif

#endif  /* __EUREKA_M_STRINGS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
