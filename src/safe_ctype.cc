//------------------------------------------------------------------------
//  SAFE CTYPE WRAPPERS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025 Ioan Chera
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
//  Safe wrappers for <ctype.h> functions that handle the full 0-255
//  (or -128 to +127 for signed char) range without triggering assertions
//  in Visual Studio 2022 Debug mode.
//
//  These wrappers treat characters >127 (UTF-8 bytes) as simple printable
//  non-space non-alphanumeric characters.
//
//------------------------------------------------------------------------

#include "safe_ctype.h"

int safe_isspace(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// Only ASCII characters can be space
	if (uc > 127)
		return 0;
	return isspace(uc);
}

int safe_isalpha(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// Only ASCII characters can be alphabetic
	if (uc > 127)
		return 0;
	return isalpha(uc);
}

int safe_isdigit(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// Only ASCII characters can be digits
	if (uc > 127)
		return 0;
	return isdigit(uc);
}

int safe_isalnum(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// Only ASCII characters can be alphanumeric
	if (uc > 127)
		return 0;
	return isalnum(uc);
}

int safe_isupper(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// Only ASCII characters can be uppercase
	if (uc > 127)
		return 0;
	return isupper(uc);
}

int safe_islower(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// Only ASCII characters can be lowercase
	if (uc > 127)
		return 0;
	return islower(uc);
}

int safe_isxdigit(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// Only ASCII characters can be hex digits
	if (uc > 127)
		return 0;
	return isxdigit(uc);
}

int safe_iscntrl(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// UTF-8 bytes >127 are not control characters
	if (uc > 127)
		return 0;
	return iscntrl(uc);
}

int safe_isgraph(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// UTF-8 bytes >127 are treated as graphical characters
	if (uc > 127)
		return 1;
	return isgraph(uc);
}

int safe_isprint(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// UTF-8 bytes >127 are treated as printable characters
	if (uc > 127)
		return 1;
	return isprint(uc);
}

int safe_ispunct(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// UTF-8 bytes >127 are not punctuation
	if (uc > 127)
		return 0;
	return ispunct(uc);
}

// For completeness, also provide toupper/tolower wrappers
int safe_toupper(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// Only convert ASCII lowercase letters
	if (uc > 127)
		return c;
	return toupper(uc);
}

int safe_tolower(int c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	// Only convert ASCII uppercase letters
	if (uc > 127)
		return c;
	return tolower(uc);
}
