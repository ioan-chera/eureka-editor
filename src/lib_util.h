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

#include "PrintfMacros.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

#include <map>
#include <string>

class SString;
struct v2double_t;

void CheckTypeSizes();

void TimeDelay(unsigned int millies);
unsigned int TimeGetMillies();

double PerpDist(v2double_t v,  /* coord to test */
                v2double_t v1, v2double_t v2 /* line */);

double AlongDist(v2double_t v, /* coord to test */
                 v2double_t v1, v2double_t v2 /* line */);

// round a positive value up to the nearest power of two
int RoundPOW2(int x);

SString GetErrorMessage(int errorNumber);

template <typename K, typename V>
const V *get(const std::map<K, V> &map, const K &key)
{
    auto it = map.find(key);
    if(it == map.end())
        return nullptr;
    return &it->second;
}

/*
 *  y_isident - return true iff <c> is one of a-z, A-Z, 0-9 or "_".
 *
 *  Intentionally not using isalpha() and co. because I
 *  don't want the results to depend on the locale.
 */
#define IDENT_SET "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"

#endif  /* __EUREKA_YUTIL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
