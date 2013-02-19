//------------------------------------------------------------------------
//  SPRITE LOADING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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

#include "m_dialog.h"
#include "m_game.h"      /* yg_picture_format */
#include "levels.h"
#include "w_loadpic.h"
#include "w_rawdef.h"
#include "w_sprite.h"


// maps type number to an image
typedef std::map<int, Img *> sprite_map_t;

static sprite_map_t sprites;


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


Img * W_GetSprite(int type)
{
	sprite_map_t::iterator P = sprites.find(type);

	if (P != sprites.end ())
		return P->second;

	// sprite not in the list yet.  Add it.

	const thingtype_t *info = M_GetThingType(type);

	Img *result = NULL;

	if (y_stricmp(info->sprite, "NULL") != 0)
	{
		Lump_c *lump = Sprite_loc_by_root(info->sprite);
		if (! lump)
		{
			LogPrintf("Sprite not found: '%s'\n", info->sprite);
		}
		else
		{
			result = new Img ();

			if (! LoadPicture(*result, lump, info->sprite, 0, 0))
			{
				delete result;
				result = NULL;
			}
		}
	}

	// note that a NULL image is OK.  Our renderer will just ignore the
	// missing sprite.

	sprites[type] = result;
	return result;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
