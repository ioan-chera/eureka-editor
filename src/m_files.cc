//------------------------------------------------------------------------
//  RECENT FILES / KNOWN IWADS / BACKUPS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2018 Andrew Apted
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
#include "m_config.h"
#include "m_files.h"
#include "m_game.h"
#include "m_loadsave.h"
#include "m_parse.h"
#include "m_streams.h"
#include "w_wad.h"

#include "ui_window.h"

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

// list of known iwads (mapping GAME name --> PATH)

namespace global
{
	std::map<SString, fs::path> known_iwads;
}


void M_AddKnownIWAD(const fs::path &path)
{
	fs::path absolute_name = fs::absolute(path);

	const SString &game = GameNameFromIWAD(path);

	global::known_iwads[game] = absolute_name;
}


fs::path M_QueryKnownIWAD(const SString &game)
{
	std::map<SString, fs::path>::iterator KI;

	KI = global::known_iwads.find(game);

	if (KI != global::known_iwads.end())
		return KI->second;
	else
		return "";
}


// returns a string, with each name separated by a '|' character,
// hence directly usable with the FL_Choice::add() method.
//
SString M_CollectGamesForMenu(int *exist_val, const char *exist_name)
{
	std::map<SString, fs::path>::iterator KI;

	SString result;
	result.reserve(2000);

	int index = 0;

	for (KI = global::known_iwads.begin() ; KI != global::known_iwads.end() ; KI++, index++)
	{
		const SString &name = KI->first;

		if (result[0])
			result += '|';

		result += name;

		if (name.noCaseEqual(exist_name))
			*exist_val = index;
	}

	return result;
}

static void M_WriteKnownIWADs(std::ostream &os, const std::map<SString, fs::path> &known_iwads)
{
	std::map<SString, fs::path>::const_iterator KI;

	for (KI = known_iwads.begin() ; KI != known_iwads.end() ; KI++)
	{
		os << "known_iwad " << KI->first.spaceEscape() << " " << escape(KI->second) << std::endl;
	}
}


void M_ValidateGivenFiles()
{
	for (const fs::path &pwad : global::Pwad_list)
	{
		if (! Wad_file::Validate(pwad))
			ThrowException("Given pwad does not exist or is invalid: %s\n",
						   pwad.u8string().c_str());
	}
}


int M_FindGivenFile(const char *filename)
{
	for (int i = 0 ; i < (int)global::Pwad_list.size() ; i++)
		if (global::Pwad_list[i] == fs::u8path(filename))
			return i;

	return -1;  // Not Found
}


//------------------------------------------------------------------------
//  PORT PATH HANDLING
//------------------------------------------------------------------------

// the set of all known source port paths

namespace global
{
	std::map<SString, port_path_info_t> port_paths;
}


port_path_info_t * M_QueryPortPath(const SString &name, std::map<SString, port_path_info_t> &port_paths, bool create_it)
{
	std::map<SString, port_path_info_t>::iterator IT;

	IT = port_paths.find(name);

	if (IT != port_paths.end())
		return &IT->second;

	if (create_it)
	{
		port_path_info_t info;
		port_paths[name] = info;

		return M_QueryPortPath(name, port_paths);
	}

	return NULL;
}


bool M_IsPortPathValid(const port_path_info_t *info)
{
	if(info->exe_filename.u8string().length() < 2)
		return false;

	if (! FileExists(info->exe_filename))
		return false;

	return true;
}

//
// Reads an entire buffer from file
//
bool readBuffer(FILE* f, size_t size, std::vector<byte>& target)
{
	target.resize(size);
	size_t toRead = size;
	while (toRead > 0)
	{
		size_t r = fread(target.data() + size - toRead, 1, toRead, f);
		if (!r)
			return false;
		toRead -= r;
	}
	return true;
}


//
// Parse port path
//
static void M_ParsePortPath(const SString &name, const SString &cpath, std::map<SString, port_path_info_t> &port_paths)
{
	SString path(cpath);
	path.trimLeadingSpaces();
	size_t pos = path.find('|');
	if(pos == std::string::npos)
	{
		// TODO : Warn
		return;
	}

	// terminate arguments
	path.erase(0, pos + 1);

	port_path_info_t *info = M_QueryPortPath(name, port_paths, true);
	if (! info)	// should not fail!
		return;

	info->exe_filename = fs::u8path(path.get());

	// parse any other arguments
	// [ none needed atm.... ]
}

static void M_WritePortPaths(std::ostream &os, const std::map<SString, port_path_info_t> &port_paths)
{
	std::map<SString, port_path_info_t>::const_iterator IT;

	for (IT = port_paths.begin() ; IT != port_paths.end() ; IT++)
	{
		const port_path_info_t& info = IT->second;
		os << "port_path " << IT->first.spaceEscape() << " |" << escape(info.exe_filename) << std::endl;
	}
}

// Recent files

recent_file_data_c *RecentFiles_c::getData(int index) const
{
	SYS_ASSERT(0 <= index && index < (int)list.size());

	return new recent_file_data_c(list[index]);
}

RecentFiles_c::Deque::iterator RecentFiles_c::find(const fs::path &file)
{
	// ignore the path when matching filenames
	SString A = file.filename().u8string();

	for(auto it = list.begin(); it != list.end(); ++it)
	{
		SString B = it->file.filename().u8string();
		if(!A.noCaseEqual(B))
			continue;

		return it;
	}

	return list.end();	// not found
}

void RecentFiles_c::push_front(const fs::path &file, const SString &map)
{
	if(list.size() >= MAX_RECENT)
	{
		list.pop_back();
	}
	list.emplace_front(file, map);
}

void RecentFiles_c::insert(const fs::path &file, const SString &map)
{
	// ensure filename (without any path) is unique
	auto it = find(file);

	if(it != list.end())
		list.erase(it);

	push_front(file, map);
}

void RecentFiles_c::Write(std::ostream &stream) const
{
	// file is in opposite order, newest at the end
	// (this allows the parser to merely insert() items in the
	//  order they are read).

	for(auto it = list.rbegin(); it != list.rend(); ++it)
		stream << "recent " << it->map.spaceEscape() << " " << escape(it->file) << std::endl;
}

SString RecentFiles_c::Format(int index) const
{
	SYS_ASSERT(index < (int)list.size());

	SString name = list[index].file.filename().u8string();

	return SString::printf("%s%s%d:  %-.42s", (index < 9) ? "  " : "",
		(index < 9) ? "&" : "", 1 + index, name.c_str());
}

void RecentFiles_c::Lookup(int index, fs::path *file_v, SString *map_v) const
{
	SYS_ASSERT(index >= 0);
	SYS_ASSERT(index < (int)list.size());

	*file_v = list[index].file;
	*map_v = list[index].map;
}

namespace global
{
	RecentFiles_c  recent_files;
}

//
// Parse miscellaneous config
//
static void ParseMiscConfig(std::istream &is, RecentFiles_c &recent_files,
							std::map<SString, fs::path> &known_iwads,
							std::map<SString, port_path_info_t> &port_paths)
{
	SString line;
	while(M_ReadTextLine(line, is))
	{
		SString keyword;
		TokenWordParse parse(line);
		if(!parse.getNext(keyword))
			continue;	// blank line
		if(keyword == "recent")
		{
			SString map;
			fs::path path;
			if(!parse.getNext(map))
			{
				gLog.printf("Expected map name after 'recent' in recents config\n");
				continue;
			}
			if(!parse.getNext(path))
			{
				gLog.printf("Expected WAD path as second arg in 'recent' in recents config\n");
				continue;
			}
			if(Wad_file::Validate(path))
				recent_files.insert(path, map);
			else
				gLog.printf("  no longer exists: %s\n", path.u8string().c_str());
		}
		else if(keyword == "known_iwad")
		{
			SString name;
			fs::path path;
			if(!parse.getNext(name))
			{
				gLog.printf("Expected IWAD name after 'known_iwad' in recents config\n");
				continue;
			}
			if(!parse.getNext(path))
			{
				gLog.printf("Expected WAD path as second arg in 'known_iwad' in recents config\n");
				continue;
			}
			// ignore plain freedoom.wad (backwards compatibility)
			if(name.noCaseEqual("freedoom"))
				gLog.printf("  ignoring for compatibility: %s\n", path.u8string().c_str());
			else if(Wad_file::Validate(path))
				known_iwads[name] = path;
			else
				gLog.printf("  no longer exists: %s\n", path.u8string().c_str());
		}
		else if(keyword == "port_path")
		{
			SString name, barpath, path;
			if(!parse.getNext(name))
			{
				gLog.printf("Expected port name after 'port_path' in recents config\n");
				continue;
			}
			if(!parse.getNext(barpath))
			{
				gLog.printf("Expected | followed by port path after port name\n");
				continue;
			}
			if(parse.getNext(path))	// allow space after |
				barpath += path;
			M_ParsePortPath(name, barpath, port_paths);
		}
		else
			gLog.printf("Unknown keyword '%s' in recents config\n", keyword.c_str());
	}
}


void M_LoadRecent(const fs::path &home_dir, RecentFiles_c &recent_files,
				  std::map<SString, fs::path> &known_iwads,
				  std::map<SString, port_path_info_t> &port_paths)
{
	fs::path filename = home_dir / "misc.cfg";

	std::ifstream is(filename);
	if(!is.is_open())
	{
		gLog.printf("No recent list at: %s\n", filename.u8string().c_str());
		return;
	}

	gLog.printf("Reading recent list from: %s\n", filename.u8string().c_str());

	recent_files.clear();
	known_iwads.clear();
	port_paths.clear();

	ParseMiscConfig(is, recent_files, known_iwads, port_paths);
}


void M_SaveRecent(const fs::path &home_dir, const RecentFiles_c &recent_files, const std::map<SString, fs::path> &known_iwads, const std::map<SString, port_path_info_t> &port_paths)
{
	fs::path filename = home_dir / "misc.cfg";

	std::ofstream os(filename, std::ios::trunc);
	if(!os.is_open())
	{
		gLog.printf("Failed to save recent list to: %s\n", filename.u8string().c_str());
		return;
	}

	gLog.printf("Writing recent list to: %s\n", filename.u8string().c_str());
	os << "# Eureka miscellaneous stuff" << std::endl;

	recent_files.Write(os);

	M_WriteKnownIWADs(os, known_iwads);

	M_WritePortPaths(os, port_paths);
}


int M_RecentCount()
{
	return global::recent_files.getSize();
}

SString M_RecentShortName(int index)
{
	return global::recent_files.Format(index);
}

void * M_RecentData(int index)
{
	return global::recent_files.getData(index);
}


void M_OpenRecentFromMenu(void *priv_data)
{
	SYS_ASSERT(priv_data);

	recent_file_data_c *data = (recent_file_data_c *)priv_data;

	OpenFileMap(data->file.u8string(), data->map);
}


void M_AddRecent(const SString &filename, const SString &map_name)
{
	global::recent_files.insert(GetAbsolutePath(filename.get()), map_name);

	M_SaveRecent(global::home_dir, global::recent_files, global::known_iwads, global::port_paths);  // why wait?
}


bool Instance::M_TryOpenMostRecent()
{
	if (global::recent_files.getSize() == 0)
		return false;

	SString filename;
	SString map_name;

	fs::path filepath;
	global::recent_files.Lookup(0, &filepath, &map_name);
	filename = filepath.generic_u8string();

	// M_LoadRecent has already validated the filename, so this should
	// normally work.

	std::shared_ptr<Wad_file> wad = Wad_file::Open(filename,
												   WadOpenMode::append);

	if (! wad)
	{
		gLog.printf("Failed to load most recent pwad: %s\n", filename.c_str());
		return false;
	}

	// make sure at least one level can be loaded
	if (wad->LevelCount() == 0)
	{
		gLog.printf("No levels in most recent pwad: %s\n", filename.c_str());

		return false;
	}

	/* -- OK -- */

	if (wad->LevelFind(map_name) >= 0)
		loaded.levelName = map_name;
	else
		loaded.levelName.clear();

	this->wad.master.Pwad_name = filename;

	this->wad.master.edit_wad = wad;

	return true;
}


//------------------------------------------------------------------------
//  EUREKA LUMP HANDLING
//------------------------------------------------------------------------

#ifdef WIN32
#define PATH_SEPARATOR  ';'
#else
#define PATH_SEPARATOR  ':'
#endif

static bool ExtractOnePath(const char *paths, char *dir, int index)
{
	for (; index > 0 ; index--)
	{
		paths = strchr(paths, PATH_SEPARATOR);

		if (! paths)
			return false;

		paths++;
	}

	// handle a trailing separator
	if (! paths[0])
		return false;


	int len;

	const char * sep_pos = strchr(paths, PATH_SEPARATOR);

	if (sep_pos)
		len = (int)(sep_pos - paths);
	else
		len = (int)strlen(paths);

	if (len > FL_PATH_MAX - 2)
		len = FL_PATH_MAX - 2;


	if (len == 0)  // ouch
    {
        dir[0] = '.';
        dir[1] = '\0';
		return true;
    }

	// remove trailing slash
	while (len > 1 && paths[len - 1] == DIR_SEP_CH)
		len--;

	memcpy(dir, paths, len);

	dir[len] = 0;

	return true;
}


static SString SearchDirForIWAD(const SString &dir_name, const SString &game)
{
	char name_buf[FL_PATH_MAX];

	snprintf(name_buf, sizeof(name_buf), "%s/%s.wad", dir_name.c_str(), game.c_str());

	gLog.debugPrintf("  trying: %s\n", name_buf);

	if (Wad_file::Validate(name_buf))
		return name_buf;

	// try uppercasing the name, to find e.g. DOOM2.WAD

	y_strupr(name_buf + dir_name.length() + 1);

	gLog.debugPrintf("  trying: %s\n", name_buf);

	if (Wad_file::Validate(name_buf))
		return name_buf;

	return "";
}


static SString SearchForIWAD(const SString &game)
{
	gLog.debugPrintf("Searching for '%s' IWAD\n", game.c_str());

	static char dir_name[FL_PATH_MAX];

	// 1. look in ~/.eureka/iwads first

	snprintf(dir_name, FL_PATH_MAX, "%s/iwads", global::home_dir.u8string().c_str());
	dir_name[FL_PATH_MAX-1] = 0;

	SString path = SearchDirForIWAD(dir_name, game);
	if (!path.empty())
		return path;

	// 2. look in $DOOMWADPATH

	const char *doomwadpath = getenv("DOOMWADPATH");
	if (doomwadpath)
	{
		for (int i = 0 ; i < 999 ; i++)
		{
			if (! ExtractOnePath(doomwadpath, dir_name, i))
				break;

			path = SearchDirForIWAD(dir_name, game);
			if (!path.empty())
				return path;
		}
	}

	// 3. look in $DOOMWADDIR

	const char *doomwaddir = getenv("DOOMWADDIR");
	if (doomwaddir)
	{
		path = SearchDirForIWAD(SString(doomwaddir), game);
		if (!path.empty())
			return path;
	}

	// 4. look in various standard places

	/* WISH: check the Steam folder(s) for WIN32 */

	static const char *standard_iwad_places[] =
	{
#ifdef WIN32
		"c:/doom",
		"c:/doom2",
		"c:/doom95",
#else
		"/usr/share/games/doom",
		"/usr/share/doom",
		"/usr/local/share/games/doom",
		"/usr/local/games/doom",
#endif
		NULL
	};

	for (int i = 0 ; standard_iwad_places[i] ; i++)
	{
		path = SearchDirForIWAD(standard_iwad_places[i], game);
		if (!path.empty())
			return path;
	}

	// 5. last resort : the current directory

	path = SearchDirForIWAD(".", game);
	if (!path.empty())
		return path;

	return "";  // not found
}


//
// search for iwads in various places
//
void M_LookForIWADs()
{
	gLog.printf("Looking for IWADs....\n");

	std::vector<SString> game_list = M_CollectKnownDefs("games");

	for (const SString &game : game_list)
	{
		// already have it?
		if (!M_QueryKnownIWAD(game).empty())
			continue;

		SString path = SearchForIWAD(game);

		if (!path.empty())
		{
			gLog.printf("Found '%s' IWAD file: %s\n", game.c_str(), path.c_str());

			M_AddKnownIWAD(fs::u8path(path.get()));
		}
	}

	M_SaveRecent(global::home_dir, global::recent_files, global::known_iwads, global::port_paths);
}


SString Instance::M_PickDefaultIWAD() const
{
	// guess either DOOM or DOOM 2 based on level names
	const char *default_game = "doom2";

	if (!loaded.levelName.empty() && toupper(loaded.levelName[0]) == 'E')
	{
		default_game = "doom";
	}
	else if (wad.master.edit_wad)
	{
		int idx = wad.master.edit_wad->LevelFindFirst();

		if (idx >= 0)
		{
			idx = wad.master.edit_wad->LevelHeader(idx);
			const SString &name = wad.master.edit_wad->GetLump(idx)->Name();

			if (toupper(name[0]) == 'E')
				default_game = "doom";
		}
	}

	gLog.debugPrintf("pick default iwad, trying: '%s'\n", default_game);

	fs::path result;

	result = M_QueryKnownIWAD(default_game);
	if (!result.empty())
		return result.u8string();

	// try FreeDoom

	if (strcmp(default_game, "doom") == 0)
		default_game = "freedoom1";
	else
		default_game = "freedoom2";

	gLog.debugPrintf("pick default iwad, trying: '%s'\n", default_game);

	result = M_QueryKnownIWAD(default_game);
	if (!result.empty())
		return result.u8string();

	// try any known iwad

	gLog.debugPrintf("pick default iwad, trying first known iwad...\n");

	std::map<SString, fs::path>::iterator KI;

	KI = global::known_iwads.begin();

	if (KI != global::known_iwads.end())
		return KI->second.u8string();

	// nothing left to try
	gLog.debugPrintf("pick default iwad failed.\n");

	return "";
}


static void M_AddResource_Unique(LoadingData &loading, const SString & filename)
{
	// check if base filename (without path) already exists
	for (const fs::path &resource : loading.resourceList)
	{
		SString A = fs::u8path(filename.get()).filename().u8string();
		SString B = resource.filename().u8string();

		if(A.noCaseEqual(B))
			return;		// found it
	}

	loading.resourceList.push_back(fs::u8path(filename.get()));
}


//
// returns false if user wants to cancel the load
//
bool LoadingData::parseEurekaLump(const Wad_file *wad, bool keep_cmd_line_args)
{
	gLog.printf("Parsing '%s' lump\n", EUREKA_LUMP);

	Lump_c * lump = wad->FindLump(EUREKA_LUMP);

	if (! lump)
	{
		gLog.printf("--> does not exist.\n");
		return true;
	}

	lump->Seek();

	fs::path new_iwad;
	SString new_port;

	std::vector<SString> new_resources;

	SString line;

	while (lump->GetLine(line))
	{
		// comment?
		if (line[0] == '#')
			continue;

		line.trimTrailingSpaces();

		size_t pos = line.find(' ');

		if(pos == std::string::npos || !pos)
		{
			gLog.printf("WARNING: bad syntax in %s lump\n", EUREKA_LUMP);
			continue;
		}

		SString value;
		line.cutWithSpace(pos, &value);

		if (line == "game")
		{
			if (! M_CanLoadDefinitions(GAMES_DIR, value))
			{
				gLog.printf("  unknown game: %s\n", value.c_str() /* show full path */);

				int res = DLG_Confirm({ "&Ignore", "&Cancel Load" },
				                      "Warning: the pwad specifies an unsupported "
									  "game:\n\n          %s", value.c_str());
				if (res == 1)
					return false;
			}
			else
			{
				new_iwad = M_QueryKnownIWAD(value);

				if (new_iwad.empty())
				{
					int res = DLG_Confirm({ "&Ignore", "&Cancel Load" },
					                      "Warning: the pwad specifies an IWAD "
										  "which cannot be found:\n\n          %s.wad",
										  value.c_str());
					if (res == 1)
						return false;
				}
			}
		}
		else if (line == "resource")
		{
			fs::path resourcePath = fs::u8path(value.get());

			if(resourcePath.is_relative())
			{
				fs::path wadDirPath = fs::path(wad->PathName().c_str()).parent_path();
				resourcePath = (wadDirPath / resourcePath).lexically_normal();
			}

			// if not found at expected location, try same place as PWAD
			if (!fs::exists(resourcePath))
			{
				gLog.printf("  file not found: %s\n", resourcePath.u8string().c_str());
				fs::path wadDirPath = fs::path(wad->PathName().c_str()).parent_path();
				resourcePath = wadDirPath / resourcePath.filename();

				gLog.printf("  trying: %s\n", resourcePath.u8string().c_str());
			}
			// Still doesn't exist? Try IWAD path, if any
			if (!fs::exists(resourcePath) && !new_iwad.empty())
			{
				fs::path wadDirPath = new_iwad.parent_path();
				resourcePath = wadDirPath / resourcePath.filename();
				gLog.printf("  trying: %s\n", resourcePath.u8string().c_str());
			}

			if (fs::exists(resourcePath))
				new_resources.push_back(resourcePath.u8string());
			else
			{
				DLG_Notify("Warning: the pwad specifies a resource "
				           "which cannot be found:\n\n%s", value.c_str());
			}
		}
		else if (line == "port")
		{
			if (M_CanLoadDefinitions(PORTS_DIR, value))
				new_port = value;
			else
			{
				gLog.printf("  unknown port: %s\n", value.c_str());

				DLG_Notify("Warning: the pwad specifies an unknown port:\n\n%s", value.c_str());
			}
		}
		else
		{
			gLog.printf("WARNING: unknown keyword '%s' in %s lump\n", line.c_str(), EUREKA_LUMP);
			continue;
		}
	}

	/* OK */

	// When 'keep_cmd_line_args' is true, we do not override any value which
	// has been set via command line arguments.  In other words, cmd line
	// arguments will override the EUREKA_LUMP.
	//
	// Resources are trickier, we merge the EUREKA_LUMP resources into the ones
	// supplied on the command line, ensuring that we don't get any duplicates.

	if (!new_iwad.empty())
	{
		if (! (keep_cmd_line_args && !iwadName.empty()))
			iwadName = new_iwad;
	}

	if (!new_port.empty())
	{
		if (! (keep_cmd_line_args && !portName.empty()))
			portName = new_port;
	}

	if (! keep_cmd_line_args)
		resourceList.clear();

	for (const SString &resource : new_resources)
	{
		M_AddResource_Unique(*this, resource);
	}

	return true;
}


void LoadingData::writeEurekaLump(Wad_file *wad) const
{
	gLog.printf("Writing '%s' lump\n", EUREKA_LUMP);

	int oldie = wad->FindLumpNum(EUREKA_LUMP);
	if (oldie >= 0)
		wad->RemoveLumps(oldie, 1);

	Lump_c *lump = wad->AddLump(EUREKA_LUMP);

	lump->Printf("# Eureka project info\n");

	if (!gameName.empty())
		lump->Printf("game %s\n", gameName.c_str());

	if (!portName.empty())
		lump->Printf("port %s\n", portName.c_str());

	fs::path pwadPath = fs::absolute(wad->PathName().c_str()).remove_filename();

	for (const fs::path &resource : resourceList)
	{
		fs::path absoluteResourcePath = fs::absolute(resource);
		fs::path relative = fs::proximate(absoluteResourcePath, pwadPath);

		lump->Printf("resource %s\n", relative.generic_u8string().c_str());
	}
}


//------------------------------------------------------------------------
//  BACKUP SYSTEM
//------------------------------------------------------------------------


// config variables
int config::backup_max_files = 30;
int config::backup_max_space = 60;  // MB


struct backup_scan_data_t
{
	int low;
	int high;
};


static void backup_scan_file(const fs::path &name, int flags, void *priv_dat)
{
	backup_scan_data_t * data = (backup_scan_data_t *)priv_dat;

	if (flags & SCAN_F_Hidden)
		return;

	if (flags & SCAN_F_IsDir)
		return;

	if (! isdigit(name.u8string()[0]))
		return;

	int num = atoi(name.u8string());

	data->low  = std::min(data->low,  num);
	data->high = std::max(data->high, num);
}


inline static SString Backup_Name(const SString &dir_name, int slot)
{
	return SString::printf("%s/%d.wad", dir_name.c_str(), slot);
}


static void Backup_Prune(const SString &dir_name, int b_low, int b_high, int wad_size)
{
	// Note: the logic here for checking space is very crude, it assumes
	//       all existing backups have the same size as the currrent wad.

	// do calculations in KB units
	wad_size = wad_size / 1024 + 1;

	int backup_num = 2 + config::backup_max_space * 1024 / wad_size;

	if (backup_num > config::backup_max_files)
		backup_num = config::backup_max_files;

	for ( ; b_low <= b_high - backup_num + 1 ; b_low++)
	{
		FileDelete(Backup_Name(dir_name, b_low).get());
	}
}


void M_BackupWad(Wad_file *wad)
{
	// disabled ?
	if (config::backup_max_files <= 0 || config::backup_max_space <= 0)
		return;

	// convert wad filename to a directory name in $cache_dir/backups

	SString filename = (global::cache_dir / "backups" / fs::u8path(wad->PathName().get()).filename()).u8string();
	SString dir_name = ReplaceExtension(filename.get(), NULL).u8string();

	gLog.debugPrintf("dir_name for backup: '%s'\n", dir_name.c_str());

	// create the directory if it doesn't already exist
	// (this will fail if it DOES already exist, but that's OK)
	FileMakeDir(dir_name.get());

	// scan directory to determine lowest and highest numbers in use
	backup_scan_data_t  scan_data;

	scan_data.low  = (1 << 30);
	scan_data.high = 0;

	if (ScanDirectory(fs::u8path(dir_name.get()), backup_scan_file, &scan_data) < 0)
	{
		// Hmmm, show a dialog ??
		gLog.printf("WARNING: backup failed (cannot scan dir)\n");
		return;
	}

	int b_low  = scan_data.low;
	int b_high = scan_data.high;

	if (b_low < b_high)
	{
		int wad_size = wad->TotalSize();

		Backup_Prune(dir_name, b_low, b_high, wad_size);
	}

	// actually back-up the file

	SString dest_name = Backup_Name(dir_name, b_high + 1);

	if (! wad->Backup(dest_name.c_str()))
	{
		// Hmmm, show a dialog ??
		gLog.printf("WARNING: backup failed (cannot copy file)\n");
		return;
	}

	gLog.printf("Backed up wad to: %s\n", dest_name.c_str());
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
