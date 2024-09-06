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

#include "tl/optional.hpp"

#include <memory>
#include <vector>

static constexpr img_pixel_t IS_RGB_PIXEL = 0x8000;

#define IMG_PIXEL_RED(col)    (((col) >> 10) & 31)
#define IMG_PIXEL_GREEN(col)  (((col) >>  5) & 31)
#define IMG_PIXEL_BLUE(col)   (((col)      ) & 31)

static constexpr img_pixel_t pixelMakeRGB(int r, int g, int b)
{
	return static_cast<img_pixel_t>(IS_RGB_PIXEL | r << 10 | g << 5 | b);
}


// the color number used to represent transparent pixels in an Img_c.
const img_pixel_t TRANS_PIXEL = 255;

class Fl_RGB_Image;
class Instance;
class Palette;
struct ConfigData;
struct WadData;

class Img_c
{
private:
	std::vector<img_pixel_t> pixels;

	int  w = 0;  // Width
	int  h = 0;  // Height
	int spriteOffsetX = 0;
	int spriteOffsetY = 0;

	// texture identifier for OpenGL, 0 if not uploaded yet
	GLuint gl_tex = 0;

public:
	 Img_c() = default;
	 Img_c(int width, int height, bool _dummy = false);

	static Img_c createLightSprite(const Palette &palette);
	static Img_c createMapSpotSprite(const Palette &pal, int base_r, int base_g,
													  int base_b);
	static Img_c createDogSprite(const Palette &pal);

	inline bool is_null() const
	{
		return pixels.empty();
	}

	inline int width() const noexcept
	{
		return w;
	}

	inline int height() const noexcept
	{
		return h;
	}

	GLuint gl_texture() const
	{
		return gl_tex;
	}

	// read access
	const img_pixel_t *buf() const noexcept;

	// read/write access
	img_pixel_t *wbuf();

	// set all pixels to TRANS_PIXEL
	void clear();

	void resize(int new_width, int new_height);

	// paste a copy of another image into this one, but skip any
	// transparent pixels.
	void compose(const Img_c &other, int x, int y);
	
	void flipHorizontally();

	Img_c spectrify(const ConfigData &config) const;

	Img_c color_remap(int src1, int src2, int targ1, int targ2) const;

	bool has_transparent() const;

	// upload to OpenGL, overwriting 'gl_tex' field.
	void load_gl(const WadData &wad);

	// invalidate the 'gl_tex' field, deleting old texture if possible.
	void unload_gl(bool can_delete);

	void bind_gl(const WadData &wad);

	void setSpriteOffset(int x, int y)
	{
		spriteOffsetX = x;
		spriteOffsetY = y;
	}
	void getSpriteOffset(int& x, int& y) const
	{
		x = spriteOffsetX;
		y = spriteOffsetY;
	}

private:
	static Img_c createFromText(const Palette &pal, int W, int H,
												 const char * const*text,
												 const rgb_color_t *palette, int pal_size);
};

tl::optional<Img_c> IM_ConvertRGBImage(const Fl_RGB_Image &src);
Img_c IM_ConvertTGAImage(const rgba_color_t *data, int W, int H);

#endif  /* __EUREKA_IM_IMG_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
