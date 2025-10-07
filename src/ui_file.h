//------------------------------------------------------------------------
//  FILE-RELATED DIALOGS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2019 Andrew Apted
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

class UI_ChooseMap : public UI_Escapable_Window
{
private:
	Fl_Input *map_name;
	Fl_Group *map_buttons;

	Fl_Return_Button *ok_but;

	// normally NULL, when present will prevent using an existing level name
	std::shared_ptr<const Wad_file> rename_wad;

	enum
	{
		ACT_none = 0,
		ACT_CANCEL,
		ACT_ACCEPT
	};

	int action = ACT_none;

	void CheckMapName();

public:
	UI_ChooseMap(const char *initial_name = "",
			     const std::shared_ptr<const Wad_file> &_rename_wad = NULL);
	virtual ~UI_ChooseMap()
	{
	}

	// format is 'E' for ExMx, or 'M' for MAPxx
	void PopulateButtons(char format, const Wad_file *test_wad = NULL);

	// returns map name on success, NULL on cancel
	SString Run();

private:
	static void     ok_callback(Fl_Widget *, void *);
	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
	static void  input_callback(Fl_Widget *, void *);
	static void    new_callback(Fl_Widget *, void *);
};


//------------------------------------------------------------------------

class UI_OpenMap : public UI_Escapable_Window
{
private:
	enum
	{
		LOOK_PWad = 0,
		LOOK_IWad,
		LOOK_Resource
	};

	Fl_Output *pwad_name;
	Fl_Choice *look_where;

	Fl_Input  *map_name;
	UI_Scroll *button_grp;

	Fl_Return_Button *ok_but;

	enum class Action
	{
		none,
		cancel,
		accept
	};

	Action action = Action::none;

	// the WAD file opened by the "Load" button (initially NULL)
	std::shared_ptr<Wad_file> loaded_wad;

	// the WAD file which we are showing map buttons for.
	// can be the "game_wad" or "edit_wad" globals, the "loaded_wad"
	// field above, or NULL.
	std::shared_ptr<Wad_file> using_wad;

	Instance &inst;

public:
	UI_OpenMap(Instance &inst);
	virtual ~UI_OpenMap();

	// Run the dialog and return an opened wad (from Wad_file::Open)
	// or edit_wad/game_wad, or NULL if the user cancelled.
	//
	// "map_v" parameter must be non-NULL, it receives the chosen map
	// name, or set to NULL when cancelled.
	//
	// "did_load" is true when the user loaded a new pwad and this
	// method returned it.  It should become the next edit_wad.
	//
	std::shared_ptr<Wad_file> Run(SString* map_v, bool * did_load);

private:
	void Populate();
	void PopulateButtons();

	void LoadFile();
	void SetPWAD(const SString &name);
	void CheckMapName();

private:
	static void     ok_callback(Fl_Widget *, void *);
	static void  close_callback(Fl_Widget *, void *);
	static void   look_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
	static void  input_callback(Fl_Widget *, void *);
	static void   load_callback(Fl_Widget *, void *);
};


//------------------------------------------------------------------------

class UI_ProjectSetup : public UI_Escapable_Window
{
public:
	enum
	{
		RES_NUM = 4
	};
	struct Result
	{
		SString game;
		SString port;
		MapFormat mapFormat = MapFormat::invalid;
		SString nameSpace;
		fs::path resources[RES_NUM];
	};

private:
	Fl_Choice *game_choice;
	Fl_Choice *port_choice;
	Fl_Choice *format_choice;

	Fl_Output *res_name[RES_NUM];

	Fl_Button *ok_but;
	Fl_Button *cancel;

	map_format_bitset_t usable_formats;

	enum class Action
	{
		none,
		cancel,
		accept
	};

	Action action = Action::none;

	static void   game_callback(Fl_Choice*, void*);
	static void   port_callback(Fl_Choice*, void*);
	static void format_callback(Fl_Choice*, void*);
	static void namespace_callback(Fl_Choice*, void*);

	static void  find_callback(Fl_Button*, void*);
	static void setup_callback(Fl_Button*, void*);
	static void  kill_callback(Fl_Button*, void*);
	static void  load_callback(Fl_Button*, void*);

	static void close_callback(Fl_Widget*, void*);
	static void   use_callback(Fl_Button*, void*);

	void PopulateIWADs();
	void PopulatePort();
	void PopulateMapFormat();
	void PopulateNamespaces();
	void PopulateResources();

	Result result;

	Fl_Button* mResourceFileButtons[RES_NUM] = {};
	Fl_Button* mResourceDirButtons[RES_NUM] = {};
	Fl_Button* mClearButtons[RES_NUM] = {};

	Instance &inst;

public:
	UI_ProjectSetup(Instance &inst, bool new_project = false, bool is_startup = false);

	// returns true if something changed
	tl::optional<Result> Run();
};

#endif  /* __EUREKA_UI_FILE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
