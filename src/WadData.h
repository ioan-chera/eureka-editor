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

class Img_c;
struct ConfigData;

//
// Wad data, loaded during resource setup
//
struct WadData
{
	Img_c *IM_MissingTex(const ConfigData &config);
	Img_c *IM_UnknownTex(const ConfigData &config);
	Img_c *IM_SpecialTex();
	Img_c *IM_UnknownFlat(const ConfigData &config);
	Img_c *IM_UnknownSprite(const ConfigData &config);

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

	Img_c *digit_font_11x14 = nullptr;
	Img_c *digit_font_14x19 = nullptr;
	Img_c *missing_tex_image = nullptr;
	Img_c *special_tex_image = nullptr;
	Img_c *unknown_flat_image = nullptr;
	Img_c *unknown_sprite_image = nullptr;
	Img_c *unknown_tex_image = nullptr;
};

#endif /* WadData_h */
