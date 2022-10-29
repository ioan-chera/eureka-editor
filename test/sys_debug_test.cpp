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

//
// Temporary directory
//
class SysDebugTempDir : public TempDirContext
{
};

TEST_F(SysDebugTempDir, LifeCycle)
{
    std::vector<SString> localWindowMessages;
    global::Debugging = false;
    global::in_fatal_error = false;

    Log log;

    fs::path path = getChildPath("log.txt");
    log.printf("Test message\n");
    log.printf("Here it goes\n");
    log.debugPrintf("No text\n");
    ASSERT_TRUE(log.openFile(path.u8string()));
    mDeleteList.push(path);
    log.printf("One more message\n");

    fs::path savedPath = getChildPath("log2.txt");
    std::ofstream os(savedPath, std::ios::trunc);
    ASSERT_TRUE(os.is_open());
    mDeleteList.push(savedPath);
    log.saveTo(os);
    os.close();

    log.openWindow([](const SString &text, void *userData)
                   {
        auto localWindowMessages = static_cast<std::vector<SString> *>(userData);
        localWindowMessages->push_back(text);
    }, &localWindowMessages);
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
        LineFile file(path.u8string());
        checkFileLines(file);
        SString line;
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======== END OF LOGS ========");
        ASSERT_FALSE(file.readLine(line));
    }

    // Now check
    {
        SString line;
        LineFile file(savedPath.u8string());
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
    global::Debugging = true;
    log.debugPrintf("No print here\n");
    // Mark fatal error to see it doesn't get added
    log.markFatalError();
    log.printf("Extra stuff three\n");
    ASSERT_EQ(localWindowMessages.size(), 2);    // it didn't get added to missing window

    // Now check saving current status works
    os.open(savedPath, std::ios::trunc);
    ASSERT_TRUE(os.is_open());
    log.saveTo(os);
    os.close();


    log.openFile(path.u8string());
    log.debugPrintf("Debug writeout\n");
    log.printf("Extra stuff four\n");
    log.debugPrintf("Debug\nwriteout2\n");
    log.debugPrintf("%s", "");    // this shall not be printed!
    global::Debugging = false;
    log.debugPrintf("Debug writeout3\n");
    log.debugPrintf("Debug writeout4\n");
    log.printf("Extra stuff five\n");
    log.close();

    {
        LineFile file(path.u8string());
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
        ASSERT_EQ(line, "# Debug writeout");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff four");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "# Debug");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "# writeout2");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "Extra stuff five");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "");
        ASSERT_TRUE(file.readLine(line));
        ASSERT_EQ(line, "======== END OF LOGS ========");
        ASSERT_FALSE(file.readLine(line));
    }

    {
        LineFile file(savedPath.u8string());
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
