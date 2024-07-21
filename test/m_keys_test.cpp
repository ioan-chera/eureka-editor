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
	{EMOD_COMMAND | 'j',
#ifdef __APPLE__
		"CMD-j",                                "  CMD j         "
#else
		"CTRL-j",                               " CTRL j         "
#endif
	},
	{EMOD_META | 'm',
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
