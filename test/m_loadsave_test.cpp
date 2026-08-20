//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2026 Ioan Chera
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

#include "m_loadsave.h"
#include "Instance.h"
#include "w_rawdef.h"

#include "gtest/gtest.h"

enum Skill : int16_t
{
   EASY = 1,
   MEDIUM = 2,
   HARD = 4
};

TEST(MLoadSave, LoadThingsDoomAddsUDMFSkills)
{
	// Populate the things
	std::shared_ptr<Wad_file> wad = Wad_file::Open("wad.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	wad->AddLevel("MAP01");
	wad->AddLump("THINGS");
	wad->AddLump("LINEDEFS");
	wad->AddLump("SIDEDEFS");
	wad->AddLump("VERTEXES");
	wad->AddLump("SEGS");
	wad->AddLump("SSECTORS");
	wad->AddLump("NODES");
	wad->AddLump("SECTORS");
	wad->AddLump("REJECT");
	wad->AddLump("BLOCKMAP");

	Lump_c *things = wad->GetLump(1);
	ASSERT_TRUE(things);

	struct PackedThing
	{
		int16_t x, y, angle, doomednum, options;
	};

	static const PackedThing data[] = {
		{ 0, 0, 0, 100, EASY },
		{ 0, 0, 0, 100, MEDIUM },
		{ 0, 0, 0, 100, HARD },
		{ 0, 0, 0, 100, EASY | MEDIUM },
		{ 0, 0, 0, 100, MEDIUM | HARD },
		{ 0, 0, 0, 100, EASY | HARD },
	};

	things->Write(data, sizeof(data));

	// Now populate a document
	Instance inst;
	inst.loaded.levelFormat = MapFormat::udmf;
	inst.level.LoadThings(0, wad.get());

	int easycount = 0;
	int hardcount = 0;

	for(const auto &T : inst.level.things)
	{
		if(T->options & MTF_Easy)
		{
			ASSERT_TRUE(T->udmfflags & MTF_UDMF_skill1);
			++easycount;
		}
		if(T->options & MTF_Hard)
		{
			ASSERT_TRUE(T->udmfflags & MTF_UDMF_skill5);
			++hardcount;
		}
	}

	ASSERT_EQ(easycount, 3);
	ASSERT_EQ(hardcount, 3);
}

TEST(MLoadSave, LoadThingsHexenAddsUDMFSkills)
{
	std::shared_ptr<Wad_file> wad = Wad_file::Open("wad.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	wad->AddLevel("MAP01");
	wad->AddLump("THINGS");
	wad->AddLump("LINEDEFS");
	wad->AddLump("SIDEDEFS");
	wad->AddLump("VERTEXES");
	wad->AddLump("SEGS");
	wad->AddLump("SSECTORS");
	wad->AddLump("NODES");
	wad->AddLump("SECTORS");
	wad->AddLump("REJECT");
	wad->AddLump("BLOCKMAP");
	wad->AddLump("BEHAVIOR");

	Lump_c *things = wad->GetLump(1);
	ASSERT_TRUE(things);

	struct PackedThing
	{
		int16_t tid, x, y, z, angle, doomednum, options;
		uint8_t special, args[5];
	};

	static const PackedThing data[] = {
		{ 0, 0, 0, 0, 0, 100, EASY,          0, { 0, 0, 0, 0, 0 } },
		{ 0, 0, 0, 0, 0, 100, MEDIUM,        0, { 0, 0, 0, 0, 0 } },
		{ 0, 0, 0, 0, 0, 100, HARD,          0, { 0, 0, 0, 0, 0 } },
		{ 0, 0, 0, 0, 0, 100, EASY | MEDIUM, 0, { 0, 0, 0, 0, 0 } },
		{ 0, 0, 0, 0, 0, 100, MEDIUM | HARD, 0, { 0, 0, 0, 0, 0 } },
		{ 0, 0, 0, 0, 0, 100, EASY | HARD,   0, { 0, 0, 0, 0, 0 } },
	};

	things->Write(data, sizeof(data));

	// Now populate a document
	Instance inst;
	inst.loaded.levelFormat = MapFormat::udmf;
	inst.level.LoadThings_Hexen(0, wad.get());

	int easycount = 0;
	int hardcount = 0;

	for(const auto &T : inst.level.things)
	{
		if(T->options & MTF_Easy)
		{
			ASSERT_TRUE(T->udmfflags & MTF_UDMF_skill1);
			++easycount;
		}
		if(T->options & MTF_Hard)
		{
			ASSERT_TRUE(T->udmfflags & MTF_UDMF_skill5);
			++hardcount;
		}
	}

	ASSERT_EQ(easycount, 3);
	ASSERT_EQ(hardcount, 3);

}
