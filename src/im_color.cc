//------------------------------------------------------------------------
//  COLORS
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

#include "Instance.h"

#include "im_color.h"
#include "m_config.h"


// config item
int config::usegamma = 2;

int config::panel_gamma = 2;

bool Palette::updateGamma(int usegamma, int panel_gamma)
{
	if(usegamma < 0 || panel_gamma < 0 || usegamma >= (int)lengthof(gammatable) ||
	   panel_gamma >= (int)lengthof(gammatable))
	{
		return false;
	}
	for (int c = 0 ; c < 256 ; c++)
	{
		byte r = raw_palette[c][0];
		byte g = raw_palette[c][1];
		byte b = raw_palette[c][2];

		byte r2 = static_cast<byte>(gammatable[usegamma][r]);
		byte g2 = static_cast<byte>(gammatable[usegamma][g]);
		byte b2 = static_cast<byte>(gammatable[usegamma][b]);

		palette[c] = rgbMake(r2, g2, b2);

		r2 = static_cast<byte>(gammatable[panel_gamma][r]);
		g2 = static_cast<byte>(gammatable[panel_gamma][g]);
		b2 = static_cast<byte>(gammatable[panel_gamma][b]);

		palette_medium[c] = rgbMake(r2, g2, b2);
	}

	for (int d = 0 ; d < 32 ; d++)
	{
		int i = d * 255 / 31;

		rgb555_gamma [d] = static_cast<byte>(gammatable[usegamma][i]);
		rgb555_medium[d] = static_cast<byte>(gammatable[panel_gamma][i]);
	}
	return true;
}

bool Palette::loadPalette(const Lump_c &lump, int usegamma, int panel_gamma)
{
	if(lump.getData().size() < sizeof(raw_palette))
	{
		gLog.printf("PLAYPAL: read error\n");
		return false;
	}
	memcpy(raw_palette, lump.getData().data(), sizeof(raw_palette));
	
	// find the colour closest to TRANS_PIXEL
	byte tr = raw_palette[TRANS_PIXEL][0];
	byte tg = raw_palette[TRANS_PIXEL][1];
	byte tb = raw_palette[TRANS_PIXEL][2];

	trans_replace = findPaletteColor(tr, tg, tb);

	if(!updateGamma(usegamma, panel_gamma))
		return false;

	createBrightMap();

	return true;
}


void Palette::loadColormap(const Lump_c *lump)
{
	if (! lump)
	{
		ThrowException("COLORMAP lump not found.\n");
		return;
	}
	
	LumpInputStream stream(*lump);

	if (! stream.read(raw_colormap, sizeof(raw_colormap)))
	{
		gLog.printf("COLORMAP: read error\n");
		return;
	}

	// ensure colormap does not transparent pixel
	for (int i = 0 ; i < 32  ; i++)
	for (int c = 0 ; c < 256 ; c++)
	{
		if (raw_colormap[i][c] == TRANS_PIXEL)
			raw_colormap[i][c] = static_cast<byte>(trans_replace);
	}

	// workaround for Harmony having a bugged colormap
	if (raw_palette[0][0] == 0 &&
		raw_palette[0][1] == 0 &&
		raw_palette[0][2] == 0)
	{
		for (int k = 0 ; k < 32 ; k++)
			raw_colormap[k][0] = 0;
	}
}


rgb_color_t DarkerColor(rgb_color_t col)
{
	int r = RGB_RED(col);
	int g = RGB_GREEN(col);
	int b = RGB_BLUE(col);

	return fl_rgb_color(static_cast<uchar>(r*2/3), static_cast<uchar>(g*2/3),
                        static_cast<uchar>(b*2/3));
}


static rgb_color_t LighterColor(rgb_color_t col)
{
	int r = RGB_RED(col);
	int g = RGB_GREEN(col);
	int b = RGB_BLUE(col);

	r = r * 13 / 16 + 48;
	g = g * 13 / 16 + 48;
	b = b * 13 / 16 + 48;

	return rgbMake(r, g, b);
}


byte Palette::findPaletteColor(int r, int g, int b) const
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

	return static_cast<byte>(best);
}


void Palette::createBrightMap()
{
	for (int c = 0 ; c < 256 ; c++)
	{
		byte r = raw_palette[c][0];
		byte g = raw_palette[c][1];
		byte b = raw_palette[c][2];

		rgb_color_t col = LighterColor(rgbMake(r, g, b));

		r = RGB_RED(col);
		g = RGB_GREEN(col);
		b = RGB_BLUE(col);

		bright_map[c] = findPaletteColor(r, g, b);
	}
}


rgb_color_t ParseColor(const SString &cstr)
{
	SString str(cstr);
	if(str[0] == '#')
		str.erase(0, 1);

	if (str.length() >= 6)  // long form #rrggbb
	{
		int number = (int)strtol(str, NULL, 16);

		int r = (number & 0xFF0000) >> 16;
		int g = (number & 0x00FF00) >> 8;
		int b = (number & 0x0000FF);

		return fl_rgb_color(static_cast<uchar>(r), static_cast<uchar>(g), static_cast<uchar>(b));
	}
	else  // short form: #rgb
	{
		int number = (int)strtol(str, NULL, 16);

		int r = (number & 0xF00) >> 8;
		int g = (number & 0x0F0) >> 4;
		int b = (number & 0x00F);

		return fl_rgb_color(static_cast<uchar>(r*17), static_cast<uchar>(g*17),
                            static_cast<uchar>(b*17));
	}
}


rgb_color_t SectorLightColor(int light)
{
	int lt = light;

	// sample three distances, produce average
	lt = R_DoomLightingEquation(light,  60.0) +
		 R_DoomLightingEquation(light, 160.0) +
		 R_DoomLightingEquation(light, 560.0);

	lt = (93 - lt) * 255 / 93;

	// need to gamma-correct the light level
	if (config::usegamma > 0)
		lt = gammatable[config::usegamma][lt];

	return rgbMake(lt, lt, lt);
}


int HashedPalColor(const SString &name, const int *cols)
{
	// cols is array of two elements: start and end color.
	// this returns a palette color somewhere between start
	// and end, depending on the texture name.

	int len = (int)name.length();

	int hash = name[0]*41;

	if (len >= 2) hash += name[2]*13;
	if (len >= 4) hash += name[4]*17;
	if (len >= 5) hash += name[5]*7;
	if (len >= 7) hash += name[7]*3;

	hash ^= (hash >> 5);

	int c1 = cols[0];
	int c2 = cols[1];

	if (c1 > c2)
		std::swap(c1, c2);

	if (c1 == c2)
		return c1;

	return c1 + hash % (c2 - c1 + 1);
}


//------------------------------------------------------------------------


const int gammatable[5][256] =
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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
