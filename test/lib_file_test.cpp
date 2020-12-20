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

#include "lib_file.h"
#include "m_strings.h"
#include "gtest/gtest.h"

void DebugPrintf(const char *str, ...)
{
}

const char *fl_filename_name(const char * filename)
{
    return nullptr;
}

int fl_filename_absolute(char *to, int tolen, const char *from)
{
    return 0;
}

TEST(LibFile, HasExtension)
{
	ASSERT_FALSE(HasExtension("man/doc."));
	ASSERT_TRUE(HasExtension("man/doom.wad"));
	ASSERT_FALSE(HasExtension("man.wad/doom"));
	ASSERT_TRUE(HasExtension("man.wad/doom.wad"));
	ASSERT_TRUE(HasExtension("man.wad/doom..wad"));
	ASSERT_FALSE(HasExtension(".okay"));
	ASSERT_FALSE(HasExtension("man/.okay"));
	ASSERT_FALSE(HasExtension("man/.okay."));
	ASSERT_TRUE(HasExtension("man/.okay.wad"));
	ASSERT_FALSE(HasExtension("/."));
	ASSERT_FALSE(HasExtension("."));
	ASSERT_FALSE(HasExtension(".."));
	ASSERT_FALSE(HasExtension(""));
}

TEST(LibFile, MatchExtension)
{
	ASSERT_TRUE(MatchExtension("man/doc.", nullptr));
	ASSERT_TRUE(MatchExtension("man/doc.", ""));
	ASSERT_FALSE(MatchExtension("man/.doc.", "doc"));
	ASSERT_TRUE(MatchExtension("man/doc. ", " "));
	ASSERT_TRUE(MatchExtension("man.wad/doom", nullptr));
	ASSERT_FALSE(MatchExtension("man.wad/doom", "doom"));
	ASSERT_TRUE(MatchExtension("man.wad/doom.wad", ".WAD"));
	ASSERT_TRUE(MatchExtension("man.wad/doom..wad", ".WAD"));
	ASSERT_TRUE(MatchExtension(".okay", ""));
	ASSERT_FALSE(MatchExtension(".okay", "okay"));
	ASSERT_TRUE(MatchExtension("man/.okay", ""));
	ASSERT_FALSE(MatchExtension("man/.okay", "okay"));
	ASSERT_TRUE(MatchExtension("man/.okay.WAD", "wad"));
	ASSERT_TRUE(MatchExtension("/.", nullptr));
	ASSERT_TRUE(MatchExtension(".", nullptr));
	ASSERT_TRUE(MatchExtension("..", nullptr));
	ASSERT_FALSE(MatchExtension("..", "."));
	ASSERT_TRUE(MatchExtension("", nullptr));
}

TEST(LibFile, FilenameGetPath)
{
    ASSERT_EQ(FilenameGetPath("path/to/file"), "path/to");
    ASSERT_EQ(FilenameGetPath("path/to" DIR_SEP_STR DIR_SEP_STR "file"), "path/to");
    ASSERT_EQ(FilenameGetPath("file"), ".");
    ASSERT_EQ(FilenameGetPath(DIR_SEP_STR "file"), DIR_SEP_STR);
#ifdef _WIN32
    ASSERT_EQ(FilenameGetPath("C:" DIR_SEP_STR "file"), "C:" DIR_SEP_STR);
#endif
}

TEST(LibFile, FindBaseName)
{
    ASSERT_EQ(FindBaseName("path/to///file"), 10);
    ASSERT_EQ(FindBaseName("path/to/file"), 8);
    ASSERT_EQ(FindBaseName("/file"), 1);
    ASSERT_EQ(FindBaseName("//file"), 2);
    ASSERT_EQ(FindBaseName("file"), 0);
}
