//------------------------------------------------------------------------
//  TEST (PLAY) THE MAP
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2016 Andrew Apted
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

#include "Errors.h"
#include "Instance.h"

#include "main.h"

#include "m_files.h"
#include "m_loadsave.h"
#include "m_parse.h"
#include "m_testmap.h"
#include "w_wad.h"

#include "ui_menu.h"
#include "ui_window.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

class DirChangeContext
{
public:
	explicit DirChangeContext(const fs::path &path);
	~DirChangeContext();
private:
	char old_dir[FL_PATH_MAX] = {};
};

DirChangeContext::DirChangeContext(const fs::path &path)
{
	// remember the previous working directory
	if(!getcwd(old_dir, sizeof(old_dir)))
		old_dir[0] = '\0';
	old_dir[FL_PATH_MAX - 1] = '\0';	// just in case
	gLog.printf("Changing current dir to: %s\n", path.u8string().c_str());
	if(!FileChangeDir(path))
		throw std::runtime_error("Failed changing directory to port location");
}

DirChangeContext::~DirChangeContext()
{
	// restore previous working directory
	if(*old_dir)
		FileChangeDir(fs::u8path(old_dir));
}

static SString QueryName(const SString &port, const SString &cgame)
{
	SYS_ASSERT(port.good());

	SString game = cgame;

	if (port.noCaseEqual("vanilla"))
	{
		if (game.empty())
			game = "doom2";

		return "vanilla_" + game;
	}

	return port;
}

static bool isMacOSAppBundle(const fs::path &path)
{
#ifdef __APPLE__
	CFStringRef pathString = CFStringCreateWithCString(kCFAllocatorDefault, path.u8string().c_str(), kCFStringEncodingUTF8);
	if(!pathString)
	{
		gLog.printf("ERROR: Failed allocating macOS app bundle path CF string: %s\n", path.u8string().c_str());
		return false;
	}
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, pathString, kCFURLPOSIXPathStyle, true);
	
	CFRelease(pathString);
	if(!url)
	{
		gLog.printf("ERROR: Failed allocating macOS app bundle CF URL: %s\n", path.u8string().c_str());
		
		return false;
	}
	
	CFBundleRef bundle = CFBundleCreate(kCFAllocatorDefault, url);
	CFRelease(url);
	if(!bundle)
	{
		gLog.printf("Could not load, or invalid macOS app CF bundle: %s\n", path.u8string().c_str());
		
		return false;
	}
	
	CFDictionaryRef infoDict = CFBundleGetInfoDictionary(bundle);
	CFRelease(bundle);
	
	return !!infoDict;
#else
	return false;
#endif
}

class UI_PortPathDialog : public UI_Escapable_Window
{
public:
	static const int PADDING = 20;
	static const int LABEL_HEIGHT = 30;
	static const int INTER_LABEL_SPACE = 5;
	static const int LABEL_TEXT_BOX_SPACE = 15;
	static const int TEXT_BOX_LEFT = 118;
	static const int TEXT_BOX_HEIGHT = 26;
	static const int TEXT_BOX_BUTTON_SPACE = 22;
	static const int TEXT_BOX_BUTTON_WIDTH = 60;
	static const int PADDING_BEFORE_BOTTOM = 55;
	static const int BOTTOM_BAR_HEIGHT = 70;
	static const int BOTTOM_BAR_OUTSET = 10;
	static const int BOTTOM_BUTTON_WIDTH = 95;
	static const int BOTTOM_BUTTON_HEIGHT = 30;
	static const int BOTTOM_PADDING = 15;
	static const int BOTTOM_RIGHT_PADDING = 25;
	static const int BOTTOM_BUTTON_SPACING = 45;

	Fl_Output *exe_display;
	
	Fl_Button *ok_but;
	Fl_Button *cancel_but;

	// the chosen EXE name, or NULL if cancelled
	fs::path exe_name;

	bool want_close = false;

	const Instance& inst;

private:
	Fl_Input* other_args;
	SString command_line;

public:
	void SetEXE(const fs::path &newbie)
	{
		exe_name = newbie;

		// NULL is ok here
		exe_display->value(exe_name.u8string().c_str());

		if (!exe_name.empty() && (FileExists(exe_name) || isMacOSAppBundle(exe_name)))
			ok_but->activate();
		else
			ok_but->deactivate();
	}

	void SetCommandLine(const SString& command_line)
	{
		this->command_line = command_line;
		other_args->value(command_line.c_str());
	}

	void HideCommandLine()
	{
		other_args->deactivate();
	}

	const char* getCommandLine() const
	{
		return other_args->value();
	}

	static void ok_callback(Fl_Widget *w, void *data)
	{
		UI_PortPathDialog * that = (UI_PortPathDialog *)data;

		that->want_close = true;
	}

	static void close_callback(Fl_Widget *w, void *data)
	{
		UI_PortPathDialog * that = (UI_PortPathDialog *)data;

		that->SetEXE("");

		that->want_close = true;
	}

	static void find_callback(Fl_Widget *w, void *data)
	{
		UI_PortPathDialog * that = (UI_PortPathDialog *)data;

		Fl_Native_File_Chooser chooser;

		chooser.title("Pick the executable file");
		chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
#ifdef WIN32
		chooser.filter("Executables\t*.exe");
#endif

		// FIXME : if we have an exe_filename already, and folder exists, go there
		//         [ especially for vanilla -- look in path of Iwad_name ]
		chooser.directory(that->inst.Main_FileOpFolder().u8string().c_str());

		switch (chooser.show())
		{
			case -1:  // error
				DLG_Notify("Unable to use that exe:\n\n%s", chooser.errmsg());
				return;

			case 1:  // cancelled
				return;

			default:
				break;  // OK
		}

		// we assume the chosen file exists

		that->SetEXE(fs::u8path(chooser.filename()));
	}

public:



	UI_PortPathDialog(const SString &port_name, const Instance &inst) :
		UI_Escapable_Window(580, PADDING + LABEL_HEIGHT * 2 + INTER_LABEL_SPACE + 2 * (LABEL_TEXT_BOX_SPACE + TEXT_BOX_HEIGHT) + PADDING_BEFORE_BOTTOM + BOTTOM_BAR_HEIGHT, "Port Settings"),
		inst(inst)
	{


		char message_buf[256];

		snprintf(message_buf, sizeof(message_buf), "Setting up location of the executable (EXE) for %s.", port_name.c_str());

		Fl_Box *header = new Fl_Box(FL_NO_BOX, PADDING, PADDING, w() - 2 * PADDING, LABEL_HEIGHT, "");
		header->copy_label(message_buf);
		header->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

		header = new Fl_Box(FL_NO_BOX, PADDING, header->y() + header->h() + INTER_LABEL_SPACE, w() - 2 * PADDING, LABEL_HEIGHT,
		           "This is only needed for the Test Map command.");
		header->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);


		exe_display = new Fl_Output(TEXT_BOX_LEFT, header->y() + header->h() + LABEL_TEXT_BOX_SPACE, w() - TEXT_BOX_LEFT - PADDING - TEXT_BOX_BUTTON_WIDTH - TEXT_BOX_BUTTON_SPACE, TEXT_BOX_HEIGHT, "Exe path: ");
		other_args = new Fl_Input(TEXT_BOX_LEFT, exe_display->y() + exe_display->h() + LABEL_TEXT_BOX_SPACE, w() - TEXT_BOX_LEFT - PADDING, TEXT_BOX_HEIGHT, "Command line: ");

		Fl_Button *find_but = new Fl_Button(w()-TEXT_BOX_BUTTON_WIDTH-PADDING, exe_display->y(), TEXT_BOX_BUTTON_WIDTH, TEXT_BOX_HEIGHT, "Find");
		find_but->callback((Fl_Callback*)find_callback, this);

		/* bottom buttons */

		Fl_Group * grp = new Fl_Group(0, h() - BOTTOM_BAR_HEIGHT + BOTTOM_BAR_OUTSET, w(), BOTTOM_BAR_HEIGHT);
		grp->box(FL_FLAT_BOX);
		grp->color(WINDOW_BG, WINDOW_BG);
		{
			cancel_but = new Fl_Button(w() - BOTTOM_RIGHT_PADDING - BOTTOM_BUTTON_WIDTH * 2 - BOTTOM_BUTTON_SPACING, h() - BOTTOM_BUTTON_HEIGHT - BOTTOM_PADDING, BOTTOM_BUTTON_WIDTH, BOTTOM_BUTTON_HEIGHT, "Cancel");
			cancel_but->callback(close_callback, this);

			ok_but = new Fl_Button(w() - BOTTOM_BUTTON_WIDTH - BOTTOM_RIGHT_PADDING, h() - BOTTOM_BUTTON_HEIGHT - BOTTOM_PADDING, BOTTOM_BUTTON_WIDTH, BOTTOM_BUTTON_HEIGHT, "OK");
			ok_but->labelfont(FL_HELVETICA_BOLD);
			ok_but->callback(ok_callback, this);
			ok_but->shortcut(FL_Enter);
			ok_but->deactivate();
		}
		grp->end();

		end();

		resizable(NULL);

		callback(close_callback, this);
	}

	virtual ~UI_PortPathDialog()
	{ }

	// returns true if user clicked OK
	bool Run()
	{
		set_modal();
		show();

		while (! want_close)
			Fl::wait(0.2);

		return !exe_name.empty();
	}
};


bool Instance::M_PortSetupDialog(const SString &port, const SString &game, const tl::optional<SString>& commandLine)
{
	SString name_buf;

	if (port.noCaseEqual("vanilla"))
		name_buf = "vanilla " + game.asTitle();
	else if (port.noCaseEqual("mbf"))	// temp hack for aesthetics
		name_buf = "MBF";
	else
		name_buf = port.asTitle();

	UI_PortPathDialog dialog(name_buf, *this);

	// populate the EXE name from existing info, if exists
	const fs::path *info = global::recent.queryPortPath(QueryName(port, game));

	if (info && !info->empty())
		dialog.SetEXE(*info);

	if (commandLine)
		dialog.SetCommandLine(*commandLine);
	else
		dialog.HideCommandLine();

	bool ok = dialog.Run();

	if (ok)
	{
		// persist the new port settings
		global::recent.setPortPath(QueryName(port, game),
								   GetAbsolutePath(dialog.exe_name));
		if(main_win)
			testmap::updateMenuName(main_win->menu_bar, loaded);

		global::recent.save(global::home_dir);

		if (commandLine)
			loaded.testingCommandLine = dialog.getCommandLine();
	}

	return ok;
}


//------------------------------------------------------------------------

static void CalcWarpString(const SString& levelName, std::vector<SString> &args)
{
	SYS_ASSERT(!levelName.empty());
	// FIXME : EDGE allows a full name: -warp MAP03
	//         Eternity too.
	//         ZDOOM too, but different syntax: +map MAP03

	// most common syntax is "MAP##" or "MAP###"
	if (levelName.length() >= 4 && levelName.noCaseStartsWith("MAP") && isdigit(levelName[3]))
	{
		long number = strtol(levelName.c_str() + 3, nullptr, 10);
		args.push_back("-warp");
		args.push_back(std::to_string(number));
		return;
	}

	// detect "E#M#" syntax of Ultimate-Doom and Heretic, which need
	// a pair of numbers after -warp
	if (levelName.length() >= 4 && !isdigit(levelName[0]) && isdigit(levelName[1]) &&
		!isdigit(levelName[2]) && isdigit(levelName[3]))
	{
		args.push_back("-warp");
		args.push_back(SString::printf("%c", levelName[1]));
		args.push_back(levelName.c_str() + 3);
		return;
	}

	// map name is non-standard, find the first digit group and hope
	// for the best...

	size_t digitPos = levelName.findDigit();
	if (digitPos != std::string::npos)
	{
		args.push_back("-warp");
		args.push_back(levelName.c_str() + digitPos);
		return;
	}

	// no digits at all, oh shit!
}

static void GrabWadNamesArgs(const Instance& inst, std::vector<SString> &args)
{
	bool has_file = false;
	int use_merge = 0;

	// see if we should use the "-merge" parameter, which is
	// required for Chocolate-Doom and derivates like Crispy Doom.
	// TODO : is there a better way to do this?
	if (inst.loaded.portName.noCaseEqual("vanilla"))
	{
		use_merge = 1;
	}

	// always specify the iwad
	if (inst.wad.master.gameWad())
	{
		args.push_back("-iwad");
		args.push_back(inst.wad.master.gameWad()->PathName().u8string());
	}

	// add any resource wads
	for (const std::shared_ptr<Wad_file>& wad : inst.wad.master.resourceWads())
	{
		if (use_merge == 1)
			args.push_back("-merge");
		else if (!use_merge && !has_file)
			args.push_back("-file");
		args.push_back(wad->PathName().u8string());

		if (use_merge)
			use_merge++;
		else
			has_file = true;
	}

	// the current PWAD, if exists, must be last
	if (inst.wad.master.editWad())
	{
		if (!has_file)
			args.push_back("-file");
		args.push_back(inst.wad.master.editWad()->PathName().u8string());
	}
}

static SString buildArgString(const std::vector<SString>& args, bool backslash)
{
	SString result;
	for (const SString& arg : args)
	{
		if (!result.empty())
			result += " ";
		result += arg.spaceEscape(backslash);
	}
	return result;
}

static void logArgs(const SString& args)
{
	gLog.printf("Testing map using the following command:\n");
	gLog.printf("--> %s\n", args.c_str());
}

#ifdef _WIN32
// On Windows the process is started as if user ran it individually
static void testMapOnWindows(const Instance &inst, const fs::path& portPath)
{
	std::vector<SString> args;
	GrabWadNamesArgs(inst, args);
	CalcWarpString(inst.loaded.levelName, args);

	SString argString = inst.loaded.testingCommandLine + " " + buildArgString(args, false);
	logArgs(argString);
	std::wstring argsWide = UTF8ToWide(argString.c_str());

	HINSTANCE result = ShellExecuteW(nullptr, L"open", portPath.wstring().c_str(), argsWide.c_str(),
		FilenameGetPath(portPath).wstring().c_str(), SW_SHOW);
	if ((INT_PTR)result <= 32)
	{
		DWORD error = GetLastError();
		ThrowException("Failed starting %s: error %s\n\n%s", portPath.u8string().c_str(),
			GetShellExecuteErrorMessage(result).c_str(), GetWindowsErrorMessage(error).c_str());
	}
	inst.Status_Set("Started the game");
}
#else

static void testMapOnMacBundle(const Instance &inst, const fs::path& portPath)
{
	std::vector<SString> args;
	GrabWadNamesArgs(inst, args);
	CalcWarpString(inst.loaded.levelName, args);
	
	SString argString = SString("/usr/bin/open -a ") + SString(portPath.u8string()).spaceEscape(true) + " --args " + inst.loaded.testingCommandLine + " " + buildArgString(args, true);
	logArgs(argString);
	
	system(argString.c_str());
}

static void testMapOnPOSIX(const Instance &inst, const fs::path& portPath)
{
	std::vector<SString> args;
	GrabWadNamesArgs(inst, args);
	CalcWarpString(inst.loaded.levelName, args);

	SString arg;
	TokenWordParse parse(inst.loaded.testingCommandLine, false);
	while(parse.getNext(arg))
		args.push_back(arg);
	args.insert(args.begin(), portPath.u8string());
	
	std::vector<char *> argv;
	argv.reserve(args.size() + 2);
	fs::path portName = portPath.filename();
	SString argString;
	for(SString &arg : args)
	{
		argString += arg + " ";
		argv.push_back(arg.get().data());
	}

	argv.push_back(nullptr);

	logArgs(argString);

	pid_t pid = fork();
	if(pid == -1)
	{
		// fail
		ThrowException("Failed forking to start %s: %s", portName.u8string().c_str(),
					   GetErrorMessage(errno).c_str());
	}
	else if(pid == 0)
	{
		// child process
		try
		{
			DirChangeContext dirChangeContext(FilenameGetPath(portPath));
			execvp(portPath.u8string().c_str(), argv.data());
			
			// on failure
			int err = errno;
			gLog.printf("--> Failed starting %s: %s\n", portName.u8string().c_str(), GetErrorMessage(err).c_str());
			
			_exit(err);
		}
		catch(const std::exception &e)
		{
			gLog.printf("--> Failed starting %s: %s\n", portName.u8string().c_str(), e.what());
			_exit(EXIT_FAILURE);
		}
		catch(...)
		{
			// Need to guard it best here
			_exit(EXIT_FAILURE);
		}
	}

	// Parent process. Continue work.
}
#endif

void Instance::CMD_ChangeTestSettings()
{
	try
	{
		M_PortSetupDialog(loaded.portName, loaded.gameName, loaded.testingCommandLine);
	}
	catch (const std::runtime_error& e)
	{
		Beep("Failed: %s\n", e.what());
	}
}

static bool M_IsPortPathValid(const fs::path &path)
{
	if(path.u8string().length() < 2)
		return false;

	if (! FileExists(path) && !isMacOSAppBundle(path))
		return false;

	return true;
}

void Instance::CMD_TestMap()
{
	try
	{
		if (level.MadeChanges)
		{
			if (DLG_Confirm({ "Cancel", "&Save" },
				"You have unsaved changes, do you want to save them now "
				"and build the nodes?") <= 0)
			{
				return;
			}

			if (!M_SaveMap(false))
				return;
		}


		// check if we know the executable path, if not then ask
		const fs::path* info = global::recent.queryPortPath(QueryName(loaded.portName,
			loaded.gameName));

		if (!info || !M_IsPortPathValid(*info))
		{
			if (!M_PortSetupDialog(loaded.portName, loaded.gameName, loaded.testingCommandLine))
				return;

			info = global::recent.queryPortPath(QueryName(loaded.portName, loaded.gameName));
		}

		// this generally can't happen, but we check anyway...
		if (!info || !M_IsPortPathValid(*info))
		{
			Beep("invalid path to executable");
			return;
		}

		Status_Set("TESTING MAP");
		if(main_win)
			main_win->redraw();
		Fl::wait(0.1);
		Fl::wait(0.1);

#ifdef _WIN32
		testMapOnWindows(*this, *info);
#else
		if(isMacOSAppBundle(*info))
			testMapOnMacBundle(*this, *info);
		else
			testMapOnPOSIX(*this, *info);
#endif
		if(main_win)
			main_win->redraw();
		Fl::wait(0.1);
		Fl::wait(0.1);
		
	}
	catch(const std::runtime_error &e)
	{
		Status_Set("Failed testing map");
		DLG_ShowError(false, "Could not start map for testing: %s", e.what());
	}
	
}

namespace testmap
{
void updateMenuName(Fl_Sys_Menu_Bar *bar, const LoadingData &loading)
{
	if(loading.portName.empty() || loading.gameName.empty())
		return;	// premature
	const fs::path* info = global::recent.queryPortPath(QueryName(loading.portName,
		loading.gameName));
	if(!info || !M_IsPortPathValid(*info))
		menu::setTestMapDetail(bar, "");
	else
		menu::setTestMapDetail(bar, SString(info->filename().replace_extension().u8string()));
}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
