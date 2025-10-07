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

//
// Writes data to file
//
void BufferedOutFile::write(const void *vdata, size_t size)
{
	auto data = static_cast<const uint8_t*>(vdata);
	mData.insert(mData.end(), data, data + size);
}

// WARNING: this throws.
void BufferedOutFile::commit()
{
	try
	{
		std::ofstream stream;
		stream.exceptions(std::ios::failbit);
		stream.open(mPath, std::ios::out | std::ios::binary);
		stream.write(reinterpret_cast<const char*>(mData.data()), mData.size());
	}
	catch(const std::ofstream::failure &e)
	{
		SString errorMessage = GetErrorMessage(e.code().value());
		gLog.printf("Failed writing %s: %s\n", mPath.u8string().c_str(), errorMessage.c_str());
		throw std::runtime_error(errorMessage.get());
	}
}
