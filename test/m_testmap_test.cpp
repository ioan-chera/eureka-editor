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

#include "testUtils/TempDirContext.hpp"

#include "Instance.h"
#include "m_files.h"

#include "gtest/gtest.h"

#ifdef _WIN32
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <chrono>
#include <fstream>
#include <thread>

class TestMapFixture : public TempDirContext
{
protected:
	void SetUp() override;
	void setPortName(const char* name);
	void addIWAD();
	void addResources();
	void addPWAD();
	std::vector<std::string> getResultLines() const;

	std::vector<std::string> testMapAndGetLines();

	Instance inst;
	fs::path outputPath;
	fs::path finishMarkPath;	// empty file added by test script to indicate it's done
	fs::path portPath;
	fs::path gameWadPath;
	fs::path res1Path;
	fs::path res2Path;
	fs::path editWadPath;
	
#ifdef __APPLE__
	fs::path macPath;
#endif
	
private:
#ifndef _WIN32
	void writeShellScript(const fs::path &path);
#endif
};

#ifndef _WIN32
void TestMapFixture::writeShellScript(const fs::path &path)
{
	std::ofstream stream(path);
	ASSERT_TRUE(stream.is_open());
	mDeleteList.push(path);
	stream << "#!/bin/bash" << std::endl;
	stream << "echo running script" << std::endl;
	stream << "echo \"$0\" > " << SString(outputPath.u8string()).spaceEscape(true) << std::endl;
	stream << "for var in \"$@\"" << std::endl;
	stream << "do" << std::endl;
	stream << "echo \"$var\" >> " << SString(outputPath.u8string()).spaceEscape(true) <<
		std::endl;
	stream << "done" << std::endl;
	stream << "echo done > " << SString(finishMarkPath.u8string()).spaceEscape(true) << std::endl;
	stream.close();
	int r = chmod(path.u8string().c_str(), S_IRWXU);
	ASSERT_FALSE(r);
}
#endif

void TestMapFixture::SetUp()
{
	TempDirContext::SetUp();
	outputPath = getChildPath("output.list");
	finishMarkPath = getChildPath("finish.mark");

	// Setup the program
#ifdef _WIN32
	portPath = getChildPath("port.bat");
	std::ofstream stream(portPath);
	ASSERT_TRUE(stream.is_open());
	mDeleteList.push(portPath);
	stream << "@echo off" << std::endl;
	stream << "echo running script" << std::endl;
	stream << "echo %0 > " << SString(outputPath.u8string()).spaceEscape(false) << std::endl;
	stream << "for %%x in (%*) do (" << std::endl;
	stream << "echo %%x >> " << SString(outputPath.u8string()).spaceEscape(false) << std::endl;
	stream << ")" << std::endl;
	stream << "echo done > " << SString(finishMarkPath.u8string()).spaceEscape(false) << std::endl;
	stream.close();
#else
	portPath = getChildPath("port.sh");
	writeShellScript(portPath);
#endif
#ifdef __APPLE__
	macPath = getChildPath("port.app");
	bool result;
	result = FileMakeDir(macPath);
	ASSERT_TRUE(result);
	mDeleteList.push(macPath);
	fs::path contdir = macPath / "Contents";
	result = FileMakeDir(contdir);
	ASSERT_TRUE(result);
	mDeleteList.push(contdir);
	fs::path macosdir = contdir / "MacOS";
	result = FileMakeDir(macosdir);
	ASSERT_TRUE(result);
	mDeleteList.push(macosdir);
	fs::path execpath = macosdir / "portentry";
	writeShellScript(execpath);
	// Now the info plist file
	fs::path infopath = contdir / "Info.plist";
	std::ofstream infostream(infopath);
	ASSERT_TRUE(infostream.is_open());
	mDeleteList.push(infopath);
	infostream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
	infostream << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" << std::endl;
	infostream << "<plist version=\"1.0\">" << std::endl;
	infostream << "<dict>" << std::endl;
	infostream << "<key>CFBundleExecutable</key>" << std::endl;
	infostream << "<string>" << execpath.filename().u8string() << "</string>" << std::endl;
	infostream << "<key>CFBundleIdentifier</key>" << std::endl;
	infostream << "<string>com.eureka.testmaptest</string>" << std::endl;
	infostream << "</dict>" << std::endl;
	infostream << "</plist>" << std::endl;
	infostream.close();
#endif
}


void TestMapFixture::setPortName(const char* name)
{
	// Prepare the conditions
	global::recent.setPortPath(!strcmp(name, "vanilla") ? "vanilla_doom2" : name, portPath);

	// Populate for GrabWadNamesArgs
	inst.loaded.portName = name;
}

void TestMapFixture::addIWAD()
{
	gameWadPath = getChildPath("ga me.wad");
	std::shared_ptr<Wad_file> gameWad = Wad_file::Open(gameWadPath, WadOpenMode::write);
	inst.wad.master.setGameWad(gameWad);
}

void TestMapFixture::addResources()
{
	std::vector<std::shared_ptr<Wad_file>> resources;
	res1Path = getChildPath("re s1.wad");
	res2Path = getChildPath("re s2.wad");
	resources.push_back(Wad_file::Open(res1Path, WadOpenMode::write));
	resources.push_back(Wad_file::Open(res2Path, WadOpenMode::write));
	inst.wad.master.setResources(resources);
}

void TestMapFixture::addPWAD()
{
	editWadPath = getChildPath("ed it.wad");
	std::shared_ptr<Wad_file> editWad = Wad_file::Open(editWadPath, WadOpenMode::write);
	inst.wad.master.ReplaceEditWad(editWad);
}

std::vector<std::string> TestMapFixture::getResultLines() const
{
	// Try until it's open
	for (int i = 0; i < 20; ++i)
	{
		if (fs::exists(finishMarkPath))
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::ifstream input;
	input.open(outputPath);
	EXPECT_TRUE(input.is_open());

	std::vector<std::string> result;
	while (input && !input.eof())
	{
		std::string line;
		std::getline(input, line);
		if (line.empty() && input.eof())
			return result;
		// Windows may add quotes on arguments with spaces
#ifdef _WIN32
		SString changedLine = SString(line);
		changedLine.trimTrailingSpaces();
		line = changedLine.get();
		if (line.length() >= 2 && line[0] == '"' && line.back() == '"')
			line = line.substr(1, line.length() - 2);
#endif
		result.push_back(line);
	}
	return result;
}

std::vector<std::string> TestMapFixture::testMapAndGetLines()
{
	// Now run
	inst.CMD_TestMap();
	mDeleteList.push(outputPath);
	mDeleteList.push(finishMarkPath);

	return getResultLines();
}


TEST_F(TestMapFixture, TestMapVanillaWithResources)
{
	setPortName("vanilla");
	
	addIWAD();
	
	addResources();
	
	addPWAD();
	
	inst.loaded.levelName = "MAP14";
	
	// Now run
	
	std::vector<std::string> lines = testMapAndGetLines();
	std::vector<std::string> expected = {portPath.u8string(), "-iwad", gameWadPath.u8string(), "-merge",
		res1Path.u8string(), res2Path.u8string(), "-file", editWadPath.u8string(), "-warp", "14"};
	ASSERT_EQ(lines, expected);
}

TEST_F(TestMapFixture, TestMapPortWithResources)
{
	setPortName("boom");
	
	addIWAD();
	
	addResources();
	
	addPWAD();
	
	inst.loaded.levelName = "MAP14";
		
	std::vector<std::string> lines = testMapAndGetLines();
	std::vector<std::string> expected = {portPath.u8string(), "-iwad", gameWadPath.u8string(), "-file",
		res1Path.u8string(), res2Path.u8string(), editWadPath.u8string(), "-warp", "14"};
	ASSERT_EQ(lines, expected);
}

TEST_F(TestMapFixture, TestMapPortWithoutResources)
{
	setPortName("boom");
	
	addIWAD();
		
	addPWAD();
	
	inst.loaded.levelName = "MAP14";
	
	std::vector<std::string> lines = testMapAndGetLines();
	std::vector<std::string> expected = {portPath.u8string(), "-iwad", gameWadPath.u8string(), "-file", editWadPath.u8string(), "-warp", "14"};
	ASSERT_EQ(lines, expected);
}

TEST_F(TestMapFixture, TestMapPortWithoutResourcesDoom1Map)
{
	setPortName("boom");
	
	addIWAD();
		
	addPWAD();
	
	inst.loaded.levelName = "E6M9";
	
	std::vector<std::string> lines = testMapAndGetLines();
	std::vector<std::string> expected = {portPath.u8string(), "-iwad", gameWadPath.u8string(), "-file", editWadPath.u8string(), "-warp", "6", "9"};
	ASSERT_EQ(lines, expected);
}

TEST_F(TestMapFixture, TestMapPortWithoutResourcesNonstandardMap)
{
	setPortName("boom");
	
	addIWAD();
		
	addPWAD();
	
	inst.loaded.levelName = "ZOMFG65";
		
	std::vector<std::string> lines = testMapAndGetLines();
	std::vector<std::string> expected = {portPath.u8string(), "-iwad", gameWadPath.u8string(), "-file", editWadPath.u8string(), "-warp", "65"};
	ASSERT_EQ(lines, expected);
}

TEST_F(TestMapFixture, TestMapPortWithoutResourcesBadMap)
{
	setPortName("boom");
	
	addIWAD();
		
	addPWAD();
	
	inst.loaded.levelName = "NOTHING";
	
	std::vector<std::string> lines = testMapAndGetLines();
	std::vector<std::string> expected = {portPath.u8string(), "-iwad", gameWadPath.u8string(), "-file", editWadPath.u8string()};
	ASSERT_EQ(lines, expected);
}

// TODO: add mac app bundle test
