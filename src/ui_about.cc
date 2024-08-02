//------------------------------------------------------------------------
//  ABOUT WINDOW
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
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
#include "ui_window.h"
#include "ui_about.h"


#define ABOUT_W  (440)   // matches logo image
#define ABOUT_H  (230 + 290)


class UI_About : public UI_Escapable_Window
{
private:
	static UI_About * _instance;

	static Fl_RGB_Image *about_img;

	UI_About(int W, int H, const char *label = NULL);

	virtual ~UI_About()
	{
		// nothing needed
	}

	static void LoadImage()
	{
		static char filename[FL_PATH_MAX];

		// FIXME : use this location for all platforms
#ifdef WIN32
		snprintf(filename, sizeof(filename), "%s/common/about_logo.png", global::install_dir.u8string().c_str());
#else
		snprintf(filename, sizeof(filename), "%s/about_logo.png", global::install_dir.u8string().c_str());
#endif
		filename[FL_PATH_MAX-1] = 0;

		if (FileExists(filename))
		{
			about_img = new Fl_PNG_Image(filename);
		}
	}

public:
	static void Open()
	{
		if (_instance)  // already up?
			return;

		if (! about_img)
			LoadImage();

		_instance = new UI_About(ABOUT_W, ABOUT_H, "About Eureka v" EUREKA_VERSION);

		_instance->show();
	}

private:
	static void close_callback(Fl_Widget *w, void *data)
	{
		if (_instance)
		{
			_instance->default_callback(_instance, data);
			_instance = NULL;
		}
	}

	static const char *Text1;
	static const char *Text2;
	static const char *URL;
};


UI_About * UI_About::_instance;

Fl_RGB_Image * UI_About::about_img;


const char *UI_About::Text1 =
	"EUREKA is a map editor for classic DOOM\n"
	"It uses code from the Yadex editor";


const char *UI_About::Text2 =
	"Copyright (C) 2014-2024 Ioan Chera                \n"
	"Copyright (C) 2001-2020 Andrew Apted, et al\n"
	"Copyright (C) 1997-2003 André Majorel, et al\n"
	"\n"
	"This program is free software, and may be\n"
	"distributed and modified under the terms of\n"
	"the GNU General Public License\n"
	"\n"
	"There is ABSOLUTELY NO WARRANTY\n"
	"Use at your OWN RISK";


const char *UI_About::URL = "http://awwports.sf.net/eureka";


//
// Constructor
//
UI_About::UI_About(int W, int H, const char *label) :
    UI_Escapable_Window(W, H, label)
{
	// non-resizable
	size_range(W, H, W, H);

	callback(close_callback, this);


	// nice big logo image

	Fl_Box *box = new Fl_Box(FL_NO_BOX, 0, 0, W, 230, NULL);
	box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
	box->color(FL_BLACK, FL_BLACK);

	if (about_img)
	{
		box->image(about_img);

		// overlay a small version number in bottom right corner
		Fl_Box *v_box = new Fl_Box(FL_NO_BOX, 0, 0, W - 2, 230, "");

		v_box->label("v" EUREKA_VERSION);
		v_box->labelsize(20);
		v_box->labelcolor(fl_rgb_color(160, 255, 128));
		v_box->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT | FL_ALIGN_BOTTOM);
	}
	else
	{
		box->box(FL_FLAT_BOX);
		box->label("EUREKA\nDoom Editor\nv" EUREKA_VERSION);
		box->labelsize(40);
		box->labelcolor(fl_rgb_color(255, 200, 100));
	}


	int cy = 240;


	// the very informative text

	int pad = 26;

	box = new Fl_Box(FL_NO_BOX, pad, cy, W-pad-pad, 44, Text1);
	box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
	box->labelfont(FL_HELVETICA_BOLD);

	cy += box->h();


	box = new Fl_Box(FL_NO_BOX, pad, cy, W-pad-pad, 186, Text2);
	box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
	box->labelfont(FL_HELVETICA);

	cy += box->h();


#if 0
	// website address
	UI_HyperLink *link = new UI_HyperLink(10, cy, W-20, 30, URL, URL);
	link->align(FL_ALIGN_CENTER);
	link->labelsize(20);
	link->color(color());

	cy += link->h() + 16;
#endif


	// finally add an "OK" button

	int bw = 70;
	int bh = 33;

	cy += (H - cy - bh) / 2 - 6;

	Fl_Color but_color = fl_rgb_color(128, 208, 255);

	Fl_Button *button = new Fl_Button((W-10-bw)/2, cy, bw, bh, "OK!");
	button->color(but_color, but_color);
	button->callback(close_callback, this);


	end();
}


void DLG_AboutText(void)
{
	UI_About::Open();
}


#ifdef __APPLE__
static void about_callback_macosx(Fl_Widget *, void *)
{
	UI_About::Open();
}
#endif


void InitAboutDialog()
{
#ifdef __APPLE__
	fl_mac_set_about(about_callback_macosx, NULL);
#endif
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
