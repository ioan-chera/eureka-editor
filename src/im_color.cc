//------------------------------------------------------------------------
//  COLORS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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

#include "im_color.h"
#include "im_img.h"  // TRANS_PIXEL
#include "w_wad.h"


rgb_color_t palette[256];

int trans_replace;

byte raw_palette[256][3];
byte raw_colormap[32][256];


int usegamma = 2;

extern int gammatable[5][256];

byte bright_map[256];


void W_UpdateGamma()
{
	for (int c = 0 ; c < 256 ; c++)
	{
		byte r = raw_palette[c][0];
		byte g = raw_palette[c][1];
		byte b = raw_palette[c][2];

		r = gammatable[usegamma][r];
		g = gammatable[usegamma][g];
		b = gammatable[usegamma][b];
		
		palette[c] = fl_rgb_color(r, g, b);
	}
}


void W_LoadPalette()
{
	Lump_c *lump = W_FindLump("PLAYPAL");

	if (! lump)
	{
		FatalError("PLAYPAL lump not found.\n");
		return;
	}

	if (! lump->Seek() ||
		! lump->Read(raw_palette, sizeof(raw_palette)))
	{
		LogPrintf("PLAYPAL: read error\n");
		return;
	}

	// find the colour closest to TRANS_PIXEL
	byte tr = raw_palette[TRANS_PIXEL][0];
	byte tg = raw_palette[TRANS_PIXEL][1];
	byte tb = raw_palette[TRANS_PIXEL][2];

	trans_replace = W_FindPaletteColor(tr, tg, tb);

	W_UpdateGamma();

	W_CreateBrightMap();
}


void W_LoadColormap()
{
	Lump_c *lump = W_FindLump("COLORMAP");

	if (! lump)
	{
		FatalError("COLORMAP lump not found.\n");
		return;
	}

	if (! lump->Seek() ||
		! lump->Read(raw_colormap, sizeof(raw_colormap)))
	{
		LogPrintf("COLORMAP: read error\n");
		return;
	}
}


rgb_color_t DarkerColor(rgb_color_t col)
{
	int r = RGB_RED(col);
	int g = RGB_GREEN(col);
	int b = RGB_BLUE(col);

	return fl_rgb_color(r*2/3, g*2/3, b*2/3);
}


rgb_color_t LighterColor(rgb_color_t col)
{
	int r = RGB_RED(col);
	int g = RGB_GREEN(col);
	int b = RGB_BLUE(col);

	r = r * 13 / 16 + 48;
	g = g * 13 / 16 + 48;
	b = b * 13 / 16 + 48;

	return fl_rgb_color(r, g, b);
}


byte W_FindPaletteColor(int r, int g, int b)
{
	int best = 0;
	int best_dist = (1 << 30);

	for (int c = 0 ; c < 256 ; c++)
	{
		if (c == TRANS_PIXEL)
			continue;

		int dr = r - (int)raw_palette[c][0];
		int dg = g - (int)raw_palette[c][1];
		int db = b - (int)raw_palette[c][2];

		int dist = dr*dr + dg*dg + db*db;

		if (dist < best_dist)
		{
			best = c;
			best_dist = dist;
		}
	}

	return best;
}


void W_CreateBrightMap()
{
	for (int c = 0 ; c < 256 ; c++)
	{
		byte r = raw_palette[c][0];
		byte g = raw_palette[c][1];
		byte b = raw_palette[c][2];

		rgb_color_t col = LighterColor(fl_rgb_color(r, g, b));

		r = RGB_RED(col);
		g = RGB_GREEN(col);
		b = RGB_BLUE(col);

		bright_map[c] = W_FindPaletteColor(r, g, b);
	}
}


rgb_color_t ParseColor(const char *str)
{
	if (*str == '#')
		 str++;

	if (strlen(str) >= 6)  // long form #rrggbb
	{
		int number = (int)strtol(str, NULL, 16);

		int r = (number & 0xFF0000) >> 16;
		int g = (number & 0x00FF00) >> 8;
		int b = (number & 0x0000FF);

		return fl_rgb_color(r, g, b);
	}
	else  // short form: #rgb
	{
		int number = (int)strtol(str, NULL, 16);

		int r = (number & 0xF00) >> 8;
		int g = (number & 0x0F0) >> 4;
		int b = (number & 0x00F);

		return fl_rgb_color(r*17, g*17, b*17);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
