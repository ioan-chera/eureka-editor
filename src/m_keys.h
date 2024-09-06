//------------------------------------------------------------------------
//  KEY BINDINGS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2013-2016 Andrew Apted
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

#include "m_strings.h"
#include "objid.h"
#include "FL/Enumerations.H"

class Instance;
struct editor_command_t;
struct key_binding_t;

/* Key value:
 *   - can be a printable ASCII character, e.g. 'a', '2', ';'
 *   - spacebar is ' ' (ASCII code 32)
 *   - letter keys are lowercase
 *   - all other keys use the FLTK code (e.g. FL_Enter, FL_Up, etc)
 *   - control keys (like CTRL-A) use EMOD_COMMAND flag (never '\001')
 *
 * Modifier (MOD_XXXX value) is or-ed with the bare key.
 *   - uppercase letters are the lowercase letter + EMOD_SHIFT
 *   - can extract bare key with FL_KEY_MASK
 *   - can extract modifier with EMOD_ALL_MASK
 *   - only a single modifier can be present:
 *       EMOD_COMMAND > EMOD_META > EMOD_ALT > EMOD_SHIFT
 *   - using my own names since "FL_CONTROL" is fucking confusing
 */
typedef unsigned int keycode_t;

namespace global
{
extern std::vector<key_binding_t> pref_binds;
}

#define EMOD_none     0

#define EMOD_COMMAND  FL_COMMAND
#define EMOD_META     FL_CONTROL
#define EMOD_ALT      FL_ALT
#define EMOD_SHIFT    FL_SHIFT

#define EMOD_ALL_MASK  (EMOD_COMMAND | EMOD_META | EMOD_ALT | EMOD_SHIFT)


// made-up values to represent the mouse wheel
#define FL_WheelUp		0xEF91
#define FL_WheelDown	0xEF92
#define FL_WheelLeft	0xEF93
#define FL_WheelRight	0xEF94


bool is_mouse_wheel (keycode_t key);
bool is_mouse_button(keycode_t key);

enum class KeyContext
{
	none,	// INVALID

	browser,
	render,

	vertex,
	thing,
	sector,
	line,

	general
};

inline constexpr KeyContext validKeyContexts[] =
{
	KeyContext::browser,
	KeyContext::render,

	KeyContext::vertex,
	KeyContext::thing,
	KeyContext::sector,
	KeyContext::line,
	
	KeyContext::general
};


/* --- general manipulation --- */

int M_KeyCmp(keycode_t A, keycode_t B);

KeyContext M_ParseKeyContext(const SString &str);

keycode_t M_ParseKeyString(const SString &str);
namespace keys
{
SString toString(keycode_t key);
}

keycode_t M_TranslateKey(int key, int state);

int M_KeyToShortcut(keycode_t key);

KeyContext M_ModeToKeyContext(ObjType mode);


/* --- preferences dialog stuff --- */

void M_CopyBindings(bool from_defaults = false);
void M_SortBindings(char column, bool reverse);
void M_ApplyBindings();

int  M_NumBindings();
void M_DetectConflictingBinds();

namespace keys
{
SString stringForFunc(const key_binding_t &bind);
const char * stringForBinding(const key_binding_t& bind, bool changing_key = false);
}

void M_GetBindingInfo(int index, keycode_t *key, KeyContext *context);

void M_ChangeBindingKey(int index, keycode_t key);
const char * M_SetLocalBinding(int index, keycode_t key, KeyContext context,
                               const SString &func_str);
const char * M_AddLocalBinding(int after, keycode_t key, KeyContext context,
                               const char *func_str);
void M_DeleteLocalBinding(int index);


void M_LoadBindings();
void M_SaveBindings();

bool M_IsKeyBound   (keycode_t key, KeyContext context);
void M_RemoveBinding(keycode_t key, KeyContext context);


/* --- command execution stuff --- */

typedef void (Instance::*command_func_t)();

struct editor_command_t
{
	const char *const name;

	// used in the Key binding preferences
	// when NULL, will be computed from 'req_context'
	const char *group_name;

	command_func_t func;

	const char *const flag_list;
	const char *const keyword_list;

	// this value is computed when registering
	KeyContext req_context;

};


void M_RegisterCommandList(editor_command_t * list);

const editor_command_t *   FindEditorCommand(SString name);
const editor_command_t * LookupEditorCommand(int index);


// parameter(s) for command function -- must be valid strings
#define MAX_EXEC_PARAM   16
#define MAX_BIND_LENGTH  64

bool findKeyCodeForCommandName(const char *command, const char *params[MAX_EXEC_PARAM],
							   keycode_t *code);

struct key_binding_t
{
	keycode_t key;

	KeyContext context;

	const editor_command_t *cmd;

	SString param[MAX_EXEC_PARAM];

	// this field ONLY used by M_DetectConflictingBinds()
	bool is_duplicate;
};

#endif  /* __EUREKA_M_KEYS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
