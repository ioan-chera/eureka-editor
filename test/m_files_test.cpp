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

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

#include "gtest/gtest.h"

#define WAD_NAME "EurekaLump.wad"
#define IMMEDIATE_RESOURCE "Michael"
#define NESTED_RESOURCE "Jackson/George"

//
// Writing lump fixture
//
class EurekaLumpFixture : public TempDirContext
{
protected:
	LoadingData loaded;	// keep a convenient pointer to the LoadingData
};

TEST_F(EurekaLumpFixture, WriteEurekaLump)
{
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

	// In case of Windows, also check different drive letter
	fs::path wadpathobj = fs::path(wadpath.c_str());
	char otherdriveletter = '\0';
	if(wadpathobj.has_root_name())
	{
		char c = wadpathobj.u8string()[0];
		c = toupper(c) < 'Z' ? c + 1 : 'A';
		char otherpath[] = "C:\\other\\path";
		otherpath[0] = c;
		otherdriveletter = c;
		loaded.resourceList.push_back(otherpath);
	}

	loaded.writeEurekaLump(wad.get());

	Lump_c *lump = wad->FindLump(EUREKA_LUMP);
	ASSERT_TRUE(lump);

	// Now read the data
	// Expected content is:
	SString expected = 
		"# Eureka project info\n"
		"game Mood\n"
		"port Voom\n"
		"resource res.png\n"
		"resource deep/call.txt\n"
		"resource ../upper.txt\n"
		"resource ../second/music.mid\n"
	;
	if(wadpathobj.has_root_name())
	{
		expected += "resource ";
		expected += otherdriveletter;
		expected += ":/other/path\n";
	}
	SString content(static_cast<const char *>(lump->getData()), lump->Length());
	ASSERT_EQ(content, expected);

	// Now make a change: remove port and resources, see that the lump is updated (not deleted).
	// Add a couple of lumps before and after before doing it, to see how it gets updated alone.
	wad->InsertPoint(0);
	Lump_c *beforelump = wad->AddLump("BEFORE");
	ASSERT_TRUE(beforelump);
	wad->InsertPoint();
	Lump_c *lastlump = wad->AddLump("LAST");
	ASSERT_TRUE(lastlump);
	ASSERT_EQ(wad->NumLumps(), 3);

	loaded.portName.clear();
	loaded.resourceList.clear();
	loaded.writeEurekaLump(wad.get());

	// Check we still have 3 lumps
	ASSERT_EQ(wad->NumLumps(), 3);
	ASSERT_EQ(wad->GetLump(0)->Name(), "BEFORE");
	ASSERT_EQ(wad->GetLump(1)->Name(), "LAST");
	ASSERT_EQ(wad->GetLump(2)->Name(), EUREKA_LUMP);	// moved to the end

	static const char expected2[] = {
		"# Eureka project info\n"
		"game Mood\n"
	};
	lump = wad->GetLump(2);
	content = SString(static_cast<const char *>(lump->getData()), lump->Length());
	ASSERT_EQ(content, expected2);
}

//
// Fixture for parsing. Depends on TempDirContext due to technicalities.
//
class ParseEurekaLumpFixture : public TempDirContext
{
protected:
	void SetUp() override;
	void TearDown() override;
	void assertEmptyLoading() const;

	std::shared_ptr<Wad_file> wad;
	LoadingData loading;

private:
	void clearKnownIwads();
};

//
// Init WAD and other stuff
//
void ParseEurekaLumpFixture::SetUp()
{
	TempDirContext::SetUp();

	wad = Wad_file::Open(getChildPath("wad.wad"), WadOpenMode::write);
	ASSERT_TRUE(wad);
}

//
// Clear all globals
//
void ParseEurekaLumpFixture::TearDown()
{
	global::home_dir.clear();
	global::install_dir.clear();
	clearKnownIwads();
	DLG_Notify_Override = nullptr;
	DLG_Confirm_Override = nullptr;

	TempDirContext::TearDown();
}

//
// Since global::known_iwads is static, we need to do something more complex to access and clear it.
//
void ParseEurekaLumpFixture::clearKnownIwads()
{
	// The only method to clear the structure is M_LoadRecent
	// Prerequisites:
	// - global::home_dir / "misc.cfg": existing empty file

	// Must set home_dir temporarily for this to work
	global::home_dir = mTempDir;

	SString miscPath = getChildPath("misc.cfg");
	FILE *f = fopen(miscPath.c_str(), "wb");
	ASSERT_TRUE(f);
	mDeleteList.push(miscPath);

	int result = fclose(f);
	ASSERT_FALSE(result);

	M_LoadRecent();

	global::home_dir.clear();
}

/*
 parseEurekaLump conditions:
 - Wad_file with written Eureka lump
	# test without lump
	- test with invalid syntax lines
 - global::home_dir, global::install_dir (must reset them)
	- test without any or both of them
 - base/<game>.ugh, base/<port>.ugh
	- test without that file, simulate dialog box
 - global::known_iwads (must reset it)
	- test without it
 - resource paths
	- try relative
	- try absolute
	- try inexistent resources
	- check that we don't add duplicate resources (SAME NAME RULE MAY NEED FIXING)
 */

//
// Common check we didn't load anything
//
void ParseEurekaLumpFixture::assertEmptyLoading() const
{
	ASSERT_TRUE(loading.gameName.empty());
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_TRUE(loading.iwadName.empty());
	ASSERT_TRUE(loading.resourceList.empty());
}

TEST_F(ParseEurekaLumpFixture, TryWithoutLump)
{
	// Safe on empty wad
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	assertEmptyLoading();

	// Safe on wad with some lumps
	Lump_c *lump1 = wad->AddLump("LUMP1");
	ASSERT_TRUE(lump1);
	lump1->Printf("Data 1");

	Lump_c *lump2 = wad->AddLump("LUMP2");
	ASSERT_TRUE(lump2);
	lump2->Printf("Data 2");

	ASSERT_EQ(wad->NumLumps(), 2);

	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	assertEmptyLoading();
}

TEST_F(ParseEurekaLumpFixture, TryGameButNoDefinitions)
{
	Lump_c *eureka = wad->AddLump(EUREKA_LUMP);
	ASSERT_TRUE(eureka);
	eureka->Printf("game emag\n");	// have a game name there
	eureka->Printf("port trop\n");	// add something else to check how we go

	// Situation 1: home, install dir unset
	// Prerequisite: set the DLG_Confirm override
	int decision = 0;
	bool gameWarning = false, iwadWarning = false, portWarning = false;
	DLG_Confirm_Override = [&decision, &gameWarning, &iwadWarning](const std::vector<SString> &,
																   const char *message, va_list)
	{
		if(strstr(message, "game"))
			gameWarning = true;
		else if(strstr(message, "IWAD"))
			iwadWarning = true;
		return decision;
	};
	DLG_Notify_Override = [&portWarning](const char *message, va_list)
	{
		if(strstr(message, "port"))
			portWarning = true;
	};

	// Decide to keep trying, but nothing to get
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_TRUE(loading.gameName.empty());
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_TRUE(gameWarning);
	ASSERT_FALSE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 2: same, but with cancelling
	decision = 1;
	ASSERT_FALSE(loading.parseEurekaLump(wad.get()));

	// Situation 3: add home dir
	global::home_dir = getChildPath("home");
	ASSERT_TRUE(FileMakeDir(global::home_dir));
	mDeleteList.push(global::home_dir);

	fs::path gamesDir = fs::path(global::home_dir.c_str()) / "games";
	ASSERT_TRUE(FileMakeDir(gamesDir.u8string()));
	mDeleteList.push(gamesDir.u8string());

	// but no emag.ugh: same problem
	decision = 0;	// no ignore
	gameWarning = portWarning = false;
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_TRUE(loading.gameName.empty());
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_TRUE(gameWarning);
	ASSERT_FALSE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 4: add emag.ugh. But no known IWAD
	fs::path emagDir = gamesDir / "emag.ugh";
	FILE *f = fopen(emagDir.u8string().c_str(), "wb");
	ASSERT_TRUE(f);
	mDeleteList.push(emagDir.u8string());

	gameWarning = iwadWarning = portWarning = false;
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_TRUE(loading.gameName.empty());
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_FALSE(gameWarning);
	ASSERT_TRUE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 5: now add the IWAD
}
