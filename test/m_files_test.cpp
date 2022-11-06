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
#include "m_parse.h"
#include "m_streams.h"

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
	global::known_iwads.clear();
	DLG_Notify_Override = nullptr;
	DLG_Confirm_Override = nullptr;

	TempDirContext::TearDown();
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
	global::home_dir = getChildPath("home");
	ASSERT_TRUE(FileMakeDir(global::home_dir));
	mDeleteList.push(global::home_dir);
}

//
// Make the games directory, which it returns
//
fs::path ParseEurekaLumpFixture::makeGamesDir()
{
	fs::path gamesDir = global::home_dir / "games";
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
	M_AddKnownIWAD(iwadPath);	// NOTE: no need to add file

	decision = 0;
	gameWarning = iwadWarning = portWarning = false;
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_EQ(loading.iwadName, iwadPath);
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_FALSE(gameWarning);
	ASSERT_FALSE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 6: now remove the home dir to see it won't be found
	fs::path goodHomeDir = global::home_dir;
	global::home_dir.clear();
	decision = 1;
	gameWarning = iwadWarning = portWarning = false;
	ASSERT_FALSE(loading.parseEurekaLump(wad.get()));

	// Situation 7: set the install dir instead
	global::install_dir = goodHomeDir;
	decision = 0;
	gameWarning = iwadWarning = portWarning = false;
	ASSERT_TRUE(loading.parseEurekaLump(wad.get()));
	ASSERT_EQ(loading.iwadName, iwadPath);
	ASSERT_TRUE(loading.portName.empty());
	ASSERT_FALSE(gameWarning);
	ASSERT_FALSE(iwadWarning);
	ASSERT_TRUE(portWarning);

	// Situation 8: add port
	global::home_dir = global::install_dir;
	fs::path portsDir = global::home_dir / "ports";
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
	ASSERT_EQ(loading.iwadName, iwadPath);
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
	M_AddKnownIWAD(getChildPath(fs::path("iwad") / "doom.wad"));

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

TEST_F(ParseEurekaLumpFixture, ResourcesAreUniqueByFileNameNoCase)
{
	Lump_c *eureka = wad->AddLump(EUREKA_LUMP);
	ASSERT_TRUE(eureka);
	eureka->Printf("game doom\n");
	eureka->Printf("resource samename.wad\n");
	eureka->Printf("resource sub/SameName.Wad\n");	// repeat so we check we don't add it again
	eureka->Printf("resource othername.wad\n");

	// Make the files
	std::ofstream stream;
	fs::path path = getChildPath("samename.wad");
	stream.open(path);
	ASSERT_TRUE(stream.is_open());
	stream.close();
	mDeleteList.push(path);
	path = getChildPath("sub");
	ASSERT_TRUE(FileMakeDir(path));
	mDeleteList.push(path);
	path = getChildPath("sub/SameName.Wad");
	stream.open(path);
	ASSERT_TRUE(stream.is_open());
	stream.close();
	mDeleteList.push(path);
	path = getChildPath("othername.wad");
	stream.open(path);
	ASSERT_TRUE(stream.is_open());
	stream.close();
	mDeleteList.push(path);
	stream.close();

	// Prepare the IWAD
	path = getChildPath("iwad");
	ASSERT_TRUE(FileMakeDir(path));
	mDeleteList.push(path);
	M_AddKnownIWAD(getChildPath("iwad/doom.wad"));

	// Prepare the 'game' for the IWAD
	prepareHomeDir();
	makeGame(makeGamesDir(), "doom.ugh");

	loading.parseEurekaLump(wad.get());
	ASSERT_EQ(loading.resourceList.size(), 2);
	ASSERT_EQ(loading.resourceList[0], getChildPath("samename.wad"));	// not the second one too
	ASSERT_EQ(loading.resourceList[1], getChildPath("othername.wad"));
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

// Recent files

TEST(RecentFiles, StartEmpty)
{
	RecentFiles_c files;
	ASSERT_EQ(files.getSize(), 0);

	// Clear it: still empty
	files.clear();
	ASSERT_EQ(files.getSize(), 0);
}

TEST(RecentFiles, InsertAndLookup)
{
	RecentFiles_c files;
	files.insert("Wad1.wad", "MAP01");
	files.insert("Sub/Doo.wad", "E3M6");
	files.insert("SomeOther.wad", "E4M4");
	files.insert("Jack.wad", "H5M5");
	files.insert("Other/Jack.wad", "H5M6");	// will override previous map
	files.insert("Doo.wad", "MAP03");	// this will override second map

	// Size must be 3
	ASSERT_EQ(files.getSize(), 4);	// Five made it

	fs::path file;
	SString map;

	files.Lookup(0, &file, &map);
	ASSERT_EQ(file, "Doo.wad");
	ASSERT_EQ(map, "MAP03");

	files.Lookup(1, &file, &map);
	ASSERT_EQ(file, "Other/Jack.wad");
	ASSERT_EQ(map, "H5M6");

	files.Lookup(2, &file, &map);
	ASSERT_EQ(file, "SomeOther.wad");
	ASSERT_EQ(map, "E4M4");

	files.Lookup(3, &file, &map);
	ASSERT_EQ(file, "Wad1.wad");
	ASSERT_EQ(map, "MAP01");

	// See one format
	SString text = files.Format(1);
	ASSERT_EQ(text, "  &2:  Jack.wad");

	// Get one data
	recent_file_data_c *vdata = files.getData(1);
	ASSERT_TRUE(vdata);
	ASSERT_EQ(vdata->file, "Other/Jack.wad");
	ASSERT_EQ(vdata->map, "H5M6");
	delete vdata;

	// Test clearing it
	files.clear();
	ASSERT_EQ(files.getSize(), 0);
}

TEST(RecentFiles, InsertPastCap)
{
	RecentFiles_c files;
	for(int i = 0; i < MAX_RECENT + 3; ++i)
	{
		files.insert(fs::path("sub") / SString::printf("Wad%d.wad", i).get(), SString::printf("MAP%d", i));
	}

	// Still max recent entries
	ASSERT_EQ(files.getSize(), MAX_RECENT);

	// Check that the latest recent are there
	fs::path file;
	SString map;
	for(int i = 0; i < MAX_RECENT; ++i)
	{
		files.Lookup(i, &file, &map);
		ASSERT_EQ(SString(file.generic_u8string()), SString::printf("sub/Wad%d.wad", MAX_RECENT + 3 - i - 1));
		ASSERT_EQ(map, SString::printf("MAP%d", MAX_RECENT + 3 - i - 1));
	}

	// Test formatting when larger than 9
	SString text;
	for(int i = 0; i < 9; ++i)
	{
		text = files.Format(i);
		ASSERT_EQ(text, SString::printf("  &%d:  Wad%d.wad", i + 1, MAX_RECENT + 3 - i - 1));
	}
	for(int i = 9; i < MAX_RECENT; ++i)
	{
		text = files.Format(i);
		ASSERT_EQ(text, SString::printf("%d:  Wad%d.wad", i + 1, MAX_RECENT + 3 - i - 1));
	}
}

class RecentFilesFixture : public TempDirContext
{
};

TEST_F(RecentFilesFixture, WriteFile)
{
	RecentFiles_c files;
	files.insert("Wad1.wad", "MAP01");
	files.insert("Sub/Doo.wad", "E3M6");
	files.insert("SomeOther.wad", "E4 M4");
	files.insert("Jack.wad", "H5M5");
	files.insert("Oth er/Jack.wad", "H5M6");	// will override previous map
	files.insert("Doo.wad", "MAP03");	// this will override second map

	fs::path datapath = getChildPath("data.ini");

	std::ofstream os(datapath);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(datapath);

	files.Write(os);

	os.close();

	std::ifstream stream(datapath);
	ASSERT_TRUE(stream.is_open());
	
	std::string keyword, map, file;
	stream >> keyword >> map >> file;
	ASSERT_EQ(keyword, "recent");
	ASSERT_EQ(map, "MAP01");
	ASSERT_EQ(file, "Wad1.wad");
	std::string line;
	std::getline(stream, line);	// get past current line
	std::getline(stream, line);
	ASSERT_EQ(line, "recent \"E4 M4\" SomeOther.wad");
	std::getline(stream, line);
	ASSERT_EQ(line, "recent H5M6 \"Oth er/Jack.wad\"");
	stream >> keyword >> map >> file;
	ASSERT_EQ(keyword, "recent");
	ASSERT_EQ(map, "MAP03");
	ASSERT_EQ(file, "Doo.wad");

	ASSERT_FALSE(stream.eof());
	stream >> keyword;
	ASSERT_TRUE(stream.eof());

	stream.close();
}

TEST_F(RecentFilesFixture, MLoadRecent)
{
	fs::path home_dir = mTempDir;
	RecentFiles_c files;
	std::map<SString, fs::path> known_iwads;
	std::map<SString, port_path_info_t> port_paths;

	// TODO: write the files
	// TODO: write the necessary IWADs
	fs::path path = getChildPath("misc.cfg");
	std::ofstream stream(path);
	ASSERT_TRUE(stream.is_open());
	mDeleteList.push(path);

	stream << "# Misc.cfg file" << std::endl;
	stream << "# recent E1M1 doom.wad" << std::endl;	// ignore comment
	stream << " " << std::endl;	// ignore blank line
	stream << "  # comment after line" << std::endl;	// ignore comment after space


	fs::path hticPath = getChildPath("htic.wad");
	fs::path doom3Path = getChildPath("doom3.wad");
	fs::path freedoomPath = getChildPath("freedoom.wad");
	fs::path deletedPath = getChildPath("deleted.wad");
	fs::path badPath = getChildPath("bad.wad");
	fs::path hereticPath = getChildPath("here tic.wad");

	stream << "recent MAP02 " << escape(doom3Path) << " # recent" << std::endl;
	stream << "recent H5M6 " << std::endl;	// malformed
	stream << "recent E1M1 " << escape(badPath) << std::endl;	// invalid wad
	stream << "recent E3M5 " << escape(hereticPath) << std::endl;
	stream << "recent \"E3 M5\" " << escape(hticPath) << std::endl;
	stream << "recent E7M7 " << escape(deletedPath) << std::endl;	// missing wad
	stream << "recent MAP03 " << escape(doom3Path) << std::endl;	// should overwrite the other Doom3 wad recent
	
	stream << "known_iwad heretic " << escape(hticPath) << std::endl;
	stream << "known_iwad mood " << std::endl;	// malformed
	stream << "known_iwad \"doom \"\"3\"\"\" " << escape(doom3Path) << std::endl;
	stream << "known_iwad FreeDoom " << escape(freedoomPath) << std::endl;	// ignore freedoom
	stream << "known_iwad inexistent " << escape(deletedPath) << std::endl;	// file not found
	stream << "known_iwad malformed " << escape(badPath) << std::endl;	// bad file (both cases should be "invalid"
	stream << "known_iwad heretic " << escape(hereticPath) << std::endl;	// overwrite
	
	stream << "port_path boom |/home/jillson/boom.wad" << std::endl;
	stream << "port_path zdoom |/home/jillson/zdoom.wad" << std::endl;
	stream << "port_path goom /home/jillson/goom.wad" << std::endl;	// malformed
	stream << "port_path zdoom | /home/jackson/zdoom.wad" << std::endl;	// overwrite

	stream.close();

	// Prepare the IWAD files
	for(const fs::path &path : { hticPath, doom3Path, freedoomPath, badPath, hereticPath })
	{
		std::ofstream wadstream(path, std::ios::binary);
		ASSERT_TRUE(wadstream.is_open());
		mDeleteList.push(path);
		if(path != badPath)
			wadstream.write("PWAD\0\0\0\0\x0c\0\0\0", 12);
		ASSERT_FALSE(wadstream.fail());
		wadstream.close();
	}

	M_LoadRecent(home_dir, files, known_iwads, port_paths);

	// Check the recent files
	ASSERT_EQ(files.getSize(), 3);
	SString map;

	files.Lookup(0, &path, &map);
	ASSERT_EQ(path, doom3Path);
	ASSERT_EQ(map, "MAP03");

	files.Lookup(1, &path, &map);
	ASSERT_EQ(path, hticPath);
	ASSERT_EQ(map, "E3 M5");

	files.Lookup(2, &path, &map);
	ASSERT_EQ(path, hereticPath);
	ASSERT_EQ(map, "E3M5");

	// Check the known IWADs map
	ASSERT_EQ(known_iwads.size(), 2);
	ASSERT_EQ(known_iwads["heretic"], hereticPath);
	ASSERT_EQ(known_iwads["doom \"3\""], doom3Path);

	// Check the port paths
	ASSERT_EQ(port_paths.size(), 2);
	ASSERT_EQ(port_paths["zdoom"].exe_filename, "/home/jackson/zdoom.wad");
	ASSERT_EQ(port_paths["boom"].exe_filename, "/home/jillson/boom.wad");
}

TEST_F(RecentFilesFixture, MSaveRecent)
{
	RecentFiles_c recent_files;
	recent_files.insert("file1", "map1");
	recent_files.insert("file2", "map 2");
	recent_files.insert("file1/file 4", "map #");

	std::map<SString, fs::path> known_iwads;
	known_iwads["doom1"] = "path/doom1.wad";
	known_iwads["doom#"] = "path/doom 2.wad";

	std::map<SString, port_path_info_t> port_paths;
	port_paths["foom"].exe_filename = "port/foom.exe";
	port_paths["joom generation"].exe_filename = "port/joom generation.exe";

	M_SaveRecent(mTempDir, recent_files, known_iwads, port_paths);
	mDeleteList.push(mTempDir / "misc.cfg");

	// Now check
	std::ifstream is(mTempDir / "misc.cfg");
	ASSERT_TRUE(is.is_open());

	SString line;
	RecentFiles_c readRecentFiles;
	std::map<SString, fs::path> readKnownIwads;
	std::map<SString, port_path_info_t> readPortPaths;
	while(M_ReadTextLine(line, is))
	{
		TokenWordParse parse(line);
		SString keyword;
		if(!parse.getNext(keyword))
			continue;
		SString name;
		fs::path path;
		ASSERT_TRUE(parse.getNext(name));
		ASSERT_TRUE(parse.getNext(path));
		if(keyword == "recent")
		{
			readRecentFiles.insert(path, name);
			continue;
		}
		if(keyword == "known_iwad")
		{
			readKnownIwads[name] = path;
			continue;
		}
		if(keyword == "port_path")
		{
			readPortPaths[name].exe_filename = path;
			continue;
		}
	}
	// Now check content
	ASSERT_EQ(readRecentFiles.getSize(), 3);
	for(int i = 0; i < readRecentFiles.getSize(); ++i)
	{
		SString myMap, readMap;
		fs::path myPath, readPath;
		recent_files.Lookup(i, &myPath, &myMap);
		readRecentFiles.Lookup(i, &readPath, &readMap);
		ASSERT_EQ(myMap, readMap);
		ASSERT_EQ(myPath, readPath);
	}
	ASSERT_EQ(known_iwads, readKnownIwads);
	ASSERT_EQ(port_paths.size(), readPortPaths.size());
	for(const auto &item : port_paths)
	{
		ASSERT_EQ(port_paths[item.first].exe_filename, item.second.exe_filename);
	}
}
