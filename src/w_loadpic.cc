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

#include "main.h"

#include "im_color.h"  /* trans_replace */
#include "im_img.h"

#include "w_loadpic.h"
#include "w_rawdef.h"
#include "w_wad.h"


// posts are runs of non masked source pixels
typedef struct
{
	// offset down from top.  P_SENTINEL terminates the list.
	byte topdelta;

    // length data bytes follows
	byte length;

	/* byte pixels[length+2] */
}
post_t;

#define P_SENTINEL  0xFF


static void DrawColumn(Img_c& img, const post_t *column, int x, int y)
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
				pix = trans_replace;

			dest[top * W] = pix;
		}

		column = (const post_t *) ((const byte *) column + column->length + 4);
	}
}


/*
 *  LoadPicture - read a picture from a wad file into an Img_c object
 *
 *  If img->is_null() is false, LoadPicture() does not allocate the
 *  buffer itself. The buffer and the picture don't have to have the
 *  same dimensions. Thanks to this, it can also be used to compose
 *  textures : you allocate a single buffer for the whole texture
 *  and then you call LoadPicture() on it once for each patch.
 *  LoadPicture() takes care of all the necessary clipping.
 *
 *  If img->is_null() is true, LoadPicture() sets the size of img
 *  to match that of the picture. This is useful in display_pic().
 *
 *  Return true on success, false on failure.
 *
 *  If pic_x_offset == INT_MIN, the picture is centred horizontally.
 *  If pic_y_offset == INT_MIN, the picture is centred vertically.
 */
bool LoadPicture(Img_c& img,      // image to load picture into
	Lump_c *lump,
	const char *pic_name,   // Picture name (for messages)
	int pic_x_offset,    // Coordinates of top left corner of picture
	int pic_y_offset,    // relative to top left corner of buffer
	int *pic_width,    // To return the size of the picture
	int *pic_height)   // (can be NULL)
{
	byte *raw_data;
	W_LoadLumpData(lump, &raw_data);

	const patch_t *pat = (patch_t *) raw_data;

	int width    = LE_S16(pat->width);
	int height   = LE_S16(pat->height);
//	int offset_x = LE_S16(pat->leftoffset);
//	int offset_y = LE_S16(pat->topoffset);

	// FIXME: validate values (in case we got flat data or so)

	if (pic_width)  *pic_width  = width;
	if (pic_height) *pic_height = height;

	if (img.is_null())
	{
		// our new image will be completely transparent
		img.resize (width, height);
	}

	// Centre the picture?
	if (pic_x_offset == INT_MIN)
		pic_x_offset = (img.width() - width) / 2;
	if (pic_y_offset == INT_MIN)
		pic_y_offset = (img.height() - height) / 2;

	for (int x = 0 ; x < width ; x++)
	{
		int offset = LE_S32(pat->columnofs[x]);

		if (offset < 0 || offset >= lump->Length())
		{
			LogPrintf("WARNING: bad image offset 0x%08x in patch [%s]\n",
			          offset, pic_name);
			return false;
		}

		const post_t *column = (const post_t *) ((const byte *)pat + offset);

		DrawColumn(img, column, pic_x_offset + x, pic_y_offset);
	}

	W_FreeLumpData(&raw_data);
	return true;
}


char W_DetectImageFormat(Lump_c *lump)
{
	byte header[20];

	int length = lump->Length();

	if (length < (int)sizeof(header))
		return 0;

	if (! lump->Seek())
		return 0;

	if (! lump->Read(header, (int)sizeof(header)))
		return 0;

	// PNG is clearly marked in the header, so check it first.

	if (header[0] == 0x89 &&
		header[1] == 'P'  &&
		header[2] == 'N'  &&
		header[3] == 'G')
	{
		return 'p'; /* PNG */
	}

	// check some other common image formats....

	if (header[0] == 0xFF &&
		header[1] == 0xD8)
	{
		return 'j'; /* JPEG */
	}

	if (header[0] == 'G' &&
		header[1] == 'I' &&
		header[2] == 'F' &&
		header[3] == '8')
	{
		return 'g'; /* GIF */
	}

	if (header[0] == 'B' &&
		header[1] == 'M')
	{
		return 'b'; /* BMP */
	}

	if (header[0] == 'D' &&
		header[1] == 'D' &&
		header[2] == 'S' &&
		header[3] == 0x20)
	{
		return 's'; /* DDS (DirectDraw Surface) */
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
		return 't'; /* TGA */
	}

	// check for raw patches last

	 width = (int)header[0] + (int)(header[1] << 8);
	height = (int)header[2] + (int)(header[3] << 8);

	int ofs_x = (int)header[4] + (int)(header[5] << 8);
	int ofs_y = (int)header[6] + (int)(header[7] << 8);

	if (width  > 0 && width  <= 2048 && abs(ofs_x) <= 2048 &&
		height > 0 && height <=  512 && abs(ofs_y) <=  512 &&
		length > width * 4 /* columnofs */)
	{
		return 'd'; /* Doom patch */
	}

	return 0;	// unknown!
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
