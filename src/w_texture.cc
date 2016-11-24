//------------------------------------------------------------------------
//  TEXTURE LOADING
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

#include "lib_tga.h"

#include "m_game.h"      /* yg_picture_format */
#include "levels.h"
#include "w_loadpic.h"
#include "w_rawdef.h"
#include "w_texture.h"


std::map<std::string, Img_c *> textures;

// textures which can cause the Medusa Effect in vanilla/chocolate DOOM
static std::map<std::string, int> medusa_textures;


static void DeleteTex(const std::map<std::string, Img_c *>::value_type& P)
{
	delete P.second;
}

static void W_ClearTextures()
{
	std::for_each(textures.begin(), textures.end(), DeleteTex);

	textures.clear();

	medusa_textures.clear();
}


static void W_AddTexture(const char *name, Img_c *img, bool is_medusa)
{
	// free any existing one with the same name

	std::string tex_str = name;

	std::map<std::string, Img_c *>::iterator P = textures.find(tex_str);

	if (P != textures.end())
	{
		delete P->second;

		P->second = img;
	}
	else
	{
		textures[tex_str] = img;
	}

	medusa_textures[tex_str] = is_medusa ? 1 : 0;
}


static void LoadTextures_Strife(Lump_c *lump, byte *pnames, int pname_size,
                                bool skip_first)
{
	// skip size word at front of PNAMES
	pnames += 4;

	pname_size /= 8;

	// load TEXTUREx data into memory for easier processing
	byte *tex_data;
	int tex_length = W_LoadLumpData(lump, &tex_data);

	// shut up, compiler
	(void) tex_length;

	// at the front of the TEXTUREx lump are some 4-byte integers
	s32_t *tex_data_s32 = (s32_t *)tex_data;

	int num_tex = LE_S32(tex_data_s32[0]);

	for (int n = skip_first ? 1 : 0 ; n < num_tex ; n++)
	{
		int offset = LE_S32(tex_data_s32[1+n]);

		// FIXME: validate offset

		const raw_strife_texture_t *raw = (const raw_strife_texture_t *)(tex_data + offset);

		// create the new image
		int width  = LE_U16(raw->width);
		int height = LE_U16(raw->height);

		DebugPrintf("Texture [%.8s] : %dx%d\n", raw->name, width, height);

		if (width == 0 || height == 0)
			FatalError("W_InitTextures: Texture '%.8s' has zero size\n", raw->name);

		Img_c *img = new Img_c(width, height, false);
		bool is_medusa = false;

		// apply all the patches
		int num_patches = LE_S16(raw->patch_count);
		if (! num_patches)
			FatalError("W_InitTextures: Texture '%.8s' has no patches\n", raw->name);

		const raw_strife_patchdef_t *patdef = (const raw_strife_patchdef_t *) & raw->patches[0];

		if (num_patches >= 2)
			is_medusa = true;

		for (int j = 0 ; j < num_patches ; j++, patdef++)
		{
			int xofs = LE_S16(patdef->x_origin);
			int yofs = LE_S16(patdef->y_origin);
			int pname_idx = LE_U16(patdef->pname);

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

		W_AddTexture(namebuf, img, is_medusa);
	}

	W_FreeLumpData(&tex_data);
}


static void LoadTexturesLump(Lump_c *lump, byte *pnames, int pname_size,
                             bool skip_first)
{
	// FIXME : do this better [ though hard to autodetect based on lump contents ]
	if (strcmp(Game_name, "strife1") == 0)
	{
		LoadTextures_Strife(lump, pnames, pname_size, skip_first);
		return;
	}

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

	for (int n = skip_first ? 1 : 0 ; n < num_tex ; n++)
	{
		int offset = LE_S32(tex_data_s32[1+n]);

		// FIXME: validate offset

		const raw_texture_t *raw = (const raw_texture_t *)(tex_data + offset);

		// create the new image
		int width  = LE_U16(raw->width);
		int height = LE_U16(raw->height);

		DebugPrintf("Texture [%.8s] : %dx%d\n", raw->name, width, height);

		if (width == 0 || height == 0)
			FatalError("W_InitTextures: Texture '%.8s' has zero size\n", raw->name);

		Img_c *img = new Img_c(width, height, false);
		bool is_medusa = false;

		// apply all the patches
		int num_patches = LE_S16(raw->patch_count);
		if (! num_patches)
			FatalError("W_InitTextures: Texture '%.8s' has no patches\n", raw->name);

		const raw_patchdef_t *patdef = (const raw_patchdef_t *) & raw->patches[0];

		// andrewj: this is not strictly correct, the Medusa Effect is only
		//          triggered when multiple patches occupy a single column of
		//          the texture.  But checking for that is a major pain since
		//          we don't know the width of each patch here....
		if (num_patches >= 2)
			is_medusa = true;

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

		W_AddTexture(namebuf, img, is_medusa);
	}

	W_FreeLumpData(&tex_data);
}


static void LoadTexture_SinglePatch(const char *name, Lump_c *lump)
{
	Img_c *img = new Img_c();

	if (! LoadPicture(*img, lump, name, 0, 0))
	{
		delete img;

		return;
	}

	W_AddTexture(name, img, false /* is_medusa */);
}


static void LoadTexture_PNG(const char *name, Lump_c *lump, char img_fmt)
{
	// load the raw data
	byte *tex_data;
	int tex_length = W_LoadLumpData(lump, &tex_data);

	// pass it to FLTK for decoding
	Fl_PNG_Image fltk_img(NULL, tex_data, tex_length);

	W_FreeLumpData(&tex_data);

	if (fltk_img.w() <= 0)
	{
		// failed to decode
		LogPrintf("Failed to decode PNG image in '%s' lump.\n", name);
		return;
	}

	// convert it
	Img_c *img = IM_ConvertRGBImage(&fltk_img);

	W_AddTexture(name, img, false /* is_medusa */);
}


static void LoadTexture_JPEG(const char *name, Lump_c *lump, char img_fmt)
{
	// load the raw data
	byte *tex_data;
	int tex_length = W_LoadLumpData(lump, &tex_data);

	(void) tex_length;

	// pass it to FLTK for decoding
	Fl_JPEG_Image fltk_img(NULL, tex_data);

	W_FreeLumpData(&tex_data);

	if (fltk_img.w() <= 0)
	{
		// failed to decode
		LogPrintf("Failed to decode JPEG image in '%s' lump.\n", name);
		return;
	}

	// convert it
	Img_c *img = IM_ConvertRGBImage(&fltk_img);

	W_AddTexture(name, img, false /* is_medusa */);
}


static void LoadTexture_TGA(const char *name, Lump_c *lump, char img_fmt)
{
	// load the raw data
	byte *tex_data;
	int tex_length = W_LoadLumpData(lump, &tex_data);

	// decode it
	int width;
	int height;

	rgba_color_t * rgba = TGA_DecodeImage(tex_data, (size_t)tex_length, width, height);

	W_FreeLumpData(&tex_data);

	if (! rgba)
	{
		// failed to decode
		LogPrintf("Failed to decode TGA image in '%s' lump.\n", name);
		return;
	}

	// convert it
	Img_c *img = IM_ConvertTGAImage(rgba, width, height);

	W_AddTexture(name, img, false /* is_medusa */);

	TGA_FreeImage(rgba);
}


void W_LoadTextures_TX_START(Wad_file *wf)
{
	for (int k = 0 ; k < (int)wf->tx_tex.size() ; k++)
	{
		Lump_c *lump = wf->GetLump(wf->tx_tex[k]);

		char img_fmt = W_DetectImageFormat(lump);

		DebugPrintf("TX_TEX %d : '%s' type=%c\n", k, lump->Name(), img_fmt ? img_fmt : '?');

		switch (img_fmt)
		{
			case 'd': /* Doom patch */
				LoadTexture_SinglePatch(lump->Name(), lump);
				break;

			case 'p': /* PNG */
				LoadTexture_PNG(lump->Name(), lump, img_fmt);
				break;

			case 't': /* TGA */
				LoadTexture_TGA(lump->Name(), lump, img_fmt);
				break;

			case 'j': /* JPEG */
				LoadTexture_JPEG(lump->Name(), lump, img_fmt);
				break;

			case 0:
				LogPrintf("Unknown texture format in '%s' lump\n", lump->Name());
				break;

			default:
				LogPrintf("Unsupported texture format in '%s' lump\n", lump->Name());
				break;
		}
	}
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

		if (pnames)
		{
			byte *pname_data;
			int pname_size = W_LoadLumpData(pnames, &pname_data);

			if (texture1)
				LoadTexturesLump(texture1, pname_data, pname_size, true);

			if (texture2)
				LoadTexturesLump(texture2, pname_data, pname_size, false);
		}

		if (game_info.tx_start)
		{
			W_LoadTextures_TX_START(master_dir[i]);
		}
	}
}


Img_c * W_GetTexture(const char *name, bool try_uppercase)
{
	if (is_missing_tex(name))
		return NULL;

	std::string t_str = name;

	std::map<std::string, Img_c *>::iterator P = textures.find(t_str);

	if (P != textures.end())
		return P->second;

	if (try_uppercase)
	{
		return W_GetTexture(NormalizeTex(name), false);
	}

	return NULL;
}


bool W_TextureIsKnown(const char *name)
{
	// FIXME : is_missing_tex
	if (name[0] == '-') return true;

	// special textures
	if (name[0] == '#') return true;

	std::string t_str = name;

	std::map<std::string, Img_c *>::iterator P = textures.find(t_str);

	return (P != textures.end());
}


bool W_TextureCausesMedusa(const char *name)
{
	std::string t_str = name;

	std::map<std::string, int>::iterator P = medusa_textures.find(t_str);

	return (P != medusa_textures.end() && P->second > 0);
}


const char *NormalizeTex(const char *name)
{
	static char buffer[WAD_TEX_NAME+1];

	memset(buffer, 0, sizeof(buffer));

	strncpy(buffer, name, WAD_TEX_NAME);

	for (int i = 0 ; i < WAD_TEX_NAME ; i++)
		buffer[i] = toupper(buffer[i]);

	return buffer;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
