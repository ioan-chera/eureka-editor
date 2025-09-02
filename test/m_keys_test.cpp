//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2024 Ioan Chera
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
#include "m_keys.h"
#include "gtest/gtest.h"

struct KeyMapping
{
	keycode_t code;
	const char *dashString;
	const char *spaceString;
};

// NOTE: keep the old names in the dashed names for backward compatibility
static const KeyMapping testKeyCombos[] =
{
	{EMOD_SHIFT | 'k', "K",                     "      K         "},
	{static_cast<keycode_t>(EMOD_COMMAND) | 'j',
#ifdef __APPLE__
		"CMD-j",                                "  CMD j         "
#else
		"CTRL-j",                               " CTRL j         "
#endif
	},
	{static_cast<keycode_t>(EMOD_META) | 'm',
#ifdef __APPLE__
		"META-m",                 " CTRL m         "
#else
		"META-m",                 " META m         "
#endif
	},
	{EMOD_ALT | 'a', "ALT-a",                   "  ALT a         "},
	{EMOD_ALT | (FL_Button + 3), "ALT-MOUSE3",  "  ALT MOUSE3    "},
	{EMOD_ALT | FL_Volume_Down, "ALT-VOL_DOWN", "  ALT VOL_DOWN  "},
	{EMOD_SHIFT | '5', "SHIFT-5",               "SHIFT 5         "},
	{EMOD_SHIFT | '"', "SHIFT-DBLQUOTE",        "SHIFT DBLQUOTE  "},
	{EMOD_SHIFT | (FL_F + 4), "SHIFT-F4",       "SHIFT F4        "},
	{FL_SCROLL_LOCK | 's', "LAX-s",             "  LAX s         "},

};

// TODO: use this array in tests

TEST(MKeys, KeyToString)
{
	for(const KeyMapping &mapping : testKeyCombos)
	{
		keycode_t key = mapping.code;
		ASSERT_EQ(keys::toString(key), mapping.dashString);
	}
}

TEST(MKeys, StringForFunc)
{
	key_binding_t bind = {};
	editor_command_t command = {"CommandName"};
	bind.cmd = &command;

	
	// No params
	ASSERT_EQ(keys::stringForFunc(bind), "CommandName");
	
	bind.param[0] = "parm1";
	
	ASSERT_EQ(keys::stringForFunc(bind), "CommandName: parm1");
	
	bind.param[1] = "other";
	
	ASSERT_EQ(keys::stringForFunc(bind), "CommandName: parm1 other");
	
	bind.param[2] = "thing";
	
	ASSERT_EQ(keys::stringForFunc(bind), "CommandName: parm1 other thing");
	
	bind.param[1].clear();
	
	ASSERT_EQ(keys::stringForFunc(bind), "CommandName: parm1");
	
	bind.param[1] = "aaaaabbbbbcccccdddddeeeeefffffggggg";
	
	ASSERT_EQ(keys::stringForFunc(bind), "CommandName: parm1 aaaaabbbbbcccccdddddeeeeefffff thing");
}

TEST(MKeys, StringForBindingCheckModName)
{
	key_binding_t bind = {};
	editor_command_t command = {"CommandName"};
	bind.cmd = &command;
	
	bind.context = KeyContext::browser;
	
	for(const KeyMapping &mapping : testKeyCombos)
	{
		bind.key = mapping.code;
		const char *string = keys::stringForBinding(bind);
		
		SString expected = SString(mapping.spaceString) + " browser   CommandName";
		ASSERT_EQ(expected, string);
	}
}

TEST(MKeys, ParseKeyString)
{
	ASSERT_EQ(M_ParseKeyString("a"), static_cast<keycode_t>('a'));
	ASSERT_EQ(M_ParseKeyString("z"), static_cast<keycode_t>('z'));
	ASSERT_EQ(M_ParseKeyString("0"), static_cast<keycode_t>('0'));
	ASSERT_EQ(M_ParseKeyString("9"), static_cast<keycode_t>('9'));
	ASSERT_EQ(M_ParseKeyString("!"), static_cast<keycode_t>('!'));
	ASSERT_EQ(M_ParseKeyString(";"), static_cast<keycode_t>(';'));
	
	ASSERT_EQ(M_ParseKeyString("A"), static_cast<keycode_t>(EMOD_SHIFT | 'a'));
	ASSERT_EQ(M_ParseKeyString("Z"), static_cast<keycode_t>(EMOD_SHIFT | 'z'));
	
	ASSERT_EQ(M_ParseKeyString("CMD-a"), static_cast<keycode_t>(EMOD_COMMAND | 'a'));
	ASSERT_EQ(M_ParseKeyString("CTRL-b"), static_cast<keycode_t>(EMOD_COMMAND | 'b'));
	ASSERT_EQ(M_ParseKeyString("META-c"), static_cast<keycode_t>(EMOD_META | 'c'));
	ASSERT_EQ(M_ParseKeyString("ALT-d"), static_cast<keycode_t>(EMOD_ALT | 'd'));
	ASSERT_EQ(M_ParseKeyString("SHIFT-e"), static_cast<keycode_t>(EMOD_SHIFT | 'e'));
	ASSERT_EQ(M_ParseKeyString("LAX-f"), static_cast<keycode_t>(FL_SCROLL_LOCK | 'f'));
	
	ASSERT_EQ(M_ParseKeyString("cmd-a"), static_cast<keycode_t>(EMOD_COMMAND | 'a'));
	ASSERT_EQ(M_ParseKeyString("Ctrl-B"), static_cast<keycode_t>(EMOD_COMMAND | EMOD_SHIFT | 'b'));
	ASSERT_EQ(M_ParseKeyString("meta-C"), static_cast<keycode_t>(EMOD_META | EMOD_SHIFT | 'c'));
	ASSERT_EQ(M_ParseKeyString("Alt-D"), static_cast<keycode_t>(EMOD_ALT | EMOD_SHIFT | 'd'));
	ASSERT_EQ(M_ParseKeyString("shift-E"), static_cast<keycode_t>(EMOD_SHIFT | EMOD_SHIFT | 'e'));
	ASSERT_EQ(M_ParseKeyString("lax-F"), static_cast<keycode_t>(FL_SCROLL_LOCK | EMOD_SHIFT | 'f'));
	
	ASSERT_EQ(M_ParseKeyString("CMD-ALT-g"), static_cast<keycode_t>(EMOD_COMMAND | EMOD_ALT | 'g'));
	ASSERT_EQ(M_ParseKeyString("SHIFT-CTRL-h"), static_cast<keycode_t>(EMOD_SHIFT | EMOD_COMMAND | 'h'));
	ASSERT_EQ(M_ParseKeyString("META-SHIFT-i"), static_cast<keycode_t>(EMOD_META | EMOD_SHIFT | 'i'));
	ASSERT_EQ(M_ParseKeyString("ALT-SHIFT-LAX-j"), static_cast<keycode_t>(EMOD_ALT | EMOD_SHIFT | FL_SCROLL_LOCK | 'j'));
	
	ASSERT_EQ(M_ParseKeyString("F1"), static_cast<keycode_t>(FL_F + 1));
	ASSERT_EQ(M_ParseKeyString("F12"), static_cast<keycode_t>(FL_F + 12));
	ASSERT_EQ(M_ParseKeyString("f3"), static_cast<keycode_t>(FL_F + 3));
	ASSERT_EQ(M_ParseKeyString("SHIFT-F5"), static_cast<keycode_t>(EMOD_SHIFT | FL_F + 5));
	ASSERT_EQ(M_ParseKeyString("CMD-f10"), static_cast<keycode_t>(EMOD_COMMAND | FL_F + 10));
	
	ASSERT_EQ(M_ParseKeyString("MOUSE1"), static_cast<keycode_t>(FL_Button + 1));
	ASSERT_EQ(M_ParseKeyString("MOUSE3"), static_cast<keycode_t>(FL_Button + 3));
	ASSERT_EQ(M_ParseKeyString("mouse5"), static_cast<keycode_t>(FL_Button + 5));
	ASSERT_EQ(M_ParseKeyString("ALT-MOUSE2"), static_cast<keycode_t>(EMOD_ALT | FL_Button + 2));
	
	ASSERT_EQ(M_ParseKeyString("SPACE"), static_cast<keycode_t>(' '));
	ASSERT_EQ(M_ParseKeyString("space"), static_cast<keycode_t>(' '));
	ASSERT_EQ(M_ParseKeyString("SPC"), static_cast<keycode_t>(' '));
	ASSERT_EQ(M_ParseKeyString("DBLQUOTE"), static_cast<keycode_t>('"'));
	ASSERT_EQ(M_ParseKeyString("BS"), static_cast<keycode_t>(FL_BackSpace));
	ASSERT_EQ(M_ParseKeyString("TAB"), static_cast<keycode_t>(FL_Tab));
	ASSERT_EQ(M_ParseKeyString("tab"), static_cast<keycode_t>(FL_Tab));
	ASSERT_EQ(M_ParseKeyString("ENTER"), static_cast<keycode_t>(FL_Enter));
	ASSERT_EQ(M_ParseKeyString("ESC"), static_cast<keycode_t>(FL_Escape));
	ASSERT_EQ(M_ParseKeyString("LEFT"), static_cast<keycode_t>(FL_Left));
	ASSERT_EQ(M_ParseKeyString("UP"), static_cast<keycode_t>(FL_Up));
	ASSERT_EQ(M_ParseKeyString("RIGHT"), static_cast<keycode_t>(FL_Right));
	ASSERT_EQ(M_ParseKeyString("DOWN"), static_cast<keycode_t>(FL_Down));
	ASSERT_EQ(M_ParseKeyString("PGUP"), static_cast<keycode_t>(FL_Page_Up));
	ASSERT_EQ(M_ParseKeyString("PGDN"), static_cast<keycode_t>(FL_Page_Down));
	ASSERT_EQ(M_ParseKeyString("HOME"), static_cast<keycode_t>(FL_Home));
	ASSERT_EQ(M_ParseKeyString("END"), static_cast<keycode_t>(FL_End));
	ASSERT_EQ(M_ParseKeyString("INS"), static_cast<keycode_t>(FL_Insert));
	ASSERT_EQ(M_ParseKeyString("DEL"), static_cast<keycode_t>(FL_Delete));
	ASSERT_EQ(M_ParseKeyString("VOL_DOWN"), static_cast<keycode_t>(FL_Volume_Down));
	ASSERT_EQ(M_ParseKeyString("WHEEL_UP"), static_cast<keycode_t>(0xEF91));
	ASSERT_EQ(M_ParseKeyString("WHEEL_DOWN"), static_cast<keycode_t>(0xEF92));
	
	ASSERT_EQ(M_ParseKeyString("KP_5"), static_cast<keycode_t>(FL_KP + '5'));
	ASSERT_EQ(M_ParseKeyString("kp_9"), static_cast<keycode_t>(FL_KP + '9'));
	ASSERT_EQ(M_ParseKeyString("KP_Enter"), static_cast<keycode_t>(FL_KP_Enter));
	
	ASSERT_EQ(M_ParseKeyString("0x41"), static_cast<keycode_t>(0x41));
	ASSERT_EQ(M_ParseKeyString("0xFF"), static_cast<keycode_t>(0xFF));
	ASSERT_EQ(M_ParseKeyString("0x1234"), static_cast<keycode_t>(0x1234));
	ASSERT_EQ(M_ParseKeyString("0x0"), static_cast<keycode_t>(0x0));
	ASSERT_EQ(M_ParseKeyString("0xabcd"), static_cast<keycode_t>(0xabcd));
	
	ASSERT_EQ(M_ParseKeyString(""), 0);
	ASSERT_EQ(M_ParseKeyString("invalid"), 0);
	ASSERT_EQ(M_ParseKeyString("NONEXISTENT"), 0);
	ASSERT_EQ(M_ParseKeyString("F"), static_cast<keycode_t>(EMOD_SHIFT | 'f'));
	ASSERT_EQ(M_ParseKeyString("MOUSE"), 0);
	ASSERT_EQ(M_ParseKeyString("KP_"), 0);
	ASSERT_EQ(M_ParseKeyString("0x"), 0);
	ASSERT_EQ(M_ParseKeyString("CMD-"), 0);
	ASSERT_EQ(M_ParseKeyString("SHIFT-"), 0);
	
	ASSERT_EQ(M_ParseKeyString("~"), static_cast<keycode_t>('~'));
	ASSERT_EQ(M_ParseKeyString("@"), static_cast<keycode_t>('@'));
	ASSERT_EQ(M_ParseKeyString("#"), static_cast<keycode_t>('#'));
	ASSERT_EQ(M_ParseKeyString("$"), static_cast<keycode_t>('$'));
	ASSERT_EQ(M_ParseKeyString("%"), static_cast<keycode_t>('%'));
	ASSERT_EQ(M_ParseKeyString("^"), static_cast<keycode_t>('^'));
	ASSERT_EQ(M_ParseKeyString("&"), static_cast<keycode_t>('&'));
	ASSERT_EQ(M_ParseKeyString("*"), static_cast<keycode_t>('*'));
	ASSERT_EQ(M_ParseKeyString("("), static_cast<keycode_t>('('));
	ASSERT_EQ(M_ParseKeyString(")"), static_cast<keycode_t>(')'));
	ASSERT_EQ(M_ParseKeyString("-"), static_cast<keycode_t>('-'));
	ASSERT_EQ(M_ParseKeyString("CMD--"), static_cast<keycode_t>(EMOD_COMMAND | '-'));
	ASSERT_EQ(M_ParseKeyString("ALT-SHIFT-META-CMD--"), static_cast<keycode_t>(EMOD_COMMAND | EMOD_ALT | EMOD_META | EMOD_SHIFT | '-'));
	ASSERT_EQ(M_ParseKeyString("_"), static_cast<keycode_t>('_'));
	ASSERT_EQ(M_ParseKeyString("="), static_cast<keycode_t>('='));
	ASSERT_EQ(M_ParseKeyString("+"), static_cast<keycode_t>('+'));
	ASSERT_EQ(M_ParseKeyString("["), static_cast<keycode_t>('['));
	ASSERT_EQ(M_ParseKeyString("]"), static_cast<keycode_t>(']'));
	ASSERT_EQ(M_ParseKeyString("\\"), static_cast<keycode_t>('\\'));
	ASSERT_EQ(M_ParseKeyString("|"), static_cast<keycode_t>('|'));
	ASSERT_EQ(M_ParseKeyString(","), static_cast<keycode_t>(','));
	ASSERT_EQ(M_ParseKeyString("."), static_cast<keycode_t>('.'));
	ASSERT_EQ(M_ParseKeyString("/"), static_cast<keycode_t>('/'));
	ASSERT_EQ(M_ParseKeyString("?"), static_cast<keycode_t>('?'));
	ASSERT_EQ(M_ParseKeyString("'"), static_cast<keycode_t>('\''));
	ASSERT_EQ(M_ParseKeyString("`"), static_cast<keycode_t>('`'));
}
