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

#include "main.h"
#include "m_files.h"
#include "m_config.h"
#include "m_game.h"
#include "m_loadsave.h"
#include "w_wad.h"

#include "ui_window.h"


// list of known iwads (mapping GAME name --> PATH)

static std::map<SString, SString> known_iwads;


void M_AddKnownIWAD(const char *path)
{
	char absolute_name[FL_PATH_MAX];
	fl_filename_absolute(absolute_name, path);

	SString game = GameNameFromIWAD(path);

	known_iwads[game] = absolute_name;
}


SString M_QueryKnownIWAD(const char *game)
{
	std::map<SString, SString>::iterator KI;

	KI = known_iwads.find(game);

	if (KI != known_iwads.end())
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

	for (KI = known_iwads.begin() ; KI != known_iwads.end() ; KI++, index++)
	{
		const char *name = KI->first.c_str();

		if (result[0])
			result += '|';

		result += name;

		if (y_stricmp(name, exist_name) == 0)
			*exist_val = index;
	}

	return result;
}


void M_WriteKnownIWADs(FILE *fp)
{
	std::map<SString, SString>::iterator KI;

	for (KI = known_iwads.begin() ; KI != known_iwads.end() ; KI++)
	{
		fprintf(fp, "known_iwad %s %s\n", KI->first.c_str(), KI->second.c_str());
	}
}


void M_ValidateGivenFiles()
{
	for (const SString &pwad : Pwad_list)
	{
		if (! Wad_file::Validate(pwad.c_str()))
			FatalError("Given pwad does not exist or is invalid: %s\n",
					   pwad.c_str());
	}
}


int M_FindGivenFile(const char *filename)
{
	for (int i = 0 ; i < (int)Pwad_list.size() ; i++)
		if (Pwad_list[i] == filename)
			return i;

	return -1;  // Not Found
}


//------------------------------------------------------------------------
//  PORT PATH HANDLING
//------------------------------------------------------------------------

// the set of all known source port paths

static std::map<SString, port_path_info_t> port_paths;


port_path_info_t * M_QueryPortPath(const char *name, bool create_it)
{
	std::map<SString, port_path_info_t>::iterator IT;

	IT = port_paths.find(name);

	if (IT != port_paths.end())
		return &IT->second;

	if (create_it)
	{
		port_path_info_t info;

		memset(&info, 0, sizeof(port_path_info_t));

		port_paths[name] = info;

		return M_QueryPortPath(name);
	}

	return NULL;
}


bool M_IsPortPathValid(const port_path_info_t *info)
{
	if (strlen(info->exe_filename) < 2)
		return false;

	if (! FileExists(info->exe_filename))
		return false;

	return true;
}


void M_ParsePortPath(const char *name, char *line)
{
	while (isspace(*line))
		line++;

	char *arg_pos = line;

	(void) arg_pos;	 // shut up a warning

	line = strchr(line, '|');
	if (! line)
	{
		// TODO : Warn
		return;
	}

	// terminate arguments
	*line++ = 0;

	port_path_info_t *info = M_QueryPortPath(name, true);
	if (! info)	// should not fail!
		return;

	snprintf(info->exe_filename, sizeof(info->exe_filename), "%s", line);

	// parse any other arguments
	// [ none needed atm.... ]
}


void M_WritePortPaths(FILE *fp)
{
	std::map<SString, port_path_info_t>::iterator IT;

	for (IT = port_paths.begin() ; IT != port_paths.end() ; IT++)
	{
		port_path_info_t& info = IT->second;

		fprintf(fp, "port_path %s |%s\n", IT->first.c_str(), info.exe_filename);
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
	const char *file;
	const char *map;

public:
	recent_file_data_c(const char *_file, const char *_map) :
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

		return new recent_file_data_c(filenames[index].c_str(), map_names[index].c_str());
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

	int find(const char *file, const char *map = NULL)
	{
		// ignore the path when matching filenames
		const char *A = fl_filename_name(file);

		for (int k = 0 ; k < size ; k++)
		{
			const char *B = fl_filename_name(filenames[k].c_str());

			if (y_stricmp(A, B) != 0)
				continue;

			if (! map || y_stricmp(map_names[k].c_str(), map) == 0)
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

	void push_front(const char *file, const char *map)
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

	void insert(const char *file, const char *map)
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

	void Lookup(int index, const char ** file_v, const char ** map_v)
	{
		SYS_ASSERT(index >= 0);
		SYS_ASSERT(index < size);

		*file_v = filenames[index].c_str();
		*map_v  = map_names[index].c_str();
	}
};


static RecentFiles_c  recent_files;


static void ParseMiscConfig(FILE * fp)
{
	static char line[FL_PATH_MAX];
	static char * map;

	while (M_ReadTextLine(line, sizeof(line), fp))
	{
		// comment?
		if (line[0] == '#')
			continue;

		char *pos = strchr(line, ' ');
		if (! pos)
		{
			// FIXME warning
			continue;
		}

		*pos++ = 0;

		map = pos;

		pos = strchr(map, ' ');
		if (! pos)
		{
			// FIXME warning
			continue;
		}

		*pos++ = 0;

		if (strcmp(line, "recent") == 0)
		{
			if (Wad_file::Validate(pos))
				recent_files.insert(pos, map);
			else
				LogPrintf("  no longer exists: %s\n", pos);
		}
		else if (strcmp(line, "known_iwad") == 0)
		{
			// ignore plain freedoom.wad (backwards compatibility)
			if (y_stricmp(map, "freedoom") == 0)
				LogPrintf("  ignoring for compatibility: %s\n", pos);
			else if (Wad_file::Validate(pos))
				known_iwads[map] = SString(pos);
			else
				LogPrintf("  no longer exists: %s\n", pos);
		}
		else if (strcmp(line, "port_path") == 0)
		{
			M_ParsePortPath(map, pos);
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
	static char filename[FL_PATH_MAX];

	snprintf(filename, sizeof(filename), "%s/misc.cfg", home_dir.c_str());

	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		LogPrintf("No recent list at: %s\n", filename);
		return;
	}

	LogPrintf("Reading recent list from: %s\n", filename);

	recent_files.clear();
	 known_iwads.clear();
	  port_paths.clear();

	ParseMiscConfig(fp);

	fclose(fp);
}


void M_SaveRecent()
{
	static char filename[FL_PATH_MAX];

	snprintf(filename, sizeof(filename), "%s/misc.cfg", home_dir.c_str());

	FILE *fp = fopen(filename, "w");

	if (! fp)
	{
		LogPrintf("Failed to save recent list to: %s\n", filename);
		return;
	}

	LogPrintf("Writing recent list to: %s\n", filename);

	fprintf(fp, "# Eureka miscellaneous stuff\n");

	recent_files.WriteFile(fp);

	M_WriteKnownIWADs(fp);

	M_WritePortPaths(fp);


	fclose(fp);
}


int M_RecentCount()
{
	return recent_files.getSize();
}

SString M_RecentShortName(int index)
{
	return recent_files.Format(index);
}

void * M_RecentData(int index)
{
	return recent_files.getData(index);
}


void M_OpenRecentFromMenu(void *priv_data)
{
	SYS_ASSERT(priv_data);

	recent_file_data_c *data = (recent_file_data_c *)priv_data;

	OpenFileMap(data->file, data->map);
}


void M_AddRecent(const char *filename, const char *map_name)
{
	char absolute_name[FL_PATH_MAX];
	fl_filename_absolute(absolute_name, filename);

	recent_files.insert(absolute_name, map_name);

	M_SaveRecent();  // why wait?
}


bool M_TryOpenMostRecent()
{
	if (recent_files.getSize() == 0)
		return false;

	const char *filename;
	const char *map_name;

	recent_files.Lookup(0, &filename, &map_name);

	// M_LoadRecent has already validated the filename, so this should
	// normally work.

	Wad_file *wad = Wad_file::Open(filename, 'a');

	if (! wad)
	{
		LogPrintf("Failed to load most recent pwad: %s\n", filename);
		return false;
	}

	// make sure at least one level can be loaded
	if (wad->LevelCount() == 0)
	{
		LogPrintf("No levels in most recent pwad: %s\n", filename);

		delete wad;
		return false;
	}

	/* -- OK -- */

	if (wad->LevelFind(map_name) >= 0)
		Level_name = map_name;
	else
		Level_name.clear();

	Pwad_name = filename;

	edit_wad = wad;

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
		return ".";

	// remove trailing slash
	while (len > 1 && paths[len - 1] == DIR_SEP_CH)
		len--;

	memcpy(dir, paths, len);

	dir[len] = 0;

	return dir;
}


static SString SearchDirForIWAD(const char *dir_name, const char *game)
{
	char name_buf[FL_PATH_MAX];

	snprintf(name_buf, sizeof(name_buf), "%s/%s.wad", dir_name, game);

	DebugPrintf("  trying: %s\n", name_buf);

	if (Wad_file::Validate(name_buf))
		return name_buf;

	// try uppercasing the name, to find e.g. DOOM2.WAD

	y_strupr(name_buf + strlen(dir_name) + 1);

	DebugPrintf("  trying: %s\n", name_buf);

	if (Wad_file::Validate(name_buf))
		return name_buf;

	return "";
}


static SString SearchForIWAD(const char *game)
{
	DebugPrintf("Searching for '%s' IWAD\n", game);

	static char dir_name[FL_PATH_MAX];

	// 1. look in ~/.eureka/iwads first

	snprintf(dir_name, FL_PATH_MAX, "%s/iwads", home_dir.c_str());
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
		path = SearchDirForIWAD(SString(doomwaddir).c_str(), game);
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
	LogPrintf("Looking for IWADs....\n");

	std::vector<SString> game_list = M_CollectKnownDefs("games");

	for (const SString &game : game_list)
	{
		// already have it?
		if (!M_QueryKnownIWAD(game.c_str()).empty())
			continue;

		SString path = SearchForIWAD(game.c_str());

		if (!path.empty())
		{
			LogPrintf("Found '%s' IWAD file: %s\n", game.c_str(), path.c_str());

			M_AddKnownIWAD(path.c_str());
		}
	}

	M_SaveRecent();
}


SString M_PickDefaultIWAD()
{
	// guess either DOOM or DOOM 2 based on level names
	const char *default_game = "doom2";

	if (!Level_name.empty() && toupper(Level_name[0]) == 'E')
	{
		default_game = "doom";
	}
	else if (edit_wad)
	{
		int idx = edit_wad->LevelFindFirst();

		if (idx >= 0)
		{
			idx = edit_wad->LevelHeader(idx);
			const char *name = edit_wad->GetLump(idx)->Name();

			if (toupper(name[0]) == 'E')
				default_game = "doom";
		}
	}

	DebugPrintf("pick default iwad, trying: '%s'\n", default_game);

	SString result;

	result = M_QueryKnownIWAD(default_game);
	if (!result.empty())
		return result;

	// try FreeDoom

	if (strcmp(default_game, "doom") == 0)
		default_game = "freedoom1";
	else
		default_game = "freedoom2";

	DebugPrintf("pick default iwad, trying: '%s'\n", default_game);

	result = M_QueryKnownIWAD(default_game);
	if (!result.empty())
		return result;

	// try any known iwad

	DebugPrintf("pick default iwad, trying first known iwad...\n");

	std::map<SString, SString>::iterator KI;

	KI = known_iwads.begin();

	if (KI != known_iwads.end())
		return KI->second;

	// nothing left to try
	DebugPrintf("pick default iwad failed.\n");

	return "";
}


static void M_AddResource_Unique(const char * filename)
{
	// check if base filename (without path) already exists
	for (const SString &resource : Resource_list)
	{
		const char *A = fl_filename_name(filename);
		const char *B = fl_filename_name(resource.c_str());

		if (y_stricmp(A, B) == 0)
			return;		// found it
	}

	Resource_list.push_back(filename);
}


//
// returns false if user wants to cancel the load
//
bool M_ParseEurekaLump(Wad_file *wad, bool keep_cmd_line_args)
{
	LogPrintf("Parsing '%s' lump\n", EUREKA_LUMP);

	Lump_c * lump = wad->FindLump(EUREKA_LUMP);

	if (! lump)
	{
		LogPrintf("--> does not exist.\n");
		return true;
	}

	if (! lump->Seek())
	{
		LogPrintf("--> error seeking.\n");
		return true;
	}


	SString new_iwad;
	SString new_port;

	std::vector<SString> new_resources;


	static char line[FL_PATH_MAX];

	while (lump->GetLine(line, sizeof(line)))
	{
		// comment?
		if (line[0] == '#')
			continue;

		StringRemoveCRLF(line);

		char *pos = strchr(line, ' ');

		if (! pos || pos == line)
		{
			LogPrintf("WARNING: bad syntax in %s lump\n", EUREKA_LUMP);
			continue;
		}

		*pos++ = 0;

		if (strcmp(line, "game") == 0)
		{
			if (! M_CanLoadDefinitions("games", pos))
			{
				LogPrintf("  unknown game: %s\n", pos /* show full path */);

				int res = DLG_Confirm("&Ignore|&Cancel Load",
				                      "Warning: the pwad specifies an unsupported "
									  "game:\n\n          %s", pos);
				if (res == 1)
					return false;
			}
			else
			{
				new_iwad = M_QueryKnownIWAD(pos);

				if (new_iwad.empty())
				{
					int res = DLG_Confirm("&Ignore|&Cancel Load",
					                      "Warning: the pwad specifies an IWAD "
										  "which cannot be found:\n\n          %s.wad", pos);
					if (res == 1)
						return false;
				}
			}
		}
		else if (strcmp(line, "resource") == 0)
		{
			SString res = pos;

			// if not found at absolute location, try same place as PWAD

			if (! FileExists(res.c_str()))
			{
				LogPrintf("  file not found: %s\n", pos);

				res = FilenameReposition(pos, wad->PathName());
				LogPrintf("  trying: %s\n", res.c_str());
			}

			if (! FileExists(res.c_str()) && !new_iwad.empty())
			{
				res = FilenameReposition(pos, new_iwad.c_str());
				LogPrintf("  trying: %s\n", res.c_str());
			}

			if (FileExists(res.c_str()))
				new_resources.push_back(res);
			else
			{
				DLG_Notify("Warning: the pwad specifies a resource "
				           "which cannot be found:\n\n%s", pos);
			}
		}
		else if (strcmp(line, "port") == 0)
		{
			if (M_CanLoadDefinitions("ports", pos))
				new_port = pos;
			else
			{
				LogPrintf("  unknown port: %s\n", pos);

				DLG_Notify("Warning: the pwad specifies an unknown port:\n\n%s", pos);
			}
		}
		else
		{
			LogPrintf("WARNING: unknown keyword '%s' in %s lump\n", line, EUREKA_LUMP);
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
		if (! (keep_cmd_line_args && !Iwad_name.empty()))
			Iwad_name = new_iwad;
	}

	if (!new_port.empty())
	{
		if (! (keep_cmd_line_args && !Port_name.empty()))
			Port_name = new_port;
	}

	if (! keep_cmd_line_args)
		Resource_list.clear();

	for (const SString &resource : new_resources)
	{
		M_AddResource_Unique(resource.c_str());
	}

	return true;
}


void M_WriteEurekaLump(Wad_file *wad)
{
	LogPrintf("Writing '%s' lump\n", EUREKA_LUMP);

	wad->BeginWrite();

	int oldie = wad->FindLumpNum(EUREKA_LUMP);
	if (oldie >= 0)
		wad->RemoveLumps(oldie, 1);

	Lump_c *lump = wad->AddLump(EUREKA_LUMP);

	lump->Printf("# Eureka project info\n");

	if (!Game_name.empty())
		lump->Printf("game %s\n", Game_name.c_str());

	if (!Port_name.empty())
		lump->Printf("port %s\n", Port_name.c_str());

	for (const SString &resource : Resource_list)
	{
		char absolute_name[FL_PATH_MAX];
		fl_filename_absolute(absolute_name, resource.c_str());

		lump->Printf("resource %s\n", absolute_name);
	}

	lump->Finish();

	wad->EndWrite();
}


//------------------------------------------------------------------------
//  BACKUP SYSTEM
//------------------------------------------------------------------------


// config variables
int backup_max_files = 30;
int backup_max_space = 60;  // MB


typedef struct
{
	int low;
	int high;

} backup_scan_data_t;


static void backup_scan_file(const char *name, int flags, void *priv_dat)
{
	backup_scan_data_t * data = (backup_scan_data_t *)priv_dat;

	if (flags & SCAN_F_Hidden)
		return;

	if (flags & SCAN_F_IsDir)
		return;

	if (! isdigit(name[0]))
		return;

	int num = atoi(name);

	data->low  = MIN(data->low,  num);
	data->high = MAX(data->high, num);
}


static const char *Backup_Name(const char *dir_name, int slot)
{
	static char filename[FL_PATH_MAX];

	snprintf(filename, sizeof(filename), "%s/%d.wad", dir_name, slot);

	return filename;
}


static void Backup_Prune(const char *dir_name, int b_low, int b_high, int wad_size)
{
	// Note: the logic here for checking space is very crude, it assumes
	//       all existing backups have the same size as the currrent wad.

	// do calculations in KB units
	wad_size = wad_size / 1024 + 1;

	int backup_num = 2 + backup_max_space * 1024 / wad_size;

	if (backup_num > backup_max_files)
		backup_num = backup_max_files;

	for ( ; b_low <= b_high - backup_num + 1 ; b_low++)
	{
		FileDelete(Backup_Name(dir_name, b_low));
	}
}


void M_BackupWad(Wad_file *wad)
{
	// disabled ?
	if (backup_max_files <= 0 || backup_max_space <= 0)
		return;

	// convert wad filename to a directory name in $cache_dir/backups

	static char filename[FL_PATH_MAX];

	snprintf(filename, sizeof(filename), "%s/backups/%s", cache_dir.c_str(), fl_filename_name(wad->PathName()));

	SString dir_name = ReplaceExtension(filename, NULL);

	DebugPrintf("dir_name for backup: '%s'\n", dir_name.c_str());

	// create the directory if it doesn't already exist
	// (this will fail if it DOES already exist, but that's OK)
	FileMakeDir(dir_name.c_str());

	// scan directory to determine lowest and highest numbers in use
	backup_scan_data_t  scan_data;

	scan_data.low  = (1 << 30);
	scan_data.high = 0;

	if (ScanDirectory(dir_name.c_str(), backup_scan_file, &scan_data) < 0)
	{
		// Hmmm, show a dialog ??
		LogPrintf("WARNING: backup failed (cannot scan dir)\n");
		return;
	}

	int b_low  = scan_data.low;
	int b_high = scan_data.high;

	if (b_low < b_high)
	{
		int wad_size = wad->TotalSize();

		Backup_Prune(dir_name.c_str(), b_low, b_high, wad_size);
	}

	// actually back-up the file

	const char * dest_name = Backup_Name(dir_name.c_str(), b_high + 1);

	if (! wad->Backup(dest_name))
	{
		// Hmmm, show a dialog ??
		LogPrintf("WARNING: backup failed (cannot copy file)\n");
		return;
	}

	LogPrintf("Backed up wad to: %s\n", dest_name);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
