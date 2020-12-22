//------------------------------------------------------------------------
//  UTILITIES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
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

#include "Errors.h"
#include "main.h"

#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <time.h>

#include "m_strings.h"
#include "w_rawdef.h"

void TimeDelay(unsigned int millies)
{
	SYS_ASSERT(millies < 300000);

#ifdef WIN32
	::Sleep(millies);

#else // LINUX or MacOSX

	usleep(millies * 1000);
#endif
}


unsigned int TimeGetMillies()
{
	// Note: you *MUST* handle overflow (it *WILL* happen)

#ifdef WIN32
	return GetTickCount();
#else
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	return ((int)tv.tv_sec * 1000 + (int)tv.tv_usec / 1000);
#endif
}


//
// sanity checks for the sizes and properties of certain types.
// useful when porting.
//

#define assert_size(type,size)            \
  do                  \
  {                 \
    if (sizeof (type) != size)            \
      ThrowException("sizeof " #type " is %d (should be " #size ")\n",  \
  (int) sizeof (type));           \
  }                 \
  while (0)

#define assert_wrap(type,high,low)          \
  do                  \
  {                 \
    type n = high;              \
    if (++n != low)             \
      FatalError("Type " #type " wraps around to %lu (should be " #low ")\n",\
  (unsigned long) n);           \
  }                 \
  while (0)


void CheckTypeSizes()
{
	assert_size(u8_t,  1);
	assert_size(s8_t,  1);
	assert_size(u16_t, 2);
	assert_size(s16_t, 2);
	assert_size(u32_t, 4);
	assert_size(s32_t, 4);

	assert_size(raw_linedef_t, 14);
	assert_size(raw_sector_s,  26);
	assert_size(raw_sidedef_t, 30);
	assert_size(raw_thing_t,   10);
	assert_size(raw_vertex_t,   4);
}


double PerpDist(double x, double y,
                double x1, double y1, double x2, double y2)
{
	x  -= x1; y  -= y1;
	x2 -= x1; y2 -= y1;

	double len = sqrt(x2*x2 + y2*y2);

	SYS_ASSERT(len > 0);

	return (x * y2 - y * x2) / len;
}


double AlongDist(double x, double y,
                 double x1, double y1, double x2, double y2)
{
	x  -= x1; y  -= y1;
	x2 -= x1; y2 -= y1;

	double len = sqrt(x2*x2 + y2*y2);

	SYS_ASSERT(len > 0);

	return (x * x2 + y * y2) / len;
}

//
// rounds the value _up_ to the nearest power of two.
//
int RoundPOW2(int x)
{
	if (x <= 2)
		return x;

	x--;

	for (int tmp = x >> 1 ; tmp ; tmp >>= 1)
		x |= tmp;

	return x + 1;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
