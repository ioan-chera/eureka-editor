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

TEST(LibFile, FilenameGetPath)
{
    ASSERT_EQ(FilenameGetPath("path/to/file"), "path/to");
}
