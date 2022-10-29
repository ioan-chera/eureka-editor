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

#include "lib_file.h"
#include "lib_util.h"
#include "Errors.h"
#include "SafeOutFile.h"
#include "sys_debug.h"

#include <chrono>

enum
{
	RANDOM_PATH_ATTEMPTS = 16
};

// Limited set of ASCII
static const char skSafeAscii[] = "123456789(0)-_=qQwWeErRtTyYuUiIoOpPaAsSdDfFg"
	"GhHjJkKlLzZcCvVbBnNmM";

//
// Prepare the path now
//
SafeOutFile::SafeOutFile(const fs::path &path) : mPath(path)
{
	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	mRandom.seed(static_cast<std::mt19937::result_type>(seed));
}

//
// Open the file for writing
//
ReportedResult SafeOutFile::openForWriting()
{
	ReportedResult result;
	if(!(result = makeValidRandomPath(mRandomPath)).success)
		return result;

	fs::path randomPath = mRandomPath;
	close();
	mRandomPath = randomPath;

	mStream.open(mRandomPath, std::ios::binary | std::ios::trunc);
	if(!mStream.is_open())
		return { false, "couldn't open the file." };

	return { true };
}

//
// Commit the writing to the final file.
//
ReportedResult SafeOutFile::commit()
{
	ReportedResult result;
	if(!mStream.is_open())
		return { false, "couldn't create the file." };
	// First, to be ultra-safe, make another temp path
	fs::path safeRandomPath;
	int i = 0;
	for(; i < RANDOM_PATH_ATTEMPTS; ++i)
	{
		if(!(result = makeValidRandomPath(safeRandomPath)).success)
			return result;
		if(!FileExists(safeRandomPath) || !fs::equivalent(safeRandomPath, mRandomPath))
		{
			// also make sure it doesn't collide with ours
			break;
		}
	}
	if(i == RANDOM_PATH_ATTEMPTS)
		return { false, "failed on several attempts." };

	// Now we need to close our work. Store the paths of interest in a variable
	fs::path finalPath = mPath;
	// Rename the old file, if any, to a safe random path. It may fail if the
	// file doesn't exist
	bool overwriteOldFile = true;

	try
	{
		fs::rename(finalPath, safeRandomPath);
	}
	catch(const fs::filesystem_error &e)
	{
		if(fs::exists(finalPath))
			return { false, e.what() };
		overwriteOldFile = false;
	}

	fs::path writtenPath = mRandomPath;
	if(mStream.is_open())
		mStream.close();

	try
	{
		fs::rename(writtenPath, finalPath);
		if(overwriteOldFile)
			fs::remove(safeRandomPath);
		close();
		return { true };
	}
	catch(const fs::filesystem_error &e)
	{
		return { false, e.what() };
	}
}

//
// Closes the file. WARNING: merely doing this will just remove the temp file
// and cancel everything. You need to commit first
//
void SafeOutFile::close()
{
	if(mStream.is_open())
	{
		mStream.close();
		try
		{
			fs::remove(mRandomPath);
		}
		catch(const fs::filesystem_error &e)
		{
			gLog.printf("Error removing %s: %s\n", mRandomPath.u8string().c_str(), e.what());
		}
	}
	mRandomPath.clear();
}

//
// Writes data to file
//
ReportedResult SafeOutFile::write(const void *data, size_t size)
{
	if(!mStream.is_open())
		return { false, "file wasn't created." };
	if(!mStream.write(static_cast<const char *>(data), size))
		return { false, "failed writing the entire data." };
	return { true };
}

//
// Generate the random path
//
fs::path SafeOutFile::generateRandomPath() const
{
	fs::path randomname = fs::u8path(mPath.filename().u8string() +
									 skSafeAscii[mRandom() % (sizeof(skSafeAscii) - 1)] +
									 skSafeAscii[mRandom() % (sizeof(skSafeAscii) - 1)] +
									 skSafeAscii[mRandom() % (sizeof(skSafeAscii) - 1)] +
									 skSafeAscii[mRandom() % (sizeof(skSafeAscii) - 1)]);
	return mPath.parent_path() / randomname;
}

//
// Try to make a random path for writing
//
ReportedResult SafeOutFile::makeValidRandomPath(fs::path &path) const
{
	fs::path randomPath;
	int i = 0;
	for(; i < RANDOM_PATH_ATTEMPTS; ++i)
	{
		randomPath = generateRandomPath();
		if(!FileExists(randomPath))
			break;
	}
	if(i == RANDOM_PATH_ATTEMPTS)
		return { false, "failed writing after several attempts." };
	path = std::move(randomPath);
	return { true };
}
