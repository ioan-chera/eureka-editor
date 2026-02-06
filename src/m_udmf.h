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
	COUNT  // Sentinel value for array sizing
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
	COUNT  // Sentinel value for array sizing
};

bool UDMF_AddSideFeature(ConfigData &config, const char *featureName);
bool UDMF_HasSideFeature(const ConfigData &config, UDMF_SideFeature feature);

#endif
