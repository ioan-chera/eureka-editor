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

TEST(SString, NoCaseCheck)
{
	ASSERT_TRUE(SString("Michael").noCaseEqual("MiCHaEL"));
	ASSERT_TRUE(SString("Michael").noCaseEqual(SString("MiCHaEL")));
	ASSERT_FALSE(SString("Michael").noCaseEqual("Jackson"));
	ASSERT_TRUE(SString("").noCaseEqual(nullptr));

	ASSERT_LT(SString("jackson").noCaseCompare("Michael"), 0);
	ASSERT_EQ(SString("jackson").noCaseCompare("JackSON"), 0);
	ASSERT_GT(SString("Jackson").noCaseCompare("III"), 0);

	ASSERT_TRUE(SString("MICHAEL JACKSON").noCaseStartsWith("mich"));
	ASSERT_FALSE(SString("MICHAEL JACKSON").noCaseStartsWith("Jack"));

	ASSERT_EQ(SString("Michael Jackson").findNoCase("jack"), 8);
	ASSERT_EQ(SString("Michael Jackson").findNoCase("ACK"), 9);
	ASSERT_EQ(SString("Michael Jackson").findNoCase("Jax"), std::string::npos);
}

TEST(SString, StartsWith)
{
	ASSERT_TRUE(SString("Michael Jackson").startsWith("Mich"));
	ASSERT_FALSE(SString("Michael Jackson").startsWith("mich"));
	ASSERT_TRUE(SString("Michael Jackson").startsWith(nullptr));
}

TEST(SString, Operators)
{
	ASSERT_NE(SString("Daniel"), SString("daniel"));
	ASSERT_NE(SString("Daniel"), "daniel");
	ASSERT_EQ(SString("Daniel"), SString("Daniel"));
	ASSERT_EQ(SString("Daniel"), "Daniel");
	ASSERT_LT(SString("Daniel"), SString("Earn"));
	ASSERT_FALSE(SString("EDaniel") < SString("Darn"));
}

TEST(SString, Indexing)
{
	SString name = "Jackson";
	ASSERT_EQ(name[0], 'J');
	ASSERT_EQ(name[1], 'a');
	ASSERT_EQ(name[2], 'c');
	ASSERT_EQ(name[-1], 'n');
	ASSERT_EQ(name[-2], 'o');
	ASSERT_EQ(name[-3], 's');

	name[-3] = 't';
	name[1] = 'e';
	ASSERT_EQ(name, "Jeckton");
}

TEST(SString, NulBan)
{
	SString name = "Jackson";
	name += '\0';
	ASSERT_EQ(name, "Jackson");
	ASSERT_EQ(name.length(), 7);
	name += 't';
	ASSERT_EQ(name, "Jacksont");
}

TEST(SString, CutWithSpace)
{
	SString name = "Michael Jackson";
	SString first, last;
	name.getCutWithSpace(7, &first, &last);
	ASSERT_EQ(first, "Michael");
	ASSERT_EQ(last, "Jackson");
	SString tail;
	last.cutWithSpace(6, &tail);
	ASSERT_EQ(last, "Jackso");
	ASSERT_TRUE(tail.empty());

	SString surname;
	name.cutWithSpace(7, &surname);
	ASSERT_EQ(name, "Michael");
	ASSERT_EQ(surname, "Jackson");
}
