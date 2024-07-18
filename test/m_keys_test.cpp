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
	ASSERT_EQ(M_KeyToString(key), "K");
	
	key = EMOD_COMMAND | 'j';
#ifdef __APPLE__
	ASSERT_EQ(M_KeyToString(key), "CMD-j");
#else
	ASSERT_EQ(M_KeyToString(key), "CTRL-j");
#endif
	
	key = EMOD_META | 'm';
	ASSERT_EQ(M_KeyToString(key), "META-m");
	
	key = EMOD_ALT | 'a';
	ASSERT_EQ(M_KeyToString(key), "ALT-a");
	
	key = EMOD_ALT | (FL_Button + 3);
	ASSERT_EQ(M_KeyToString(key), "ALT-MOUSE3");
	
	key = EMOD_ALT | FL_Volume_Down;
	ASSERT_EQ(M_KeyToString(key), "ALT-VOL_DOWN");
	
	key = EMOD_SHIFT | '5';
	ASSERT_EQ(M_KeyToString(key), "SHIFT-5");
	
	key = EMOD_SHIFT | '"';
	ASSERT_EQ(M_KeyToString(key), "SHIFT-DBLQUOTE");
	
	key = EMOD_SHIFT | (FL_F + 4);
	ASSERT_EQ(M_KeyToString(key), "SHIFT-F4");
	
	key = FL_SCROLL_LOCK | 's';
	ASSERT_EQ(M_KeyToString(key), "LAX-s");
}
