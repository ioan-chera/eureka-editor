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
#include "r_misc.h"
#include "levels.h"    /* Because of "viewtex" */

#include "w_flats.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"


/*
 *  Constants (declared in main.h)
 */
const char *const msg_unexpected = "unexpected error";
const char *const msg_nomem      = "Not enough memory";


/*
 *  Not real variables -- just unique pointer values
 *  used by functions that return pointers to 
 */
char error_non_unique[1];  // Found more than one
char error_none[1];        // Found none
char error_invalid[1];     // Invalid parameter


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

int       copy_linedef_reuse_sidedefs = 0;

int       default_ceiling_height  = 128;
char      default_ceiling_texture[WAD_FLAT_NAME + 1]  = "CEIL3_5";
int       default_floor_height        = 0;
char      default_floor_texture[WAD_FLAT_NAME + 1]  = "FLOOR4_8";
int       default_light_level       = 144;
char      default_lower_texture[WAD_TEX_NAME + 1] = "STARTAN3";
char      default_middle_texture[WAD_TEX_NAME + 1]  = "STARTAN3";
char      default_upper_texture[WAD_TEX_NAME + 1] = "STARTAN3";

const char *install_dir;
const char *home_dir;

const char *Game_name;
const char *Port_name;
const char *Level_name;

int  remind_to_build_nodes = 0;  // Remind user to build nodes

bool Replacer = false;

const char *Iwad = NULL;
const char *Pwad = NULL;

unsigned long scroll_less   = 10;
unsigned long scroll_more   = 90;
int       sprite_scale  = 100;

int       show_help     = 0;
int       show_version  = 0;


/*
 *  Prototypes of private functions
 */
static void TermFLTK();


/*
 *  parse_environment_vars
 *  Check certain environment variables.
 *  Returns 0 on success, <>0 on error.
 */
static int parse_environment_vars ()
{
#if 0
	char *value;

	value = getenv ("EUREKA_GAME");
	if (value != NULL)
		Game = value;
#endif

	return 0;
}


/*
   play a fascinating tune
*/

void Beep ()
{
	fl_beep();
}


/*
 *  print_error_message
 *  Print an error message to stderr.
 */
static void print_error_message (const char *fmt, va_list args)
{
	fflush (stdout);
	fputs ("Error: ", stderr);
	vfprintf (stderr, fmt, args);
	fputc ('\n', stderr);
	fflush (stderr);
}


/*
 *  fatal_error
 *  Print an error message and terminate the program with code 2.
 */
void FatalError(const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	print_error_message (fmt, args);

	TermFLTK();

// TODO	CloseWadFiles ();
	LogClose();

	exit(2);
}


void BugError(const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	print_error_message (fmt, args);  // FUCKEN FIXME QUICK

	TermFLTK();

	LogClose();

// TODO	CloseWadFiles ();
	exit(9);
}


static void Determine_HomeDir(const char *argv0)
{
	// already set by cmd-line option?
	if (! home_dir)
	{
#ifdef WIN32
	home_dir = GetExecutablePath(argv0);

#else
	char * path = StringNew(FL_PATH_MAX + 4);

	if (fl_filename_expand(path, "$HOME/.eureka") == 0)
		FatalError("Unable to find home directory!\n");

	home_dir = path;

	// try to create it (doesn't matter if it already exists)
	FileMakeDir(home_dir);
#endif
	}

	///---  if (! home_dir)
	///---    home_dir = StringDup(".");

#if 1  // DEBUG
    fprintf(stderr, "Home dir: [%s]\n", home_dir ? home_dir : "NOT SET");
#endif
}


static void Determine_InstallPath(const char *argv0)
{
	// already set by cmd-line option?
	if (! install_dir)
	{
#ifdef WIN32
	SYS_ASSERT(home_dir);

	install_dir = StringDup(home_dir);

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

#if 0  // DEBUG
		fprintf(stderr, "Trying install path: [%s]\n", install_dir);
		fprintf(stderr, "   looking for file: [%s]\n\n", filename);
#endif

		bool exists = FileExists(filename);

		StringFree(filename);

		if (exists)
			break;

		StringFree(install_dir);
		install_dir = NULL;
	}
#endif
	}

	if (! install_dir)
		FatalError("Unable to find install directory!\n");

#if 1  // DEBUG
	fprintf(stderr, "Install dir: [%s]\n", install_dir);
#endif
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


static bool InitFLTK()
{
	/*
	 *  Create the window
	 */
	Fl::visual(FL_RGB);

	// FIXME: CONFIG OPTION
	bool alternate_look = false;

	if (! alternate_look)
	{
		Fl::background(236, 232, 228);
		Fl::background2(255, 255, 255);
		Fl::foreground(0, 0, 0);

		Fl::scheme("plastic");
	}

#ifdef UNIX
	Fl_File_Icon::load_system_icons();
#endif


	int screen_w = Fl::w();
	int screen_h = Fl::h();

	fprintf(stderr, "-- SCREEN SIZE %dx%d\n", screen_w, screen_h);

	KF = 1;
#if 0  // TODO
	KF = 0;
	if (screen_w >= 1000) KF = 1;
	if (screen_w >= 1180) KF = 2;
#endif

	KF_fonth = (14 + KF * 2);


	main_win = new UI_MainWin();

	// show window (pass some dummy arguments)
	{
		int   argc = 1;
		char *argv[2];

		argv[0] = strdup("Eureka.exe");
		argv[1] = NULL;

		main_win->show(argc, argv);
	}

	// kill the stupid bright background of the "plastic" scheme
	if (! alternate_look)
	{
		delete Fl::scheme_bg_;
		Fl::scheme_bg_ = NULL;

		main_win->image(NULL);
	}

    Fl::add_handler(Main_key_handler);

	SetWindowSize (main_win->canvas->w(), main_win->canvas->h());

	return true;
}


static void TermFLTK()
{
}


const char *SearchDirForIWAD(const char *dir_name)
{
	LogPrintf("Looking for IWAD in dir: %s\n", dir_name);

	// TODO: have an iwad list in EUREKA.CFG, try each in list

	char name_buf[FL_PATH_MAX];

	sprintf(name_buf, "%s/%s", dir_name, "doom2.wad");

	if (FileExists(name_buf))
		return strdup(name_buf);

	return NULL;
}


const char *DetermineIWAD()
{
	const char *path;

	// handle -iwad parameter
	// TODO: if has no path _and_ not exist _and_ $DOOMWADDIR exists
	//       THEN try prepending $DOOMWADDIR
	if (Iwad)
	{
		// handle a directory name
		if (fl_filename_isdir(Iwad))
		{
			path = SearchDirForIWAD(Iwad);
			if (path)
				return path;

			FatalError("Unable to find any IWAD in directory: %s\n", Iwad);
		}
		else
		{
			// FIXME: if extension is missing, add ".wad"

			return Iwad;
		}
	}

	// FIXME: "standard" locations....

	const char *doomwaddir = getenv("DOOMWADDIR");

	if (doomwaddir)
	{
		doomwaddir = strdup(doomwaddir);

		path = SearchDirForIWAD(doomwaddir);
		if (path)
			return path;
	}

	path = SearchDirForIWAD(".");
	if (path)
		return path;

	FatalError("Unable to find any IWAD\n");
	return "";
}


const char *DetermineGame(const char *iwad_name)
{
	// FIXME: allow override via -game parameter

	char game_name[FL_PATH_MAX];

	strcpy(game_name, fl_filename_name(iwad_name));

	fl_filename_setext(game_name, "");

	LogPrintf("IWAD name: '%s'\n", iwad_name);
	LogPrintf("Game name: '%s'\n", game_name);

	return strdup(game_name);
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


/*
   the driving program
*/
int main(int argc, char *argv[])
{
	init_progress = 0;

	int r;

	// a quick pass through the command line arguments
	// to handle special options, like --help, --install, --config
	r = parse_command_line_options(argc - 1, argv + 1, 1);
	if (r)
		exit(3);

	if (show_help)
	{
		puts ("# Eureka help... \n");  // FIXME
		return 0;
	}
	else if (show_version)
	{
		puts ("# Eureka version \n");  // FIXME
		return 0;
	}

	//printf ("%s\n", what ());


	LogOpen(log_file);

	init_progress = 1;


	LogPrintf("\n");
	LogPrintf("*** " EUREKA_TITLE " v" EUREKA_VERSION " ***\n");
	LogPrintf("\n");

	// Sanity checks (useful when porting).
	check_types();


	Determine_HomeDir(argv[0]);
	Determine_InstallPath(argv[0]);


	// A config file can provides some values
	if (config_file)
		r = parse_config_file_user(config_file);
	else
		r = parse_config_file_default();

	if (r == 0)
	{
		// Environment variables can override them
		r = parse_environment_vars();
	}

	if (r == 0)
	{
		// And command line arguments can override both
		r = parse_command_line_options(argc - 1, argv + 1, 2);
	}

	if (r != 0)
	{
		fprintf(stderr, "Error parsing config or cmd-line options.\n");
		exit(1);
	}


	init_progress = 2;


	// determine IWAD and GAME name
	const char *iwad_name = DetermineIWAD();

	Game_name = DetermineGame(iwad_name);

	if (! Port_name)
		Port_name = "boom";


	// Load game definitions (*.ygd).
	InitDefinitions();

	LoadDefinitions("games", Game_name);
	LoadDefinitions("ports", Port_name);

	base_wad = Wad_file::Open(iwad_name, 'r');  // FIXME: read_only
	if (! base_wad)
		FatalError("Failed to open IWAD: %s\n", iwad_name);
	
	MasterDir_Add(base_wad);


	// TODO!!!  open resource wads (-merge option)

	if (Pwad)
	{
		if (! FileExists(Pwad))
			FatalError("Given pwad does not exist: %s\n", Pwad);

		edit_wad = Wad_file::Open(Pwad, 'a');

		if (! edit_wad)
			FatalError("Cannot load pwad: %s\n", Pwad);

		MasterDir_Add(edit_wad);
	}

	if (! Level_name || ! Level_name[0])
		Level_name = "MAP01";  // FIXME


	W_LoadPalette();

	Editor_Init();


    if (! InitFLTK())
        exit(9);

	init_progress = 3;


	W_LoadFlats();
	W_LoadTextures();
	W_LoadColormap();

	main_win->browser->Populate();


    LogPrintf("Loading initial map : %s\n", Level_name);

    LoadLevel(edit_wad ? edit_wad : base_wad, Level_name);

	main_win->UpdateTotals();


	Editor_Loop();


// FIXME: use dialog box instead
	if (remind_to_build_nodes)
		printf ("\n"
				"** You have made changes to one or more wads. Don't forget to pass\n"
				"** them through a nodes builder (E.G. BSP) before running them.\n"
				"** Like this: \"ybsp foo.wad -o tmp.wad; doom -file tmp.wad\"\n\n");


	LogPrintf("Quit!\n");

    TermFLTK();

	BA_ClearAll();

	W_ClearTextures();

	/* that's all, folks! */
// TODO	CloseWadFiles();
	FreeDefinitions();

	LogClose();

	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
