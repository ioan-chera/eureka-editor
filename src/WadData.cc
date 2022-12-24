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
#include "w_wad.h"

//
// Comparator for the master directory lumps
//
bool LumpNameCompare::operator()(const Lump_c &lump1, const Lump_c &lump2) const
{
	return lump1.Name() < lump2.Name();
}

//
// Add wad to list and take note of all its lumps
//
void WadAggregate::add(const std::shared_ptr<Wad_file> &wad)
{
	dir.push_back(wad);
	for(const LumpRef &ref : wad->getDir())
	{
		if(ref.ns != WadNamespace::Sprites)
			continue;
		mSpriteLumps.insert(*ref.lump);
	}
}

//
// Aggregate's remove call
//
void WadAggregate::remove(const std::shared_ptr<Wad_file> &wad)
{
	auto ENDP = std::remove(dir.begin(), dir.end(), wad);
	dir.erase(ENDP, dir.end());
}
