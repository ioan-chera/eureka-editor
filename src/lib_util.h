//------------------------------------------------------------------------
//  UTILITIES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2013 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __EUREKA_LIB_UTIL_H__
#define __EUREKA_LIB_UTIL_H__

class SString;

int y_stricmp (const char *s1, const char *s2);
int y_strnicmp (const char *s1, const char *s2, size_t len);

void y_strupr (char *str);
void y_strlowr (char *str);

char *StringNew(int length);
char *StringDup(const char *orig, int limit = -1);
SString StringUpper(const SString &name);
SString StringLower(const SString &name);
SString StringPrintf(EUR_FORMAT_STRING(const char *str), ...) EUR_PRINTF(1, 2);
SString StringVPrintf(const char *str, va_list ap);
void StringCopy(char *buffer, size_t size, const SString &source);

void StringRemoveCRLF(char *str);
void StringRemoveCRLF(SString &string);

void CheckTypeSizes();

void TimeDelay(unsigned int millies);
unsigned int TimeGetMillies();

unsigned int ComputeAngle (int, int);
unsigned int ComputeDist  (int, int);

double PerpDist(double x, double y,  /* coord to test */
                double x1, double y1, double x2, double y2 /* line */);

double AlongDist(double x, double y, /* coord to test */
                 double x1, double y1, double x2, double y2 /* line */);

// round a positive value up to the nearest power of two
int RoundPOW2(int x);


const char * Int_TmpStr(int value);


/*
 *  dectoi
 *  If <c> is a decimal digit ("[0-9]"), return its value.
 *  Else, return a negative number.
 */
inline int dectoi (char c)
{
  if (isdigit ((unsigned char) c))
    return c - '0';
  else
    return -1;
}


/*
 *  hextoi
 *  If <c> is a hexadecimal digit ("[0-9A-Fa-f]"), return its value.
 *  Else, return a negative number.
 */
inline int hextoi (char c)
{
  if (isdigit ((unsigned char) c))
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else
    return -1;
}


/*
 *  y_isident - return true iff <c> is one of a-z, A-Z, 0-9 or "_".
 *
 *  Intentionally not using isalpha() and co. because I
 *  don't want the results to depend on the locale.
 */
#define IDENT_SET "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"

inline bool y_isident (char c)
{
  switch (c)
  {
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':

    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':

    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':

    case '_':

      return true;

    default:

      return false;
  }
}


/*
 *  round_up
 *  Round a value up to the next multiple of quantum.
 *
 *  Both the value and the quantum are supposed to be positive.
 */
inline void round_up (int& value, int quantum)
{
  value = ((value + quantum - 1) / quantum) * quantum;
}


/*
 *  y_isprint
 *  Is <c> a printable character in ISO-8859-1 ?
 */
inline bool y_isprint (char c)
{
  return (c & 0x60) && (c != 0x7f);
}

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

	SString(const SString &other, size_t pos, size_t len = std::string::npos) :
			data(other.data, pos, len)
	{
	}

	SString(std::string &&consume) : data(std::move(consume))
	{
	}

	bool empty() const
	{
		return data.empty();
	}

	bool noCaseEqual(const SString &c) const
	{
		return noCaseEqual(c.c_str());
	}

	bool noCaseEqual(const char *c) const
	{
		return !y_stricmp(data.c_str(), c ? c : "");
	}

	int noCaseCompare(const SString &other) const
	{
		return y_stricmp(data.c_str(), other.c_str());
	}

	bool noCaseStartsWith(const char *c, int pos = 0) const
	{
		return !y_strnicmp(data.c_str() + pos, c ? c : "", c ? strlen(c) : 0);
	}

	size_t findNoCase(const char *c) const
	{
		SString upperme = StringUpper(*this);
		SString upperthem = StringUpper(c);
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

	const char *c_str() const
	{
		return data.c_str();
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

	SString &assign(const SString &other, size_t subpos, size_t sublen = std::string::npos)
	{
		data.assign(other.get(), subpos, sublen);
		return *this;
	}

	SString &insert(size_t pos, const char *s)
	{
		data.insert(pos, s ? s : "");
		return *this;
	}

	void clear() noexcept
	{
		data.clear();
	}

	void erase(size_t n, size_t len = std::string::npos)
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
	// Convenience operator
	//
	operator bool() const
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
	void removeCRLF();

	void getCutWithSpace(size_t pos, SString *word0, SString *word1) const;
	void cutWithSpace(size_t pos, SString *second);

	void trimLeadingSpaces();
	void trimTrailingSpaces();

	size_t findSpace() const;

private:
	std::string data;
};

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

SString StringTidy(const SString &str, const SString &bad_chars = "");

#endif  /* __EUREKA_YUTIL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
