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

#include "WadData.h"
#include "m_config.h"
#include "m_loadsave.h"
#include "w_wad.h"

//
// Comparator for the master directory lumps
//
bool LumpNameCompare::operator()(const Lump_c &lump1, const Lump_c &lump2) const
{
	return lump1.Name() < lump2.Name();
}

//
// Locates the first four-stem sprite from the master dir
//
std::vector<SpriteLumpRef> MasterDir::findFirstSpriteLump(const SString &stem) const
{
	SString firstName;
	std::vector<SpriteLumpRef> result;
	std::vector<std::shared_ptr<Wad_file>> wads = getAll();
	for(auto it = wads.rbegin(); it != wads.rend(); ++it)
	{
		std::vector<SpriteLumpRef> spriteset = (*it)->findFirstSpriteLump(stem);
		if(spriteset.empty())
			continue;
		if(spriteset.size() == 1)
			return spriteset;
		if(result.empty())
			result = spriteset;
		else
		{
			for(size_t i = 0; i < spriteset.size(); ++i)
				if(!result[i].lump && spriteset[i].lump)
					result[i] = spriteset[i];
		}
	}
	return result;
}

void WadData::W_LoadPalette() noexcept(false)
{
	const Lump_c *lump = master.findGlobalLump("PLAYPAL");
	if(!lump)
	{
		ThrowException("PLAYPAL lump not found.\n");
	}
	palette.loadPalette(*lump, config::usegamma, config::panel_gamma);
	images.IM_ResetDummyTextures();
}

void WadData::reloadResources(const std::shared_ptr<Wad_file> &gameWad, const ConfigData &config, const std::vector<std::shared_ptr<Wad_file>> &resourceWads) noexcept(false)
{
	// reset the master directory
	WadData newWad = *this;
	try
	{
		newWad.master.setGameWad(gameWad);
		newWad.master.setResources(resourceWads);
				
		// finally, load textures and stuff...
		newWad.W_LoadPalette();
		newWad.W_LoadColormap();
		
		newWad.W_LoadFlats();
		newWad.W_LoadTextures(config);
		newWad.images.W_ClearSprites();
	}
	catch(const std::runtime_error &e)
	{
		gLog.printf("Failed reloading resources: %s", e.what());
		throw;
	}
	// Commit
	*this = newWad;
}
