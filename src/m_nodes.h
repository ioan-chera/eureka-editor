//------------------------------------------------------------------------
//  BUILDING NODES / PLAY THE MAP
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2023 Andrew Apted
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

#pragma once
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Widget.H>

class UI_NodeDialog : public Fl_Double_Window
{
public:
	Fl_Browser *browser;

	Fl_Progress *progress;

	Fl_Button * button;

	int cur_prog = -1;
	char prog_label[64];

	bool finished = false;

	bool want_cancel = false;
	bool want_close = false;

public:
	UI_NodeDialog();

	/* FLTK method */
	int handle(int event);

public:
	void SetProg(int perc);

	void Print(const char *str);

	void Finish_OK();
	void Finish_Cancel();
	void Finish_Error();

	bool WantCancel() const { return want_cancel; }
	bool WantClose()  const { return want_close;  }

private:
	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
};
