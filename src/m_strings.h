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

//
// Safe string: doesn't crash if given NULL (can happen due to refactoring old "char *" code.
//
class SString
{
public:
	SString() = default;

	SString(const char *c) : data(c ? c : "")
	{
	}

	SString(const char *s, size_t len) : data(s ? s : "", s ? len : 0)
	{
	}

	SString(const char *s, size_t pos, size_t len) : data(s ? s : "", s ? pos : 0, s ? len : 0)
	{
	}

	SString(const SString &other, size_t pos, size_t len) : data(other.data, pos, len)
	{
	}

	bool empty() const
	{
		return data.empty();
	}

	bool operator != (const char *c) const
	{
		return data != (c ? c : "");
	}

	bool operator == (const char *c) const
	{
		return data == (c ? c : "");
	}

	bool operator < (const SString &other) const
	{
		return data < other.data;
	}

	char operator[](int n) const
	{
		return data[n];
	}

	char &operator[](int n)
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

	const char *c_str() const
	{
		return data.c_str();
	}

	size_t find(const char *s) const
	{
		return data.find(s ? s : "");
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

	SString &operator += (char c)
	{
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

	void assign(const char *c, size_t n)
	{
		data.assign(c ? c : "", c ? n : 0);
	}

	void clear() noexcept
	{
		data.clear();
	}

	void erase(size_t n)
	{
		data.erase(n);
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

	const char *begin() const
	{
		return &*data.begin();
	}
	char *begin()
	{
		return &*data.begin();
	}
	const char *end() const
	{
		return &*data.end();
	}
	char *end()
	{
		return &*data.end();
	}

	enum
	{
		npos = std::string::npos
	};

private:
	std::string data;
};


#endif  /* __EUREKA_M_STRINGS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
