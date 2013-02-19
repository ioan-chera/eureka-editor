//------------------------------------------------------------------------
//  GRID STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include "r_grid.h"
#include "ui_window.h"


Grid_State_c  grid;

// config items
int  default_grid_size = 64;
bool default_grid_snap = true;
int  default_grid_mode = 1;  // normal


Grid_State_c::Grid_State_c() :
	 step(16 /* dummy */),
	 snap(true), shown(true), mode(0),
     orig_x(0.0), orig_y(0.0), Scale(1.0)
{
}

Grid_State_c::~Grid_State_c()
{
}


void Grid_State_c::Init()
{
	step = default_grid_size - 1;

	if (step < 1)
		step = 1;

	if (step >= grid_values[1])
		step =  grid_values[1] - 1;

	shown = true;  // prevent a beep in AdjustStep

	AdjustStep(+1);

	if (default_grid_mode == 0)
	{
		shown = false;

		if (main_win)
			main_win->info_bar->SetGrid(0);
	}
	else
	{
		shown = true;
		mode  = default_grid_mode - 1;
	}

	snap = default_grid_snap;

	if (main_win)
		main_win->info_bar->UpdateSnap();
}


void Grid_State_c::CenterMapAt(int x, int y)
{
	if ((int)orig_x != x || (int)orig_y != y)
	{
		orig_x = x;
		orig_y = y;

		if (main_win)
			main_win->canvas->redraw();
	}
}


int Grid_State_c::ForceSnapX(int map_x) const
{
	if (map_x >= 0)
		return grid.step * ((map_x + grid.step / 2) / grid.step);
	else
		return grid.step * ((map_x - grid.step / 2) / grid.step);
}

int Grid_State_c::ForceSnapY(int map_y) const
{
	if (map_y >= 0)
		return grid.step * ((map_y + grid.step / 2) / grid.step);
	else
		return grid.step * ((map_y - grid.step / 2) / grid.step);
}


int Grid_State_c::SnapX(int map_x) const
{
	if (! snap || grid.step == 0)
		return map_x;

	return ForceSnapX(map_x);
}

int Grid_State_c::SnapY(int map_y) const
{
	if (! snap || grid.step == 0)
		return map_y;

	return ForceSnapY(map_y);
}


int Grid_State_c::QuantSnapX(int map_x, int furthest, int *dir) const
{
	if (OnGridX(map_x))
	{
		if (dir)
			*dir = 0;
		return map_x;
	}

	int new_x = ForceSnapX(map_x);

	if (dir)
	{
		if (new_x < map_x)
			*dir = -1;
		else
			*dir = +1;
	}

	if (! furthest)
		return new_x;

	if (new_x < map_x)
		return ForceSnapX(map_x + (step - 1));
	else
		return ForceSnapX(map_x - (step - 1));
}

int Grid_State_c::QuantSnapY(int map_y, int furthest, int *dir) const
{
	// FIXME
	return QuantSnapX(map_y, furthest, dir);
}


bool Grid_State_c::OnGridX(int map_x) const
{
	if (map_x < 0)
		map_x = -map_x;
	
	return (map_x % step) == 0;
}

bool Grid_State_c::OnGridY(int map_y) const
{
	if (map_y < 0)
		map_y = -map_y;
	
	return (map_y % step) == 0;
}

bool Grid_State_c::OnGrid(int map_x, int map_y) const
{
	return OnGridX(map_x) && OnGridY(map_y);
}


void Grid_State_c::RefocusZoom(int map_x, int map_y, float before_Scale)
{
	float dist_factor = (1.0 - before_Scale / Scale);

	orig_x += (map_x - orig_x) * dist_factor;
	orig_y += (map_y - orig_y) * dist_factor;

	if (main_win)
		main_win->canvas->redraw();
}


const double Grid_State_c::scale_values[] =
{
	16.0, 8.0, 6.0, 4.0,  3.0, 2.0, 1.5, 1.0,

	1.0 / 1.5, 1.0 / 2.0, 1.0 / 3.0,  1.0 / 4.0,
	1.0 / 6.0, 1.0 / 8.0, 1.0 / 16.0, 1.0 / 32.0,
	1.0 / 64.0
};


const int Grid_State_c::digit_scales[] =
{
	1, 3, 5, 7, 9, 11, 13, 14, 15  /* index into scale_values[] */
};

const int Grid_State_c::grid_values[] =
{
	-1 /* OFF */,

	1024, 512, 256, 128, 64, 32, 16, 8, 4, 2
};

#define NUM_SCALE_VALUES  17
#define NUM_GRID_VALUES   11


const char *Grid_State_c::scale_options()
{
	return  "x 16|x 8|x 6|x 4|x 3|x 2|x 1.5|"
			" 100% |"
			"/ 1.5|/ 2|/ 3|/ 4|/ 6|/ 8|/ 16|/ 32|/ 64"; 
}

const char *Grid_State_c::grid_options()
{
	return "OFF|1024|512|256|128| 64| 32| 16|  8|  4|  2";
}


void Grid_State_c::ScaleFromWidget(int i)
{
	SYS_ASSERT(0 <= i && i < NUM_SCALE_VALUES);

	Scale = scale_values[i];

///---	if (_auto)
///---		StepFromScale();

	edit.RedrawMap = 1;
}


void Grid_State_c::StepFromWidget(int i)
{
	SYS_ASSERT(0 <= i && i < NUM_GRID_VALUES);

	if (i == 0)  /* OFF */
	{
		shown = false;

		edit.RedrawMap = 1;
	}
	else
	{
		shown = true;

		step = grid_values[i];

		edit.RedrawMap = 1;
	}
}


void Grid_State_c::StepFromScale()
{
	int pixels_min = 16;

	int result = 1;

	for (int i = 1 ; i < NUM_GRID_VALUES ; i++)
	{
		result = i;

		if (grid_values[i] * Scale / 2 < pixels_min)
			break;
	}

	if (step == grid_values[result])
		return; // no change

	step = grid_values[result];

	if (main_win)
	{
		main_win->canvas->redraw();
	}
}


void Grid_State_c::AdjustStep(int delta)
{
	if (! shown)
	{
		Beep("Grid is off (cannot change step)");
		return;
	}

	int result = -1;

	if (delta > 0)
	{
		for (int i = NUM_GRID_VALUES-1 ; i >= 1 ; i--)
		{
			if (grid_values[i] > step)
			{
				result = i;
				break;
			}
		}
	}
	else // (delta < 0)
	{
		for (int i = 1 ; i < NUM_GRID_VALUES ; i++)
		{
			if (grid_values[i] < step)
			{
				result = i;
				break;
			}
		}
	}

	// already at the extreme end?
	if (result < 0)
		return;

	StepFromWidget(result);

	if (main_win)
		main_win->info_bar->SetGrid(result);
}


void Grid_State_c::AdjustScale(int delta)
{
	int result = -1;

	if (delta > 0)
	{
		for (int i = NUM_SCALE_VALUES-1 ; i >= 0 ; i--)
		{
			if (scale_values[i] > Scale*1.01)
			{
				result = i;
				break;
			}
		}
	}
	else // (delta < 0)
	{
		for (int i = 0 ; i < NUM_SCALE_VALUES ; i++)
		{
			if (scale_values[i] < Scale*0.99)
			{
				result = i;
				break;
			}
		}
	}

	// already at the extreme end?
	if (result < 0)
		return;

	ScaleFromWidget(result);

	if (main_win)
		main_win->info_bar->SetScale(result);
}


void Grid_State_c::DoSetScale()
{
	if (! main_win)
		return;

	for (int i = NUM_SCALE_VALUES-1 ; i >= 0 ; i--)
	{
		float ratio = scale_values[i] / Scale;

		if (ratio > 0.99 && ratio < 1.01)
		{
			main_win->info_bar->SetScale(i);
			return;
		}
	}

	int index = 10;  //meh

	Scale = scale_values[index];

	main_win->info_bar->SetScale(index);
}


void Grid_State_c::DoSetGrid()
{
	if (! main_win)
		return;

	if (! shown)
	{
		main_win->info_bar->SetGrid(0);
		return;
	}

	// find the step
	for (int i = 1 ; i < NUM_GRID_VALUES ; i++)
	{
		if (grid_values[i] == step)
		{
			main_win->info_bar->SetGrid(i);
			return;
		}
	}

	// bad step?
	Init();
}


void Grid_State_c::SetShown(bool enable)
{
	shown = enable;

	edit.RedrawMap = 1;

	DoSetGrid();
}

void Grid_State_c::ToggleShown()
{
	if (! shown)
	{
		shown = true;
	}
	else if (mode == 1)
	{
		shown = false;
		mode  = 0;
	}
	else
	{
		mode = 1;
	}

	edit.RedrawMap = 1;

	DoSetGrid();
}


void Grid_State_c::SetMode(int new_mode)
{
	mode = new_mode;

	edit.RedrawMap = 1;

	DoSetGrid();
}

void Grid_State_c::ToggleMode()
{
	if (! shown)
	{
		shown = true;
	}

	SetMode(1 - mode);
}


void Grid_State_c::SetSnap(bool enable)
{
	snap = enable;

	if (main_win)
		main_win->info_bar->UpdateSnap();

	UpdateHighlight();
}

void Grid_State_c::ToggleSnap()
{
	SetSnap(! snap);
}


void Grid_State_c::NearestScale(double want_scale)
{
	int result = 0;

	for (int i = 0 ; i < NUM_SCALE_VALUES ; i++)
	{
		result = i;

		if (scale_values[i] < want_scale)
			break;
	}

	ScaleFromWidget(result);

	if (main_win)
		main_win->info_bar->SetScale(result);
}


void Grid_State_c::ScaleFromDigit(int digit)
{
	// digit must be 1 to 9
	SYS_ASSERT(1 <= digit && digit <= 9);

	int result = digit_scales[digit - 1];

	ScaleFromWidget(result);

	if (main_win)
		main_win->info_bar->SetScale(result);
}


void Grid_State_c::StepFromDigit(int digit)
{
	// digit must be 1 to 9
	SYS_ASSERT(1 <= digit && digit <= 9);

	int result = NUM_GRID_VALUES - digit;

	StepFromWidget(result);

	if (main_win)
		main_win->info_bar->SetGrid(result);
}


bool Grid_ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "map_pos") == 0 && num_tok >= 4)
	{
		grid.orig_x = atof(tokens[1]);
		grid.orig_y = atof(tokens[2]);

		grid.Scale = atof(tokens[3]);

		grid.DoSetScale();

		edit.RedrawMap = 1;

		return true;
	}

	if (strcmp(tokens[0], "grid") == 0 && num_tok >= 4)
	{
		grid.shown = atoi(tokens[1]) ? true : false;
		grid.mode  = atoi(tokens[2]);
		grid.step  = atoi(tokens[3]);

		if (grid.mode < 0 || grid.mode > 1)
			grid.mode = 1;

		grid.DoSetGrid();

		edit.RedrawMap = 1;

		return true;
	}

	if (strcmp(tokens[0], "snap") == 0 && num_tok >= 2)
	{
		grid.snap = atoi(tokens[1]) ? true : false;

		if (main_win)
			main_win->info_bar->UpdateSnap();

		return true;
	}

	return false;
}


void Grid_WriteUser(FILE *fp)
{
	fprintf(fp, "map_pos %1.0f %1.0f %1.6f\n",
	        grid.orig_x,
			grid.orig_y,
			grid.Scale);

	fprintf(fp, "grid %d %d %d\n",
			grid.shown ? 1 : 0,
			grid.mode,
			grid.step);

	fprintf(fp, "snap %d\n",
	        grid.snap ? 1 : 0);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
