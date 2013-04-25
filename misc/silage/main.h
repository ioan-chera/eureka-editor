//------------------------------------------------------------------------
//  MAIN DEFINITIONS
//------------------------------------------------------------------------

#ifndef __MAIN_H__
#define __MAIN_H__


#define SILAGE_VERSION  "0.5.490"

/*
 *  Standard headers
 */

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <vector>


/*
 *  Additional headers
 */

// #include "sys_type.h"
// #include "sys_macro.h"

#include "window.h"


/*
 *  Useful macros
 */
#ifndef NULL
#define NULL    ((void*) 0)
#endif

#ifndef MAX
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)  ((a) < 0 ? -(a) : (a))
#endif

#ifndef I_ROUND
#define I_ROUND(x)  ((int) (((x) < 0.0f) ? ((x) - 0.5f) : ((x) + 0.5f)))
#endif

#ifndef CLAMP
#define CLAMP(low,x,high)  \
    ((x) < (low) ? (low) : (x) > (high) ? (high) : (x))
#endif


/*
 *  Exports from main.cc
 */
void FatalError(const char *fmt, ...);


#endif  /* __MAIN_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
