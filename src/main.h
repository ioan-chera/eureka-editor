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

#define EUREKA_VERSION  "2.00"

#define EUREKA_LUMP  "__EUREKA"


/*
 *  Windows support
 */

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
  #ifndef WIN32
  #define WIN32
  #endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

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


/*
 *  Standard headers
 */

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


enum map_format_e
{
	MAPF_INVALID = 0,

	MAPF_Doom,
	MAPF_Hexen,
	MAPF_UDMF
};


// for this, set/clear/test bits using (1 << MAPF_xxx)
typedef int map_format_bitset_t;


/*
 *  Interfile global variables
 */

extern bool want_quit;
extern bool app_has_focus;
extern bool in_fatal_error;

extern SString install_dir;  // install dir (e.g. /usr/share/eureka)
extern SString home_dir;     // home dir (e.g. $HOME/.eureka)
extern SString cache_dir;    // for caches and backups, can be same as home_dir

extern SString Game_name;   // Name of game "doom", "doom2", "heretic", ...
extern SString Port_name;   // Name of source port "vanilla", "boom", ...
extern SString Level_name;  // Name of map lump we are editing

extern map_format_e Level_format;   // format of current map
extern SString  Udmf_namespace; // for UDMF, the current namespace

extern SString config_file; // Name of the configuration file, or NULL
extern SString log_file;    // Name of log file, or NULL

extern SString Iwad_name;   // Filename of the iwad
extern SString Pwad_name;   // Filename of current wad, or NULL

extern std::vector<SString> Pwad_list;
extern std::vector<SString> Resource_list;


extern int	default_floor_h;
extern int	default_ceil_h;
extern int	default_light_level;
extern int	default_thing;

extern SString default_wall_tex;
extern SString default_floor_tex;
extern SString default_ceil_tex;


extern int   show_help;		// Print usage message and exit.
extern int   show_version;	// Print version info and exit.


extern int MadeChanges;


/*
 *  Various global functions
 */

bool Main_ConfirmQuit(const char *action);
void Main_LoadResources();
void Main_Quit();

#ifdef __GNUC__
__attribute__((noreturn))
#endif
void FatalError(const char *fmt, ...);
void ThrowException(const char *fmt, ...);

#define BugError  FatalError


void DLG_ShowError(const char *msg, ...);
void DLG_Notify(const char *msg, ...);
int  DLG_Confirm(const char *buttons, const char *msg, ...);

SString GameNameFromIWAD(const char *iwad_name);

SString Main_FileOpFolder();


void Beep(const char *msg, ...);

void Status_Set(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(1, 2);
void Status_Clear();


#endif  /* __EUREKA_MAIN_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
