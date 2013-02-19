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

#include <algorithm>


const char * EXEC_Param[4];

int EXEC_Errno;


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

	// some synonyms for user input
	{ ' ',			"SPC" },
	{ FL_BackSpace,	"BACKSPACE" },
	{ FL_Enter,		"RETURN" },
	{ FL_Escape,	"ESCAPE" },
	{ FL_Insert,	"INSERT" },
	{ FL_Delete,	"DELETE" },
	{ FL_Page_Up,	"PAGEUP" },
	{ FL_Page_Down,	"PAGEDOWN" },

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
		key |= MOD_SHIFT;  str += 6;
	}

	if (strlen(str) == 1 && str[0] > 32 && str[0] < 127 && isprint(str[0]))
		return key | (unsigned char) str[0];

	if (y_strnicmp(str, "F", 1) == 0 && isdigit(str[1]))
		return key | (FL_F + atoi(str + 1));

	// find name in mapping table
	for (int k = 0 ; key_map[k].name ; k++)
		if (y_stricmp(str, key_map[k].name) == 0)
			return key | key_map[k].key;

	if (y_strnicmp(str, "KP_", 3) == 0 && 33 < str[3] && (FL_KP + str[3]) <= FL_KP_Last)
		return key | (FL_KP + str[3]);

	if (str[0] == '0' && str[1] == 'x')
		return key | (int)strtol(str, NULL, 0);

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


static const char *ModName(keycode_t mod)
{
	if (mod & MOD_COMMAND) return "CMD-";
	if (mod & MOD_META)    return "META-";
	if (mod & MOD_ALT)     return "ALT-";
	if (mod & MOD_SHIFT)   return "SHIFT-";

	return "";
}


const char * M_KeyToString(keycode_t key)
{
	static char buffer[200];

	strcpy(buffer, ModName(key));

	strcat(buffer, BareKeyName(key & FL_KEY_MASK));

	return buffer;
}


int M_KeyCmp(keycode_t A, keycode_t B)
{
	keycode_t A_mod = A & MOD_ALL_MASK;
	keycode_t B_mod = B & MOD_ALL_MASK;

	A &= FL_KEY_MASK;
	B &= FL_KEY_MASK;

	// we want lower- and uppercase of a key together (e.g. a + A)

	if (A < 256 && isupper(A))
	{
		A = tolower(A); A_mod |= MOD_SHIFT;
	}

	if (B < 256 && isupper(B))
	{
		B = tolower(B); B_mod |= MOD_SHIFT;
	}

	// base key is most important

	if (A != B)
		return (int)A - (int)B;

	// modifiers are the least important

	return (int)A_mod - (int)B_mod;
}


//------------------------------------------------------------------------


key_context_e M_ParseKeyContext(const char *str)
{
	if (y_stricmp(str, "browser") == 0) return KCTX_Browser;
	if (y_stricmp(str, "render")  == 0) return KCTX_Render;
	if (y_stricmp(str, "general") == 0) return KCTX_General;

	if (y_stricmp(str, "line")    == 0) return KCTX_Line;
	if (y_stricmp(str, "sector")  == 0) return KCTX_Sector;
	if (y_stricmp(str, "thing")   == 0) return KCTX_Thing;
	if (y_stricmp(str, "vertex")  == 0) return KCTX_Vertex;
	if (y_stricmp(str, "radtrig") == 0) return KCTX_RadTrig;

// TEMPORARY for compatibility
	if (y_stricmp(str, "global")  == 0) return KCTX_General;

	return KCTX_NONE;
}

const char * M_KeyContextString(key_context_e context)
{
	switch (context)
	{
		case KCTX_Browser: return "browser";
		case KCTX_Render:  return "render";
		case KCTX_General: return "general";

		case KCTX_Line:    return "line";
		case KCTX_Sector:  return "sector";
		case KCTX_Thing:   return "thing";
		case KCTX_Vertex:  return "vertex";
		case KCTX_RadTrig: return "radtrig";

		default:
			break;
	}

	return "INVALID";
}


//------------------------------------------------------------------------

#define MAX_BIND_PARAM_LEN  16

typedef struct
{
	keycode_t key;

	key_context_e context;

	const editor_command_t *cmd;

	char param[2][MAX_BIND_PARAM_LEN];

	// this field ONLY used by M_DetectConflictingBinds()
	bool is_duplicate;

} key_binding_t;


static std::vector<key_binding_t> all_bindings;
static std::vector<key_binding_t> install_binds;


void M_RemoveBinding(keycode_t key, key_context_e context)
{
	std::vector<key_binding_t>::iterator IT;

	for (IT = all_bindings.begin() ; IT != all_bindings.end() ; IT++)
	{
		if (IT->key == key && IT->context == context)
		{
			// found it
			all_bindings.erase(IT);

			// there should never be more than one
			// (besides, our iterator is now invalid)
			return;
		}
	}
}


static void ParseKeyBinding(const char ** tokens, int num_tok)
{
	key_binding_t temp;

	// this ensures the parameters are NUL terminated
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


	// handle un-bound keys
	if (y_stricmp(tokens[2], "UNBOUND") == 0)
	{
#if 0
fprintf(stderr, "REMOVED BINDING key:%04x (%s)\n", temp.key, tokens[0]);
#endif
		M_RemoveBinding(temp.key, temp.context);
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
		strncpy(temp.param[0], tokens[3], MAX_BIND_PARAM_LEN-1);

	if (num_tok >= 5)
		strncpy(temp.param[1], tokens[4], MAX_BIND_PARAM_LEN-1);

#if 0  // DEBUG
fprintf(stderr, "ADDED BINDING key:%04x --> %s\n", temp.key, tokens[2]);
#endif

	M_RemoveBinding(temp.key, temp.context);

	all_bindings.push_back(temp);
}


#define MAX_TOKENS  8

static bool LoadBindingsFromPath(const char *path, bool required)
{
	static char filename[FL_PATH_MAX];

	sprintf(filename, "%s/bindings.cfg", path);

	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		if (! required)
			return false;

		FatalError("Cannot open key bindings file:\n%s\n", filename);
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

		int num_tok = M_ParseLine(line, tokens, MAX_TOKENS, false /* do_strings */);

		if (num_tok == 0)
			continue;

		if (num_tok < 3)
		{
			LogPrintf("Syntax error in bindings: %s\n", line);
			continue;
		}

		ParseKeyBinding(tokens, num_tok);
	}

	fclose(fp);

	return true;
}


static void CopyInstallBindings()
{
	install_binds.clear();

	for (unsigned int i = 0 ; i < all_bindings.size() ; i++)
	{
		install_binds.push_back(all_bindings[i]);
	}
}


static bool BindingExists(std::vector<key_binding_t>& list, key_binding_t& bind,
                          bool full_match)
{
	for (unsigned int i = 0 ; i < list.size() ; i++)
	{
		key_binding_t& other = list[i];

		if (bind.key != other.key)
			continue;

		if (bind.context != other.context)
			continue;

		if (! full_match ||
			(bind.cmd == other.cmd &&
			 strcmp(bind.param[0], other.param[0]) == 0 &&
			 strcmp(bind.param[1], other.param[1]) == 0))
		{
			return true;
		}
	}

	return false;
}


void M_LoadBindings()
{
	all_bindings.clear();

	LoadBindingsFromPath(install_dir, true /* required */);

	// keep a copy of the install_dir bindings
	CopyInstallBindings();

	LoadBindingsFromPath(home_dir, false);
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

	fprintf(fp, "# Eureka key bindings (local)\n");
	fprintf(fp, "# vi:ts=16:noexpandtab\n\n");

	for (int ctx = KCTX_Browser ; ctx <= KCTX_General ; ctx++)
	{
		int count = 0;

		for (unsigned int i = 0 ; i < all_bindings.size() ; i++)
		{
			key_binding_t& bind = all_bindings[i];

			if (bind.context != (key_context_e)ctx)
				continue;

			// no need to write it if unchanged from install_dir
			if (BindingExists(install_binds, bind, true /* full match */))
				continue;

			fprintf(fp, "%s\t%s\t%s", M_KeyContextString(bind.context),
					M_KeyToString(bind.key), bind.cmd->name);

			if (bind.param[0][0]) fprintf(fp, "\t%s", bind.param[0]);
			if (bind.param[1][0]) fprintf(fp, "\t%s", bind.param[1]);

			fprintf(fp, "\n");
			count++;
		}

		// find un-bound keys (relative to installation)

		for (unsigned int i = 0 ; i < install_binds.size() ; i++)
		{
			key_binding_t& bind = install_binds[i];

			if (bind.context != ctx)
				continue;

			if (! BindingExists(all_bindings, bind, false /* full match */))
			{
				fprintf(fp, "%s\t%s\t%s\n", M_KeyContextString(bind.context),
						M_KeyToString(bind.key), "UNBOUND");
				count++;
			}
		}

		if (count > 0)
			fprintf(fp, "\n");
	}
}


//------------------------------------------------------------------------
//  PREFERENCE DIALOG STUFF
//------------------------------------------------------------------------

// local copy of the bindings
// these only become live after M_ApplyBindings()
static std::vector<key_binding_t> pref_binds;


void M_CopyBindings(bool from_defaults)
{
	// returns # of bindings

	pref_binds.clear();

	if (from_defaults)
	{
		for (unsigned int i = 0 ; i < install_binds.size() ; i++)
			pref_binds.push_back(install_binds[i]);
	}
	else
	{
		for (unsigned int i = 0 ; i < all_bindings.size() ; i++)
			pref_binds.push_back(all_bindings[i]);
	}
}


void M_ApplyBindings()
{
	all_bindings.clear();

	for (unsigned int i = 0 ; i < pref_binds.size() ; i++)
		all_bindings.push_back(pref_binds[i]);
}


int M_NumBindings()
{
	return (int)pref_binds.size();
}


struct KeyBind_CMP_pred
{
private:
	char column;

public:
	KeyBind_CMP_pred(char _col) : column(_col)
	{ }

	inline bool operator() (const key_binding_t& k1, const key_binding_t& k2) const
	{
		if (column == 'c' && k1.context != k2.context)
			return k1.context > k2.context;
			
		if (column != 'f' && k1.key != k2.key)
			return M_KeyCmp(k1.key, k2.key) < 0;

///		if (column == 'k' && k1.context != k2.context)
///			return k1.context > k2.context;
			
		if (k1.cmd != k2.cmd)
			return y_stricmp(k1.cmd->name, k2.cmd->name) < 0;

		int param_cmp = y_stricmp(k1.param[0], k2.param[0]) * 2 +
		                y_stricmp(k1.param[1], k2.param[1]);
		
		if (param_cmp != 0)
			return param_cmp < 0;

		if (column == 'f' && k1.key != k2.key)
			return M_KeyCmp(k1.key, k2.key) < 0;

		return k1.context < k2.context;
	}
};


void M_SortBindings(char column, bool reverse)
{
	std::sort(pref_binds.begin(), pref_binds.end(),
	          KeyBind_CMP_pred(column));

	if (reverse)
		std::reverse(pref_binds.begin(), pref_binds.end());
}


void M_DetectConflictingBinds()
{
	// copy the local bindings and sort them.
	// duplicate bindings will be contiguous in the list.

	std::vector<key_binding_t> list;

	for (unsigned int i = 0 ; i < pref_binds.size() ; i++)
	{
		pref_binds[i].is_duplicate = false;

		list.push_back(pref_binds[i]);
	}

	std::sort(list.begin(), list.end(), KeyBind_CMP_pred('c'));

	for (unsigned int k = 0 ; k + 1 < list.size() ; k++)
	{
		if (! (list[k].key     == list[k+1].key &&
			   list[k].context == list[k+1].context))
			continue;

		// mark these in the local bindings

		for (unsigned int n = 0 ; n < pref_binds.size() ; n++)
		{
			if (pref_binds[n].key     == list[k].key &&
			    pref_binds[n].context == list[k].context)
			{
				pref_binds[n].is_duplicate = true;
			}
		}
	}
}


const char * M_StringForFunc(int index)
{
	static char buffer[300];

	key_binding_t& bind = pref_binds[index];

	strcpy(buffer, bind.cmd->name);

	// add the parameters

	char *pos = buffer;

	bool saw_arg = false;

	for (int k = 0 ; k < 2 ; k++)
	{
		const char *param = bind.param[k];

		if (! param[0])
			break;

		if (! saw_arg)
		{
			strcat(buffer, "(");
			saw_arg = true;
		}

		pos = buffer + strlen(buffer);

		if (k > 0)
		{
			*pos++ = ',';
			*pos++ = ' ';
		}

		sprintf(pos, "%.30s", param);
	}

	if (saw_arg)
		strcat(buffer, ")");

	return buffer;
}


const char * M_StringForBinding(int index, bool changing_key)
{
	SYS_ASSERT(index < (int)pref_binds.size());

	key_binding_t& bind = pref_binds[index];

	static char buffer[600];

	sprintf(buffer, "%s%6.6s%-9.9s %-10.10s %.30s",
			bind.is_duplicate ? "@C1" : "",
			changing_key ? "<?"     : ModName(bind.key),
			changing_key ? "\077?>" : BareKeyName(bind.key & FL_KEY_MASK),
			M_KeyContextString(bind.context),
			M_StringForFunc(index) );

	return buffer;
}


void M_GetBindingInfo(int index, keycode_t *key, key_context_e *context)
{
	// hmmm... exposing key_binding_t may have been easier...

	*key     = pref_binds[index].key;
	*context = pref_binds[index].context;
}


void M_ChangeBindingKey(int index, keycode_t key)
{
	SYS_ASSERT(0 <= index && index < (int)pref_binds.size());
	SYS_ASSERT(key != 0);

	pref_binds[index].key = key;
}


static const char * DoParseBindingFunc(key_binding_t& bind, const char * func_str)
{
	static char error_msg[1024];

	// convert the brackets and commas into spaces and use the
	// line tokeniser.

	static char buffer[600];
	strncpy(buffer, func_str, sizeof(buffer));
	buffer[sizeof(buffer) - 1] = 0;

	for (unsigned int k = 0 ; buffer[k] ; k++)
		if (buffer[k] == '(' || buffer[k] == ')' || buffer[k] == ',')
			buffer[k] = ' ';

	const char * tokens[MAX_TOKENS];

	int num_tok = M_ParseLine(buffer, tokens, MAX_TOKENS, false /* do_strings */);

	if (num_tok == 0)
		return "Missing function name";

	const editor_command_t * cmd = FindEditorCommand(tokens[0]);

	if (! cmd)
	{
		sprintf(error_msg, "Unknown function name: %s", tokens[0]);
		return error_msg;
	}

	// check context is suitable

	if (cmd->req_context != KCTX_NONE &&
	    bind.context != cmd->req_context)
	{
		char *mode = StringUpper(M_KeyContextString(cmd->req_context));

		sprintf(error_msg, "%s can only be used in %s mode",
		        tokens[0], mode);
		
		StringFree(mode);

		return error_msg;
	}

	/* OK : change the binding function */

	bind.cmd = cmd;

	if (num_tok >= 2)
		strncpy(bind.param[0], tokens[1], MAX_BIND_PARAM_LEN-1);

	if (num_tok >= 3)
		strncpy(bind.param[1], tokens[2], MAX_BIND_PARAM_LEN-1);

	return NULL;
}


bool M_IsBindingFuncValid(key_context_e context, const char * func_str)
{
	key_binding_t temp;

	temp.key = 'a';  // dummy key
	temp.context = context;

	return (DoParseBindingFunc(temp, func_str) == NULL);
}


// returns an error message, or NULL if OK
const char * M_SetLocalBinding(int index, keycode_t key, key_context_e context,
                       const char *func_str)
{
	SYS_ASSERT(0 <= index && index < (int)pref_binds.size());

	pref_binds[index].key = key;
	pref_binds[index].context = context;

	const char *err_msg = DoParseBindingFunc(pref_binds[index], func_str);

	return err_msg;
}


const char * M_AddLocalBinding(int after, keycode_t key, key_context_e context,
                       const char *func_str)
{
	key_binding_t temp;

	// this ensures the parameters are NUL terminated
	memset(&temp, 0, sizeof(temp));

	temp.key = key;
	temp.context = context;

	const char *err_msg = DoParseBindingFunc(temp, func_str);

	if (err_msg)
		return err_msg;

	if (after < 0)
		pref_binds.push_back(temp);
	else
		pref_binds.insert(pref_binds.begin() + (1 + after), temp);

	return NULL;  // OK
}


void M_DeleteLocalBinding(int index)
{
	SYS_ASSERT(0 <= index && index < (int)pref_binds.size());

	pref_binds.erase(pref_binds.begin() + index);
}


//------------------------------------------------------------------------
//  COMMAND EXECUTION STUFF
//------------------------------------------------------------------------


keycode_t M_TranslateKey(int key, int state)
{
	// ignore modifier keys themselves
	switch (key)
	{
		case FL_Num_Lock:
		case FL_Caps_Lock:

		case FL_Shift_L: case FL_Control_L:
		case FL_Shift_R: case FL_Control_R:
		case FL_Meta_L:  case FL_Alt_L:
		case FL_Meta_R:  case FL_Alt_R:
			return 0;
	}

	if (key == '\t') key = FL_Tab;
	if (key == '\b') key = FL_BackSpace;

	// modifier logic -- only allow a single one 

	     if (state & MOD_COMMAND) key |= MOD_COMMAND;
	else if (state & MOD_META)    key |= MOD_META;
	else if (state & MOD_ALT)     key |= MOD_ALT;
	else if (state & MOD_SHIFT)
	{
		// Note: SHIFT + digit is kept that way (rather than get '!', '@' etc)

		if (key < 127 && isalpha(key))
			key = toupper(key);
		else if (key < 127 && ispunct(key) && strlen(Fl::event_text()) == 1)
			key = Fl::event_text()[0];
		else
			key |= MOD_SHIFT;
	}

	return key;
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
	Status_Clear();

	EXEC_Param[0] = EXEC_Param[1] = "";
	EXEC_Param[2] = EXEC_Param[3] = "";

	EXEC_Errno = 0;

	for (unsigned int i = 0 ; i < all_bindings.size() ; i++)
	{
		key_binding_t& bind = all_bindings[i];

		if (bind.key == key && bind.context == context)
		{
			EXEC_Param[0] = bind.param[0];
			EXEC_Param[1] = bind.param[1];

			(* bind.cmd->func)();

			return true;
		}
	}

	return false;
}


bool ExecuteCommand(const char *name, const char *param1,
                    const char *param2, const char *param3)
{
	const editor_command_t * cmd = FindEditorCommand(name);

	if (! cmd)
		return false;
	
	Status_Clear();

	EXEC_Param[0] = param1;
	EXEC_Param[1] = param2;
	EXEC_Param[2] = param3;
	EXEC_Param[3] = "";

	EXEC_Errno = 0;

	(* cmd->func)();

	return true;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
