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

TEST(SString, Test)
{
    SString nullString(nullptr);
    ASSERT_EQ(nullString, "");
    ASSERT_FALSE(nullString);
    ASSERT_TRUE(nullString.empty());

    SString aString("str");
    ASSERT_EQ(aString, "str");
    ASSERT_STREQ(aString.c_str(), "str");
    ASSERT_EQ(aString.length(), 3);
    ASSERT_TRUE(aString);
    ASSERT_FALSE(aString.empty());

    char sizedBuffer[8] = "Flat";
    SString trimmedString(sizedBuffer, (int)sizeof(sizedBuffer));
    ASSERT_EQ(trimmedString, "Flat");
    ASSERT_EQ(trimmedString.length(), 4);

    char fullBuffer[4] = {'P', 'W', 'A', 'D'};
    SString goodString(fullBuffer, 4);
    ASSERT_EQ(goodString, "PWAD");
    ASSERT_EQ(goodString.length(), 4);

    SString nullStringSized(nullptr, 4);
    ASSERT_FALSE(nullStringSized);
}
