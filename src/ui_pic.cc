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

#include "main.h"
#include "ui_pic.h"
#include "ui_window.h"

#include "im_img.h"
#include "im_color.h"
#include "m_config.h"
#include "m_game.h"
#include "e_things.h"
#include "w_rawdef.h"

#include "w_flats.h"
#include "w_sprite.h"
#include "w_texture.h"


//
// UI_Pic Constructor
//
UI_Pic::UI_Pic(int X, int Y, int W, int H, const char *L) :
    Fl_Box(FL_BORDER_BOX, X, Y, W, H, ""),
    rgb(NULL), special(SP_None), selected(false)
{
	color(FL_DARK2);

	what_text = StringDup(L);

	what_color = (gui_color_set == 1) ? (FL_GRAY0 + 4) : FL_BACKGROUND_COLOR;

	label(what_text);
	labelcolor(what_color);
	labelsize(16);

	align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
}

//
// UI_Pic Destructor
//
UI_Pic::~UI_Pic()
{
	StringFree(what_text);
	what_text = NULL;
}


void UI_Pic::Clear()
{
	special = SP_None;

	color(FL_DARK2);
	labelcolor(what_color);
	labelsize(16);

	label(what_text);

	if (rgb)
	{
		delete rgb; rgb = NULL;
	}

	redraw();
}


void UI_Pic::MarkUnknown()
{
	Clear();

	special = SP_Unknown;

	color(FL_CYAN);
	labelcolor(FL_BLACK);
	labelsize(40);

	label("?");

	redraw();
}

void UI_Pic::MarkMissing()
{
	Clear();

	special = SP_Missing;

	color(fl_rgb_color(255, 128, 0));
	labelcolor(FL_BLACK);
	labelsize(40);

	label("!");

	redraw();
}


void UI_Pic::GetFlat(const char * fname)
{
	Img_c *img = W_GetFlat(fname, true /* try_uppercase */);

	TiledImg(img);
}


void UI_Pic::GetTex(const char * tname)
{
	if (is_null_tex(tname))
	{
		Clear();
		return;
	}

	Img_c *img = W_GetTexture(tname, true /* try_uppercase */);

	TiledImg(img);
}


void UI_Pic::GetSprite(int type, Fl_Color back_color)
{
	//  color(FL_GRAY0 + 2);

	Clear();

	Img_c *img = W_GetSprite(type);

	if (! img || img->width() < 1 || img->height() < 1)
	{
		MarkUnknown();
		return;
	}

	const thingtype_t *info = M_GetThingType(type);

	bool new_img = false;

	if (info->flags & THINGDEF_INVIS)
	{
		img = img->spectrify();
		new_img = true;
	}


	u32_t back = Fl::get_color(back_color);

	int iw = img->width();
	int ih = img->height();

	int nw = w();
	int nh = h();

	int scale = 1;

	if (iw*3 < nw && ih*3 < nh)
		scale = 2;


	uchar *buf = new uchar[nw * nh * 3];

	for (int y = 0 ; y < nh ; y++)
	for (int x = 0 ; x < nw ; x++)
	{
		byte *dest = buf + ((y * nw + x) * 3);

		// black border
		if (x == 0 || x == nw-1 || y == 0 || y == nh-1)
		{
			dest[0] = 0;
			dest[1] = 0;
			dest[2] = 0;
			continue;
		}

		int ix = x / scale - (nw / scale - iw) / 2;
		//  int iy = (ih-1) - (nh-4 - y);
		int iy = y / scale - (nh / scale - ih) / 2;

		img_pixel_t pix = TRANS_PIXEL;

		if (ix >= 0 && ix < iw && iy >= 0 && iy < ih)
		{
			pix = img->buf() [iy * iw + ix];
		}

		if (pix == TRANS_PIXEL)
		{
			dest[0] = RGB_RED(back);
			dest[1] = RGB_GREEN(back);
			dest[2] = RGB_BLUE(back);
		}
		else
		{
			IM_DecodePixel(pix, dest[0], dest[1], dest[2]);
		}
	}

	UploadRGB(buf, 3);

	if (new_img)
		delete img;
}


void UI_Pic::TiledImg(Img_c *img)
{
	color(FL_DARK2);

	Clear();

	if (! img || img->width() < 1 || img->height() < 1)
	{
		MarkUnknown();
		return;
	}


	int iw = img->width();
	int ih = img->height();

	int nw = w();
	int nh = h();

	int scale = 1;

	while (nw*scale < iw || nh*scale < ih)
		scale = scale * 2;


	const u32_t back = RGB_MAKE(255, 255, 255);  // CYAN


	uchar *buf = new uchar[nw * nh * 3];

	for (int y = 0 ; y < nh ; y++)
	for (int x = 0 ; x < nw ; x++)
	{
		int ix = (x * scale) % iw;
		int iy = (y * scale) % ih;

		img_pixel_t pix = img->buf() [iy*iw+ix];

		byte *dest = buf + ((y * nw + x) * 3);

		if (pix == TRANS_PIXEL)
		{
			dest[0] = RGB_RED(back);
			dest[1] = RGB_GREEN(back);
			dest[2] = RGB_BLUE(back);
		}
		else
		{
			IM_DecodePixel(pix, dest[0], dest[1], dest[2]);
		}
	}

	UploadRGB(buf, 3);
}


void UI_Pic::UploadRGB(const byte *buf, int depth)
{
	rgb = new Fl_RGB_Image(buf, w(), h(), depth, 0);

	// HACK ALERT: make the Fl_RGB_Image class think it allocated
	//             the buffer, so that it will get freed properly
	//             by the Fl_RGB_Image destructor.
	rgb->alloc_array = true;

	// remove label
	label("");

	special = SP_None;

	redraw();
}


//------------------------------------------------------------------------


int UI_Pic::handle(int event)
{
	switch (event)
	{
		case FL_ENTER:
			main_win->SetCursor(FL_CURSOR_HAND);
			return 1;

		case FL_LEAVE:
			main_win->SetCursor(FL_CURSOR_DEFAULT);
			return 1;

		case FL_PUSH:
			do_callback();
			return 1;

		default:
			break;
	}

	return 0;  // unused
}


void UI_Pic::draw()
{
	if (rgb)
		rgb->draw(x(), y());
	else
		Fl_Box::draw();

	if (selected)
		draw_selected();
}


void UI_Pic::draw_selected()
{
	int X = x();
	int Y = y();
	int W = w();
	int H = h();

	fl_rect(X+0, Y+0, W-0, H-0, FL_RED);
	fl_rect(X+1, Y+1, W-2, H-2, FL_RED);
	fl_rect(X+2, Y+2, W-4, H-4, FL_BLACK);
}


void UI_Pic::Selected(bool _val)
{
	if (selected != _val)
		redraw();

	selected = _val;
}


//------------------------------------------------------------------------


UI_PicName::UI_PicName(int X, int Y, int W, int H, const char *L) :
	Fl_Input(X, Y, W, H, L),
	callback2_(NULL), data2_(NULL)
{ }


UI_PicName::~UI_PicName()
{ }


int UI_PicName::handle(int event)
{
	// FIXME

	int res = Fl_Input::handle(event);

	return res;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
