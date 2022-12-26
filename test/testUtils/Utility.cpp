//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022 Ioan Chera
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

#include "Utility.hpp"

//
// Produces a vector for a grayscale palette
//
std::vector<uint8_t> makeGrayscale()
{
	std::vector<uint8_t> data;
	data.reserve(768);
	for(int i = 0; i < 256; ++i)
	{
		// Make a simple f(x) = [x, x, x] data
		data.push_back((uint8_t)i);
		data.push_back((uint8_t)i);
		data.push_back((uint8_t)i);
	}
	return data;
}
