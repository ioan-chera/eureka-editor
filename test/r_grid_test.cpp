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
	virtual void gridRedrawMap() override
	{
		++redrawMapCounts;
	}
	virtual void gridSetGrid(int grid) override
	{
		gridSettings.push_back(grid);
	}
	virtual void gridUpdateSnap() override
	{
		++snapUpdates;
	}
	virtual void gridAdjustPos() override
	{
		++positionUpdates;
	}
	virtual void gridPointerPos() override
	{
		++pointerPositionUpdates;
	}
	virtual void gridSetScale(double scale) override
	{
		scaleSettings.push_back(scale);
	}
	virtual void gridBeep(const char* message) override
	{
		beeps.push_back(message);
	}
	virtual void gridUpdateRatio() override
	{
		++ratioUpdates;
	}
protected:
	void TearDown() override
	{
		config::grid_default_size = initialGridDefaultSize;
		config::grid_default_mode = initialGridDefaultMode;
		config::grid_default_snap = initialGridDefaultSnap;
		config::grid_hide_in_free_mode = initialGridHideInFreeMode;
	}
	
	int redrawMapCounts = 0;
	std::vector<int> gridSettings;
	int snapUpdates = 0;
	int positionUpdates = 0;
	int pointerPositionUpdates = 0;
	std::vector<double> scaleSettings;
	std::vector<std::string> beeps;
	int ratioUpdates = 0;

private:
	int initialGridDefaultSize = config::grid_default_size;
	bool initialGridDefaultMode = config::grid_default_mode;
	bool initialGridDefaultSnap = config::grid_default_snap;
	bool initialGridHideInFreeMode = config::grid_hide_in_free_mode;
};

TEST_F(GridStateFixture, InitNormalGrid)
{
	Grid_State_c grid(*this);

	config::grid_default_size = 16;
	config::grid_default_mode = true;
	
	grid.Init();
	ASSERT_TRUE(grid.isShown());
	ASSERT_EQ(grid.getStep(), 16);
	ASSERT_GE(gridSettings.size(), 1);
	ASSERT_EQ(gridSettings.back(), 16);
	
	ASSERT_GE(redrawMapCounts, 1);
}

TEST_F(GridStateFixture, InitNonPowerOf2Grid)
{
	Grid_State_c grid(*this);

	config::grid_default_size = 12;
	config::grid_default_mode = true;

	grid.Init();
	ASSERT_TRUE(grid.isShown());
	ASSERT_EQ(grid.getStep(), 16);
	ASSERT_GE(gridSettings.size(), 1);
	ASSERT_EQ(gridSettings.back(), 16);
	
	ASSERT_GE(redrawMapCounts, 1);
}

TEST_F(GridStateFixture, InitUnderflowGridCapsAt2)
{
	{
		Grid_State_c grid(*this);
		
		config::grid_default_size = 1;
		config::grid_default_mode = true;
		
		grid.Init();
		ASSERT_TRUE(grid.isShown());
		ASSERT_EQ(grid.getStep(), 2);
		ASSERT_GE(gridSettings.size(), 1);
		ASSERT_EQ(gridSettings.back(), 2);
		
		ASSERT_GE(redrawMapCounts, 1);
	}
	{
		Grid_State_c grid(*this);
		
		config::grid_default_size = -8;
		config::grid_default_mode = true;
		
		grid.Init();
		ASSERT_TRUE(grid.isShown());
		ASSERT_EQ(grid.getStep(), 2);
		ASSERT_GE(gridSettings.size(), 1);
		ASSERT_EQ(gridSettings.back(), 2);
		
		ASSERT_GE(redrawMapCounts, 2);
	}
}

TEST_F(GridStateFixture, InitOverflowGridCapsAt1024)
{
	{
		Grid_State_c grid(*this);
		
		config::grid_default_size = 1025;
		config::grid_default_mode = true;
		
		grid.Init();
		ASSERT_TRUE(grid.isShown());
		ASSERT_EQ(grid.getStep(), 1024);
		ASSERT_GE(gridSettings.size(), 1);
		ASSERT_EQ(gridSettings.back(), 1024);
		
		ASSERT_GE(redrawMapCounts, 1);
	}
	{
		Grid_State_c grid(*this);
		
		config::grid_default_size = 2048;
		config::grid_default_mode = true;
		
		grid.Init();
		ASSERT_TRUE(grid.isShown());
		ASSERT_EQ(grid.getStep(), 1024);
		ASSERT_GE(gridSettings.size(), 1);
		ASSERT_EQ(gridSettings.back(), 1024);
		
		ASSERT_GE(redrawMapCounts, 2);
	}
}

TEST_F(GridStateFixture, SnapCalled)
{
	Grid_State_c grid(*this);

	grid.Init();
	ASSERT_EQ(snapUpdates, 1);
	
	ASSERT_GE(redrawMapCounts, 1);
}

TEST_F(GridStateFixture, GridDisabledWhenNotConfigured)
{
	Grid_State_c grid(*this);

	config::grid_default_mode = false;
	grid.Init();
	ASSERT_FALSE(grid.isShown());
	ASSERT_GE(gridSettings.size(), 1);
	ASSERT_EQ(gridSettings.back(), -1);
	
	ASSERT_GE(redrawMapCounts, 1);
}

TEST_F(GridStateFixture, InitWithoutSnappingButVisible)
{
	Grid_State_c grid(*this);
	
	config::grid_default_mode = true;
	config::grid_default_snap = false;
	config::grid_default_size = 16;
	grid.Init();
	ASSERT_EQ(snapUpdates, 1);
	ASSERT_FALSE(grid.snaps());
	ASSERT_EQ(grid.getStep(), 16);
	ASSERT_TRUE(grid.isShown());
	ASSERT_GE(gridSettings.size(), 1);
	ASSERT_EQ(gridSettings.back(), 16);
	
	ASSERT_GE(redrawMapCounts, 1);
}

TEST_F(GridStateFixture, InitWithoutSnappingAndInvisible)
{
	Grid_State_c grid(*this);
	
	config::grid_default_mode = false;
	config::grid_default_snap = false;
	config::grid_default_size = 16;
	grid.Init();
	ASSERT_EQ(snapUpdates, 1);
	ASSERT_FALSE(grid.snaps());
	ASSERT_EQ(grid.getStep(), 16);
	ASSERT_FALSE(grid.isShown());
	ASSERT_GE(gridSettings.size(), 1);
	ASSERT_EQ(gridSettings.back(), -1);
	
	ASSERT_GE(redrawMapCounts, 1);
}

TEST_F(GridStateFixture, ChangeShownStatus)
{
	Grid_State_c grid(*this);
	
	config::grid_default_mode = true;
	config::grid_default_size = 16;
	config::grid_default_snap = true;
	grid.Init();
	ASSERT_TRUE(grid.isShown());
	
	grid.SetShown(false);
	ASSERT_FALSE(grid.isShown());
	ASSERT_FALSE(gridSettings.empty());
	ASSERT_EQ(gridSettings.back(), -1);
	
	grid.SetShown(true);
	ASSERT_TRUE(grid.isShown());
	ASSERT_FALSE(gridSettings.empty());
	ASSERT_EQ(gridSettings.back(), 16);
	
	// Now also with snapping
	config::grid_hide_in_free_mode = true;
	int befsnaps = snapUpdates;
	grid.SetShown(false);
	ASSERT_FALSE(grid.isShown());
	ASSERT_FALSE(gridSettings.empty());
	ASSERT_EQ(gridSettings.back(), -1);
	ASSERT_FALSE(grid.snaps());
	ASSERT_EQ(snapUpdates, befsnaps + 1);
	
	befsnaps = snapUpdates;
	grid.SetShown(true);
	ASSERT_TRUE(grid.isShown());
	ASSERT_FALSE(gridSettings.empty());
	ASSERT_EQ(gridSettings.back(), 16);
	ASSERT_TRUE(grid.snaps());
	ASSERT_EQ(snapUpdates, befsnaps + 1);
}

TEST_F(GridStateFixture, SetSnapSameValueChangesNothing)
{
	{
		Grid_State_c grid(*this);
		config::grid_default_snap = false;
		grid.Init();
		int snapUpdatesBefore = snapUpdates;
		int mapRedrawsBefore = redrawMapCounts;
		grid.SetSnap(false);
		ASSERT_EQ(snapUpdates, snapUpdatesBefore);
		ASSERT_EQ(redrawMapCounts, mapRedrawsBefore);
	}
	{
		Grid_State_c grid(*this);
		config::grid_default_snap = true;
		grid.Init();
		int snapUpdatesBefore = snapUpdates;
		int mapRedrawsBefore = redrawMapCounts;
		grid.SetSnap(true);
		ASSERT_EQ(snapUpdates, snapUpdatesBefore);
		ASSERT_EQ(redrawMapCounts, mapRedrawsBefore);
	}
}

TEST_F(GridStateFixture, ToggleSnappingWithoutHidingInFreeMode)
{
	Grid_State_c grid(*this);
	
	config::grid_default_snap = false;
	config::grid_default_mode = false;
	grid.Init();
	ASSERT_FALSE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
	int snapUpdatesBefore = snapUpdates;
	int mapRedrawsBefore = redrawMapCounts;
	grid.SetSnap(true);
	ASSERT_TRUE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
	ASSERT_EQ(snapUpdates, snapUpdatesBefore + 1);
	ASSERT_EQ(redrawMapCounts, mapRedrawsBefore + 1);
	grid.SetSnap(false);
	ASSERT_FALSE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
	ASSERT_EQ(snapUpdates, snapUpdatesBefore + 2);
	ASSERT_EQ(redrawMapCounts, mapRedrawsBefore + 2);
}

TEST_F(GridStateFixture, ToggleSnappingWithHidingInFreeMode)
{
	Grid_State_c grid(*this);

	config::grid_default_snap = false;
	config::grid_default_mode = false;
	config::grid_hide_in_free_mode = true;
	grid.Init();
	ASSERT_FALSE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
	int snapUpdatesBefore = snapUpdates;
	int mapRedrawsBefore = redrawMapCounts;
	grid.SetSnap(true);
	ASSERT_TRUE(grid.snaps());
	ASSERT_TRUE(grid.isShown());
	ASSERT_EQ(snapUpdates, snapUpdatesBefore + 1);
	ASSERT_GE(redrawMapCounts, mapRedrawsBefore + 1);
	ASSERT_FALSE(gridSettings.empty());
	ASSERT_GE(gridSettings.back(), 2);
	mapRedrawsBefore = redrawMapCounts;	// refresh it because the increment is unknown, it's just >= 1
	grid.SetSnap(false);
	ASSERT_FALSE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
	ASSERT_EQ(snapUpdates, snapUpdatesBefore + 2);
	ASSERT_GE(redrawMapCounts, mapRedrawsBefore + 1);
	ASSERT_FALSE(gridSettings.empty());
	ASSERT_GE(gridSettings.back(), -1);
}

TEST_F(GridStateFixture, ToggleControlsWithoutLinking)
{
	Grid_State_c grid(*this);

	config::grid_default_snap = false;
	config::grid_default_mode = false;
	config::grid_hide_in_free_mode = false;
	grid.Init();
	ASSERT_FALSE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
	grid.ToggleShown();
	ASSERT_FALSE(grid.snaps());
	ASSERT_TRUE(grid.isShown());
	grid.ToggleShown();
	ASSERT_FALSE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
	grid.ToggleSnap();
	ASSERT_TRUE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
	grid.ToggleSnap();
	ASSERT_FALSE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
}

TEST_F(GridStateFixture, ToggleControlsWithLinking)
{
	Grid_State_c grid(*this);

	config::grid_default_snap = false;
	config::grid_default_mode = false;
	config::grid_hide_in_free_mode = true;
	grid.Init();
	ASSERT_FALSE(grid.snaps());
	ASSERT_FALSE(grid.isShown());
	grid.ToggleShown();
	ASSERT_TRUE(grid.isShown());
	ASSERT_TRUE(grid.snaps());
	grid.ToggleShown();
	ASSERT_FALSE(grid.isShown());
	ASSERT_FALSE(grid.snaps());
	grid.ToggleSnap();
	ASSERT_TRUE(grid.isShown());
	ASSERT_TRUE(grid.snaps());
	grid.ToggleSnap();
	ASSERT_FALSE(grid.isShown());
	ASSERT_FALSE(grid.snaps());
}
