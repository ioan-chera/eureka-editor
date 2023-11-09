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

#include "Errors.h"
#include "im_color.h"
#include "im_img.h"
#include "m_strings.h"
#include "sys_type.h"

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

class Img_c;
class Lump_c;
class Palette;
class Wad_file;
struct ConfigData;
struct LoadingData;

// maps type number to an image
typedef std::map<int, tl::optional<Img_c>> sprite_map_t;

//
// Wad image set
//
class ImageSet
{
public:
	const Img_c &IM_SpecialTex(const Palette &palette);
	Img_c &getMutableSpecialTexture(const Palette &palette)
	{
		return const_cast<Img_c &>(IM_SpecialTex(palette));
	}
	const Img_c &IM_MissingTex(const ConfigData &config);
	Img_c &getMutableMissingTexture(const ConfigData &config)
	{
		return const_cast<Img_c &>(IM_MissingTex(config));
	}
	const Img_c &IM_UnknownTex(const ConfigData &config);
	Img_c &getMutableUnknownTexture(const ConfigData &config)
	{
		return const_cast<Img_c &>(IM_UnknownTex(config));
	}
	const Img_c &IM_UnknownFlat(const ConfigData &config);
	Img_c &getMutableUnknownFlat(const ConfigData &config)
	{
		return const_cast<Img_c &>(IM_UnknownFlat(config));
	}
	Img_c &IM_UnknownSprite(const ConfigData &config);

	Img_c &IM_DigitFont_11x14();
	Img_c &IM_DigitFont_14x19();

	void IM_UnloadDummyTextures();
	void IM_ResetDummyTextures();

	void W_AddTexture(const SString &name, Img_c &&img, bool is_medusa);
	const Img_c *getTexture(const ConfigData &config, const SString &name, bool try_uppercase = false) const;
	Img_c *getMutableTexture(const ConfigData &config, const SString &name, bool try_uppercase = false)
	{
		return const_cast<Img_c *>(getTexture(config, name, try_uppercase));
	}
	int W_GetTextureHeight(const ConfigData &config, const SString &name) const;
	bool W_TextureCausesMedusa(const SString &name) const;
	bool W_TextureIsKnown(const ConfigData &config, const SString &name) const;
	void W_ClearTextures();
	const std::map<SString, Img_c> &getTextures() const
	{
		return textures;
	}

	void W_AddFlat(const SString &name, Img_c &&img);
	const Img_c *W_GetFlat(const ConfigData &config, const SString &name, bool try_uppercase = false) const;
	Img_c *getMutableFlat(const ConfigData &config, const SString &name, bool try_uppercase = false)
	{
		return const_cast<Img_c *>(W_GetFlat(config, name, try_uppercase));
	}
	bool W_FlatIsKnown(const ConfigData &config, const SString &name) const;
	void W_ClearFlats();
	const std::map<SString, Img_c> &getFlats() const
	{
		return flats;
	}

	void W_ClearSprites();

	void W_UnloadAllTextures();

public: // TODO: make private
	sprite_map_t sprites;

private:	
	std::map<SString, Img_c> textures;
	// textures which can cause the Medusa Effect in vanilla/chocolate DOOM
	std::map<SString, int> medusa_textures;
	std::map<SString, Img_c> flats;
	

	int missing_tex_color = 0;
	tl::optional<Img_c> missing_tex_image;

	int unknown_tex_color = 0;
	tl::optional<Img_c> unknown_tex_image;

	int special_tex_color = 0;
	tl::optional<Img_c> special_tex_image;

	int unknown_flat_color = 0;
	tl::optional<Img_c> unknown_flat_image;

	int unknown_sprite_color = 0;
	tl::optional<Img_c> unknown_sprite_image;

	tl::optional<Img_c> digit_font_11x14;
	tl::optional<Img_c> digit_font_14x19;
};


struct LumpNameCompare
{
	bool operator()(const Lump_c &lump1, const Lump_c &lump2) const;
};

//
// Manages the WAD loading
//
class MasterDir
{
public:
	
	void setGameWad(const std::shared_ptr<Wad_file> &gameWad)
	{
		game_wad = gameWad;
	}
	void RemoveEditWad();
	void ReplaceEditWad(const std::shared_ptr<Wad_file> &wad);
	void setResources(const std::vector<std::shared_ptr<Wad_file>> &wads)
	{
		resource_wads = wads;
	}
	void MasterDir_CloseAll();
	bool MasterDir_HaveFilename(const SString &chk_path) const;

	const Lump_c *findGlobalLump(const SString &name) const;
	const Lump_c *findFirstSpriteLump(const SString &stem) const;

	const std::shared_ptr<Wad_file> &gameWad() const
	{
		return game_wad;
	}
	const std::shared_ptr<Wad_file> &editWad() const noexcept
	{
		return edit_wad;
	}
	const std::shared_ptr<Wad_file> &activeWad() const
	{
		return edit_wad ? edit_wad : game_wad;
	}
	const std::vector<std::shared_ptr<Wad_file>> &resourceWads() const
	{
		return resource_wads;
	}
	
	std::vector<std::shared_ptr<Wad_file>> getAll() const
	{
		std::vector<std::shared_ptr<Wad_file>> result;
		result.reserve(resource_wads.size() + 2);
		if(game_wad)
			result.push_back(game_wad);
		result.insert(result.end(), resource_wads.begin(), resource_wads.end());
		if(edit_wad)
			result.push_back(edit_wad);
		return result;
	}
	
private:
	// the current PWAD, or NULL for none.
	// when present it is also at master_dir.back()
	std::shared_ptr<Wad_file> edit_wad;
	std::vector<std::shared_ptr<Wad_file>> resource_wads;
	std::shared_ptr<Wad_file> game_wad;
};

//
// Wad data, loaded during resource setup
//
struct WadData
{
	void W_LoadTextures(const ConfigData &config);

	const Img_c *getSprite(const ConfigData &config, int type, const LoadingData &loading);
	Img_c *getMutableSprite(const ConfigData &config, int type, const LoadingData &loading)
	{
		return const_cast<Img_c *>(getSprite(config, type, loading));
	}
	
	Failable<void> reloadResources(const std::shared_ptr<Wad_file> &gameWad, const ConfigData &config, const std::vector<std::shared_ptr<Wad_file>> &resourceWads) noexcept(false);

	ImageSet images;
	Palette palette;
	MasterDir master;
	
private:
	Failable<void> W_LoadPalette() noexcept(false);
	Failable<void> W_LoadColormap()
	{
		return palette.loadColormap(master.findGlobalLump("COLORMAP"));
	}
	void W_LoadFlats();
};

#endif /* WadData_h */
