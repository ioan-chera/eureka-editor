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

TEST(MSelect, CountObj)
{
	selection_c selection(ObjType::things);
	selection.set(2);
	selection.set(3);
	selection.set(5);
	ASSERT_EQ(selection.count_obj(), 3);
	ASSERT_FALSE(selection.empty());
	ASSERT_TRUE(selection.notempty());
}

TEST(MSelect, ChangingTypeClearsContent)
{
	selection_c selection(ObjType::things);
	selection.set(2);
	selection.set(3);
	selection.set(5);
	selection.change_type(ObjType::vertices);
	ASSERT_EQ(selection.count_obj(), 0);
	ASSERT_TRUE(selection.empty());
	ASSERT_FALSE(selection.notempty());
}

TEST(MSelect, ClearAll)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);
	selection.clear_all();
	ASSERT_EQ(selection.count_obj(), 0);
	ASSERT_TRUE(selection.empty());
	ASSERT_FALSE(selection.notempty());
	selection.set(4);
	ASSERT_EQ(selection.count_obj(), 1);
	ASSERT_FALSE(selection.empty());
	ASSERT_TRUE(selection.notempty());
}

TEST(MSelect, MaxObj)
{
	selection_c selection;
	selection.set(2);
	selection.set(5);
	selection.set(3);
	ASSERT_EQ(selection.max_obj(), 5);
}

TEST(MSelect, Get)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);
	ASSERT_FALSE(selection.get(0));
	ASSERT_FALSE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_TRUE(selection.get(3));
	ASSERT_FALSE(selection.get(4));
	ASSERT_TRUE(selection.get(5));
	ASSERT_FALSE(selection.get(6));	// going right is fine
}

TEST(MSelect, ClearOne)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);
	selection.clear(3);
	selection.clear(4);	// clearing unset is fine
	ASSERT_FALSE(selection.get(0));
	ASSERT_FALSE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_FALSE(selection.get(3));
	ASSERT_FALSE(selection.get(4));
	ASSERT_TRUE(selection.get(5));
}

TEST(MSelect, MaxObjGetsUpdated)
{
	selection_c selection;
	selection.set(2);
	selection.set(5);
	selection.set(3);
	selection.clear(5);
	ASSERT_EQ(selection.max_obj(), 3);
	selection.set(7);
	ASSERT_EQ(selection.max_obj(), 7);
}

TEST(MSelect, Toggle)
{
	selection_c selection;
	selection.set(2);
	selection.set(3);
	selection.set(5);
	selection.toggle(3);
	selection.toggle(4);	// clearing unset is fine
	ASSERT_FALSE(selection.get(0));
	ASSERT_FALSE(selection.get(1));
	ASSERT_TRUE(selection.get(2));
	ASSERT_FALSE(selection.get(3));
	ASSERT_TRUE(selection.get(4));
	ASSERT_TRUE(selection.get(5));
}

// TODO: extended lists, frob, set operations, finding 1st and 2nd, iterators
// TODO: MAX_STORE_SEL stability
