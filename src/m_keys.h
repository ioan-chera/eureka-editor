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

#undef  MOD_ALT    // these clash with Windows defines
#undef  MOD_SHIFT  //

#define MOD_COMMAND  FL_COMMAND
#define MOD_META     FL_CONTROL
#define MOD_ALT      FL_ALT
#define MOD_SHIFT    FL_SHIFT

#define MOD_ALL_MASK  (MOD_COMMAND | MOD_META | MOD_ALT | MOD_SHIFT)


typedef enum
{
	KCTX_NONE = 0,  /* INVALID */

	KCTX_Browser,
	KCTX_Render,

	KCTX_Vertex,
	KCTX_RadTrig,
	KCTX_Thing,
	KCTX_Sector,
	KCTX_Line,

	KCTX_General

} key_context_e;

typedef void (* command_func_t)(void);


/* --- general manipulation --- */

int M_KeyCmp(keycode_t A, keycode_t B);

key_context_e M_ParseKeyContext(const char *str);
const char * M_KeyContextString(key_context_e context);

keycode_t M_ParseKeyString(const char *str);
const char * M_KeyToString(keycode_t key);


void M_RegisterCommand(const char *name, command_func_t func,
                       key_context_e req_context = KCTX_NONE);

void M_LoadBindings();
void M_SaveBindings();

void M_RemoveBinding(keycode_t key, key_context_e context);


/* --- preferences dialog stuff --- */

void M_CopyBindings(bool from_defaults = false);
void M_SortBindings(char column, bool reverse);
void M_ApplyBindings();

int  M_NumBindings();
void M_DetectConflictingBinds();

const char * M_StringForFunc(int index);
const char * M_StringForBinding(int index, bool changing_key = false);

void M_GetBindingInfo(int index, keycode_t *key, key_context_e *context);
bool M_IsBindingFuncValid(key_context_e context, const char * func_str);

void M_ChangeBindingKey(int index, keycode_t key);
const char * M_SetLocalBinding(int index, keycode_t key, key_context_e context,
                               const char *func_str);
const char * M_AddLocalBinding(int after, keycode_t key, key_context_e context,
                               const char *func_str);
void M_DeleteLocalBinding(int index);


/* --- command execution stuff --- */

keycode_t M_TranslateKey(int key, int state);

key_context_e M_ModeToKeyContext(obj_type_e mode);

// parameter(s) for command function -- must be valid strings
extern const char * EXEC_Param[4];

// result from command function, 0 is OK
extern int EXEC_Errno;

bool ExecuteKey(keycode_t key, key_context_e context);

bool ExecuteCommand(const char *name, const char *param1 = "",
                    const char *param2 = "", const char *param3 = "");

#endif  /* __EUREKA_M_KEYS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
