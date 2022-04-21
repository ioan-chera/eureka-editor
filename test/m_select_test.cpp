//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022 Ioan Chera
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

#include "m_select.h"
#include "gtest/gtest.h"

TEST(MSelect, ChangeType)
{
	selection_c selection(ObjType::things);
	ASSERT_EQ(selection.what_type(), ObjType::things);

	selection.change_type(ObjType::sectors);
	ASSERT_EQ(selection.what_type(), ObjType::sectors);
}

TEST(MSelect, InitiallyEmpty)
{
	selection_c selection;
	ASSERT_TRUE(selection.empty());
	ASSERT_FALSE(selection.notempty());
	ASSERT_EQ(selection.count_obj(), 0);
	ASSERT_EQ(selection.max_obj(), -1);
	ASSERT_EQ(selection.find_first(), -1);
	ASSERT_EQ(selection.find_second(), -1);
}
