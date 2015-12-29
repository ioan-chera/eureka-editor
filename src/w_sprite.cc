//------------------------------------------------------------------------
//  SPRITE LOADING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2015 Andrew Apted
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

#include "main.h"

#include <map>
#include <algorithm>
#include <string>

#include "m_game.h"      /* yg_picture_format */
#include "levels.h"
#include "w_loadpic.h"
#include "w_rawdef.h"
#include "w_sprite.h"


// maps type number to an image
typedef std::map<int, Img_c *> sprite_map_t;

static sprite_map_t sprites;


static Img_c * CreateDogSprite();


static void DeleteSprite(const sprite_map_t::value_type& P)
{
	delete P.second;
}

void W_ClearSprites()
{
	std::for_each(sprites.begin(), sprites.end(), DeleteSprite);

	sprites.clear();
}


// find sprite by prefix
Lump_c * Sprite_loc_by_root (const char *name)
{
	char buffer[16];

	strcpy(buffer, name);

	if (strlen(buffer) == 4)
		strcat(buffer, "A");

	if (strlen(buffer) == 5)
		strcat(buffer, "0");

	Lump_c *lump = W_FindSpriteLump(buffer);

	if (! lump)
	{
		buffer[5] = '1';
		lump = W_FindSpriteLump(buffer);
	}

	if (! lump)
	{
		strcat(buffer, "D1");
		lump = W_FindSpriteLump(buffer);
	}

	return lump;
}


Img_c * W_GetSprite(int type)
{
	sprite_map_t::iterator P = sprites.find(type);

	if (P != sprites.end ())
		return P->second;

	// sprite not in the list yet.  Add it.

	const thingtype_t *info = M_GetThingType(type);

	Img_c *result = NULL;

	if (y_stricmp(info->sprite, "NULL") != 0)
	{
		Lump_c *lump = Sprite_loc_by_root(info->sprite);
		if (! lump)
		{
			LogPrintf("Sprite not found: '%s'\n", info->sprite);

			// FUCK DOCUMENT SHIT
			if (y_stricmp(info->sprite, "DOGS") == 0)
				result = CreateDogSprite();
		}
		else
		{
			result = new Img_c ();

			if (! LoadPicture(*result, lump, info->sprite, 0, 0))
			{
				delete result;
				result = NULL;
			}
		}
	}

	// player color remapping
	// [ FIXME : put colors into game definition file ]
	// [ TODO  : support types 4001..4004 ]
	if (result && type >= 2 && type <= 4)
	{
		Img_c *old_img = result;

		switch (type)
		{
			case 2:
				result = old_img->color_remap(0x70, 0x7f, 0x60, 0x6f);
				break;

			case 3:
				result = old_img->color_remap(0x70, 0x7f, 0x40, 0x4f);
				break;

			case 4:
			default:
				result = old_img->color_remap(0x70, 0x7f, 0x20, 0x2f);
				break;
		}

		delete old_img;
	}

	// note that a NULL image is OK.  Our renderer will just ignore the
	// missing sprite.

	sprites[type] = result;
	return result;
}


//------------------------------------------------------------------------

//
// This dog sprite was sourced from OpenGameArt.org
// Authors are 'Benalene' and 'qudobup' (users on the OGA site).
// License is CC-BY 3.0 (Creative Commons Attribution license).
//

static const rgb_color_t dog_palette[] =
{
	0x302020ff,
	0x944921ff,
	0x000000ff,
	0x844119ff,
	0x311800ff,
	0x4A2400ff,
	0x633119ff,
};


static const char *dog_image_text[] =
{
	"       aaaa                                 ",
	"      abbbba                                ",
	"     abbbbbba                               ",
	" aaaabcbbbbbda                              ",
	"aeedbbbfbbbbda                              ",
	"aegdddbbdbbdbbaaaaaaaaaaaaaaaaa           a ",
	"affggddbgddgbccceeeeeeeeeeeeeeeaa        aba",
	" affgggdfggfccceeeeeeeeeeeeeefffgaaa   aaba ",
	"  afffaafgecccefffffffffffffffggggddaaabbba ",
	"   aaa  aeeccggggffffffffffffggddddbbbbbaa  ",
	"         accbdddggfffffffffffggdbbbbbbba    ",
	"          aabbdbddgfffffffffggddbaaaaaa     ",
	"            abbbbdddfffffffggdbbba          ",
	"            abbbbbbdddddddddddbbba          ",
	"           aeebbbbbbbbaaaabbbbbbbba         ",
	"           aeebbbbbaaa    aeebbbbbba        ",
	"          afebbbbaa       affeebbbba        ",
	"         agfbbbaa         aggffabbbba       ",
	"        agfebba           aggggaabbba       ",
	"      aadgfabba            addda abba       ",
	"     abbddaabbbaa           adddaabba       ",
	"    abbbba  abbbba          adbbaabba       ",
	"     aaaa    abbba         abbba  abba      ",
	"              aaa         abbba   abba      ",
	"                         abbba   abbba      ",
	"                          aaa     aaa       "
};


static Img_c * CreateDogSprite()
{
	return IM_CreateFromText(44, 26, dog_image_text, dog_palette, 7);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
