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

#ifndef TempDirContext_hpp
#define TempDirContext_hpp

#include "m_strings.h"
#include "gtest/gtest.h"

#include <stack>

class TempDirContext : public ::testing::Test
{
protected:
	void SetUp() override;
	void TearDown() override;
	SString getChildPath(const char *path);

	SString mTempDir;
	std::stack<SString> mDeleteList;
};

#endif /* TempDirContext_hpp */
