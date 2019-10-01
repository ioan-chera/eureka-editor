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

#include "main.h"

#include "im_img.h"
#include "m_game.h"


static int missing_tex_color;
static int unknown_tex_color;
static int unknown_flat_color;
static int unknown_sprite_color;

static Img_c * missing_tex_image;
static Img_c * unknown_tex_image;
static Img_c * unknown_flat_image;
static Img_c * unknown_sprite_image;


inline rgb_color_t IM_PixelToRGB(img_pixel_t p)
{
	if (p & IS_RGB_PIXEL)
	{
		byte r = IMG_PIXEL_RED(p)   << 3;
		byte g = IMG_PIXEL_GREEN(p) << 3;
		byte b = IMG_PIXEL_BLUE(p)  << 3;

		return RGB_MAKE(r, g, b);
	}
	else
	{
		byte r = raw_palette[p][0];
		byte g = raw_palette[p][1];
		byte b = raw_palette[p][2];

		return RGB_MAKE(r, g, b);
	}
}


//
// default constructor, creating a null image
//
Img_c::Img_c() : pixels(NULL), w(0), h(0), gl_tex(0)
{ }


//
// a constructor with dimensions
//
Img_c::Img_c(int width, int height, bool _dummy) :
	pixels(NULL), w(0), h(0), gl_tex(0)
{
	resize(width, height);
}


//
// destructor
//
Img_c::~Img_c()
{
	delete []pixels;
}


//
//  return a const pointer on the buffer.
//  if the image is null, return a NULL pointer.
//
const img_pixel_t *Img_c::buf() const
{
	return pixels;
}


//
// return a writable pointer on the buffer.
// if the image is null, return a NULL pointer.
//
img_pixel_t *Img_c::wbuf()
{
	return pixels;
}


//
// clear the image to fully transparent
//
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


//
// resize the image.  if either dimension is zero,
// the image becomes a null image.
//
void Img_c::resize(int new_width, int new_height)
{
	if (new_width == w && new_height == h)
		return;

	// unallocate old buffer
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


//
// make a game image look vaguely like a spectre
//
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


//
//  scale a game image, returning a new one.
//
//  the implementation is very simplistic and not optimized.
//
//  andrewj: turned into a method, but untested...
//
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


//
// copy the image, remapping pixels in the range 'src1..src2' to the
// range 'targ1..targ2'.
//
// TODO : make it work with RGB pixels (find nearest in palette).
//
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
			const rgb_color_t col = IM_PixelToRGB(pix);

			byte r = RGB_RED(col)   >> 3;
			byte g = RGB_GREEN(col) >> 3;
			byte b = RGB_BLUE(col)  >> 3;

			src[y * W + x] = IMG_PIXEL_MAKE_RGB(r, g, b);
		}
	}
}


void Img_c::upload_gl()
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// original texture ID is overwritten.

	// NOTE: we cannot use glDeleteTextures() since the context
	//       has very likely changed and we would end up deleting
	//       the wrong images.

	glGenTextures(1, &gl_tex);
	glBindTexture(GL_TEXTURE_2D, gl_tex);

	// construct a power-of-two sized bottom-up RGBA image
	int tw = RoundPOW2(w);
	int th = RoundPOW2(h);

	byte *rgba = new byte[tw * th * 4];

	memset(rgba, 0, (size_t)(tw * th * 4));

	bool has_trans = has_transparent();

	int ex = (has_trans ? w : tw) - 1;
	int ey = (has_trans ? h : th) - 1;

	int x, y;

	for (y = 0; y < ey ; y++)
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

			// TODO convert pixel to RGBA
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0 /* border */,
		GL_RGBA, GL_UNSIGNED_BYTE, rgba);

	delete[] rgba;
}


//------------------------------------------------------------------------


void IM_ResetDummyTextures()
{
	missing_tex_color  = -1;
	unknown_tex_color  = -1;
	unknown_flat_color = -1;
	unknown_sprite_color = -1;
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


Img_c * IM_UnknownSprite()
{
	if (! unknown_sprite_image || unknown_sprite_color != game_info.unknown_tex)
	{
		unknown_sprite_color = game_info.unknown_tex;

		if (unknown_sprite_image)
			delete unknown_sprite_image;

		unknown_sprite_image = new Img_c(64, 64, true);

		img_pixel_t *obuf = unknown_sprite_image->wbuf();

		for (int y = 0 ; y < 64 ; y++)
		for (int x = 0 ; x < 64 ; x++)
		{
			obuf[y * 64 + x] = unknown_graphic[(y/4) * 16 + (x/4)] ? unknown_sprite_color : TRANS_PIXEL;
		}
	}

	return unknown_sprite_image;
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


Img_c * IM_ConvertRGBImage(Fl_RGB_Image *src)
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


Img_c * IM_ConvertTGAImage(const rgba_color_t * data, int W, int H)
{
	Img_c *img = new Img_c(W, H);

	img_pixel_t *dest = img->wbuf();

	for (int i = W * H ; i > 0 ; i--, data++, dest++)
	{
		if (RGBA_ALPHA(*data) & 128)
		{
			byte r = RGB_RED(  *data) >> 3;
			byte g = RGB_GREEN(*data) >> 3;
			byte b = RGB_BLUE( *data) >> 3;

			*dest = IMG_PIXEL_MAKE_RGB(r, g, b);
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
const char * arrow_0_xpm[] =
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
const char * arrow_45_xpm[] =
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
const char * arrow_90_xpm[] =
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
const char * arrow_135_xpm[] =
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
const char * arrow_180_xpm[] =
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
const char * arrow_225_xpm[] =
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
const char * arrow_270_xpm[] =
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
const char * arrow_315_xpm[] =
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


static const char *dog_image_text[] =
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


Img_c * IM_CreateDogSprite()
{
	return IM_CreateFromText(44, 26, dog_image_text, dog_palette, 7);
}


//------------------------------------------------------------------------

Img_c * IM_CreateLightSprite()
{
	int W = 11;
	int H = 11;

	Img_c *result = new Img_c(W, H);

	result->clear();

	for (int y = 0 ; y < H ; y++)
	for (int x = 0 ; x < W ; x++)
	{
		byte pix = TRANS_PIXEL;

		if (true) // x > 0 && x < W-1 && y > 0 && y < H-1)
		{
			float dx = (W - 2*x) / (float)W;
			float dy = (H - 2*y) / (float)H;

			float dist = sqrt((dx) * (dx) + (dy) * (dy));

			float ity = 1.0 / (dist + 0.5) / (dist + 0.5);

			if (ity < 0.5)
				continue;

			ity = (ity - 0.4) / (1.0 - 0.4);

			int r = 255 * ity;
			int g = 235 * ity;
			int b = 90  * ity;

			pix = W_FindPaletteColor(r, g, b);
		}

		result->wbuf() [ y * W + x ] = pix;
	}

	return result;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
