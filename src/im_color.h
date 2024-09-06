//------------------------------------------------------------------------
//  COLORS
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

#ifndef __EUREKA_IM_COLOR_H__
#define __EUREKA_IM_COLOR_H__

#include "sys_macro.h"
#include "sys_type.h"
#include "WindowsSanitization.h"	// needed for Windows
#include <algorithm>

class Lump_c;
class SString;
struct WadData;

typedef uint32_t rgb_color_t;

#define RGB_RED(col)    ((col >> 24) & 255)
#define RGB_GREEN(col)  ((col >> 16) & 255)
#define RGB_BLUE(col)   ((col >>  8) & 255)

//
// Constexpr maker
//
static constexpr rgb_color_t rgbMake(int r, int g, int b)
{
	return static_cast<rgb_color_t>(r << 24 | g << 16 | b << 8);
}

// this is a version of rgb_color_t with an alpha channel
// [ currently only used by the TGA loading code ]
typedef uint32_t rgba_color_t;
#define RGBA_ALPHA(col)   ((col) & 255)
#define RGBA_MAKE(r, g, b, a)  (((r) << 24) | ((g) << 16) | ((b) << 8) | (a))

namespace config
{
extern int usegamma;
}

extern const int gammatable[5][256];

// make the color darker
rgb_color_t DarkerColor(rgb_color_t col);

rgb_color_t ParseColor(const SString &str);

rgb_color_t SectorLightColor(int light);

int HashedPalColor(const SString &name, const int *cols);

inline int R_DoomLightingEquation(int L, float dist)
{
	/* L in the range 0 to 256 */
	L >>= 2;

	int min_L = clamp(0, 36 - L, 31);

	int index = (59 - L) - int(1280 / std::max(1.0f, dist));

	/* result is colormap index (0 bright .. 31 dark) */
	return clamp(min_L, index, 31);
}


// this is a 16-bit value:
//   - when high bit is clear, it is a palette index 0-255
//     (value 255 is used to represent fully transparent).
//   - when high bit is set, the remainder is 5:5:5 RGB
typedef unsigned short  img_pixel_t;

//
// Wad palette info
//
class Palette
{
public:
	bool updateGamma(int usegamma, int panel_gamma);
	void decodePixel(img_pixel_t p, byte &r, byte &g, byte &b) const;
	void decodePixelMedium(img_pixel_t p, byte &r, byte &g, byte &b) const noexcept;
	void createBrightMap();

	rgb_color_t getPaletteColor(int index) const
	{
		return palette[index];
	}

	bool loadPalette(const Lump_c &lump, int usegamma, int panel_gamma);
	void loadColormap(const Lump_c *lump);

	byte findPaletteColor(int r, int g, int b) const;
	rgb_color_t pixelToRGB(img_pixel_t p) const;
	byte getColormapIndex(int cmap, int pos) const
	{
		return raw_colormap[cmap][pos];
	}

	int getTransReplace() const
	{
		return trans_replace;
	}

private:
	// this palette has the gamma setting applied
	rgb_color_t palette[256] = {};
	rgb_color_t palette_medium[256] = {};
	byte rgb555_gamma[32];
	byte rgb555_medium[32];
	byte bright_map[256] = {};
	byte raw_palette[256][3] = {};
	byte raw_colormap[32][256] = {};
	// the palette color closest to what TRANS_PIXEL really is
	int trans_replace = 0;

};

//------------------------------------------------------------//


#define BLACK           FL_BLACK
#define BLUE            FL_BLUE
#define GREEN           FL_GREEN
#define CYAN            FL_CYAN
#define RED             FL_RED
#define MAGENTA         FL_MAGENTA
#define BROWN           FL_DARK_RED
#define YELLOW          fl_rgb_color(255,255,0)
#define WHITE           FL_WHITE

#define LIGHTGREY       fl_rgb_color(144,144,144)
#define DARKGREY        fl_rgb_color(80,80,80)

#define LIGHTBLUE       fl_rgb_color(128,128,255)
#define LIGHTGREEN      fl_rgb_color(128,255,128)
#define LIGHTCYAN       fl_rgb_color(128,255,255)
#define LIGHTRED        fl_rgb_color(255,128,128)
#define LIGHTMAGENTA    fl_rgb_color(255,128,255)

#define OBJECT_NUM_COL  fl_rgb_color(0x44, 0xdd, 0xff)
#define CLR_ERROR       fl_rgb_color(0xff, 0,    0)

#define SECTOR_TAG      fl_rgb_color(0x00, 0xff, 0x00)
#define SECTOR_TAGTYPE  fl_rgb_color(0x00, 0xe0, 0xe0)
#define SECTOR_TYPE     fl_rgb_color(0x00, 0x80, 0xff)

#define SEL_COL         fl_rgb_color(128,192,255)
#define SEL3D_COL       fl_rgb_color(128,255,128)
#define HI_COL          fl_rgb_color(255,255,0)
#define HI_AND_SEL_COL  fl_rgb_color(255,128,0)

#define THING_MODE_COL  fl_rgb_color(255,64,255)
#define LINE_MODE_COL   fl_rgb_color(0,160,255)
#define SECTOR_MODE_COL fl_rgb_color(255,255,0)
#define VERTEX_MODE_COL fl_rgb_color(0,255,128)


#endif  /* __EUREKA_IM_COLOR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
