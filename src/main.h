//------------------------------------------------------------------------
//  MAIN DEFINITIONS
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

#ifndef __EUREKA_MAIN_H__
#define __EUREKA_MAIN_H__


#define EUREKA_TITLE  "Eureka DOOM Editor"

#define EUREKA_VERSION  "0.85"


#define Y_UNIX
#define Y_SNPRINTF


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
#endif

#include <vector>


/*
 *  Additional libraries
 */

#include "hdr_fltk.h"


/*
 *  Platform-independant types and formats.
 */
void FatalError(const char *fmt, ...);

#define BugError  FatalError


#include "sys_type.h"
#include "sys_macro.h"
#include "sys_endian.h"
#include "sys_debug.h"


#include "objid.h"
#include "m_bitvec.h"  /* bv_set, bv_clear, bv_toggle */
#include "m_select.h"

#include "lib_util.h"
#include "lib_file.h"


typedef enum
{
	SIDE_RIGHT = +1,
	SIDE_LEFT  = -1
}
side_ref_e;


#include "e_basis.h"


// key modifier (does not allow two at once, e.g. CTRL+SHIFT)
typedef enum
{
	KM_none = 0,

	KM_SHIFT = 1,
	KM_CTRL  = 2,
	KM_ALT   = 3,
}
keymod_e;


typedef std::vector< const char * > string_list_t;


/*
 *  Doom definitions
 *  Things about the Doom engine
 *  FIXME should move as much of this as possible to the ygd file...
 *  FIXME Hexen has a different value for MIN_DEATHMATH_STARTS
 */
const int DOOM_PLAYER_HEIGHT  = 56;
const size_t DOOM_MIN_DEATHMATCH_STARTS = 4;
const size_t DOOM_MAX_DEATHMATCH_STARTS = 10;


/*
 *  More stuff
 */


// Operations on the selection :

#include "objects.h"


// Confirmation options are stored internally this way :
typedef enum
{
   YC_YES      = 'y',
   YC_NO       = 'n',
   YC_ASK      = 'a',
   YC_ASK_ONCE = 'o'
}
confirm_t;


/*
 *  Even more stuff ("the macros and constants")
 */

// AYM 19980213: InputIntegerValue() uses this to mean that Esc was pressed
#define IIV_CANCEL  INT_MIN


/*
 *  Interfile global variables
 */

extern int  init_progress;
extern bool want_quit;

extern const char *install_dir;  // install dir (e.g. /usr/share/eureka) 
extern const char *home_dir;     // home dir (e.g. $HOME/.eureka)

extern const char *Game_name;   // Name of game "doom", "doom2", "heretic", ...
extern const char *Port_name;   // Name of source port "vanilla", "boom", ...
extern const char *Level_name;  // Name of map lump we are editing

extern const char *config_file; // Name of the configuration file, or NULL
extern const char *log_file;    // Name of log file, or NULL

extern const char *Iwad_name; // Name of the iwad
extern const char *Pwad_name;

extern std::vector< const char * > ResourceWads;


extern int remind_to_build_nodes; // Remind the user to build nodes

extern bool Replacer;     // the new map will destroy an existing one if saved

extern int	default_floor_h;
extern int	default_ceil_h;
extern int	default_light_level;
extern int	default_thing;

extern const char * default_floor_tex;
extern const char * default_ceil_tex;
extern const char * default_lower_tex;
extern const char * default_mid_tex;
extern const char * default_upper_tex;


extern int   show_help;     // Print usage message and exit.
extern int   show_version;  // Print version info and exit.

extern int   scroll_less;// %s of screenful to scroll by
extern int   scroll_more;// %s of screenful to scroll by


extern int KF;  // Kromulent Factor
extern int KF_fonth;  // default font size



int entryname_cmp (const char *entry1, const char *entry2);


bool Main_ConfirmQuit(const char *action);
bool Main_ProjectSetup(bool is_startup = false);
void Main_LoadResources();


void Beep();
void Beep(const char *msg, ...);


int vertex_radius (double scale);


#endif  /* __EUREKA_MAIN_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
