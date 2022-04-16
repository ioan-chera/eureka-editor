//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022 Ioan Chera
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

#include "Sector.h"
#include "e_basis.h"
#include "m_game.h"
#include "m_strings.h"

SString Sector::FloorTex() const
{
	return global::basis_strtab.get(floor_tex);
}

SString Sector::CeilTex() const
{
	return global::basis_strtab.get(ceil_tex);
}

void Sector::SetDefaults(const ConfigData &config)
{
	floorh = global::default_floor_h;
	 ceilh = global::default_ceil_h;

	floor_tex = BA_InternaliseString(config.default_floor_tex);
	 ceil_tex = BA_InternaliseString(config.default_ceil_tex);

	light = global::default_light_level;
}
