//------------------------------------------------------------------------
//  Miscellaneous UI Dialogs
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2016 Andrew Apted
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

#ifndef __EUREKA_UI_MISC_H__
#define __EUREKA_UI_MISC_H__

class UI_MoveDialog : public UI_Escapable_Window
{
private:
	Fl_Int_Input *delta_x;
	Fl_Int_Input *delta_y;
	Fl_Int_Input *delta_z;

	Fl_Button *ok_but;
	Fl_Button *cancel_but;

	bool want_close;

public:
	UI_MoveDialog(bool want_dz);
	virtual ~UI_MoveDialog();

	void Run();

private:
	static void    ok_callback(Fl_Widget *, void *);
	static void close_callback(Fl_Widget *, void *);
};


//------------------------------------------------------------------------

class UI_ScaleDialog : public UI_Escapable_Window
{
private:
	Fl_Input *scale_x;
	Fl_Input *scale_y;
	Fl_Input *scale_z;

	Fl_Choice *origin_x;
	Fl_Choice *origin_y;
	Fl_Choice *origin_z;

	Fl_Button *ok_but;
	Fl_Button *cancel_but;

	bool want_close;

public:
	UI_ScaleDialog();
	virtual ~UI_ScaleDialog();

	void Run();

private:
	static void    ok_callback(Fl_Widget *, void *);
	static void close_callback(Fl_Widget *, void *);
};


//------------------------------------------------------------------------

class UI_RotateDialog : public UI_Escapable_Window
{
private:
	Fl_Float_Input *angle;

	Fl_Choice *dir;
	Fl_Choice *origin;

	Fl_Button *ok_but;
	Fl_Button *cancel_but;

	bool want_close;

public:
	UI_RotateDialog();
	virtual ~UI_RotateDialog();

	void Run();

private:
	static void    ok_callback(Fl_Widget *, void *);
	static void close_callback(Fl_Widget *, void *);
};


//------------------------------------------------------------------------

class UI_JumpToDialog : public UI_Escapable_Window
{
private:
	Fl_Int_Input *input;

	Fl_Button *ok_but;
	Fl_Button *cancel_but;

	bool want_close;

	int limit;
	int result;

public:
	UI_JumpToDialog(const char *_objname, int _limit);
	virtual ~UI_JumpToDialog();

	// returns the typed number, or -1 if cancelled
	int Run();

private:
	static void    ok_callback(Fl_Widget *, void *);
	static void close_callback(Fl_Widget *, void *);
	static void input_callback(Fl_Widget *, void *);
};


#endif  /* __EUREKA_UI_MISC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
