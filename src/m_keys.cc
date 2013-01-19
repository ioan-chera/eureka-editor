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
#include "m_config.h"
#include "m_dialog.h"


const char * EXEC_Param[4];


typedef struct
{
	const char *name;
	command_func_t func;
	key_context_e req_context;

} editor_command_t;


static std::vector<editor_command_t *> all_commands;


/* this should only be called during startup */
void M_RegisterCommand(const char *name, command_func_t func,
                       key_context_e req_context)
{
	editor_command_t *cmd = new editor_command_t;

	cmd->name = name;
	cmd->func = func;
	cmd->req_context = req_context;

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


typedef struct
{
	keycode_t key;
	const char * name;

} key_mapping_t;


static key_mapping_t key_map[] =
{
	{ ' ',			"SPACE" },
	{ FL_BackSpace,	"BS" },
	{ FL_Tab,		"TAB" },
	{ FL_Enter,		"ENTER" },
	{ FL_Pause,		"PAUSE" },
	{ FL_Escape,	"ESC" },
	{ FL_Left,		"LEFT" },
	{ FL_Up,		"UP" },
	{ FL_Right,		"RIGHT" },
	{ FL_Down,		"DOWN" },
	{ FL_Page_Up,	"PGUP" },
	{ FL_Page_Down,	"PGDN" },
	{ FL_Home,		"HOME" },
	{ FL_End,		"END" },
	{ FL_Print,		"PRINT" },
	{ FL_Insert,	"INS" },
	{ FL_Delete,	"DEL" },
	{ FL_Menu,		"MENU" },
	{ FL_Help,		"HELP" },

	{ FL_KP_Enter,	"KP_Enter"},

	{ FL_Volume_Down,	"VOL_DOWN" },
	{ FL_Volume_Mute,	"VOL_MUTE" },
	{ FL_Volume_Up,		"VOL_UP" },
	{ FL_Media_Play,	"CD_PLAY" },
	{ FL_Media_Stop,	"CD_STOP" },
	{ FL_Media_Prev,	"CD_PREV" },
	{ FL_Media_Next,	"CD_NEXT" },
	{ FL_Home_Page,		"HOME_PAGE" },

	{ FL_Mail,		"MAIL" },
	{ FL_Search,	"SEARCH" },
	{ FL_Back,		"BACK" },
	{ FL_Forward,	"FORWARD" },
	{ FL_Stop,		"STOP" },
	{ FL_Refresh,	"REFRESH" },
	{ FL_Sleep,		"SLEEP" },
	{ FL_Favorites,	"FAVORITES" },

	{ 0, NULL } // the end
};


/* returns zero (an invalid key) if parsing fails */
keycode_t M_ParseKeyString(const char *str)
{
	int key = 0;

	if (y_strnicmp(str, "CMD-", 4) == 0)
	{
		key |= MOD_COMMAND;  str += 4;
	}
	else if (y_strnicmp(str, "META-", 5) == 0)
	{
		key |= MOD_META;  str += 5;
	}
	else if (y_strnicmp(str, "ALT-", 4) == 0)
	{
		key |= MOD_ALT;  str += 4;
	}
	else if (y_strnicmp(str, "SHIFT-", 6) == 0)
	{
		key |= MOD_META;  str += 6;
	}

	if (strlen(str) == 1 && str[0] > 32 && str[0] < 127 && isprint(str[0]))
		return key | (unsigned char) str[0];

	if (y_strnicmp(str, "F", 1) == 0 && isdigit(str[1]))
		return key | (FL_F + atoi(str + 1));

	// find name in mapping table
	for (int k = 0 ; key_map[k].name ; k++)
		if (y_stricmp(str, key_map[k].name) == 0)
			return key_map[k].key;

	if (y_strnicmp(str, "KP_", 3) == 0 && 33 < str[3] && str[3] <= 0x3d)
		return key | (FL_KP + str[3]);

	if (str[0] == '0' && str[1] == 'x')
		return key | atoi(str);

	return 0;
}


static const char * BareKeyName(keycode_t key)
{
	static char buffer[200];

	if (key < 127 && key > 32 && isprint(key))
	{
		buffer[0] = (char) key;
		buffer[1] = 0;

		return buffer;
	}

	if (FL_F < key && key <= FL_F_Last)
	{
		sprintf(buffer, "F%d", key - FL_F);
		return buffer;
	}

	// find key in mapping table
	for (int k = 0 ; key_map[k].name ; k++)
		if (key == key_map[k].key)
			return key_map[k].name;

	if (FL_KP + 33 <= key && key <= FL_KP_Last)
	{
		sprintf(buffer, "KP_%c", (char)(key & 127));
		return buffer;
	}

	// fallback : hex code

	sprintf(buffer, "0x%04x", key);

	return buffer;
}


const char * M_KeyToString(keycode_t key)
{
	const char *mod = "";

	if (key & MOD_COMMAND)
		mod = "CMD-";
	else if (key & MOD_META)
		mod = "META-";
	else if (key & MOD_ALT)
		mod = "ALT-";
	else if (key & MOD_SHIFT)
		mod = "SHIFT-";


	static char buffer[200];

	strcpy(buffer, mod);

	strcat(buffer, BareKeyName(key & FL_KEY_MASK));

	return buffer;
}


//------------------------------------------------------------------------


key_context_e M_ParseKeyContext(const char *str)
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

	return KCTX_NONE;
}

const char * M_KeyContextString(key_context_e context)
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


static void ParseBinding(const char ** tokens, int num_tok)
{
	key_binding_t temp;

	// this ensures param1/2 are NUL terminated
	memset(&temp, 0, sizeof(temp));

	temp.key = M_ParseKeyString(tokens[1]);

	if (! temp.key)
	{
		LogPrintf("bindings.cfg: cannot parse key name: %s\n", tokens[1]);
		return;
	}

	temp.context = M_ParseKeyContext(tokens[0]);

	if (temp.context == KCTX_NONE)
	{
		LogPrintf("bindings.cfg: unknown context: %s\n", tokens[0]);
		return;
	}

	temp.cmd = FindEditorCommand(tokens[2]);

	if (! temp.cmd)
	{
		LogPrintf("bindings.cfg: unknown function: %s\n", tokens[2]);
		return;
	}

	if (temp.cmd->req_context != KCTX_NONE &&
	    temp.context != temp.cmd->req_context)
	{
		LogPrintf("bindings.cfg: function '%s' in wrong context '%s'\n",
				  tokens[2], tokens[0]);
		return;
	}

	if (num_tok >= 4)
		strncpy(temp.param1, tokens[3], MAX_BIND_PARAM_LEN-1);

	if (num_tok >= 5)
		strncpy(temp.param2, tokens[4], MAX_BIND_PARAM_LEN-1);

#if 0  // DEBUG
fprintf(stderr, "ADDED BINDING key:%04x --> %s\n", temp.key, tokens[2]);
#endif

	all_bindings.push_back(temp);
}


#define MAX_TOKENS  8

void M_LoadBindings()
{
	all_bindings.clear();

	static char filename[FL_PATH_MAX];

	sprintf(filename, "%s/bindings.cfg", home_dir);

	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		// fallback to installed version
		sprintf(filename, "%s/bindings.cfg", install_dir);

		fp = fopen(filename, "r");

		if (! fp)
			FatalError("Cannot open default key bindings file:\n%s\n", filename);
	}

	LogPrintf("Reading key bindings from: %s\n", filename);

	static char line_buf[FL_PATH_MAX];

	const char * tokens[MAX_TOKENS];

	while (! feof(fp))
	{
		char *line = fgets(line_buf, FL_PATH_MAX, fp);

		if (! line)
			break;
		
		StringRemoveCRLF(line);

		int num_tok = M_ParseLine(line, tokens, MAX_TOKENS);

		if (num_tok == 0)
			continue;

		if (num_tok < 3)
		{
			LogPrintf("Syntax error in bindings: %s\n", line);
			continue;
		}

		ParseBinding(tokens, num_tok);
	}

	fclose(fp);
}


void M_SaveBindings()
{
	static char filename[FL_PATH_MAX];

	sprintf(filename, "%s/bindings.cfg", home_dir);

	FILE *fp = fopen(filename, "w");

	if (! fp)
	{
		LogPrintf("Failed to save key bindings to: %s\n", filename);

		Notify(-1, -1, "Warning: failed to save key bindings\n"
		               "(filename: %s)", filename);
		return;
	}

	LogPrintf("Writing key bindings to: %s\n", filename);

	fprintf(fp, "# Eureka key bindings\n");
	fprintf(fp, "# vi:ts=16:noexpandtab\n\n");

	for (unsigned int i = 0 ; i < all_bindings.size() ; i++)
	{
		key_binding_t& bind = all_bindings[i];

		if (bind.context == KCTX_NONE)
			continue;
		
		fprintf(fp, "%s\t%s\t%s", M_KeyContextString(bind.context),
		        M_KeyToString(bind.key), bind.cmd->name);

		if (bind.param1[0]) fprintf(fp, "\t%s", bind.param1);
		if (bind.param2[0]) fprintf(fp, "\t%s", bind.param2);

		fprintf(fp, "\n");
	}
}


key_context_e M_ModeToKeyContext(obj_type_e mode)
{
	switch (mode)
	{
		case OBJ_THINGS:   return KCTX_Thing;
		case OBJ_LINEDEFS: return KCTX_Line;
		case OBJ_SECTORS:  return KCTX_Sector;
		case OBJ_VERTICES: return KCTX_Vertex;
		case OBJ_RADTRIGS: return KCTX_RadTrig;

		default: break;
	}

	return KCTX_NONE;  /* shouldn't happen */
}


bool ExecuteKey(keycode_t key, key_context_e context)
{
	EXEC_Param[0] = EXEC_Param[1] = NULL;
	EXEC_Param[2] = EXEC_Param[3] = NULL;

	for (unsigned int i = 0 ; i < all_bindings.size() ; i++)
	{
		key_binding_t& bind = all_bindings[i];

		if (bind.key == key && bind.context == context)
		{
			EXEC_Param[0] = bind.param1;
			EXEC_Param[1] = bind.param2;

			(* bind.cmd->func)();

			return true;
		}
	}

	return false;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
