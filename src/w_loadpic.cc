//------------------------------------------------------------------------
//  WAD PIC LOADER
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2020 Andrew Apted
//  Copyright (C) 1997-2003 Andr� Majorel et al
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
//  in the public domain in 1994 by Rapha�l Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "Errors.h"
#include "Instance.h"

#include "main.h"

#include "im_color.h"  /* trans_replace */
#include "im_img.h"

#include "lib_tga.h"
#include "m_game.h"

#include "w_loadpic.h"
#include "w_rawdef.h"
#include "w_wad.h"


// posts are runs of non masked source pixels
struct post_t
{
	// offset down from top.  P_SENTINEL terminates the list.
	byte topdelta;

    // length data bytes follows
	byte length;

	/* byte pixels[length+2] */
};

#define P_SENTINEL  0xFF


static void DrawColumn(const Palette &pal, const ConfigData &config,Img_c& img, const post_t *column, int x, int y)
{
	SYS_ASSERT(column);

	int W = img.width();
	int H = img.height();

	// clip horizontally
	if (x < 0 || x >= W)
		return;

	while (column->topdelta != P_SENTINEL)
	{
		int top = y + (int) column->topdelta;
		int count = column->length;

		byte *src = (byte *) column + 3;

		img_pixel_t *dest = img.wbuf() + x;

		if (top < 0)
		{
			// The original DOOM did not honor negative y-offsets for
			// patches but some ports like ZDoom do.
			if (config.features.neg_patch_offsets)
				src -= top;
            count += top;

			top = 0;
		}

		if (top + count > H)
			count = H - top;

		// copy the pixels, remapping any TRANS_PIXEL values
		for (; count > 0; count--, top++)
		{
			byte pix = *src++;

			if (pix == TRANS_PIXEL)
				pix = static_cast<byte>(pal.getTransReplace());

			dest[top * W] = pix;
		}

		column = (const post_t *) ((const byte *) column + column->length + 4);
	}
}


tl::optional<Img_c> LoadImage_PNG(const Lump_c &lump, const SString &name)
{
	// load the raw data
	const std::vector<byte> &tex_data = lump.getData();

	// pass it to FLTK for decoding
	Fl_PNG_Image fltk_img(NULL, tex_data.data(), (int)tex_data.size());

	if (fltk_img.w() <= 0)
	{
		// failed to decode
		gLog.printf("Failed to decode PNG image in '%s' lump.\n", name.c_str());
		return {};
	}

	// convert it

	return IM_ConvertRGBImage(fltk_img);
}


tl::optional<Img_c> LoadImage_JPEG(const Lump_c &lump, const SString &name)
{
	// load the raw data
	const std::vector<byte> &tex_data = lump.getData();

	// pass it to FLTK for decoding
	Fl_JPEG_Image fltk_img(NULL, tex_data.data());

	if (fltk_img.w() <= 0)
	{
		// failed to decode
		gLog.printf("Failed to decode JPEG image in '%s' lump.\n", name.c_str());
		return {};
	}

	// convert it

	return IM_ConvertRGBImage(fltk_img);
}


tl::optional<Img_c> LoadImage_TGA(const Lump_c &lump, const SString &name)
{
	// load the raw data
	const std::vector<byte> &tex_data = lump.getData();

	// decode it
	int width;
	int height;

	rgba_color_t * rgba = TGA_DecodeImage(tex_data.data(), tex_data.size(),  width, height);

	if (! rgba)
	{
		// failed to decode
		gLog.printf("Failed to decode TGA image in '%s' lump.\n", name.c_str());
		return {};
	}

	// convert it
	Img_c img = IM_ConvertTGAImage(rgba, width, height);

	TGA_FreeImage(rgba);

	return img;
}


static bool ComposePicture(Img_c& dest, const tl::optional<Img_c> &sub,
	int pic_x_offset, int pic_y_offset,
	int *pic_width, int *pic_height)
{
	if (!sub)
		return false;

	int width  = sub->width();
	int height = sub->height();

	if (pic_width)  *pic_width  = width;
	if (pic_height) *pic_height = height;

	if (dest.is_null())
	{
		// our new image will be completely transparent
		dest.resize (width, height);
	}

	dest.compose(*sub, pic_x_offset, pic_y_offset);

	return true;
}


//
//  LoadPicture - read a picture from a wad file into an Img_c object
//
//  If img->is_null() is false, LoadPicture() does not allocate the
//  buffer itself. The buffer and the picture don't have to have the
//  same dimensions. Thanks to this, it can also be used to compose
//  textures : you allocate a single buffer for the whole texture
//  and then you call LoadPicture() on it once for each patch.
//  LoadPicture() takes care of all the necessary clipping.
//
//  If img->is_null() is true, LoadPicture() sets the size of img
//  to match that of the picture. This is useful in display_pic().
//
//  Return true on success, false on failure.
//
bool LoadPicture(const Palette &pal, const ConfigData &config, Img_c& dest,      // image to load picture into
	const Lump_c &lump,
	const SString &pic_name,   // picture name (for messages)
	int pic_x_offset,    // coordinates of top left corner of picture
	int pic_y_offset,    // relative to top left corner of buffer
	int *pic_width,    // To return the size of the picture
	int *pic_height)   // (can be NULL)
{
	ImageFormat img_fmt = W_DetectImageFormat(lump);
	tl::optional<Img_c> sub;
	
	bool result;
	int localPicWidth = 0, localPicHeight = 0;

	switch (img_fmt)
	{
	case ImageFormat::doom:
		// use the code below to load/compose the DOOM format
		break;

	case ImageFormat::png:
		sub = LoadImage_PNG(lump, pic_name);
		result = ComposePicture(dest, sub, pic_x_offset, pic_y_offset, &localPicWidth, &localPicHeight);
		if (pic_width)
			*pic_width = localPicWidth;
		if (pic_height)
			*pic_height = localPicHeight;
		dest.setSpriteOffset(localPicWidth / 2, localPicHeight);
		return result;

	case ImageFormat::jpeg:
		sub = LoadImage_JPEG(lump, pic_name);
		result = ComposePicture(dest, sub, pic_x_offset, pic_y_offset, &localPicWidth, &localPicHeight);
		if (pic_width)
			*pic_width = localPicWidth;
		if (pic_height)
			*pic_height = localPicHeight;
		dest.setSpriteOffset(localPicWidth / 2, localPicHeight);
		return result;

	case ImageFormat::tga:
		sub = LoadImage_TGA(lump, pic_name);
		result = ComposePicture(dest, sub, pic_x_offset, pic_y_offset, &localPicWidth, &localPicHeight);
		if (pic_width)
			*pic_width = localPicWidth;
		if (pic_height)
			*pic_height = localPicHeight;
		dest.setSpriteOffset(localPicWidth / 2, localPicHeight);
		return result;

	case ImageFormat::unrecognized:
		gLog.printf("Unknown image format in '%s' lump\n", pic_name.c_str());
		return false;

	default:
		gLog.printf("Unsupported image format in '%s' lump\n", pic_name.c_str());
		return false;
	}

	/* DOOM format */

	const std::vector<byte> &raw_data = lump.getData();

	auto pat = reinterpret_cast<const patch_t *>(raw_data.data());

	int width    = LE_S16(pat->width);
	int height   = LE_S16(pat->height);
	int offset_x = LE_S16(pat->leftoffset);
	int offset_y = LE_S16(pat->topoffset);

	dest.setSpriteOffset(offset_x, offset_y);

	// FIXME: validate values (in case we got flat data or so)

	if (pic_width)  *pic_width  = width;
	if (pic_height) *pic_height = height;

	if (dest.is_null())
	{
		// our new image will be completely transparent
		dest.resize (width, height);
	}

	for (int x = 0 ; x < width ; x++)
	{
		int offset = LE_S32(pat->columnofs[x]);

		if (offset < 0 || offset >= lump.Length())
		{
			gLog.printf("WARNING: bad image offset 0x%08x in patch [%s]\n",
			          offset, pic_name.c_str());
			return false;
		}

		const post_t *column = (const post_t *) ((const byte *)pat + offset);

		DrawColumn(pal, config, dest, column, pic_x_offset + x, pic_y_offset);
	}

	return true;
}


ImageFormat W_DetectImageFormat(const Lump_c &lump)
{
	int length = lump.Length();

	if (length < 20)
		return ImageFormat::unrecognized;
	
	const byte *header = lump.getData().data();

	// PNG is clearly marked in the header, so check it first.

	if (header[0] == 0x89 && header[1] == 'P' &&
		header[2] == 'N'  && header[3] == 'G' &&
		header[4] == 0x0D && header[5] == 0x0A)
	{
		return ImageFormat::png; /* PNG */
	}

	// check some other common image formats....

	if (header[0] == 0xFF && header[1] == 0xD8 &&
		header[2] == 0xFF && header[3] >= 0xE0 &&
		((header[6] == 'J' && header[7] == 'F') ||
		 (header[6] == 'E' && header[7] == 'x')))
	{
		return ImageFormat::jpeg; /* JPEG */
	}

	if (header[0] == 'G' && header[1] == 'I' &&
		header[2] == 'F' && header[3] == '8' &&
		header[4] >= '7' && header[4] <= '9' &&
		header[5] == 'a')
	{
		return ImageFormat::gif; /* GIF */
	}

	if (header[0] == 'D' && header[1] == 'D'  &&
		header[2] == 'S' && header[3] == 0x20 &&
		header[4] == 124 && header[5] == 0    &&
		header[6] == 0)
	{
		return ImageFormat::dds; /* DDS (DirectDraw Surface) */
	}

	// TGA (Targa) is not clearly marked, but better than Doom patches,
	// so check it next.

	int  width = (int)header[12] + (int)(header[13] << 8);
	int height = (int)header[14] + (int)(header[15] << 8);

	byte cmap_type = header[1];
	byte img_type  = header[2];
	byte depth     = header[16];

	if (width  > 0 && width  <= 2048 &&
		height > 0 && height <= 2048 &&
		(cmap_type == 0 || cmap_type == 1) &&
		((img_type | 8) >= 8 && (img_type | 8) <= 11) &&
		(depth == 8 || depth == 15 || depth == 16 || depth == 24 || depth == 32))
	{
		return ImageFormat::tga; /* TGA */
	}

	// check for raw patches last

	 width = (int)header[0] + (int)(header[1] << 8);
	height = (int)header[2] + (int)(header[3] << 8);

	int ofs_x = (int)header[4] + (int)((signed char)header[5] * 256);
	int ofs_y = (int)header[6] + (int)((signed char)header[7] * 256);

	if (width  > 0 && width  <= 2048 && abs(ofs_x) <= 2048 &&
		height > 0 && height <=  512 && abs(ofs_y) <=  512 &&
		length > width * 4 /* columnofs */)
	{
		return ImageFormat::doom; /* Doom patch */
	}

	return ImageFormat::unrecognized;	// unknown!
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
