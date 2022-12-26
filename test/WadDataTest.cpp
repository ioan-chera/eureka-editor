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

#include "WadData.h"
#include "w_wad.h"
#include "gtest/gtest.h"

TEST(MasterDir, FindFirstSpriteLump)
{
	MasterDir master;

	auto wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	wad->AddLump("S_START");
	Lump_c *wad1possa1 = wad->AddLump("POSSA1");
	wad1possa1->Printf("a");	// need to have content to be considered
	wad->AddLump("TROOC1")->Printf("a");
	Lump_c *wad1troob1 = wad->AddLump("TROOB1");
	wad1troob1->Printf("a");
	wad->AddLump("TROOD1")->Printf("a");
	wad->AddLump("S_END");

	master.MasterDir_Add(wad);

	auto wad2 = Wad_file::Open("dummy2.wad", WadOpenMode::write);
	ASSERT_TRUE(wad2);

	wad2->AddLump("S_START");
	Lump_c *wad2possa1 = wad2->AddLump("POSSA1");
	wad2possa1->Printf("a");
	wad2->AddLump("TROOE1")->Printf("a");
	wad2->AddLump("S_END");

	master.MasterDir_Add(wad2);

	ASSERT_EQ(master.findFirstSpriteLump("POSS"), wad2possa1);
	ASSERT_EQ(master.findFirstSpriteLump("TROO"), wad1troob1);
}
