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
	anycross,
	checkswitchrange,
	damagespecial,
	deathspecial,

	firstsideonly,
	impact,
	missilecross,
	monsteractivate,

	monstercross,
	monsterpush,
	monsteruse,
	playercross,

	playerpush,
	playeruse,
	playeruseback,
	repeatspecial,
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

#endif
