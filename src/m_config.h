//------------------------------------------------------------------------
//  CONFIG FILE
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

#ifndef __EUREKA_M_CONFIG_H__
#define __EUREKA_M_CONFIG_H__


typedef std::vector< const char * > string_list_t;


int parse_config_file_user(const char *filename);
int parse_config_file_default();

int parse_command_line_options (int argc, const char *const *argv, int pass);

void dump_parameters (FILE *fp);
void dump_command_line_options (FILE *fp);


// returns number of tokens, zero for comment, negative on error
int M_ParseLine(const char *line, const char ** tokens, int max_tok);

// user state persistence (stuff like camera pos, grid settings, ...)
bool M_LoadUserState();
bool M_SaveUserState();

void M_DefaultUserState();

#endif  /* __EUREKA_M_CONFIG_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
