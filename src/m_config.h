//------------------------------------------------------------------------
//  CONFIG FILE
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

#ifndef __EUREKA_M_CONFIG_H__
#define __EUREKA_M_CONFIG_H__

#include "im_color.h"

/* ==== CONFIG VARIABLES ==================== */

extern bool digits_set_zoom;
extern bool leave_offsets_alone;
extern bool mouse_wheel_scrolls_map;
extern bool new_islands_are_void;
extern bool same_mode_clears_selection;
extern bool swap_sidedefs;

extern int gui_scheme;
extern int gui_color_set;
extern rgb_color_t gui_custom_bg;
extern rgb_color_t gui_custom_ig;
extern rgb_color_t gui_custom_fg;

extern int new_sector_size;

extern int  default_grid_size;
extern bool default_grid_snap;
extern int  default_grid_mode;

extern int multi_select_modifier;

extern int backup_max_files;
extern int backup_max_space;

extern bool glbsp_fast;
extern bool glbsp_verbose;
extern bool glbsp_warn;


/* ==== FUNCTIONS ==================== */

int M_ParseConfigFile();
int M_WriteConfigFile();

int M_ParseEnvironmentVars();
int M_ParseCommandLine(int argc, const char *const *argv, int pass);

void dump_parameters (FILE *fp);
void dump_command_line_options (FILE *fp);


// returns number of tokens, zero for comment, negative on error
int M_ParseLine(const char *line, const char ** tokens, int max_tok,
                bool do_strings);

// user state persistence (stuff like camera pos, grid settings, ...)
bool M_LoadUserState();
bool M_SaveUserState();

void M_DefaultUserState();

#endif  /* __EUREKA_M_CONFIG_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
