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

#ifndef __EUREKA_SAFE_CTYPE_H__
#define __EUREKA_SAFE_CTYPE_H__

#include <ctype.h>

// Force character to unsigned range [0, 255] before passing to ctype functions
// This prevents assertions on characters with values >127 when char is signed

int safe_isspace(int c);

int safe_isalpha(int c);

int safe_isdigit(int c);

int safe_isalnum(int c);

int safe_isupper(int c);

int safe_islower(int c);

int safe_isxdigit(int c);

int safe_iscntrl(int c);

int safe_isgraph(int c);

int safe_isprint(int c);

int safe_ispunct(int c);

// For completeness, also provide toupper/tolower wrappers
int safe_toupper(int c);

int safe_tolower(int c);

#endif  /* __EUREKA_SAFE_CTYPE_H__ */
