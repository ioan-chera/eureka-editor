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

	Fl_Return_Button *ok_but;

	enum
	{
		ACT_none = 0,
		ACT_CANCEL,
		ACT_ACCEPT
	};

	int action;

	void CheckMapName();

public:
	UI_ChooseMap(const char *initial_name = "");
	virtual ~UI_ChooseMap();

	// format is 'E' for ExMx, or 'M' for MAPxx
	void PopulateButtons(char format, Wad_file *test_wad = NULL);

	const char * Run();

private:
	static void     ok_callback(Fl_Widget *, void *);
	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
	static void  input_callback(Fl_Widget *, void *);
};

//------------------------------------------------------------------------

class UI_OpenMap : public Fl_Double_Window
{
private:
	Fl_Round_Button *look_iwad;
	Fl_Round_Button *look_res;
	Fl_Round_Button *look_pwad;

	Fl_Output *pwad_name;
	Fl_Input  *map_name;

	Fl_Group *button_grp;

	Fl_Return_Button *ok_but;

	enum
	{
		ACT_none = 0,
		ACT_CANCEL,
		ACT_ACCEPT
	};

	int action;

	Wad_file * result_wad;
	Wad_file * new_pwad;

	void Populate();
	void PopulateButtons(Wad_file *wad);

	void LoadFile();
	void SetPWAD(const char *name);
	void CheckMapName();

public:
	UI_OpenMap();
	virtual ~UI_OpenMap();

	// the 'wad' result will be NULL when cancelled.
	// when OK and 'is_new_pwad' is set, the wad should become the edit_wad
	void Run(Wad_file ** wad_v, bool * is_new_pwad, const char ** map_v);

private:
	static void     ok_callback(Fl_Widget *, void *);
	static void  close_callback(Fl_Widget *, void *);
	static void   look_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
	static void  input_callback(Fl_Widget *, void *);
	static void   load_callback(Fl_Widget *, void *);
};

//------------------------------------------------------------------------

class UI_ProjectSetup : public Fl_Double_Window
{
public:
	enum
	{
		RES_NUM = 4
	};

private:
	Fl_Choice *iwad_choice;
	Fl_Choice *port_choice;

	Fl_Output *res_name[RES_NUM];

	Fl_Button *ok_but;
	Fl_Button *cancel;

	enum
	{
		ACT_none = 0,
		ACT_CANCEL,
		ACT_ACCEPT
	};

	int action;

	static UI_ProjectSetup * _instance;  // meh!

	static void   iwad_callback(Fl_Choice*, void*);
	static void   port_callback(Fl_Choice*, void*);
	static void browse_callback(Fl_Button*, void*);

	static void  kill_callback(Fl_Button*, void*);
	static void  load_callback(Fl_Button*, void*);

	static void close_callback(Fl_Widget*, void*);
	static void   use_callback(Fl_Button*, void*);

	void Populate();
	void PopulateIWADs(const char *curr_iwad);

public:
	/*
	 * current state
	 */
	const char * iwad;
	const char * port;
	const char * res[RES_NUM];

public:
	UI_ProjectSetup(bool is_startup = false);
	virtual ~UI_ProjectSetup();

	// returns true if something changed
	bool Run();
};

#endif  /* __EUREKA_UI_FILE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
