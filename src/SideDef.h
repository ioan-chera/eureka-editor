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

	// UDMF ints
	int light_top = 0;
	int light_mid = 0;
	int light_bottom = 0;

	// UDMF per-part offsets
	double offsetx_top = 0.0;
	double offsety_top = 0.0;
	double offsetx_mid = 0.0;
	double offsety_mid = 0.0;
	double offsetx_bottom = 0.0;
	double offsety_bottom = 0.0;

	// UDMF per-part scales
	double scalex_top = 1.0;
	double scaley_top = 1.0;
	double scalex_mid = 1.0;
	double scaley_mid = 1.0;
	double scalex_bottom = 1.0;
	double scaley_bottom = 1.0;

	enum IntAddress
	{
		F_X_OFFSET,
		F_Y_OFFSET,
		F_SECTOR = 5,
		F_LIGHT_TOP,
		F_LIGHT_MID,
		F_LIGHT_BOTTOM,
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

	// use new_tex when >= 0, otherwise use default_wall_tex
	void SetDefaults(const ConfigData &config, bool two_sided, StringID new_tex = StringID(-1));
};

#endif
