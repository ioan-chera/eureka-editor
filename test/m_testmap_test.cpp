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

class TestMapFixture : public TempDirContext
{
};

#ifdef _WIN32
#else
TEST_F(TestMapFixture, TestMapOnPOSIX)
{
	Instance inst;
	
	static const fs::path execname = "port.sh";
	
	fs::path outputFile = getChildPath("output.list");
	
	// Setup the program
	fs::path path = getChildPath(execname);
	std::ofstream stream(path);
	ASSERT_TRUE(stream.is_open());
	mDeleteList.push(path);
	stream << "#!/bin/bash" << std::endl;
	stream << "echo running script" << std::endl;
	stream << "echo \"$0\" > " << SString(path.u8string()).spaceEscape(true) << std::endl;
	stream << "for var in \"$@\"" << std::endl;
	stream << "do" << std::endl;
	stream << "echo \"$var\" >> " << SString(outputFile).spaceEscape(true) <<
	std::endl;
	stream << "done" << std::endl;
	stream.close();
	int r = chmod(path.u8string().c_str(), S_IRWXU);
	ASSERT_FALSE(r);
	
	// Prepare the conditions
	global::recent.setPortPath("vanilla_doom2", path);
	
	// Populate for GrabWadNamesArgs
	inst.loaded.portName = "vanilla";
	// TODO: also non-vanilla
	fs::path gameWadPath = getChildPath("game.wad");
	std::shared_ptr<Wad_file> gameWad = Wad_file::Open(gameWadPath, WadOpenMode::write);
	inst.wad.master.setGameWad(gameWad);
	
	std::vector<std::shared_ptr<Wad_file>> resources;
	fs::path res1Path = getChildPath("res1.wad");
	fs::path res2Path = getChildPath("res2.wad");
	resources.push_back(Wad_file::Open(res1Path, WadOpenMode::write));
	resources.push_back(Wad_file::Open(res2Path, WadOpenMode::write));
	inst.wad.master.setResources(resources);
	// TODO: also no resources
	
	fs::path editWadPath = getChildPath("edit.wad");
	std::shared_ptr<Wad_file> editWad = Wad_file::Open(editWadPath, WadOpenMode::write);
	inst.wad.master.ReplaceEditWad(editWad);
	
	inst.loaded.levelName = "MAP14";
	// TODO: also ExMy, also nonstandard, also something else
	
	// Now run
	inst.CMD_TestMap();
	
	mDeleteList.push(outputFile);

	// Try until it's open
	std::ifstream input;
	for(int i = 0; i < 20; ++i)
	{
		input.open(outputFile);
		if(input.is_open())
			break;
		usleep(100000);	// retry
	}
	ASSERT_TRUE(input.is_open());
	
	int index = 0;
	std::vector<std::string> expected = {path.u8string(), "-iwad", gameWadPath.u8string(), "-merge", 
		res1Path.u8string(), res2Path.u8string(), "-file", editWadPath.u8string(), "-warp", "14"};
	while(!input.eof())
	{
		std::string line;
		std::getline(input, line);
		if(index >= (int)expected.size())
		{
			ASSERT_TRUE(line.empty());
			ASSERT_TRUE(input.eof());
			continue;
		}
		ASSERT_EQ(line, expected[index]);
		++index;
	}
}
#endif
