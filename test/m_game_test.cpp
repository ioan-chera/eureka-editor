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

#include "testUtils/TempDirContext.hpp"

#include "gtest/gtest.h"

#include "Instance.h"
#include "m_game.h"

class MGameFixture : public TempDirContext
{
};

//=============================================================================
//
// TESTS
//
//=============================================================================


//
// Convenience operator
//
static bool operator == (const thingtype_t &type1, const thingtype_t &type2)
{
	return type1.group == type2.group && type1.flags == type2.flags &&
	type1.radius == type2.radius && type1.scale == type2.scale && type1.desc == type2.desc &&
	type1.sprite == type2.sprite && type1.color == type2.color && type1.args[0] == type2.args[0] &&
	type1.args[1] == type2.args[1] && type1.args[2] == type2.args[2] &&
	type1.args[3] == type2.args[3] && type1.args[4] == type2.args[4];
}

TEST(MGame, ConfigDataGetThingType)
{
	ConfigData config = {};
	thingtype_t type = {};
	type.group = 'a';
	type.flags = THINGDEF_LIT;
	type.radius = 16;
	type.scale = 1.0f;
	type.desc = "Bright spot";
	type.sprite = "BRIG";
	type.color = rgbMake(128, 0, 0);

	config.thing_types[111] = type;

	thingtype_t type2 = {};

	type2.group = 'b';
	type2.flags = 0;
	type2.radius = 16;
	type2.scale = 1.0f;
	type2.desc = "Dark spot";
	type2.sprite = "DARK";
	type2.color = rgbMake(0, 0, 128);

	config.thing_types[222] = type2;

	ASSERT_EQ(config.getThingType(111), type);
	ASSERT_EQ(config.getThingType(222), type2);
	ASSERT_EQ(config.getThingType(1).desc, "UNKNOWN TYPE");
	ASSERT_EQ(config.getThingType(0).desc, "UNKNOWN TYPE");
	ASSERT_EQ(config.getThingType(-1).desc, "UNKNOWN TYPE");
}

TEST_F(MGameFixture, MCollectKnownDefs)
{
	//
	// Helper to make dir and put it to stack
	//
	auto makeDir = [this](const fs::path &path)
	{
		ASSERT_TRUE(FileMakeDir(path));
		mDeleteList.push(path);
	};

	//
	// Helper to create empty file, complete with check
	//
	auto makeFile = [this](const fs::path &path)
	{
		std::ofstream os(path);
		ASSERT_TRUE(os.is_open());
		mDeleteList.push(path);
	};

	// We need the install and home dirs for this test
	const fs::path install_dir = getChildPath("install");
	makeDir(install_dir);

	const fs::path home_dir = getChildPath("home");
	makeDir(home_dir);

	// Now add some other folder inside both of them
	const fs::path folder = "conf";
	const fs::path home = home_dir / folder;
	const fs::path install = install_dir / folder;
	makeDir(install);
	makeDir(home);

	// Now add unrelated folders in each
	fs::path unrelated[2] = {install_dir / "unrel", home_dir / "ated"};
	for(const fs::path &unrel : unrelated)
		makeDir(unrel);

	// Now produce all sorts of files
	makeFile(install / "empty");
	makeFile(install / "hasugh.ugh");
	makeFile(install / "DOOM.ugh");
	makeFile(install / "Config.cfg");
	makeFile(install / "MooD.uGH");
	makeFile(home / "doom.UGH");
	makeFile(home / "Heretic.Ugh");
	makeFile(home / "Junk");
	makeFile(home / "extra.cfg");
	makeFile(home / ".hidden.ugh");	// skip for being hidden
	makeDir(home / "directory.ugh");	// skip for being dir
	makeFile(home / "directory.ugh" / "subfile.ugh");	// skip for being under dir
	makeDir(home / "folder");
	makeFile(home / "folder" / "subfile2.ugh");	// make sure to always skip subdir
	makeFile(home / "Extra.ugh");
	makeFile(unrelated[0] / "bandit.ugh");
	makeFile(unrelated[0] / "brigand.ugh");
	makeFile(unrelated[1] / "jackson.ugh");
	makeFile(unrelated[1] / "jordan.cfg");

	// Requirement: the last entry takes precedence and it's sorted.
	// Hidden and dirs are skipped
	auto expected = std::vector<SString>{"doom", "Extra", "hasugh", "Heretic", "MooD"};
	ASSERT_EQ(M_CollectKnownDefs({install_dir, home_dir}, folder), expected);
}

TEST_F(MGameFixture, ParseDefinitionFileClearThingFlags)
{
	std::unordered_map<SString, SString> parseVars;
	ParsePurpose purpose = ParsePurpose::normal;
	fs::path filename = getChildPath("test.ugh");

	std::ofstream stream(filename);
	ASSERT_TRUE(stream.is_open());
	mDeleteList.push(filename);

	stream << "thingflag 0 0 easy    on  0x01" << std::endl;
	stream << "thingflag 1 0 medium  on  0x02" << std::endl;
	stream << "thingflag 2 0 hard    on  0x02" << std::endl;
	stream << "clear thingflags"               << std::endl;
	stream << "thingflag 2 1 ambush  off 0x04" << std::endl;
	stream << "thingflag 3 1 friend  on-opposite 0x08" << std::endl;

	stream.close();

	ConfigData config = {};

	M_ParseDefinitionFile(parseVars, purpose, &config, filename);

	ASSERT_EQ(config.thing_flags.size(), 2);
	ASSERT_EQ(config.thing_flags[0].row, 2);
	ASSERT_EQ(config.thing_flags[0].column, 1);
	ASSERT_EQ(config.thing_flags[0].value, 4);
	ASSERT_EQ(config.thing_flags[0].defaultSet, thingflag_t::DefaultMode::off);
	ASSERT_EQ(config.thing_flags[1].row, 3);
	ASSERT_EQ(config.thing_flags[1].column, 1);
	ASSERT_EQ(config.thing_flags[1].value, 8);
	ASSERT_EQ(config.thing_flags[1].defaultSet, thingflag_t::DefaultMode::onOpposite);
}
