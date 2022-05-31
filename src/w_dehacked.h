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

void loadDehackedFile(SString resource, ConfigData &config);
void readDehacked(std::istream *is, ConfigData &config);
void readThing(std::istream *is, ConfigData &config, thingtype_t *newthing, int *newthingid);

//Relevant DEH values for editor things
enum
{
	SPAWNFRAME,
	FLAGS,
	RADIUS,
	ID
};

const SString DEH_FIELDS[] =
{
	"Initial frame = ",
	"Bits = ",
	"Width = ",
	"ID # = "
};

struct dehframe_t
{
	int number;
	int subnumber;
};

const int DEH_THING_NUM_TO_TYPE[] = 
{
	-1, -1, 3004, 9, 64, -1, 66, -1, -1, 67, -1, 65, 3001, 3002, 58, 3005, 3003, -1, 69,
	3006, 7, 68, 16, 71, 84, 72, 88, 89, 87, -1, -1, 2035, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, 14, -1, 2018, 2019, 2014, 2015, 5, 13, 6, 39, 38, 40, 2011, 2012, 2013, 2022, 
	2023, 2024, 2025, 2026, 2045, 83, 2007, 2048, 2010, 2046, 2047, 17, 2008, 2049, 8,
	2006, 2002, 2005, 2003, 2004, 2001, 82, 85, 86, 2028, 30, 31, 32, 33, 37, 36, 41, 42,
	43, 44, 45, 46, 55, 56, 57, 47, 48, 34, 35, 49, 50, 51, 52, 53, 59, 60, 61, 62, 63, 22, 15, 18,
	21, 23, 20, 19, 10, 12, 28, 24, 27, 29, 25, 26, 54, 70, 73, 74, 75, 76, 77, 78, 79,
	80, 81
};

#endif  /* __EUREKA_W_DEHACKED_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab

