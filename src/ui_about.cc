//------------------------------------------------------------------------
//  About Window
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2013 Andrew Apted
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


class UI_About : public Fl_Window
{
private:
  static UI_About * _instance;

  UI_About(int W, int H, const char *label = NULL);

  virtual ~UI_About()
  {
    // nothing needed
  }

public:
  static void Open()
  {
    if (_instance)  // already up?
      return;

    int about_w = 326 + KF * 30;
    int about_h = 330 + KF * 40;

    _instance = new UI_About(about_w, about_h, "About Eureka");

    _instance->show();
  }

  // FLTK virtual method for handling input events.
  int handle(int event)
  {
    if (event == FL_KEYDOWN || event == FL_SHORTCUT)
    {
      int key = Fl::event_key();

      if (key == FL_Escape)
      {
        callback_Close(this, this);
        return 1;
      }

      // eat all other function keys
      if (FL_F+1 <= key && key <= FL_F+12)
        return 1;
    }

    return Fl_Window::handle(event);
  }

private:
  static void callback_Close(Fl_Widget *w, void *data)
  {
    if (_instance)
    {
      _instance->default_callback(_instance, data);
      _instance = NULL;
    }
  }

  static const char *Logo;
  static const char *Text1;
  static const char *Text2;
  static const char *URL;
};


UI_About * UI_About::_instance;


const char *UI_About::Logo = EUREKA_TITLE "\n v" EUREKA_VERSION;

const char *UI_About::Text1 =
  "EUREKA is a map editor for DOOM 1 and 2\n"
  "EUREKA uses code from the Yadex editor.";


const char *UI_About::Text2 =
  "Copyright (C) 2001-2013 Andrew Apted       \n"
  "Copyright (C) 1997-2003 André Majorel et al\n"
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
    Fl_Window(W, H, label)
{
  // non-resizable
  size_range(W, H, W, H);
  callback(callback_Close, this);

  int cy = 22;


  // nice big logo text
  Fl_Box *box = new Fl_Box(0, cy, W, 70, Logo);
  box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
  box->labelcolor(FL_BLUE);
  box->labelsize(24);

  cy += box->h() + 10;
  

  // the very informative text

  int pad = 20 + KF * 6;

  box = new Fl_Box(FL_NO_BOX, pad, cy, W-pad-pad, 42, Text1);
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
  int bw = 60 + KF * 10;
  int bh = 30 + KF * 3;

  cy += (H - cy - bh) / 2 - 8;

  Fl_Color but_color = fl_rgb_color(128, 128, 255);

  Fl_Button *button = new Fl_Button((W-10-bw)/2, cy, bw, bh, "OK!");
  button->color(but_color, but_color);
  button->callback(callback_Close, this);


  end();
}


void DLG_AboutText(void)
{
  UI_About::Open();
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
