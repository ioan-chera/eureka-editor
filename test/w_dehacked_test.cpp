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

#include "w_dehacked.h"
#include "m_game.h"
#include "testUtils/TempDirContext.hpp"
#include "gtest/gtest.h"

class DehackedTest : public TempDirContext
{
};

TEST_F(DehackedTest, LoadNoFile)
{
	ConfigData config;
	ASSERT_FALSE(dehacked::loadFile("bogus.deh", config));
}
