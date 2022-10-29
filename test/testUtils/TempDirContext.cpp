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
#include <Windows.h>
#include <rpc.h>
#else
#include <unistd.h>
#endif

// Linux: mkdtemp
// Windows: GetTempPath

void TempDirContext::SetUp()
{
#ifdef _WIN32
	char tempPath[MAX_PATH] = {};
	DWORD result = GetTempPathA(sizeof(tempPath), tempPath);
	
	ASSERT_GT(result, 0u);
	ASSERT_EQ(result, strlen(tempPath));

	UUID uuid = {};
	RPC_STATUS status = UuidCreate(&uuid);
	ASSERT_EQ(status, RPC_S_OK);

	ASSERT_NE(uuid.Data1, 0);

	mTempDir = SString::printf("%sEurekaTest%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x", tempPath, uuid.Data1, uuid.Data2, uuid.Data3, uuid.Data4[0], uuid.Data4[1], uuid.Data4[2], uuid.Data4[3], uuid.Data4[4], uuid.Data4[5], uuid.Data4[6], uuid.Data4[7]);

	ASSERT_TRUE(CreateDirectoryA(mTempDir.c_str(), nullptr));
#else
	char pattern[] = "/tmp/tempdirXXXXXX";
	char *result = mkdtemp(pattern);
	ASSERT_NE(result, nullptr);
	mTempDir = result;
	ASSERT_TRUE(mTempDir.good());
#endif
}

//
// Portable way to delete file or folder. Under Windows it seems that remove can't delete folders.
//
static bool deleteFileOrFolder(const char *path)
{
#ifdef _WIN32
	DWORD attributes = GetFileAttributesA(path);
	EXPECT_NE(attributes, INVALID_FILE_ATTRIBUTES);
	if(attributes != INVALID_FILE_ATTRIBUTES && attributes & FILE_ATTRIBUTE_DIRECTORY)
		return RemoveDirectoryA(path) != FALSE;
	return remove(path) == 0;
#else
	return remove(path) == 0;
#endif
}

void TempDirContext::TearDown()
{
	if(mTempDir.good())
	{
		while(!mDeleteList.empty())
		{
			// Don't assert fatally, so we get the change to delete what we can.
			EXPECT_TRUE(deleteFileOrFolder(mDeleteList.top().c_str()));
			mDeleteList.pop();
		}
		ASSERT_TRUE(deleteFileOrFolder(mTempDir.c_str()));
	}
}

//
// Gets a child path
//
fs::path TempDirContext::getChildPath(const fs::path &path) const
{
	return fs::u8path(mTempDir.get()) / path;
}

TEST_F(TempDirContext, Test)
{
	ASSERT_TRUE(mTempDir.good());
}
