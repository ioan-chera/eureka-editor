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

#include "gtest/gtest.h"
#include "testUtils/TempDirContext.hpp"

#include "Instance.h"

#include <fstream>

class MEventsFixture : public TempDirContext
{
protected:
    void SetUp() override
    {
        TempDirContext::SetUp();
        prevInstallDir = global::install_dir;
        prevHomeDir = global::home_dir;
        prevOldHomeDir = global::old_linux_home_and_cache_dir;
    }
    void TearDown() override
    {
        global::install_dir = prevInstallDir;
        global::home_dir = prevHomeDir;
        global::old_linux_home_and_cache_dir = prevOldHomeDir;
        TempDirContext::TearDown();
    }

private:
    fs::path prevInstallDir;
    fs::path prevHomeDir;
    fs::path prevOldHomeDir;
};

TEST_F(MEventsFixture, ParseOperationFileOldPathFallback)
{
    Instance inst;
    global::home_dir = getSubPath("homedir");
    ASSERT_TRUE(FileMakeDir(global::home_dir));
    mDeleteList.push(global::home_dir);

    global::old_linux_home_and_cache_dir = getSubPath("olddir");
    ASSERT_TRUE(FileMakeDir(global::old_linux_home_and_cache_dir));
    mDeleteList.push(global::old_linux_home_and_cache_dir);

    global::install_dir = mTempDir;

    std::ofstream os(global::old_linux_home_and_cache_dir / "operations.cfg");
    os.close();
    mDeleteList.push(global::old_linux_home_and_cache_dir / "operations.cfg");

    inst.M_LoadOperationMenus();

    ASSERT_FALSE(inst.no_operation_cfg);
}
