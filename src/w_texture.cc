//------------------------------------------------------------------------
//  TEXTURE LOADING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
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
#include "w_texture.h"


std::map<std::string, Img *> textures;


static void DeleteTex(const std::map<std::string, Img *>::value_type& P)
{
	delete P.second;
}

static void W_ClearTextures()
{
	std::for_each(textures.begin(), textures.end(), DeleteTex);

	textures.clear();
}


static void LoadTextureLump(Lump_c *lump, byte *pnames, int pname_size)
{
	// skip size word at front of PNAMES
	pnames += 4;

	pname_size /= 8;

	// load TEXTUREx data into memory for easier processing
	byte *tex_data;
	int tex_length = W_LoadLumpData(lump, &tex_data);

	// shut the fuck up, compiler
	(void) tex_length;

	// at the front of the TEXTUREx lump are some 4-byte integers
	s32_t *tex_data_s32 = (s32_t *)tex_data;

	int num_tex = LE_S32(tex_data_s32[0]);

	// FIXME validate num_tex
	
	// Note: we skip the first entry (e.g. AASHITTY) which is not really
    //       usable (in the DOOM engine the #0 texture is not drawn).

	for (int n = 1 ; n < num_tex ; n++)
	{
		int offset = LE_S32(tex_data_s32[1+n]);

		// FIXME: validate offset

		const raw_texture_t *raw = (const raw_texture_t *)(tex_data + offset);

		// create the new Img
		int width  = LE_U16(raw->width);
		int height = LE_U16(raw->height);
DebugPrintf("Texture [%.8s] : %dx%d\n", raw->name, width, height);

		if (width == 0 || height == 0)
			FatalError("W_InitTextures: Texture '%.8s' has zero size\n", raw->name);

		Img *img = new Img(width, height, false);

		// apply all the patches
		int num_patches = LE_S16(raw->patch_count);
		if (! num_patches)
			FatalError("W_InitTextures: Texture '%.8s' has no patches\n", raw->name);

		const raw_patchdef_t *patdef = (const raw_patchdef_t *) & raw->patches[0];

		for (int j = 0 ; j < num_patches ; j++, patdef++)
		{
			int xofs = LE_S16(patdef->x_origin);
			int yofs = LE_S16(patdef->y_origin);
			int pname_idx = LE_U16(patdef->pname);

			// AYM 1998-08-08: Yes, that's weird but that's what Doom
			// does. Without these two lines, the few textures that have
			// patches with negative y-offsets (BIGDOOR7, SKY1, TEKWALL1,
			// TEKWALL5 and a few others) would not look in the texture
			// viewer quite like in Doom. This should be mentioned in
			// the UDS, by the way.
			if (yofs < 0)
				yofs = 0;

			if (pname_idx >= pname_size)
			{
				LogPrintf("Invalid pname in texture '%.8s'\n", raw->name);
				continue;
			}

			char picname[16];
			memcpy(picname, pnames + 8*pname_idx, 8);
			picname[8] = 0;

//DebugPrintf("-- %d patch [%s]\n", j, picname);
			Lump_c *lump = W_FindPatchLump(picname);

			if (! lump ||
				! LoadPicture(*img, lump, picname, xofs, yofs, 0, 0))
			{
				LogPrintf("texture '%.8s': patch '%.8s' not found.\n",
						raw->name, picname);
			}
		}

		// store the new texture
		char namebuf[16];
		memcpy(namebuf, raw->name, 8);
		namebuf[8] = 0;

		std::string t_str(namebuf);

		// FIXME: free any existing one with same name

		textures[t_str] = img;
	}

	W_FreeLumpData(&tex_data);
}


static void LoadTexture_SinglePatch(const char *name, Lump_c *lump)
{
	Img *img = new Img();

	if (! LoadPicture(*img, lump, name, 0, 0))
	{
		delete img;
		return;
	}

	std::string t_str(name);

	// FIXME: free any existing one with same name

	textures[t_str] = img;
}


void W_LoadTextures()
{
	W_ClearTextures();

	for (int i = 0 ; i < (int)master_dir.size() ; i++)
	{
		LogPrintf("Loading Textures from WAD #%d\n", i+1);

		Lump_c *pnames   = master_dir[i]->FindLump("PNAMES");
		Lump_c *texture1 = master_dir[i]->FindLump("TEXTURE1");
		Lump_c *texture2 = master_dir[i]->FindLump("TEXTURE2");

		// Note that we _require_ the PNAMES lump to exist along
		// with the TEXTURE1/2 lump which uses it.  Probably a
		// few wads exist which lack the PNAMES lump (relying on
		// the one in the IWAD), however this practice is too
		// error-prone (using the wrong IWAD will break it),
		// so I think supporting it is a bad idea.  -- AJA

		if (!pnames)
			continue;
		
		byte *pname_data;
		int pname_size = W_LoadLumpData(pnames, &pname_data);

		if (texture1)
			LoadTextureLump(texture1, pname_data, pname_size);

		if (texture2)
			LoadTextureLump(texture2, pname_data, pname_size);
	}
}


Img * W_GetTexture(const char *name)
{
	if (name[0] == 0 || name[0] == '-')
		return 0;

	std::string t_str = name;

	std::map<std::string, Img *>::iterator P = textures.find(t_str);

	if (P != textures.end())
		return P->second;

	return NULL;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
