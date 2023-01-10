//------------------------------------------------------------------------
//  WAD PIC LOADER
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

#ifndef __EUREKA_W_LOADPIC_H__
#define __EUREKA_W_LOADPIC_H__

#include "im_img.h"
#include "w_wad.h"
#include "tl/optional.hpp"


// Determine the image format of the given wad lump.
//
// Return values are:
//    'p'  : PNG format
//    't'  : TGA (Targa) format
//    'd'  : Doom patch
//
//    'j'  : JPEG
//    'g'  : GIF
//    's'  : DDS
//
//    NUL  : unrecognized
//
enum class ImageFormat
{
	unrecognized,

	png,
	tga,
	doom,

	jpeg,
	gif,
	dds,
};
ImageFormat W_DetectImageFormat(Lump_c *lump);

tl::optional<Img_c> LoadImage_JPEG(Lump_c *lump, const SString &name);
tl::optional<Img_c> LoadImage_PNG(Lump_c *lump, const SString &name);
tl::optional<Img_c> LoadImage_TGA(Lump_c *lump, const SString &name);
bool LoadPicture(const Palette &pal, const ConfigData &config, Img_c &dest, Lump_c *lump, const SString &pic_name, int pic_x_offset, int pic_y_offset, int *pic_width = nullptr, int *pic_height = nullptr);

#endif  /* __EUREKA_W_LOADPIC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
