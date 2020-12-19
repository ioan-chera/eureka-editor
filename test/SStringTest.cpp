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
    ASSERT_TRUE(nullString.empty());

    SString aString("str");
    ASSERT_EQ(aString, "str");
    ASSERT_STREQ(aString.c_str(), "str");
    ASSERT_EQ(aString.length(), 3);
    ASSERT_TRUE(aString.good());
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
    ASSERT_TRUE(nullStringSized.empty());
}

TEST(SString, FindSpace)
{
    ASSERT_EQ(SString("Michael Jackson").findSpace(), 7);
    ASSERT_EQ(SString("Michael \tJackson").findSpace(), 7);
    ASSERT_EQ(SString("Michae\n \tJackson").findSpace(), 6);
    ASSERT_EQ(SString("Micha\t \nJackson").findSpace(), 5);
    ASSERT_EQ(SString("Mich\r \tJackson").findSpace(), 4);
    ASSERT_EQ(SString("MichJackson").findSpace(), std::string::npos);
    ASSERT_EQ(SString("").findSpace(), std::string::npos);
    ASSERT_EQ(SString("\t").findSpace(), 0);
}

TEST(SString, TrimLeadingSpaces)
{
    SString string = " \t\t\nMichael  ";
    string.trimLeadingSpaces();
    ASSERT_EQ(string, "Michael  ");

    SString emptyString = "";
    emptyString.trimLeadingSpaces();
    ASSERT_TRUE(emptyString.empty());

    SString invariant = "Jackson  ";
    invariant.trimLeadingSpaces();
    ASSERT_EQ(invariant, "Jackson  ");

    SString spaceOnly = "  \t\n\r  ";
    spaceOnly.trimLeadingSpaces();
    ASSERT_TRUE(spaceOnly.empty());
}

TEST(SString, TrimTrailingSet)
{
    SString string = "//\\//Maglev//\\\\///";
    string.trimTrailingSet("/\\");
    ASSERT_EQ(string, "//\\//Maglev");

    SString emptyString = "";
    emptyString.trimTrailingSet("a");
    ASSERT_TRUE(emptyString.empty());

    SString fullRemoval = "jackson";
    fullRemoval.trimTrailingSet("acjknos");
    ASSERT_TRUE(fullRemoval.empty());
}

TEST(SString, TrimTrailingSpaces)
{
    SString string = " \t\t\nMichael \t\n\r";
    string.trimTrailingSpaces();
    ASSERT_EQ(string, " \t\t\nMichael");

    SString emptyString = "";
    emptyString.trimTrailingSpaces();
    ASSERT_TRUE(emptyString.empty());

    SString invariant = "  Jackson";
    invariant.trimTrailingSpaces();
    ASSERT_EQ(invariant, "  Jackson");

    SString spaceOnly = "  \t\n\r  ";
    spaceOnly.trimTrailingSpaces();
    ASSERT_TRUE(spaceOnly.empty());
}