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

#include "m_strings.h"

#ifdef _WIN32
#include <Windows.h>
#include <rpc.h>
#else
#include <unistd.h>
#endif

#include <thread>
#include <chrono>

// Linux: mkdtemp
// Windows: GetTempPath

//
// Helper function to remove a file/directory with retry logic for Windows
//
static bool removeWithRetry(const fs::path& path, int maxRetries = 5)
{
	for (int attempt = 0; attempt < maxRetries; ++attempt)
	{
		std::error_code ec;
		bool removed = fs::remove(path, ec);
		
		if (removed || !ec)
		{
			return true;
		}
		
#ifdef _WIN32
		// On Windows, check if it's a "file in use" error
		if (ec.value() == ERROR_SHARING_VIOLATION || ec.value() == ERROR_ACCESS_DENIED)
		{
			if (attempt < maxRetries - 1)
			{
				// Wait a bit before retrying (exponential backoff)
				std::this_thread::sleep_for(std::chrono::milliseconds(50 * (1 << attempt)));
				continue;
			}
		}
#endif
		
		// If we get here, either it's not a retryable error or we've exhausted retries
		return false;
	}
	
	return false;
}

void TempDirContext::SetUp()
{
#ifdef _WIN32
	fs::path tempPath = fs::temp_directory_path();

	UUID uuid = {};
	RPC_STATUS status = UuidCreate(&uuid);
	ASSERT_EQ(status, RPC_S_OK);

	ASSERT_NE(uuid.Data1, 0);

	mTempDir = tempPath / SString::printf("EurekaTest%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x", 
		uuid.Data1, uuid.Data2, uuid.Data3, uuid.Data4[0], uuid.Data4[1], uuid.Data4[2], uuid.Data4[3], 
		uuid.Data4[4], uuid.Data4[5], uuid.Data4[6], uuid.Data4[7]).get();

	fs::create_directory(mTempDir);
#else
	char pattern[] = "/tmp/tempdirXXXXXX";
	char *result = mkdtemp(pattern);
	ASSERT_NE(result, nullptr);
	mTempDir = result;
	ASSERT_FALSE(mTempDir.empty());
#endif
}

void TempDirContext::TearDown()
{
	if(!mTempDir.empty())
	{
		while(!mDeleteList.empty())
		{
			// Don't assert fatally, so we get the change to delete what we can.
			ASSERT_TRUE(removeWithRetry(mDeleteList.top()));
			mDeleteList.pop();
		}
		ASSERT_TRUE(removeWithRetry(mTempDir));
	}
}

//
// Gets a sub path
//
fs::path TempDirContext::getSubPath(const fs::path &path) const
{
	return mTempDir / path;
}

TEST_F(TempDirContext, Test)
{
	ASSERT_FALSE(mTempDir.empty());
}
