//------------------------------------------------------------------------
//  DEHACKED
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

// THANKS TO Isaac Colón (https://github.com/iccolon818) for this contribution. I added reformatting
// and some input checking.

#include "w_dehacked.h"

#include "dehconsts.h"
#include "lib_util.h"
#include "m_game.h"
#include "m_parse.h"
#include "m_streams.h"
#include "m_strings.h"
#include "sys_debug.h"
#include "w_wad.h"
#include "WadData.h"
#include <map>
#include <fstream>
#include <sstream>
#include <assert.h>

namespace dehacked
{

//Relevant DEH values for editor things
enum
{
	// Things
	SPAWNFRAME = 0,
	RADIUS,
	ID,
	FLAGS,

	//Frames
	SPRITENUM,
	SUBSPRITENUM
};

enum
{
	SOLID = 1 << 1,
	SPAWNCEILING = 1 << 8,
	SHADOW = 1 << 18
};

static const char* FIELDS[] =
{
	"Initial frame = ",
	"Width = ",
	"ID # = ",
	"Bits = ",
	"Sprite number = ",
	"Sprite subnumber = "
};

struct thing_t
{
	thingtype_t thing;
	int spawnframenum;
};

inline const struct {SString cat; char group;} DB_CATEGORIES[] =
{
	{"Monsters", 'm'},
	{"Weapons", 'w'},
	{"Ammunition", 'a'},
	{"Health and Armor", 'h'},
	{"Powerups", 'b'},
	{"Keys", 'k'},
	{"Obstacles", 'd'},
	{"Light sources", 'l'},
	{"Decoration", 'g'}
};

static void readThing(std::istream &is, const ConfigData &config, thing_t &newthing, int &newthingid)
{
	const thingtype_t *configThing = get(config.thing_types, newthingid);
	thingtype_t thing = {};
	if(configThing)
		thing = *configThing;
	
	SString dehline;
	bool morelines = M_ReadTextLine(dehline, is);

	while(dehline.good() && morelines)
	{
		if (dehline.startsWith(FIELDS[SPAWNFRAME]))
			newthing.spawnframenum = atoi(dehline.substr(dehline.find("=") + 2));

		else if (dehline.startsWith(FIELDS[RADIUS]))
			thing.radius = (short)(atoi(dehline.substr(dehline.find("=") + 2)) >> 16);

		else if (dehline.startsWith(FIELDS[ID]))
			newthingid = atoi(dehline.substr(dehline.find("=") + 2));

		else if (dehline.startsWith(FIELDS[FLAGS]))
		{
			SString dehbits = dehline.substr(dehline.find("=") + 2);
			bool vanillabits = true;

			for (int i = 0; (size_t)i < dehbits.length(); i++)
			{
				if (dehbits[i] < '0' || dehbits[i] > '9')
					vanillabits = false;
			}

			if (vanillabits)
			{
				int bits = atoi(dehbits);

				if (bits ^ SOLID)
					thing.flags |= THINGDEF_PASS;
				else
					thing.flags &= ~THINGDEF_PASS;

				if (bits & SPAWNCEILING)
					thing.flags |= THINGDEF_CEIL;
				else
					thing.flags &= ~THINGDEF_CEIL;

				if (bits & SHADOW)
					thing.flags |= THINGDEF_INVIS;
				else
					thing.flags &= ~THINGDEF_INVIS;
			}
			else
			{	if (dehbits.find("SOLID") != std::string::npos)
					thing.flags |= THINGDEF_PASS;
				else
					thing.flags &= ~THINGDEF_PASS;

				if (dehbits.find("SPAWNCEILING") != std::string::npos)
					thing.flags |= THINGDEF_CEIL;
				else
					thing.flags &= ~THINGDEF_CEIL;

				if (dehbits.find("SHADOW") != std::string::npos)
					thing.flags |= THINGDEF_INVIS;
				else
					thing.flags &= ~THINGDEF_INVIS;

			}
		}
		else if (dehline.startsWith("#$Editor category = "))
		{
			SString category = dehline.substr(dehline.find("=") + 2);

			for (int i = 0; i < 9; i++)
			{
				if (category == DB_CATEGORIES[i].cat)
				{
					thing.group = DB_CATEGORIES[i].group;
					break;
				}
			}
		}

		morelines = M_ReadTextLine(dehline, is);
	}

	newthing.thing = thing;
}

static SString thingName(std::vector<SString> tokens)
{
	SString ret = "";
	for (int i = 2; (size_t)i < tokens.size(); i++)
	{
		ret += tokens[i];

		if ((long unsigned int)i != tokens.size()-1)
			ret += " ";
	}
	return ret.substr(1, ret.length()-2);
}

static void readFrame(std::istream &is, frame_t &frame)
{
	SString dehline;
	bool morelines = M_ReadTextLine(dehline, is);

	while(dehline.good() && morelines)
	{
		if (dehline.startsWith(FIELDS[SPRITENUM]))
			frame.spritenum = atoi(dehline.substr(dehline.find("=") + 2));

		else if (dehline.startsWith(FIELDS[SUBSPRITENUM]))
		{
			int subnumber = atoi(dehline.substr(dehline.find("=") + 2));

			if (subnumber & 0x8000)
			{
				frame.bright = true;
				subnumber &= (~0x8000);
			}

			frame.subspritenum = subnumber;
		}

		morelines = M_ReadTextLine(dehline, is);
	}
}

static void read(std::istream &is, ConfigData &config)
{
	SString dehline;
	bool morelines;
	std::map<int, thing_t> deh_things;
	std::map<int, frame_t> spawn_frames;
	std::map<int, SString> dsdhacked_sprites;
	std::map<SString, SString> renamed_sprites;
	
	auto checkCount = [](int count, const char *label)
	{
		assert(count >= 0);
		if(count <= 1)
		{
			gLog.printf("Missing number for %s\n", label);
			return false;
		}
		return true;
	};
	
	auto getNum = [](const std::vector<SString> &tokens, const char *label, int &dehnum)
	{
		char *endptr = nullptr;
		dehnum = (int)strtol(tokens[1].c_str(), &endptr, 10);
		if(endptr == tokens[1].c_str())
		{
			gLog.printf("Invalid %s number %s\n", label, tokens[1].c_str());
			return false;
		}
		
		return true;
	};

	for(morelines = M_ReadTextLine(dehline, is); morelines; morelines = M_ReadTextLine(dehline, is))
	{
		//Get Dehacked Thing
		if (dehline.startsWith("Thing"))
		{
			std::vector<SString> tokens;
			int count = M_ParseLine(dehline, tokens, ParseOptions::noStrings);
			if(!checkCount(count, "Thing"))
				continue;

			thing_t newthing = {};
			int dehnum;
			if(!getNum(tokens, "Thing", dehnum))
				continue;
			int newthingid = -1;

			if (dehnum < (int)lengthof(THING_NUM_TO_TYPE))
				newthingid = THING_NUM_TO_TYPE[dehnum];
			if(dehnum < (int)lengthof(THING_NUM_TO_SPRITE))
				newthing.spawnframenum = THING_NUM_TO_SPRITE[dehnum];

			int oldthingid = newthingid;
			readThing(is, config, newthing, newthingid);
			newthing.thing.desc = thingName(tokens);

			//No things with DoomEd Number -1
			if (newthingid == -1)
				config.thing_types.erase(newthingid);
			else
			{
				if(newthingid != oldthingid)
					config.thing_types.erase(oldthingid);
				deh_things[newthingid] = newthing;
			}
		}

		//Get Dehacked Frame
		else if (dehline.startsWith("Frame"))
		{
			std::vector<SString> tokens;
			int count = M_ParseLine(dehline, tokens, ParseOptions::noStrings);
			if(!checkCount(count, "Frame"))
				continue;

			int framenum;
			if(!getNum(tokens, "Frame", framenum))
				continue;
			frame_t frame = {};

			if (framenum < (int)(lengthof(FRAMES) - 1))
				frame = FRAMES[framenum]; //Init with default value

			else
				frame = DEHEXTRA_FRAME;


			readFrame(is, frame);

			spawn_frames[framenum] = frame;
		}

		//Get changed sprite names
		else if (dehline == "Text 4 4")
		{
			morelines = M_ReadTextLine(dehline, is);

			if (morelines)
			{
				SString oldname = dehline.substr(0, 4);
				SString newname = dehline.substr(4, 4);
				renamed_sprites[oldname] = newname;
			}
		}
		//Read DSDHacked sprites table
		else if (dehline == "[SPRITES]")
		{
			morelines = M_ReadTextLine(dehline, is);

			while(morelines && dehline.good())
			{
				std::vector<SString> tokens;
				int count = M_ParseLine(dehline, tokens, ParseOptions::noStrings);
				assert(count >= 0);
				if(count < 3)
				{
					gLog.printf("Invalid [SPRITES] syntax, found only %d words\n", count);
					continue;
				}
				int spritenum = atoi(tokens[0]);
				dsdhacked_sprites[spritenum] = tokens[2];
				morelines = M_ReadTextLine(dehline, is);
			}
		}
	}

	//Apply DEHACKED things to editor config
	for (std::map<int, thing_t>::iterator it = deh_things.begin(); it != deh_things.end(); it++)
	{
		int spawnframenum = it->second.spawnframenum;
		frame_t spawnframe = {};

		if (spawn_frames.find(spawnframenum) != spawn_frames.end())
			spawnframe = spawn_frames[spawnframenum];

		else if (spawnframenum < (int)(lengthof(FRAMES) - 1))
			spawnframe = FRAMES[spawnframenum]; //Get default

		else
			spawnframe = DEHEXTRA_FRAME;

		if (spawnframe.bright)
			it->second.thing.flags |= THINGDEF_LIT;
		else
			it->second.thing.flags &= ~THINGDEF_LIT;

		SString sprite = "";

		if (dsdhacked_sprites.find(spawnframe.spritenum) != dsdhacked_sprites.end())
			sprite = dsdhacked_sprites[spawnframe.spritenum];
		else if (spawnframe.spritenum < (int)lengthof(SPRITE_BY_INDEX))
			sprite = SPRITE_BY_INDEX[spawnframe.spritenum];
		else if (spawnframe.spritenum < (int)(lengthof(SPRITE_BY_INDEX) + 10))
			sprite = "SP0" + std::to_string(spawnframe.spritenum - lengthof(SPRITE_BY_INDEX));
		else if (spawnframe.spritenum < (int)(lengthof(SPRITE_BY_INDEX) + 99))
			sprite = "SP" + std::to_string(spawnframe.spritenum - lengthof(SPRITE_BY_INDEX));

		if (renamed_sprites.find(sprite) != renamed_sprites.end())
		{
			sprite = renamed_sprites[sprite];
		}

		//This shouldn't happen.
		if (sprite.empty())
			continue;

		if (spawnframe.subspritenum > 0)
			sprite += char('A' + spawnframe.subspritenum);

		it->second.thing.sprite = sprite;
		config.thing_types[it->first] = it->second.thing;
	}
}

bool loadFile(const fs::path &resource, ConfigData &config)
{
	std::ifstream ifs(resource);
	if(!ifs.is_open())
		return false;
	
	read(ifs, config);
	return true;
}

void loadLumps(const MasterDir &master, ConfigData &config)
{
	auto wads = master.getAll();
	for (const auto &wad : wads)
	{
		const Lump_c *dehlump = wad->FindLump("DEHACKED");

		if (!dehlump)
			continue;

		std::istringstream iss(std::string(reinterpret_cast<const char *>(dehlump->getData().data()), dehlump->Length()));
		read(iss, config);
	}
}
}
