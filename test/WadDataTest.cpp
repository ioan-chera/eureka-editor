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

//
// Add a valid lump with content so it can be part of the namespace
//
static void addValidLump(const std::shared_ptr<Wad_file> &wad, const char *name)
{
	Lump_c &lump = wad->AddLump(name);
	lump.Printf("a");
}

TEST(MasterDir, FindGlobalLump)
{
	MasterDir master;
	auto wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	addValidLump(wad, "LUMP1");	// 0
	addValidLump(wad, "LUMP2");
	addValidLump(wad, "S_START");
	addValidLump(wad, "LUMP3");
	addValidLump(wad, "LUMP4");	// 4
	addValidLump(wad, "S_END");
	addValidLump(wad, "LUMP5");
	addValidLump(wad, "F_START");
	addValidLump(wad, "LUMP6");	// 8
	addValidLump(wad, "F_END");
	addValidLump(wad, "LUMP7");

	master.setGameWad(wad);

	auto wad2 = Wad_file::Open("dummy.wad", WadOpenMode::write);
	ASSERT_TRUE(wad2);

	addValidLump(wad2, "LUMP11");	// 0
	addValidLump(wad2, "LUMP2");
	addValidLump(wad2, "SS_START");
	addValidLump(wad2, "LUMP5");
	addValidLump(wad2, "S_END");	// 4
	addValidLump(wad2, "LUMP3");
	addValidLump(wad2, "FF_START");
	addValidLump(wad2, "LUMP6");
	addValidLump(wad2, "FF_END");	// 8
	addValidLump(wad2, "LUMP7");
	
	master.ReplaceEditWad(wad2);

	ASSERT_EQ(master.findGlobalLump("LUMP1"), wad->GetLump(0));
	ASSERT_EQ(master.findGlobalLump("LUMP2"), wad2->GetLump(1));
	ASSERT_EQ(master.findGlobalLump("LUMP3"), wad2->GetLump(5));
	ASSERT_EQ(master.findGlobalLump("LUMP4"), nullptr);
	ASSERT_EQ(master.findGlobalLump("LUMP5"), wad->GetLump(6));
	ASSERT_EQ(master.findGlobalLump("LUMP6"), nullptr);
	ASSERT_EQ(master.findGlobalLump("LUMP7"), wad2->GetLump(9));
	ASSERT_EQ(master.findGlobalLump("LUMP11"), wad2->GetLump(0));
	ASSERT_EQ(master.findGlobalLump("LUMP12"), nullptr);
}

TEST(MasterDir, FindFirstSpriteLump)
{
	MasterDir master;

	auto wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	wad->AddLump("S_START");
	Lump_c &wad1possa1 = wad->AddLump("POSSA1");
	wad1possa1.Printf("a");	// need to have content to be considered
	wad->AddLump("TROOC1").Printf("a");
	Lump_c &wad1troob1 = wad->AddLump("TROOB1");
	wad1troob1.Printf("a");
	Lump_c &wad1trood1 = wad->AddLump("TROOD1");
	wad1trood1.Printf("a");
	wad->AddLump("S_END");

	master.setGameWad(wad);

	auto wad2 = Wad_file::Open("dummy2.wad", WadOpenMode::write);
	ASSERT_TRUE(wad2);

	wad2->AddLump("S_START");
	Lump_c &wad2possa1 = wad2->AddLump("POSSA1");
	wad2possa1.Printf("a");
	Lump_c &wad2trooe1 = wad2->AddLump("TROOE1");
	wad2trooe1.Printf("a");
	wad2->AddLump("S_END");

	master.ReplaceEditWad(wad2);
	
	std::vector<SpriteLumpRef> tested, expected;
	tested = master.findFirstSpriteLump("POSS");
	expected = {
		{&wad2possa1, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
	};
	for(int i = 0; i < 8; ++i)
	{
		ASSERT_EQ(tested.at(i).lump, expected[i].lump);
		ASSERT_EQ(tested.at(i).flipped, expected[i].flipped);
	}
	
	tested = master.findFirstSpriteLump("TROO");
	expected = {
		{&wad2trooe1, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
		{nullptr, false},
	};
	
	for(int i = 0; i < 8; ++i)
	{
		ASSERT_EQ(tested.at(i).lump, expected[i].lump);
		ASSERT_EQ(tested.at(i).flipped, expected[i].flipped);
	}
}
