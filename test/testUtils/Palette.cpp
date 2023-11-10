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

#include "Palette.hpp"
#include "im_color.h"
#include "w_wad.h"
#include "gtest/gtest.h"
#include <stdint.h>

//
// Uses a common palette generated from 3:3:2 RGB (thus no grays)
//
void makeCommonPalette(Palette &palette)
{
	auto wad = attempt(Wad_file::Open("dummy.wad", WadOpenMode::write));
	EXPECT_TRUE(wad);
	Lump_c &lump = wad->AddLump("PALETTE");

	// Use 6:8:5 RGB + gray levels
	static const uint8_t redLevels[6] = {0, 51, 102, 153, 204, 255};
	static const uint8_t greenLevels[8] = {0, 37, 73, 109, 146, 182, 219, 255};
	static const uint8_t blueLevels[5] = {0, 64, 128, 192, 255};
	for(uint8_t red : redLevels)
		for(uint8_t green : greenLevels)
			for(uint8_t blue : blueLevels)
			{
				uint8_t data[3] = {red, green, blue};
				lump.Write(data, 3);
			}
	constexpr int numGrays = 256 - lengthof(redLevels) * lengthof(greenLevels) *
			lengthof(blueLevels);
	for(int i = 0; i < numGrays; ++i)
	{
		uint8_t data = (uint8_t)(256 / numGrays * i + 256 / numGrays / 2);
		lump.Write(&data, 1);
		lump.Write(&data, 1);
		lump.Write(&data, 1);
	}

	EXPECT_TRUE(palette.loadPalette(lump, 2, 2));
}

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

