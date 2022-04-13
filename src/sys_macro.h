//------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2008 Andrew Apted
//  Copyright (C) 2005      Simon Howard
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

#ifndef __SYS_MACRO_H__
#define __SYS_MACRO_H__

#include <math.h>

// basic macros

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

//
// Concise round() cast to int
//
inline static int iround(double x)
{
	return static_cast<int>(round(x));
}
inline static int iround(float x)
{
	return static_cast<int>(roundf(x));
}

#ifndef CLAMP
#define CLAMP(low,x,high)  \
    ((x) < (low) ? (low) : (x) > (high) ? (high) : (x))
#endif

//
// The packed attribute forces structures to be packed into the minimum
// space necessary.  If this is not done, the compiler may align structure
// fields differently to optimize memory access, inflating the overall
// structure size.  It is important to use the packed attribute on certain
// structures where alignment is important, particularly data read/written
// to disk.
//

#ifdef __GNUC__
#define PACKEDATTR __attribute__((packed))
#else
#define PACKEDATTR
#endif

#define lengthof(x) (sizeof(x) / sizeof(*(x)))

#endif  /* __SYS_MACRO_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
