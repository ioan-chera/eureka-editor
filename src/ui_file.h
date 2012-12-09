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
	Fl_Input *map_name;

	Fl_Round_Button *kind_MAPxx;
	Fl_Round_Button *kind_ExMx;

	enum
	{
		ACT_none = 0,
		ACT_CANCEL,
		ACT_ACCEPT
	};

	bool action;

public:
	UI_ChooseMap();
	virtual ~UI_ChooseMap();

	const char * Run();

private:
	static void    ok_callback(Fl_Widget *, void *);
	static void close_callback(Fl_Widget *, void *);
};


#endif  /* __EUREKA_UI_FILE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
