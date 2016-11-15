//------------------------------------------------------------------------
//  Information Bar (bottom of window)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
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


class Img_c;


class UI_Pic : public Fl_Box
{
private:
	Fl_RGB_Image *rgb;

	enum
	{
		SP_None = 0,
		SP_Unknown,   // texture name is not found
		SP_Missing,   // texture is '-' but should be present
	};

	int special;

	bool selected;

	const char *what_text;
	Fl_Color    what_color;

public:
	UI_Pic(int X, int Y, int W, int H, const char *L = "");
	virtual ~UI_Pic();

	// FLTK method for event handling
	int handle(int event);

public:
	void Clear();

	void MarkUnknown();
	void MarkMissing();

	void GetFlat(const char * fname);
	void GetTex (const char * tname);
	void GetSprite(int type, Fl_Color back_color);

	bool Selected() { return selected; }
	void Selected(bool _val);

private:
	// FLTK virtual method for drawing.
	void draw();

	void draw_selected();

	void UploadRGB(const byte *buf, int depth);

	void TiledImg(Img_c *img);
};


//------------------------------------------------------------------------


class UI_PicName : public Fl_Input
{
private:
	Fl_Callback *callback2_;
	void *data2_;

public:
	UI_PicName(int X, int Y, int W, int H, const char *L = NULL);
	virtual ~UI_PicName();

	// FLTK method for event handling
	int handle(int event);

	// main callback is done on ENTER or RELEASE, but this
	// secondary callback is done on each change by the user.
	void callback2(Fl_Callback *cb, void *data)
	{
		callback2_ = cb; data2_ = data;
	}
};

#endif  /* __EUREKA_UI_PIC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
