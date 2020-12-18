//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
//  Copyright (C) 2001-2019 Andrew Apted
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

#ifndef PrintfMacros_h
#define PrintfMacros_h

//
// Printf detection
//

#ifndef __GNUC__
#define EUR_PRINTF(f, a)
#else
#define EUR_PRINTF(f, a) __attribute__((format(printf, f, a)))
#endif

#if _MSC_VER >= 1400 && defined(_DEBUG)
#include <sal.h>
#if _MSC_VER > 1400
#define EUR_FORMAT_STRING(p) _Printf_format_string_ p
#else
#define EUR_FORMAT_STRING(p) __format_string p
#endif
#else
#define EUR_FORMAT_STRING(p) p
#endif


#endif /* PrintfMacros_h */
