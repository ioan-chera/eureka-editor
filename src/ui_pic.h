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

#include "ui_panelinput.h"
#include "FL/Fl_Input.H"
#include "FL/Fl_RGB_Image.H"
#include <memory>

class Img_c;


class UI_Pic : public Fl_Box
{
private:
	std::unique_ptr<Fl_RGB_Image> rgb;
	std::vector<byte> rgbBuffer;

	bool allow_hl = false;

	bool highlighted = false;
	bool selected = false;

	SString what_text;
	Fl_Color    what_color = {};

	Instance &inst;

public:
	UI_Pic(Instance &inst, int X, int Y, int W, int H, const char *L = "");

	// FLTK method for event handling
	int handle(int event);

public:
	void Clear();

	void MarkUnknown();
	void MarkMissing();
	void MarkSpecial();

	void GetFlat(const SString & fname);
	void GetTex (const SString & tname);
	void GetSprite(int type, Fl_Color back_color);

	void AllowHighlight(bool enable) { allow_hl = enable; redraw(); }
	bool Highlighted() const { return allow_hl && highlighted; }
	void Unhighlight();

	bool Selected() const { return selected; }
	void Selected(bool _val);

private:
	// FLTK virtual method for drawing.
	void draw();

	void draw_highlighted();
	void draw_selected();

	void UploadRGB(std::vector<byte> &&buf, int depth);

	void TiledImg(const Img_c *img);
};


//------------------------------------------------------------------------


class UI_DynInput : public Fl_Input, public ICallback2
{
	/* this widget provides a secondary callback which can be
	 * used to dynamically update a picture or description.
	 */

private:
	Fl_Callback *callback2_ = nullptr;
	void *data2_ = nullptr;

public:
	UI_DynInput(int X, int Y, int W, int H, const char *L = nullptr) : Fl_Input(X, Y, W, H, L)
	{
	}

	// FLTK method for event handling
	int handle(int event) override;

	// main callback is done on ENTER or RELEASE, but this
	// secondary callback is done on each change by the user.
	void callback2(Fl_Callback *cb, void *data) override
	{
		callback2_ = cb; data2_ = data;
	}
	Fl_Callback *callback2() const override
	{
		return callback2_;
	}
	void *user_data2() const override
	{
		return data2_;
	}
	const char *value() const
	{
		return Fl_Input::value();
	}

	ICALLBACK2_BOILERPLATE()
private:
	void value(const char *s)	// prevent direct editing
	{
		Fl_Input::value(s);
	}
};

#endif  /* __EUREKA_UI_PIC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
