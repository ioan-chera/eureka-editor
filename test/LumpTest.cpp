//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2021 Ioan Chera
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

#include "Lump.h"
#include "m_strings.h"

#include "gtest/gtest.h"

TEST(Lump, General)
{
	Lump lump;

	ASSERT_STREQ(lump.getName(), "");
	ASSERT_STREQ(lump.getDataAsString(), "");

	lump.setName("abcde");
	ASSERT_STREQ(lump.getName(), "ABCDE");

	lump.setName("abcdefghhij");
	ASSERT_STREQ(lump.getName(), "ABCDEFGH");

	lump.setName("");
	ASSERT_STREQ(lump.getName(), "");

	lump.setData({ '1', '2', '3', '4', '5', '6', '7', '8', '9'});
	ASSERT_STREQ(lump.getDataAsString(), "123456789");

	lump.setData({ '1', '2', '3', '4', '5', '6', 0, '8', '9'});
	ASSERT_STREQ(lump.getDataAsString(), "123456");
}
