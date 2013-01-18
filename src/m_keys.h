//------------------------------------------------------------------------
//  KEY BINDINGS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2013 Andrew Apted
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

#ifndef __EUREKA_M_KEYS_H__
#define __EUREKA_M_KEYS_H__


/* key value:
 *   - can be a printable ASCII character, e.g. 'a', 'A', '2', ';'
 *   - spacebar is ' '
 *   - all other keys use the FLTK code (e.g. FL_Enter, FL_Up, etc)
 *   - control keys (like CTRL-A) use MOD_COMMAND flag (never '\001')
 *
 * modifier (MOD_XXXX value) is or-ed with the bare key.
 *   - uppercase letters (etc) do _not_ have the MOD_SHIFT flag
 *   - digits, however, _do_ have MOD_SHIFT
 *   - can extract bare key with FL_KEY_MASK
 *   - can extract modifier with MOD_ALL_MASK
 *   - currently only a single modifier will be present:
 *       MOD_COMMAND > MOD_META > MOD_ALT > MOD_SHIFT
 *   - using my own names since "FL_CONTROL" is fucking confusing
 */
typedef unsigned int keycode_t;

#define MOD_none     0

#define MOD_COMMAND  FL_COMMAND
#define MOD_META     FL_CONTROL
#define MOD_ALT      FL_ALT
#define MOD_SHIFT    FL_SHIFT

#define MOD_ALL_MASK  (MOD_COMMAND | MOD_META | MOD_ALT | MOD_SHIFT)


int M_ParseBindings();
int M_WriteBindings();


#endif  /* __EUREKA_M_KEYS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
