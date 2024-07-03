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
#include <memory>
#include <stdexcept>

#include "im_color.h"
#include "m_config.h"
#include "m_game.h"
#include "m_files.h"
#include "m_loadsave.h"
#include "m_testmap.h"

#include "e_main.h"
#include "m_events.h"
#include "m_strings.h"
#include "r_render.h"
#include "r_subdiv.h"

#include "w_dehacked.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_about.h"
#include "ui_file.h"

#ifndef WIN32
#include <time.h>
#ifndef __APPLE__

#ifndef Bool
#define Bool int	// We need some pollutants back for the following include
#endif

#include <X11/xpm.h>	// for the window icon

#undef Bool

#endif
#endif

// IOANCH: be able to call OSX specific routines (needed for ~/Library)
#ifdef __APPLE__
#include "OSXCalls.h"
#include <signal.h>
#elif defined(_WIN32)
#else
#include <sys/types.h>
#include <sys/wait.h>
#endif


//
//  global variables
//

bool global::want_quit = false;
bool global::app_has_focus = false;

namespace signalling
{
#ifndef _WIN32
	static bool hasChildProcessStatus;
	static int childProcessStatus;
#endif
}

fs::path global::config_file;
fs::path global::log_file;

fs::path global::install_dir;
fs::path global::home_dir;
fs::path global::cache_dir;

std::vector<fs::path> global::Pwad_list;

//
// config items
//
bool config::auto_load_recent = false;
bool config::begin_maximized  = false;
bool config::map_scroll_bars  = true;

SString config::default_port = "vanilla";

int config::gui_scheme    = 1;  // gtk+
int config::gui_color_set = 1;  // bright

rgb_color_t config::gui_custom_bg = rgbMake(0xCC, 0xD5, 0xDD);
rgb_color_t config::gui_custom_ig = rgbMake(255, 255, 255);
rgb_color_t config::gui_custom_fg = rgbMake(0, 0, 0);


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

bool global::show_help;
bool global::show_version;


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

		gLog.close();
		exit(4);
	}

	// minimise chance of a infinite loop of errors
	global::in_fatal_error = true;
	gLog.markFatalError();

	if (init_progress == ProgressStatus::nothing || global::Quiet || !global::log_file.empty())
	{
		fprintf(stderr, "\nFATAL ERROR: %s", buffer.c_str());
	}

	if (init_progress >= ProgressStatus::early)
	{
		gLog.printf("\nFATAL ERROR: %s\n", buffer.c_str());
	}

	if (init_progress >= ProgressStatus::loaded)
	{
		RemoveSingleNewlines(buffer);

		DLG_ShowError(true, "%s", buffer.c_str());

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
	gLog.close();

	exit(2);
}

static void CreateHomeDirs()
{
	SYS_ASSERT(!global::home_dir.empty());

	fs::path dir_name;

#ifdef __APPLE__
   // IOANCH 20130825: modified to use name-independent calls
	dir_name = OSX_UserDomainDirectory(macOSDirType::library, nullptr);
	FileMakeDir(dir_name);

	dir_name = OSX_UserDomainDirectory(macOSDirType::libraryAppSupport, nullptr);
	FileMakeDir(dir_name);

	dir_name = OSX_UserDomainDirectory(macOSDirType::libraryCache, nullptr);
	FileMakeDir(dir_name);
#endif

	// try to create home_dir (doesn't matter if it already exists)
	FileMakeDir(global::home_dir);
	FileMakeDir(global::cache_dir);

	static const fs::path subdirs[] =
	{
		// these under $cache_dir
		"cache", "backups",

		// these under $home_dir
		"iwads", "games", "ports"
	};

	for (int i = 0 ; i < (int)lengthof(subdirs) ; i++)
	{
		dir_name = (i < 2 ? global::cache_dir : global::home_dir) / subdirs[i];
		FileMakeDir(dir_name);
	}
}


static void Determine_HomeDir(const char *argv0) noexcept(false)
{
	// already set by cmd-line option?
	if (global::home_dir.empty())
	{
#if defined(WIN32)
	// get the %APPDATA% location
	// if that fails, use a folder at the EXE location

		wchar_t *wpath = nullptr;
		if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr,
										  &wpath)))
		{
			global::home_dir = fs::path(wpath) / "EurekaEditor";
			CoTaskMemFree(wpath);
		}
		else
		{
			SYS_ASSERT(!global::install_dir.empty());
			global::home_dir = global::install_dir / "app_data";
		}
		wpath = nullptr;
		if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr,
										  &wpath)))
		{
			global::cache_dir = fs::path(wpath) / "EurekaEditor";
			CoTaskMemFree(wpath);
		}

#elif defined(__APPLE__)
		fs::path path = OSX_UserDomainDirectory(macOSDirType::libraryAppSupport, "eureka-editor");
		global::home_dir = path;

		path = OSX_UserDomainDirectory(macOSDirType::libraryCache, "eureka-editor");
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

	gLog.printf("Home  dir: %s\n", global::home_dir.u8string().c_str());
	gLog.printf("Cache dir: %s\n", global::cache_dir.u8string().c_str());

	// create cache directory (etc)
	CreateHomeDirs();

	// determine log filename
	global::log_file = global::home_dir / "logs.txt";
}


static void Determine_InstallPath(const char *argv0) noexcept(false)
{
	// already set by cmd-line option?
	if (global::install_dir.empty())
	{
#ifdef WIN32
		global::install_dir = GetExecutablePath(argv0);

#else
		static const fs::path prefixes[] =
		{
			"/usr/local",
			"/usr",
			"/opt",
		};

		for (const fs::path &prefix : prefixes)
		{
			global::install_dir = prefix / "share" / "eureka";

			fs::path filename = global::install_dir / "games" / "doom2.ugh";

			gLog.debugPrintf("Trying install path: %s\n",
							 global::install_dir.u8string().c_str());
			gLog.debugPrintf("   looking for file: %s\n", filename.u8string().c_str());

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

	gLog.printf("Install dir: %s\n", global::install_dir.u8string().c_str());
}


SString GameNameFromIWAD(const fs::path &iwad_name)
{
	return SString(iwad_name.stem().u8string()).asLower();
}


static bool DetermineIWAD(Instance &inst)
{
	// this mainly handles a value specified on the command-line,
	// since values in a EUREKA_LUMP are already vetted.  Hence
	// producing a fatal error here is OK.

	if (!inst.loaded.iwadName.empty() && FilenameIsBare(inst.loaded.iwadName))
	{
		// a bare name (e.g. "heretic") is treated as a game name
		SString game = SString(inst.loaded.iwadName.u8string()).asLower();

		// make lowercase
		inst.loaded.iwadName = fs::u8path(game.get());

		if (! M_CanLoadDefinitions(global::home_dir, global::install_dir, GAMES_DIR, game))
			ThrowException("Unknown game '%s' (no definition file)\n", game.c_str());

		const fs::path *path = global::recent.queryIWAD(game);

		if (!path)
			ThrowException("Cannot find IWAD for game '%s'\n", game.c_str());

		inst.loaded.iwadName = *path;
	}
	else if (!inst.loaded.iwadName.empty())
	{
		// if extension is missing, add ".wad"
		if (! HasExtension(inst.loaded.iwadName))
			inst.loaded.iwadName = ReplaceExtension(inst.loaded.iwadName, "wad");

		if (! Wad_file::Validate(inst.loaded.iwadName))
			ThrowException("IWAD does not exist or is invalid: %s\n", inst.loaded.iwadName.u8string().c_str());

		SString game = GameNameFromIWAD(inst.loaded.iwadName);

		if (! M_CanLoadDefinitions(global::home_dir, global::install_dir, GAMES_DIR, game))
			ThrowException("Unknown game '%s' (no definition file)\n", inst.loaded.iwadName.u8string().c_str());

		global::recent.addIWAD(inst.loaded.iwadName);
		global::recent.save(global::home_dir);
	}
	else
	{
		inst.loaded.iwadName = inst.M_PickDefaultIWAD();

		if (inst.loaded.iwadName.empty())
		{
			// show the "Missing IWAD!" dialog.
			// if user cancels it, we have no choice but to quit.
			if (! inst.MissingIWAD_Dialog())
				return false;
		}
	}

	inst.loaded.gameName = GameNameFromIWAD(inst.loaded.iwadName);

	return true;
}


static void DeterminePort(Instance &inst)
{
	// user supplied value?
	// NOTE: values from the EUREKA_LUMP are already verified.
	if (!inst.loaded.portName.empty())
	{
		if (! M_CanLoadDefinitions(global::home_dir, global::install_dir, PORTS_DIR, inst.loaded.portName))
			ThrowException("Unknown port '%s' (no definition file)\n",
						   inst.loaded.portName.c_str());

		return;
	}

	SString base_game = M_GetBaseGame(inst.loaded.gameName);

	// ensure the 'default_port' value is OK
	if (config::default_port.empty())
	{
		gLog.printf("WARNING: Default port is empty, using vanilla.\n");
		config::default_port = "vanilla";
	}
	else if (! M_CanLoadDefinitions(global::home_dir, global::install_dir, PORTS_DIR, config::default_port))
	{
		gLog.printf("WARNING: Default port '%s' is unknown, using vanilla.\n",
				  config::default_port.c_str());
		config::default_port = "vanilla";
	}
	else if (! M_CheckPortSupportsGame(base_game, config::default_port))
	{
		gLog.printf("WARNING: Default port '%s' not compatible with '%s'\n",
				  config::default_port.c_str(), inst.loaded.gameName.c_str());
		config::default_port = "vanilla";
	}

	inst.loaded.portName = config::default_port;
	if(inst.main_win)
		testmap::updateMenuName(inst.main_win->menu_bar, inst.loaded);
}


static SString DetermineLevel(const Instance &inst)
{
	// most of the logic here is to handle a numeric level number
	// e.g. -warp 15
	//
	// DOOM 1 level number is 10 * episode + map, e.g. 23 --> E2M3
	// (there is logic in command-line parsing to handle two numbers after -warp)

	int level_number = 0;

	if (!inst.loaded.levelName.empty())
	{
		if (! isdigit(inst.loaded.levelName[0]))
			return inst.loaded.levelName.asUpper();

		level_number = atoi(inst.loaded.levelName);
	}

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		std::shared_ptr<Wad_file> wad = (pass == 0) ? inst.wad.master.editWad() : inst.wad.master.gameWad();

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
		gInstance->EV_EscapeKey();
		return 1;
	}

	return 0;
}


// see comment in Main_SetupFLTK...
#if !defined(WIN32) && !defined(__APPLE__)
int x11_check_focus_change(void *xevent, void *data)
{
	// TODO: get multiple windows
	if (gInstance->main_win != NULL)
	{
		const XEvent *xev = (const XEvent *)xevent;

		Window xid = xev->xany.window;

		if (fl_find(xid) == gInstance->main_win)
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

	gLog.printf("Detected Screen Size: %dx%d\n", screen_w, screen_h);
}


//
// Creates the main window
//
static void Main_OpenWindow(Instance &inst)
{
	inst.main_win = new UI_MainWindow(inst);

	// Set menu bindings now that we have them.
	menu::updateBindings(inst.main_win->menu_bar);

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
#ifdef _WIN32
#include "../misc/eureka.xpm"
	Fl_Pixmap pixmap(logo_E4_32x32_xpm);
	Fl_RGB_Image image(&pixmap);
	inst.main_win->icon(&image);
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

	log_viewer = new UI_LogViewer(*gInstance);

	gLog.openWindow([](const SString &text, void *userData)
                    {
        LogViewer_AddLine(text.c_str());
    }, nullptr);

	Fl::add_handler(Main_key_handler);

	inst.main_win->BrowserMode(BrowserMode::hide);
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
bool Document::Main_ConfirmQuit(const char *action) const
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
fs::path Instance::Main_FileOpFolder() const
{
	if (wad.master.editWad())
		return FilenameGetPath(wad.master.editWad()->PathName());

	return "";
}


static void updateStatusByChildProcesses(const Instance &inst)
{
#ifdef _WIN32
#else
	if(signalling::hasChildProcessStatus)
	{
		signalling::hasChildProcessStatus = false;
		int status = signalling::childProcessStatus;

		SString message;

		if(WIFEXITED(status))
		{
			message = SString::printf("Exited with status %d", WEXITSTATUS(status));
		}
		else if(WIFSIGNALED(status))
		{
			message = SString::printf("Signalled by %d", WTERMSIG(status));
		}
		if(message.good())
		{
			inst.Status_Set("%s", message.c_str());
			gLog.printf("--> %s\n", message.c_str());
		}
	}
#endif
}


void Main_Loop()
{
	// TODO: must think this through
	gInstance->RedrawMap();

	for (;;)
	{
		// TODO: determine the active instance
		if (gInstance->edit.is_navigating)
		{
			gInstance->Nav_Navigate();

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
			if (gInstance->level.Main_ConfirmQuit("quit"))
				break;

			global::want_quit = false;
		}

		// TODO: handle these in a better way

		// TODO: HANDLE ALL INSTANCES
		gInstance->main_win->UpdateTitle(gInstance->level.MadeChanges ? '*' : 0);

		gInstance->main_win->scroll->UpdateBounds();

		if (gInstance->edit.Selected->empty())
			gInstance->edit.error_mode = false;

		updateStatusByChildProcesses(*gInstance);
	}
}


static void readGameInfo(std::unordered_map<SString, SString> &parseVars, LoadingData &loading, ConfigData &config)
		noexcept(false)
{
	loading.gameName = GameNameFromIWAD(loading.iwadName);

	gLog.printf("Game name: '%s'\n", loading.gameName.c_str());
	gLog.printf("IWAD file: '%s'\n", loading.iwadName.u8string().c_str());

	readConfiguration(parseVars, "games", loading.gameName, config);
}


static void readPortInfo(std::unordered_map<SString, SString> &parseVars, LoadingData &loading, ConfigData &config) noexcept(false)
{
	// we assume that the port name is valid, i.e. a config file
	// exists for it.  That is checked by DeterminePort() and
	// the EUREKA_LUMP parsing code.

	SYS_ASSERT(!loading.portName.empty());

	SString base_game = M_GetBaseGame(loading.gameName);

	// warn user if this port is incompatible with the game
	if (! M_CheckPortSupportsGame(base_game, loading.portName))
	{
		gLog.printf("WARNING: the port '%s' is not compatible with the game "
					"'%s'\n", loading.portName.c_str(), loading.gameName.c_str());

		int res = DLG_Confirm({ "&vanilla", "No Change" },
							  "Warning: the given port '%s' is not compatible "
							  "with this game (%s)."
							  "\n\n"
							  "To prevent seeing invalid line and sector "
							  "types, it is recommended to reset the port to "
							  "something valid.\n"
							  "Select a new port now?",
							  loading.portName.c_str(), loading.gameName.c_str());

		if (res == 0)
		{
			loading.portName = "vanilla";
		}
	}

	gLog.printf("Port name: '%s'\n", loading.portName.c_str());

	readConfiguration(parseVars, "ports", loading.portName, config);

	// prevent UI weirdness if the port is forced to BOOM / MBF
	if (config.features.strife_flags)
	{
		config.features.pass_through = 0;
		config.features.coop_dm_flags = 0;
		config.features.friend_flag = 0;
	}
}

NewResources loadResources(const LoadingData& loading, const WadData &waddata) noexcept(false)
{
	auto newres = NewResources();
	newres.loading = loading;
	newres.waddata = waddata;

	gLog.printf("\n");
	gLog.printf("----- Loading Resources -----\n");

	// clear the parse variables, pre-set a few vars
	std::unordered_map<SString, SString> parseVars = loading.prepareConfigVariables();
	std::vector<std::shared_ptr<Wad_file>> resourceWads;

	try
	{
		readGameInfo(parseVars, newres.loading, newres.config);
		readPortInfo(parseVars, newres.loading, newres.config);
	

		for (const fs::path& resource : newres.loading.resourceList)
		{
			if(fs::is_directory(resource))
			{
				std::shared_ptr<Wad_file> wad = Wad_file::readFromDir(resource);
				if(!wad)
					gLog.printf("Failed opening directory: %s\n", resource.u8string().c_str());
				else
				{
					resourceWads.push_back(wad);
				}
				continue;
			}
			if (MatchExtensionNoCase(resource, ".ugh"))
			{
				M_ParseDefinitionFile(parseVars, ParsePurpose::resource,
					&newres.config, resource);
				continue;
			}
			if(MatchExtensionNoCase(resource, ".deh") || MatchExtensionNoCase(resource, ".bex"))
			{
				if(!dehacked::loadFile(resource, newres.config))
					gLog.printf("Error loading Dehacked file %s\n", resource.u8string().c_str());
				continue;
			}
			// Otherwise wad
			if (!Wad_file::Validate(resource))
				ThrowException("Invalid resource WAD file: %s", resource.u8string().c_str());

			std::shared_ptr<Wad_file> wad = Wad_file::Open(resource,
				WadOpenMode::read);
			if (!wad)
				ThrowException("Cannot load resource: %s", resource.u8string().c_str());

			resourceWads.push_back(wad);
		}

		std::shared_ptr<Wad_file> gameWad = Wad_file::Open(newres.loading.iwadName, WadOpenMode::read);
		if (!gameWad)
			ThrowException("Could not load IWAD file");

		newres.waddata.reloadResources(gameWad, newres.config, resourceWads);
		dehacked::loadLumps(newres.waddata.master, newres.config);
	}
	catch (const std::runtime_error&e )
	{
		gLog.printf("%s\n", e.what());
		throw;
	}

	return newres;
}

//
// load all game/port definitions (*.ugh).
// open all wads in the master directory.
// read important content from the wads (palette, textures, etc).
//
void Instance::Main_LoadResources(const LoadingData &loading) noexcept(false)
{
	auto newres = loadResources(loading, wad);

	// Commit it
	wad = std::move(newres.waddata);
	conf = std::move(newres.config);
	loaded = std::move(newres.loading);
	if(main_win)
		testmap::updateMenuName(main_win->menu_bar, loaded);
	
	UpdateViewOnResources();
}

void Instance::UpdateViewOnResources()
{
	// Must deselect now
	if(edit.Selected)
		edit.Selected->clear_all();
	
	gLog.printf("--- DONE ---\n");
	gLog.printf("\n");

	// reset sector info (for slopes and 3D floors)
	Subdiv_InvalidateAll();

	if (main_win)
	{
		// kill all loaded OpenGL images
		if (main_win->canvas)
			main_win->canvas->DeleteContext();

		main_win->UpdateGameInfo(loaded, conf);

		main_win->browser->Populate();

		// TODO: only call this when the IWAD has changed
		main_win->propsLoadValues();
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

	gLog.printf("Current time: %02d:%02d on %04d/%02d/%02d\n",
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

	gLog.printf("Current time: %02d:%02d on %04d/%02d/%02d\n",
			  calend_time->tm_hour, calend_time->tm_min,
			  calend_time->tm_year + 1900, calend_time->tm_mon + 1,
			  calend_time->tm_mday);
#endif
}

//
// Sets up the config path before using it
//
static void prepareConfigPath()
{
	if (global::config_file.empty())
	{
		if(global::home_dir.empty())
			ThrowException("Home directory not set.");

		global::config_file = global::home_dir / "config.cfg";
	}
}

static void setupSignalHandlers()
{
#ifdef _WIN32
#else
	struct sigaction action = {};
	action.sa_handler = [](int signalNumber)
	{
		int status;
		pid_t pid;
		while((pid = waitpid(-1, &status, WNOHANG)) > 0)
		{
			signalling::hasChildProcessStatus = true;
			signalling::childProcessStatus = status;
		}
	};
	action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	int r = sigaction(SIGCHLD, &action, nullptr);
	if (r == -1)
	{
		ThrowException("Failed setting up process reaper signal handler: %s", 
					   GetErrorMessage(errno).c_str());
	}
#endif
}

//
//  Common entry point, called by main or other handlers depending on system
//
int EurekaMain(int argc, char *argv[])
{
	try
	{
		try
		{
			setupSignalHandlers();
		}
		catch(const std::runtime_error &e)
		{
			// non-critical error, may cause issues
			gLog.printf("WARNING: %s\n", e.what());
		}

		init_progress = ProgressStatus::nothing;
		
		Instance instance;
		gInstance = &instance;


		// a quick pass through the command line arguments
		// to handle special options, like --help, --install, --config
		M_ParseCommandLine(argc - 1, argv + 1, CommandLinePass::early, global::Pwad_list, options);

		if (global::show_help)
		{
			ShowHelp();
			return 0;
		}
		if (global::show_version)
		{
			ShowVersion();
			return 0;
		}

		init_progress = ProgressStatus::early;


		gLog.printf("\n");
		gLog.printf("*** " EUREKA_TITLE " v" EUREKA_VERSION " (C) 2020 The Eureka Team ***\n");
		gLog.printf("\n");

		// sanity checks type sizes (useful when porting)
		CheckTypeSizes();

		ShowTime();


		Determine_InstallPath(argv[0]);
		Determine_HomeDir(argv[0]);

		if(!gLog.openFile(global::log_file))
			gLog.printf("WARNING: failed opening log file '%s'\n",
						global::log_file.u8string().c_str());


		// load all the config settings
		config::preloading = gInstance->loaded;
		prepareConfigPath();
		M_ParseConfigFile(global::config_file, options);

		// environment variables can override them
		M_ParseEnvironmentVars();

		// and command line arguments will override both
		M_ParseCommandLine(argc - 1, argv + 1, CommandLinePass::normal, global::Pwad_list, options);
		
		gInstance->loaded = config::preloading;	// update state now

		// TODO: create a new instance
		gInstance->Editor_Init();

		Main_SetupFLTK();

		init_progress = ProgressStatus::loaded;

		global::recent.load(global::home_dir);

		M_LoadBindings();

		global::recent.lookForIWADs(global::install_dir, global::home_dir);

		Main_OpenWindow(*gInstance);

		init_progress = ProgressStatus::window;

		gInstance->M_LoadOperationMenus();


		// open a specified PWAD now
		// [ the map is loaded later.... ]

		if (!global::Pwad_list.empty())
		{
			// this fatal errors on any missing file
			// [ hence the Open() below is very unlikely to fail ]
			M_ValidateGivenFiles();

			// TODO: main instance
			std::shared_ptr<Wad_file> editWad = Wad_file::Open(global::Pwad_list[0], WadOpenMode::append);
			if(!editWad)
			{
				ThrowException("Cannot load pwad: %s\n", global::Pwad_list[0].u8string().c_str());
			}
			
			// Note: the Main_LoadResources() call will ensure this gets
			//       placed at the correct spot (at the end)
			gInstance->wad.master.ReplaceEditWad(editWad);
		}
		// don't auto-load when --iwad or --warp was used on the command line
		else if (config::auto_load_recent && ! (!gInstance->loaded.iwadName.empty() || !gInstance->loaded.levelName.empty()))
		{
			gInstance->M_TryOpenMostRecent();
		}


		// Handle the '__EUREKA' lump.  It is almost equivalent to using the
		// -iwad, -merge and -port command line options, but with extra
		// checks (to allow editing a wad containing dud information).
		//
		// Note: there is logic in M_ParseEurekaLump() to ensure that command
		// line arguments can override the EUREKA_LUMP values.

		if (gInstance->wad.master.editWad())
		{
			if (! gInstance->loaded.parseEurekaLump(global::home_dir, global::install_dir, global::recent, gInstance->wad.master.editWad().get(), true /* keep_cmd_line_args */))
			{
				// user cancelled the load
				gInstance->wad.master.RemoveEditWad();
			}
		}


		// determine which IWAD to use
		// TODO: instance management
		std::shared_ptr<Wad_file> gameWad;
		if (! DetermineIWAD(*gInstance))
			goto quit;
		gameWad = Wad_file::Open(gInstance->loaded.iwadName, WadOpenMode::read);
		if(!gameWad)
			goto quit;

		DeterminePort(*gInstance);

		// temporarily load the iwad, the following few functions need it.
		// it will get loaded again in Main_LoadResources().
		// TODO: check result
		gInstance->wad.master.setGameWad(gameWad);
		gameWad.reset();

		// load the initial level
		// TODO: first instance
		gInstance->loaded.levelName = DetermineLevel(*gInstance);

		gLog.printf("Loading initial map : %s\n", gInstance->loaded.levelName.c_str());

		// TODO: the first instance
		gInstance->LoadLevel(gInstance->wad.master.activeWad().get(), gInstance->loaded.levelName);

		// do this *after* loading the level, since config file parsing
		// can depend on the map format and UDMF namespace.
		gInstance->Main_LoadResources(gInstance->loaded);	// TODO: instance management


		Main_Loop();

	quit:
		/* that's all folks! */

		gLog.printf("Quit\n");

		init_progress = ProgressStatus::nothing;
		global::app_has_focus = false;

		// TODO: all instances
		gLog.close();

		return 0;
	}
	catch(const std::exception &e)
	{
		FatalError("%s\n", e.what());
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
