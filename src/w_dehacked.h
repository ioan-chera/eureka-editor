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
	SPAWNFRAME = 0,
	RADIUS,
	ID,
	FLAGS
};

inline const char* DEH_FIELDS[] =
{
	"Initial frame = ",
	"Width = ",
	"ID # = ",
	"Bits = "
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
SString thingName(std::vector<SString> tokens);

// Consts

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

inline const SString SPRITE_BY_INDEX[] =
{
	"TROO", "SHTG", "PUNG", "PISG", "PISF", "SHTF", "SHT2",
	"CHGG", "CHGF", "MISG", "MISF", "SAWG", "PLSG", "PLSF",
	"BFGG", "BFGF", "BLUD", "PUFF", "BAL1", "BAL2", "PLSS",
	"PLSE", "MISL", "BFS1", "BFE1", "BFE2", "TFOG", "IFOG",
	"PLAY", "POSS", "SPOS", "VILE", "FIRE", "FATB", "FBXP",
	"SKEL", "MANF", "FATT", "CPOS", "SARG", "HEAD", "BAL7",
	"BOSS", "BOS2", "SKUL", "SPID", "BSPI", "APLS", "APBX",
	"CYBR", "PAIN", "SSWV", "KEEN", "BBRN", "BOSF", "ARM1",
	"ARM2", "BAR1", "BEXP", "FCAN", "BON1", "BON2", "BKEY",
	"RKEY", "YKEY", "BSKU", "RSKU", "YSKU", "STIM", "MEDI",
	"SOUL", "PINV", "PSTR", "PINS", "MEGA", "SUIT", "PMAP",
	"PVIS", "CLIP", "AMMO", "ROCK", "BROK", "CELL", "CELP",
	"SHEL", "SBOX", "BPAK", "BFUG", "MGUN", "CSAW", "LAUN",
	"PLAS", "SHOT", "SGN2", "COLU", "SMT2", "GOR1", "POL2",
	"POL5", "POL4", "POL3", "POL1", "POL6", "GOR2", "GOR3",
	"GOR4", "GOR5", "SMIT", "COL1", "COL2", "COL3", "COL4",
	"CAND", "CBRA", "COL6", "TRE1", "TRE2", "ELEC", "CEYE",
	"FSKU", "COL5", "TBLU", "TGRN", "TRED", "SMBT", "SMGT",
	"SMRT", "HDB1", "HDB2", "HDB3", "HDB4", "HDB5", "HDB6",
	"POB1", "POB2", "BRS1", "TLMP", "TLP2", "TNT1", "DOGS",
	"PLS1", "PLS2", "BON3", "BON4"
};



#endif  /* __EUREKA_W_DEHACKED_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab

