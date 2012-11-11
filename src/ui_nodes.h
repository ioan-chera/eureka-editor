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
	Fl_Box *status;

	Fl_Progress *progress;

	Fl_Browser *browser;

	Fl_Button * but_OK;
	Fl_Button * but_Cancel;

	int cur_prog;

public:
	UI_NodeDialog();
	virtual ~UI_NodeDialog();

public:
	void SetStatus(const char *str);

	void SetProg(int perc);

	void Print(const char *str);

private:
	static void  close_callback(Fl_Widget *, void *);
	static void cancel_callback(Fl_Widget *, void *);
	static void     ok_callback(Fl_Widget *, void *);
};


#endif  /* __EUREKA_UI_NODES_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
