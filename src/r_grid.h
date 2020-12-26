//------------------------------------------------------------------------
//  GRID STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
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

#ifndef __EUREKA_R_GRID_H__
#define __EUREKA_R_GRID_H__


class Grid_State_c
{
friend bool Grid_ParseUser(Instance &inst, const std::vector<SString> &tokens);

public:
	// the actual grid step (64, 128, etc)
	int step;

	// if true, new and moved objects are forced to be on the grid
	bool snap;

	// if non-zero, new lines will be forced to have a certain ratio
	int ratio;

	// whether the grid is being displayed or not.
	bool shown;

	// map coordinates for centre of canvas
	double orig_x;
	double orig_y;

	// scale for drawing map
	// (multiply a map coordinate by this to get a screen coord)
	double Scale;

public:
	explicit Grid_State_c(Instance &inst);
	virtual ~Grid_State_c();

public:
	void Init();

	inline bool isShown() const
	{
		return shown;
	}

	void SetShown(bool enable);
	void SetSnap (bool enable);

	void ToggleShown();
	void ToggleSnap();

	// change the view so that the map coordinates (x, y)
	// appear at the centre of the window
	void MoveTo(double new_x, double new_y);

	void Scroll(double delta_x, double delta_y);

	// move the origin so that the focus point of the last zoom
	// operation (scale change) is map_x/y.
	void RefocusZoom(double map_x, double map_y, float before_Scale);

	// choose the scale nearest to (and less than) the wanted one
	void NearestScale(double want_scale);

	// force grid stepping size to arbitrary value
	void ForceStep(int new_step);

	// compute new grid step from current scale
	void StepFromScale();

	// increase or decrease the grid size.  The 'delta' parameter
	// is positive to increase it, negative to decrease it.
	void AdjustStep (int delta);
	void AdjustScale(int delta);

	// return X/Y coordinate snapped to grid
	// (or unchanged is the 'snap' flag is off)
	double SnapX(double map_x) const;
	double SnapY(double map_y) const;

	// return X/Y coordinate snapped to grid (always)
	int ForceSnapX(double map_x) const;
	int ForceSnapY(double map_y) const;

	// snap X/Y coordinate to ratio lock
	// (unchanged is the ratio snapping is off)
	void RatioSnapXY(double& var_x, double& var_y, double start_x, double start_y) const;

	// quantization snap, can pick coordinate on other side
	int QuantSnapX(double map_x, bool want_furthest, int *dir = NULL) const;
	int QuantSnapY(double map_y, bool want_furthest, int *dir = NULL) const;

	// snap to the natural resolution of canvas
	void NaturalSnapXY(double& var_x, double& var_y) const;

	// check if the X/Y coordinate is on a grid point
	bool OnGridX(double map_x) const;
	bool OnGridY(double map_y) const;

	bool OnGrid(double map_x, double map_y) const;

private:
	void DoSetScale(double new_scale);

	void RawSetStep(int i);
	void RawSetScale(int i);
	void RawSetShown(bool new_shown);

private:
	static const double scale_values[];
	static const int digit_scales[];
	static const int grid_values[];

	Instance &inst;
};

extern Grid_State_c grid;


bool Grid_ParseUser(Instance &inst, const std::vector<SString> &tokens);
void Grid_WriteUser(std::ostream &os);

#endif  /* __EUREKA_R_GRID_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
