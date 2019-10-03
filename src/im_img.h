//------------------------------------------------------------------------
//  IMAGES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
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

#ifndef __EUREKA_IM_IMG_H__
#define __EUREKA_IM_IMG_H__

#include "im_color.h"

#ifdef NO_OPENGL
typedef unsigned int GLuint;
#else
#include "FL/gl.h"
#endif

// this is a 16-bit value:
//   - when high bit is clear, it is a palette index 0-255
//     (value 255 is used to represent fully transparent).
//   - when high bit is set, the remainder is 5:5:5 RGB
typedef unsigned short  img_pixel_t;

const img_pixel_t IS_RGB_PIXEL = 0x8000;

#define IMG_PIXEL_RED(col)    (((col) >> 10) & 31)
#define IMG_PIXEL_GREEN(col)  (((col) >>  5) & 31)
#define IMG_PIXEL_BLUE(col)   (((col)      ) & 31)

#define IMG_PIXEL_MAKE_RGB(r, g, b)  (IS_RGB_PIXEL | ((r) << 10) | ((g) << 5) | (b))


// the color number used to represent transparent pixels in an Img_c.
const img_pixel_t TRANS_PIXEL = 255;


class Img_c
{
private:
	img_pixel_t *pixels;

	int  w;  // Width
	int  h;  // Height

	// texture identifier for OpenGL, 0 if not uploaded yet
	GLuint gl_tex;

public:
	 Img_c();
	 Img_c(int width, int height, bool _dummy = false);
	~Img_c();

	bool is_null() const
	{
		return (! pixels);
	}

	int width() const
	{
		return w;
	}

	int height() const
	{
		return h;
	}

	GLuint gl_texture() const
	{
		return gl_tex;
	}

	// read access
	const img_pixel_t *buf() const;

	// read/write access
	img_pixel_t *wbuf();

public:
	// set all pixels to TRANS_PIXEL
	void clear();

	void resize(int new_width, int new_height);

	Img_c * spectrify() const;

	Img_c * scale_img(double scale) const;

	Img_c * color_remap(int src1, int src2, int targ1, int targ2) const;

	bool has_transparent() const;

	// upload to OpenGL, overwriting 'gl_tex' field.
	void load_gl();

	// invalidate the 'gl_tex' field, deleting old texture if possible.
	void unload_gl(bool can_delete);

	void bind_gl();

	// convert pixels to RGB mode, for testing other code
	void test_make_RGB();

private:
	Img_c            (const Img_c&);  // No need to implement it
	Img_c& operator= (const Img_c&);  // No need to implement it
};


void IM_ResetDummyTextures();
void IM_UnloadDummyTextures();

Img_c * IM_MissingTex();
Img_c * IM_UnknownTex();
Img_c * IM_UnknownFlat();
Img_c * IM_UnknownSprite();

Img_c * IM_CreateFromText(int W, int H, const char **text, const rgb_color_t *palette, int pal_size);
Img_c * IM_CreateDogSprite();
Img_c * IM_CreateLightSprite();

Img_c * IM_ConvertRGBImage(Fl_RGB_Image *src);
Img_c * IM_ConvertTGAImage(const rgba_color_t * data, int W, int H);


//------------------------------------------------------------------------


inline void IM_DecodePixel_raw(img_pixel_t p, byte& r, byte& g, byte& b)
{
	if (p & IS_RGB_PIXEL)
	{
		r = IMG_PIXEL_RED(p)   << 3;
		g = IMG_PIXEL_GREEN(p) << 3;
		b = IMG_PIXEL_BLUE(p)  << 3;
	}
	else
	{
		r = raw_palette[p][0];
		g = raw_palette[p][1];
		b = raw_palette[p][2];
	}
}

// this one applies the current gamma.
// for rendering the 3D view or the 2D sectors and sprites.
inline void IM_DecodePixel(img_pixel_t p, byte& r, byte& g, byte& b)
{
	if (p & IS_RGB_PIXEL)
	{
		r = rgb555_gamma[IMG_PIXEL_RED(p)];
		g = rgb555_gamma[IMG_PIXEL_GREEN(p)];
		b = rgb555_gamma[IMG_PIXEL_BLUE(p)];
	}
	else
	{
		const rgb_color_t col = palette[p];

		r = RGB_RED(col);
		g = RGB_GREEN(col);
		b = RGB_BLUE(col);
	}
}

// this applies a constant gamma.
// for textures/flats/things in the browser and panels.
inline void IM_DecodePixel_medium(img_pixel_t p, byte& r, byte& g, byte& b)
{
	if (p & IS_RGB_PIXEL)
	{
		r = rgb555_medium[IMG_PIXEL_RED(p)];
		g = rgb555_medium[IMG_PIXEL_GREEN(p)];
		b = rgb555_medium[IMG_PIXEL_BLUE(p)];
	}
	else
	{
		const rgb_color_t col = palette_medium[p];

		r = RGB_RED(col);
		g = RGB_GREEN(col);
		b = RGB_BLUE(col);
	}
}

#endif  /* __EUREKA_IM_IMG_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
