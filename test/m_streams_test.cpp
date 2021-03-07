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

#include "m_streams.h"
#include "m_strings.h"
#include "gtest/gtest.h"

TEST(MStreams, MReadTextLine)
{
    // Check that the BOM gets stripped, that both CRLF and LF are detected
    // and that no newline at the end of stream is fine
    std::string content = "\xef\xbb\xbfLine 1 is here\n"
    "Line 2 is here\r\n"
    "Line 3 is here\n"
    "Line 4 is here";

    SString line;
    {
        std::istringstream ss(content);

        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_EQ(line, "Line 1 is here");
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_EQ(line, "Line 2 is here");
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_EQ(line, "Line 3 is here");
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_EQ(line, "Line 4 is here");
        ASSERT_FALSE(M_ReadTextLine(line, ss));
    }

    // Check that a trailing newline won't generate an empty line
    content = "Line 5 is here\n"
    "Line 6 is here\n";

    {
        std::istringstream ss(content);
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_EQ(line, "Line 5 is here");
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_EQ(line, "Line 6 is here");
        ASSERT_FALSE(M_ReadTextLine(line, ss));
    }

    // Check that a final line with just a space gets treated as an empty line
    content = "Line 5 is here\n"
    "Line 6 is here\n"
    " ";

    {
        std::istringstream ss(content);
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_EQ(line, "Line 5 is here");
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_EQ(line, "Line 6 is here");
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_TRUE(line.empty());
        ASSERT_FALSE(M_ReadTextLine(line, ss));
    }

    // Check that a newline shall show up as one empty line
    content = "\n";
    {
        std::istringstream ss(content);
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_TRUE(line.empty());
        ASSERT_FALSE(M_ReadTextLine(line, ss));
    }

    // Check that a mere space shows up as one empty line
    content = " ";
    {
        std::istringstream ss(content);
        ASSERT_TRUE(M_ReadTextLine(line, ss));
        ASSERT_TRUE(line.empty());
        ASSERT_FALSE(M_ReadTextLine(line, ss));
    }

    content = "";
    {
        std::istringstream ss(content);
        ASSERT_FALSE(M_ReadTextLine(line, ss));
    }
}
