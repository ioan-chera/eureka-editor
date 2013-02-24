//------------------------------------------------------------------------
//  MAIN PROGRAM
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2013 Andrew Apted
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

#include "e_loadsave.h"
#include "im_color.h"
#include "m_config.h"
#include "editloop.h"
#include "m_dialog.h"  /* for Confirm() */
#include "m_game.h"
#include "m_files.h"
#include "levels.h"    /* Because of "viewtex" */

#include "w_flats.h"
#include "w_rawdef.h"
#include "w_sprite.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_file.h"

#ifndef WIN32
#include <time.h>
#endif


/*
 *  Global variables
 */

// progress during initialisation:
//   0 = nothing yet
//   1 = read early options, set up logging
//   2 = read all options, and possibly a config file
//   3 = inited FLTK, opened main window
int  init_progress;

bool want_quit = false;

const char *config_file = NULL;
const char *log_file;

const char *install_dir;
const char *home_dir;
const char *cache_dir;


const char *Iwad_name = NULL;
const char *Pwad_name = NULL;

std::vector< const char * > ResourceWads;

const char *Game_name;
const char *Port_name;
const char *Level_name;


int show_help     = 0;
int show_version  = 0;

bool Replacer = false;

int KF;
int KF_fonth;


// config items
int scroll_less   = 10;
int scroll_more   = 90;

int gui_scheme    = 2;  // plastic
int gui_color_set = 1;  // bright

rgb_color_t gui_custom_bg = 0xCCD5DD00;
rgb_color_t gui_custom_ig = 0xFFFFFF00;
rgb_color_t gui_custom_fg = 0x00000000;


/*
 *  Prototypes of private functions
 */
static void TermFLTK();


/*
 *  play a fascinating tune
 */
void Beep(const char *fmt, ...)
{
	va_list arg_ptr;

	static char buffer[MSG_BUF_LEN];

	va_start(arg_ptr, fmt);
	vsnprintf(buffer, MSG_BUF_LEN-1, fmt, arg_ptr);
	va_end(arg_ptr);

	buffer[MSG_BUF_LEN-1] = 0;

	if (buffer[0])
	{
		Status_Set("%s", buffer);

		LogPrintf("BEEP: %s\n", buffer);
	}
	else
		Status_Set("Problem occurred");

	EXEC_Errno = 1;

	fl_beep();
}


/*
 *  Show an error message and terminate the program
 */
void FatalError(const char *fmt, ...)
{
	va_list arg_ptr;

	static char buffer[MSG_BUF_LEN];

	va_start(arg_ptr, fmt);
	vsnprintf(buffer, MSG_BUF_LEN-1, fmt, arg_ptr);
	va_end(arg_ptr);

	buffer[MSG_BUF_LEN-1] = 0;

	if (init_progress < 1 || Quiet || log_file)
	{
		fprintf(stderr, "\nFATAL ERROR: %s", buffer);
	}

	if (init_progress >= 1)
	{
		LogPrintf("\nFATAL ERROR: %s", buffer);
	}

	if (init_progress >= 3)
	{
		DLG_ShowError("%s", buffer);

		init_progress = 2;

		TermFLTK();
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

	MasterDir_CloseAll();
	LogClose();

	exit(2);
}


static void CreateHomeDirs()
{
	SYS_ASSERT(home_dir);

	static char dir_name[FL_PATH_MAX];

#ifdef __APPLE__
	fl_filename_expand(dir_name, "$HOME/Library");
	FileMakeDir(dir_name);

	fl_filename_expand(dir_name, "$HOME/Library/Application Support");
	FileMakeDir(dir_name);

	fl_filename_expand(dir_name, "$HOME/Library/Caches");
	FileMakeDir(dir_name);
#endif

	// try to create home_dir (doesn't matter if it already exists)
	FileMakeDir(home_dir);
	FileMakeDir(cache_dir);

	static const char *const subdirs[] =
	{
		"cache", "backups",
		"iwads", "games", "ports", "mods",
		NULL
	};

	for (int i = 0 ; subdirs[i] ; i++)
	{
		snprintf(dir_name, FL_PATH_MAX, "%s/%s", (i < 2) ? cache_dir : home_dir, subdirs[i]);
		dir_name[FL_PATH_MAX-1] = 0;

		FileMakeDir(dir_name);
	}
}


static void Determine_HomeDir(const char *argv0)
{
	// already set by cmd-line option?
	if (! home_dir)
	{
#if defined(WIN32)
	// get the %APPDATA% location
	// FIXME: have a fallback e.g. EXE path + "/local"

	TCHAR * path = StringNew(MAX_PATH);

	if (! SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
		FatalError("Failed to read %%APPDATA%% location");

	strcat(path, "\\EurekaEditor");

	home_dir = StringDup(path);

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path)))
	{
		strcat(path, "\\EurekaEditor");

		cache_dir = StringDup(path);
	}

	StringFree(path);

#elif defined(__APPLE__)
	char * path = StringNew(FL_PATH_MAX + 4);

	if (fl_filename_expand(path, "$HOME/Library/Application Support/eureka-editor"))
		home_dir = StringDup(path);

	if (fl_filename_expand(path, "$HOME/Library/Caches/eureka-editor"))
		cache_dir = StringDup(path);

	StringFree(path);

#else  // UNIX
	char * path = StringNew(FL_PATH_MAX + 4);

	if (fl_filename_expand(path, "$HOME/.eureka"))
		home_dir = path;
#endif
	}

	if (! home_dir)
		FatalError("Unable to find home directory!\n");
	
	if (! cache_dir)
		cache_dir = home_dir;

    LogPrintf("Home  dir: %s\n", home_dir);
    LogPrintf("Cache dir: %s\n", cache_dir);

	// create cache directory (etc)
	CreateHomeDirs();

	// determine log filename
	log_file = StringPrintf("%s/logs.txt", home_dir);
}


static void Determine_InstallPath(const char *argv0)
{
	// already set by cmd-line option?
	if (! install_dir)
	{
#ifdef WIN32
	// read the registry key which the installer created
	HKEY key;

	if (! SUCCEEDED(RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\EurekaEditor", 
	                              0, KEY_QUERY_VALUE, &key)))
		FatalError("Broken installation (missing registry key)\n");

	DWORD type;
	DWORD len = (DWORD)FL_PATH_MAX;

	char *reg_string = StringNew(FL_PATH_MAX + 100);

	if (! SUCCEEDED(RegQueryValueExA(key, "Install_Dir", 0L,
	                                 &type, (BYTE*)reg_string, &len)))
		FatalError("Broken installation (missing registry value)\n");

	if (type != REG_SZ)
		FatalError("Broken installation (registry value is not a string)\n");

	// ensure string is NUL-terminated
	reg_string[len] = 0;

	install_dir = StringDup(reg_string);
	StringFree(reg_string);

	RegCloseKey(key);

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
		install_dir = StringPrintf("%s/share/eureka", prefixes[i]);

		const char *filename = StringPrintf("%s/games/doom2.ugh", install_dir);

		DebugPrintf("Trying install path: %s\n", install_dir);
		DebugPrintf("   looking for file: %s\n", filename);

		bool exists = FileExists(filename);

		StringFree(filename);

		if (exists)
			break;

		StringFree(install_dir);
		install_dir = NULL;
	}
#endif
	}

	// fallback : look in current directory
	if (! install_dir)
	{
		if (FileExists("./games/doom2.ugh"))
			install_dir = ".";
	}

	if (! install_dir)
		FatalError("Unable to find install directory!\n");

	LogPrintf("Install dir: %s\n", install_dir);
}


const char * DetermineGame(const char *iwad_name)
{
	static char game_name[FL_PATH_MAX];

	strcpy(game_name, fl_filename_name(iwad_name));

	fl_filename_setext(game_name, "");

	y_strlowr(game_name);

	return StringDup(game_name);
}


static bool DetermineIWAD()
{
	if (Iwad_name && FilenameIsBare(Iwad_name))
	{
		// a bare name (e.g. "heretic") is treated as a game name

		// make lowercase
		Iwad_name = StringDup(Iwad_name);
		y_strlowr((char *)Iwad_name);

		if (! CanLoadDefinitions("games", Iwad_name))
			FatalError("Unsupported game: %s (no definition file)\n", Iwad_name);

		const char * path = M_QueryKnownIWAD(Iwad_name);

		if (! path)
			FatalError("Cannot find IWAD for game: %s\n", Iwad_name);

		Iwad_name = StringDup(path);
	}
	else if (Iwad_name)
	{
		// if extension is missing, add ".wad"
		if (! HasExtension(Iwad_name))
			Iwad_name = ReplaceExtension(Iwad_name, "wad");

		if (! FileExists(Iwad_name))
			FatalError("Given IWAD does not exist: %s\n", Iwad_name);

		const char *game = DetermineGame(Iwad_name);

		if (! CanLoadDefinitions("games", game))
			FatalError("Unsupported game: %s (no definition file)\n", Iwad_name);

		M_AddKnownIWAD(Iwad_name);
		M_SaveRecent();
	}
	else
	{
		Iwad_name = M_PickDefaultIWAD();

		if (Iwad_name)
			Iwad_name = StringDup(Iwad_name);
		else
		{
			if (! Main_ProjectSetup(true))
				return false;
		}
	}

	return true;
}


static const char * DetermineMod(const char *res_name)
{
	static char mod_name[FL_PATH_MAX];
		
	strcpy(mod_name, fl_filename_name(res_name));

	fl_filename_setext(mod_name, "");

	y_strlowr(mod_name);

	return StringDup(mod_name);
}


static const char * DetermineLevel()
{
	int level_number = 0;

	// handle a numeric level number, e.g. -warp 15
	if (Level_name && Level_name[0])
	{
		if (! isdigit(Level_name[0]))
			return StringUpper(Level_name);

		level_number = atoi(Level_name);
	}

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		Wad_file *wad = (pass == 0) ? edit_wad : game_wad;

		if (! wad)
			continue;

		short lev_idx;

		if (level_number > 0)
		{
			lev_idx = wad->FindLevelByNumber(level_number);
			if (lev_idx < 0)
				FatalError("Level '%d' not found (no matches)\n", level_number);
		}
		else
		{
			lev_idx = wad->FindFirstLevel();
			if (lev_idx < 0)
				FatalError("No levels found in %s!\n", (pass == 0) ? "PWAD" : "IWAD");
		}

		Lump_c *lump = wad->GetLump(lev_idx);
		SYS_ASSERT(lump);

		return StringDup(lump->Name());
	}

	// cannot get here
	return "XXX";
}


/* this is only to prevent ESCAPE key from quitting */
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


static void InitFLTK()
{
	/*
	 *  Create the window
	 */
	Fl::visual(FL_RGB);


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


#ifdef UNIX
	Fl_File_Icon::load_system_icons();
#endif


	int screen_w = Fl::w();
	int screen_h = Fl::h();

	DebugPrintf("Detected Screen Size: %dx%d\n", screen_w, screen_h);

	KF = 1;
#if 0  // TODO
	KF = 0;
	if (screen_w >= 1000) KF = 1;
	if (screen_w >= 1180) KF = 2;
#endif

	KF_fonth = (14 + KF * 2);


	main_win = new UI_MainWin();

	main_win->label("Eureka v" EUREKA_VERSION);

	// show window (pass some dummy arguments)
	{
		int   argc = 1;
		char *argv[2];

		argv[0] = StringDup("Eureka.exe");
		argv[1] = NULL;

		main_win->show(argc, argv);
	}

	// kill the stupid bright background of the "plastic" scheme
	if (gui_scheme == 2)
	{
		delete Fl::scheme_bg_;
		Fl::scheme_bg_ = NULL;

		main_win->image(NULL);
	}

	log_viewer = new UI_LogViewer();

	LogOpenWindow();

    Fl::add_handler(Main_key_handler);

	main_win->ShowBrowser(0);
	main_win->NewEditMode('t');
}


static void TermFLTK()
{
}


// used for 'New Map' / 'Open Map' functions too
bool Main_ConfirmQuit(const char *action)
{
	if (! MadeChanges)
		return true;

	char buffer[200];

	sprintf(buffer, "You have unsaved changes. "
	                "Do you really want to %s?", action);

	if (Confirm(-1, -1, buffer, 0))
		return true;

	return false;
}


void Main_Loop()
{
	UpdateHighlight();

    for (;;)
    {
        Fl::wait(0.2);

        if (edit.RedrawMap)
        {
            main_win->canvas->redraw();

            edit.RedrawMap = 0;
        }

		if (want_quit)
		{
			if (Main_ConfirmQuit("quit"))
				break;

			want_quit = false;
		}

		// TODO: handle this a better way
		main_win->UpdateTitle(MadeChanges ? true : false);
    }
}


void Main_LoadResources()
{
	LogPrintf("----- Loading Resources -----\n");

	// Load game definitions (*.ugh)
	InitDefinitions();

	Game_name = DetermineGame(Iwad_name);

	LogPrintf("IWAD name: '%s'\n", Iwad_name);
	LogPrintf("Game name: '%s'\n", Game_name);

	LoadDefinitions("games", Game_name);

	// normally the game definition will set a default port
	if (! Port_name)
		Port_name = "vanilla";

	LogPrintf("Port name: '%s'\n", Port_name);

	LoadDefinitions("ports", Port_name);


	// reset the master directory
	if (edit_wad)
		MasterDir_Remove(edit_wad);
	
	MasterDir_CloseAll();


	// Load the IWAD (read only)
	game_wad = Wad_file::Open(Iwad_name, 'r');
	if (! game_wad)
		FatalError("Failed to open game IWAD: %s\n", Iwad_name);

	MasterDir_Add(game_wad);


	// Load all resource wads
	for (int i = 0 ; i < (int)ResourceWads.size() ; i++)
	{
		if (! FileExists(ResourceWads[i]))
			FatalError("Resource does not exist: %s\n", ResourceWads[i]);

		Wad_file *wad = Wad_file::Open(ResourceWads[i], 'r');

		if (! wad)
			FatalError("Cannot load resource: %s\n", ResourceWads[i]);

		MasterDir_Add(wad);

		// load corresponding mod file (if it exists)
		const char *mod_name = DetermineMod(ResourceWads[i]);
		
		if (CanLoadDefinitions("mods", mod_name))
		{
			LoadDefinitions("mods", mod_name);
		}
	}


	if (edit_wad)
		MasterDir_Add(edit_wad);


	// finally, load textures and stuff...

	W_LoadPalette();
	W_LoadColormap();

	W_LoadFlats();
	W_LoadTextures();
	W_ClearSprites();

	if (main_win)
	{
		main_win->browser->Populate();

		// TODO: only call this when the IWAD has changed
		Props_LoadValues();
	}
}


bool Main_ProjectSetup(bool is_startup)
{
	UI_ProjectSetup * dialog = new UI_ProjectSetup(is_startup);

	bool ok = dialog->Run();

	if (ok)
	{
		// grab new information

		Iwad_name = StringDup(dialog->iwad);
		Port_name = StringDup(dialog->port);

		ResourceWads.clear();

		for (int i = 0 ; i < UI_ProjectSetup::RES_NUM ; i++)
		{
			if (dialog->res[i])
				ResourceWads.push_back(StringDup(dialog->res[i]));
		}
	}

	delete dialog;

	Fl::wait(0.1);
	Fl::wait(0.1);

	if (! ok)
		return false;

	if (! is_startup)
		Main_LoadResources();

	return true;
}


/* ----- user information ----------------------------- */

static void ShowHelp()
{
	printf(	"\n"
			"*** " EUREKA_TITLE " v" EUREKA_VERSION " (C) 2013 Andrew Apted ***\n"
			"\n");

	printf(	"Eureka is free software, under the terms of the GNU General\n"
			"Public License (GPL), and comes with ABSOLUTELY NO WARRANTY.\n"
			"Home page: http://eureka-editor.sf.net/\n"
			"\n");

	printf( "USAGE: eureka [options...] [FILE]\n"
			"\n"
			"Available options are:\n");

	dump_command_line_options(stdout);

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


/*
 *  the driving program
 */
int main(int argc, char *argv[])
{
	init_progress = 0;

	int r;

	// a quick pass through the command line arguments
	// to handle special options, like --help, --install, --config
	r = M_ParseCommandLine(argc - 1, argv + 1, 1);

	if (r)
		exit(3);

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

	//printf ("%s\n", what ());


	init_progress = 1;


	LogPrintf("\n");
	LogPrintf("*** " EUREKA_TITLE " v" EUREKA_VERSION " (C) 2013 Andrew Apted ***\n");
	LogPrintf("\n");

	// Sanity checks (useful when porting).
	check_types();

	ShowTime();


	Determine_HomeDir(argv[0]);
	Determine_InstallPath(argv[0]);


	LogOpenFile(log_file);


	// a config file can provides some values
	M_ParseConfigFile();

	if (r == 0)
	{
		// environment variables can override them
		r = M_ParseEnvironmentVars();
	}

	if (r == 0)
	{
		// and command line arguments will override both
		r = M_ParseCommandLine(argc - 1, argv + 1, 2);
	}

	if (r != 0)
	{
		// FIXME
		fprintf(stderr, "Error parsing config or cmd-line options.\n");
		exit(1);
	}


	init_progress = 2;


	M_LoadRecent();
	M_LookForIWADs();

	Editor_Init();

	M_LoadBindings();

	InitFLTK();  // creates the main window

	init_progress = 3;


	// open a specified PWAD now
	if (Pwad_name)
	{
		if (! FileExists(Pwad_name))
			FatalError("Given pwad does not exist: %s\n", Pwad_name);

		edit_wad = Wad_file::Open(Pwad_name, 'a');

		if (! edit_wad)
			FatalError("Cannot load pwad: %s\n", Pwad_name);

		// Note: the Main_LoadResources() call will ensure this gets
		//       placed at the correct spot (at the end)
		MasterDir_Add(edit_wad);
	}


	// handle the '__EUREKA' lump.  It is almost equivalent to using the
	// -iwad, -merge and -port command line options, but with extra
	// checks (to allow editing a wad containing dud information).

	if (edit_wad)
	{
		if (! M_ParseEurekaLump(edit_wad))
		{
			// user cancelled the load
			RemoveEditWad();
			goto quit;
		}
	}


	// determine which IWAD to use
	if (! DetermineIWAD())
		goto quit;


	Main_LoadResources();


	// load the initial level

	Level_name = DetermineLevel();

	LogPrintf("Loading initial map : %s\n", Level_name);

	LoadLevel(edit_wad ? edit_wad : game_wad, Level_name);

	main_win->UpdateTotals();


	Main_Loop();


quit:
	/* that's all folks! */

	LogPrintf("Quit\n");


	init_progress = 2;

	TermFLTK();


	init_progress = 0;

	MasterDir_CloseAll();
	LogClose();

	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
