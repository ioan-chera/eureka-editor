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

#include "main.h"
#include "ui_pic.h"
#include "ui_window.h"

#include "im_img.h"
#include "im_color.h"
#include "m_game.h"
#include "e_things.h"
#include "w_rawdef.h"

#include "w_flats.h"
#include "w_sprite.h"
#include "w_texture.h"


//
// UI_Pic Constructor
//
UI_Pic::UI_Pic(int X, int Y, int W, int H) :
    Fl_Box(FL_BORDER_BOX, X, Y, W, H, ""),
    rgb(NULL), unknown(false), selected(false)
{
	color(FL_DARK2);
	labelcolor(FL_LIGHT2);

	align(FL_ALIGN_INSIDE);
}

//
// UI_Pic Destructor
//
UI_Pic::~UI_Pic()
{
}


void UI_Pic::Clear()
{
	if (rgb)
	{
		delete rgb; rgb = NULL;
		label("");
		redraw();
	}
}


void UI_Pic::GetFlat(const char * fname)
{
	TiledImg(W_GetFlat(fname), false /* has_trans */);
}

void UI_Pic::GetTex(const char * tname)
{
	TiledImg(W_GetTexture(tname), true /* has_trans */);
}


void UI_Pic::GetSprite(int type)
{
	//  color(FL_GRAY0 + 2);

	if (rgb)
		Clear();

	Img *img = W_GetSprite(type);

	if (! img || img->width() < 1 || img->height() < 1)
		return;

	const thingtype_t *info = M_GetThingType(type);

	bool new_img = false;

	if (info->flags & THINGDEF_INVIS)
	{
		img = img->spectrify();
		new_img = true;
	}


	u32_t back_col = Fl::get_color(color());

	int iw = img->width();
	int ih = img->height();

	int nw = w();
	int nh = h();

	int scale = 1;

	if (iw*3 < nw && ih*3 < nh)
		scale = 2;

	///---  if (iw*2 < nw+4 && ih*2 < nh+8)
	///---    scale = 2;
	///---  if (iw*4 < nw && ih*4 < nh)
	///---    scale = 3;


	uchar *buf = new uchar[nw * nh * 3];

	for (int y = 0; y < nh; y++)
	for (int x = 0; x < nw; x++)
	{
		int ix = x / scale - (nw / scale - iw) / 2;
		//  int iy = (ih-1) - (nh-4 - y);
		int iy = y / scale - (nh / scale - ih) / 2;

		u32_t col = back_col;

		if (ix >= 0 && ix < iw && iy >= 0 && iy < ih)
		{
			img_pixel_t pix = img->buf() [iy*iw+ix];

			if (pix != TRANS_PIXEL)
				col = palette[pix];
		}

		// Black border
		if (x == 0 || x == nw-1 || y == 0 || y == nh-1)
			col = 0;

		byte *dest = buf + ((y*nw+x) * 3);

		dest[0] = RGB_RED(col);
		dest[1] = RGB_GREEN(col);
		dest[2] = RGB_BLUE(col);
	}

	UploadRGB(buf, 3);

	if (new_img)
		delete img;
}


void UI_Pic::TiledImg(Img *img, bool has_trans)
{
	color(FL_DARK2);

	if (rgb)
		Clear();

	if (! img || img->width() < 1 || img->height() < 1)
		return;


	int iw = img->width();
	int ih = img->height();

	int nw = w();
	int nh = h();

	int scale = 1;

	while (nw*scale < iw || nh*scale < ih)
		scale = scale * 2;


	const u32_t back_col = 0x00FFFF00; // CYAN

	uchar *buf = new uchar[nw * nh * 3];

	for (int y = 0 ; y < nh ; y++)
	for (int x = 0 ; x < nw ; x++)
	{
		int ix = (x * scale) % iw;
		int iy = (y * scale) % ih;

		img_pixel_t pix = img->buf() [iy*iw+ix];

		u32_t col = back_col;

		if (! (has_trans && pix == TRANS_PIXEL))
			col = palette[pix];

		byte *dest = buf + ((y*nw+x) * 3);

		dest[0] = RGB_RED(col);
		dest[1] = RGB_GREEN(col);
		dest[2] = RGB_BLUE(col);
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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
