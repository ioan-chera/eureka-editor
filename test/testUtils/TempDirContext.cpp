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

#include "TempDirContext.hpp"

#ifdef _WIN32
#error TODO Windows
#else
#include <unistd.h>
#endif

// Linux: mkdtemp
// Windows: GetTempPath

void TempDirContext::SetUp()
{
#ifdef _WIN32
#error TODO Windows
#else
	char pattern[] = "tempdirXXXXXX";
	char *result = mkdtemp(pattern);
	ASSERT_NE(result, nullptr);
	mTempDir = result;
	ASSERT_TRUE(mTempDir.good());
#endif
}

void TempDirContext::TearDown()
{
	if(mTempDir.good())
	{
		for(const SString &path : mDeleteList)
		{
			int result = remove(path.c_str());
			ASSERT_EQ(result, 0);
		}
		int result = remove(mTempDir.c_str());
		ASSERT_EQ(result, 0);
	}
}

//
// Gets a child path
//
SString TempDirContext::getChildPath(const char *path)
{
	EXPECT_TRUE(path);
	EXPECT_TRUE(*path);
	return mTempDir + "/" + path;
}

TEST_F(TempDirContext, Test)
{
	ASSERT_TRUE(mTempDir.good());
}
