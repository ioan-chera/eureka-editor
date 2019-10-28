//------------------------------------------------------------------------
//  GRID STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
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
#include "e_main.h"
#include "ui_window.h"


Grid_State_c  grid;

// config items
int  grid_default_size = 64;
bool grid_default_snap = false;
int  grid_default_mode = 0;  // off

int  grid_style;  // 0 = squares, 1 = dotty
bool grid_hide_in_free_mode = false;


Grid_State_c::Grid_State_c() :
	 step(16 /* dummy */),
	 snap(true), shown(true),
     orig_x(0.0), orig_y(0.0), Scale(1.0)
{ }

Grid_State_c::~Grid_State_c()
{ }


void Grid_State_c::Init()
{
	step = grid_default_size;

	if (step < 1)
		step = 1;

	if (step > grid_values[0])
		step = grid_values[0];

	shown = true;  // prevent a beep in AdjustStep

	AdjustStep(+1);

	if (grid_default_mode == 0)
	{
		shown = false;

		if (main_win)
			main_win->info_bar->SetGrid(-1);
	}
	else
	{
		shown = true;
	}

	snap = grid_default_snap;

	if (main_win)
		main_win->info_bar->UpdateSnap();
}


void Grid_State_c::MoveTo(double x, double y)
{
	// no change?
	if (fabs(x - orig_x) < 0.01 &&
	    fabs(y - orig_y) < 0.01)
		return;

	orig_x = x;
	orig_y = y;

	if (main_win)
	{
		main_win->scroll->AdjustPos();
		main_win->canvas->PointerPos();

		RedrawMap();
	}
}


void Grid_State_c::Scroll(double delta_x, double delta_y)
{
	MoveTo(orig_x + delta_x, orig_y + delta_y);
}


int Grid_State_c::ForceSnapX(double map_x) const
{
	return grid.step * round(map_x / (double)grid.step);
}

int Grid_State_c::ForceSnapY(double map_y) const
{
	return grid.step * round(map_y / (double)grid.step);
}


double Grid_State_c::SnapX(double map_x) const
{
	if (! snap || grid.step == 0)
		return map_x;

	return ForceSnapX(map_x);
}

double Grid_State_c::SnapY(double map_y) const
{
	if (! snap || grid.step == 0)
		return map_y;

	return ForceSnapY(map_y);
}


void Grid_State_c::RatioSnapXY(double& var_x, double& var_y,
							   double start_x, double start_y) const
{
	double dx = fabs(var_x - start_x);
	double dy = fabs(var_y - start_y);

	switch (0)
	{
	case 0: // no change
		break;

	case 1: // axis aligned
		if (dx < dy)
			var_x = start_x;
		else
			var_y = start_y;
		break;

	case 2:
		// todo
		return;
	}

	var_x = grid.SnapX(var_x);
	var_y = grid.SnapY(var_y);
}


int Grid_State_c::QuantSnapX(double map_x, bool want_furthest, int *dir) const
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

	if (! want_furthest)
		return new_x;

	if (new_x < map_x)
		return ForceSnapX(map_x + (step - 1));
	else
		return ForceSnapX(map_x - (step - 1));
}

int Grid_State_c::QuantSnapY(double map_y, bool want_furthest, int *dir) const
{
	// this is sufficient since the grid is always square

	return QuantSnapX(map_y, want_furthest, dir);
}


bool Grid_State_c::OnGridX(double map_x) const
{
	if (map_x < 0)
		map_x = -map_x;

	int map_x2 = (int)map_x;

	if (map_x != (double)map_x2)
		return false;

	return (map_x2 % step) == 0;
}

bool Grid_State_c::OnGridY(double map_y) const
{
	if (map_y < 0)
		map_y = -map_y;

	int map_y2 = (int)map_y;

	if (map_y != (double)map_y2)
		return false;

	return (map_y2 % step) == 0;
}

bool Grid_State_c::OnGrid(double map_x, double map_y) const
{
	return OnGridX(map_x) && OnGridY(map_y);
}


void Grid_State_c::RefocusZoom(double map_x, double map_y, float before_Scale)
{
	double dist_factor = (1.0 - before_Scale / Scale);

	orig_x += (map_x - orig_x) * dist_factor;
	orig_y += (map_y - orig_y) * dist_factor;

	if (main_win)
	{
		main_win->canvas->PointerPos();
		RedrawMap();
	}
}


const double Grid_State_c::scale_values[] =
{
	32.0, 16.0, 8.0, 6.0, 4.0,  3.0, 2.0, 1.5, 1.0,

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
	1024, 512, 256, 192, 128, 64, 32, 16, 8, 4, 2,

	-1 /* OFF */,
};

#define NUM_SCALE_VALUES  18
#define NUM_GRID_VALUES   12


void Grid_State_c::RawSetScale(int i)
{
	SYS_ASSERT(0 <= i && i < NUM_SCALE_VALUES);

	Scale = scale_values[i];

	if (! main_win)
		return;

	main_win->scroll->AdjustPos();
	main_win->canvas->PointerPos();
	main_win->info_bar->SetScale(Scale);

	RedrawMap();
}


void Grid_State_c::RawSetStep(int i)
{
	SYS_ASSERT(0 <= i && i < NUM_GRID_VALUES);

	if (i == NUM_GRID_VALUES-1)  /* OFF */
	{
		shown = false;

		if (main_win)
			main_win->info_bar->SetGrid(-1);
	}
	else
	{
		shown = true;
		step  = grid_values[i];

		if (main_win)
			main_win->info_bar->SetGrid(step);
	}

	if (grid_hide_in_free_mode)
		SetSnap(shown);

	RedrawMap();
}


void Grid_State_c::ForceStep(int new_step)
{
	step  = new_step;
	shown = true;

	if (main_win)
		main_win->info_bar->SetGrid(step);

	RedrawMap();
}


void Grid_State_c::StepFromScale()
{
	int pixels_min = 16;

	int result = 0;

	for (int i = 0 ; i < NUM_GRID_VALUES-1 ; i++)
	{
		result = i;

		if (grid_values[i] * Scale / 2 < pixels_min)
			break;
	}

	if (step == grid_values[result])
		return; // no change

	step = grid_values[result];

	RedrawMap();
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
		for (int i = NUM_GRID_VALUES-2 ; i >= 0 ; i--)
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
		for (int i = 0 ; i < NUM_GRID_VALUES-1 ; i++)
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

	RawSetStep(result);
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

	RawSetScale(result);
}


void Grid_State_c::RawSetShown(bool new_value)
{
	shown = new_value;

	if (! main_win)
		return;

	if (! shown)
	{
		main_win->info_bar->SetGrid(-1);
		RedrawMap();
		return;
	}

	// update the info-bar
	for (int i = 0 ; i < NUM_GRID_VALUES-1 ; i++)
	{
		if (grid_values[i] == step)
		{
			main_win->info_bar->SetGrid(i);
			break;
		}
	}

	RedrawMap();
}


void Grid_State_c::SetShown(bool enable)
{
	RawSetShown(enable);

	if (grid_hide_in_free_mode)
		SetSnap(enable);
}

void Grid_State_c::ToggleShown()
{
	SetShown(!shown);
}


void Grid_State_c::SetSnap(bool enable)
{
	if (snap == enable)
		return;

	snap = enable;

	if (grid_hide_in_free_mode && snap != shown)
		SetShown(snap);

	if (main_win)
		main_win->info_bar->UpdateSnap();

	RedrawMap();
}

void Grid_State_c::ToggleSnap()
{
	SetSnap(! snap);
}


void Grid_State_c::NearestScale(double want_scale)
{
	int best = 0;

	for (int i = 0 ; i < NUM_SCALE_VALUES ; i++)
	{
		best = i;

		if (scale_values[i] < want_scale * 1.1)
			break;
	}

	RawSetScale(best);
}


bool Grid_ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "map_pos") == 0 && num_tok >= 4)
	{
		double x = atof(tokens[1]);
		double y = atof(tokens[2]);

		grid.MoveTo(x, y);

		double new_scale = atof(tokens[3]);

		grid.NearestScale(new_scale);

		RedrawMap();
		return true;
	}

	if (strcmp(tokens[0], "grid") == 0 && num_tok >= 4)
	{
		bool t_shown = atoi(tokens[1]) ? true : false;

		grid.step = atoi(tokens[3]);

		// tokens[2] was grid.mode, currently unused

		grid.RawSetShown(t_shown);

		RedrawMap();

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
			grid_style ? 0 : 1,  /* was grid.mode, now unused */
			grid.step);

	fprintf(fp, "snap %d\n",
	        grid.snap ? 1 : 0);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
