//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2021 Ioan Chera
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

#ifndef WadData_h
#define WadData_h

#include "im_color.h"
#include "sys_type.h"

struct MasterDirectory;

//
// Wad data, loaded during resource setup
//
struct WadData
{
	void loadPalette(const MasterDirectory &master) noexcept(false);
	byte findPaletteColor(int r, int g, int b) const noexcept;
	void updateGamma() noexcept;
	void resetDummyTextures() noexcept;

	void loadColormap(const MasterDirectory &master) noexcept(false);

	// this palette has the gamma setting applied
	rgb_color_t palette[256] = {};
	rgb_color_t palette_medium[256] = {};
	byte rgb555_gamma[32];
	byte rgb555_medium[32];

	byte bright_map[256] = {};

	byte raw_palette[256][3] = {};
	// the palette color closest to what TRANS_PIXEL really is
	int trans_replace = 0;

	int missing_tex_color = 0;
	int unknown_tex_color = 0;
	int special_tex_color = 0;
	int unknown_flat_color = 0;
	int unknown_sprite_color = 0;

	byte raw_colormap[32][256] = {};
};

#endif /* WadData_h */
