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

#include "Instance.h"
#include "main.h"

#include "dehconsts.h"
#include "m_game.h"
#include "m_parse.h"
#include "m_streams.h"
#include "w_dehacked.h"

#include <istream>
#include <map>
#include <sstream>
#include <string>

void WadData::W_LoadDehacked(ConfigData &config)
{
	for (int i = 0 ; i < (int)master.dir.size() ; i++)
	{
		Lump_c *dehlump = master.dir[i]->FindLump("DEHACKED");
		
		if (!dehlump)
			continue;
			
		std::istringstream iss((char*)dehlump->getData());
		readDehacked(&iss, config);
	}
}

void loadDehackedFile(const SString resource, ConfigData &config)
{
	std::ifstream ifs(resource.get(), std::ifstream::in);
	readDehacked(&ifs, config);
}

void readDehacked(std::istream *is, ConfigData &config)
{
		SString dehline;
		bool morelines = M_ReadTextLine(dehline, *is);
		std::map<int, dehthing_t> deh_things;
		std::map<int, dehframe_t> spawn_frames;
		std::map<SString, SString> renamed_sprites;
		
		while (morelines)
		{
			
			//Get Dehacked Thing
			if (dehline.startsWith("Thing"))
			{
				std::vector<SString> tokens;
				M_ParseLine(dehline, tokens, ParseOptions::noStrings);
				
				dehthing_t newthing;
				int dehnum = atoi(tokens[1]);
				int newthingid = -1;
				
				if (dehnum < 145)
					newthingid = DEH_THING_NUM_TO_TYPE[dehnum];
				newthing.spawnframenum = DEH_THING_NUM_TO_SPRITE[dehnum];
				
				readThing(is, config, &newthing, &newthingid);
				newthing.thing.desc = thingName(tokens);
				
				//No things with DoomEd Number -1
				if (newthingid == -1)
					config.thing_types.erase(newthingid);
				else
					deh_things[newthingid] = newthing;
			}
			
			//Get Dehacked Frame
			else if (dehline.startsWith("Frame"))
			{
				std::vector<SString> tokens;
				M_ParseLine(dehline, tokens, ParseOptions::noStrings);
				
				int framenum = atoi(tokens[1]);
				dehframe_t frame;
				
				if (framenum < 1089)
					frame = DEH_FRAMES[framenum]; //Init with default value
				
				else
					frame = DEHEXTRA_FRAME;
					
					
				readFrame(is, &frame);
				
				spawn_frames[framenum] = frame;
			}
			
			//Get changed sprite names
			else if (dehline == "Text 4 4")
			{
				morelines = M_ReadTextLine(dehline, *is);
				
				if (morelines)
				{
					SString oldname = dehline.substr(0, 4);
					SString newname = dehline.substr(4, 4);
					renamed_sprites[oldname] = newname;
				}
			}
			
			morelines = M_ReadTextLine(dehline, *is);
		}	
		
		//Apply DEHACKED things to editor config
		for (std::map<int, dehthing_t>::iterator it = deh_things.begin(); it != deh_things.end(); it++)
		{
			int spawnframenum = it->second.spawnframenum;
			dehframe_t spawnframe;
			
			if (spawn_frames.find(spawnframenum) != spawn_frames.end())
				spawnframe = spawn_frames[spawnframenum];
			
			else if (spawnframenum < 1089)
				spawnframe = DEH_FRAMES[spawnframenum]; //Get default
			
			else
				spawnframe = DEHEXTRA_FRAME;
				
			if (spawnframe.bright)
				it->second.thing.flags |= THINGDEF_LIT;
			else
				it->second.thing.flags &= ~THINGDEF_LIT;
				
			SString sprite = ""; 
			
			if (spawnframe.spritenum < 145)
				sprite = SPRITE_BY_INDEX[spawnframe.spritenum];
			else if (spawnframe.spritenum < 155)
				sprite = "SP0" + std::to_string(spawnframe.spritenum - 145);
			else if (spawnframe.spritenum < 244)
				sprite = "SP" + std::to_string(spawnframe.spritenum - 145);
			
			if (renamed_sprites.find(sprite) != renamed_sprites.end())
			{
				sprite = renamed_sprites[sprite];
			}
			
			//This shouldn't happen.
			if (sprite.empty())
				continue;
			
			if (spawnframe.subspritenum > 0)
				sprite += ('A' + spawnframe.subspritenum);
			
			it->second.thing.sprite = sprite;
			config.thing_types[it->first] = it->second.thing;
		}
}

void readThing(std::istream *is, ConfigData &config, dehthing_t *newthing, int *newthingid)
{
	thingtype_t thing = config.thing_types[*newthingid];
	thing.sprite = config.thing_types[*newthingid].sprite;
	
	SString dehline;
	bool morelines = M_ReadTextLine(dehline, *is);
	
	while(dehline.good() && morelines)
	{
		if (dehline.startsWith(DEH_FIELDS[SPAWNFRAME]))
			newthing->spawnframenum = atoi(dehline.substr(dehline.find("=") + 2));

		else if (dehline.startsWith(DEH_FIELDS[RADIUS]))
		{
			newthing->thing.radius = (short)(atoi(dehline.substr(dehline.find("=") + 2)) >> 16);
			gLog.printf("%d\n", newthing->thing.radius);
		}

		else if (dehline.startsWith(DEH_FIELDS[ID]))
			*newthingid = atoi(dehline.substr(dehline.find("=") + 2));
			
		else if (dehline.startsWith(DEH_FIELDS[FLAGS]))
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
		}
			
		morelines = M_ReadTextLine(dehline, *is);
	}
	
	newthing->thing = thing;
}

void readFrame(std::istream *is, dehframe_t *frame)
{
	SString dehline;
	bool morelines = M_ReadTextLine(dehline, *is);
	
	while(dehline.good() && morelines)
	{
		if (dehline.startsWith(DEH_FIELDS[SPRITENUM]))
			frame->spritenum = atoi(dehline.substr(dehline.find("=") + 2));
			
		else if (dehline.startsWith(DEH_FIELDS[SUBSPRITENUM]))
		{
			int subnumber = atoi(dehline.substr(dehline.find("=") + 2));
			
			if (subnumber & 0x8000)
			{
				frame->bright = true;
				subnumber &= (~0x8000);
			}
			
			frame->subspritenum = subnumber;
		}
			
		morelines = M_ReadTextLine(dehline, *is);
	}
}

SString thingName(std::vector<SString> tokens)
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
