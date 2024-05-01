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
#include "testUtils/TempDirContext.hpp"
#include "gtest/gtest.h"

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
	stream << "Patch file for Dehacked v3.1" << std::endl;
	stream << "# Here is the file" << std::endl;
	stream << std::endl;
	stream << "Thing 2 (Imp Sarge Trooper Vanilla Shadow Ceiling Weapon)" << std::endl;
	stream << "Initial frame = 442" << std::endl;
	stream << "Width = 655360" << std::endl;
	stream << "ID # = 2222" << std::endl;
	stream << "Bits = 262400" << std::endl;
	stream << "#$Editor category = Weapons" << std::endl;
	stream << std::endl;
	stream << "Frame 442" << std::endl;
	stream << "Sprite number = 31" << std::endl;
	stream << "Sprite subnumber = 32769" << std::endl;
	stream << std::endl;
	stream << "Text 4 4" << std::endl;
	stream << "VILEABCD" << std::endl;
	stream << std::endl;
	stream << "[SPRITES]" << std::endl;
	stream << "30 = NEWB" << std::endl;
	stream << std::endl;
	stream << "Thing 3 (Newbie)" << std::endl;
	stream << "Bits = SPAWNCEILING+SHADOW" << std::endl;
}

TEST_F(DehackedTest, LoadNoFile)
{
	ConfigData config;
	ASSERT_FALSE(dehacked::loadFile("bogus.deh", config));
}

TEST_F(DehackedTest, LoadFile)
{
	makeFile();
	
	ConfigData config;
	thingtype_t *type = &config.thing_types[3004];
	type->group = 'm';
	type->flags = 0;
	type->radius = 20.0f;
	type->desc = "Trooper";
	type->sprite = "POSS";
	
	type = &config.thing_types[9];
	type->group = 'm';
	type->flags = 0;
	type->radius = 20.0f;
	type->desc = "Sergeant";
	type->sprite = "SPOS";
	
	bool result = dehacked::loadFile(getChildPath("tested.deh"), config);
	ASSERT_TRUE(result);
	
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
