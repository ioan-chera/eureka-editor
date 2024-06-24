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

#include "Instance.h"

#include "main.h"
#include "ui_pic.h"
#include "ui_window.h"

#include "im_img.h"
#include "im_color.h"
#include "m_config.h"
#include "m_game.h"
#include "e_things.h"
#include "w_rawdef.h"
#include "w_texture.h"


//
// UI_Pic Constructor
//
UI_Pic::UI_Pic(Instance &inst, int X, int Y, int W, int H, const char *L) :
	Fl_Box(FL_BORDER_BOX, X, Y, W, H, ""),
	inst(inst)
{
	color(FL_DARK2);

	what_text = L;

	what_color = (config::gui_color_set == 1) ? (FL_GRAY0 + 4) : FL_BACKGROUND_COLOR;

	label(what_text.c_str());
	labelcolor(what_color);
	labelsize(16);

	align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
}

void UI_Pic::Clear() noexcept
{
	color(FL_DARK2);
	labelcolor(what_color);
	labelsize(16);

	label(what_text.c_str());

	rgb.reset();

	redraw();
}


void UI_Pic::MarkUnknown() noexcept
{
	Clear();

	color(FL_CYAN);
	labelcolor(FL_BLACK);
	labelsize(40);

	label("?");

	redraw();
}

void UI_Pic::MarkMissing()
{
	Clear();

	color(fl_rgb_color(255, 128, 0));
	labelcolor(FL_BLACK);
	labelsize(40);

	label("!");

	redraw();
}

void UI_Pic::MarkSpecial()
{
	Clear();

	color(fl_rgb_color(192, 0, 192));
	labelcolor(FL_WHITE);
	labelsize(40);

	label("?");

	redraw();
}


void UI_Pic::GetFlat(const SString & fname) noexcept
{
	const Img_c *img = inst.wad.images.W_GetFlat(inst.conf, fname, true /* try_uppercase */);

	TiledImg(img);
}


void UI_Pic::GetTex(const SString & tname)
{
	if (is_special_tex(tname))
	{
		MarkSpecial();
		return;
	}
	else if (is_null_tex(tname))
	{
		Clear();
		return;
	}

	const Img_c *img = inst.wad.images.getTexture(inst.conf, tname, true /* try_uppercase */);

	TiledImg(img);
}


void UI_Pic::GetSprite(int type, Fl_Color back_color)
{
	Clear();

	const Img_c *img = inst.wad.getSprite(inst.conf, type, inst.loaded, 1);
	tl::optional<Img_c> new_img;

	if (! img || img->width() < 1 || img->height() < 1)
	{
		MarkUnknown();
		return;
	}

	const thingtype_t &info = inst.conf.getThingType(type);



	uint32_t back = Fl::get_color(back_color);

	int iw = img->width();
	int ih = img->height();

	int nw = w();
	int nh = h();

	float scale = info.scale;

	float fit_x = iw * scale / (float)nw;
	float fit_y = ih * scale / (float)nh;

	// shrink if too big
	if (fit_x > 1.00 || fit_x > 1.00)
		scale = scale / std::max(fit_x, fit_y);

	// enlarge if too small
	if (fit_x < 0.4 && fit_y < 0.4)
		scale = scale / (2.5f * std::max(fit_x, fit_y));


	std::vector<uchar> buf;
	buf.resize(nw * nh * 3);

	for (int y = 0 ; y < nh ; y++)
	for (int x = 0 ; x < nw ; x++)
	{
		byte *dest = buf.data() + ((y * nw + x) * 3);

		// black border
		if (x == 0 || x == nw-1 || y == 0 || y == nh-1)
		{
			dest[0] = 0;
			dest[1] = 0;
			dest[2] = 0;
			continue;
		}

		int ix = static_cast<int>(x / scale - (nw / scale - iw) / 2);
		int iy = static_cast<int>(y / scale - (nh / scale - ih) / 2);

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
			inst.wad.palette.decodePixelMedium(pix, dest[0], dest[1], dest[2]);
		}
	}

	UploadRGB(std::move(buf), 3);
}


void UI_Pic::TiledImg(const Img_c *img) noexcept
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


	const uint32_t back = config::transparent_col;


	std::vector<uchar> buf;
	buf.resize(nw * nh * 3);

	for (int y = 0 ; y < nh ; y++)
	for (int x = 0 ; x < nw ; x++)
	{
		int ix = (x * scale) % iw;
		int iy = (y * scale) % ih;

		img_pixel_t pix = img->buf() [iy*iw+ix];

		byte *dest = buf.data() + ((y * nw + x) * 3);

		if (pix == TRANS_PIXEL)
		{
			dest[0] = RGB_RED(back);
			dest[1] = RGB_GREEN(back);
			dest[2] = RGB_BLUE(back);
		}
		else
		{
			inst.wad.palette.decodePixelMedium(pix, dest[0], dest[1], dest[2]);
		}
	}

	UploadRGB(std::move(buf), 3);
}


void UI_Pic::UploadRGB(std::vector<byte> &&buf, int depth) noexcept
{
	rgbBuffer = std::move(buf);
	rgb = std::make_unique<Fl_RGB_Image>(rgbBuffer.data(), w(), h(), depth, 0);

	// remove label
	label("");

	redraw();
}


//------------------------------------------------------------------------


int UI_Pic::handle(int event)
{
	switch (event)
	{
		case FL_ENTER:
			inst.main_win->SetCursor(FL_CURSOR_HAND);
			highlighted = true;
			redraw();
			return 1;

		case FL_LEAVE:
			if (highlighted)
			{
				inst.main_win->SetCursor(FL_CURSOR_DEFAULT);
				highlighted = false;
				redraw();
			}
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
	else if (Highlighted())
		draw_highlighted();
}


void UI_Pic::draw_highlighted()
{
	int X = x();
	int Y = y();
	int W = w();
	int H = h();

	fl_rect(X+0, Y+0, W-0, H-0, FL_YELLOW);
	fl_rect(X+1, Y+1, W-2, H-2, FL_YELLOW);
	fl_rect(X+2, Y+2, W-4, H-4, FL_BLACK);
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


void UI_Pic::Unhighlight()
{
	handle(FL_LEAVE);
}

//------------------------------------------------------------------------

int UI_DynInput::handle(int event)
{
	int res = Fl_Input::handle(event);

	if ((event == FL_KEYBOARD || event == FL_PASTE) && callback2_)
	{
		callback2_(this, data2_);
	}

	return res;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
