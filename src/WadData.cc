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
const Lump_c *MasterDir::findFirstSpriteLump(const SString &stem) const
{
	SString firstName;
	const Lump_c *result = nullptr;
	std::vector<std::shared_ptr<Wad_file>> wads = getAll();
	for(auto it = wads.rbegin(); it != wads.rend(); ++it)
	{
		const Lump_c *lump = (*it)->findFirstSpriteLump(stem);
		if(!lump)
			continue;
		if(firstName.empty() || firstName.get() > lump->Name().get())
		{
			firstName = lump->Name();
			result = lump;
		}
	}
	return result;
}

Failable<void> WadData::W_LoadPalette() noexcept(false)
{
	const Lump_c *lump = master.findGlobalLump("PLAYPAL");
	if(!lump)
	{
		return fail("PLAYPAL lump not found.\n");
	}
	palette.loadPalette(*lump, config::usegamma, config::panel_gamma);
	images.IM_ResetDummyTextures();
	return {};
}

Failable<void> WadData::reloadResources(const std::shared_ptr<Wad_file> &gameWad, const ConfigData &config, const std::vector<std::shared_ptr<Wad_file>> &resourceWads) noexcept(false)
{
	// reset the master directory
	WadData newWad = *this;
	try
	{
		newWad.master.setGameWad(gameWad);
		newWad.master.setResources(resourceWads);
				
		// finally, load textures and stuff...
		attempt(newWad.W_LoadPalette());
		attempt(newWad.W_LoadColormap());
		
		newWad.W_LoadFlats();
		newWad.W_LoadTextures(config);
		newWad.images.W_ClearSprites();
	}
	catch(const std::runtime_error &e)
	{
		gLog.printf("Failed reloading resources: %s", e.what());
		return fail(e);
	}
	// Commit
	*this = newWad;
	return{};
}
