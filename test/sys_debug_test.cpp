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

#include "sys_debug.h"
#include "testUtils/TempDirContext.hpp"
#include "gtest/gtest.h"

static std::vector<SString> windowMessages;

//
// Temporary directory
//
class SysDebugTempDir : public TempDirContext
{
};

//
// Do nothing
//
void LogViewer_AddLine(const char *str)
{
	windowMessages.push_back(str);
}

TEST_F(SysDebugTempDir, LifeCycle)
{
	SString path = getChildPath("log.txt");
	LogPrintf("Test message\n");
	LogPrintf("Here it goes\n");
	LogOpenFile(path.c_str());
	LogPrintf("One more message\n");
	LogOpenWindow();
	LogPrintf("Now we're on window\n");
	LogPrintf("And again\n");
	LogClose();

	// TODO: read the file and assert here
}
