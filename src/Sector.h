//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022-2026 Ioan Chera
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
#include <set>

struct ConfigData;

struct Sector
{
	static constexpr const int kDefaultDamageInterval = 32;
	static constexpr const double kDefaultFrictionFactor = 0.90625;
	static constexpr const double kDefaultMoveFactor = 0.03125;
	// Basic fields (raw-indexed)
	int floorh = 0;
	int ceilh = 0;
	StringID floor_tex;
	StringID ceil_tex;
	int light = 0;
	int type = 0;
	int tag = 0;

	// UDMF int fields (raw-indexed, continued)
	int lightfloor = 0;
	int lightceiling = 0;
	int flags = 0;
	StringID colormap;
	int damageamount = 0;
	int damageinterval = kDefaultDamageInterval;
	int leakiness = 0;
	int scrollfloormode = 0;
	int scrollceilingmode = 0;
	int thrustgroup = 0;
	int thrustlocation = 0;
	StringID skyfloor;
	StringID skyceiling;

	// UDMF double fields (member-pointer indexed)
	double xpanningfloor = 0.0;
	double ypanningfloor = 0.0;
	double xpanningceiling = 0.0;
	double ypanningceiling = 0.0;
	double xscalefloor = 1.0;
	double yscalefloor = 1.0;
	double xscaleceiling = 1.0;
	double yscaleceiling = 1.0;
	double rotationfloor = 0.0;
	double rotationceiling = 0.0;
	double gravity = 1.0;
	double xscrollfloor = 0.0;
	double yscrollfloor = 0.0;
	double xscrollceiling = 0.0;
	double yscrollceiling = 0.0;
	double xthrust = 0.0;
	double ythrust = 0.0;
	double frictionfactor = kDefaultFrictionFactor;
	double movefactor = kDefaultMoveFactor;

	// UDMF set field
	std::set<int> moreIDs;

	enum
	{
		FLAG_LIGHTFLOORABSOLUTE =   0x00000001,
		FLAG_LIGHTCEILINGABSOLUTE = 0x00000002,
		FLAG_SILENT =               0x00000004,
		FLAG_NOATTACK =             0x00000008,
		FLAG_HIDDEN =               0x00000010,
		FLAG_DAMAGEHAZARD =         0x00000020,
	};

	enum IntAddress
	{
		F_FLOORH,
		F_CEILH,
		F_LIGHT = 4,
		F_TYPE,
		F_TAG,
		F_LIGHTFLOOR,
		F_LIGHTCEILING,
		F_FLAGS,
		// 10 = F_COLORMAP (StringIDAddress)
		F_DAMAGEAMOUNT = 11,
		F_DAMAGEINTERVAL,
		F_LEAKINESS,
		F_SCROLLFLOORMODE,
		F_SCROLLCEILINGMODE,
		F_THRUSTGROUP,
		F_THRUSTLOCATION,
		// 18 = F_SKYFLOOR (StringIDAddress)
		// 19 = F_SKYCEILING (StringIDAddress)
	};

	enum StringIDAddress
	{
		F_FLOOR_TEX = 2,
		F_CEIL_TEX = 3,
		F_COLORMAP = 10,
		F_SKYFLOOR = 18,
		F_SKYCEILING = 19,
	};

	SString FloorTex() const noexcept;
	SString CeilTex() const noexcept;
	SString Colormap() const noexcept;
	SString SkyFloor() const noexcept;
	SString SkyCeiling() const noexcept;

	int HeadRoom() const
	{
		return ceilh - floorh;
	}

	void SetDefaults(const ConfigData &config);
};

#endif
