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

#include "testUtils/TempDirContext.hpp"

#include "m_files.h"
#include "m_loadsave.h"

#include "gtest/gtest.h"

#define WAD_NAME "EurekaLump.wad"
#define IMMEDIATE_RESOURCE "Michael"
#define NESTED_RESOURCE "Jackson/George"

class EurekaLumpFixture : public TempDirContext
{
protected:
	void SetUp() override;

	LoadingData loaded;
//	std::shared_ptr<Wad_file> wad;
};

void EurekaLumpFixture::SetUp()
{
	TempDirContext::SetUp();

//	wad = Wad_file::Open(getChildPath(WAD_NAME), WadOpenMode::write);
//	ASSERT_TRUE(wad);
	// no writing to disk
}

TEST_F(EurekaLumpFixture, WriteEurekaLumpResourcesRelatively)
{
	static const char wadpathdir[] = "first";
	static const char wadpathbase[] = "first/wad.wad";
	SString wadpath(getChildPath(wadpathbase));
	// Set up a nested directory WAD so we can test relative paths to resources
	std::shared_ptr<Wad_file> wad = Wad_file::Open(wadpath, WadOpenMode::write);
	ASSERT_TRUE(wad);

	loaded.gameName = "Mood";	// just pick two random names
	loaded.portName = "Voom";
	loaded.resourceList.push_back(getChildPath("first/res.png"));	// same path
	loaded.resourceList.push_back(getChildPath("first/deep/call.txt"));	// child path
	loaded.resourceList.push_back(getChildPath("upper.txt"));		// upper path
	loaded.resourceList.push_back(getChildPath("second/music.mid"));	// sibling path

	// Prerequisite: make directory for wad
	bool result = FileMakeDir(getChildPath(wadpathdir));
	ASSERT_TRUE(result);
	mDeleteList.push(getChildPath(wadpathdir));

	loaded.writeEurekaLump(wad.get());
	mDeleteList.push(wadpath);

	// Now test the result
	std::shared_ptr<Wad_file> verify = Wad_file::Open(wadpath, WadOpenMode::read);
	ASSERT_TRUE(wad);

	Lump_c *lump = verify->FindLump(EUREKA_LUMP);
	ASSERT_TRUE(lump);

	// Now read the data
	// Expected content is:
	static const char expected[] = {
		"# Eureka project info\n"
		"game Mood\n"
		"port Voom\n"
		"resource res.png\n"
		"resource deep/call.txt\n"
		"resource ../upper.txt\n"
		"resource ../second/music.mid\n"
	};
	SString content(static_cast<const char *>(lump->getData()), lump->Length());
	ASSERT_EQ(content, expected);
}

TEST_F(EurekaLumpFixture, OldEurekaLumpIsReplaced)
{
	// TODO
}
