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

#include "m_streams.h"
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
	windowMessages.clear();

	SString path = getChildPath("log.txt");
	LogPrintf("Test message\n");
	LogPrintf("Here it goes\n");
	LogOpenFile(path.c_str());
	mDeleteList.push(path);
	LogPrintf("One more message\n");

    SString savedPath = getChildPath("log2.txt");
    FILE *f = fopen(savedPath.c_str(), "wb");
    ASSERT_NE(f, nullptr);
    mDeleteList.push(savedPath);
    LogSaveTo(f);
    fclose(f);

	LogOpenWindow();
	LogPrintf("Now we're on window\n");
	LogPrintf("And again\n");
	LogClose();

    auto checkFileLines = [](LineFile &file)
    {
        SString line;
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======= START OF LOGS =======");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_TRUE(line.empty());
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Test message");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Here it goes");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "One more message");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Now we're on window");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "And again");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
    };

	{
		LineFile file(path);
		checkFileLines(file);
        SString line;
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======== END OF LOGS ========");
        ASSERT_FALSE(file.readLine(line));
	}

    // Now check
    {
        SString line;
        LineFile file(savedPath);
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======= START OF LOGS =======");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_TRUE(line.empty());
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Test message");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Here it goes");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "One more message");
        ASSERT_FALSE(file.readLine(line));  // stopped here
    }

	ASSERT_EQ(windowMessages.size(), 5);
	ASSERT_EQ(windowMessages[0], "Test message\n");
	ASSERT_EQ(windowMessages[1], "Here it goes\n");
	ASSERT_EQ(windowMessages[2], "One more message\n");
	ASSERT_EQ(windowMessages[3], "Now we're on window\n");
	ASSERT_EQ(windowMessages[4], "And again\n");

    windowMessages.clear();

	// Now write more stuff
	LogPrintf("Extra stuff one\n");
    LogOpenWindow();
	LogPrintf("Extra stuff two\n");
    // Mark fatal error to see it doesn't get added
    global::in_fatal_error = true;
    LogPrintf("Extra stuff three\n");
	ASSERT_EQ(windowMessages.size(), 2);    // it didn't get added to missing window

    // Now check saving current status works
    f = fopen(savedPath.c_str(), "wb");
    ASSERT_NE(f, nullptr);
    LogSaveTo(f);
    fclose(f);


	LogOpenFile(path.c_str());
    LogClose();

    {
        LineFile file(path);
        SString line;
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======= START OF LOGS =======");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_TRUE(line.empty());
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff one");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff two");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff three");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======== END OF LOGS ========");
        ASSERT_FALSE(file.readLine(line));
    }

    {
        LineFile file(savedPath);
        SString line;
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======= START OF LOGS =======");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_TRUE(line.empty());
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff one");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff two");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff three");
        ASSERT_FALSE(file.readLine(line));
    }

    ASSERT_EQ(windowMessages.size(), 2);
    ASSERT_EQ(windowMessages[0], "Extra stuff one\n");
    ASSERT_EQ(windowMessages[1], "Extra stuff two\n");
}

TEST_F(SysDebugTempDir, NewLifeCycle)
{
    std::vector<SString> localWindowMessages;

    Log log;

    log.setWindowAddCallback([](const SString &text, void *userData)
                             {
        auto localWindowMessages = static_cast<std::vector<SString> *>(userData);
        localWindowMessages->push_back(text);
    }, &localWindowMessages);

    SString path = getChildPath("log.txt");
    log.printf("Test message\n");
    log.printf("Here it goes\n");
    ASSERT_TRUE(log.openFile(path.c_str()));
    mDeleteList.push(path);
    log.printf("One more message\n");

    SString savedPath = getChildPath("log2.txt");
    FILE *f = fopen(savedPath.c_str(), "wb");
    ASSERT_NE(f, nullptr);
    mDeleteList.push(savedPath);
    log.saveTo(f);
    fclose(f);

    log.openWindow();
    log.printf("Now we're on window\n");
    log.printf("And again\n");
    log.close();

    auto checkFileLines = [](LineFile &file)
    {
        SString line;
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======= START OF LOGS =======");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_TRUE(line.empty());
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Test message");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Here it goes");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "One more message");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Now we're on window");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "And again");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
    };

    {
        LineFile file(path);
        checkFileLines(file);
        SString line;
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======== END OF LOGS ========");
        ASSERT_FALSE(file.readLine(line));
    }

    // Now check
    {
        SString line;
        LineFile file(savedPath);
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======= START OF LOGS =======");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_TRUE(line.empty());
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Test message");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Here it goes");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "One more message");
        ASSERT_FALSE(file.readLine(line));  // stopped here
    }

    ASSERT_EQ(localWindowMessages.size(), 5);
    ASSERT_EQ(localWindowMessages[0], "Test message\n");
    ASSERT_EQ(localWindowMessages[1], "Here it goes\n");
    ASSERT_EQ(localWindowMessages[2], "One more message\n");
    ASSERT_EQ(localWindowMessages[3], "Now we're on window\n");
    ASSERT_EQ(localWindowMessages[4], "And again\n");

    localWindowMessages.clear();

    // Now write more stuff
    log.printf("Extra stuff one\n");
    log.openWindow();
    log.printf("Extra stuff two\n");
    log.markFatalError();
    log.printf("Extra stuff three\n");
    ASSERT_EQ(localWindowMessages.size(), 2);    // it didn't get added to missing window

    // Now check saving current status works
    f = fopen(savedPath.c_str(), "wb");
    ASSERT_NE(f, nullptr);
    log.saveTo(f);
    fclose(f);


    log.openFile(path.c_str());
    log.close();

    {
        LineFile file(path);
        SString line;
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======= START OF LOGS =======");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_TRUE(line.empty());
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff one");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff two");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff three");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======== END OF LOGS ========");
        ASSERT_FALSE(file.readLine(line));
    }

    {
        LineFile file(savedPath);
        SString line;
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======= START OF LOGS =======");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_TRUE(line.empty());
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff one");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff two");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff three");
        ASSERT_FALSE(file.readLine(line));
    }

    ASSERT_EQ(localWindowMessages.size(), 2);
    ASSERT_EQ(localWindowMessages[0], "Extra stuff one\n");
    ASSERT_EQ(localWindowMessages[1], "Extra stuff two\n");
}



//TEST_F(SysDebugTempDir, WindowThenFile)
//{
//	windowMessages.clear();
//
//	SString path = getChildPath("log.txt");
//	LogPrintf("Test message\n");
//	LogPrintf("Here it goes\n");
//	LogOpenWindow();
//	LogPrintf("One more message\n");
//	LogPrintf("Now we're on window\n");
//	LogOpenFile(path.c_str());
//	mDeleteList.push(path);
//	LogPrintf("And again\n");
//
//	LineFile file(path);
//	SString line;
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_EQ(line, "======= START OF LOGS =======");
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_TRUE(line.empty());
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_EQ(line, "Test message");
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_EQ(line, "Here it goes");
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_EQ(line, "One more message");
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_EQ(line, "Now we're on window");
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_EQ(line, "And again");
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_EQ(line, "");
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_EQ(line, "");
//	ASSERT_TRUE(file.readLine(line));
//	ASSERT_EQ(line, "======== END OF LOGS ========");
//	ASSERT_FALSE(file.readLine(line));
//}
