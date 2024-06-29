//------------------------------------------------------------------------
//  TEXTURES / FLATS / SPRITES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2020 Andrew Apted
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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include <map>
#include <algorithm>
#include <string>

#include "m_game.h"      /* yg_picture_format */
#include "w_loadpic.h"
#include "w_rawdef.h"
#include "w_texture.h"


//----------------------------------------------------------------------
//    TEXTURE HANDLING
//----------------------------------------------------------------------

void ImageSet::W_ClearTextures()
{
	textures.clear();

	medusa_textures.clear();
}


void ImageSet::W_AddTexture(const SString &name, Img_c &&img, bool is_medusa)
{
	// free any existing one with the same name

	textures[name] = std::move(img);
	medusa_textures[name] = is_medusa ? 1 : 0;
}


static bool CheckTexturesAreStrife(const byte *tex_data, int tex_length, int num_tex,
								   bool skip_first)
{
	// we follow the ZDoom logic here: check ALL the texture entries
	// assuming DOOM format, and if any have a patch_count of zero or
	// the last two bytes of columndir are non-zero then assume Strife.

	const int32_t *tex_data_s32 = (const int32_t *)tex_data;

	for (int n = skip_first ? 1 : 0 ; n < num_tex ; n++)
	{
		int offset = LE_S32(tex_data_s32[1 + n]);

		// ignore invalid offsets here  [ caught later ]
		if (offset < 4 * num_tex || offset >= tex_length)
			continue;

		const raw_texture_t *raw = (const raw_texture_t *)(tex_data + offset);

		if (LE_S16(raw->patch_count) <= 0)
			return true;

		if (raw->column_dir[1] != 0)
			return true;
	}

	return false;
}


static void LoadTextureEntry_Strife(WadData &wad, const ConfigData &config, const byte *tex_data, int tex_length, int offset,
									const byte *pnames, int pname_size, bool skip_first)
{
	const raw_strife_texture_t *raw = (const raw_strife_texture_t *)(tex_data + offset);

	// create the new image
	int width  = LE_U16(raw->width);
	int height = LE_U16(raw->height);

	gLog.debugPrintf("Texture [%.8s] : %dx%d\n", raw->name, width, height);

	if (width == 0 || height == 0)
		ThrowException("W_LoadTextures: Texture '%.8s' has zero size\n", raw->name);

	Img_c img(width, height, false);
	bool is_medusa = false;

	// apply all the patches
	int num_patches = LE_S16(raw->patch_count);

	if (! num_patches)
		ThrowException("W_LoadTextures: Texture '%.8s' has no patches\n", raw->name);

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
			gLog.printf("Invalid pname in texture '%.8s'\n", raw->name);
			continue;
		}

		char picname[16];
		memcpy(picname, pnames + 8*pname_idx, 8);
		picname[8] = 0;

		const Lump_c *lump = wad.master.findGlobalLump(picname);

		if (! lump ||
			! LoadPicture(wad.palette, config, img, *lump, picname, xofs, yofs))
		{
			gLog.printf("texture '%.8s': patch '%.8s' not found.\n", raw->name, picname);
		}
	}

	// store the new texture
	char namebuf[16];
	memcpy(namebuf, raw->name, 8);
	namebuf[8] = 0;

	wad.images.W_AddTexture(namebuf, std::move(img), is_medusa);
}


static void LoadTextureEntry_DOOM(WadData &wad, const ConfigData &config, const byte *tex_data, int tex_length, int offset,
									const byte *pnames, int pname_size, bool skip_first)
{
	const raw_texture_t *raw = (const raw_texture_t *)(tex_data + offset);

	// create the new image
	int width  = LE_U16(raw->width);
	int height = LE_U16(raw->height);

	gLog.debugPrintf("Texture [%.8s] : %dx%d\n", raw->name, width, height);

	if (width == 0 || height == 0)
		ThrowException("W_LoadTextures: Texture '%.8s' has zero size\n", raw->name);

	Img_c img(width, height, false);
	bool is_medusa = false;

	// apply all the patches
	int num_patches = LE_S16(raw->patch_count);

	if (! num_patches)
		ThrowException("W_LoadTextures: Texture '%.8s' has no patches\n", raw->name);

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

		if (pname_idx >= pname_size)
		{
			gLog.printf("Invalid pname in texture '%.8s'\n", raw->name);
			continue;
		}

		char picname[16];
		memcpy(picname, pnames + 8*pname_idx, 8);
		picname[8] = 0;

//gLog.debugPrintf("-- %d patch [%s]\n", j, picname);
		const Lump_c *lump = wad.master.findGlobalLump(picname);

		if (! lump ||
			! LoadPicture(wad.palette, config, img, *lump, picname, xofs, yofs))
		{
			gLog.printf("texture '%.8s': patch '%.8s' not found.\n", raw->name, picname);
		}
	}

	// store the new texture
	char namebuf[16];
	memcpy(namebuf, raw->name, 8);
	namebuf[8] = 0;

	wad.images.W_AddTexture(namebuf, std::move(img), is_medusa);
}


static void LoadTexturesLump(WadData &wad, const ConfigData &config, const Lump_c &lump, const byte *pnames, int pname_size,
                             bool skip_first)
{
	// TODO : verify size word at front of PNAMES ??

	// skip size word at front of PNAMES
	pnames += 4;

	pname_size /= 8;

	// load TEXTUREx data into memory for easier processing
	const std::vector<byte> &tex_data = lump.getData();

	// at the front of the TEXTUREx lump are some 4-byte integers
	int32_t *tex_data_s32 = (int32_t *)tex_data.data();

	int num_tex = LE_S32(tex_data_s32[0]);

	// it seems having a count of zero is valid
	if (num_tex == 0)
	{
		return;
	}

	if (num_tex < 0 || num_tex > (1<<20))
		ThrowException("W_LoadTextures: TEXTURE1/2 lump is corrupt, bad count.\n");

	bool is_strife = CheckTexturesAreStrife(tex_data.data(), (int)tex_data.size(), num_tex, skip_first);

	// Note: we skip the first entry (e.g. AASHITTY) which is not really
    //       usable (in the DOOM engine the #0 texture means "do not draw").

	for (int n = skip_first ? 1 : 0 ; n < num_tex ; n++)
	{
		int offset = LE_S32(tex_data_s32[1 + n]);

		if (offset < 4 * num_tex || offset >= (int)tex_data.size())
			ThrowException("W_LoadTextures: TEXTURE1/2 lump is corrupt, bad offset.\n");

		if (is_strife)
			LoadTextureEntry_Strife(wad, config, tex_data.data(), (int)tex_data.size(), offset, pnames, pname_size, skip_first);
		else
			LoadTextureEntry_DOOM(wad, config, tex_data.data(), (int)tex_data.size(), offset, pnames, pname_size, skip_first);
	}
}


static void W_LoadTextures_TX_START(WadData &wad, const ConfigData &config, const Wad_file *wf)
{
	for(const LumpRef &lumpRef : wf->getDir())
	{
		if(lumpRef.ns != WadNamespace::TextureLumps)
			continue;
		const Lump_c *lump = lumpRef.lump.get();

		ImageFormat img_fmt = W_DetectImageFormat(*lump);
		const SString &name = lump->Name();
		tl::optional<Img_c> img;

		switch (img_fmt)
		{
			case ImageFormat::doom: /* Doom patch */
				img = Img_c();
				if (! LoadPicture(wad.palette, config, *img, *lump, name, 0, 0))
				{
					img.reset();
				}
				break;

			case ImageFormat::png: /* PNG */
				img = LoadImage_PNG(*lump, name);
				break;

			case ImageFormat::tga: /* TGA */
				img = LoadImage_TGA(*lump, name);
				break;

			case ImageFormat::jpeg: /* JPEG */
				img = LoadImage_JPEG(*lump, name);
				break;

			case ImageFormat::unrecognized:
				gLog.printf("Unknown texture format in '%s' lump\n", name.c_str());
				break;

			default:
				gLog.printf("Unsupported texture format in '%s' lump\n", lump->Name().c_str());
				break;
		}

		// if we successfully loaded the texture, add it
		if (img)
		{
			wad.images.W_AddTexture(name, std::move(*img), false /* is_medusa */);
		}
	}
}


void WadData::W_LoadTextures(const ConfigData &config)
{
	images.W_ClearTextures();

	std::vector<std::shared_ptr<Wad_file>> wads = master.getAll();
	for (int i = 0 ; i < (int)wads.size() ; i++)
	{
		gLog.printf("Loading Textures from WAD #%d\n", i+1);

		const Lump_c *pnames   = wads[i]->FindLumpInNamespace("PNAMES", WadNamespace::Global);
		const Lump_c *texture1 = wads[i]->FindLumpInNamespace("TEXTURE1", WadNamespace::Global);
		const Lump_c *texture2 = wads[i]->FindLumpInNamespace("TEXTURE2", WadNamespace::Global);

		// Note that we _require_ the PNAMES lump to exist along
		// with the TEXTURE1/2 lump which uses it.  Probably a
		// few wads exist which lack the PNAMES lump (relying on
		// the one in the IWAD), however this practice is too
		// error-prone (using the wrong IWAD will break it),
		// so I think supporting it is a bad idea.  -- AJA

		if (pnames)
		{
			const std::vector<byte> &pname_data = pnames->getData();

			if (texture1)
				LoadTexturesLump(*this, config, *texture1, pname_data.data(), (int)pname_data.size(), true);

			if (texture2)
				LoadTexturesLump(*this, config, *texture2, pname_data.data(), (int)pname_data.size(), false);
		}

		if (config.features.tx_start)
		{
			W_LoadTextures_TX_START(*this, config, wads[i].get());
		}
	}
}


const Img_c * ImageSet::getTexture(const ConfigData &config, const SString &name, bool try_uppercase) const
{
	if (is_null_tex(name))
		return NULL;

	if (name.empty())
		return NULL;

	SString t_str = name;
	std::map<SString, Img_c>::const_iterator P = textures.find(t_str);

	if (P != textures.end())
		return &P->second;

	if (try_uppercase)
	{
		return getTexture(config, NormalizeTex(name), false);
	}

	if (config.features.mix_textures_flats)
	{
		std::map<SString, Img_c>::const_iterator P = flats.find(t_str);

		if (P != flats.end())
			return &P->second;
	}

	return NULL;
}


int ImageSet::W_GetTextureHeight(const ConfigData &config, const SString &name) const
{
	const Img_c *img = getTexture(config, name);

	if (! img)
		return 128;

	return img->height();
}

// accepts "-", "#xxxx" or an existing texture name
bool ImageSet::W_TextureIsKnown(const ConfigData &config, const SString &name) const
{
	if (is_null_tex(name) || is_special_tex(name))
		return true;

	if (name.empty())
		return false;

	std::map<SString, Img_c>::const_iterator P = textures.find(name);

	if (P != textures.end())
		return true;

	if (config.features.mix_textures_flats)
	{
		std::map<SString, Img_c>::const_iterator P = flats.find(name);

		if (P != flats.end())
			return true;
	}

	return false;
}


bool ImageSet::W_TextureCausesMedusa(const SString &name) const
{
	std::map<SString, int>::const_iterator P = medusa_textures.find(name);

	return (P != medusa_textures.end() && P->second > 0);
}


SString NormalizeTex(const SString &name)
{
	if (name[0] == 0)
		return "-";

	SString buffer;

	for (size_t i = 0 ; i < WAD_TEX_NAME && name[i]; i++)
	{
		buffer.push_back(name[i]);

		// remove double quotes
		if (buffer[i] == '"')
			buffer[i] = '_';
		else
			buffer[i] = static_cast<char>(toupper(buffer[i]));
	}

	return buffer;
}


//----------------------------------------------------------------------
//    FLAT HANDLING
//----------------------------------------------------------------------

void ImageSet::W_ClearFlats()
{
	flats.clear();
}


void ImageSet::W_AddFlat(const SString &name, Img_c &&img)
{
	// find any existing one with same name, and free it
	flats[name] = std::move(img);
}


static Img_c LoadFlatImage(const WadData &wad, const SString &name, const Lump_c *lump)
{
	// TODO: check size == 64*64

	Img_c img(64, 64, false);

	int size = 64 * 64;

	byte *raw = new byte[size];

	LumpInputStream stream(*lump);
	if (! stream.read(raw, size))
	{
		gLog.printf("%s: flat '%s' is too small, should be at least %d.\n",
					__func__, name.c_str(), size);
		int smallsize = lump->Length();
		if(smallsize > 0)
			memset(raw + smallsize, raw[smallsize - 1], size - smallsize);
		else
			memset(raw, 0, size);
	}

	for (int i = 0 ; i < size ; i++)
	{
		img_pixel_t pix = raw[i];

		if (pix == TRANS_PIXEL)
			pix = static_cast<img_pixel_t>(wad.palette.getTransReplace());

		img.wbuf() [i] = pix;
	}

	delete[] raw;

	return img;
}


void WadData::W_LoadFlats()
{
	images.W_ClearFlats();

	std::vector<std::shared_ptr<Wad_file>> wads = master.getAll();
	for (int i = 0 ; i < (int)wads.size() ; i++)
	{
		gLog.printf("Loading Flats from WAD #%d\n", i+1);

		const Wad_file *wf = wads[i].get();

		for(const LumpRef &lumpRef : wf->getDir())
		{
			if(lumpRef.ns != WadNamespace::Flats)
				continue;
			const Lump_c *lump = lumpRef.lump.get();

			images.W_AddFlat(lump->Name(), LoadFlatImage(*this, lump->Name(), lump));
		}
	}
}


const Img_c * ImageSet::W_GetFlat(const ConfigData &config, const SString &name, bool try_uppercase) const noexcept
{
	std::map<SString, Img_c>::const_iterator P = flats.find(name);

	if (P != flats.end())
		return &P->second;

	if (config.features.mix_textures_flats)
	{
		std::map<SString, Img_c>::const_iterator P = textures.find(name);

		if (P != textures.end())
			return &P->second;
	}

	if (try_uppercase)
	{
		return W_GetFlat(config, NormalizeTex(name), false);
	}

	return NULL;
}


bool ImageSet::W_FlatIsKnown(const ConfigData &config, const SString &name) const
{
	// sectors do not support "-" (but our code can make it)
	if (is_null_tex(name))
		return false;

	if (name.empty())
		return false;

	std::map<SString, Img_c>::const_iterator P = flats.find(name);

	if (P != flats.end())
		return true;

	if (config.features.mix_textures_flats)
	{
		std::map<SString, Img_c>::const_iterator P = textures.find(name);

		if (P != textures.end())
			return true;
	}

	return false;
}


//----------------------------------------------------------------------
//    SPRITE HANDLING
//----------------------------------------------------------------------
void ImageSet::W_ClearSprites()
{
	sprites.clear();
}


// find sprite by prefix
static std::vector<SpriteLumpRef> Sprite_loc_by_root (const MasterDir &master, const ConfigData &config, const SString &name)
{
	// first look for one in the sprite namespace (S_START..S_END),
	// only if that fails do we check the whole wad.

	SString buffer;
	buffer.reserve(16);
	buffer = name;
	std::vector<SpriteLumpRef> spriteset = master.findFirstSpriteLump(buffer);

	if (!spriteset.empty())
		return spriteset;

	// check outside of the sprite namespace...
	const Lump_c *lump = nullptr;

	if (config.features.lax_sprites)
	{
		buffer = name;
		if(buffer.length() == 4)
			buffer += 'A';
		if(buffer.length() == 5)
			buffer += '0';

		lump = master.findGlobalLump(buffer);

		if (! lump)
		{
			if(buffer.length() >= 6)
				buffer[5] = '1';
			lump = master.findGlobalLump(buffer);
		}

		// TODO: verify lump is OK (size etc)
		if (lump)
		{
			gLog.printf("WARNING: using sprite '%s' outside of S_START..S_END\n", name.c_str());
		}
	}

	if (!lump)
	{
		// Still no lump? Try direct lookup
		// TODO: verify lump is OK (size etc)
		lump = master.findGlobalLump(name);
	}

	return lump ? std::vector<SpriteLumpRef>{{lump, false}} : std::vector<SpriteLumpRef>{};
}


const Img_c *WadData::getSprite(const ConfigData &config, int type, const LoadingData &loading, int rotation)
{
	assert(rotation >= 1 && rotation <= 8);
	const std::vector<Img_c> *existing = get(images.sprites, type);
	if(existing)
	{
		assert(existing->size() <= 1 || existing->size() == 8);
		return existing->empty() ? nullptr :
				existing->size() == 1 ? &(*existing)[0] : &(*existing)[rotation - 1];
	}

	// sprite not in the list yet.  Add it.

	const thingtype_t &info = config.getThingType(type);

	std::vector<Img_c> result;

	if (info.desc.startsWith("UNKNOWN"))
	{
		// leave as NULL
	}
	else if (info.sprite.noCaseEqual("_LYT"))
	{
		result = {Img_c::createLightSprite(palette)};
	}
	else if (info.sprite.noCaseEqual("_MSP"))
	{
		result = {Img_c::createMapSpotSprite(palette, 0, 255, 0)};
	}
	else if (info.sprite.noCaseEqual("NULL"))
	{
		result = {Img_c::createMapSpotSprite(palette, 70, 70, 255)};
	}
	else
	{
		std::vector<SpriteLumpRef> spriteset = Sprite_loc_by_root(master, config, info.sprite);
		if (spriteset.empty())
		{
			// for the MBF dog, create our own sprite for it, since
			// it is defined in the Boom definition file and the
			// missing sprite looks ugly in the thing browser.

			if (info.sprite.noCaseEqual("DOGS"))
				result = {Img_c::createDogSprite(palette)};
			else
				gLog.printf("Sprite not found: '%s'\n", info.sprite.c_str());
		}
		else
		{
			if(spriteset.size() == 1)
			{
				result.resize(1);
				result[0] = Img_c();
				assert(spriteset[0].lump);
				if (! LoadPicture(palette, config, result[0], *spriteset[0].lump, info.sprite, 0, 0))
				{
					result.clear();
				}
				else if(spriteset[0].flipped)
					result[0].flipHorizontally();
			}
			else if(spriteset.size() == 8)
			{
				result.resize(8);
				int firstRot = -1;
				for(int i = 0; i < 8; ++i)
				{
					if(!spriteset[i].lump)
						continue;
					
					if (! LoadPicture(palette, config, result[i], *spriteset[i].lump, info.sprite, 0, 0))
					{
						gLog.printf("Failed loading %s rotation %d\n", info.sprite.c_str(), i + 1);
					}
					else
					{
						firstRot = i;
						if(spriteset[i].flipped)
						{
							result[i].flipHorizontally();
						}
					}
				}
				// Now sweep them for missing images
				if(firstRot == -1)	// nothing found
					result.clear();
				else for(Img_c &img : result)
					if(!img.width())
						img = result[firstRot];
			}
		}
	}

	// player color remapping
	// [ FIXME : put colors into game definition file ]
	if (!result.empty())
	{
		if(info.flags & THINGDEF_INVIS)
		{
			for(Img_c &img : result)
				img = img.spectrify(config);
		}
		else if(info.group == 'p')
		{
			tl::optional<Img_c> new_img;
			
			int src1, src2;
			int targ1[4], targ2[4];
			if (M_GetBaseGame(loading.gameName) == "heretic")
			{
				src1 = 225;
				src2 = 240;
				targ1[0] = 114;
				targ2[0] = 129;
				targ1[1] = 145;
				targ2[1] = 160;
				targ1[2] = targ1[3] = 190;
				targ2[2] = targ2[3] = 205;
				
			}
			else
			{
				src1 = 0x70;
				src2 = 0x7f;
				targ1[0] = 0x60;
				targ2[0] = 0x6f;
				targ1[1] = 0x40;
				targ2[1] = 0x4f;
				targ1[2] = 0x20;
				targ2[2] = 0x2f;
				targ1[3] = 0xc4;
				targ2[3] = 0xcf;
			}
			
			for(Img_c &img : result)
			{
				switch (type)
				{
					case 1:
						// no change
						break;
						
					case 2:
						new_img = img.color_remap(src1, src2, targ1[0], targ2[0]);
						break;
						
					case 3:
						new_img = img.color_remap(src1, src2, targ1[1], targ2[1]);
						break;
						
					case 4:
						new_img = img.color_remap(src1, src2, targ1[2], targ2[2]);
						break;
						
						// blue for the extra coop starts
					case 4001:
					case 4002:
					case 4003:
					case 4004:
						new_img = img.color_remap(src1, src2, targ1[3], targ2[3]);
						break;
				}
				
				if (new_img)
				{
					img = std::move(*new_img);
				}
			}
		}
	}

	// note that a NULL image is OK.  Our renderer will just ignore the
	// missing sprite.
	
	images.sprites[type] = result;
	std::vector<Img_c> &sprites = images.sprites[type];
	
	return sprites.empty() ? nullptr : sprites.size() == 8 ? &sprites[rotation - 1] : &sprites[0];
}


//----------------------------------------------------------------------

static void UnloadTex(std::map<SString, Img_c>::value_type& P)
{
	P.second.unload_gl(false);
}

static void UnloadFlat(std::map<SString, Img_c>::value_type& P)
{
	P.second.unload_gl(false);
}

static void UnloadSprite(sprite_map_t::value_type& P)
{
	for(Img_c &img : P.second)
		img.unload_gl(false);
}

void ImageSet::W_UnloadAllTextures()
{
	std::for_each(textures.begin(), textures.end(), UnloadTex);
	std::for_each(flats.begin(),    flats.end(), UnloadFlat);
	std::for_each(sprites.begin(), sprites.end(), UnloadSprite);

	IM_UnloadDummyTextures();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
