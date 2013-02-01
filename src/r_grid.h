//------------------------------------------------------------------------
//  GRID STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2012 Andrew Apted
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
public:
	// the actual grid step (64, 128, etc)
	int step;

	// if true, new and moved objects are forced to be on the grid
	bool snap;

	// whether the grid is being displayed or not.
	bool shown;

	// the mode / style of grid : 0 is normal, 1 is simple.
	int mode;

	// map coordinates for centre of canvas
	double orig_x;
	double orig_y;

	// scale for drawing map
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

	// change the view so that the map coordinates (x, y)
	// appear at the centre of the window
	void CenterMapAt(int x, int y);

	// return X/Y coordinate snapped to grid
	// (or unchanged is the 'snap' flag is off)
	int SnapX(int map_x) const;
	int SnapY(int map_y) const;

	// return X/Y coordinate snapped to grid (always)
	int ForceSnapX(int map_x) const;
	int ForceSnapY(int map_y) const;

	// quantization snap : FIXME
	int QuantSnapX(int map_x, int furthest, int *dir = NULL) const;
	int QuantSnapY(int map_y, int furthest, int *dir = NULL) const;

	// check if the X/Y coordinate is on a grid point
	bool OnGridX(int map_x) const;
	bool OnGridY(int map_y) const;

	bool OnGrid(int map_x, int map_y) const;

	// interface with UI_Infobar widget...
	static const char *scale_options();
	static const char *grid_options();

	void ScaleFromWidget(int i);
	void  StepFromWidget(int i);

	// compute new grid step from current scale
	void StepFromScale();

	// increase or decrease the grid size.  The 'delta' parameter
	// is positive to increase it, negative to decrease it.
	void AdjustStep(int delta);
	void AdjustScale(int delta);

	void SetShown(bool enable);
	void SetSnap (bool enable);
	void SetMode (int new_mode);

	void ToggleShown();
	void ToggleMode();
	void ToggleSnap();

	// choose the scale nearest to (and less than) the wanted one
	void NearestScale(double want_scale);

	void ScaleFromDigit(int digit);
	void  StepFromDigit(int digit);

	// move the origin so that the focus point of the last zoom
	// operation (scale change) is map_x/y.
	void RefocusZoom(int map_x, int map_y, float before_Scale);

	// these are really private, but needed by Grid_ParseUser()
	void DoSetGrid();
	void DoSetScale();

private:
	static const double scale_values[];
	static const int digit_scales[];
	static const int grid_values[];
};

extern Grid_State_c grid;


bool Grid_ParseUser(const char ** tokens, int num_tok);
void Grid_WriteUser(FILE *fp);


#endif  /* __EUREKA_R_GRID_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
