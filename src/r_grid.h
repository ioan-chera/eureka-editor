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
friend bool Grid_ParseUser(const std::vector<std::string> &tokens);

public:
	// the actual grid step (64, 128, etc)
	int step;

	// if true, new and moved objects are forced to be on the grid
	bool snap;

	// whether the grid is being displayed or not.
	bool shown;

	// map coordinates for centre of canvas
	double orig_x;
	double orig_y;

	// scale for drawing map
	// (multiply a map coordinate by this to get a screen coord)
	float Scale;

public:
	Grid_State_c();
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
	void RefocusZoom(int map_x, int map_y, float before_Scale);

	// choose the scale nearest to (and less than) the wanted one
	void NearestScale(double want_scale);

	void ScaleFromWidget(int i);

	// set grid stepping size
	void SetStep(int new_step);

	void StepFromWidget(int i);

	// compute new grid step from current scale
	void StepFromScale();

	// increase or decrease the grid size.  The 'delta' parameter
	// is positive to increase it, negative to decrease it.
	void AdjustStep (int delta);
	void AdjustScale(int delta);

	// return X/Y coordinate snapped to grid
	// (or unchanged is the 'snap' flag is off)
	int SnapX(int map_x) const;
	int SnapY(int map_y) const;

	// return X/Y coordinate snapped to grid (always)
	int ForceSnapX(int map_x) const;
	int ForceSnapY(int map_y) const;

	// quantization snap, can pick coordinate on other side
	int QuantSnapX(int map_x, bool want_furthest, int *dir = NULL) const;
	int QuantSnapY(int map_y, bool want_furthest, int *dir = NULL) const;

	// check if the X/Y coordinate is on a grid point
	bool OnGridX(int map_x) const;
	bool OnGridY(int map_y) const;

	bool OnGrid(int map_x, int map_y) const;

	// interface with UI_Infobar widget...
	static const char *scale_options();
	static const char * grid_options();

private:
	void DoSetShown(bool   new_shown);
	void DoSetScale(double new_scale);

private:
	static const double scale_values[];
	static const int digit_scales[];
	static const int grid_values[];
};

extern Grid_State_c grid;


bool Grid_ParseUser(const std::vector<std::string> &tokens);
void Grid_WriteUser(FILE *fp);


#endif  /* __EUREKA_R_GRID_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
