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

#include "main.h"


typedef struct
{
	const char *name;
	command_func_t func;

} editor_command_t;


static std::vector<editor_command_t *> all_commands;


/* this should only be called during startup */
void M_AddEditorCommand(const char *name, command_func_t func)
{
	editor_command_t *cmd = new editor_command_t;

	cmd->name = name;
	cmd->func = func;

	all_commands.push_back(cmd);
}


static const editor_command_t * FindEditorCommand(const char *name)
{
	for (unsigned int i = 0 ; i < all_commands.size() ; i++)
		if (y_stricmp(all_commands[i]->name, name) == 0)
			return all_commands[i];

	return NULL;
}


//------------------------------------------------------------------------


typedef enum
{
	KCTX_INVALID = 0,

	KCTX_Global,
	KCTX_Browser,
	KCTX_Render,

	KCTX_Line,
	KCTX_Sector,
	KCTX_Thing,
	KCTX_Vertex,
	KCTX_RadTrig,

	KCTX_Edit

} key_context_e;


static int ParseKeyContext(const char *str)
{
	if (y_stricmp(str, "global")  == 0) return KCTX_Global;
	if (y_stricmp(str, "browser") == 0) return KCTX_Browser;
	if (y_stricmp(str, "render")  == 0) return KCTX_Render;
	if (y_stricmp(str, "line")    == 0) return KCTX_Line;
	if (y_stricmp(str, "sector")  == 0) return KCTX_Sector;
	if (y_stricmp(str, "thing")   == 0) return KCTX_Thing;
	if (y_stricmp(str, "vertex")  == 0) return KCTX_Vertex;
	if (y_stricmp(str, "radtrig") == 0) return KCTX_RadTrig;
	if (y_stricmp(str, "edit")    == 0) return KCTX_Edit;

	return KCTX_INVALID;
}

static const char *KeyContextString(key_context_e context)
{
	switch (context)
	{
		case KCTX_Global:  return "global";
		case KCTX_Browser: return "browser";
		case KCTX_Render:  return "render";
		case KCTX_Line:    return "line";
		case KCTX_Sector:  return "sector";
		case KCTX_Thing:   return "thing";
		case KCTX_Vertex:  return "vertex";
		case KCTX_RadTrig: return "radtrig";
		case KCTX_Edit:    return "edit";

		default:
			break;
	}

	return "INVALID";
}


#define MAX_BIND_PARAM_LEN  16

typedef struct
{
	keycode_t key;

	key_context_e context;

	const editor_command_t *cmd;

	char param1[MAX_BIND_PARAM_LEN];
	char param2[MAX_BIND_PARAM_LEN];

} key_binding_t;


static std::vector<key_binding_t> all_bindings;


int M_ParseBindings()
{
	// TODO
}


int M_WriteBindings()
{
	// TODO
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
