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

#include "Errors.h"
#include "Instance.h"
#include "main.h"
#include "m_parse.h"

#include <assert.h>
#include <algorithm>

// this fake mod represents the "LAX-" prefix and is NOT used
// anywhere except in the key_binding_t structure.
#define MOD_LAX_SHIFTCTRL	FL_SCROLL_LOCK

namespace global
{
	static std::vector< editor_command_t * > all_commands;
}

static KeyContext RequiredContextFromName(const char *name)
{
	if (strncmp(name, "LIN_", 4) == 0) return KeyContext::line;
	if (strncmp(name, "SEC_", 4) == 0) return KeyContext::sector;
	if (strncmp(name, "TH_",  3) == 0) return KeyContext::thing;
	if (strncmp(name, "VT_",  3) == 0) return KeyContext::vertex;

	// we don't need anything for KCTX_Browser or KCTX_Render

	return KeyContext::none;
}


static const char * CalcGroupName(const char *given, KeyContext ctx)
{
	if (given)
		return given;

	switch (ctx)
	{
		case KeyContext::line:    return "Line";
		case KeyContext::sector:  return "Sector";
		case KeyContext::thing:   return "Thing";
		case KeyContext::vertex:  return "Vertex";
		case KeyContext::render:  return "3D View";
		case KeyContext::browser: return "Browser";

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

		global::all_commands.push_back(list);
	}
}


const editor_command_t * FindEditorCommand(SString name)
{
	// backwards compatibility
	if (name.noCaseEqual("GRID_Step"))
		name = "GRID_Bump";
	else if (name.noCaseEqual("Check"))
		name = "MapCheck";
	else if (name.noCaseEqual("3D_Click"))
		name = "ACT_Click";
	else if (name.noCaseEqual("3D_NAV_MouseMove"))
		name = "NAV_MouseScroll";
	else if (name.noCaseEqual("OperationMenu"))
		name = "OpMenu";

	for (const editor_command_t *command : global::all_commands)
		if (name.noCaseEqual(command->name))
			return command;

	return NULL;
}


const editor_command_t * LookupEditorCommand(int idx)
{
	if (idx >= (int)global::all_commands.size())
		return NULL;

	return global::all_commands[idx];
}


//------------------------------------------------------------------------


struct key_mapping_t
{
	const keycode_t key;
	const char *const name;
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
keycode_t M_ParseKeyString(const SString &mstr)
{
	int key = 0;

	// for EMOD_COMMAND, accept both CMD and CTRL prefixes

	SString str = mstr;

	if (str.noCaseStartsWith("CMD-"))
	{
		key |= EMOD_COMMAND;
		str.erase(0, 4);
	}
	else if (str.noCaseStartsWith("CTRL-"))
	{
		key |= EMOD_COMMAND;
		str.erase(0, 5);
	}
	else if (str.noCaseStartsWith("META-"))
	{
		key |= EMOD_META;
		str.erase(0, 5);
	}
	else if (str.noCaseStartsWith("ALT-"))
	{
		key |= EMOD_ALT;
		str.erase(0, 4);
	}
	else if (str.noCaseStartsWith("SHIFT-"))
	{
		key |= EMOD_SHIFT;
		str.erase(0, 6);
	}
	else if (str.noCaseStartsWith("LAX-"))
	{
		key |= MOD_LAX_SHIFTCTRL;
		str.erase(0, 4);
	}

	// convert uppercase letter --> lowercase + EMOD_SHIFT
	if (str.length() == 1 && str[0] >= 'A' && str[0] <= 'Z')
		return EMOD_SHIFT | (unsigned char) tolower(str[0]);

	if (str.length() == 1 && str[0] > 32 && str[0] < 127 && isprint(str[0]))
		return key | (unsigned char) str[0];

	if (str.noCaseStartsWith("F") && isdigit(str[1]))
		return key | (FL_F + atoi(str.c_str() + 1));

	if (str.noCaseStartsWith("MOUSE") && isdigit(str[5]))
		return key | (FL_Button + atoi(str.c_str() + 5));

	// find name in mapping table
	for (int k = 0 ; s_key_map[k].name ; k++)
		if (str.noCaseEqual(s_key_map[k].name))
			return key | s_key_map[k].key;

	if (str.noCaseStartsWith("KP_") && 33 < str[3] && (FL_KP + str[3]) <= FL_KP_Last)
		return key | (FL_KP + str[3]);

	if (str[0] == '0' && str[1] == 'x')
		return key | (int)strtol(str.c_str(), NULL, 0);

	return 0;
}

static SString BareKeyName(keycode_t key)
{
	if(key < 127 && key > 32 && isprint(key) && key != '"')
		return SString(static_cast<char>(key));
	if(FL_F < key && key <= FL_F_Last)
		return SString::printf("F%d", key - FL_F);
	if(is_mouse_button(key))
		return SString::printf("MOUSE%d", key - FL_Button);
	// find key in mapping table
	for(int k = 0; s_key_map[k].name; k++)
		if(key == s_key_map[k].key)
			return s_key_map[k].name;
	if(FL_KP + 33 <= key && key <= FL_KP_Last)
		return SString::printf("KP_%c", (char)(key & 127));

	// fallback : hex code
	return SString::printf("0x%04x", key);
}


static const char *ModName_Dash(keycode_t mod)
{
#ifdef __APPLE__
	if (mod & EMOD_COMMAND) return "CMD-";
#else
	if (mod & EMOD_COMMAND) return "CTRL-";
#endif
	if (mod & EMOD_META)    return "META-";
	if (mod & EMOD_ALT)     return "ALT-";
	if (mod & EMOD_SHIFT)   return "SHIFT-";

	if (mod & MOD_LAX_SHIFTCTRL) return "LAX-";

	return "";
}


static const char *ModName_Space(keycode_t mod)
{
#ifdef __APPLE__
	if (mod & EMOD_COMMAND) return "CMD ";
#else
	if (mod & EMOD_COMMAND) return "CTRL ";
#endif
	if (mod & EMOD_META)    return "META ";
	if (mod & EMOD_ALT)     return "ALT ";
	if (mod & EMOD_SHIFT)   return "SHIFT ";

	if (mod & MOD_LAX_SHIFTCTRL) return "LAX ";

	return "";
}


SString M_KeyToString(keycode_t key)
{
	// convert SHIFT + letter --> uppercase letter
	if ((key & EMOD_ALL_MASK) == EMOD_SHIFT &&
		(key & FL_KEY_MASK)  <  127 &&
		isalpha(key & FL_KEY_MASK))
	{
        return SString::printf("%c", toupper(key & FL_KEY_MASK));
	}

    return SString::printf("%s%s", ModName_Dash(key),
                           BareKeyName(key & FL_KEY_MASK).c_str());
}


int M_KeyCmp(keycode_t A, keycode_t B)
{
	keycode_t A_mod = A & EMOD_ALL_MASK;
	keycode_t B_mod = B & EMOD_ALL_MASK;

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


KeyContext M_ParseKeyContext(const SString &str)
{
	if (str.noCaseEqual("browser")) return KeyContext::browser;
	if (str.noCaseEqual("render")) return KeyContext::render;
	if (str.noCaseEqual("general")) return KeyContext::general;

	if (str.noCaseEqual("line")) return KeyContext::line;
	if (str.noCaseEqual("sector")) return KeyContext::sector;
	if (str.noCaseEqual("thing")) return KeyContext::thing;
	if (str.noCaseEqual("vertex")) return KeyContext::vertex;

	return KeyContext::none;
}

const char * M_KeyContextString(KeyContext context)
{
	switch (context)
	{
		case KeyContext::browser: return "browser";
		case KeyContext::render:  return "render";
		case KeyContext::general: return "general";

		case KeyContext::line:    return "line";
		case KeyContext::sector:  return "sector";
		case KeyContext::thing:   return "thing";
		case KeyContext::vertex:  return "vertex";

		default: break;
	}

	return "INVALID";
}


//------------------------------------------------------------------------

struct key_binding_t
{
	keycode_t key;

	KeyContext context;

	const editor_command_t *cmd;

	SString param[MAX_EXEC_PARAM];

	// this field ONLY used by M_DetectConflictingBinds()
	bool is_duplicate;
};

namespace global
{
	static std::vector<key_binding_t> all_bindings;
	static std::vector<key_binding_t> install_binds;
}

//
// Helper for the predicates looking for the binding
//
struct KeyBindLookup
{
    const keycode_t key;
    const KeyContext context;

    bool operator()(const key_binding_t &bind)
    {
        return bind.key == key && bind.context == context;
    }
};

bool M_IsKeyBound(keycode_t key, KeyContext context)
{
    return std::any_of(global::all_bindings.begin(), global::all_bindings.end(),
                       KeyBindLookup{key, context});
}


void M_RemoveBinding(keycode_t key, KeyContext context)
{
    auto it = std::remove_if(global::all_bindings.begin(), global::all_bindings.end(),
                             KeyBindLookup{key, context});
    // there should never be more than one
    assert(it == global::all_bindings.end() || it + 1 == global::all_bindings.end());

    global::all_bindings.erase(it, global::all_bindings.end());
}


static void ParseKeyBinding(const std::vector<SString> &tokens)
{
	// this ensures all parameters are NUL terminated
	key_binding_t temp = {};

	assert(tokens.size() >= 2);
	temp.key = M_ParseKeyString(tokens[1]);

	if (! temp.key)
	{
		gLog.printf("bindings.cfg: cannot parse key name: %s\n", tokens[1].c_str());
		return;
	}

	temp.context = M_ParseKeyContext(tokens[0]);

	if (temp.context == KeyContext::none)
	{
		gLog.printf("bindings.cfg: unknown context: %s\n", tokens[0].c_str());
		return;
	}


	// handle un-bound keys
	assert(tokens.size() >= 3);
	if (tokens[2].noCaseEqual("UNBOUND"))
	{
#if 0
		fprintf(stderr, "REMOVED BINDING key:%04x (%s)\n", temp.key, tokens[0].c_str());
#endif
		M_RemoveBinding(temp.key, temp.context);
		return;
	}

	temp.cmd = FindEditorCommand(tokens[2]);

	if (! temp.cmd)
	{
		gLog.printf("bindings.cfg: unknown function: %s\n", tokens[2].c_str());
		return;
	}

	if (temp.cmd->req_context != KeyContext::none &&
	    temp.context != temp.cmd->req_context)
	{
		gLog.printf("bindings.cfg: function '%s' in wrong context '%s'\n",
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

	global::all_bindings.push_back(temp);
}


#define MAX_TOKENS  (MAX_EXEC_PARAM + 8)

static bool LoadBindingsFromPath(const SString &path, bool required)
{
	SString filename = path + "/bindings.cfg";

	std::ifstream fp(filename.c_str());
	if(!fp.is_open())
	{
		if (! required)
			return false;

		ThrowException("Missing key bindings file:\n\n%s\n", filename.c_str());
	}

	gLog.printf("Reading key bindings from: %s\n", filename.c_str());

	while (! fp.eof())
	{
		SString line;
		std::getline(fp, line.get());

		std::vector<SString> tokens;
		int num_tok = M_ParseLine(line, tokens, ParseOptions::haveStringsKeepQuotes);

		if (num_tok == 0)
			continue;

		if (num_tok < 3)
		{
			gLog.printf("Syntax error in bindings: %s\n", line.c_str());
			continue;
		}

		ParseKeyBinding(tokens);
	}

	return true;
}


static void CopyInstallBindings()
{
    global::install_binds = global::all_bindings;
}


static bool BindingExists(std::vector<key_binding_t>& list, const key_binding_t& bind,
                          bool full_match)
{
	for (const key_binding_t &other : list)
	{
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
	global::all_bindings.clear();

	LoadBindingsFromPath(global::install_dir.u8string(), true /* required */);

	// keep a copy of the install_dir bindings
	CopyInstallBindings();

	LoadBindingsFromPath(global::home_dir.u8string(), false);

	if(gInstance->main_win)
		menu::updateBindings(gInstance->main_win->menu_bar);
}


void M_SaveBindings()
{
	SString filename = (global::home_dir / "bindings.cfg").u8string();

	std::ofstream os(filename.get(), std::ios::trunc);
	if (! os.is_open())
	{
		gLog.printf("Failed to save key bindings to: %s\n", filename.c_str());

		DLG_Notify("Warning: failed to save key bindings\n"
		           "(filename: %s)", filename.c_str());
		return;
	}

	gLog.printf("Writing key bindings to: %s\n", filename.c_str());

	os << "# Eureka key bindings (local)\n";
	os << "# vi:ts=16:noexpandtab\n\n";

	for (KeyContext ctx : validKeyContexts)
	{
		int count = 0;

		for (const key_binding_t &bind : global::all_bindings)
		{
			if (bind.context != ctx)
				continue;

			// no need to write it if unchanged from install_dir
			if (BindingExists(global::install_binds, bind, true /* full match */))
				continue;

			os << M_KeyContextString(bind.context) << '\t' << M_KeyToString(bind.key) << '\t' <<
				bind.cmd->name;

			for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
			{
				if (bind.param[p].good())
					os << '\t' << bind.param[p];
			}

			os << '\n';
			count++;
		}

		// find un-bound keys (relative to installation)

		for (unsigned int i = 0 ; i < global::install_binds.size() ; i++)
		{
			const key_binding_t& bind = global::install_binds[i];

			if (bind.context != ctx)
				continue;

			if (! BindingExists(global::all_bindings, bind, false /* full match */))
			{
				os << M_KeyContextString(bind.context) << '\t' << M_KeyToString(bind.key) << '\t' <<
					"UNBOUND" << '\n';
				count++;
			}
		}

		if (count > 0)
			os << '\n';
	}
}


//------------------------------------------------------------------------
//  PREFERENCE DIALOG STUFF
//------------------------------------------------------------------------

namespace global
{
	// local copy of the bindings
	// these only become live after M_ApplyBindings()
	static std::vector<key_binding_t> pref_binds;
}

void M_CopyBindings(bool from_defaults)
{
	// returns # of bindings
    global::pref_binds = from_defaults ? global::install_binds : global::all_bindings;
}


void M_ApplyBindings()
{
    global::all_bindings = global::pref_binds;

	if(gInstance->main_win)
		menu::updateBindings(gInstance->main_win->menu_bar);
}


int M_NumBindings()
{
	return (int)global::pref_binds.size();
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
	std::sort(global::pref_binds.begin(), global::pref_binds.end(),
	          KeyBind_CMP_pred(column));

	if (reverse)
		std::reverse(global::pref_binds.begin(), global::pref_binds.end());
}


void M_DetectConflictingBinds()
{
	// copy the local bindings and sort them.
	// duplicate bindings will be contiguous in the list.

	std::vector<key_binding_t> list;

	for (unsigned int i = 0 ; i < global::pref_binds.size() ; i++)
	{
		global::pref_binds[i].is_duplicate = false;

		list.push_back(global::pref_binds[i]);
	}

	std::sort(list.begin(), list.end(), KeyBind_CMP_pred('c'));

	for (unsigned int k = 0 ; k + 1 < list.size() ; k++)
	{
		if (! (list[k].key     == list[k+1].key &&
			   list[k].context == list[k+1].context))
			continue;

		// mark these in the local bindings

		for (unsigned int n = 0 ; n < global::pref_binds.size() ; n++)
		{
			if (global::pref_binds[n].key     == list[k].key &&
				global::pref_binds[n].context == list[k].context)
			{
				global::pref_binds[n].is_duplicate = true;
			}
		}
	}
}


SString M_StringForFunc(int index)
{
	SString buffer;
	buffer.reserve(2048);

	SYS_ASSERT(index >= 0 && index < static_cast<int>(global::pref_binds.size()));
	const key_binding_t& bind = global::pref_binds[index];

	SYS_ASSERT(!!bind.cmd);
	buffer = bind.cmd->name;

	// add the parameters

	for (int k = 0 ; k < MAX_EXEC_PARAM ; k++)
	{
		const SString &param = bind.param[k];

		if (param.empty())
			break;

		if (k == 0)
			buffer.push_back(':');

		buffer.push_back(' ');
		buffer += SString::printf("%.30s", param.c_str());
	}

	return buffer;
}


const char * M_StringForBinding(int index, bool changing_key)
{
	SYS_ASSERT(index < (int)global::pref_binds.size());

	const key_binding_t& bind = global::pref_binds[index];

	static char buffer[600];

	// we prefer the UI to say "3D view" instead of "render"
	const char *ctx_name = M_KeyContextString(bind.context);
	if (y_stricmp(ctx_name, "render") == 0)
		ctx_name = "3D view";

	// display SHIFT + letter as an uppercase letter
	keycode_t tempk = bind.key;
	if ((tempk & EMOD_ALL_MASK) == EMOD_SHIFT &&
		(tempk & FL_KEY_MASK)  <  127 &&
		isalpha(tempk & FL_KEY_MASK))
	{
		tempk = toupper(tempk & FL_KEY_MASK);
	}

	snprintf(buffer, sizeof(buffer), "%s%6.6s%-10.10s %-9.9s %.32s",
			bind.is_duplicate ? "@C1" : "",
			changing_key ? "<?"     : ModName_Space(tempk),
			changing_key ? "\077?>" : BareKeyName(tempk & FL_KEY_MASK).c_str(),
			ctx_name,
			 M_StringForFunc(index).c_str() );

	return buffer;
}


void M_GetBindingInfo(int index, keycode_t *key, KeyContext *context)
{
	// hmmm... exposing key_binding_t may have been easier...

	*key     = global::pref_binds[index].key;
	*context = global::pref_binds[index].context;
}


void M_ChangeBindingKey(int index, keycode_t key)
{
	SYS_ASSERT(0 <= index && index < (int)global::pref_binds.size());
	SYS_ASSERT(key != 0);

	global::pref_binds[index].key = key;
}


static const char * DoParseBindingFunc(key_binding_t& bind, const SString & func_str)
{
	static char error_msg[1024];

	// convert the brackets and commas into spaces and use the
	// line tokeniser.

	SString buffer = func_str;

	for (char &c : buffer)
		if (c == ',' || c == ':')
			c = ' ';

	std::vector<SString> tokens;

	int num_tok = M_ParseLine(buffer, tokens, ParseOptions::haveStringsKeepQuotes);

	if (num_tok <= 0)
		return "Missing function name";

	const editor_command_t * cmd = FindEditorCommand(tokens[0]);

	if (! cmd)
	{
		snprintf(error_msg, sizeof(error_msg), "Unknown function name: %s", tokens[0].c_str());
		return error_msg;
	}

	// check context is suitable

	if (cmd->req_context != KeyContext::none &&
	    bind.context != cmd->req_context)
	{
		SString mode = SString(M_KeyContextString(cmd->req_context)).asUpper();

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
const char * M_SetLocalBinding(int index, keycode_t key, KeyContext context,
							   const SString &func_str)
{
	SYS_ASSERT(0 <= index && index < (int)global::pref_binds.size());

	global::pref_binds[index].key = key;
	global::pref_binds[index].context = context;

	const char *err_msg = DoParseBindingFunc(global::pref_binds[index], func_str);

	return err_msg;
}


const char * M_AddLocalBinding(int after, keycode_t key, KeyContext context,
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
		global::pref_binds.push_back(temp);
	else
		global::pref_binds.insert(global::pref_binds.begin() + (1 + after), temp);

	return NULL;  // OK
}


void M_DeleteLocalBinding(int index)
{
	SYS_ASSERT(0 <= index && index < (int)global::pref_binds.size());

	global::pref_binds.erase(global::pref_binds.begin() + index);
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

	     if (state & EMOD_COMMAND) key |= EMOD_COMMAND;
	else if (state & EMOD_META)    key |= EMOD_META;
	else if (state & EMOD_ALT)     key |= EMOD_ALT;
	else if (state & EMOD_SHIFT)   key |= EMOD_SHIFT;

	return key;
}


int M_KeyToShortcut(keycode_t key)
{
	int shortcut = key & FL_KEY_MASK;

	     if (key & EMOD_COMMAND) shortcut |= EMOD_COMMAND;
	else if (key & EMOD_ALT)     shortcut |= EMOD_ALT;
	else if (key & EMOD_SHIFT)   shortcut |= EMOD_SHIFT;

	return shortcut;
}


KeyContext M_ModeToKeyContext(ObjType mode)
{
	switch (mode)
	{
		case ObjType::things:   return KeyContext::thing;
		case ObjType::linedefs: return KeyContext::line;
		case ObjType::sectors:  return KeyContext::sector;
		case ObjType::vertices: return KeyContext::vertex;

		default: break;
	}

	return KeyContext::none;  /* shouldn't happen */
}


bool Instance::Exec_HasFlag(const char *flag) const
{
	// the parameter should include a leading '/'

	for (int i = 0 ; i < MAX_EXEC_PARAM ; i++)
	{
		if (EXEC_Flags[i].empty())
			break;

		if (EXEC_Flags[i].noCaseEqual(flag))
			return true;
	}

	return false;
}


extern void Debug_CheckUnusedStuff(Document &doc);


void Instance::DoExecuteCommand(const editor_command_t *cmd)
{
	(this->*cmd->func)();

//	Debug_CheckUnusedStuff();
}


static const key_binding_t *FindBinding(keycode_t key, KeyContext context, bool lax_only)
{
	for (const key_binding_t &bind : global::all_bindings)
	{
		SYS_ASSERT(bind.cmd);

		if (bind.context != context)
			continue;

		// match modifiers "loosely" for certain commands (esp. NAV_xxx)
		bool is_lax = !!(bind.key & MOD_LAX_SHIFTCTRL);

		if (lax_only != is_lax)
			continue;

		keycode_t key1 = key;
		keycode_t key2 = bind.key;

		if (is_lax)
		{
			key1 &= ~ (EMOD_SHIFT | EMOD_COMMAND | MOD_LAX_SHIFTCTRL);
			key2 &= ~ (EMOD_SHIFT | EMOD_COMMAND | MOD_LAX_SHIFTCTRL);
		}

		if (key1 != key2)
			continue;

		// found a match
		return &bind;
	}

	// not found
	return nullptr;
}


bool Instance::ExecuteKey(keycode_t key, KeyContext context)
{
	for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
	{
		EXEC_Param[p].clear();
		EXEC_Flags[p].clear();
	}

	EXEC_Errno = 0;

	// this logic means we only use a LAX binding if an exact match
	// could not be found.
    const key_binding_t *bind = FindBinding(key, context, false);

	if (!bind)
		bind = FindBinding(key, context, true);

	if (!bind)
		return false;

	int p_idx = 0;
	int f_idx = 0;

	for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
	{
		if (! bind->param[p][0])
			break;

		// separate flags from normal parameters
		if (bind->param[p][0] == '/')
			EXEC_Flags[f_idx++] = bind->param[p];
		else
			EXEC_Param[p_idx++] = bind->param[p];
	}

	EXEC_CurKey = key;

	// commands can test for LAX mode via a special flag
	bool is_lax = (bind->key & MOD_LAX_SHIFTCTRL) ? true : false;

	if (is_lax && f_idx < MAX_EXEC_PARAM)
	{
		EXEC_Flags[f_idx++] = "/LAX";
	}

	DoExecuteCommand(bind->cmd);

	return true;
}


bool Instance::ExecuteCommand(const editor_command_t *cmd,
					const SString &param1, const SString &param2,
                    const SString &param3, const SString &param4)
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


bool Instance::ExecuteCommand(const SString &name,
					const SString &param1, const SString &param2,
					const SString &param3, const SString &param4)
{
	const editor_command_t * cmd = FindEditorCommand(name);

	if (! cmd)
		return false;

	return ExecuteCommand(cmd, param1, param2, param3, param4);
}


//
//  play a fascinating tune
//
void Instance::Beep(EUR_FORMAT_STRING(const char *fmt), ...)
{
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	SString text = SString::vprintf(fmt, arg_ptr);
	va_end(arg_ptr);

	Status_Set("%s", text.c_str());
	gLog.printf("BEEP: %s\n", text.c_str());

	fl_beep();

	EXEC_Errno = 1;
}

//
// True if the given shortcut can really show up on menu, instead of garbage
//
static bool canShowUpOnMenu(keycode_t code)
{
    code &= FL_KEY_MASK;
    // All plats
    switch(code)
    {
        case FL_Iso_Key:
        case FL_WheelUp:
        case FL_WheelDown:
        case FL_WheelLeft:
        case FL_WheelRight:
            return false;
    }
    // Try and pick as many buttons
    if(code >= FL_Button && code <= FL_Button + 9)
        return false;
    // For the macOS menu, don't show any fake unicode
#ifdef __APPLE__
    switch(code)
    {
        case FL_Button:
        case FL_Iso_Key:
        case FL_Pause:
        case FL_Scroll_Lock:
        case FL_Kana:
        case FL_Eisu:
        case FL_Yen:
        case FL_JIS_Underscore:
        case FL_Print:
        case FL_Insert:
        case FL_Menu:
        case FL_Help:
        case FL_Num_Lock:
        case FL_Shift_L:
        case FL_Shift_R:
        case FL_Control_L:
        case FL_Control_R:
        case FL_Caps_Lock:
        case FL_Meta_L:
        case FL_Meta_R:
        case FL_Alt_L:
        case FL_Alt_R:
        case FL_Volume_Down:
        case FL_Volume_Mute:
        case FL_Volume_Up:
        case FL_Media_Play:
        case FL_Media_Stop:
        case FL_Media_Prev:
        case FL_Media_Next:
        case FL_Home_Page:
        case FL_Mail:
        case FL_Search:
        case FL_Back:
        case FL_Forward:
        case FL_Stop:
        case FL_Refresh:
        case FL_Sleep:
        case FL_Favorites:
            return false;
    }
    if(code >= FL_KP && code <= FL_KP_Last)
        return false;
#endif
    return true;
}

//
// Finds key code for given command name
//
bool findKeyCodeForCommandName(const char *command, const char *params[MAX_EXEC_PARAM], 
							   keycode_t *code)
{
    assert(!!code);
    *code = UINT_MAX;
    for(const key_binding_t &binding : global::all_bindings)
	{
		if(y_stricmp(binding.cmd->name, command))
			continue;
		
		bool skip = false;
		for(int i = 0; i < MAX_EXEC_PARAM; ++i)
		{
			if(CSTRING_EMPTY(params[i]) ^ binding.param[i].empty())
			{
				skip = true;
				break;
			}
			if(CSTRING_EMPTY(params[i]) && binding.param[i].empty())
				break;
			if(!binding.param[i].noCaseEqual(params[i]))
			{
				skip = true;
				break;
			}
		}
		if(skip)
			continue;
        if(canShowUpOnMenu(binding.key) && binding.key < *code)
            *code = binding.key;
        // Continue looking until we find the lowest numbered key code. That is the smallest ASCII
        // character with no shortcut keys
	}
	return *code < UINT_MAX;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
