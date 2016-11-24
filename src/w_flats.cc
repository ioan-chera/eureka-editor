//------------------------------------------------------------------------
//  FLAT LOADING
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

#include "main.h"

#include <map>
#include <algorithm>
#include <string>

#include "m_game.h"      /* yg_picture_format */
#include "levels.h"
#include "w_loadpic.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "w_flats.h"
#include "w_wad.h"


std::map<std::string, Img_c *> flats;


static void DeleteFlat(const std::map<std::string, Img_c *>::value_type& P)
{
	delete P.second;
}


static void W_ClearFlats()
{
	std::for_each(flats.begin(), flats.end(), DeleteFlat);

	flats.clear();
}


static void W_AddFlat(const char *name, Img_c *img)
{
	// find any existing one with same name, and free it

	std::string flat_str = name;

	std::map<std::string, Img_c *>::iterator P = flats.find(flat_str);

	if (P != flats.end())
	{
		delete P->second;

		P->second = img;
	}
	else
	{
		flats[flat_str] = img;
	}
}


static Img_c * LoadFlatImage(const char *name, Lump_c *lump)
{
	// TODO: check size == 64*64

	Img_c *img = new Img_c(64, 64, false);

	int size = 64 * 64;

	byte *raw = new byte[size];

	if (! (lump->Seek() && lump->Read(raw, size)))
		FatalError("Error reading flat from WAD.\n");

	for (int i = 0 ; i < size ; i++)
	{
		img_pixel_t pix = raw[i];

		if (pix == TRANS_PIXEL)
			pix = trans_replace;

		img->wbuf() [i] = pix;
	}

	delete[] raw;

	return img;
}


void W_LoadFlats()
{
	W_ClearFlats();

	for (int i = 0 ; i < (int)master_dir.size() ; i++)
	{
		LogPrintf("Loading Flats from WAD #%d\n", i+1);

		Wad_file *wf = master_dir[i];

		for (int k = 0 ; k < (int)wf->flats.size() ; k++)
		{
			Lump_c *lump = wf->GetLump(wf->flats[k]);

			DebugPrintf("  Flat %d : '%s'\n", k, lump->Name());

			Img_c * img = LoadFlatImage(lump->Name(), lump);

			if (img)
				W_AddFlat(lump->Name(), img);
		}
	}
}


Img_c * W_GetFlat(const char *name, bool try_uppercase)
{
	std::string f_str = name;

	std::map<std::string, Img_c *>::iterator P = flats.find(f_str);

	if (P != flats.end())
		return P->second;

	if (try_uppercase)
	{
		return W_GetFlat(NormalizeTex(name), false);
	}

	return NULL;
}


bool W_FlatIsKnown(const char *name)
{
	std::string f_str = name;

	std::map<std::string, Img_c *>::iterator P = flats.find(f_str);

	return (P != flats.end());
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
