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

#include "im_color.h"
#include "w_wad.h"
#include "testUtils/Palette.hpp"
#include "gtest/gtest.h"
#include <vector>
#include <stdint.h>

TEST(Palette, LoadPalette)
{
	Palette palette;
	// Need a wad
	auto wad = attempt(Wad_file::Open("dummy.wad", WadOpenMode::write));
	ASSERT_TRUE(wad);
	Lump_c &lump = wad->AddLump("PALETTE");

	// Try on empty lump: should fail
	ASSERT_FALSE(palette.loadPalette(lump, 2, 2));

	// Try insufficient lump: should still fail
	std::vector<uint8_t> data;
	data.resize(256);
	lump.Write(data.data(), (int)data.size());
	ASSERT_FALSE(palette.loadPalette(lump, 2, 2));

	// Repopulate lump, this time valid
	data = makeGrayscale();
	lump.clearData();
	lump.Write(data.data(), (int)data.size());
	ASSERT_TRUE(palette.loadPalette(lump, 2, 2));
	ASSERT_TRUE(palette.loadPalette(lump, 0, 0));
	ASSERT_FALSE(palette.loadPalette(lump, -1, 0));
	ASSERT_FALSE(palette.loadPalette(lump, 0, -1));
	ASSERT_TRUE(palette.loadPalette(lump, 4, 4));
	ASSERT_FALSE(palette.loadPalette(lump, 4, 5));
	ASSERT_FALSE(palette.loadPalette(lump, 5, 4));
}

TEST(Palette, FindPaletteColor)
{
	Palette palette;
	auto wad = attempt(Wad_file::Open("dummy.wad", WadOpenMode::write));
	ASSERT_TRUE(wad);
	Lump_c &lump = wad->AddLump("PALETTE");
	std::vector<uint8_t> data = makeGrayscale();
	lump.Write(data.data(), (int)data.size());

	ASSERT_TRUE(palette.loadPalette(lump, 2, 2));
	// Now look for some
	ASSERT_EQ(palette.findPaletteColor(100, 100, 98), 99);
	ASSERT_EQ(palette.findPaletteColor(63, 64, 65), 64);
	ASSERT_EQ(palette.findPaletteColor(255, 255, 255), 254);	// not the trans pixel
}
