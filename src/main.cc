//------------------------------------------------------------------------
//  MAIN PROGRAM
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
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
#include "r_misc.h"
#include "levels.h"    /* Because of "viewtex" */

#include "w_flats.h"
#include "w_rawdef.h"
#include "w_sprite.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_file.h"


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
const char *log_file = NULL;

const char *install_dir;
const char *home_dir;


const char *Iwad_name = NULL;
const char *Pwad_name = NULL;

std::vector< const char * > ResourceWads;

const char *Game_name;
const char *Port_name;
const char *Level_name;


int show_help     = 0;
int show_version  = 0;

bool Replacer = false;


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
void Beep(const char *msg, ...)
{
	// TODO: show message in a status bar and/or log file

	fl_beep();
}


void Beep()
{
	Beep("");
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
	fl_filename_expand(dir_name, "$HOME/documents");
	FileMakeDir(dir_name);
#endif

	// try to create home_dir (doesn't matter if it already exists)
	FileMakeDir(home_dir);

	static const char *const subdirs[] =
	{
		"cache", "iwads", "games", "ports", "mods", NULL
	};

	for (int i = 0 ; subdirs[i] ; i++)
	{
		snprintf(dir_name, FL_PATH_MAX, "%s/%s", home_dir, subdirs[i]);
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
	// FIXME: check for %appdata% ??
	//        also, fallback should be EXE path + "/local"

	home_dir = GetExecutablePath(argv0);

#elif defined(__APPLE__)
	char * path = StringNew(FL_PATH_MAX + 4);

	if (fl_filename_expand(path, "$HOME/documents/eureka"))
		home_dir = path;

#else  // UNIX
	char * path = StringNew(FL_PATH_MAX + 4);

	if (fl_filename_expand(path, "$HOME/.eureka"))
		home_dir = path;
#endif
	}

	if (! home_dir)
		FatalError("Unable to find home directory!\n");

    LogPrintf("Home dir: %s\n", home_dir);

	// create cache directory (etc)
	CreateHomeDirs();
}


static void Determine_InstallPath(const char *argv0)
{
	// already set by cmd-line option?
	if (! install_dir)
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


static const char * DetermineIWAD()
{
#if 0
	const char *path;

	// handle -iwad parameter
	if (Iwad_name)
	{
		// handle a directory name
		if (fl_filename_isdir(Iwad_name))
		{
			path = SearchDirForIWAD(Iwad_name);
			if (path)
				return path;

			FatalError("Unable to find any IWAD in directory: %s\n", Iwad_name);
		}

		// if extension is missing, add ".wad"
		if (! HasExtension(Iwad_name))
		{
			Iwad_name = ReplaceExtension(Iwad_name, "wad");
		}

		// handle a full path
		if (FindBaseName(Iwad_name) != Iwad_name)
			return Iwad_name;

		// FALL THROUGH to below code
		// (since Iwad_name contains a bare name)
	}
#endif

	FatalError("Unable to find any IWAD\n");
	return ""; /* NOT REACHED */
}


const char * DetermineGame(const char *iwad_name)
{
	// IDEA: allow override via -game parameter

	static char game_name[FL_PATH_MAX];

	strcpy(game_name, fl_filename_name(iwad_name));

	fl_filename_setext(game_name, "");

	y_strlowr(game_name);

	return StringDup(game_name);
}


const char * DetermineMod(const char *res_name)
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
			return Level_name;

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


int Main_key_handler(int event)
{
	if (event != FL_SHORTCUT)
		return 0;

	switch (Fl::event_key())
	{
		case FL_Escape:
			fl_beep();  // FIXME
			return 1;

		case FL_F+1:   // F1 = HELP
			// TODO
			return 1;

		default: break;
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

    Fl::add_handler(Main_key_handler);

	main_win->ShowBrowser(0);

	SetWindowSize (main_win->canvas->w(), main_win->canvas->h());
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
			"*** " EUREKA_TITLE " v" EUREKA_VERSION " (C) 2012 Andrew Apted ***\n"
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


	LogOpen(log_file);

	init_progress = 1;


	LogPrintf("\n");
	LogPrintf("*** " EUREKA_TITLE " v" EUREKA_VERSION " (C) 2012 Andrew Apted ***\n");
	LogPrintf("\n");

	// Sanity checks (useful when porting).
	check_types();


	Determine_HomeDir(argv[0]);
	Determine_InstallPath(argv[0]);


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


	// the '__EUREKA' lump, here at least, is equivalent to using the
	// -iwad, -merge and -port command line options.

	if (edit_wad && edit_wad->FindLump("EUREKA_LUMP"))
	{
		// FIXME: M_ParseEurekaLump(edit_wad);
	}


	// determine which IWAD to use

	if (Iwad_name)
	{
		if (! FileExists(Iwad_name))
			FatalError("Given IWAD does not exist: %s\n", Iwad_name);

		const char *game = DetermineGame(Iwad_name);

		M_AddKnownIWAD(game, Iwad_name);
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
				goto quit;
		}
	}


	Main_LoadResources();


	// load the initial level

	Level_name = DetermineLevel();

	LogPrintf("Loading initial map : %s\n", Level_name);

	LoadLevel(edit_wad ? edit_wad : game_wad, Level_name);

	main_win->UpdateTotals();


	Main_Loop();


	/* that's all folks! */

quit:
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
