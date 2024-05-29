//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2024 Ioan Chera
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

#include "w_dehacked.h"
#include "m_game.h"
#include "w_wad.h"
#include "WadData.h"
#include "testUtils/TempDirContext.hpp"
#include "gtest/gtest.h"

static const char *headerBoilerplate[] = {
	"Patch file for Dehacked v3.1",
	"# Here is the file",
	""
};

static const char *part1[] = {
	"Thing 2 (Imp Sarge Trooper Vanilla Shadow Ceiling Weapon)",
	"Initial frame = 442",
	"Width = 655360",
	"ID # = 2222",
	"Bits = 262400",
	"#$Editor category = Weapons",
	"",
	"Frame 442",
	"Sprite number = 31",
	"Sprite subnumber = 32769",
	"",
	"Text 4 4",
	"VILEABCD",
	""
};

static const char *part2[] = {
	"[SPRITES]",
	"30 = NEWB",
	"",
	"Thing 3 (Newbie)",
	"Bits = SPAWNCEILING+SHADOW"
};

class DehackedTest : public TempDirContext
{
protected:
	void makeFile();
};

void DehackedTest::makeFile()
{
	std::ofstream stream(getChildPath("tested.deh"));
	ASSERT_TRUE(stream.is_open());
	mDeleteList.push(getChildPath("tested.deh"));
	
	// Add some bogus headers
	for(const char *line : headerBoilerplate)
		stream << line << std::endl;
	for(const char *line : part1)
		stream << line << std::endl;
	for(const char *line : part2)
		stream << line << std::endl;
}

TEST_F(DehackedTest, LoadNoFile)
{
	ConfigData config;
	ASSERT_FALSE(dehacked::loadFile("bogus.deh", config));
}

static ConfigData baseConfig()
{
	ConfigData config;
	thingtype_t *type = &config.thing_types[3004];
	type->group = 'm';
	type->flags = 0;
	type->radius = 20;
	type->desc = "Trooper";
	type->sprite = "POSS";
	
	type = &config.thing_types[9];
	type->group = 'm';
	type->flags = 0;
	type->radius = 20;
	type->desc = "Sergeant";
	type->sprite = "SPOS";

	return config;
}

static void assertChangedConfig(const ConfigData &config)
{
	ASSERT_EQ(config.thing_types.find(3004), config.thing_types.end());
	auto it = config.thing_types.find(2222);
	ASSERT_NE(it, config.thing_types.end());
	
	const thingtype_t *newtype = &it->second;
	
	ASSERT_EQ(newtype->group, 'w');
	ASSERT_EQ(newtype->flags, THINGDEF_PASS | THINGDEF_CEIL | THINGDEF_INVIS | THINGDEF_LIT);
	ASSERT_EQ(newtype->radius, 10.0f);
	ASSERT_EQ(newtype->desc, "Imp Sarge Trooper Vanilla Shadow Ceiling Weapon");
	ASSERT_EQ(newtype->sprite, "ABCDB");
	
	it = config.thing_types.find(9);
	ASSERT_NE(it, config.thing_types.end());
	newtype = &it->second;
	ASSERT_EQ(newtype->group, 'm');
	ASSERT_EQ(newtype->flags, THINGDEF_CEIL | THINGDEF_INVIS);
	ASSERT_EQ(newtype->radius, 20.0f);
	ASSERT_EQ(newtype->desc, "Newbie");
	ASSERT_EQ(newtype->sprite, "NEWB");
}

TEST_F(DehackedTest, LoadFile)
{
	makeFile();
	
	ConfigData config = baseConfig();
	
	bool result = dehacked::loadFile(getChildPath("tested.deh"), config);
	ASSERT_TRUE(result);
	
	assertChangedConfig(config);
}

TEST_F(DehackedTest, LoadLumps)
{
	auto wad = Wad_file::Open("test.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	Lump_c &dhe1 = wad->AddLump("DEHACKED");
	for(const char *line : part1)
		dhe1.Printf("%s\n", line);
	
	std::vector<std::shared_ptr<Wad_file>> resources;
	resources.push_back(wad);
	
	wad = Wad_file::Open("test2.wad", WadOpenMode::write);
	resources.push_back(wad);
	Lump_c &dhe2 = wad->AddLump("DEHACKED");
	for(const char *line : part2)
		dhe2.Printf("%s\n", line);
	
	MasterDir dir;
	dir.setResources(resources);
	
	ConfigData config = baseConfig();
	
	dehacked::loadLumps(dir, config);
	assertChangedConfig(config);
}
