//------------------------------------------------------------------------
//  MAIN PROGRAM
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2018 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include <time.h>

#include "im_color.h"
#include "m_config.h"
#include "m_game.h"
#include "m_files.h"
#include "m_loadsave.h"

#include "e_main.h"
#include "m_events.h"
#include "r_render.h"

#include "w_rawdef.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_about.h"
#include "ui_file.h"

#ifndef WIN32
#include <time.h>
#endif

// IOANCH: be able to call OSX specific routines (needed for ~/Library)
#ifdef __APPLE__
#include "OSXCalls.h"
#endif


//
//  global variables
//

bool want_quit = false;
bool app_has_focus = false;

std::string config_file;
std::string log_file;

std::string install_dir;
std::string home_dir;
const char *cache_dir;


std::string Iwad_name;
std::string Pwad_name;

std::vector<std::string> Pwad_list;
std::vector< const char * > Resource_list;

std::string Game_name;
const char *Port_name;
std::string Level_name;

map_format_e Level_format;


//
// config items
//
bool auto_load_recent = false;
bool begin_maximized  = false;
bool map_scroll_bars  = true;

std::string default_port = "vanilla";

int gui_scheme    = 1;  // gtk+
int gui_color_set = 1;  // bright

rgb_color_t gui_custom_bg = RGB_MAKE(0xCC, 0xD5, 0xDD);
rgb_color_t gui_custom_ig = RGB_MAKE(255, 255, 255);
rgb_color_t gui_custom_fg = RGB_MAKE(0, 0, 0);


// Progress during initialisation:
//    0 = nothing yet
//    1 = read early options, set up logging
//    2 = parsed all options, inited FLTK
//    3 = opened the main window
int  init_progress;

int show_help     = 0;
int show_version  = 0;


static void RemoveSingleNewlines(char *buffer)
{
	for (char *p = buffer ; *p ; p++)
	{
		if (*p == '\n' && p[1] == '\n')
			while (*p == '\n')
				p++;

		if (*p == '\n')
			*p = ' ';
	}
}


//
//  show an error message and terminate the program
//
void FatalError(const char *fmt, ...)
{
	va_list arg_ptr;

	static char buffer[MSG_BUF_LEN];

	va_start(arg_ptr, fmt);
	vsnprintf(buffer, MSG_BUF_LEN-1, fmt, arg_ptr);
	va_end(arg_ptr);

	buffer[MSG_BUF_LEN-1] = 0;

	if (init_progress < 1 || Quiet || !log_file.empty())
	{
		fprintf(stderr, "\nFATAL ERROR: %s", buffer);
	}

	if (init_progress >= 1)
	{
		LogPrintf("\nFATAL ERROR: %s", buffer);
	}

	if (init_progress >= 2)
	{
		RemoveSingleNewlines(buffer);

		DLG_ShowError("%s", buffer);

		init_progress = 1;
	}
#ifdef WIN32
	else
	{
		MessageBox(NULL, buffer, "Eureka : Error",
		           MB_ICONEXCLAMATION | MB_OK |
				   MB_SYSTEMMODAL | MB_SETFOREGROUND);
	}
#endif

	init_progress = 0;
	app_has_focus = false;

	MasterDir_CloseAll();
	LogClose();

	exit(2);
}


static void CreateHomeDirs()
{
	SYS_ASSERT(!home_dir.empty());

	static char dir_name[FL_PATH_MAX];

#ifdef __APPLE__
   // IOANCH 20130825: modified to use name-independent calls
	fl_filename_expand(dir_name, OSX_UserDomainDirectory(osx_LibDir, NULL));
	FileMakeDir(dir_name);

	fl_filename_expand(dir_name, OSX_UserDomainDirectory(osx_LibAppSupportDir, NULL));
	FileMakeDir(dir_name);

	fl_filename_expand(dir_name, OSX_UserDomainDirectory(osx_LibCacheDir, NULL));
	FileMakeDir(dir_name);
#endif

	// try to create home_dir (doesn't matter if it already exists)
	FileMakeDir(home_dir.c_str());
	FileMakeDir(cache_dir);

	static const char *const subdirs[] =
	{
		// these under $cache_dir
		"cache", "backups",

		// these under $home_dir
		"iwads", "games", "ports",

		NULL	// end of list
	};

	for (int i = 0 ; subdirs[i] ; i++)
	{
		snprintf(dir_name, FL_PATH_MAX, "%s/%s", (i < 2) ? cache_dir : home_dir.c_str(), subdirs[i]);
		dir_name[FL_PATH_MAX-1] = 0;

		FileMakeDir(dir_name);
	}
}


static void Determine_HomeDir(const char *argv0)
{
	// already set by cmd-line option?
	if (home_dir.empty())
	{
#if defined(WIN32)
	// get the %APPDATA% location
	// if that fails, use a folder at the EXE location

	TCHAR path[MAX_PATH];

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
	{
		home_dir = path;
		home_dir += "\\EurekaEditor";
	}
	else
	{
		SYS_ASSERT(!install_dir.empty());
		home_dir = install_dir + "\\app_data";
	}

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path)))
	{
		strcat(path, "\\EurekaEditor");

		cache_dir = StringDup(path);
	}

#elif defined(__APPLE__)
	char path[FL_PATH_MAX + 4];

   fl_filename_expand(path, OSX_UserDomainDirectory(osx_LibAppSupportDir, "eureka-editor"));
   home_dir = path;

   fl_filename_expand(path, OSX_UserDomainDirectory(osx_LibCacheDir, "eureka-editor"));
   cache_dir = StringDup(path);

#else  // UNIX
	char path[FL_PATH_MAX + 4];

	if (fl_filename_expand(path, "$HOME/.eureka"))
		home_dir = path;
#endif
	}

	if (home_dir.empty())
		FatalError("Unable to find home directory!\n");

	if (! cache_dir)
		cache_dir = home_dir.c_str();

	LogPrintf("Home  dir: %s\n", home_dir.c_str());
	LogPrintf("Cache dir: %s\n", cache_dir);

	// create cache directory (etc)
	CreateHomeDirs();

	// determine log filename
	log_file = home_dir + "/logs.txt";
}


static void Determine_InstallPath(const char *argv0)
{
	// already set by cmd-line option?
	if (install_dir.empty())
	{
#ifdef WIN32
	install_dir = GetExecutablePath(argv0);

#else
	static const char *prefixes[] =
	{
		"/usr/local",
		"/usr",
		"/opt",
		NULL
	};

	for (int i = 0 ; prefixes[i] ; i++)
	{
		install_dir = prefixes[i];
		install_dir += "/share/eureka";

		std::string filename = install_dir + "/games/doom2.ugh";

		DebugPrintf("Trying install path: %s\n", install_dir.c_str());
		DebugPrintf("   looking for file: %s\n", filename.c_str());

		if (FileExists(filename.c_str()))
			break;

		install_dir.clear();
	}
#endif
	}

	// fallback : look in current directory
	if (install_dir.empty())
	{
		if (FileExists("./games/doom2.ugh"))
			install_dir = ".";
	}

	if (install_dir.empty())
		FatalError("Unable to find install directory!\n");

	LogPrintf("Install dir: %s\n", install_dir.c_str());
}


std::string GameNameFromIWAD(const char *iwad_name)
{
	char game_name[FL_PATH_MAX];

	strcpy(game_name, fl_filename_name(iwad_name));

	fl_filename_setext(game_name, "");

	y_strlowr(game_name);

	return game_name;
}


static bool DetermineIWAD()
{
	// this mainly handles a value specified on the command-line,
	// since values in a EUREKA_LUMP are already vetted.  Hence
	// producing a fatal error here is OK.

	if (!Iwad_name.empty() && FilenameIsBare(Iwad_name.c_str()))
	{
		// a bare name (e.g. "heretic") is treated as a game name

		// make lowercase
		y_strlowr(Iwad_name);

		if (! M_CanLoadDefinitions("games", Iwad_name.c_str()))
			FatalError("Unknown game '%s' (no definition file)\n", Iwad_name.c_str());

		std::string path = M_QueryKnownIWAD(Iwad_name.c_str());

		if (path.empty())
			FatalError("Cannot find IWAD for game '%s'\n", Iwad_name.c_str());

		Iwad_name = path;
	}
	else if (!Iwad_name.empty())
	{
		// if extension is missing, add ".wad"
		if (! HasExtension(Iwad_name.c_str()))
			Iwad_name = ReplaceExtension(Iwad_name.c_str(), "wad");

		if (! Wad_file::Validate(Iwad_name.c_str()))
			FatalError("IWAD does not exist or is invalid: %s\n", Iwad_name.c_str());

		std::string game = GameNameFromIWAD(Iwad_name.c_str());

		if (! M_CanLoadDefinitions("games", game.c_str()))
			FatalError("Unknown game '%s' (no definition file)\n", Iwad_name.c_str());

		M_AddKnownIWAD(Iwad_name.c_str());
		M_SaveRecent();
	}
	else
	{
		Iwad_name = M_PickDefaultIWAD();

		if (Iwad_name.empty())
		{
			// show the "Missing IWAD!" dialog.
			// if user cancels it, we have no choice but to quit.
			if (! MissingIWAD_Dialog())
				return false;
		}
	}

	Game_name = GameNameFromIWAD(Iwad_name.c_str());

	return true;
}


static void DeterminePort()
{
	// user supplied value?
	// NOTE: values from the EUREKA_LUMP are already verified.
	if (Port_name && Port_name[0])
	{
		if (! M_CanLoadDefinitions("ports", Port_name))
			FatalError("Unknown port '%s' (no definition file)\n", Port_name);

		return;
	}

	std::string var_game = M_VariantForGame(Game_name.c_str());

	// ensure the 'default_port' value is OK
	if (default_port.empty())
	{
		LogPrintf("WARNING: Default port is empty, using vanilla.\n");
		default_port = "vanilla";
	}
	else if (! M_CanLoadDefinitions("ports", default_port.c_str()))
	{
		LogPrintf("WARNING: Default port '%s' is unknown, using vanilla.\n",
				default_port.c_str());
		default_port = "vanilla";
	}
	else if (! M_CheckPortSupportsGame(var_game.c_str(), default_port.c_str()))
	{
		LogPrintf("WARNING: Default port '%s' not compatible with '%s'\n",
				default_port.c_str(), Game_name.c_str());
		default_port = "vanilla";
	}

	Port_name = default_port.c_str();
}


static const char * DetermineLevel()
{
	// most of the logic here is to handle a numeric level number
	// e.g. -warp 15
	//
	// DOOM 1 level number is 10 * episode + map, e.g. 23 --> E2M3
	// (there is logic in command-line parsing to handle two numbers after -warp)

	int level_number = 0;

	if (!Level_name.empty() && Level_name[0])
	{
		if (! isdigit(Level_name[0]))
			return StringUpper(Level_name.c_str());

		level_number = atoi(Level_name.c_str());
	}

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		Wad_file *wad = (pass == 0) ? edit_wad : game_wad;

		if (! wad)
			continue;

		short lev_num;

		if (level_number > 0)
		{
			lev_num = wad->LevelFindByNumber(level_number);
			if (lev_num < 0)
				FatalError("Level '%d' not found (no matches)\n", level_number);

		}
		else
		{
			lev_num = wad->LevelFindFirst();
			if (lev_num < 0)
				FatalError("No levels found in the %s!\n", (pass == 0) ? "PWAD" : "IWAD");
		}

		short idx = wad->LevelHeader(lev_num);

		Lump_c *lump = wad->GetLump(idx);
		SYS_ASSERT(lump);

		return StringDup(lump->Name());
	}

	// cannot get here
	return "XXX";
}


// this is only to prevent ESCAPE key from quitting
int Main_key_handler(int event)
{
	if (event != FL_SHORTCUT)
		return 0;

	if (Fl::event_key() == FL_Escape)
	{
		fl_beep();
		return 1;
	}

	return 0;
}


// see comment in Main_SetupFLTK...
#if !defined(WIN32) && !defined(__APPLE__)
int x11_check_focus_change(void *xevent, void *data)
{
	if (main_win != NULL)
	{
		const XEvent *xev = (const XEvent *)xevent;

		Window xid = xev->xany.window;

		if (fl_find(xid) == main_win)
		{
			switch (xev->type)
			{
				case FocusIn:
					app_has_focus = true;
					// fprintf(stderr, "** app_has_focus: TRUE **\n");
					break;

				case FocusOut:
					app_has_focus = false;
					// fprintf(stderr, "** app_has_focus: false **\n");
					break;

				default:
					break;
			}
		}
	}

	// we never eat the event
	return 0;
}
#endif


static void Main_SetupFLTK()
{
	Fl::visual(FL_DOUBLE | FL_RGB);

	// disable keyboard navigation, as it often interferes with our
	// user interface, especially TAB key for toggling the 3D view.
	Fl::option(Fl::OPTION_VISIBLE_FOCUS, false);

	if (gui_color_set == 0)
	{
		// use default colors
	}
	else if (gui_color_set == 1)
	{
		Fl::background(236, 232, 228);
		Fl::background2(255, 255, 255);
		Fl::foreground(0, 0, 0);
	}
	else
	{
		// custom colors
		Fl::background (RGB_RED(gui_custom_bg), RGB_GREEN(gui_custom_bg), RGB_BLUE(gui_custom_bg));
		Fl::background2(RGB_RED(gui_custom_ig), RGB_GREEN(gui_custom_ig), RGB_BLUE(gui_custom_ig));
		Fl::foreground (RGB_RED(gui_custom_fg), RGB_GREEN(gui_custom_fg), RGB_BLUE(gui_custom_fg));
	}

	if (gui_scheme == 0)
	{
		// use default scheme
	}
	else if (gui_scheme == 1)
	{
		Fl::scheme("gtk+");
	}
	else
	{
		Fl::scheme("plastic");
	}

	// for Linux + X11, add a system event handler that detects
	// when our main window gains or loses focus.  We need to do
	// it this way since FLTK is so broken.
#if !defined(WIN32) && !defined(__APPLE__)
	Fl::add_system_handler(x11_check_focus_change, NULL);
#endif

#ifndef WIN32
	Fl_File_Icon::load_system_icons();
#endif

	int screen_w = Fl::w();
	int screen_h = Fl::h();

	LogPrintf("Detected Screen Size: %dx%d\n", screen_w, screen_h);

	KF = 1;
#if 0  // TODO
	KF = 0;
	if (screen_w >= 1000) KF = 1;
	if (screen_w >= 1180) KF = 2;
#endif

	KF_fonth = (14 + KF * 2);
}


//
// Creates the main window
//
static void Main_OpenWindow()
{
	main_win = new UI_MainWindow();

	main_win->label("Eureka v" EUREKA_VERSION);

	// show window (pass some dummy arguments)
	{
		int   argc = 1;
		char *argv[2];

		argv[0] = StringDup("Eureka.exe");
		argv[1] = NULL;

		main_win->show(argc, argv);

		app_has_focus = true;
	}

	// kill the stupid bright background of the "plastic" scheme
	if (gui_scheme == 2)
	{
		delete Fl::scheme_bg_;
		Fl::scheme_bg_ = NULL;

		main_win->image(NULL);
	}

	Fl::check();

	InitAboutDialog();

	if (begin_maximized)
		main_win->Maximize();

	log_viewer = new UI_LogViewer();

	LogOpenWindow();

	Fl::add_handler(Main_key_handler);

	main_win->BrowserMode(0);
	main_win->NewEditMode(edit.mode);

	// allow processing keyboard events, even before the mouse
	// pointer has entered our window.
	Fl::focus(main_win->canvas);

	Fl::check();
}


void Main_Quit()
{
	want_quit = true;
}


// used for 'New Map' / 'Open Map' functions too
bool Main_ConfirmQuit(const char *action)
{
	if (! MadeChanges)
		return true;

	char buttons[200];

	snprintf(buttons, sizeof(buttons), "Cancel|&%s", action);

	// convert action string like "open a new map" to a simple "Open"
	// string for the yes choice.

	buttons[8] = toupper(buttons[8]);

	char *p = strchr(buttons, ' ');
	if (p)
		*p = 0;

	if (DLG_Confirm(buttons,
	                "You have unsaved changes.  "
	                "Do you really want to %s?", action) == 1)
	{
		return true;
	}

	return false;
}


//
// the directory we should use for a file open/save operation.
// returns NULL when not sure.
//
const char * Main_FileOpFolder()
{
	static char folder[FL_PATH_MAX];

	if (!Pwad_name.empty())
	{
		FilenameGetPath(folder, sizeof(folder), Pwad_name.c_str());

		if (folder[0])
			return folder;
	}

	return NULL;
}


void Main_Loop()
{
	RedrawMap();

	for (;;)
	{
		if (edit.is_navigating)
		{
			Nav_Navigate();

			Fl::wait(0);

			if (want_quit)
				break;
		}
		else
		{
			Fl::wait(0.2);
		}

		if (want_quit)
		{
			if (Main_ConfirmQuit("quit"))
				break;

			want_quit = false;
		}

		// TODO: handle these in a better way
		main_win->UpdateTitle(MadeChanges ? '*' : 0);

		main_win->scroll->UpdateRenderMode();
		main_win->scroll->UpdateBounds();

		if (edit.Selected->empty())
			edit.error_mode = false;
	}
}


static void LoadResourceFile(const char *filename)
{
	// support loading "ugh" config files
	if (MatchExtension(filename, "ugh"))
	{
		M_ParseDefinitionFile(PURPOSE_Resource, filename);
		return;
	}

	if (! Wad_file::Validate(filename))
		FatalError("Resource does not exist: %s\n", filename);

	Wad_file *wad = Wad_file::Open(filename, 'r');

	if (! wad)
		FatalError("Cannot load resource: %s\n", filename);

	MasterDir_Add(wad);
}


static void Main_LoadIWAD()
{
	// Load the IWAD (read only).
	// The filename has been checked in DetermineIWAD().
	game_wad = Wad_file::Open(Iwad_name.c_str(), 'r');
	if (! game_wad)
		FatalError("Failed to open game IWAD: %s\n", Iwad_name.c_str());

	MasterDir_Add(game_wad);
}


static void ReadGameInfo()
{
	Game_name = GameNameFromIWAD(Iwad_name.c_str());

	LogPrintf("Game name: '%s'\n", Game_name.c_str());
	LogPrintf("IWAD file: '%s'\n", Iwad_name.c_str());

	M_LoadDefinitions("games", Game_name.c_str());
}


static void ReadPortInfo()
{
	// we assume that the port name is valid, i.e. a config file
	// exists for it.  That is checked by DeterminePort() and
	// the EUREKA_LUMP parsing code.

	SYS_ASSERT(Port_name);

	std::string var_game = M_VariantForGame(Game_name.c_str());

	// warn user if this port is incompatible with the game
	if (! M_CheckPortSupportsGame(var_game.c_str(), Port_name))
	{
		LogPrintf("WARNING: the port '%s' is not compatible with the game '%s'\n",
				Port_name, Game_name.c_str());

		int res = DLG_Confirm("&vanilla|No Change",
						"Warning: the given port '%s' is not compatible with "
						"this game (%s)."
						"\n\n"
						"To prevent seeing invalid line and sector types, "
						"it is recommended to reset the port to something valid.\n"
						"Select a new port now?",
						Port_name, Game_name.c_str());

		if (res == 0)
		{
			Port_name = "vanilla";
		}
	}

	LogPrintf("Port name: '%s'\n", Port_name);

	M_LoadDefinitions("ports", Port_name);

	// prevent UI weirdness if the port is forced to BOOM / MBF
	if (game_info.strife_flags)
	{
		game_info.pass_through = 0;
		game_info.coop_dm_flags = 0;
		game_info.friend_flag = 0;
	}
}


//
// load all game/port definitions (*.ugh).
// open all wads in the master directory.
// read important content from the wads (palette, textures, etc).
//
void Main_LoadResources()
{
	LogPrintf("\n");
	LogPrintf("----- Loading Resources -----\n");

	M_ClearAllDefinitions();

	ReadGameInfo();

	ReadPortInfo();

	// reset the master directory
	if (edit_wad)
		MasterDir_Remove(edit_wad);

	MasterDir_CloseAll();

	Main_LoadIWAD();

	// load all resource wads
	for (int i = 0 ; i < (int)Resource_list.size() ; i++)
	{
		LoadResourceFile(Resource_list[i]);
	}

	if (edit_wad)
		MasterDir_Add(edit_wad);

	// finally, load textures and stuff...
	W_LoadPalette();
	W_LoadColormap();

	W_LoadFlats();
	W_LoadTextures();
	W_ClearSprites();

	LogPrintf("--- DONE ---\n");
	LogPrintf("\n");

	if (main_win)
	{
		main_win->UpdateGameInfo();

		main_win->browser->Populate();

		// TODO: only call this when the IWAD has changed
		Props_LoadValues();
	}
}


/* ----- user information ----------------------------- */

static void ShowHelp()
{
	printf(	"\n"
			"*** " EUREKA_TITLE " v" EUREKA_VERSION " (C) 2018 Andrew Apted, et al ***\n"
			"\n");

	printf(	"Eureka is free software, under the terms of the GNU General\n"
			"Public License (GPL), and comes with ABSOLUTELY NO WARRANTY.\n"
			"Home page: http://eureka-editor.sf.net/\n"
			"\n");

	printf( "USAGE: eureka [options...] [FILE...]\n"
			"\n"
			"Available options are:\n");

	M_PrintCommandLineOptions(stdout);

	fflush(stdout);
}


static void ShowVersion()
{
	printf("Eureka version " EUREKA_VERSION " (" __DATE__ ")\n");

	fflush(stdout);
}


static void ShowTime()
{
#ifdef WIN32
	SYSTEMTIME sys_time;

	GetSystemTime(&sys_time);

	LogPrintf("Current time: %02d:%02d on %04d/%02d/%02d\n",
			  sys_time.wHour, sys_time.wMinute,
			  sys_time.wYear, sys_time.wMonth, sys_time.wDay);

#else // LINUX or MACOSX

	time_t epoch_time;
	struct tm *calend_time;

	if (time(&epoch_time) == (time_t)-1)
		return;

	calend_time = localtime(&epoch_time);
	if (! calend_time)
		return;

	LogPrintf("Current time: %02d:%02d on %04d/%02d/%02d\n",
			  calend_time->tm_hour, calend_time->tm_min,
			  calend_time->tm_year + 1900, calend_time->tm_mon + 1,
			  calend_time->tm_mday);
#endif
}


//
//  the program starts here
//
int main(int argc, char *argv[])
{
	init_progress = 0;


	// a quick pass through the command line arguments
	// to handle special options, like --help, --install, --config
	M_ParseCommandLine(argc - 1, argv + 1, 1);

	if (show_help)
	{
		ShowHelp();
		return 0;
	}
	else if (show_version)
	{
		ShowVersion();
		return 0;
	}

	init_progress = 1;


	LogPrintf("\n");
	LogPrintf("*** " EUREKA_TITLE " v" EUREKA_VERSION " (C) 2018 Andrew Apted, et al ***\n");
	LogPrintf("\n");

	// sanity checks type sizes (useful when porting)
	CheckTypeSizes();

	ShowTime();


	Determine_InstallPath(argv[0]);
	Determine_HomeDir(argv[0]);

	LogOpenFile(log_file.c_str());


	// load all the config settings
	M_ParseConfigFile();

	// environment variables can override them
	M_ParseEnvironmentVars();

	// and command line arguments will override both
	M_ParseCommandLine(argc - 1, argv + 1, 2);


	Editor_Init();

	Main_SetupFLTK();

	init_progress = 2;


	M_LoadRecent();
	M_LoadBindings();

	M_LookForIWADs();

	Main_OpenWindow();

	init_progress = 3;

	M_LoadOperationMenus();


	// open a specified PWAD now
	// [ the map is loaded later.... ]

	if (!Pwad_list.empty())
	{
		// this fatal errors on any missing file
		// [ hence the Open() below is very unlikely to fail ]
		M_ValidateGivenFiles();

		Pwad_name = Pwad_list[0];

		edit_wad = Wad_file::Open(Pwad_name.c_str(), 'a');
		if (! edit_wad)
			FatalError("Cannot load pwad: %s\n", Pwad_name.c_str());

		// Note: the Main_LoadResources() call will ensure this gets
		//       placed at the correct spot (at the end)
		MasterDir_Add(edit_wad);
	}
	// don't auto-load when --iwad or --warp was used on the command line
	else if (auto_load_recent && Iwad_name.empty() && Level_name.empty())
	{
		if (M_TryOpenMostRecent())
		{
			MasterDir_Add(edit_wad);
		}
	}


	// Handle the '__EUREKA' lump.  It is almost equivalent to using the
	// -iwad, -merge and -port command line options, but with extra
	// checks (to allow editing a wad containing dud information).
	//
	// Note: there is logic in M_ParseEurekaLump() to ensure that command
	// line arguments can override the EUREKA_LUMP values.

	if (edit_wad)
	{
		if (! M_ParseEurekaLump(edit_wad, true /* keep_cmd_line_args */))
		{
			// user cancelled the load
			RemoveEditWad();
		}
	}


	// determine which IWAD to use
	if (! DetermineIWAD())
		goto quit;

	DeterminePort();


	// temporarily load the iwad, the following few functions need it.
	// it gets loaded again in Main_LoadResources().
	Main_LoadIWAD();

	Level_name = DetermineLevel();

	// config file parsing can depend on the map format, so get it now
	GetLevelFormat(edit_wad ? edit_wad : game_wad, Level_name.c_str());

	Main_LoadResources();


	// load the initial level
	LogPrintf("Loading initial map : %s\n", Level_name.c_str());

	LoadLevel(edit_wad ? edit_wad : game_wad, Level_name.c_str());


	Main_Loop();


quit:
	/* that's all folks! */

	LogPrintf("Quit\n");

	init_progress = 0;
	app_has_focus = false;

	MasterDir_CloseAll();
	LogClose();

	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
