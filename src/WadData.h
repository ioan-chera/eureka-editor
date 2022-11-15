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
#include "im_img.h"
#include "m_strings.h"
#include "sys_type.h"

#include <map>
#include <memory>
#include <vector>

class Img_c;
class Lump_c;
class Palette;
class Wad_file;
struct ConfigData;

// maps type number to an image
typedef std::map<int, Img_c *> sprite_map_t;

//
// Wad image set
//
class ImageSet
{
public:
	Img_c *IM_SpecialTex(const Palette &palette);
	Img_c *IM_MissingTex(const ConfigData &config);
	Img_c *IM_UnknownTex(const ConfigData &config);
	Img_c *IM_UnknownFlat(const ConfigData &config);
	Img_c *IM_UnknownSprite(const ConfigData &config);

	Img_c *IM_DigitFont_11x14();
	Img_c *IM_DigitFont_14x19();

	void IM_UnloadDummyTextures() const;
	void IM_ResetDummyTextures();

	void W_AddTexture(const SString &name, Img_c *img, bool is_medusa);
	Img_c *getTexture(const ConfigData &config, const SString &name, bool try_uppercase = false) const;
	int W_GetTextureHeight(const ConfigData &config, const SString &name) const;
	bool W_TextureCausesMedusa(const SString &name) const;
	bool W_TextureIsKnown(const ConfigData &config, const SString &name) const;
	void W_ClearTextures();
	const std::map<SString, Img_c *> &getTextures() const
	{
		return textures;
	}

	void W_AddFlat(const SString &name, Img_c *img);
	Img_c *W_GetFlat(const ConfigData &config, const SString &name, bool try_uppercase = false) const;
	bool W_FlatIsKnown(const ConfigData &config, const SString &name) const;
	void W_ClearFlats();
	const std::map<SString, Img_c *> &getFlats() const
	{
		return flats;
	}

	void W_ClearSprites();

	void W_UnloadAllTextures() const;

public:	// TODO: make private
	std::map<SString, Img_c *> textures;
	// textures which can cause the Medusa Effect in vanilla/chocolate DOOM
	std::map<SString, int> medusa_textures;
	std::map<SString, Img_c *> flats;
	sprite_map_t sprites;

	int missing_tex_color = 0;
	Img_c *missing_tex_image = nullptr;

	int unknown_tex_color = 0;
	Img_c *unknown_tex_image = nullptr;

	int special_tex_color = 0;
	Img_c *special_tex_image = nullptr;

	int unknown_flat_color = 0;
	Img_c *unknown_flat_image = nullptr;

	int unknown_sprite_color = 0;
	Img_c *unknown_sprite_image = nullptr;

	Img_c *digit_font_11x14 = nullptr;
	Img_c *digit_font_14x19 = nullptr;
};

//
// Wad palette info
//
class Palette
{
public:
	void updateGamma();
	void decodePixel(img_pixel_t p, byte &r, byte &g, byte &b) const;
	void decodePixelMedium(img_pixel_t p, byte &r, byte &g, byte &b) const;
	void createBrightMap();

	rgb_color_t getPaletteColor(int index) const
	{
		return palette[index];
	}

	void loadPalette(Lump_c *lump);
	void loadColormap(Lump_c *lump);
	
	byte findPaletteColor(int r, int g, int b) const;
	rgb_color_t pixelToRGB(img_pixel_t p) const;
	byte getColormapIndex(int cmap, int pos) const
	{
		return raw_colormap[cmap][pos];
	}

	int getTransReplace() const
	{
		return trans_replace;
	}

private:
	// this palette has the gamma setting applied
	rgb_color_t palette[256] = {};
	rgb_color_t palette_medium[256] = {};
	byte rgb555_gamma[32];
	byte rgb555_medium[32];
	byte bright_map[256] = {};
	byte raw_palette[256][3] = {};
	byte raw_colormap[32][256] = {};
	// the palette color closest to what TRANS_PIXEL really is
	int trans_replace = 0;

};

//
// Manages the WAD loading
//
class MasterDir
{
public:
	void RemoveEditWad();
	void replaceEditWad(const std::shared_ptr<Wad_file> &new_wad);

	void MasterDir_Add(const std::shared_ptr<Wad_file> &wad);
	void MasterDir_Remove(const std::shared_ptr<Wad_file> &wad);
	void MasterDir_CloseAll();
	bool MasterDir_HaveFilename(const SString &chk_path) const;

	Lump_c *findGlobalLump(const SString &name) const;
	Lump_c *findSpriteLump(const SString &name) const;
public:	// TODO: make private
	// the current PWAD, or NULL for none.
	// when present it is also at master_dir.back()
	std::shared_ptr<Wad_file> edit_wad;
	std::shared_ptr<Wad_file> game_wad;
	std::vector<std::shared_ptr<Wad_file>> dir;	// the IWAD, never NULL, always at master_dir.front()
	SString Pwad_name;	// Filename of current wad
};

//
// Wad data, loaded during resource setup
//
struct WadData
{
	void W_LoadTextures(const ConfigData &config);

	void W_LoadFlats();

	Img_c *W_GetSprite(const ConfigData &config, int type);
	
	void W_LoadPalette()
	{
		palette.loadPalette(master.findGlobalLump("PLAYPAL"));
		images.IM_ResetDummyTextures();
	}

	void W_LoadColormap()
	{
		palette.loadColormap(master.findGlobalLump("COLORMAP"));
	}

	ImageSet images;
	Palette palette;
	MasterDir master;
};

#endif /* WadData_h */
