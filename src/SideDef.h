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

#ifndef SIDEDEF_H_
#define SIDEDEF_H_

#include "m_strings.h"

struct Sector;
struct ConfigData;
struct Document;

struct SideDef
{
	int x_offset = 0;
	int y_offset = 0;
	StringID upper_tex;
	StringID mid_tex;
	StringID lower_tex;
	int sector = 0;

	enum IntAddress
	{
		F_X_OFFSET,
		F_Y_OFFSET,
		F_SECTOR = 5,
	};

	enum StringIDAddress
	{
		F_UPPER_TEX = 2,
		F_MID_TEX,
		F_LOWER_TEX,
	};

	SString UpperTex() const;
	SString MidTex()   const;
	SString LowerTex() const;

	Sector *SecRef(const Document &doc) const;

	// use new_tex when >= 0, otherwise use default_wall_tex
	void SetDefaults(const ConfigData &config, bool two_sided, StringID new_tex = StringID(-1));
};

#endif
