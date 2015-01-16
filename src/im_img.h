//------------------------------------------------------------------------
//  IMAGES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2015 Andrew Apted
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

typedef byte  img_pixel_t;

class Img_priv;


/* The colour number used to represent transparent pixels in an Img.
   Any value will do but zero is probably best performance-wise.
 */
const img_pixel_t TRANS_PIXEL = 247;


class Img
{
private:
	img_pixel_t *pixels;

	int  w;  // Width
	int  h;  // Height

public:
	 Img();
	 Img(int width, int height, bool _dummy = false);
	~Img();

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

	// read access
	const img_pixel_t *buf() const;

	// read/write access
	img_pixel_t *wbuf();

public:
	// set all pixels to TRANS_PIXEL
	void clear();

	void resize(int new_width, int new_height);

	Img * spectrify() const;

	Img * scale_img(double scale) const;

	Img * color_remap(int src1, int src2, int targ1, int targ2) const;

private:
	Img            (const Img&);  // Too lazy to implement it
	Img& operator= (const Img&);  // Too lazy to implement it
};


Img * IM_CreateUnknownTex(int bg, int fg);
Img * IM_CreateMissingTex(int bg, int fg);


#endif  /* __EUREKA_IM_IMG_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
