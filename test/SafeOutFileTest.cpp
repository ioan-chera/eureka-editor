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

#include "Errors.h"
#include "SafeOutFile.h"
#include "testUtils/TempDirContext.hpp"
#include "gtest/gtest.h"

class SafeOutFileTest : public TempDirContext
{
};

static void checkFileContent(const fs::path &path, const char *content)
{
	std::ifstream stream(path);
	ASSERT_TRUE(stream.is_open());
	char msg[128] = {};
	ASSERT_TRUE(stream.get(msg, sizeof(msg)));
	ASSERT_TRUE(stream.eof());
	ASSERT_FALSE(stream.fail());
	stream.close();
	ASSERT_STREQ(msg, content);
}

TEST_F(SafeOutFileTest, Stuff)
{
	fs::path path = getChildPath("somefile.txt");

	// Assert no file if merely created (will fail when tearing down)
	{
		BufferedOutFile sof(path);
	}

	{
		{
			BufferedOutFile sof(path);
			sof.write("Hello, world!", 13);	// and this
			sof.write(" more.", 6);	// and this
			// forget about commiting.
			// Tearing down should not be blocked by child folder
		}

		BufferedOutFile sof(path);	// reopen it
		sof.write("Hello, world!", 13);	// and this
		sof.write(" more.", 6);	// and this
		// Check the destructor too
	}

	{
		// Now it will work
		BufferedOutFile sof(path);
		sof.write("Hello, world2!", 14);	// and this
		sof.write(" more.", 6);	// and this
		sof.commit();
	}
	mDeleteList.push(path);	// the delete list

	// Now check it
	checkFileContent(path, "Hello, world2! more.");

	{
		// Check that forgetting to commit won't overwrite the original
		BufferedOutFile sof(path);
		sof.write("New stuff!", 10);
		// no commit
	}

	checkFileContent(path, "Hello, world2! more.");

	{
		// Check that we can overwrite an old file
		BufferedOutFile sof(path);
		sof.write("New stuff!", 10);
		sof.commit();
	}

	checkFileContent(path, "New stuff!");
}
