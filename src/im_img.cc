//------------------------------------------------------------------------
//  IMAGES
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

#include "im_img.h"


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


// Holds the private data members
class Img_priv
{
public:
    Img_priv () { buf = 0; width = 0; height = 0; opaque = false; }
    ~Img_priv () { if (buf != 0) delete[] buf; }
    img_pixel_t *buf;
    int    width;
    int    height;
    bool         opaque;
};


/*
 *  Img::Img - default constructor
 *
 *  The new image is a null image.
 */
Img::Img ()
{
	p = new Img_priv;
}


/*
 *  Img::Img - constructor with dimensions
 *
 *  The new image is set to the specified dimensions.
 */
Img::Img (int width, int height, bool opaque)
{
	p = new Img_priv;
	resize (width, height);
	set_opaque (opaque);
}


/*
 *  Img::~Img - dtor
 */
Img::~Img ()
{
	delete p;
}


/*
 *  Img::is_null - return true iff this is a null image
 */
bool Img::is_null () const
{
	return p->buf == 0;
}


/*
 *  Img::width - return the current width
 *
 *  If the image is null, return 0.
 */
int Img::width () const
{
	return p->width;
}


/*
 *  Img::height - return the current height
 *
 *  If the image is null, return 0.
 */
int Img::height () const
{
	return p->height;
}


/*
 *  Img::buf - return a const pointer on the buffer
 *
 *  If the image is null, return a null pointer.
 */
const img_pixel_t *Img::buf () const
{
	return p->buf;
}


/*
 *  Img::wbuf - return a writable pointer on the buffer
 *
 *  If the image is null, return a null pointer.
 */
img_pixel_t *Img::wbuf ()
{
	return p->buf;
}


/*
 *  Img::clear - clear the image
 */
void Img::clear ()
{
	if (p->buf != 0)
		memset (p->buf, TRANS_PIXEL, p->width * p->height);
}


/*
 *  Img::set_opaque - set or clear the opaque flag
 */
void Img::set_opaque (bool opaque)
{
	p->opaque = opaque;
}


/*
 *  Img::resize - resize the image
 *
 *  If either dimension is zero, the image becomes a null
 *  image.
 */
void Img::resize (int width, int height)
{
	if (width == p->width && height == p->height)
		return;

	// Unallocate old buffer
	if (p->buf != 0)
	{
		delete[] p->buf;
		p->buf = 0;
	}

	// Is it a null image ?
	if (width == 0 || height == 0)
	{
		p->width  = 0;
		p->height = 0;
		return;
	}

	// Allocate new buffer
	p->width  = width;
	p->height = height;
	p->buf = new img_pixel_t[width * height + 10];  // Some slack
	clear ();
}


/*
 *  spectrify_img - make a game image look vaguely like a spectre
 */
Img * Img::spectrify() const
{
	Img *omg = new Img(width(), height(), p->opaque);

	int x,y;
	byte grey = 104;

	// FIXME GAME CONFIG ITEM
	if (strcmp(Game_name, "heretic") == 0)
		grey = 8;

	int W = width();
	int H = height();

	const img_pixel_t *src = buf();
	img_pixel_t *dest = omg->wbuf ();

	for (y = 0; y < H; y++)
	for (x = 0; x < W; x++)
	{
		img_pixel_t pix = src[y * W + x];

		if (pix != TRANS_PIXEL)
			pix = grey + (rand () >> 6) % 7;  // FIXME more kludgery

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
 *    Img raw;
 *    Img scaled;
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
 */
void scale_img (const Img& img, double scale, Img& omg)
{
	int iwidth  = img.width ();
	int owidth  = (int) (img.width()  * scale + 0.5);
	int oheight = (int) (img.height() * scale + 0.5);
	omg.resize (owidth, oheight);
	const img_pixel_t *const ibuf = img.buf ();
	img_pixel_t       *const obuf = omg.wbuf ();

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
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
