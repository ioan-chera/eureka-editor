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
#include "m_strings.h"
#include "sys_type.h"
#include <map>
#include <memory>
#include <vector>

class Img_c;
class Wad_file;
struct ConfigData;

// maps type number to an image
typedef std::map<int, Img_c *> sprite_map_t;

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
	Img_c *IM_DigitFont_11x14();
	Img_c *IM_DigitFont_14x19();

	void W_AddTexture(const SString &name, Img_c *img, bool is_medusa);
	Img_c *W_GetTexture(const ConfigData &config, const SString &name, bool try_uppercase = false) const;
	int W_GetTextureHeight(const ConfigData &config, const SString &name) const;
	bool W_TextureIsKnown(const ConfigData &config, const SString &name) const;
	bool W_TextureCausesMedusa(const SString &name) const;
	void W_ClearTextures();

	void W_AddFlat(const SString &name, Img_c *img);
	Img_c *W_GetFlat(const ConfigData &config, const SString &name, bool try_uppercase = false) const;
	bool W_FlatIsKnown(const ConfigData &config, const SString &name) const;
	void W_ClearFlats();

	void W_ClearSprites();

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

	std::map<SString, Img_c *> textures;
	std::map<SString, Img_c *> flats;
	sprite_map_t sprites;
	// textures which can cause the Medusa Effect in vanilla/chocolate DOOM
	std::map<SString, int> medusa_textures;

	// the current PWAD, or NULL for none.
	// when present it is also at master_dir.back()
	std::shared_ptr<Wad_file> edit_wad;
	std::shared_ptr<Wad_file> game_wad;
	std::vector<std::shared_ptr<Wad_file>> master_dir;	// the IWAD, never NULL, always at master_dir.front()
	SString Pwad_name;	// Filename of current wad
};

#endif /* WadData_h */
