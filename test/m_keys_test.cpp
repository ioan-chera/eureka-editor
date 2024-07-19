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

TEST(MKeys, KeyToString)
{
	keycode_t key = EMOD_SHIFT | 'k';
	ASSERT_EQ(keys::toString(key), "K");
	
	key = EMOD_COMMAND | 'j';
#ifdef __APPLE__
	ASSERT_EQ(keys::toString(key), "CMD-j");
#else
	ASSERT_EQ(keys::toString(key), "CTRL-j");
#endif
	
	key = EMOD_META | 'm';
	ASSERT_EQ(keys::toString(key), "META-m");
	
	key = EMOD_ALT | 'a';
	ASSERT_EQ(keys::toString(key), "ALT-a");
	
	key = EMOD_ALT | (FL_Button + 3);
	ASSERT_EQ(keys::toString(key), "ALT-MOUSE3");
	
	key = EMOD_ALT | FL_Volume_Down;
	ASSERT_EQ(keys::toString(key), "ALT-VOL_DOWN");
	
	key = EMOD_SHIFT | '5';
	ASSERT_EQ(keys::toString(key), "SHIFT-5");
	
	key = EMOD_SHIFT | '"';
	ASSERT_EQ(keys::toString(key), "SHIFT-DBLQUOTE");
	
	key = EMOD_SHIFT | (FL_F + 4);
	ASSERT_EQ(keys::toString(key), "SHIFT-F4");
	
	key = FL_SCROLL_LOCK | 's';
	ASSERT_EQ(keys::toString(key), "LAX-s");
}

TEST(MKeys, StringForFunc)
{
	key_binding_t bind = {};
	editor_command_t command = {"CommandName"};
	bind.cmd = &command;

	
	// No params
	ASSERT_EQ(M_StringForFunc(bind), "CommandName");
	
	bind.param[0] = "parm1";
	
	ASSERT_EQ(M_StringForFunc(bind), "CommandName: parm1");
	
	bind.param[1] = "other";
	
	ASSERT_EQ(M_StringForFunc(bind), "CommandName: parm1 other");
	
	bind.param[2] = "thing";
	
	ASSERT_EQ(M_StringForFunc(bind), "CommandName: parm1 other thing");
	
	bind.param[1].clear();
	
	ASSERT_EQ(M_StringForFunc(bind), "CommandName: parm1");
	
	bind.param[1] = "aaaaabbbbbcccccdddddeeeeefffffggggg";
	
	ASSERT_EQ(M_StringForFunc(bind), "CommandName: parm1 aaaaabbbbbcccccdddddeeeeefffff thing");
}
