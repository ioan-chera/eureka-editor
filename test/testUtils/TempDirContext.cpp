//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
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

#include "TempDirContext.hpp"

#ifdef _WIN32
#error TODO Windows
#else
#include <unistd.h>
#endif

// Linux: mkdtemp
// Windows: GetTempPath

void TempDirContext::SetUp()
{
#ifdef _WIN32
#error TODO Windows
#else
	char pattern[] = "tempdirXXXXXX";
	char *result = mkdtemp(pattern);
	ASSERT_NE(result, nullptr);
	mTempDir = result;
#endif
}

TempDirContext::~TempDirContext()
{
	if(mTempDir.good())
		remove(mTempDir.c_str());
}

TEST_F(TempDirContext, Test)
{
	ASSERT_TRUE(mTempDir.good());
}
