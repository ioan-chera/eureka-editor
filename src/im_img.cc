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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include "im_img.h"
#include "m_game.h"
#include "tl/optional.hpp"

#ifndef NO_OPENGL
// need this for GL_UNSIGNED_INT_8_8_8_8_REV
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#else
#include "GL/glext.h"
#endif
#endif

#define DIGIT_FONT_COLOR   rgbMake(68, 221, 255)


rgb_color_t Palette::pixelToRGB(img_pixel_t p) const
{
	if (p & IS_RGB_PIXEL)
	{
		byte r = static_cast<byte>(IMG_PIXEL_RED(p)   << 3);
		byte g = static_cast<byte>(IMG_PIXEL_GREEN(p) << 3);
		byte b = static_cast<byte>(IMG_PIXEL_BLUE(p)  << 3);

		return rgbMake(r, g, b);
	}
	else
	{
		byte r = raw_palette[p][0];
		byte g = raw_palette[p][1];
		byte b = raw_palette[p][2];

		return rgbMake(r, g, b);
	}
}

//
// a constructor with dimensions
//
Img_c::Img_c(int width, int height, bool _dummy)
{
	resize(width, height);
	setSpriteOffset(width / 2, height);
}

//
//  return a const pointer on the buffer.
//  if the image is null, return a NULL pointer.
//
const img_pixel_t *Img_c::buf() const noexcept
{
	return pixels.data();
}


//
// return a writable pointer on the buffer.
// if the image is null, return a NULL pointer.
//
img_pixel_t *Img_c::wbuf()
{
	return pixels.data();
}


//
// clear the image to fully transparent
//
void Img_c::clear()
{
	img_pixel_t *dest = pixels.data();
	img_pixel_t *dest_end = dest + (w * h);

	for ( ; dest < dest_end ; dest++)
		*dest = TRANS_PIXEL;
}


//
// resize the image.  if either dimension is zero,
// the image becomes a null image.
//
void Img_c::resize(int new_width, int new_height)
{
	if (new_width == w && new_height == h)
		return;

	// unallocate old buffer
	pixels.clear();

	// Is it a null image ?
	if (new_width == 0 || new_height == 0)
	{
		w = h = 0;
		return;
	}

	// Allocate new buffer
	w = new_width;
	h = new_height;

	pixels.resize(w * h + 10);  // Some slack

	clear();
}


void Img_c::compose(const Img_c &other, int x, int y)
{
	int W = width();
	int H = height();

	int OW = other.width();
	int OH = other.height();

	for (int oy = 0 ; oy < OH ; oy++)
	{
		int iy = y + oy;
		if (iy < 0 || iy >= H)
			continue;

		const img_pixel_t *src = other.buf() + oy * OW;
		img_pixel_t *dest = wbuf() + iy * W;

		for (int ox = 0 ; ox < OW ; ox++, src++)
		{
			int ix = x + ox;
			if (ix < 0 || ix >= W)
				continue;

			if (*src != TRANS_PIXEL)
				dest[ix] = *src;
		}
	}
}

void Img_c::flipHorizontally()
{
	assert((int)pixels.size() >= w * h);
	for(int y = 0; y < h; ++y)
		for(int x = 0; x < w / 2; ++x)
		{
			assert(y * w + x >= 0 && y * w + x < (int)pixels.size());
			assert(y * w + w - x - 1 >= 0 && y * w + w - x - 1 < (int)pixels.size());
			std::swap(pixels[y * w + x], pixels[y * w + w - x - 1]);
		}
}

//
// make a game image look vaguely like a spectre
//
Img_c Img_c::spectrify(const ConfigData &config) const
{
	Img_c omg(width(), height());
	omg.spriteOffsetX = spriteOffsetX;
	omg.spriteOffsetY = spriteOffsetY;

	int invis_start = config.miscInfo.invis_colors[0];
	int invis_len   = config.miscInfo.invis_colors[1] - invis_start + 1;

	if (invis_len < 1)
		invis_len = 1;

	int W = width();
	int H = height();

	const img_pixel_t *src = buf();

	img_pixel_t *dest = omg.wbuf();

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		assert(y * W + x >= 0 && y * W + x < (int)pixels.size());
		img_pixel_t pix = src[y * W + x];

		if (pix != TRANS_PIXEL)
			pix = static_cast<img_pixel_t>(invis_start + (rand() >> 4) % invis_len);

		assert(y * W + x >= 0 && y * W + x < (int)omg.pixels.size());
		dest[y * W + x] = pix;
	}

	return omg;
}


//
// copy the image, remapping pixels in the range 'src1..src2' to the
// range 'targ1..targ2'.
//
// TODO : make it work with RGB pixels (find nearest in palette).
//
Img_c Img_c::color_remap(int src1, int src2, int targ1, int targ2) const
{
	SYS_ASSERT( src1 <=  src2);
	SYS_ASSERT(targ1 <= targ2);

	Img_c omg(width(), height());
	omg.spriteOffsetX = spriteOffsetX;
	omg.spriteOffsetY = spriteOffsetY;

	int W = width();
	int H = height();

	const img_pixel_t *src = buf();

	img_pixel_t *dest = omg.wbuf();

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		img_pixel_t pix = src[y * W + x];

		if (src1 <= pix && pix <= src2)
		{
			int diff = pix - src1;

			pix = static_cast<img_pixel_t>(targ1 + diff * (targ2 - targ1 + 1) / (src2 - src1 + 1));
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


#ifdef NO_OPENGL

void Img_c::load_gl(const WadData &wad) {}
void Img_c::unload_gl(bool can_delete) {}
void Img_c::bind_gl(const WadData &wad) {}

#else

void Img_c::load_gl(const WadData &wad)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &gl_tex);
	glBindTexture(GL_TEXTURE_2D, gl_tex);

	// construct a power-of-two sized bottom-up RGBA image
	int tw, th;
	if (global::use_npot_textures)
	{
		tw = w;
		th = h;
	}
	else
	{
		tw = RoundPOW2(w);
		th = RoundPOW2(h);
	}

	byte *rgba = new byte[tw * th * 4];

	memset(rgba, 0, (size_t)(tw * th * 4));

	bool has_trans = has_transparent();

	int ex = has_trans ? w : tw;
	int ey = has_trans ? h : th;

	int x, y;

	for (y = 0 ; y < ey ; y++)
	{
		// invert source Y for OpenGL
		int sy = h - 1 - y;
		if (sy < 0)
			sy += h;

		for (x = 0 ; x < ex ; x++)
		{
			int sx = x;
			if (sx >= w)
				sx = x - w;

			// convert pixel to RGBA
			const img_pixel_t pix = buf()[sy*w + sx];

			if (pix != TRANS_PIXEL)
			{
				byte r, g, b;

				wad.palette.decodePixel(pix, r, g, b);

				byte *dest = rgba + (y*tw + x) * 4;

				dest[0] = b;
				dest[1] = g;
				dest[2] = r;
				dest[3] = 255;
			}
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0 /* mip */,
		GL_RGBA8, tw, th, 0 /* border */,
		GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV, rgba);

	delete[] rgba;
}


void Img_c::unload_gl(bool can_delete)
{
	if (can_delete && gl_tex != 0)
	{
		glDeleteTextures(1, &gl_tex);
	}

	gl_tex = 0;
}


void Img_c::bind_gl(const WadData &wad)
{
	// create the GL texture if we haven't already
	if (gl_tex == 0)
	{
		// this will do a glBindTexture
		load_gl(wad);
		return;
	}

	glBindTexture(GL_TEXTURE_2D, gl_tex);
}

#endif

//------------------------------------------------------------------------


void ImageSet::IM_ResetDummyTextures()
{
	missing_tex_color  = -1;
	unknown_tex_color  = -1;
	special_tex_color  = -1;
	unknown_flat_color = -1;
	unknown_sprite_color = -1;
}


void ImageSet::IM_UnloadDummyTextures()
{
	bool can_delete = false;

	if (missing_tex_image)
		missing_tex_image->unload_gl(can_delete);

	if (unknown_tex_image)
		unknown_tex_image->unload_gl(can_delete);

	if (special_tex_image)
		special_tex_image->unload_gl(can_delete);

	if (unknown_flat_image)
		unknown_flat_image->unload_gl(can_delete);

	if (unknown_sprite_image)
		unknown_sprite_image->unload_gl(can_delete);

	if (digit_font_11x14)
		digit_font_11x14->unload_gl(can_delete);

	if (digit_font_14x19)
		digit_font_14x19->unload_gl(can_delete);
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


static Img_c IM_CreateDummyTex(const byte *data, int bg, int fg)
{
	Img_c omg(64, 64, true);

	img_pixel_t *obuf = omg.wbuf();

	for (int y = 0 ; y < 64 ; y++)
	for (int x = 0 ; x < 64 ; x++)
	{
		obuf[y * 64 + x] = static_cast<img_pixel_t>(data[((y/2) & 15 ) * 16 + ((x/2) & 15)] ?
                                                    fg : bg);
	}

	return omg;
}


const Img_c &ImageSet::IM_MissingTex(const ConfigData &config)
{
	if (!missing_tex_image || missing_tex_color != config.miscInfo.missing_color)
	{
		missing_tex_color = config.miscInfo.missing_color;

		missing_tex_image = IM_CreateDummyTex(missing_graphic, missing_tex_color, 0);
	}

	return missing_tex_image.value();
}


const Img_c &ImageSet::IM_UnknownTex(const ConfigData &config)
{
	if (!unknown_tex_image || unknown_tex_color != config.miscInfo.unknown_tex)
	{
		unknown_tex_color = config.miscInfo.unknown_tex;

		unknown_tex_image = IM_CreateDummyTex(unknown_graphic, unknown_tex_color, 0);
	}

	return unknown_tex_image.value();
}


const Img_c &ImageSet::IM_SpecialTex(const Palette &palette)
{
	if (special_tex_color < 0)
	{
		special_tex_color = palette.findPaletteColor(192, 0, 192);

		special_tex_image.reset();
	}

	if (!special_tex_image)
		special_tex_image = IM_CreateDummyTex(unknown_graphic, special_tex_color,
			palette.findPaletteColor(255, 255, 255));

	return special_tex_image.value();
}


const Img_c &ImageSet::IM_UnknownFlat(const ConfigData &config)
{
	if (!unknown_flat_image || unknown_flat_color != config.miscInfo.unknown_flat)
	{
		unknown_flat_color = config.miscInfo.unknown_flat;

		unknown_flat_image = IM_CreateDummyTex(unknown_graphic, unknown_flat_color, 0);
	}

	return unknown_flat_image.value();
}


Img_c &ImageSet::IM_UnknownSprite(const ConfigData &config)
{
	int unk_col = config.miscInfo.unknown_thing;
	if (unk_col == 0)
		unk_col = config.miscInfo.unknown_tex;

	if (!unknown_sprite_image || unknown_sprite_color != unk_col)
	{
		unknown_sprite_color = unk_col;

		unknown_sprite_image = Img_c(64, 64, true);

		img_pixel_t *obuf = unknown_sprite_image->wbuf();

		for (int y = 0 ; y < 64 ; y++)
		for (int x = 0 ; x < 64 ; x++)
		{
			obuf[y * 64 + x] = static_cast<img_pixel_t>(unknown_graphic[(y/4) * 16 + (x/4)] ?
				unknown_sprite_color : TRANS_PIXEL);
		}
	}

	return unknown_sprite_image.value();
}


Img_c Img_c::createFromText(const Palette &pal, int W, int H,
											 const char * const*text, const rgb_color_t *palette,
											 int pal_size)
{
	Img_c result(W, H);

	result.clear();

	// translate colors to current palette
	std::vector<byte> conv_palette;
	conv_palette.reserve(pal_size);

	for (int c = 0 ; c < pal_size ; c++)
	{
		conv_palette.push_back(pal.findPaletteColor(RGB_RED(palette[c]), RGB_GREEN(palette[c]),
													RGB_BLUE(palette[c])));
	}

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		int ch = text[y][x] & 0x7f;

		if (ch == ' ')
			continue;  // leave transparent

		if (ch < 'a' || ch >= 'a' + pal_size)
			BugError("Bad character (dec #%d) in built-in image.\n", ch);

		result.wbuf() [y * W + x] = conv_palette[ch - 'a'];
	}

	return result;
}


static Img_c IM_CreateFont(int W, int H, const char *const *text,
							 const int *intensities, int ity_size,
							 rgb_color_t color)
{
	Img_c result(W, H);

	result.clear();

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		int ch = text[y][x] & 0x7f;

		if (ch == ' ')
			continue;  // leave transparent

		if (ch < 'a' || ch >= 'a' + ity_size)
			BugError("Bad character (dec #%d) in built-in font.\n", ch);

		int ity = intensities[ch - 'a'];

		int r = (RGB_RED(color)   * ity) >> 11;
		int g = (RGB_GREEN(color) * ity) >> 11;
		int b = (RGB_BLUE(color)  * ity) >> 11;

		result.wbuf() [y * W + x] = pixelMakeRGB(r, g, b);
	}

	return result;
}


tl::optional<Img_c> IM_ConvertRGBImage(const Fl_RGB_Image &src)
{
	int W  = src.w();
	int H  = src.h();
	int D  = src.d();
	int LD = src.ld();

	LD += W;

	auto data = static_cast<const byte *>(src.array);

	if (! data)
		return {};

	if (! (D == 3 || D == 4))
		return {};

	Img_c img(W, H);

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

			dest_pix = pixelMakeRGB(r >> 3, g >> 3, b >> 3);
		}

		img.wbuf() [ y * W + x ] = dest_pix;
	}

	return img;
}


Img_c IM_ConvertTGAImage(const rgba_color_t * data, int W, int H)
{
	Img_c img(W, H);

	img_pixel_t *dest = img.wbuf();

	for (int i = W * H ; i > 0 ; i--, data++, dest++)
	{
		if (RGBA_ALPHA(*data) & 128)
		{
			byte r = RGB_RED(  *data) >> 3;
			byte g = RGB_GREEN(*data) >> 3;
			byte b = RGB_BLUE( *data) >> 3;

			*dest = pixelMakeRGB(r, g, b);
		}
		else
		{
			*dest = TRANS_PIXEL;
		}
	}

	return img;
}


//------------------------------------------------------------------------

//
// eight basic arrow sprites, made by Andrew Apted, public domain.
//

/* XPM */
extern const char *const arrow_0_xpm[] =
{
	"12 12 2 1",
	" 	c None",
	"1	c #000000",
	"      1     ",
	"      11    ",
	"      111   ",
	"      1111  ",
	"      11111 ",
	"111111111111",
	"111111111111",
	"      11111 ",
	"      1111  ",
	"      111   ",
	"      11    ",
	"      1     "
};

/* XPM */
extern const char *const arrow_45_xpm[] =
{
	"12 12 2 1",
	" 	c None",
	"1	c #000000",
	"            ",
	"   11111111 ",
	"    1111111 ",
	"     111111 ",
	"      11111 ",
	"     111111 ",
	"    111 111 ",
	"   111   11 ",
	"  111     1 ",
	" 111        ",
	" 11         ",
	"            "
};

/* XPM */
extern const char *const arrow_90_xpm[] =
{
	"12 12 2 1",
	" 	c None",
	"1	c #000000",
	"     11     ",
	"    1111    ",
	"   111111   ",
	"  11111111  ",
	" 1111111111 ",
	"111111111111",
	"     11     ",
	"     11     ",
	"     11     ",
	"     11     ",
	"     11     ",
	"     11     "
};

/* XPM */
extern const char *const arrow_135_xpm[] =
{
	"12 12 2 1",
	" 	c None",
	"1	c #000000",
	"            ",
	" 11111111   ",
	" 1111111    ",
	" 111111     ",
	" 11111      ",
	" 111111     ",
	" 111 111    ",
	" 11   111   ",
	" 1     111  ",
	"        111 ",
	"         11 ",
	"            "
};

/* XPM */
extern const char *const arrow_180_xpm[] =
{
	"12 12 2 1",
	" 	c None",
	"1	c #000000",
	"     1      ",
	"    11      ",
	"   111      ",
	"  1111      ",
	" 11111      ",
	"111111111111",
	"111111111111",
	" 11111      ",
	"  1111      ",
	"   111      ",
	"    11      ",
	"     1      "
};

/* XPM */
extern const char *const arrow_225_xpm[] =
{
	"12 12 2 1",
	" 	c None",
	"1	c #000000",
	"            ",
	"         11 ",
	"        111 ",
	" 1     111  ",
	" 11   111   ",
	" 111 111    ",
	" 111111     ",
	" 11111      ",
	" 111111     ",
	" 1111111    ",
	" 11111111   ",
	"            "
};

/* XPM */
extern const char *const arrow_270_xpm[] =
{
	"12 12 2 1",
	" 	c None",
	"1	c #000000",
	"     11     ",
	"     11     ",
	"     11     ",
	"     11     ",
	"     11     ",
	"     11     ",
	"111111111111",
	" 1111111111 ",
	"  11111111  ",
	"   111111   ",
	"    1111    ",
	"     11     "
};

/* XPM */
extern const char *const arrow_315_xpm[] =
{
	"12 12 2 1",
	" 	c None",
	"1	c #000000",
	"            ",
	" 11         ",
	" 111        ",
	"  111     1 ",
	"   111   11 ",
	"    111 111 ",
	"     111111 ",
	"      11111 ",
	"     111111 ",
	"    1111111 ",
	"   11111111 ",
	"            "
};


//------------------------------------------------------------------------

//
// This dog sprite was sourced from OpenGameArt.org
// Authors are 'Benalene' and 'qudobup' (users on the OGA site).
// License is CC-BY 3.0 (Creative Commons Attribution license).
//

static const rgb_color_t dog_palette[] =
{
	0x302020ff,
	0x944921ff,
	0x000000ff,
	0x844119ff,
	0x311800ff,
	0x4A2400ff,
	0x633119ff,
};


static const char *const dog_image_text[] =
{
	"       aaaa                                 ",
	"      abbbba                                ",
	"     abbbbbba                               ",
	" aaaabcbbbbbda                              ",
	"aeedbbbfbbbbda                              ",
	"aegdddbbdbbdbbaaaaaaaaaaaaaaaaa           a ",
	"affggddbgddgbccceeeeeeeeeeeeeeeaa        aba",
	" affgggdfggfccceeeeeeeeeeeeeefffgaaa   aaba ",
	"  afffaafgecccefffffffffffffffggggddaaabbba ",
	"   aaa  aeeccggggffffffffffffggddddbbbbbaa  ",
	"         accbdddggfffffffffffggdbbbbbbba    ",
	"          aabbdbddgfffffffffggddbaaaaaa     ",
	"            abbbbdddfffffffggdbbba          ",
	"            abbbbbbdddddddddddbbba          ",
	"           aeebbbbbbbbaaaabbbbbbbba         ",
	"           aeebbbbbaaa    aeebbbbbba        ",
	"          afebbbbaa       affeebbbba        ",
	"         agfbbbaa         aggffabbbba       ",
	"        agfebba           aggggaabbba       ",
	"      aadgfabba            addda abba       ",
	"     abbddaabbbaa           adddaabba       ",
	"    abbbba  abbbba          adbbaabba       ",
	"     aaaa    abbba         abbba  abba      ",
	"              aaa         abbba   abba      ",
	"                         abbba   abbba      ",
	"                          aaa     aaa       "
};


Img_c Img_c::createDogSprite(const Palette &pal)
{
	return createFromText(pal, 44, 26, dog_image_text, dog_palette, 7);
}


//------------------------------------------------------------------------

Img_c Img_c::createLightSprite(const Palette &palette)
{
	static const int W = 11;
	static const int H = 11;

	Img_c result(W, H);

	result.clear();

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		byte pix = TRANS_PIXEL;

		float dx = (W - 2*x) / (float)W;
		float dy = (H - 2*y) / (float)H;

		float dist = sqrt((dx) * (dx) + (dy) * (dy));

		float ity = 1.0f / (dist + 0.5f) / (dist + 0.5f);

		if (ity < 0.5)
			continue;

		ity = (ity - 0.4f) / (1.0f - 0.4f);

		int r = static_cast<int>(255 * ity);
		int g = static_cast<int>(235 * ity);
		int b = static_cast<int>(90  * ity);

		pix = palette.findPaletteColor(r, g, b);

		result.wbuf() [ y * W + x ] = pix;
	}

	return result;
}


Img_c Img_c::createMapSpotSprite(const Palette &pal, int base_r, int base_g,
												  int base_b)
{
	int W = 32;
	int H = 32;

	Img_c result(W, H);

	result.clear();

	for (int y = 4 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		byte pix = TRANS_PIXEL;

		int cx1 = y/2;
		int cx2 = W - y/2;

		if (cx1 <= x && x <= cx2)
		{
			float dx = static_cast<float>(std::min(x - cx1, cx2 - x));
			//float dy = MIN(abs(y - 4), abs(y - W));

			float ity = 0.3f + dx / 14.0f;
			if (ity > 1.0) ity = 1.0;

			int r = static_cast<int>(base_r * ity);
			int g = static_cast<int>(base_g * ity);
			int b = static_cast<int>(base_b * ity);

			pix = pal.findPaletteColor(r, g, b);
		}

		result.wbuf() [ y * W + x ] = pix;
	}

	return result;
}


//------------------------------------------------------------------------

/* a digit-only font, in two sizes */

static const int digit_font_intensities[] =
{
	0x00, 0x16, 0x24, 0x32, 0x3f,
	0x4c, 0x58, 0x65, 0x72, 0x7e,
	0x8b, 0x98, 0xa4, 0xb0, 0xbc,
	0xc9, 0xd6, 0xe2, 0xef, 0xfc,
};


static const char *const digit_11x14_text[] =
{
	"                                                                                                                                                 aaaaaaa  ",
	"  aaaaaaa    aaaaaa     aaaaaaaa    aaaaaaa     aaaaa    aaaaaaa     aaaaaa    aaaaaaaa   aaaaaaa    aaaaaaa                                     agqspda  ",
	"  agqspda    alprga     aeorsoca    apsspfa    aalrga    aprrrka    aanssna    afrrrrqa   ajrsqha   aajrspca                                    aaqogqoa  ",
	" aaqogqoa    aontha     ahojirna    amiiqpa    aertha    asmkkga   aanrjika    adkkkpqa  aasnfora   adsmgqoa     aaaa                           afteaitaa ",
	" afteaitaa   aaatha     aaaaamqa    aaaakra   aaomtha    ashaaaa   absiaaaa    aaaaarma  adtcahta   aksaajsaa    ahfa                           agtcagtba ",
	" ajsaaatga     atha        aaooa     ahjqna   ahratha    asrqmaa   agtmrpfa       agtea  aarnhopa   akraajtda    arma                           aarlanqaa ",
	" akraaatha     atha       aajtha     aqssha  aaqjathaa   ankmsna   ajtqjpraa      anqaa  aaktssiaa  aftjaptga    ajga                 aaaaaa     ajtssha  ",
	" aksaaatga     atha      aagskaa     aaanra  akqeetiba   aaaalsa   ajthactga     aarla   aftialsba  aalstotca    aaaa                 amrrla     aadhcaa  ",
	" agtcagtba     atha     aaermaa    aaaaagta  antttttka  aaaaajsa   afteaatha     ahtca   aksaaatga   aaaajsaa    aaaa       aaaa      agjjfa      aaaaa   ",
	" aarlanqaa   aaathaa    acroaaaa   afhaanra  aaaaathaa  aegacpqa   aarmaksba     aopaa   agthaktba   agadrna     amia       anha      aaaaaa              ",
	"  ajtssha    aqtttsa    ajttttra   ahtstsia      atha   ahtstrga    aisrtlaa     aska    aantrtmaa   arstpba     arma       aska                          ",
	"  aadhcaa    aaaaaaa    aaaaaaaa   aabhgaaa      aaaa   aachfaaa    aachcaa      aaaa     aadhdaa    abhfaaa     aaaa       aaaa                          ",
	"   aaaaa                            aaaaa                aaaaa       aaaaa                 aaaaa     aaaaa                                                ",
	"                                                                                                                                                          "
};


static const char *const digit_14x19_text[] =
{
	"                                                                                                                                                                                          aaaaaa    ",
	"    aaaaaa         aaaaa       aaaaaaa       aaaaaaa          aaaaa     aaaaaaaaa        aaaaaa     aaaaaaaaaa      aaaaaa        aaaaaa                                                 aadklgaa   ",
	"   aadklgaa     aaaafhca      aacjlkeaa     aadjlkfaa         adhga     adhhhhhhaa      aagllhaa    afhhhhhhga     aafklhaaa     aaglleaa                                               aajstttnaa  ",
	"  aajstttnaa    agqsttja      ansttttlaa    altttttoaa       aaqtra     akttttttda     aaottttna    aqttttttsa    aanttttqda    aaottttlaa                                              adssjgqtja  ",
	"  adssjgqtja    ajtrrtja      arqlilstia    alokikrtma      aakttra     aktommmmba    aaotpjjnma    akmmmmntpa    aktrigotoa    altqhirtha                                              amtkaaetqa  ",
	"  amtkaaetqa    acbamtja      afaaaajtoa    aaaaaaetqa      acsosra     aktjaaaaaa    ahtpaaaaaa    aaaaaamtka    aotgaaarsa    arsdaaftpa      aaaaa                                   aptcaaaqsa  ",
	"  aqtbaaaqtaa   aaaamtja      aaa  actpa         aasqa     aansdsra     aktjaaaa      anthaaaaa         aarsba    aptcaaarsa    atqaaaarsaa     adlia                                   altlaaftqa  ",
	"  arraaaaotga      amtja           aitna      aaaamtma     aftmasra     aktpqpkaa     aqsfoqohaa        agtpaa    aktmaahtpa    atqaaaartea     agtpa                                   abssljrtia  ",
	"  asqa  antia      amtja          aaqtga      aorstnaa    aapraasra     aktssttqaa    arsssrttja        antja     aantrqsqda    assaaadstha     afsoa                                   aahstttlaa  ",
	"  asqa  amtja      amtja         aantnaa      aorstqda    ajtjaasra     agiaafqtma    astrdaissaa      aarsaa     aanssstqea    aotoabpttia     aaaaa                     aaaaaaaa       aaaijdaa   ",
	"  asqa  antia      amtja        aaltpaa       aaaaltpa   aarpaaasraaa   aaaaaaetra    astjaaaotia      ahtoa      antnaaisqaa   adrttttqtga                               ahqqqqda         aaaaa    ",
	"  arraaaaotga      amtja       aajtrca           aaqsa   aitqppptspja        aarsa    artea amtka      aotia      arsaaaaotfa   aablongptba                               airrrrda                  ",
	"  aptcaaaqsaa      amtja      aahsreaa      aaa  aaqta   aisssssttsma   aaa  aassa    aotga antka     aassaa      asraaaantia    aaaaaasraa     aaaaa         aaaaa       aaaaaaaa                  ",
	"  altlaaftqa     aaamtjaaa    afssgaaaaa    abaaaaessa   aaaaaaasraaa   abaaaajtpa    aktmaaaqtea     aitoa       arseaaaqtfa   aaaaaamtma      ackha         aekga                                 ",
	"  abssljrtia     aooqtpona    aqtqooooma    aqpmknstma         asra     aqplknttia    aartmjotpaa     aotha       amtrkjotqaa   aiplkotraa      agtpa         aktna                                 ",
	"  aahstttlaa     attttttra    arttttttqa    apttttsmaa         asra     apttttskaa     agrtttpda      asraa       aaottttqfa    aittttpea       agtpa         aktna                                 ",
	"   aaaijdaa      aaaaaaaaa    aaaaaaaaaa    aadijibaa          aaaa     aadijhaaa      aaahjfaaa      aaaa         aadjjfaaa    aacijfaaa       aaaaa         aaaaa                                 ",
	"     aaaaa                                   aaaaaaa                     aaaaaa          aaaaa                      aaaaaa       aaaaaa                                                             ",
	"                                                                                                                                                                                                    ",
};


Img_c &ImageSet::IM_DigitFont_11x14()
{
	if (!digit_font_11x14)
	{
		digit_font_11x14 = IM_CreateFont(11*14, 14, digit_11x14_text,
										 digit_font_intensities, 20,
										 DIGIT_FONT_COLOR);
	}
	return digit_font_11x14.value();
}

Img_c &ImageSet::IM_DigitFont_14x19()
{
	if (!digit_font_14x19)
	{
		digit_font_14x19 = IM_CreateFont(14*14, 19, digit_14x19_text,
										 digit_font_intensities, 20,
										 DIGIT_FONT_COLOR);
	}
	return digit_font_14x19.value();
}

// this one applies the current gamma.
// for rendering the 3D view or the 2D sectors and sprites.
void Palette::decodePixel(img_pixel_t p, byte &r, byte &g, byte &b) const
{
	if(p & IS_RGB_PIXEL)
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
void Palette::decodePixelMedium(img_pixel_t p, byte &r, byte &g, byte &b) const noexcept
{
	if(p & IS_RGB_PIXEL)
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

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
