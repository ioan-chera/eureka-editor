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

#ifndef SafeOutFile_h
#define SafeOutFile_h

#include "m_strings.h"

#include <stdio.h>
#include <random>

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

struct ReportedResult;

//
// Exception-safe, atomic file writer. It starts by writing everything to an
// authorized temp path, only replacing the target path by committing to it.
//
// Does NOT directly throw exceptions.
//
class SafeOutFile
{
public:
	explicit SafeOutFile(const fs::path &path);
	~SafeOutFile()
	{
		close();
	}

	ReportedResult openForWriting();
	ReportedResult commit();
	void close();

	ReportedResult write(const void *data, size_t size) const;

private:
	SString generateRandomPath() const;
	ReportedResult makeValidRandomPath(fs::path &path) const;

	const fs::path mPath;	// the target path
	// the random temporary path. Only valid if mFile non-null
	fs::path mRandomPath;

	FILE *mFile = nullptr;
	mutable std::mt19937 mRandom;
};

#endif /* SafeOutFile_h */
