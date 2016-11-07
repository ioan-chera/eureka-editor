//------------------------------------------------------------------------
//  IMAGES
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

#include "im_img.h"
#include "m_game.h"


// Now where did these came from?
int gammatable[5][256] =
{
	{
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
		49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
		65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
		81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
		97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
		113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
		128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
		192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
	},
	{
		2, 4, 5, 7, 8, 10, 11, 12, 14, 15, 16, 18, 19, 20, 21, 23, 24, 25, 26, 27, 29, 30, 31,
		32, 33, 34, 36, 37, 38, 39, 40, 41, 42, 44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 69, 70, 71, 72, 73, 74, 75, 76, 77,
		78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
		99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
		115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 129,
		130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145,
		146, 147, 148, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
		161, 162, 163, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
		175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 186, 187, 188, 189,
		190, 191, 192, 193, 194, 195, 196, 196, 197, 198, 199, 200, 201, 202, 203, 204,
		205, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 214, 215, 216, 217, 218,
		219, 220, 221, 222, 222, 223, 224, 225, 226, 227, 228, 229, 230, 230, 231, 232,
		233, 234, 235, 236, 237, 237, 238, 239, 240, 241, 242, 243, 244, 245, 245, 246,
		247, 248, 249, 250, 251, 252, 252, 253, 254, 255
	},
	{
		4, 7, 9, 11, 13, 15, 17, 19, 21, 22, 24, 26, 27, 29, 30, 32, 33, 35, 36, 38, 39, 40, 42,
		43, 45, 46, 47, 48, 50, 51, 52, 54, 55, 56, 57, 59, 60, 61, 62, 63, 65, 66, 67, 68, 69,
		70, 72, 73, 74, 75, 76, 77, 78, 79, 80, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93,
		94, 95, 96, 97, 98, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
		113, 114, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
		129, 130, 131, 132, 133, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
		144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 153, 154, 155, 156, 157, 158, 159,
		160, 160, 161, 162, 163, 164, 165, 166, 166, 167, 168, 169, 170, 171, 172, 172, 173,
		174, 175, 176, 177, 178, 178, 179, 180, 181, 182, 183, 183, 184, 185, 186, 187, 188,
		188, 189, 190, 191, 192, 193, 193, 194, 195, 196, 197, 197, 198, 199, 200, 201, 201,
		202, 203, 204, 205, 206, 206, 207, 208, 209, 210, 210, 211, 212, 213, 213, 214, 215,
		216, 217, 217, 218, 219, 220, 221, 221, 222, 223, 224, 224, 225, 226, 227, 228, 228,
		229, 230, 231, 231, 232, 233, 234, 235, 235, 236, 237, 238, 238, 239, 240, 241, 241,
		242, 243, 244, 244, 245, 246, 247, 247, 248, 249, 250, 251, 251, 252, 253, 254, 254,
		255
	},
	{
		8, 12, 16, 19, 22, 24, 27, 29, 31, 34, 36, 38, 40, 41, 43, 45, 47, 49, 50, 52, 53, 55,
		57, 58, 60, 61, 63, 64, 65, 67, 68, 70, 71, 72, 74, 75, 76, 77, 79, 80, 81, 82, 84, 85,
		86, 87, 88, 90, 91, 92, 93, 94, 95, 96, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
		108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
		125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 135, 136, 137, 138, 139, 140,
		141, 142, 143, 143, 144, 145, 146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155,
		155, 156, 157, 158, 159, 160, 160, 161, 162, 163, 164, 165, 165, 166, 167, 168, 169,
		169, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 179, 180, 180, 181, 182,
		183, 183, 184, 185, 186, 186, 187, 188, 189, 189, 190, 191, 192, 192, 193, 194, 195,
		195, 196, 197, 197, 198, 199, 200, 200, 201, 202, 202, 203, 204, 205, 205, 206, 207,
		207, 208, 209, 210, 210, 211, 212, 212, 213, 214, 214, 215, 216, 216, 217, 218, 219,
		219, 220, 221, 221, 222, 223, 223, 224, 225, 225, 226, 227, 227, 228, 229, 229, 230,
		231, 231, 232, 233, 233, 234, 235, 235, 236, 237, 237, 238, 238, 239, 240, 240, 241,
		242, 242, 243, 244, 244, 245, 246, 246, 247, 247, 248, 249, 249, 250, 251, 251, 252,
		253, 253, 254, 254, 255
	},
	{
		16, 23, 28, 32, 36, 39, 42, 45, 48, 50, 53, 55, 57, 60, 62, 64, 66, 68, 69, 71, 73, 75, 76,
		78, 80, 81, 83, 84, 86, 87, 89, 90, 92, 93, 94, 96, 97, 98, 100, 101, 102, 103, 105, 106,
		107, 108, 109, 110, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
		125, 126, 128, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141,
		142, 143, 143, 144, 145, 146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155, 155,
		156, 157, 158, 159, 159, 160, 161, 162, 163, 163, 164, 165, 166, 166, 167, 168, 169,
		169, 170, 171, 172, 172, 173, 174, 175, 175, 176, 177, 177, 178, 179, 180, 180, 181,
		182, 182, 183, 184, 184, 185, 186, 187, 187, 188, 189, 189, 190, 191, 191, 192, 193,
		193, 194, 195, 195, 196, 196, 197, 198, 198, 199, 200, 200, 201, 202, 202, 203, 203,
		204, 205, 205, 206, 207, 207, 208, 208, 209, 210, 210, 211, 211, 212, 213, 213, 214,
		214, 215, 216, 216, 217, 217, 218, 219, 219, 220, 220, 221, 221, 222, 223, 223, 224,
		224, 225, 225, 226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 232, 232, 233, 233,
		234, 234, 235, 235, 236, 236, 237, 237, 238, 239, 239, 240, 240, 241, 241, 242, 242,
		243, 243, 244, 244, 245, 245, 246, 246, 247, 247, 248, 248, 249, 249, 250, 250, 251,
		251, 252, 252, 253, 254, 254, 255, 255
	}
};


//------------------------------------------------------------------------


/*
 *  Img_c::Img_c - default constructor
 *
 *  The new image is a null image.
 */
Img_c::Img_c() : pixels(NULL), w(0), h(0)
{ }


/*
 *  Img_c::Img_c - constructor with dimensions
 *
 *  The new image is set to the specified dimensions.
 */
Img_c::Img_c(int width, int height, bool _dummy) :
	pixels(NULL), w(0), h(0)
{
	resize(width, height);
}


/*
 *  Img_c::~Img_c - destructor
 */
Img_c::~Img_c()
{
	delete pixels;
}


/*
 *  Img_c::buf - return a const pointer on the buffer
 *
 *  If the image is null, return a null pointer.
 */
const img_pixel_t *Img_c::buf() const
{
	return pixels;
}


/*
 *  Img_c::wbuf - return a writable pointer on the buffer
 *
 *  If the image is null, return a null pointer.
 */
img_pixel_t *Img_c::wbuf()
{
	return pixels;
}


/*
 *  Img_c::clear - clear the image
 */
void Img_c::clear()
{
	if (pixels)
	{
		img_pixel_t *dest = pixels;
		img_pixel_t *dest_end = dest + (w * h);

		for ( ; dest < dest_end ; dest++)
			*dest = TRANS_PIXEL;
	}
}


/*
 *  Img_c::resize - resize the image
 *
 *  If either dimension is zero, the image becomes a null
 *  image.
 */
void Img_c::resize(int new_width, int new_height)
{
	if (new_width == w && new_height == h)
		return;

	// Unallocate old buffer
	if (pixels)
	{
		delete[] pixels;
		pixels = NULL;
	}

	// Is it a null image ?
	if (new_width == 0 || new_height == 0)
	{
		w = h = 0;
		return;
	}

	// Allocate new buffer
	w = new_width;
	h = new_height;

	pixels = new img_pixel_t[w * h + 10];  // Some slack

	clear();
}


/*
 *  spectrify_img - make a game image look vaguely like a spectre
 */
Img_c * Img_c::spectrify() const
{
	Img_c *omg = new Img_c(width(), height());

	int invis_start = game_info.invis_colors[0];
	int invis_len   = game_info.invis_colors[1] - invis_start + 1;

	if (invis_len < 1)
		invis_len = 1;

	int W = width();
	int H = height();

	const img_pixel_t *src = buf();

	img_pixel_t *dest = omg->wbuf();

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		img_pixel_t pix = src[y * W + x];

		if (pix != TRANS_PIXEL)
			pix = invis_start + (rand() >> 4) % invis_len;

		dest[y * W + x] = pix;
	}

	return omg;
}


/*
 *  scale_img - scale a game image
 *
 *  <img> is the source image, <omg> is the destination
 *  image. <scale> is the scaling factor (> 1.0 to magnify).
 *  A scaled copy of <img> is put in <omg>. <img> is not
 *  modified. Any previous data in <omg> is lost.
 *
 *  Example:
 *
 *    Img_c raw;
 *    Img_c scaled;
 *    LoadPicture (raw, ...);
 *    scale_img (raw, 2, scaled);
 *    display_img (scaled, ...);
 *
 *  The implementation is mediocre in the case of scale
 *  factors < 1 because it uses only one source pixel per
 *  destination pixel. On certain patterns, it's likely to
 *  cause a visible loss of quality.
 *
 *  In the case of scale factors > 1, the algorithm is
 *  suboptimal.
 *
 *  andrewj: turned into a method, but untested...
 */
Img_c * Img_c::scale_img(double scale) const
{
	int iwidth  = width();
	int owidth  = (int) (width()  * scale + 0.5);
	int oheight = (int) (height() * scale + 0.5);

	Img_c *omg = new Img_c(owidth, oheight);

	const img_pixel_t *const ibuf = buf();
	img_pixel_t       *const obuf = omg->wbuf();

	if (true)
	{
		img_pixel_t *orow = obuf;
		int *ix = new int[owidth];
		for (int ox = 0; ox < owidth; ox++)
			ix[ox] = (int) (ox / scale);
		const int *const ix_end = ix + owidth;
		for (int oy = 0; oy < oheight; oy++)
		{
			int iy = (int) (oy / scale);
			const img_pixel_t *const irow = ibuf + iwidth * iy;
			for (const int *i = ix; i < ix_end; i++)
				*orow++ = irow[*i];
		}
		delete[] ix;
	}

	return omg;
}


/*
 *  Copy the image, but remap pixels in the range 'src1..src2' to the
 *  range 'targ1..targ2'.
 *
 *  TODO : make it work with RGB pixels (find nearest in palette).
 */
Img_c * Img_c::color_remap(int src1, int src2, int targ1, int targ2) const
{
	SYS_ASSERT( src1 <=  src2);
	SYS_ASSERT(targ1 <= targ2);

	Img_c *omg = new Img_c(width(), height());

	int W = width();
	int H = height();

	const img_pixel_t *src = buf();

	img_pixel_t *dest = omg->wbuf();

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		img_pixel_t pix = src[y * W + x];

		if (src1 <= pix && pix <= src2)
		{
			int diff = pix - src1;

			pix = targ1 + diff * (targ2 - targ1 + 1) / (src2 - src1 + 1);
		}

		dest[y * W + x] = pix;
	}

	return omg;
}


bool Img_c::has_transparent() const
{
	int W = width();
	int H = height();

	const img_pixel_t *src = buf();

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		if (src[y * W + x] == TRANS_PIXEL)
			return true;
	}

	return false;
}


void Img_c::test_make_RGB()
{
	int W = width();
	int H = height();

	img_pixel_t *src = wbuf();

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		img_pixel_t pix = src[y * W + x];

		if (pix != TRANS_PIXEL && ! (pix & IS_RGB_PIXEL))
		{
			const rgb_color_t col = palette[pix];

			byte r = RGB_RED(col)   >> 3;
			byte g = RGB_GREEN(col) >> 3;
			byte b = RGB_BLUE(col)  >> 3;

			src[y * W + x] = IMG_PIXEL_MAKE_RGB(r, g, b);
		}
	}
}


//------------------------------------------------------------------------


static int missing_tex_color;
static int unknown_tex_color;
static int unknown_flat_color;

static Img_c * missing_tex_image;
static Img_c * unknown_tex_image;
static Img_c * unknown_flat_image;


void IM_ResetDummyTextures()
{
	missing_tex_color  = -1;
	unknown_tex_color  = -1;
	unknown_flat_color = -1;
}


static const byte unknown_graphic[16 * 16] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,1,1,1,1,0,0,0,0,0,1,1,1,0,0,
	0,0,0,1,1,0,0,0,0,0,0,1,1,1,0,0,
	0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,
	0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,
	0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


static const byte missing_graphic[16 * 16] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


static Img_c * IM_CreateDummyTex(const byte *data, int bg, int fg)
{
	Img_c *omg = new Img_c(64, 64, true);

	img_pixel_t *obuf = omg->wbuf();

	for (int y = 0 ; y < 64 ; y++)
	for (int x = 0 ; x < 64 ; x++)
	{
		obuf[y * 64 + x] = data[((y/2) & 15 ) * 16 + ((x/2) & 15)] ? fg : bg;
	}

	return omg;
}


Img_c * IM_MissingTex()
{
	if (! missing_tex_image || missing_tex_color != game_info.missing_color)
	{
		missing_tex_color = game_info.missing_color;

		if (missing_tex_image)
			delete missing_tex_image;

		missing_tex_image = IM_CreateDummyTex(missing_graphic, missing_tex_color, 0);
	}

	return missing_tex_image;
}


Img_c * IM_UnknownTex()
{
	if (! unknown_tex_image || unknown_tex_color != game_info.unknown_tex)
	{
		unknown_tex_color = game_info.unknown_tex;

		if (unknown_tex_image)
			delete unknown_tex_image;

		unknown_tex_image = IM_CreateDummyTex(unknown_graphic, unknown_tex_color, 0);
	}

	return unknown_tex_image;
}


Img_c * IM_UnknownFlat()
{
	if (! unknown_flat_image || unknown_flat_color != game_info.unknown_flat)
	{
		unknown_flat_color = game_info.unknown_flat;

		if (unknown_flat_image)
			delete unknown_flat_image;

		unknown_flat_image = IM_CreateDummyTex(unknown_graphic, unknown_flat_color, 0);
	}

	return unknown_flat_image;
}


Img_c * IM_CreateFromText(int W, int H, const char **text, const rgb_color_t *palette, int pal_size)
{
	Img_c *result = new Img_c(W, H);

	result->clear();

	// translate colors to current palette
	byte *conv_palette = new byte[pal_size];

	for (int c = 0 ; c < pal_size ; c++)
		conv_palette[c] = W_FindPaletteColor(RGB_RED(palette[c]), RGB_GREEN(palette[c]), RGB_BLUE(palette[c]));

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		int ch = text[y][x] & 0x7f;

		if (ch == ' ')
			continue;  // leave transparent

		if (ch < 'a' || ch >= 'a' + pal_size)
			BugError("Bad character (dec #%d) in built-in image.\n", ch);

		result->wbuf() [y * W + x] = conv_palette[ch - 'a'];
	}

	delete[] conv_palette;

	return result;
}


Img_c * IM_FromRGBImage(Fl_RGB_Image *src)
{
	int W  = src->w();
	int H  = src->h();
	int D  = src->d();
	int LD = src->ld();

	LD += W;

	const byte * data = (const byte *) src->array;

	if (! data)
		return NULL;

	if (! (D == 3 || D == 4))
		return NULL;

	Img_c *img = new Img_c(W, H);

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		const byte *src_pix = data + (y * LD + x) * D;

		int r = src_pix[0];
		int g = src_pix[1];
		int b = src_pix[2];
		int a = (D == 3) ? 255 : src_pix[3];

		img_pixel_t dest_pix = TRANS_PIXEL;

		if (a & 128)
		{
			// TODO : a preference to palettize it
			// dest_pix = W_FindPaletteColor(r, g, b);

			dest_pix = IMG_PIXEL_MAKE_RGB(r >> 3, g >> 3, b >> 3);
		}

		img->wbuf() [ y * W + x ] = dest_pix;
	}

	return img;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
