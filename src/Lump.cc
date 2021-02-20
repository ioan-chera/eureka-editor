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

#include "Lump.h"

#include "m_strings.h"

//
// Set lump name
// TODO: long file name of lump, when supporting packs
//
void Lump::setName(const SString& name)
{
	SString lumpname = name.asUpper();
	if (lumpname.length() > 8)
		lumpname.erase(8, SString::npos);
	strncpy(mName, lumpname.c_str(), 8);
	mName[8] = 0;
}

//
// Assign the data by move
//
void Lump::setData(std::vector<uint8_t> &&data)
{
	mData = std::move(data);
	mData.push_back(0);	// add the null termination
}
