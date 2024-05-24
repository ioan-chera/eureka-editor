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

#include "r_grid.h"
#include "m_config.h"
#include "gtest/gtest.h"

class GridStateFixture : public ::testing::Test, public GridListener
{
public:
	virtual void gridRedrawMap()
	{
	}
	virtual void gridSetGrid(int grid)
	{
	}
	virtual void gridUpdateSnap()
	{
	}
	virtual void gridAdjustPos()
	{
	}
	virtual void gridPointerPos()
	{
	}
	virtual void gridSetScale(double scale)
	{
	}
	virtual void gridBeep(const char* message)
	{
	}
	virtual void gridUpdateRatio()
	{
	}
protected:
	void SetUp() override
	{
		initialGridDefaultSize = config::grid_default_size;
	}

	void TearDown() override
	{
		config::grid_default_size = initialGridDefaultSize;
	}

private:
	int initialGridDefaultSize;
};

TEST_F(GridStateFixture, Init)
{
	Grid_State_c grid(*this);

	config::grid_default_size = 16;
	grid.Init();
	ASSERT_EQ(grid.getStep(), 16);
}
