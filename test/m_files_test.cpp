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
	fs::path wadpath(getChildPath(wadpathbase));
	// Set up a nested directory WAD so we can test relative paths to resources
	std::shared_ptr<Wad_file> wad = Wad_file::Open(wadpath.u8string(), WadOpenMode::write);
	ASSERT_TRUE(wad);

	loaded.gameName = "Mood";	// just pick two random names
	loaded.portName = "Voom";
	loaded.resourceList.push_back(getChildPath(fs::path("first") / "res.png"));	// same path
	loaded.resourceList.push_back(getChildPath(fs::path("first") / "deep" / "call.txt"));	// child path
	loaded.resourceList.push_back(getChildPath("upper.txt"));		// upper path
	loaded.resourceList.push_back(getChildPath(fs::path("second") / "music.mid"));	// sibling path

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

	void prepareHomeDir();
	fs::path makeGamesDir();
	void makeGame(const fs::path &gamesDir, const char *ughName);

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

	wad = Wad_file::Open(getChildPath("wad.wad").u8string(), WadOpenMode::write);
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
	global::home_dir = mTempDir.u8string();

	fs::path miscPath = getChildPath("misc.cfg");
	FILE *f = fopen(miscPath.u8string().c_str(), "wb");
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
 # global::home_dir, global::install_dir (must reset them)
	# test without any or both of them
 # base/<game>.ugh, base/<port>.ugh
	# test without that file, simulate dialog box
 # global::known_iwads (must reset it)
	# test without it
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

//
// Prepares the home directory
//
void ParseEurekaLumpFixture::prepareHomeDir()
{
	global::home_dir = getChildPath("home").u8string();
	ASSERT_TRUE(FileMakeDir(global::home_dir.get()));
	mDeleteList.push(global::home_dir.get());
}

//
// Make the games directory, which it returns
//
fs::path ParseEurekaLumpFixture::makeGamesDir()
{
	fs::path gamesDir = fs::path(global::home_dir.c_str()) / "games";
	EXPECT_TRUE(FileMakeDir(gamesDir));
	mDeleteList.push(gamesDir);
	return gamesDir;
}

//
// Make the game definition in the directory
//
void ParseEurekaLumpFixture::makeGame(const fs::path &gamesDir, const char *ughName)
{
	fs::path ughPath = gamesDir / ughName;
	FILE *f = fopen(ughPath.u8string().c_str(), "wb");
	ASSERT_TRUE(f);
	fclose(f);
	mDeleteList.push(ughPath);
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

TEST_F(ParseEurekaLumpFixture, TryGameAndPort)
{
	Lump_c *eureka = wad->AddLump(EUREKA_LUMP);
	ASSERT_TRUE(eureka);
	eureka->Printf("game emag\n");	// have a game name there
	eureka->Printf("invalid syntax here\n");
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
	ASSERT_TRUE(loading.iwadName.empty());
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_TRUE(gameWarning);
	ASSERT_FALSE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 2: same, but with cancelling
	decision = 1;
	ASSERT_FALSE(loading.parseEurekaLump(wad.get()));

	// Situation 3: add home dir
	prepareHomeDir();
	fs::path gamesDir = makeGamesDir();

	// but no emag.ugh: same problem
	decision = 0;	// no ignore
	gameWarning = portWarning = false;
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_TRUE(loading.iwadName.empty());
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_TRUE(gameWarning);
	ASSERT_FALSE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 4: add emag.ugh. But no known IWAD
	makeGame(gamesDir, "emag.ugh");

	gameWarning = iwadWarning = portWarning = false;
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_TRUE(loading.iwadName.empty());
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_FALSE(gameWarning);
	ASSERT_TRUE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 5: now add the IWAD
	fs::path iwadPath = getChildPath("emag.wad");
	M_AddKnownIWAD(iwadPath.u8string());	// NOTE: no need to add file

	decision = 0;
	gameWarning = iwadWarning = portWarning = false;
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_EQ(loading.iwadName, iwadPath.u8string());
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_FALSE(gameWarning);
	ASSERT_FALSE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 6: now remove the home dir to see it won't be found
	SString goodHomeDir = global::home_dir;
	global::home_dir.clear();
	decision = 1;
	gameWarning = iwadWarning = portWarning = false;
	ASSERT_FALSE(loading.parseEurekaLump(wad.get()));

	// Situation 7: set the install dir instead
	global::install_dir = goodHomeDir;
	decision = 0;
	gameWarning = iwadWarning = portWarning = false;
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_EQ(loading.iwadName, iwadPath.u8string());
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_FALSE(gameWarning);
	ASSERT_FALSE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 8: add port
	global::home_dir = global::install_dir;
	fs::path portsDir = fs::path(global::home_dir.c_str()) / "ports";
	ASSERT_TRUE(FileMakeDir(portsDir));
	mDeleteList.push(portsDir);
	fs::path tropPath = portsDir / "trop.ugh";
	FILE *f = fopen(tropPath.u8string().c_str(), "wb");
	ASSERT_TRUE(f);
	fclose(f);
	mDeleteList.push(tropPath);
	decision = 0;
	gameWarning = iwadWarning = portWarning = false;
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_EQ(loading.iwadName, iwadPath.u8string());
	ASSERT_EQ(loading.portName, "trop");
	ASSERT_FALSE(gameWarning);
	ASSERT_FALSE(iwadWarning);
	ASSERT_FALSE(portWarning);
}

TEST_F(ParseEurekaLumpFixture, TryResources)
{
	Lump_c *eureka = wad->AddLump(EUREKA_LUMP);
	ASSERT_TRUE(eureka);
	eureka->Printf("game doom\n");
	eureka->Printf("resource samepath.wad\n");
	eureka->Printf("resource samepath.wad\n");	// repeat so we check we don't add it again
	eureka->Printf("resource bogus/subpath.wad\n");
	eureka->Printf("resource iwadpath.wad\n");
	eureka->Printf("resource nopath.wad\n");
	eureka->Printf("resource %s\n", getChildPath(fs::path("abs") / "abspath.wad").u8string().c_str());
	eureka->Printf("resource \n");	// add something else to check how we go

	auto makesubfile = [this](const char *path)
	{
		fs::path filepath = getChildPath(path);
		FILE *f = fopen(filepath.u8string().c_str(), "wb");
		ASSERT_TRUE(f);
		fclose(f);
		mDeleteList.push(filepath);
	};

	makesubfile("samepath.wad");
	makesubfile("subpath.wad");

	// Prepare the IWAD
	fs::path path = getChildPath("iwad");
	ASSERT_TRUE(FileMakeDir(path));
	mDeleteList.push(path);
	M_AddKnownIWAD(getChildPath(fs::path("iwad") / "doom.wad").u8string());

	// Prepare the 'game' for the IWAD
	prepareHomeDir();
	makeGame(makeGamesDir(), "doom.ugh");

	// Add the resource at the IWAD path
	makesubfile("iwad/iwadpath.wad");

	// And add the absolute subpath
	path = getChildPath("abs");
	ASSERT_TRUE(FileMakeDir(path));
	mDeleteList.push(path);
	makesubfile("abs/abspath.wad");

	// Prepare the error message
	int errorcount = 0;
	SString errmsg;
	DLG_Notify_Override = [&errorcount, &errmsg](const char *message, va_list ap)
	{
		++errorcount;
		errmsg = SString::vprintf(message, ap);
	};

	loading.parseEurekaLump(wad.get());

	// Check that the sole error message we get is about nopath.wad
	ASSERT_EQ(errorcount, 1);
	ASSERT_NE(errmsg.find("nopath.wad"), SString::npos);

	// Check the resource content
	ASSERT_EQ(loading.resourceList.size(), 4);
	const auto &res = loading.resourceList;

	// Now check we have the resources, with their correct paths
	ASSERT_NE(std::find(res.begin(), res.end(), getChildPath("samepath.wad")), res.end());
	ASSERT_NE(std::find(res.begin(), res.end(), getChildPath("subpath.wad")), res.end());
	ASSERT_NE(std::find(res.begin(), res.end(), getChildPath(fs::path("iwad") / "iwadpath.wad")), res.end());
	ASSERT_NE(std::find(res.begin(), res.end(), getChildPath(fs::path("abs") / "abspath.wad")), res.end());
}

TEST_F(ParseEurekaLumpFixture, TryResourcesParentPath)
{
	// Re-create wad to be from a subpath
	ASSERT_TRUE(FileMakeDir(getChildPath("sub")));
	mDeleteList.push(getChildPath("sub"));
	wad = Wad_file::Open(getChildPath(fs::path("sub") / "wad.wad").u8string(), WadOpenMode::write);
	ASSERT_TRUE(wad);

	// Create a parent path resource
	FILE *f = fopen(getChildPath("res.wad").u8string().c_str(), "wb");
	ASSERT_TRUE(f);
	fclose(f);
	mDeleteList.push(getChildPath("res.wad"));

	// Prepare the lump
	Lump_c *eureka = wad->AddLump(EUREKA_LUMP);
	ASSERT_TRUE(eureka);

	// Try to use just path: won't be found
	eureka->Printf("resource res.wad\n");

	int errorcount = 0;
	DLG_Notify_Override = [&errorcount](const char *message, va_list ap)
	{
		++errorcount;
	};

	loading.parseEurekaLump(wad.get());

	ASSERT_EQ(errorcount, 1);
	ASSERT_TRUE(loading.resourceList.empty());

	// Now change content and add a relative path
	eureka->clearData();
	eureka->Printf("resource ../res.wad\n");
	loading.parseEurekaLump(wad.get());

	ASSERT_EQ(errorcount, 1);
	ASSERT_EQ(loading.resourceList.size(), 1);
	ASSERT_EQ(loading.resourceList[0], getChildPath("res.wad"));
}
