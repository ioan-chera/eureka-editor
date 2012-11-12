//------------------------------------------------------------------------
//  Node Building Window
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2012 Andrew Apted
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

#ifndef __EUREKA_UI_NODES_H__
#define __EUREKA_UI_NODES_H__


class UI_NodeDialog : public Fl_Double_Window
{
public:
	Fl_Browser *browser;

	Fl_Progress *progress;

	Fl_Button * button;

	int cur_prog;
	char prog_label[64];

	bool finished;

	bool want_cancel;
	bool want_close;

public:
	UI_NodeDialog();
	virtual ~UI_NodeDialog();

	/* FLTK method */
	int handle(int event);

public:
	void SetStatus(const char *str);

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


#endif  /* __EUREKA_UI_NODES_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
