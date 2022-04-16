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

class Sector;
class SString;
struct ConfigData;
struct Document;

class SideDef
{
public:
	int x_offset = 0;
	int y_offset = 0;
	int upper_tex = 0;
	int mid_tex = 0;
	int lower_tex = 0;
	int sector = 0;

	enum { F_X_OFFSET, F_Y_OFFSET, F_UPPER_TEX, F_MID_TEX, F_LOWER_TEX, F_SECTOR };

public:

	SString UpperTex() const;
	SString MidTex()   const;
	SString LowerTex() const;

	Sector *SecRef(const Document &doc) const;

	// use new_tex when >= 0, otherwise use default_wall_tex
	void SetDefaults(const ConfigData &config, bool two_sided, int new_tex = -1);
};

#endif
