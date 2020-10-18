//------------------------------------------------------------------------
//  KEY BINDINGS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2013-2019 Andrew Apted
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

#include <algorithm>


SString EXEC_Param[MAX_EXEC_PARAM];
SString EXEC_Flags[MAX_EXEC_PARAM];

int EXEC_Errno;

keycode_t EXEC_CurKey;


// this fake mod represents the "LAX-" prefix and is NOT used
// anywhere except in the key_binding_t structure.
#define MOD_LAX_SHIFTCTRL	FL_SCROLL_LOCK


static std::vector< editor_command_t * > all_commands;


static key_context_e RequiredContextFromName(const char *name)
{
	if (strncmp(name, "LIN_", 4) == 0) return KCTX_Line;
	if (strncmp(name, "SEC_", 4) == 0) return KCTX_Sector;
	if (strncmp(name, "TH_",  3) == 0) return KCTX_Thing;
	if (strncmp(name, "VT_",  3) == 0) return KCTX_Vertex;

	// we don't need anything for KCTX_Browser or KCTX_Render

	return KCTX_NONE;
}


static const char * CalcGroupName(const char *given, key_context_e ctx)
{
	if (given)
		return given;

	switch (ctx)
	{
		case KCTX_Line:    return "Line";
		case KCTX_Sector:  return "Sector";
		case KCTX_Thing:   return "Thing";
		case KCTX_Vertex:  return "Vertex";
		case KCTX_Render:  return "3D View";
		case KCTX_Browser: return "Browser";

		default: return "General";
	}
}


//
// this should only be called during startup
//
void M_RegisterCommandList(editor_command_t * list)
{
	// the structures are used directly

	for ( ; list->name ; list++)
	{
		list->req_context = RequiredContextFromName(list->name);
		list->group_name  = CalcGroupName(list->group_name, list->req_context);

		all_commands.push_back(list);
	}
}


const editor_command_t * FindEditorCommand(const char *name)
{
	// backwards compatibility
	if (y_stricmp(name, "GRID_Step") == 0)
		name = "GRID_Bump";
	else if (y_stricmp(name, "Check") == 0)
		name = "MapCheck";
	else if (y_stricmp(name, "3D_Click") == 0)
		name = "ACT_Click";
	else if (y_stricmp(name, "3D_NAV_MouseMove") == 0)
		name = "NAV_MouseScroll";
	else if (y_stricmp(name, "OperationMenu") == 0)
		name = "OpMenu";

	for (unsigned int i = 0 ; i < all_commands.size() ; i++)
		if (y_stricmp(all_commands[i]->name, name) == 0)
			return all_commands[i];

	return NULL;
}


const editor_command_t * LookupEditorCommand(int idx)
{
	if (idx >= (int)all_commands.size())
		return NULL;

	return all_commands[idx];
}


//------------------------------------------------------------------------


struct key_mapping_t
{
	keycode_t key;
	const char * name;
};


static const key_mapping_t s_key_map[] =
{
	{ ' ',			"SPACE" },
	{ '"',			"DBLQUOTE" },
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

	// special stuff (not in FLTK)
	{ FL_WheelUp,    "WHEEL_UP" },
	{ FL_WheelDown,  "WHEEL_DOWN" },
	{ FL_WheelLeft,  "WHEEL_LEFT" },
	{ FL_WheelRight, "WHEEL_RIGHT" },

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


bool is_mouse_wheel(keycode_t key)
{
	key &= FL_KEY_MASK;

	return (FL_WheelUp <= key && key <= FL_WheelRight);
}

bool is_mouse_button(keycode_t key)
{
	key &= FL_KEY_MASK;

	return (FL_Button < key && key <= FL_Button + 8);
}


//
// returns zero (an invalid key) if parsing fails
//
keycode_t M_ParseKeyString(const char *str)
{
	int key = 0;

	// for MOD_COMMAND, accept both CMD and CTRL prefixes

	if (y_strnicmp(str, "CMD-", 4) == 0)
	{
		key |= MOD_COMMAND;
		str += 4;
	}
	else if (y_strnicmp(str, "CTRL-", 5) == 0)
	{
		key |= MOD_COMMAND;
		str += 5;
	}
	else if (y_strnicmp(str, "META-", 5) == 0)
	{
		key |= MOD_META;
		str += 5;
	}
	else if (y_strnicmp(str, "ALT-", 4) == 0)
	{
		key |= MOD_ALT;
		str += 4;
	}
	else if (y_strnicmp(str, "SHIFT-", 6) == 0)
	{
		key |= MOD_SHIFT;
		str += 6;
	}
	else if (y_strnicmp(str, "LAX-", 4) == 0)
	{
		key |= MOD_LAX_SHIFTCTRL;
		str += 4;
	}

	// convert uppercase letter --> lowercase + MOD_SHIFT
	if (strlen(str) == 1 && str[0] >= 'A' && str[0] <= 'Z')
		return MOD_SHIFT | (unsigned char) tolower(str[0]);

	if (strlen(str) == 1 && str[0] > 32 && str[0] < 127 && isprint(str[0]))
		return key | (unsigned char) str[0];

	if (y_strnicmp(str, "F", 1) == 0 && isdigit(str[1]))
		return key | (FL_F + atoi(str + 1));

	if (y_strnicmp(str, "MOUSE", 5) == 0 && isdigit(str[5]))
		return key | (FL_Button + atoi(str + 5));

	// find name in mapping table
	for (int k = 0 ; s_key_map[k].name ; k++)
		if (y_stricmp(str, s_key_map[k].name) == 0)
			return key | s_key_map[k].key;

	if (y_strnicmp(str, "KP_", 3) == 0 && 33 < str[3] && (FL_KP + str[3]) <= FL_KP_Last)
		return key | (FL_KP + str[3]);

	if (str[0] == '0' && str[1] == 'x')
		return key | (int)strtol(str, NULL, 0);

	return 0;
}


static const char * BareKeyName(keycode_t key)
{
	static char buffer[200];

	if (key < 127 && key > 32 && isprint(key) && key != '"')
	{
		buffer[0] = (char) key;
		buffer[1] = 0;

		return buffer;
	}

	if (FL_F < key && key <= FL_F_Last)
	{
		snprintf(buffer, sizeof(buffer), "F%d", key - FL_F);
		return buffer;
	}

	if (is_mouse_button(key))
	{
		snprintf(buffer, sizeof(buffer), "MOUSE%d", key - FL_Button);
		return buffer;
	}

	// find key in mapping table
	for (int k = 0 ; s_key_map[k].name ; k++)
		if (key == s_key_map[k].key)
			return s_key_map[k].name;

	if (FL_KP + 33 <= key && key <= FL_KP_Last)
	{
		snprintf(buffer, sizeof(buffer), "KP_%c", (char)(key & 127));
		return buffer;
	}

	// fallback : hex code

	snprintf(buffer, sizeof(buffer), "0x%04x", key);
	return buffer;
}


static const char *ModName_Dash(keycode_t mod)
{
#ifdef __APPLE__
	if (mod & MOD_COMMAND) return "CMD-";
#else
	if (mod & MOD_COMMAND) return "CTRL-";
#endif
	if (mod & MOD_META)    return "META-";
	if (mod & MOD_ALT)     return "ALT-";
	if (mod & MOD_SHIFT)   return "SHIFT-";

	if (mod & MOD_LAX_SHIFTCTRL) return "LAX-";

	return "";
}


static const char *ModName_Space(keycode_t mod)
{
#ifdef __APPLE__
	if (mod & MOD_COMMAND) return "CMD ";
#else
	if (mod & MOD_COMMAND) return "CTRL ";
#endif
	if (mod & MOD_META)    return "META ";
	if (mod & MOD_ALT)     return "ALT ";
	if (mod & MOD_SHIFT)   return "SHIFT ";

	if (mod & MOD_LAX_SHIFTCTRL) return "LAX ";

	return "";
}


const char * M_KeyToString(keycode_t key)
{
	static char buffer[200];

	// convert SHIFT + letter --> uppercase letter
	if ((key & MOD_ALL_MASK) == MOD_SHIFT &&
		(key & FL_KEY_MASK)  <  127 &&
		isalpha(key & FL_KEY_MASK))
	{
		snprintf(buffer, sizeof(buffer), "%c", toupper(key & FL_KEY_MASK));
		return buffer;
	}

	snprintf(buffer, sizeof(buffer), "%s%s", ModName_Dash(key),
		BareKeyName(key & FL_KEY_MASK));

	return buffer;
}


int M_KeyCmp(keycode_t A, keycode_t B)
{
	keycode_t A_mod = A & MOD_ALL_MASK;
	keycode_t B_mod = B & MOD_ALL_MASK;

	A &= FL_KEY_MASK;
	B &= FL_KEY_MASK;

	// make mouse buttons separate from everything else

	if (is_mouse_button(A) || is_mouse_wheel(A))
		A += 0x10000;

	if (is_mouse_button(B) || is_mouse_wheel(B))
		B += 0x10000;

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

		default: break;
	}

	return "INVALID";
}


//------------------------------------------------------------------------

struct key_binding_t
{
	keycode_t key;

	key_context_e context;

	const editor_command_t *cmd;

	SString param[MAX_EXEC_PARAM];

	// this field ONLY used by M_DetectConflictingBinds()
	bool is_duplicate;
};


static std::vector<key_binding_t> all_bindings;
static std::vector<key_binding_t> install_binds;


bool M_IsKeyBound(keycode_t key, key_context_e context)
{
	for (unsigned int i = 0 ; i < all_bindings.size() ; i++)
	{
		const key_binding_t& bind = all_bindings[i];

		if (bind.key == key && bind.context == context)
			return true;
	}

	return false;
}


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


static void ParseKeyBinding(const std::vector<SString> &tokens)
{
	// this ensures all parameters are NUL terminated
	key_binding_t temp = {};

	temp.key = M_ParseKeyString(tokens[1].c_str());

	if (! temp.key)
	{
		LogPrintf("bindings.cfg: cannot parse key name: %s\n", tokens[1].c_str());
		return;
	}

	temp.context = M_ParseKeyContext(tokens[0].c_str());

	if (temp.context == KCTX_NONE)
	{
		LogPrintf("bindings.cfg: unknown context: %s\n", tokens[0].c_str());
		return;
	}


	// handle un-bound keys
	if (y_stricmp(tokens[2].c_str(), "UNBOUND") == 0)
	{
#if 0
		fprintf(stderr, "REMOVED BINDING key:%04x (%s)\n", temp.key, tokens[0].c_str());
#endif
		M_RemoveBinding(temp.key, temp.context);
		return;
	}

	temp.cmd = FindEditorCommand(tokens[2].c_str());

	if (! temp.cmd)
	{
		LogPrintf("bindings.cfg: unknown function: %s\n", tokens[2].c_str());
		return;
	}

	if (temp.cmd->req_context != KCTX_NONE &&
	    temp.context != temp.cmd->req_context)
	{
		LogPrintf("bindings.cfg: function '%s' in wrong context '%s'\n",
				  tokens[2].c_str(), tokens[0].c_str());
		return;
	}

	for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
		if((int)tokens.size() >= 4 + p)
			temp.param[p] = tokens[3 + p];

#if 0  // DEBUG
	fprintf(stderr, "ADDED BINDING key:%04x --> %s\n", temp.key, tokens[2].c_str());
#endif

	M_RemoveBinding(temp.key, temp.context);

	all_bindings.push_back(temp);
}


#define MAX_TOKENS  (MAX_EXEC_PARAM + 8)

static bool LoadBindingsFromPath(const char *path, bool required)
{
	SString filename = SString(path) + "/bindings.cfg";

	std::ifstream fp(filename.c_str());
	if(!fp.is_open())
	{
		if (! required)
			return false;

		ThrowException("Missing key bindings file:\n\n%s\n", filename.c_str());
	}

	LogPrintf("Reading key bindings from: %s\n", filename.c_str());

	while (! fp.eof())
	{
		SString line;
		std::getline(fp, line.get());

		std::vector<SString> tokens;
		int num_tok = M_ParseLine(line.c_str(), tokens, ParseOptions::haveStringsKeepQuotes);

		if (num_tok == 0)
			continue;

		if (num_tok < 3)
		{
			LogPrintf("Syntax error in bindings: %s\n", line.c_str());
			continue;
		}

		ParseKeyBinding(tokens);
	}

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


static bool BindingExists(std::vector<key_binding_t>& list, const key_binding_t& bind,
                          bool full_match)
{
	for (unsigned int i = 0 ; i < list.size() ; i++)
	{
		const key_binding_t& other = list[i];

		if (bind.key != other.key)
			continue;

		if (bind.context != other.context)
			continue;

		if (! full_match)
			return true;

		if (bind.cmd != other.cmd)
			continue;

		bool same_params = true;

		for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
		{
			if (bind.param[p] != other.param[p])
			{
				same_params = false;
				break;
			}
		}

		if (same_params)
			return true;
	}

	return false;
}


void M_LoadBindings()
{
	all_bindings.clear();

	LoadBindingsFromPath(install_dir.c_str(), true /* required */);

	// keep a copy of the install_dir bindings
	CopyInstallBindings();

	LoadBindingsFromPath(home_dir.c_str(), false);
}


void M_SaveBindings()
{
	static char filename[FL_PATH_MAX];

	snprintf(filename, sizeof(filename), "%s/bindings.cfg", home_dir.c_str());

	FILE *fp = fopen(filename, "w");

	if (! fp)
	{
		LogPrintf("Failed to save key bindings to: %s\n", filename);

		DLG_Notify("Warning: failed to save key bindings\n"
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
			const key_binding_t& bind = all_bindings[i];

			if (bind.context != (key_context_e)ctx)
				continue;

			// no need to write it if unchanged from install_dir
			if (BindingExists(install_binds, bind, true /* full match */))
				continue;

			fprintf(fp, "%s\t%s\t%s", M_KeyContextString(bind.context),
					M_KeyToString(bind.key), bind.cmd->name);

			for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
			{
				if (bind.param[p])
					fprintf(fp, "\t%s", bind.param[p].c_str());
			}

			fprintf(fp, "\n");
			count++;
		}

		// find un-bound keys (relative to installation)

		for (unsigned int i = 0 ; i < install_binds.size() ; i++)
		{
			const key_binding_t& bind = install_binds[i];

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

	fclose(fp);
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

		for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
		{
			int cmp = k1.param[p].noCaseCompare(k2.param[p]);

			if (cmp != 0)
				return cmp < 0;
		}

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


SString M_StringForFunc(int index)
{
	SString buffer;
	buffer.reserve(2048);

	SYS_ASSERT(index >= 0 && index < static_cast<int>(pref_binds.size()));
	const key_binding_t& bind = pref_binds[index];

	SYS_ASSERT(!!bind.cmd);
	buffer = bind.cmd->name;

	// add the parameters

	for (int k = 0 ; k < MAX_EXEC_PARAM ; k++)
	{
		const SString &param = bind.param[k];

		if (! param)
			break;

		if (k == 0)
			buffer.push_back(':');

		buffer.push_back(' ');
		buffer += StringPrintf("%.30s", param.c_str());
	}

	return buffer;
}


const char * M_StringForBinding(int index, bool changing_key)
{
	SYS_ASSERT(index < (int)pref_binds.size());

	const key_binding_t& bind = pref_binds[index];

	static char buffer[600];

	// we prefer the UI to say "3D view" instead of "render"
	const char *ctx_name = M_KeyContextString(bind.context);
	if (y_stricmp(ctx_name, "render") == 0)
		ctx_name = "3D view";

	// display SHIFT + letter as an uppercase letter
	keycode_t tempk = bind.key;
	if ((tempk & MOD_ALL_MASK) == MOD_SHIFT &&
		(tempk & FL_KEY_MASK)  <  127 &&
		isalpha(tempk & FL_KEY_MASK))
	{
		tempk = toupper(tempk & FL_KEY_MASK);
	}

	snprintf(buffer, sizeof(buffer), "%s%6.6s%-10.10s %-9.9s %.32s",
			bind.is_duplicate ? "@C1" : "",
			changing_key ? "<?"     : ModName_Space(tempk),
			changing_key ? "\077?>" : BareKeyName(tempk & FL_KEY_MASK),
			ctx_name,
			 M_StringForFunc(index).c_str() );

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
	StringCopy(buffer, sizeof(buffer), func_str);

	for (unsigned int k = 0 ; buffer[k] ; k++)
		if (buffer[k] == ',' || buffer[k] == ':')
			buffer[k] = ' ';

	std::vector<SString> tokens;

	int num_tok = M_ParseLine(buffer, tokens, ParseOptions::haveStringsKeepQuotes);

	if (num_tok <= 0)
		return "Missing function name";

	const editor_command_t * cmd = FindEditorCommand(tokens[0].c_str());

	if (! cmd)
	{
		snprintf(error_msg, sizeof(error_msg), "Unknown function name: %s", tokens[0].c_str());
		return error_msg;
	}

	// check context is suitable

	if (cmd->req_context != KCTX_NONE &&
	    bind.context != cmd->req_context)
	{
		SString mode = StringUpper(M_KeyContextString(cmd->req_context));

		snprintf(error_msg, sizeof(error_msg), "%s can only be used in %s mode",
				 tokens[0].c_str(), mode.c_str());

		return error_msg;
	}

	/* OK : change the binding function */

	bind.cmd = cmd;

	for(SString &param : bind.param)
		param.clear();

	for(int p = 0; p < MAX_EXEC_PARAM; p++)
		if(num_tok >= 2 + p)
			bind.param[p] = tokens[1 + p];

	return NULL;
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
	// this ensures the parameters are NUL terminated
	key_binding_t temp = {};
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
	else if (state & MOD_SHIFT)   key |= MOD_SHIFT;

	return key;
}


int M_KeyToShortcut(keycode_t key)
{
	int shortcut = key & FL_KEY_MASK;

	     if (key & MOD_COMMAND) shortcut |= MOD_COMMAND;
	else if (key & MOD_ALT)     shortcut |= MOD_ALT;
	else if (key & MOD_SHIFT)   shortcut |= MOD_SHIFT;

	return shortcut;
}


key_context_e M_ModeToKeyContext(obj_type_e mode)
{
	switch (mode)
	{
		case OBJ_THINGS:   return KCTX_Thing;
		case OBJ_LINEDEFS: return KCTX_Line;
		case OBJ_SECTORS:  return KCTX_Sector;
		case OBJ_VERTICES: return KCTX_Vertex;

		default: break;
	}

	return KCTX_NONE;  /* shouldn't happen */
}


bool Exec_HasFlag(const char *flag)
{
	// the parameter should include a leading '/'

	for (int i = 0 ; i < MAX_EXEC_PARAM ; i++)
	{
		if (! EXEC_Flags[i])
			break;

		if (EXEC_Flags[i].noCaseEqual(flag))
			return true;
	}

	return false;
}


extern void Debug_CheckUnusedStuff();


static void DoExecuteCommand(const editor_command_t *cmd)
{
	(* cmd->func)();

//	Debug_CheckUnusedStuff();
}


static int FindBinding(keycode_t key, key_context_e context, bool lax_only)
{
	for (int i = 0 ; i < (int)all_bindings.size() ; i++)
	{
		const key_binding_t& bind = all_bindings[i];

		SYS_ASSERT(bind.cmd);

		if (bind.context != context)
			continue;

		// match modifiers "loosely" for certain commands (esp. NAV_xxx)
		bool is_lax = (bind.key & MOD_LAX_SHIFTCTRL) ? true : false;

		if (lax_only != is_lax)
			continue;

		keycode_t key1 = key;
		keycode_t key2 = bind.key;

		if (is_lax)
		{
			key1 &= ~ (MOD_SHIFT | MOD_COMMAND | MOD_LAX_SHIFTCTRL);
			key2 &= ~ (MOD_SHIFT | MOD_COMMAND | MOD_LAX_SHIFTCTRL);
		}

		if (key1 != key2)
			continue;

		// found a match
		return i;
	}

	// not found
	return -1;
}


bool ExecuteKey(keycode_t key, key_context_e context)
{
	for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
	{
		EXEC_Param[p].clear();
		EXEC_Flags[p].clear();
	}

	EXEC_Errno = 0;

	// this logic means we only use a LAX binding if an exact match
	// could not be found.
	int idx = FindBinding(key, context, false);

	if (idx < 0)
		idx = FindBinding(key, context, true);

	if (idx < 0)
		return false;

	key_binding_t& bind = all_bindings[idx];

	int p_idx = 0;
	int f_idx = 0;

	for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
	{
		if (! bind.param[p][0])
			break;

		// separate flags from normal parameters
		if (bind.param[p][0] == '/')
			EXEC_Flags[f_idx++] = bind.param[p];
		else
			EXEC_Param[p_idx++] = bind.param[p];
	}

	EXEC_CurKey = key;

	// commands can test for LAX mode via a special flag
	bool is_lax = (bind.key & MOD_LAX_SHIFTCTRL) ? true : false;

	if (is_lax && f_idx < MAX_EXEC_PARAM)
	{
		EXEC_Flags[f_idx++] = "/LAX";
	}

	DoExecuteCommand(bind.cmd);

	return true;
}


bool ExecuteCommand(const editor_command_t *cmd,
					const char *param1, const char *param2,
                    const char *param3, const char *param4)
{
	for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
	{
		EXEC_Param[p].clear();
		EXEC_Flags[p].clear();
	}

	// separate flags from normal parameters
	int p_idx = 0;
	int f_idx = 0;

	if (param1[0] == '/') EXEC_Flags[f_idx++] = param1;
	else if (param1[0])   EXEC_Param[p_idx++] = param1;

	if (param2[0] == '/') EXEC_Flags[f_idx++] = param2;
	else if (param2[0])   EXEC_Param[p_idx++] = param2;

	if (param3[0] == '/') EXEC_Flags[f_idx++] = param3;
	else if (param3[0])   EXEC_Param[p_idx++] = param3;

	if (param4[0] == '/') EXEC_Flags[f_idx++] = param4;
	else if (param4[0])   EXEC_Param[p_idx++] = param4;

	EXEC_Errno  = 0;
	EXEC_CurKey = 0;

	DoExecuteCommand(cmd);

	return true;
}


bool ExecuteCommand(const char *name,
					const char *param1, const char *param2,
					const char *param3, const char *param4)
{
	const editor_command_t * cmd = FindEditorCommand(name);

	if (! cmd)
		return false;

	return ExecuteCommand(cmd, param1, param2, param3, param4);
}


//
//  play a fascinating tune
//
void Beep(EUR_FORMAT_STRING(const char *fmt), ...)
{
	va_list arg_ptr;

	static char buffer[MSG_BUF_LEN];

	va_start(arg_ptr, fmt);
	vsnprintf(buffer, MSG_BUF_LEN-1, fmt, arg_ptr);
	va_end(arg_ptr);

	buffer[MSG_BUF_LEN-1] = 0;

	Status_Set("%s", buffer);
	LogPrintf("BEEP: %s\n", buffer);

	fl_beep();

	EXEC_Errno = 1;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
