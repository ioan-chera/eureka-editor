//------------------------------------------------------------------------
//  MAIN PROGRAM
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2020 Andrew Apted
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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include <time.h>
#include <stdexcept>

#include "im_color.h"
#include "m_config.h"
#include "m_game.h"
#include "m_files.h"
#include "m_loadsave.h"

#include "e_main.h"
#include "m_events.h"
#include "m_strings.h"
#include "r_render.h"
#include "r_subdiv.h"

#include "w_rawdef.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_about.h"
#include "ui_file.h"

#ifndef WIN32
#include <time.h>
#ifndef __APPLE__
#include <X11/xpm.h>	// for the window icon
#endif
#endif

// IOANCH: be able to call OSX specific routines (needed for ~/Library)
#ifdef __APPLE__
#include "OSXCalls.h"
#endif


//
//  global variables
//

bool global::want_quit = false;
bool global::app_has_focus = false;

SString global::config_file;
SString global::log_file;

SString global::install_dir;
SString global::home_dir;
SString global::cache_dir;

std::vector<SString> global::Pwad_list;

//
// config items
//
bool config::auto_load_recent = false;
bool config::begin_maximized  = false;
bool config::map_scroll_bars  = true;

SString config::default_port = "vanilla";

int config::gui_scheme    = 1;  // gtk+
int config::gui_color_set = 1;  // bright

rgb_color_t config::gui_custom_bg = RGB_MAKE(0xCC, 0xD5, 0xDD);
rgb_color_t config::gui_custom_ig = RGB_MAKE(255, 255, 255);
rgb_color_t config::gui_custom_fg = RGB_MAKE(0, 0, 0);


// Progress during initialisation:
//    0 = nothing yet
//    1 = read early options, set up logging
//    2 = parsed all options, inited FLTK
//    3 = opened the main window
enum class ProgressStatus
{
	nothing,
	early,
	loaded,
	window
};
static ProgressStatus init_progress;

int global::show_help     = 0;
int global::show_version  = 0;


static void RemoveSingleNewlines(SString &buffer)
{
	for (size_t i = 0; i < buffer.length(); ++i)
	{
		if(buffer[i] == '\n' && buffer[i + 1] == '\n')
			while(buffer[i] == '\n')
				i++;
		if(buffer[i] == '\n')
			buffer[i] = ' ';
	}
}


//
//  show an error message and terminate the program
//
void FatalError(EUR_FORMAT_STRING(const char *fmt), ...)
{
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	SString buffer = SString::vprintf(fmt, arg_ptr);
	va_end(arg_ptr);

	// re-entered here? ouch!
	if (global::in_fatal_error)
	{
		fprintf(stderr, "\nERROR LOOP DETECTED!\n");
		fflush(stderr);

		LogClose();
		exit(4);
	}

	// minimise chance of a infinite loop of errors
	global::in_fatal_error = true;

	if (init_progress == ProgressStatus::nothing || global::Quiet || !global::log_file.empty())
	{
		fprintf(stderr, "\nFATAL ERROR: %s", buffer.c_str());
	}

	if (init_progress >= ProgressStatus::early)
	{
		LogPrintf("\nFATAL ERROR: %s", buffer.c_str());
	}

	if (init_progress >= ProgressStatus::loaded)
	{
		RemoveSingleNewlines(buffer);

		DLG_ShowError("%s", buffer.c_str());

		init_progress = ProgressStatus::early;
	}
#ifdef WIN32
	else
	{
		MessageBox(NULL, buffer.c_str(), "Eureka : Error",
		           MB_ICONEXCLAMATION | MB_OK |
				   MB_SYSTEMMODAL | MB_SETFOREGROUND);
	}
#endif

	init_progress = ProgressStatus::nothing;
	global::app_has_focus = false;

	// TODO: ALL instances. This is death.
	gInstance.MasterDir_CloseAll();
	LogClose();

	exit(2);
}

static void CreateHomeDirs()
{
	SYS_ASSERT(!global::home_dir.empty());

	char dir_name[FL_PATH_MAX];

#ifdef __APPLE__
   // IOANCH 20130825: modified to use name-independent calls
	fl_filename_expand(dir_name, OSX_UserDomainDirectory(macOSDirType::library, nullptr).c_str());
	FileMakeDir(dir_name);

	fl_filename_expand(dir_name, OSX_UserDomainDirectory(macOSDirType::libraryAppSupport,
														 nullptr).c_str());
	FileMakeDir(dir_name);

	fl_filename_expand(dir_name, OSX_UserDomainDirectory(macOSDirType::libraryCache,
														 nullptr).c_str());
	FileMakeDir(dir_name);
#endif

	// try to create home_dir (doesn't matter if it already exists)
	FileMakeDir(global::home_dir);
	FileMakeDir(global::cache_dir);

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
		snprintf(dir_name, FL_PATH_MAX, "%s/%s", (i < 2) ? global::cache_dir.c_str() : global::home_dir.c_str(), subdirs[i]);
		dir_name[FL_PATH_MAX-1] = 0;

		FileMakeDir(dir_name);
	}
}


static void Determine_HomeDir(const char *argv0)
{
	// already set by cmd-line option?
	if (global::home_dir.empty())
	{
#if defined(WIN32)
	// get the %APPDATA% location
	// if that fails, use a folder at the EXE location

		wchar_t *wpath = nullptr;
		if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &wpath)))
		{
			global::home_dir = WideToUTF8(wpath) + "\\EurekaEditor";
			CoTaskMemFree(wpath);
		}
		else
		{
			SYS_ASSERT(global::install_dir.good());
			global::home_dir = global::install_dir + "\\app_data";
		}
		wpath = nullptr;
		if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &wpath)))
		{
			global::cache_dir = WideToUTF8(wpath) + "\\EurekaEditor";
			CoTaskMemFree(wpath);
		}

#elif defined(__APPLE__)
	char path[FL_PATH_MAX + 4];

   fl_filename_expand(path, OSX_UserDomainDirectory(macOSDirType::libraryAppSupport,
													"eureka-editor").c_str());
   global::home_dir = path;

   fl_filename_expand(path, OSX_UserDomainDirectory(macOSDirType::libraryCache,
													"eureka-editor").c_str());
		global::cache_dir = path;

#else  // UNIX
	char path[FL_PATH_MAX + 4];

	if (fl_filename_expand(path, "$HOME/.eureka"))
		global::home_dir = path;
#endif
	}

	if (global::home_dir.empty())
		ThrowException("Unable to find home directory!\n");

	if (global::cache_dir.empty())
		global::cache_dir = global::home_dir;

	LogPrintf("Home  dir: %s\n", global::home_dir.c_str());
	LogPrintf("Cache dir: %s\n", global::cache_dir.c_str());

	// create cache directory (etc)
	CreateHomeDirs();

	// determine log filename
	global::log_file = global::home_dir + "/logs.txt";
}


static void Determine_InstallPath(const char *argv0)
{
	// already set by cmd-line option?
	if (global::install_dir.empty())
	{
#ifdef WIN32
		global::install_dir = GetExecutablePath(argv0);

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
		global::install_dir = SString(prefixes[i]) + "/share/eureka";

		SString filename = global::install_dir + "/games/doom2.ugh";

		DebugPrintf("Trying install path: %s\n", global::install_dir.c_str());
		DebugPrintf("   looking for file: %s\n", filename.c_str());

		bool exists = FileExists(filename);

		if (exists)
			break;

		global::install_dir.clear();
	}
#endif
	}

	// fallback : look in current directory
	if (global::install_dir.empty())
	{
		if (FileExists("./games/doom2.ugh"))
			global::install_dir = ".";
	}

	if (global::install_dir.empty())
		ThrowException("Unable to find install directory!\n");

	LogPrintf("Install dir: %s\n", global::install_dir.c_str());
}


SString GameNameFromIWAD(const SString &iwad_name)
{
	char game_name[FL_PATH_MAX];
	StringCopy(game_name, sizeof(game_name), fl_filename_name(iwad_name.c_str()));

	fl_filename_setext(game_name, "");

	y_strlowr(game_name);

	return game_name;
}


static bool DetermineIWAD(Instance &inst)
{
	// this mainly handles a value specified on the command-line,
	// since values in a EUREKA_LUMP are already vetted.  Hence
	// producing a fatal error here is OK.

	if (!inst.Iwad_name.empty() && FilenameIsBare(inst.Iwad_name))
	{
		// a bare name (e.g. "heretic") is treated as a game name

		// make lowercase
		inst.Iwad_name = inst.Iwad_name.asLower();

		if (! M_CanLoadDefinitions("games", inst.Iwad_name))
			ThrowException("Unknown game '%s' (no definition file)\n", inst.Iwad_name.c_str());

		SString path = M_QueryKnownIWAD(inst.Iwad_name);

		if (path.empty())
			ThrowException("Cannot find IWAD for game '%s'\n", inst.Iwad_name.c_str());

		inst.Iwad_name = path;
	}
	else if (!inst.Iwad_name.empty())
	{
		// if extension is missing, add ".wad"
		if (! HasExtension(inst.Iwad_name))
			inst.Iwad_name = ReplaceExtension(inst.Iwad_name, "wad");

		if (! Wad_file::Validate(inst.Iwad_name))
			FatalError("IWAD does not exist or is invalid: %s\n", inst.Iwad_name.c_str());

		SString game = GameNameFromIWAD(inst.Iwad_name);

		if (! M_CanLoadDefinitions("games", game))
			ThrowException("Unknown game '%s' (no definition file)\n", inst.Iwad_name.c_str());

		M_AddKnownIWAD(inst.Iwad_name);
		M_SaveRecent();
	}
	else
	{
		inst.Iwad_name = inst.M_PickDefaultIWAD();

		if (inst.Iwad_name.empty())
		{
			// show the "Missing IWAD!" dialog.
			// if user cancels it, we have no choice but to quit.
			if (! inst.MissingIWAD_Dialog())
				return false;
		}
	}

	inst.Game_name = GameNameFromIWAD(inst.Iwad_name);

	return true;
}


static void DeterminePort(Instance &inst)
{
	// user supplied value?
	// NOTE: values from the EUREKA_LUMP are already verified.
	if (!inst.Port_name.empty())
	{
		if (! M_CanLoadDefinitions("ports", inst.Port_name))
			ThrowException("Unknown port '%s' (no definition file)\n", inst.Port_name.c_str());

		return;
	}

	SString base_game = M_GetBaseGame(inst.Game_name);

	// ensure the 'default_port' value is OK
	if (config::default_port.empty())
	{
		LogPrintf("WARNING: Default port is empty, using vanilla.\n");
		config::default_port = "vanilla";
	}
	else if (! M_CanLoadDefinitions("ports", config::default_port))
	{
		LogPrintf("WARNING: Default port '%s' is unknown, using vanilla.\n",
				  config::default_port.c_str());
		config::default_port = "vanilla";
	}
	else if (! M_CheckPortSupportsGame(base_game, config::default_port))
	{
		LogPrintf("WARNING: Default port '%s' not compatible with '%s'\n",
				  config::default_port.c_str(), inst.Game_name.c_str());
		config::default_port = "vanilla";
	}

	inst.Port_name = config::default_port;
}


static SString DetermineLevel(const Instance &inst)
{
	// most of the logic here is to handle a numeric level number
	// e.g. -warp 15
	//
	// DOOM 1 level number is 10 * episode + map, e.g. 23 --> E2M3
	// (there is logic in command-line parsing to handle two numbers after -warp)

	int level_number = 0;

	if (!inst.Level_name.empty())
	{
		if (! isdigit(inst.Level_name[0]))
			return inst.Level_name.asUpper();

		level_number = atoi(inst.Level_name);
	}

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		Wad_file *wad = (pass == 0) ? inst.edit_wad : inst.game_wad;

		if (! wad)
			continue;

		int lev_num;

		if (level_number > 0)
		{
			lev_num = wad->LevelFindByNumber(level_number);
			if (lev_num < 0)
				ThrowException("Level '%d' not found (no matches)\n", level_number);

		}
		else
		{
			lev_num = wad->LevelFindFirst();
			if (lev_num < 0)
				ThrowException("No levels found in the %s!\n", (pass == 0) ? "PWAD" : "IWAD");
		}

		int idx = wad->LevelHeader(lev_num);

		Lump_c *lump = wad->GetLump(idx);
		SYS_ASSERT(lump);

		return lump->Name();
	}

	// cannot get here
	return "XXX";
}


// this is mainly to prevent ESCAPE key from quitting
int Main_key_handler(int event)
{
	if (event != FL_SHORTCUT)
		return 0;

	if (Fl::event_key() == FL_Escape)
	{
		// TODO: use the currently active instance instead
		gInstance.EV_EscapeKey();
		return 1;
	}

	return 0;
}


// see comment in Main_SetupFLTK...
#if !defined(WIN32) && !defined(__APPLE__)
int x11_check_focus_change(void *xevent, void *data)
{
	// TODO: get multiple windows
	if (gInstance.main_win != NULL)
	{
		const XEvent *xev = (const XEvent *)xevent;

		Window xid = xev->xany.window;

		if (fl_find(xid) == gInstance.main_win)
		{
			switch (xev->type)
			{
				case FocusIn:
					global::app_has_focus = true;
					// fprintf(stderr, "** app_has_focus: TRUE **\n");
					break;

				case FocusOut:
					global::app_has_focus = false;
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

#ifndef NO_OPENGL
	Fl::gl_visual(FL_RGB);
#endif

	// disable keyboard navigation, as it often interferes with our
	// user interface, especially TAB key for toggling the 3D view.
	Fl::option(Fl::OPTION_VISIBLE_FOCUS, false);

	if (config::gui_color_set == 0)
	{
		// use default colors
	}
	else if (config::gui_color_set == 1)
	{
		Fl::background(236, 232, 228);
		Fl::background2(255, 255, 255);
		Fl::foreground(0, 0, 0);
	}
	else
	{
		// custom colors
		Fl::background (RGB_RED(config::gui_custom_bg), RGB_GREEN(config::gui_custom_bg),
						RGB_BLUE(config::gui_custom_bg));
		Fl::background2(RGB_RED(config::gui_custom_ig), RGB_GREEN(config::gui_custom_ig),
						RGB_BLUE(config::gui_custom_ig));
		Fl::foreground (RGB_RED(config::gui_custom_fg), RGB_GREEN(config::gui_custom_fg),
						RGB_BLUE(config::gui_custom_fg));
	}

	if (config::gui_scheme == 0)
	{
		// use default scheme
	}
	else if (config::gui_scheme == 1)
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
}


//
// Creates the main window
//
static void Main_OpenWindow(Instance &inst)
{
	inst.main_win = new UI_MainWindow(inst);

	inst.main_win->label("Eureka v" EUREKA_VERSION);

#if !defined(__APPLE__) && !defined(_WIN32)
#include "../misc/eureka.xpm"

	// needed if display has not been previously opened
	fl_open_display();

	Pixmap pm, mask;
	std::vector<char> localmodified;
	localmodified.resize(sizeof(logo_E4_32x32_xpm));
	memcpy(localmodified.data(), logo_E4_32x32_xpm, localmodified.size());
	XpmCreatePixmapFromData(fl_display, DefaultRootWindow(fl_display),
	    const_cast<char**>(logo_E4_32x32_xpm), &pm, &mask, NULL);
	inst.main_win->icon((char *)pm);
#endif

	// show window (pass some dummy arguments)
	{
		int   argc = 1;
		char *argv[2];

		argv[0] = StringDup("Eureka.exe");
		argv[1] = NULL;

		inst.main_win->show(argc, argv);
#ifndef NO_OPENGL
		inst.main_win->canvas->show();  // needed for OpenGL
#endif
		global::app_has_focus = true;
	}

#if !defined(__APPLE__) && !defined(_WIN32)
	// read in the current window hints, then modify them to
	// support icon transparency (make sure that transparency
	// mask is enabled in the XPM icon)
	XWMHints* hints = XGetWMHints(fl_display, fl_xid(inst.main_win));
	hints->flags |= IconMaskHint;
	hints->icon_mask = mask;
	XSetWMHints(fl_display, fl_xid(inst.main_win), hints);
	XFree(hints);
#endif

	// kill the stupid bright background of the "plastic" scheme
	if (config::gui_scheme == 2)
	{
		delete Fl::scheme_bg_;
		Fl::scheme_bg_ = NULL;

		inst.main_win->image(NULL);
	}

	Fl::check();

	InitAboutDialog();

	if (config::begin_maximized)
		inst.main_win->Maximize();

	log_viewer = new UI_LogViewer(gInstance);

	LogOpenWindow();

	Fl::add_handler(Main_key_handler);

	inst.main_win->BrowserMode(0);
	inst.main_win->NewEditMode(inst.edit.mode);

	// allow processing keyboard events, even before the mouse
	// pointer has entered our window.
	Fl::focus(inst.main_win->canvas);

	Fl::check();
}


void Main_Quit()
{
	global::want_quit = true;
}


// used for 'New Map' / 'Open Map' functions too
bool Instance::Main_ConfirmQuit(const char *action) const
{
	if (! MadeChanges)
		return true;

	SString secondButton = SString::printf("&%s", action);
	// convert action string like "open a new map" to a simple "Open"
	// string for the yes choice.
	if (secondButton.size() >= 2)
	{
		secondButton[1] = static_cast<char>(toupper(secondButton[1]));
		size_t pos = secondButton.find(' ');
		if (pos != SString::npos)
			secondButton.erase(pos, SString::npos);
	}

	if (DLG_Confirm({ "Cancel", secondButton },
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
SString Instance::Main_FileOpFolder() const
{
	if (Pwad_name.good())
		return FilenameGetPath(Pwad_name);

	return "";
}


void Main_Loop()
{
	// TODO: must think this through
	gInstance.RedrawMap();

	for (;;)
	{
		// TODO: determine the active instance
		if (gInstance.edit.is_navigating)
		{
			gInstance.Nav_Navigate();

			Fl::wait(0);

			if (global::want_quit)
				break;
		}
		else
		{
			Fl::wait(0.2);
		}

		if (global::want_quit)
		{
			if (gInstance.Main_ConfirmQuit("quit"))
				break;

			global::want_quit = false;
		}

		// TODO: handle these in a better way

		// TODO: HANDLE ALL INSTANCES
		gInstance.main_win->UpdateTitle(gInstance.MadeChanges ? '*' : 0);

		gInstance.main_win->scroll->UpdateBounds();

		if (gInstance.edit.Selected->empty())
			gInstance.edit.error_mode = false;
	}
}


void Instance::LoadResourceFile(const SString &filename)
{
	// support loading "ugh" config files
	if (MatchExtension(filename, "ugh"))
	{
		M_ParseDefinitionFile(this, PURPOSE_Resource, nullptr, filename);
		return;
	}

	if (!Wad_file::Validate(filename))
		throw WadReadException("Invalid WAD file: " + filename);

	Wad_file *wad = Wad_file::Open(filename, WadOpenMode::read);

	if (!wad)
		throw WadReadException("Cannot load resource: " + filename);

	MasterDir_Add(wad);
}


bool Instance::Main_LoadIWAD()
{
	// Load the IWAD (read only).
	// The filename has been checked in DetermineIWAD().
	Wad_file *wad = Wad_file::Open(Iwad_name, WadOpenMode::read);
	if (!wad)
	{
		LogPrintf("Failed to open game IWAD: %s\n", Iwad_name.c_str());
		return false;
	}
	game_wad = wad;

	MasterDir_Add(game_wad);
	return true;
}


void Instance::ReadGameInfo()
{
	Game_name = GameNameFromIWAD(Iwad_name);

	LogPrintf("Game name: '%s'\n", Game_name.c_str());
	LogPrintf("IWAD file: '%s'\n", Iwad_name.c_str());

	M_LoadDefinitions("games", Game_name);
}


void Instance::ReadPortInfo()
{
	// we assume that the port name is valid, i.e. a config file
	// exists for it.  That is checked by DeterminePort() and
	// the EUREKA_LUMP parsing code.

	SYS_ASSERT(!Port_name.empty());

	SString base_game = M_GetBaseGame(Game_name);

	// warn user if this port is incompatible with the game
	if (! M_CheckPortSupportsGame(base_game, Port_name))
	{
		LogPrintf("WARNING: the port '%s' is not compatible with the game '%s'\n",
			Port_name.c_str(), Game_name.c_str());

		int res = DLG_Confirm({ "&vanilla", "No Change" },
						"Warning: the given port '%s' is not compatible with "
						"this game (%s)."
						"\n\n"
						"To prevent seeing invalid line and sector types, "
						"it is recommended to reset the port to something valid.\n"
						"Select a new port now?",
			Port_name.c_str(), Game_name.c_str());

		if (res == 0)
		{
			Port_name = "vanilla";
		}
	}

	LogPrintf("Port name: '%s'\n", Port_name.c_str());

	M_LoadDefinitions("ports", Port_name);

	// prevent UI weirdness if the port is forced to BOOM / MBF
	if (Features.strife_flags)
	{
		Features.pass_through = 0;
		Features.coop_dm_flags = 0;
		Features.friend_flag = 0;
	}
}


//
// load all game/port definitions (*.ugh).
// open all wads in the master directory.
// read important content from the wads (palette, textures, etc).
//
void Instance::Main_LoadResources()
{
	LogPrintf("\n");
	LogPrintf("----- Loading Resources -----\n");

	M_ClearAllDefinitions();

	// clear the parse variables, pre-set a few vars
	M_PrepareConfigVariables();

	ReadGameInfo();
	ReadPortInfo();

	// reset the master directory
	if (edit_wad)
		MasterDir_Remove(edit_wad);

	MasterDir_CloseAll();

	// TODO: check result
	Main_LoadIWAD();

	// load all resource wads
	for (const SString &resource : Resource_list)
	{
		try
		{
			LoadResourceFile(resource);
		}
		catch (const WadReadException& e)
		{
			LogPrintf("%s\n", e.what());
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

	LogPrintf("--- DONE ---\n");
	LogPrintf("\n");

	// reset sector info (for slopes and 3D floors)
	Subdiv_InvalidateAll();

	if (main_win)
	{
		// kill all loaded OpenGL images
		if (main_win->canvas)
			main_win->canvas->DeleteContext();

		main_win->UpdateGameInfo();

		main_win->browser->Populate();

		// TODO: only call this when the IWAD has changed
		Props_LoadValues(*this);
	}
}


/* ----- user information ----------------------------- */

static void ShowHelp()
{
	printf(	"\n"
			"*** " EUREKA_TITLE " v" EUREKA_VERSION " (C) 2020 The Eureka Team ***\n"
			"\n");

	printf(	"Eureka is free software, under the terms of the GNU General\n"
			"Public License (GPL), and comes with ABSOLUTELY NO WARRANTY.\n"
			"Home page: http://eureka-editor.sf.net/\n"
			"\n");

	printf( "USAGE: eureka [options...] [FILE...]\n"
			"\n"
			"Available options are:\n");

	M_PrintCommandLineOptions();

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
	try
	{
		init_progress = ProgressStatus::nothing;


		// a quick pass through the command line arguments
		// to handle special options, like --help, --install, --config
		M_ParseCommandLine(argc - 1, argv + 1, 1);

		if (global::show_help)
		{
			ShowHelp();
			return 0;
		}
		else if (global::show_version)
		{
			ShowVersion();
			return 0;
		}

		init_progress = ProgressStatus::early;


		LogPrintf("\n");
		LogPrintf("*** " EUREKA_TITLE " v" EUREKA_VERSION " (C) 2020 The Eureka Team ***\n");
		LogPrintf("\n");

		// sanity checks type sizes (useful when porting)
		CheckTypeSizes();

		ShowTime();


		Determine_InstallPath(argv[0]);
		Determine_HomeDir(argv[0]);

		LogOpenFile(global::log_file.c_str());


		// load all the config settings
		M_ParseConfigFile();

		// environment variables can override them
		M_ParseEnvironmentVars();

		// and command line arguments will override both
		M_ParseCommandLine(argc - 1, argv + 1, 2);

		// TODO: create a new instance
		gInstance.Editor_Init();

		Main_SetupFLTK();

		init_progress = ProgressStatus::loaded;


		M_LoadRecent();
		M_LoadBindings();

		M_LookForIWADs();

		Main_OpenWindow(gInstance);

		init_progress = ProgressStatus::window;

		gInstance.M_LoadOperationMenus();


		// open a specified PWAD now
		// [ the map is loaded later.... ]

		if (!global::Pwad_list.empty())
		{
			// this fatal errors on any missing file
			// [ hence the Open() below is very unlikely to fail ]
			M_ValidateGivenFiles();

			gInstance.Pwad_name = global::Pwad_list[0];

			// TODO: main instance
			gInstance.edit_wad = Wad_file::Open(gInstance.Pwad_name, WadOpenMode::append);
			if (!gInstance.edit_wad)
				ThrowException("Cannot load pwad: %s\n", gInstance.Pwad_name.c_str());

			// Note: the Main_LoadResources() call will ensure this gets
			//       placed at the correct spot (at the end)
			gInstance.MasterDir_Add(gInstance.edit_wad);
		}
		// don't auto-load when --iwad or --warp was used on the command line
		else if (config::auto_load_recent && ! (!gInstance.Iwad_name.empty() || !gInstance.Level_name.empty()))
		{
			if (gInstance.M_TryOpenMostRecent())
			{
				gInstance.MasterDir_Add(gInstance.edit_wad);
			}
		}


		// Handle the '__EUREKA' lump.  It is almost equivalent to using the
		// -iwad, -merge and -port command line options, but with extra
		// checks (to allow editing a wad containing dud information).
		//
		// Note: there is logic in M_ParseEurekaLump() to ensure that command
		// line arguments can override the EUREKA_LUMP values.

		if (gInstance.edit_wad)
		{
			if (! gInstance.M_ParseEurekaLump(gInstance.edit_wad, true /* keep_cmd_line_args */))
			{
				// user cancelled the load
				gInstance.RemoveEditWad();
			}
		}


		// determine which IWAD to use
		// TODO: instance management
		if (! DetermineIWAD(gInstance))
			goto quit;

		DeterminePort(gInstance);

		// temporarily load the iwad, the following few functions need it.
		// it will get loaded again in Main_LoadResources().
		// TODO: check result
		gInstance.Main_LoadIWAD();


		// load the initial level
		// TODO: first instance
		gInstance.Level_name = DetermineLevel(gInstance);

		LogPrintf("Loading initial map : %s\n", gInstance.Level_name.c_str());

		// TODO: the first instance
		gInstance.LoadLevel(gInstance.edit_wad ? gInstance.edit_wad : gInstance.game_wad, gInstance.Level_name);

		// do this *after* loading the level, since config file parsing
		// can depend on the map format and UDMF namespace.
		gInstance.Main_LoadResources();	// TODO: instance management


		Main_Loop();

	quit:
		/* that's all folks! */

		LogPrintf("Quit\n");

		init_progress = ProgressStatus::nothing;
		global::app_has_focus = false;

		// TODO: all instances
		gInstance.MasterDir_CloseAll();
		LogClose();

		return 0;
	}
	catch(const std::exception &e)
	{
		FatalError("%s\n", e.what());
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
