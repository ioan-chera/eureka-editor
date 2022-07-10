//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
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

#include "m_strings.h"

#include "gtest/gtest.h"

TEST(StringID, Test)
{
	StringID sid;
	ASSERT_EQ(sid.get(), 0);
	ASSERT_TRUE(!sid);
	ASSERT_TRUE(sid.isValid());
	ASSERT_FALSE(sid.isInvalid());
	ASSERT_FALSE(sid.hasContent());

	StringID sid2;
	ASSERT_EQ(sid, sid2);

	sid = StringID(100);
	ASSERT_NE(sid, sid2);

	ASSERT_EQ(sid.get(), 100);
	ASSERT_FALSE(!sid);
	ASSERT_TRUE(sid.isValid());
	ASSERT_FALSE(sid.isInvalid());
	ASSERT_TRUE(sid.hasContent());

	sid2 = sid;
	ASSERT_EQ(sid, sid2);

	sid = StringID(30);
	sid2 = StringID(30);
	ASSERT_EQ(sid, sid2);

	sid = StringID(-31);
	sid2 = StringID(-32);
	ASSERT_NE(sid, sid2);
	ASSERT_LT(sid.get(), 0);
	ASSERT_LT(sid2.get(), 0);
	ASSERT_FALSE(!sid);
	ASSERT_FALSE(sid.isValid());
	ASSERT_TRUE(sid.isInvalid());
	ASSERT_FALSE(sid.hasContent());
}

TEST(StringTable, Test)
{
    StringTable table;
    StringID index = table.add("Jackson");
    ASSERT_EQ(table.get(index), "Jackson");
	StringID index2 = table.add("Jackson");
    ASSERT_EQ(index2, index);
	StringID index3 = table.add("Michael");
    ASSERT_NE(index3, index2);
    ASSERT_EQ(table.get(index3), "Michael");

	StringID index4 = table.add("jackson");
    ASSERT_NE(index4, index);
    ASSERT_EQ(table.get(index), "Jackson");
    ASSERT_EQ(table.get(index4), "jackson");
}
