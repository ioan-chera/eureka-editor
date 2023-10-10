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
Lump_c *MasterDir::findFirstSpriteLump(const SString &stem) const
{
	SString firstName;
	Lump_c *result = nullptr;
	for(auto it = getDir().rbegin(); it != getDir().rend(); ++it)
	{
		Lump_c *lump = (*it)->findFirstSpriteLump(stem);
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

void WadData::W_LoadPalette()
{
	Lump_c *lump = master.findGlobalLump("PLAYPAL");
	if(!lump)
	{
		ThrowException("PLAYPAL lump not found.\n");
	}
	palette.loadPalette(*lump, config::usegamma, config::panel_gamma);
	images.IM_ResetDummyTextures();
}

void WadData::reloadResources(const LoadingData &loading, const ConfigData &config, const std::vector<std::shared_ptr<Wad_file>> &resourceWads)
{
	// reset the master directory
	if (master.edit_wad)
		master.MasterDir_Remove(master.edit_wad);

	master.MasterDir_CloseAll();

	// TODO: check result
	Main_LoadIWAD(loading);

	// load all resource wads
	for(const std::shared_ptr<Wad_file> &wad : resourceWads)
		master.MasterDir_Add(wad);

	if (master.edit_wad)
		master.MasterDir_Add(master.edit_wad);

	// finally, load textures and stuff...
	W_LoadPalette();
	W_LoadColormap();

	W_LoadFlats();
	W_LoadTextures(config);
	images.W_ClearSprites();
}
