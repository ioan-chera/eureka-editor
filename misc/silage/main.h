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
 *  Exports from main.cc
 */
void FatalError(const char *fmt, ...);


#endif  /* __MAIN_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
