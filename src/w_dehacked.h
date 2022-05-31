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

#ifndef __EUREKA_W_DEHACKED_H__
#define __EUREKA_W_DEHACKED_H__

#include "m_game.h"

#include <istream>

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

inline const char* DEH_FIELDS[] =
{
	"Initial frame = ",
	"Width = ",
	"ID # = ",
	"Bits = ",
	"Sprite number = ",
	"Sprite subnumber = "
};

enum
{
	SOLID = 1 << 1,	
	SPAWNCEILING = 1 << 8,
	SHADOW = 1 << 18
};
struct dehthing_t
{
	thingtype_t thing;
	int spawnframenum;
};

struct dehframe_t
{
	int spritenum;
	int subspritenum;
};

void loadDehackedFile(SString resource, ConfigData &config);
void readDehacked(std::istream *is, ConfigData &config);
void readThing(std::istream *is, ConfigData &config, dehthing_t *newthing, int *newthingid);
void readFrame(std::istream *is, dehframe_t *frame);
SString thingName(std::vector<SString> tokens);

#endif  /* __EUREKA_W_DEHACKED_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab

