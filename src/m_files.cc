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
#include "m_streams.h"
#include "w_wad.h"

#include "ui_window.h"

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

// list of known iwads (mapping GAME name --> PATH)

namespace global
{
	static std::map<SString, SString> known_iwads;
}


void M_AddKnownIWAD(const SString &path)
{
	const SString &absolute_name = GetAbsolutePath(path);

	const SString &game = GameNameFromIWAD(path);

	global::known_iwads[game] = absolute_name;
}


SString M_QueryKnownIWAD(const SString &game)
{
	std::map<SString, SString>::iterator KI;

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
	std::map<SString, SString>::iterator KI;

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


static void M_WriteKnownIWADs(FILE *fp)
{
	std::map<SString, SString>::iterator KI;

	for (KI = global::known_iwads.begin() ; KI != global::known_iwads.end() ; KI++)
	{
		fprintf(fp, "known_iwad %s %s\n", KI->first.c_str(), KI->second.c_str());
	}
}


void M_ValidateGivenFiles()
{
	for (const SString &pwad : global::Pwad_list)
	{
		if (! Wad_file::Validate(pwad))
			ThrowException("Given pwad does not exist or is invalid: %s\n",
						   pwad.c_str());
	}
}


int M_FindGivenFile(const char *filename)
{
	for (int i = 0 ; i < (int)global::Pwad_list.size() ; i++)
		if (global::Pwad_list[i] == filename)
			return i;

	return -1;  // Not Found
}


//------------------------------------------------------------------------
//  PORT PATH HANDLING
//------------------------------------------------------------------------

// the set of all known source port paths

namespace global
{
	static std::map<SString, port_path_info_t> port_paths;
}


port_path_info_t * M_QueryPortPath(const SString &name, bool create_it)
{
	std::map<SString, port_path_info_t>::iterator IT;

	IT = global::port_paths.find(name);

	if (IT != global::port_paths.end())
		return &IT->second;

	if (create_it)
	{
		port_path_info_t info;
		global::port_paths[name] = info;

		return M_QueryPortPath(name);
	}

	return NULL;
}


bool M_IsPortPathValid(const port_path_info_t *info)
{
	if(info->exe_filename.length() < 2)
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
static void M_ParsePortPath(const SString &name, const SString &cpath)
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

	port_path_info_t *info = M_QueryPortPath(name, true);
	if (! info)	// should not fail!
		return;

	info->exe_filename = path;

	// parse any other arguments
	// [ none needed atm.... ]
}


void M_WritePortPaths(FILE *fp)
{
	std::map<SString, port_path_info_t>::iterator IT;

	for (IT = global::port_paths.begin() ; IT != global::port_paths.end() ; IT++)
	{
		port_path_info_t& info = IT->second;

		fprintf(fp, "port_path %s |%s\n", IT->first.c_str(), info.exe_filename.c_str());
	}
}


//------------------------------------------------------------------------
//  RECENT FILE HANDLING
//------------------------------------------------------------------------

#define MAX_RECENT  24


// this is for the "File/Recent" menu callbacks
class recent_file_data_c
{
public:
	SString file;
	SString map;

public:
	recent_file_data_c(const SString &_file, const SString &_map) :
		file(_file), map(_map)
	{ }

	recent_file_data_c()
	{ }
};


// recent filenames are never freed (atm), since they need to stay
// around for the 'File/Recent' menu.
#undef FREE_RECENT_FILES


class RecentFiles_c
{
private:
	int size;

	// newest is at index [0]
	SString filenames[MAX_RECENT];
	SString map_names[MAX_RECENT];

public:
	RecentFiles_c() : size(0)
	{
	}

	~RecentFiles_c()
	{ }

	int getSize() const
	{
		return size;
	}

	recent_file_data_c *getData(int index) const
	{
		SYS_ASSERT(0 <= index && index < size);

		return new recent_file_data_c(filenames[index], map_names[index]);
	}

	void clear()
	{
		for (int k = 0 ; k < size ; k++)
		{
			filenames[k].clear();
			map_names[k].clear();
		}

		size = 0;
	}

	int find(const SString &file, const SString &map = NULL)
	{
		// ignore the path when matching filenames
		const char *A = fl_filename_name(file.c_str());

		for (int k = 0 ; k < size ; k++)
		{
			const char *B = fl_filename_name(filenames[k].c_str());

			if (y_stricmp(A, B) != 0)
				continue;

			if (map.empty() || map_names[k].noCaseEqual(map))
				return k;
		}

		return -1;  // not found
	}

	void erase(int index)
	{
		SYS_ASSERT(0 <= index && index < MAX_RECENT);

		size--;

		SYS_ASSERT(size < MAX_RECENT);

		for ( ; index < size ; index++)
		{
			filenames[index] = filenames[index + 1];
			map_names[index] = map_names[index + 1];
		}

		filenames[index].clear();
		map_names[index].clear();
	}

	void push_front(const SString &file, const SString &map)
	{
		if (size >= MAX_RECENT)
		{
			erase(MAX_RECENT - 1);
		}

		// shift elements up
		for (int k = size - 1 ; k >= 0 ; k--)
		{
			filenames[k + 1] = filenames[k];
			map_names[k + 1] = map_names[k];
		}

		filenames[0] = file;
		map_names[0] = map;

		size++;
	}

	void insert(const SString &file, const SString &map)
	{
		// ensure filename (without any path) is unique
		int f = find(file);

		if (f >= 0)
			erase(f);

		push_front(file, map);
	}

	void WriteFile(FILE * fp)
	{
		// file is in opposite order, newest at the end
		// (this allows the parser to merely insert() items in the
		//  order they are read).

		for (int k = size - 1 ; k >= 0 ; k--)
		{
			fprintf(fp, "recent %s %s\n", map_names[k].c_str(), filenames[k].c_str());
		}
	}

	SString Format(int index) const
	{
		SYS_ASSERT(index < size);

		const char *name = fl_filename_name(filenames[index].c_str());

		char buffer[256];
		snprintf(buffer, sizeof(buffer), "%s%s%d:  %-.42s", (index < 9) ? "  " : "",
				(index < 9) ? "&" : "", 1+index, name);

		return SString(buffer);
	}

	void Lookup(int index, SString * file_v, SString * map_v)
	{
		SYS_ASSERT(index >= 0);
		SYS_ASSERT(index < size);

		*file_v = filenames[index];
		*map_v  = map_names[index];
	}
};

namespace global
{
	static RecentFiles_c  recent_files;
}

//
// Parse miscellaneous config
//
static void ParseMiscConfig(std::istream &is)
{
	SString line;
	while(M_ReadTextLine(line, is))
	{
		// comment?
		if (line[0] == '#')
			continue;

		size_t pos = line.find(' ');
		if(pos == std::string::npos)
		{
			// FIXME warning
			continue;
		}
		SString map;
		line.cutWithSpace(pos, &map);
		pos = map.find(' ');
		if(pos == std::string::npos)
		{
			// FIXME warning
			continue;
		}

		SString path;
		map.cutWithSpace(pos, &path);
		if(line == "recent")
		{
			if(Wad_file::Validate(path))
				global::recent_files.insert(path, map);
			else
				gLog.printf("  no longer exists: %s\n", path.c_str());
		}
		else if(line == "known_iwad")
		{
			// ignore plain freedoom.wad (backwards compatibility)
			if(map.noCaseEqual("freedoom"))
				gLog.printf("  ignoring for compatibility: %s\n", path.c_str());
			else if(Wad_file::Validate(path))
				global::known_iwads[map] = path;
			else
				gLog.printf("  no longer exists: %s\n", path.c_str());
		}
		else if(line == "port_path")
		{
			M_ParsePortPath(map, path);
		}
		else
		{
			// FIXME: warning
			continue;
		}
	}
}


void M_LoadRecent()
{
	SString filename = global::home_dir + "/misc.cfg";

	std::ifstream is(filename.get());
	if(!is.is_open())
	{
		gLog.printf("No recent list at: %s\n", filename.c_str());
		return;
	}

	gLog.printf("Reading recent list from: %s\n", filename.c_str());

	global::recent_files.clear();
	global::known_iwads.clear();
	global::port_paths.clear();

	ParseMiscConfig(is);
}


void M_SaveRecent()
{
	SString filename = global::home_dir + "/misc.cfg";

	FILE *fp = fopen(filename.c_str(), "w");

	if (! fp)
	{
		gLog.printf("Failed to save recent list to: %s\n", filename.c_str());
		return;
	}

	gLog.printf("Writing recent list to: %s\n", filename.c_str());

	fprintf(fp, "# Eureka miscellaneous stuff\n");

	global::recent_files.WriteFile(fp);

	M_WriteKnownIWADs(fp);

	M_WritePortPaths(fp);


	fclose(fp);
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

	OpenFileMap(data->file, data->map);
}


void M_AddRecent(const SString &filename, const SString &map_name)
{
	const SString &absolute_name = GetAbsolutePath(filename);

	global::recent_files.insert(absolute_name, map_name);

	M_SaveRecent();  // why wait?
}


bool Instance::M_TryOpenMostRecent()
{
	if (global::recent_files.getSize() == 0)
		return false;

	SString filename;
	SString map_name;

	global::recent_files.Lookup(0, &filename, &map_name);

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

	snprintf(dir_name, FL_PATH_MAX, "%s/iwads", global::home_dir.c_str());
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

			M_AddKnownIWAD(path);
		}
	}

	M_SaveRecent();
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

	SString result;

	result = M_QueryKnownIWAD(default_game);
	if (!result.empty())
		return result;

	// try FreeDoom

	if (strcmp(default_game, "doom") == 0)
		default_game = "freedoom1";
	else
		default_game = "freedoom2";

	gLog.debugPrintf("pick default iwad, trying: '%s'\n", default_game);

	result = M_QueryKnownIWAD(default_game);
	if (!result.empty())
		return result;

	// try any known iwad

	gLog.debugPrintf("pick default iwad, trying first known iwad...\n");

	std::map<SString, SString>::iterator KI;

	KI = global::known_iwads.begin();

	if (KI != global::known_iwads.end())
		return KI->second;

	// nothing left to try
	gLog.debugPrintf("pick default iwad failed.\n");

	return "";
}


static void M_AddResource_Unique(Instance &inst, const SString & filename)
{
	// check if base filename (without path) already exists
	for (const SString &resource : inst.loaded.resourceList)
	{
		const char *A = fl_filename_name(filename.c_str());
		const char *B = fl_filename_name(resource.c_str());

		if (y_stricmp(A, B) == 0)
			return;		// found it
	}

	inst.loaded.resourceList.push_back(filename);
}


//
// returns false if user wants to cancel the load
//
bool Instance::M_ParseEurekaLump(const Wad_file *wad, bool keep_cmd_line_args)
{
	gLog.printf("Parsing '%s' lump\n", EUREKA_LUMP);

	Lump_c * lump = wad->FindLump(EUREKA_LUMP);

	if (! lump)
	{
		gLog.printf("--> does not exist.\n");
		return true;
	}

	lump->Seek();

	SString new_iwad;
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
			fs::path resourcePath(value.c_str());

			if(resourcePath.is_relative())
			{
				fs::path wadDirPath = fs::path(wad->PathName().c_str()).parent_path();
				resourcePath = (wadDirPath / resourcePath).lexically_normal();
			}

			// if not found at expected location, try same place as PWAD
			if (!fs::exists(resourcePath))
			{
				gLog.printf("  file not found: %s\n", resourcePath.c_str());
				fs::path wadDirPath = fs::path(wad->PathName().c_str()).parent_path();
				resourcePath = wadDirPath / resourcePath.filename();

				gLog.printf("  trying: %s\n", resourcePath.c_str());
			}
			// Still doesn't exist? Try IWAD path, if any
			if (!fs::exists(resourcePath) && new_iwad.good())
			{
				fs::path wadDirPath = fs::path(new_iwad.c_str()).parent_path();
				resourcePath = wadDirPath / resourcePath.filename();
				gLog.printf("  trying: %s\n", resourcePath.c_str());
			}

			if (fs::exists(resourcePath))
				new_resources.push_back(resourcePath.c_str());
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
		if (! (keep_cmd_line_args && !loaded.iwadName.empty()))
			loaded.iwadName = new_iwad;
	}

	if (!new_port.empty())
	{
		if (! (keep_cmd_line_args && !loaded.portName.empty()))
			loaded.portName = new_port;
	}

	if (! keep_cmd_line_args)
		loaded.resourceList.clear();

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

	for (const SString &resource : resourceList)
	{
		fs::path absoluteResourcePath = fs::absolute(resource.c_str());
		fs::path relative = fs::proximate(absoluteResourcePath, pwadPath);

		lump->Printf("resource %s\n", relative.generic_string().c_str());
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


static void backup_scan_file(const SString &name, int flags, void *priv_dat)
{
	backup_scan_data_t * data = (backup_scan_data_t *)priv_dat;

	if (flags & SCAN_F_Hidden)
		return;

	if (flags & SCAN_F_IsDir)
		return;

	if (! isdigit(name[0]))
		return;

	int num = atoi(name);

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
		FileDelete(Backup_Name(dir_name, b_low));
	}
}


void M_BackupWad(Wad_file *wad)
{
	// disabled ?
	if (config::backup_max_files <= 0 || config::backup_max_space <= 0)
		return;

	// convert wad filename to a directory name in $cache_dir/backups

	SString filename = global::cache_dir + "/backups/" + fl_filename_name(wad->PathName().c_str());
	SString dir_name = ReplaceExtension(filename, NULL);

	gLog.debugPrintf("dir_name for backup: '%s'\n", dir_name.c_str());

	// create the directory if it doesn't already exist
	// (this will fail if it DOES already exist, but that's OK)
	FileMakeDir(dir_name);

	// scan directory to determine lowest and highest numbers in use
	backup_scan_data_t  scan_data;

	scan_data.low  = (1 << 30);
	scan_data.high = 0;

	if (ScanDirectory(dir_name, backup_scan_file, &scan_data) < 0)
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
