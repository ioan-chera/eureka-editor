//------------------------------------------------------------------------
//  Information Bar (bottom of window)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2012 Andrew Apted
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

#ifndef __EUREKA_UI_PIC_H__
#define __EUREKA_UI_PIC_H__


class Img;


class UI_Pic : public Fl_Box
{
private:
	Fl_RGB_Image *rgb;

	bool unknown;

	bool selected;

public:
	UI_Pic(int X, int Y, int W, int H);
	virtual ~UI_Pic();

	// FLTK virtual method for drawing.
	int handle(int event);

public:
	void Clear();

	void GetFlat(const char * fname);
	void GetTex (const char * tname);
	void GetSprite(int type);

	bool Selected() { return selected; }
	void Selected(bool _val) { selected = _val; }

private:
	// FLTK virtual method for drawing.
	void draw();

	void draw_selected();

	void UploadRGB(const byte *buf, int depth);

	void TiledImg(Img *img, bool has_trans);
};


#endif  /* __EUREKA_UI_PIC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
