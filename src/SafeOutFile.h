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

#include <filesystem>
namespace fs = std::filesystem;

#include <stdint.h>

//
// Exception-safe, atomic file writer. It starts by writing everything to an
// authorized temp path, only replacing the target path by committing to it.
//
// Does NOT directly throw exceptions.
//
class BufferedOutFile
{
public:
	explicit BufferedOutFile(const fs::path& path) : mPath(path)
	{
	}

	void write(const void* data, size_t size);
	void commit();

private:
	const fs::path mPath;	// the target path
	std::vector<uint8_t> mData;
};

#endif /* SafeOutFile_h */
