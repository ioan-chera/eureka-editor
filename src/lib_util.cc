//------------------------------------------------------------------------
//  UTILITIES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
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

#include "main.h"

// #include "m_game.h"

#include <sys/time.h>
#include <time.h>

#include "w_rawdef.h"



/*
 *  is_absolute
 *  Tell whether a file name is absolute or relative.
 *
 *  Note: for DOS, a filename of the form "d:foo" is
 *  considered absolute, even though it's technically
 *  relative to the current working directory of "d:".
 *  My reasoning is that someone who wants to specify a
 *  name that's relative to one of the standard
 *  directories is not going to put a "d:" in front of it.
 */
int is_absolute (const char *filename)
{
#if defined Y_UNIX
	return *filename == '/';
#elif defined Y_DOS
	return *filename == '/'
		|| *filename == '\\'
		|| isalpha (*filename) && filename[1] == ':';
#endif
}


/*
 *  y_stricmp
 *  A case-insensitive strcmp()
 *  (same thing as DOS stricmp() or GNU strcasecmp())
 */
int y_stricmp (const char *s1, const char *s2)
{
	for (;;)
	{
		if (tolower (*s1) != tolower (*s2))
			return (unsigned char) *s1 - (unsigned char) *s2;
		if (! *s1)
		{
			if (! *s2)
				return 0;
			else
				return -1;
		}
		if (! *s2)
			return 1;
		s1++;
		s2++;
	}
}


/*
 *  y_strnicmp
 *  A case-insensitive strncmp()
 *  (same thing as DOS strnicmp() or GNU strncasecmp())
 */
int y_strnicmp (const char *s1, const char *s2, size_t len)
{
	while (len-- > 0)
	{
		if (tolower (*s1) != tolower (*s2))
			return (unsigned char) *s1 - (unsigned char) *s2;
		if (! *s1)
		{
			if (! *s2)
				return 0;
			else
				return -1;
		}
		if (! *s2)
			return 1;
		s1++;
		s2++;
	}
	return 0;
}


/*
 *  y_snprintf
 *  If available, snprintf(). Else sprintf().
 */
int y_snprintf (char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
#ifdef Y_SNPRINTF
	return vsnprintf (buf, size, fmt, args);
#else
	return vsprintf (buf, fmt, args);
#endif
}


/*
 *  y_vsnprintf
 *  If available, vsnprintf(). Else vsprintf().
 */
int y_vsnprintf (char *buf, size_t size, const char *fmt, va_list args)
{
#ifdef Y_SNPRINTF
	return vsnprintf (buf, size, fmt, args);
#else
	return vsprintf (buf, fmt, args);
#endif
}


/*
 *  y_strupr
 *  Upper-case a string
 */
void y_strupr (char *string)
{
	while (*string)
	{
		*string = toupper (*string);
		string++;
	}
}


char *StringNew(int length)
{
  // length does not include the trailing NUL.
  
  char *s = (char *) calloc(length + 1, 1);

  if (! s)
    FatalError("Out of memory (%d bytes for string)\n", length);

  return s;
}

char *StringDup(const char *orig, int limit)
{
  if (limit < 0)
  {
    char *s = strdup(orig);

    if (! s)
      FatalError("Out of memory (copy string)\n");

    return s;
  }

  char * s = StringNew(limit+1);
  strncpy(s, orig, limit);
  s[limit] = 0;

  return s;
}

char *StringUpper(const char *name)
{
  char *copy = StringDup(name);

  for (char *p = copy; *p; p++)
    *p = toupper(*p);

  return copy;
}

char *StringPrintf(const char *str, ...)
{
  /* Algorithm: keep doubling the allocated buffer size
   * until the output fits. Based on code by Darren Salt.
   */
  char *buf = NULL;
  int buf_size = 128;
  
  for (;;)
  {
    va_list args;
    int out_len;

    buf_size *= 2;

    buf = (char*)realloc(buf, buf_size);
    if (!buf)
      FatalError("Out of memory (formatting string)");

    va_start(args, str);
    out_len = vsnprintf(buf, buf_size, str, args);
    va_end(args);

    // old versions of vsnprintf() simply return -1 when
    // the output doesn't fit.
    if (out_len < 0 || out_len >= buf_size)
      continue;

    return buf;
  }
}

void StringFree(const char *str)
{
  if (str)
  {
    free((void*) str);
  }
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

	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}


/*
 *  check_types
 *
 *  Sanity checks about the sizes and properties of certain types.
 *  Useful when porting.
 */

#define assert_size(type,size)            \
  do                  \
  {                 \
    if (sizeof (type) != size)            \
      FatalError("sizeof " #type " is %d (should be " #size ")",  \
  (int) sizeof (type));           \
  }                 \
  while (0)
   
#define assert_wrap(type,high,low)          \
  do                  \
  {                 \
    type n = high;              \
    if (++n != low)             \
      FatalError("Type " #type " wraps around to %lu (should be " #low ")",\
  (unsigned long) n);           \
  }                 \
  while (0)

void check_types ()
{
	assert_size (u8_t,  1);
	assert_size (s8_t,  1);
	assert_size (u16_t, 2);
	assert_size (s16_t, 2);
	assert_size (u32_t, 4);
	assert_size (s32_t, 4);

	assert_size (raw_linedef_t, 14);
	assert_size (raw_sector_s,  26);
	assert_size (raw_sidedef_t, 30);
	assert_size (raw_thing_t,   10);
	assert_size (raw_vertex_t,   4);
}


/*
   translate (dx, dy) into an integer angle value (0-65535)
*/

unsigned ComputeAngle(int dx, int dy)
{
	return (unsigned) (atan2 ((double) dy, (double) dx) * 10430.37835 + 0.5);
}



/*
   compute the distance from (0, 0) to (dx, dy)
*/

unsigned ComputeDist(int dx, int dy)
{
	return (unsigned) (hypot ((double) dx, (double) dy) + 0.5);
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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
