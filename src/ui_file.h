//------------------------------------------------------------------------
//  File-related dialogs
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012 Andrew Apted
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

#ifndef __EUREKA_UI_FILE_H__
#define __EUREKA_UI_FILE_H__

class UI_ChooseMap : public Fl_Double_Window
{
private:
	Fl_Int_Input *delta_x;
	Fl_Int_Input *delta_y;

	Fl_Button *ok_but;
	Fl_Button *cancel_but;

	bool want_close;

public:
	UI_ChooseMap();
	virtual ~UI_ChooseMap();

	void Run();

	bool WantClose() { return want_close; }

private:
	static void    ok_callback(Fl_Widget *, void *);
	static void close_callback(Fl_Widget *, void *);
};


#endif  /* __EUREKA_UI_FILE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
