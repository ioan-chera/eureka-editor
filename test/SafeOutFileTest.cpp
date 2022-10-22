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

static void checkFileContent(const char *path, const char *content)
{
	FILE *f = fopen(path, "rt");
	ASSERT_NE(f, nullptr);
	char msg[128] = {};
	ASSERT_EQ(fgets(msg, sizeof(msg) - 1, f), msg);
	ASSERT_TRUE(feof(f));
	ASSERT_FALSE(ferror(f));
	ASSERT_EQ(fclose(f), 0);
	ASSERT_STREQ(msg, content);
}

TEST_F(SafeOutFileTest, Stuff)
{
	SString path = getChildPath("somefile.txt");

	// Assert no file if merely created (will fail when tearing down)
	{
		SafeOutFile sof(path.get());
	}

	{
		SafeOutFile sof(path.get());
		ASSERT_TRUE(sof.openForWriting().success);	// opening should work
		ASSERT_TRUE(sof.write("Hello, world!", 13).success);	// and this
		ASSERT_TRUE(sof.write(" more.", 6).success);	// and this
		// forget about commiting.
		// Tearing down should not be blocked by child folder
		sof.close();

		ASSERT_TRUE(sof.openForWriting().success);	// reopen it
		ASSERT_TRUE(sof.write("Hello, world!", 13).success);	// and this
		ASSERT_TRUE(sof.write(" more.", 6).success);	// and this
		// Check the destructor too
	}

	{
		// Check that writing to an unopen file will fail
		SafeOutFile sof(path.get());
		ASSERT_FALSE(sof.write("Hello, world2!", 14).success);	// and this
		ASSERT_FALSE(sof.commit().success);
	}
	{
		// Now it will work
		SafeOutFile sof(path.get());
		ASSERT_TRUE(sof.openForWriting().success);
		ASSERT_TRUE(sof.write("Hello, world2!", 14).success);	// and this
		ASSERT_TRUE(sof.write(" more.", 6).success);	// and this
		ASSERT_TRUE(sof.commit().success);
	}
	mDeleteList.push(path);	// the delete list

	// Now check it
	checkFileContent(path.c_str(), "Hello, world2! more.");

	{
		// Check that forgetting to commit won't overwrite the original
		SafeOutFile sof(path.get());
		ASSERT_TRUE(sof.openForWriting().success);
		ASSERT_TRUE(sof.write("New stuff!", 10).success);
		sof.close();	// no commit
	}

	checkFileContent(path.c_str(), "Hello, world2! more.");

	{
		// Check that we can overwrite an old file
		SafeOutFile sof(path.get());
		ASSERT_TRUE(sof.openForWriting().success);
		ASSERT_TRUE(sof.write("New stuff!", 10).success);
		ASSERT_TRUE(sof.commit().success);
		sof.close();	// no commit
	}

	checkFileContent(path.c_str(), "New stuff!");
}
