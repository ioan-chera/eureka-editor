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

#include "Errors.h"

#include "r_grid.h"
#include "m_config.h"
#include "sys_debug.h"

// config items
int  config::grid_default_size = 64;
bool config::grid_default_snap = false;
bool config::grid_default_mode = false;

int  config::grid_style;  // 0 = squares, 1 = dotty
bool config::grid_hide_in_free_mode = false;
bool config::grid_snap_indicator = true;

int  config::grid_ratio_high = 3;  // custom ratio (high must be >= low)
int  config::grid_ratio_low  = 1;  // (low must be > 0)

void grid::State::Init()
{
	step = config::grid_default_size;

	if (step < 1)
		step = 1;

	if (step > values[0])
		step = values[0];

	shown = true;  // prevent a beep in AdjustStep

	AdjustStep(0);	// correct step to power of two

	if (!config::grid_default_mode)
	{
		shown = false;

		listener.gridSetGrid(-1);
	}
	else
	{
		shown = true;
	}

	snap = config::grid_default_snap;
	listener.gridUpdateSnap();
}


void grid::State::MoveTo(const v2double_t &pos)
{
	// no change?
	if (fabs(pos.x - orig.x) < 0.01 &&
	    fabs(pos.y - orig.y) < 0.01)
		return;

	orig.x = pos.x;
	orig.y = pos.y;

	listener.gridAdjustPos();
	listener.gridPointerPos();
	listener.gridRedrawMap();
}


void grid::State::Scroll(const v2double_t &delta)
{
	MoveTo(orig + delta);
}


int grid::State::ForceSnapX(double map_x) const
{
	return static_cast<int>(step * round(map_x / (double)step));
}

int grid::State::ForceSnapY(double map_y) const
{
	return static_cast<int>(step * round(map_y / (double)step));
}


double grid::State::SnapX(double map_x) const
{
	if (! snap || step == 0)
		return map_x;

	return ForceSnapX(map_x);
}

double grid::State::SnapY(double map_y) const
{
	if (! snap || step == 0)
		return map_y;

	return ForceSnapY(map_y);
}


void grid::State::RatioSnapXY(v2double_t& var, const v2double_t &start) const
{
	// snap first, otherwise we lose the ratio
	var = Snap(var);

	double dx = var.x - start.x;
	double dy = var.y - start.y;

	double len = std::max(fabs(dx), fabs(dy));

	int sign_x = (dx >= 0) ? +1 : -1;
	int sign_y = (dy >= 0) ? +1 : -1;

	double custom;

	switch (ratio)
	{
	case 0: // unlocked
		break;

	case 1: // 1:1 (45 degrees) + axis aligned
		if (fabs(dx) * 2 < fabs(dy))
		{
			var.x = start.x;
		}
		else if (fabs(dy) * 2 < fabs(dx))
		{
			var.y = start.y;
		}
		else
		{
			var.x = start.x + sign_x * len;
			var.y = start.y + sign_y * len;
		}
		break;

	case 2: // 2:1 + axis aligned
		if (fabs(dx) * 4 < fabs(dy))
		{
			var.x = start.x;
		}
		else if (fabs(dy) * 4 < fabs(dx))
		{
			var.y = start.y;
		}
		else if (fabs(dx) < fabs(dy))
		{
			var.x = start.x + sign_x * len * 0.5;
			var.y = start.y + sign_y * len;
		}
		else
		{
			var.x = start.x + sign_x * len;
			var.y = start.y + sign_y * len * 0.5;
		}
		break;

	case 3: // 4:1 + axis aligned
		if (fabs(dx) * 8 < fabs(dy))
		{
			var.x = start.x;
		}
		else if (fabs(dy) * 8 < fabs(dx))
		{
			var.y = start.y;
		}
		else if (fabs(dx) < fabs(dy))
		{
			var.x = start.x + sign_x * len * 0.25;
			var.y = start.y + sign_y * len;
		}
		else
		{
			var.x = start.x + sign_x * len;
			var.y = start.y + sign_y * len * 0.25;
		}
		break;

	case 4: // 8:1 + axis aligned
		if (fabs(dx) * 16 < fabs(dy))
		{
			var.x = start.x;
		}
		else if (fabs(dy) * 16 < fabs(dx))
		{
			var.y = start.y;
		}
		else if (fabs(dx) < fabs(dy))
		{
			var.x = start.x + sign_x * len * 0.125;
			var.y = start.y + sign_y * len;
		}
		else
		{
			var.x = start.x + sign_x * len;
			var.y = start.y + sign_y * len * 0.125;
		}
		break;

	case 5: // 5:4 + axis aligned
		if (fabs(dx) * 3 < fabs(dy))
		{
			var.x = start.x;
		}
		else if (fabs(dy) * 3 < fabs(dx))
		{
			var.y = start.y;
		}
		else if (fabs(dx) < fabs(dy))
		{
			var.x = start.x + sign_x * len * 0.8;
			var.y = start.y + sign_y * len;
		}
		else
		{
			var.x = start.x + sign_x * len;
			var.y = start.y + sign_y * len * 0.8;
		}
		break;

	case 6: // 7:4 + axis aligned
		if (fabs(dx) * 3 < fabs(dy))
		{
			var.x = start.x;
		}
		else if (fabs(dy) * 3 < fabs(dx))
		{
			var.y = start.y;
		}
		else if (fabs(dx) < fabs(dy))
		{
			var.x = start.x + sign_x * len * 4 / 7;
			var.y = start.y + sign_y * len;
		}
		else
		{
			var.x = start.x + sign_x * len;
			var.y = start.y + sign_y * len * 4 / 7;
		}
		break;

	default: // USER SETTING
		if (config::grid_ratio_low < 1)
			config::grid_ratio_low = 1;
		if (config::grid_ratio_high < config::grid_ratio_low)
			config::grid_ratio_high = config::grid_ratio_low;

		custom = (double)config::grid_ratio_low / (double)config::grid_ratio_high;

		if (custom > 0.1 && fabs(dx) < fabs(dy) * custom * 0.3)
		{
			var.x = start.x;
		}
		else if (custom > 0.1 && fabs(dy) < fabs(dx) * custom * 0.3)
		{
			var.y = start.y;
		}
		else if (fabs(dx) < fabs(dy))
		{
			var.x = start.x + sign_x * len * custom;
			var.y = start.y + sign_y * len;
		}
		else
		{
			var.x = start.x + sign_x * len;
			var.y = start.y + sign_y * len * custom;
		}
	}
}


int grid::State::QuantSnapX(double map_x, bool want_furthest, int *dir) const
{
	if (OnGridX(map_x))
	{
		if (dir)
			*dir = 0;
		return static_cast<int>(map_x);
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

int grid::State::QuantSnapY(double map_y, bool want_furthest, int *dir) const
{
	// this is sufficient since the grid is always square

	return QuantSnapX(map_y, want_furthest, dir);
}


void grid::State::NaturalSnapXY(double& var_x, double& var_y) const
{
	// this is only used by UI_Canvas::PointerPos()

	double nat_step = 1.0;

	while (nat_step * 2.0 <= Scale)
		nat_step = nat_step * 2.0;

	while (nat_step * 0.5 >= Scale)
		nat_step = nat_step * 0.5;

	var_x = round(var_x * nat_step) / nat_step;
	var_y = round(var_y * nat_step) / nat_step;
}


bool grid::State::OnGridX(double map_x) const
{
	if (map_x < 0)
		map_x = -map_x;

	int map_x2 = (int)map_x;

	if (map_x != (double)map_x2)
		return false;

	return (map_x2 % step) == 0;
}

bool grid::State::OnGridY(double map_y) const
{
	if (map_y < 0)
		map_y = -map_y;

	int map_y2 = (int)map_y;

	if (map_y != (double)map_y2)
		return false;

	return (map_y2 % step) == 0;
}

bool grid::State::OnGrid(double map_x, double map_y) const
{
	return OnGridX(map_x) && OnGridY(map_y);
}

void grid::State::configureGrid(int step, bool shown)
{
	this->step = step;
	RawSetShown(shown);
	listener.gridRedrawMap();
}

void grid::State::configureSnap(bool snap)
{
	this->snap = snap;
	listener.gridUpdateSnap();
}

void grid::State::configureRatio(int ratio, bool redraw)
{
	this->ratio = ratio;
	listener.gridUpdateRatio();
	if(redraw)
		listener.gridRedrawMap();
}

void grid::State::RefocusZoom(const v2double_t &map, float before_Scale)
{
	double dist_factor = (1.0 - before_Scale / Scale);

	orig.x += (map.x - orig.x) * dist_factor;
	orig.y += (map.y - orig.y) * dist_factor;

	listener.gridPointerPos();
	listener.gridRedrawMap();
}


const double grid::State::scale_values[] =
{
	32.0, 16.0, 8.0, 6.0, 4.0,  3.0, 2.0, 1.5, 1.0,

	1.0 / 1.5, 1.0 / 2.0, 1.0 / 3.0,  1.0 / 4.0,
	1.0 / 6.0, 1.0 / 8.0, 1.0 / 16.0, 1.0 / 32.0,
	1.0 / 64.0
};


const int grid::State::digit_scales[] =
{
	1, 3, 5, 7, 9, 11, 13, 14, 15  /* index into scale_values[] */
};


#define NUM_SCALE_VALUES  18
#define NUM_GRID_VALUES   12


void grid::State::RawSetScale(int i)
{
	SYS_ASSERT(0 <= i && i < NUM_SCALE_VALUES);

	Scale = scale_values[i];

	listener.gridAdjustPos();
	listener.gridPointerPos();
	listener.gridSetScale(Scale);

	listener.gridRedrawMap();
}


void grid::State::RawSetStep(int i)
{
	SYS_ASSERT(0 <= i && i < NUM_GRID_VALUES);

	if (i == NUM_GRID_VALUES-1)  /* OFF */
	{
		shown = false;
		listener.gridSetGrid(-1);
	}
	else
	{
		shown = true;
		step  = values[i];
		listener.gridSetGrid(step);
	}

	if (config::grid_hide_in_free_mode)
		SetSnap(shown);

	listener.gridRedrawMap();
}


void grid::State::ForceStep(int new_step)
{
	step  = new_step;
	shown = true;

	listener.gridSetGrid(step);

	if (config::grid_hide_in_free_mode)
		SetSnap(shown);

	listener.gridRedrawMap();
}


void grid::State::StepFromScale()
{
	int pixels_min = 16;

	int result = 0;

	for (int i = 0 ; i < NUM_GRID_VALUES-1 ; i++)
	{
		result = i;

		if (values[i] * Scale / 2 < pixels_min)
			break;
	}

	if (step == values[result])
		return; // no change

	step = values[result];

	listener.gridRedrawMap();
}


void grid::State::AdjustStep(int delta)
{
	if (! shown)
	{
		listener.gridBeep("Grid is off (cannot change step)");
		return;
	}

	int result = -1;

	if (delta > 0)
	{
		for (int i = NUM_GRID_VALUES-2 ; i >= 0 ; i--)
		{
			if (values[i] > step)
			{
				result = i;
				break;
			}
		}
	}
	else if(!delta)	// this is for snapping to the closest grid
	{
		for (int i = NUM_GRID_VALUES-2 ; i >= 0 ; i--)
		{
			if (values[i] >= step)
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
			if (values[i] < step)
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


void grid::State::AdjustScale(int delta)
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


void grid::State::RawSetShown(bool new_value)
{
	shown = new_value;
	listener.gridSetGrid(shown ? step : -1);
	listener.gridRedrawMap();
}

std::string grid::getValuesFLTKMenuString()
{
	std::string result;
	result.reserve(5 * lengthof(values));
	for(size_t i = 0; i < lengthof(values); ++i)
	{
		int value = values[i];
		if(value >= 0)
			result += SString::printf("%d", value).get();
		else
			result += "OFF";
		if(i < lengthof(values) - 1)
            result += '|';
	}
	return result;
}

void grid::State::SetShown(bool enable)
{
	RawSetShown(enable);

	if (config::grid_hide_in_free_mode)
		SetSnap(enable);
}

void grid::State::ToggleShown()
{
	SetShown(!shown);
}


void grid::State::SetSnap(bool enable)
{
	if (snap == enable)
		return;

	snap = enable;

	if (config::grid_hide_in_free_mode && snap != shown)
		SetShown(snap);

	listener.gridUpdateSnap();
	listener.gridRedrawMap();
}

void grid::State::ToggleSnap()
{
	SetSnap(! snap);
}


void grid::State::NearestScale(double want_scale)
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


bool grid::State::parseUser(const std::vector<SString> &tokens)
{
	if (tokens[0] == "map_pos" && tokens.size() >= 4)
	{
		double x = atof(tokens[1]);
		double y = atof(tokens[2]);

		MoveTo({ x, y });

		double new_scale = atof(tokens[3]);

		NearestScale(new_scale);

		listener.gridRedrawMap();
		return true;
	}

	if (tokens[0] == "grid" && tokens.size() >= 4)
	{
		bool t_shown = atoi(tokens[1]) ? true : false;

		configureGrid(atoi(tokens[3]), t_shown);
		// tokens[2] was grid.mode, currently unused
		return true;
	}

	if (tokens[0] == "snap" && tokens.size() >= 2)
	{
		configureSnap(!!atoi(tokens[1]));

		return true;
	}

	return false;
}

void grid::State::writeUser(std::ostream &os) const
{
	os << "map_pos " << SString::printf("%1.0f %1.0f %1.6f", getOrig().x, getOrig().y, getScale()) <<
		'\n';
	os << "grid " << (isShown() ? 1 : 0) << ' ' << (config::grid_style ? 0 : 1) << ' ' <<
		getStep() << '\n';
	os << "snap " << (snaps() ? 1 : 0) << '\n';
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
