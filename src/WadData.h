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
#include <memory>

class Img_c;
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

	void clearFlats();
	void addFlat(const SString &name, Img_c *img);
	void loadFlats(const MasterDirectory &master);
	void clearTextures();

	void addTexture(const SString &name, std::unique_ptr<Img_c> &&img,
					bool is_medusa);
	void loadTextureEntry_Strife(const byte *tex_data, int tex_length,
								 int offset, const byte *pnames, int pname_size,
								 bool skip_first, const MasterDirectory &master,
								 const ConfigData &config);

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

	std::map<SString, std::unique_ptr<Img_c>> flats;
	std::map<SString, std::unique_ptr<Img_c>> textures;
	// textures which can cause the Medusa Effect in vanilla/chocolate DOOM
	std::map<SString, int> medusa_textures;
};

#endif /* WadData_h */
