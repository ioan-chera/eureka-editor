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

#include "m_game.h"
#include "m_parse.h"
#include "m_streams.h"
#include "w_dehacked.h"

#include <istream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

void WadData::W_LoadDehacked(ConfigData &config)
{
	for (int i = 0 ; i < (int)master.dir.size() ; i++)
	{
		std::cout << "Checking for DEHACKED in " << master.dir[i]->PathName() << "\n";
		Lump_c *dehlump = master.dir[i]->FindLump("DEHACKED");
		
		if (!dehlump)
		{
			std::cout << "None found.\n";
			continue;
		}
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
		std::map<int, thingtype_t> deh_thing_types;
		std::map<int, dehframe_t> frame_sprite_nums; // We use this to get new spawn sprites
		
		
		while (morelines)
		{
			if (dehline.startsWith("Thing"))
			{
				std::vector<SString> tokens;
				M_ParseLine(dehline, tokens, ParseOptions::noStrings);
				thingtype_t newthing;
				int newthingid = DEH_THING_NUM_TO_TYPE[atoi(tokens[1])];
				readThing(is, config, &newthing, &newthingid);
				newthing.desc = "";
				
				//Get name from tokens
				for (int i = 2; (long unsigned int)i < tokens.size(); i++)
				{
					newthing.desc += tokens[i];
					
					if ((long unsigned int)i != tokens.size()-1)
						newthing.desc += " ";
				}
				newthing.desc = newthing.desc.substr(1, newthing.desc.length()-2);
				
				if (newthingid != -1)
					config.thing_types[newthingid] = newthing;				
			}
			
			morelines = M_ReadTextLine(dehline, *is);
		}	
}

void readThing(std::istream *is, ConfigData &config, thingtype_t *newthing, int *newthingid)
{
	thingtype_t thing = config.thing_types[*newthingid];
	thing.sprite = config.thing_types[*newthingid].sprite;
	
	SString dehline;
	bool morelines = M_ReadTextLine(dehline, *is);
	
	while(dehline.good() && morelines)
	{
		morelines = M_ReadTextLine(dehline, *is);
	}
	
	*newthing = thing;
}
