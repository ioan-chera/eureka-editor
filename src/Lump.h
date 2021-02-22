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

#ifndef Lump_hpp
#define Lump_hpp

#include "PrintfMacros.h"

#include <stdint.h>
#include <vector>

class SString;

//
// Individual lump
//
class Lump
{
public:
	//
	// Get the name here
	//
	const char* getName() const
	{
		return mName;
	}

	void setName(const SString& name);
	void setData(std::vector<uint8_t> &&data);
	void setData(const void *data, size_t size);
	void setDataFromString(const SString &text);

	void write(const void *data, size_t size);
	void printf(EUR_FORMAT_STRING(const char *msg), ...) EUR_PRINTF(2, 3);

	//
	// Get the data size
	//
	const uint8_t *getData() const
	{
		return mData.data();
	}

	//
	// Get the data size
	//
	int getSize() const
	{
		return (int)mData.size() - 1;
	}

	//
	// Gets the data as a null-terminated string pointer. Safe since all data is 0-trailed
	//
	const char *getDataAsString() const
	{
		return reinterpret_cast<const char *>(mData.data());
	}

private:
	std::vector<uint8_t> mData = { 0 };	// lump data (also null-terminated)
	char mName[9] = {};	// lump name (not always limited by length)
};

#endif /* Lump_hpp */
