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

#include <fstream>

#ifdef _WIN32
#else
class TestMapFixturePOSIX : public TempDirContext
{
protected:
	void SetUp() override;
	void setPortName(const char *name);
	void addIWAD();
	void addResources();
	void addPWAD();
	std::vector<std::string> getResultLines() const;
	
	Instance inst;
	fs::path outputPath;
	fs::path portPath;
	fs::path gameWadPath;
	fs::path res1Path;
	fs::path res2Path;
	fs::path editWadPath;
};

void TestMapFixturePOSIX::SetUp()
{
	TempDirContext::SetUp();
	outputPath = getChildPath("output.list");
	
	// Setup the program
	portPath = getChildPath("port.sh");
	std::ofstream stream(portPath);
	ASSERT_TRUE(stream.is_open());
	mDeleteList.push(portPath);
	stream << "#!/bin/bash" << std::endl;
	stream << "echo running script" << std::endl;
	stream << "echo \"$0\" > " << SString(portPath.u8string()).spaceEscape(true) << std::endl;
	stream << "for var in \"$@\"" << std::endl;
	stream << "do" << std::endl;
	stream << "echo \"$var\" >> " << SString(outputPath).spaceEscape(true) <<
	std::endl;
	stream << "done" << std::endl;
	stream.close();
	int r = chmod(portPath.u8string().c_str(), S_IRWXU);
	ASSERT_FALSE(r);
}

void TestMapFixturePOSIX::setPortName(const char *name)
{
	// Prepare the conditions
	global::recent.setPortPath(!strcmp(name, "vanilla") ? "vanilla_doom2" : name, portPath);
	
	// Populate for GrabWadNamesArgs
	inst.loaded.portName = name;
}

void TestMapFixturePOSIX::addIWAD()
{
	gameWadPath = getChildPath("game.wad");
	std::shared_ptr<Wad_file> gameWad = Wad_file::Open(gameWadPath, WadOpenMode::write);
	inst.wad.master.setGameWad(gameWad);
}

void TestMapFixturePOSIX::addResources()
{
	std::vector<std::shared_ptr<Wad_file>> resources;
	res1Path = getChildPath("res1.wad");
	res2Path = getChildPath("res2.wad");
	resources.push_back(Wad_file::Open(res1Path, WadOpenMode::write));
	resources.push_back(Wad_file::Open(res2Path, WadOpenMode::write));
	inst.wad.master.setResources(resources);
}

void TestMapFixturePOSIX::addPWAD()
{
	editWadPath = getChildPath("edit.wad");
	std::shared_ptr<Wad_file> editWad = Wad_file::Open(editWadPath, WadOpenMode::write);
	inst.wad.master.ReplaceEditWad(editWad);
}

std::vector<std::string> TestMapFixturePOSIX::getResultLines() const
{
	// Try until it's open
	usleep(200000);	// sleep at the beginning to wait for file to be completely written
	std::ifstream input;
	for(int i = 0; i < 20; ++i)
	{
		
		input.open(outputPath);
		if(input.is_open())
			break;
		usleep(100000);	// retry
	}
	EXPECT_TRUE(input.is_open());
	
	std::vector<std::string> result;
	while(!input.eof())
	{
		std::string line;
		std::getline(input, line);
		if(line.empty() && input.eof())
			return result;
		result.push_back(line);
	}
	return result;
}

TEST_F(TestMapFixturePOSIX, TestMapVanillaWithResources)
{
	setPortName("vanilla");
	
	addIWAD();
	
	addResources();
	// TODO: also no resources
	
	addPWAD();
	
	inst.loaded.levelName = "MAP14";
	// TODO: also ExMy, also nonstandard, also something else
	
	// Now run
	inst.CMD_TestMap();
	mDeleteList.push(outputPath);
	
	std::vector<std::string> lines = getResultLines();
	std::vector<std::string> expected = {portPath.u8string(), "-iwad", gameWadPath.u8string(), "-merge",
		res1Path.u8string(), res2Path.u8string(), "-file", editWadPath.u8string(), "-warp", "14"};
	ASSERT_EQ(lines, expected);
}

TEST_F(TestMapFixturePOSIX, TestMapPortWithResources)
{
	setPortName("boom");
	
	addIWAD();
	
	addResources();
	// TODO: also no resources
	
	addPWAD();
	
	inst.loaded.levelName = "MAP14";
	// TODO: also ExMy, also nonstandard, also something else
	
	// Now run
	inst.CMD_TestMap();
	mDeleteList.push(outputPath);
	
	std::vector<std::string> lines = getResultLines();
	std::vector<std::string> expected = {portPath.u8string(), "-iwad", gameWadPath.u8string(), "-file",
		res1Path.u8string(), res2Path.u8string(), editWadPath.u8string(), "-warp", "14"};
	ASSERT_EQ(lines, expected);
}

#endif
