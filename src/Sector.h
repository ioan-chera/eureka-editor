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

#ifndef SECTOR_H_
#define SECTOR_H_

#include "m_strings.h"

class SString;
struct ConfigData;

class Sector
{
public:
	int floorh = 0;
	int ceilh = 0;
	StringID floor_tex;
	StringID ceil_tex;
	int light = 0;
	int type = 0;
	int tag = 0;

	enum { F_FLOORH, F_CEILH, F_FLOOR_TEX, F_CEIL_TEX, F_LIGHT, F_TYPE, F_TAG };

public:
	SString FloorTex() const;
	SString CeilTex() const;

	int HeadRoom() const
	{
		return ceilh - floorh;
	}

	void SetDefaults(const ConfigData &config);
};

#endif
