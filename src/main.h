//------------------------------------------------------------------------
//  MAIN DEFINITIONS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2020 Andrew Apted
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

#ifndef __EUREKA_MAIN_H__
#define __EUREKA_MAIN_H__


#define EUREKA_TITLE  "Eureka DOOM Editor"

#include "version.h"

#define EUREKA_LUMP  "__EUREKA"


/*
 *  Windows support
 */

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#include "WindowsInclude.h"
#endif
#ifdef None
#undef None
#endif

#include "PrintfMacros.h"

/*
 *  Standard headers
 */

#include <assert.h>
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
#else

#ifdef _MSC_VER
#include <direct.h>
#define getcwd _getcwd
#endif

#endif

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>


/*
 *  Additional libraries
 */

#include "hdr_fltk.h"


/*
 *  Commonly-used headers
 */
#include "sys_type.h"
#include "sys_macro.h"
#include "sys_endian.h"
#include "sys_debug.h"

#include "objid.h"
#include "m_bitvec.h"
#include "m_select.h"

#include "lib_util.h"
#include "lib_file.h"

#include "e_basis.h"
#include "m_keys.h"
#include "e_objects.h"


/*
 *  Miscellaneous
 */

typedef std::vector< const char * > string_list_t;

/*
 *  Interfile global variables
 */

namespace global
{
	extern bool want_quit;
	extern bool app_has_focus;
	
	extern fs::path install_dir;  // install dir (e.g. /usr/share/eureka)
	extern fs::path home_dir;      // home dir (e.g. $HOME/.eureka)
	extern fs::path cache_dir;    // for caches and backups, can be same as home_dir
}

namespace global
{
	extern fs::path config_file; // Name of the configuration file, or NULL
	extern fs::path log_file;    // Name of log file, or NULL
}

namespace global
{
	extern std::vector<fs::path> Pwad_list;
}

namespace global
{
	extern bool   show_help;		// Print usage message and exit.
	extern bool   show_version;	// Print version info and exit.
}


struct LoadingData;
struct NewResources;
struct WadData;

NewResources loadResources(const LoadingData& loading, const WadData& waddata) noexcept(false);

/*
 *  Various global functions
 */
void Main_Quit();

[[noreturn]] void FatalError(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(1, 2);

void DLG_ShowError(bool fatal, EUR_FORMAT_STRING(const char *msg), ...) EUR_PRINTF(2, 3);
void DLG_Notify(EUR_FORMAT_STRING(const char *msg), ...) EUR_PRINTF(1, 2);
int  DLG_Confirm(const std::vector<SString> &buttons, EUR_FORMAT_STRING(const char *msg), ...) EUR_PRINTF(2, 3);

extern std::function<void(const char *msg, va_list ap)> DLG_Notify_Override;
extern std::function<int(const std::vector<SString> &buttons, const char *msg,
						 va_list ap)> DLG_Confirm_Override;

SString GameNameFromIWAD(const fs::path &iwad_name);

#endif  /* __EUREKA_MAIN_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
