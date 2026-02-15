//------------------------------------------------------------------------
//  UDMF PARSING / WRITING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025 Ioan Chera
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

#ifndef M_UDMF_H_
#define M_UDMF_H_

struct ConfigData;

enum class UDMF_LineFeature : unsigned
{
	alpha,
	anycross,
	automapstyle,
	blockeverything,
	blockfloaters,
	blockhitscan,
	blocking,
	blocklandmonsters,
	blockmonsters,
	blockplayers,
	blockprojectiles,
	blocksight,
	blocksound,
	blockuse,
	checkswitchrange,
	clipmidtex,
	damagespecial,
	deathspecial,
	dontpegbottom,
	dontpegtop,
	firstsideonly,
	health,
	healthgroup,
	impact,
	jumpover,
	locknumber,
	midtex3d,
	midtex3dimpassible,
	missilecross,
	monsteractivate,
	monstercross,
	monsterpush,
	monsteruse,
	passuse,
	playercross,
	playerpush,
	playeruse,
	playeruseback,
	repeatspecial,
	revealed,
	translucent,
	transparent,
	twosided,
	wrapmidtex,
	COUNT
};

struct UDMFMapping
{
	enum class Category
	{
		line,
		thing
	};

	static const UDMFMapping* getForName(const char* name, Category category);

	const char *const name;
	const Category category;
	const int flagSet;
	const unsigned value;
};

bool UDMF_AddLineFeature(ConfigData &config, const char *featureName);
bool UDMF_HasLineFeature(const ConfigData &config, UDMF_LineFeature feature);

enum class UDMF_SideFeature : unsigned
{
	clipmidtex,
	light_bottom,
	light_mid,
	light_top,
	light,
	lightabsolute_bottom,
	lightabsolute_mid,
	lightabsolute_top,
	lightabsolute,
	nofakecontrast,
	offsetx_bottom,
	offsetx_mid,
	offsetx_top,
	offsety_bottom,
	offsety_mid,
	offsety_top,
	scalex_bottom,
	scalex_mid,
	scalex_top,
	scaley_bottom,
	scaley_mid,
	scaley_top,
	smoothlighting,
	wrapmidtex,
	xscroll,
	xscrollbottom,
	xscrollmid,
	xscrolltop,
	yscroll,
	yscrollbottom,
	yscrollmid,
	yscrolltop,
	COUNT
};

bool UDMF_AddSideFeature(ConfigData &config, const char *featureName);
bool UDMF_HasSideFeature(const ConfigData &config, UDMF_SideFeature feature);

enum class UDMF_SectorFeature : unsigned
{
	colormap,
	damageamount,
	damagehazard,
	damageinterval,
	frictionfactor,
	gravity,
	hidden,
	leakiness,
	lightceiling,
	lightceilingabsolute,
	lightfloor,
	lightfloorabsolute,
	moreids,
	movefactor,
	noattack,
	rotationceiling,
	rotationfloor,
	scrollceilingmode,
	scrollfloormode,
	silent,
	skyceiling,
	skyfloor,
	thrustgroup,
	thrustlocation,
	xpanningceiling,
	xpanningfloor,
	xscaleceiling,
	xscalefloor,
	xscrollceiling,
	xscrollfloor,
	xthrust,
	ypanningceiling,
	ypanningfloor,
	yscaleceiling,
	yscalefloor,
	yscrollceiling,
	yscrollfloor,
	ythrust,
	COUNT
};

bool UDMF_AddSectorFeature(ConfigData &config, const char *featureName);
bool UDMF_HasSectorFeature(const ConfigData &config, UDMF_SectorFeature feature);

#endif
