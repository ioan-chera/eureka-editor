//------------------------------------------------------------------------
//  Recent Files / Known Iwads
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012 Andrew Apted
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
#include "levels.h"
#include "e_loadsave.h"
#include "m_dialog.h"
#include "m_game.h"
#include "w_wad.h"

#include "ui_window.h"


// list of known iwads (mapping GAME name --> PATH)

static std::map<std::string, std::string> known_iwads;


#define MAX_RECENT  8


class RecentFiles_c
{
private:
	int size;

	// newest is at index [0]
	const char * filenames[MAX_RECENT];
	const char * map_names[MAX_RECENT];

public:
	RecentFiles_c() : size(0)
	{
		for (int k = 0 ; k < MAX_RECENT ; k++)
		{
			filenames[k] = map_names[k] = NULL;
		}
	}

	~RecentFiles_c()
	{ }

	int getSize() const
	{
		return size;
	}

	void clear()
	{
		for (int k = 0 ; k < size ; k++)
		{
			StringFree(filenames[k]);
			StringFree(map_names[k]);

			filenames[k] = NULL;
			map_names[k] = NULL;
		}

		size = 0;
	}

	int find(const char *file, const char *map = NULL)
	{
		// ignore the path when matching filenames
		const char *A = fl_filename_name(file);

		for (int k = 0 ; k < size ; k++)
		{
			const char *B = fl_filename_name(filenames[k]);

			if (y_stricmp(A, B) != 0)
				continue;

			if (! map || y_stricmp(map_names[k], map) == 0)
				return k;
		}

		return -1;  // not found
	}

	void erase(int index)
	{
		StringFree(filenames[index]);
		StringFree(map_names[index]);

		size--;

		for ( ; index < size ; index++)
		{
			filenames[index] = filenames[index + 1];
			map_names[index] = map_names[index + 1];
		}

		filenames[index] = NULL;
		map_names[index] = NULL;
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

		filenames[0] = StringDup(file);
		map_names[0] = StringDup(map);

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
			fprintf(fp, "recent %s %s\n", map_names[k], filenames[k]);
		}
	}

	void Format(char *buffer, int index) const
	{
		SYS_ASSERT(index < size);

		const char *file = fl_filename_name(filenames[index]);
		const char *map  = map_names[index];

		sprintf(buffer, "%-.32s : %-.10s", file, map);
	}

	void Lookup(int index, const char ** file_v, const char ** map_v)
	{
		SYS_ASSERT(index >= 0);
		SYS_ASSERT(index < size);

		*file_v = filenames[index];
		*map_v  = map_names[index];
	}
};


static RecentFiles_c  recent_files;


static void ParseMiscConfig(FILE * fp)
{
	static char line [FL_PATH_MAX];
	static char * map;

	// skip comment on first line
	fgets(line, sizeof(line), fp);

	while (fgets(line, sizeof(line), fp) != NULL)
	{
		StringRemoveCRLF(line);

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
			if (FileExists(pos))
				recent_files.insert(pos, map);
			else
				LogPrintf("  no longer exists: %s\n", pos);
		}
		else if (strcmp(line, "known_iwad") == 0)
		{
			if (FileExists(pos))
				known_iwads[map] = std::string(pos);
			else
				LogPrintf("  no longer exists: %s\n", pos);
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

	sprintf(filename, "%s/misc.cfg", home_dir);

	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		LogPrintf("No recent list at: %s\n", filename);
		return;
	}

	LogPrintf("Reading recent list from: %s\n", filename);

	recent_files.clear();
	 known_iwads.clear();

	ParseMiscConfig(fp);

	fclose(fp);
}


void M_SaveRecent()
{
	static char filename[FL_PATH_MAX];

	sprintf(filename, "%s/misc.cfg", home_dir);

	FILE *fp = fopen(filename, "w");

	if (! fp)
	{
		LogPrintf("Failed to save recent list to: %s\n", filename);
		return;
	}

	LogPrintf("Writing recent list to: %s\n", filename);

	fprintf(fp, "# Eureka miscellaneous stuff\n");

	recent_files.WriteFile(fp);

	// known iwad files

	std::map<std::string, std::string>::iterator KI;

	for (KI = known_iwads.begin() ; KI != known_iwads.end() ; KI++)
	{
		fprintf(fp, "known_iwad %s %s\n", KI->first.c_str(), KI->second.c_str());
	}

	fclose(fp);
}


void M_AddRecent(const char *filename, const char *map_name)
{
	char absolute_name[FL_PATH_MAX];
	fl_filename_absolute(absolute_name, filename);

	recent_files.insert(absolute_name, map_name);

	M_SaveRecent();  // why wait?
}


void M_AddKnownIWAD(const char *game, const char *path)
{
	char absolute_name[FL_PATH_MAX];
	fl_filename_absolute(absolute_name, path);

	known_iwads[game] = std::string(absolute_name);
}


const char * M_QueryKnownIWAD(const char *game)
{
	std::map<std::string, std::string>::iterator KI;

	KI = known_iwads.find(game);

	if (KI != known_iwads.end())
		return KI->second.c_str();
	else
		return NULL;
}


const char * M_KnownIWADsForMenu(int *exist_val, const char *exist_name)
{
	exist_name = fl_filename_name(exist_name);

	std::map<std::string, std::string>::iterator KI;

	static char result[2000];
	result[0] = 0;

	int index = 0;

	for (KI = known_iwads.begin() ; KI != known_iwads.end() ; KI++, index++)
	{
		const char *name = KI->first.c_str();
		
		strcat(result, "|");
		strcat(result, name);
///		strcat(result, ".wad");

		if (y_stricmp(fl_filename_name(KI->second.c_str()), exist_name) == 0)
			*exist_val = index;
	}

	return StringDup(result + 1);
}


//------------------------------------------------------------------------


static const char * SearchDirForIWAD(const char *dir_name, const char *game)
{
	char name_buf[FL_PATH_MAX];

	sprintf(name_buf, "%s/%s.wad", dir_name, game);

	DebugPrintf("  trying: %s\n", name_buf);

	if (FileExists(name_buf))
		return StringDup(name_buf);

	// try uppercasing the name, to find e.g. DOOM2.WAD

	y_strupr(name_buf + strlen(dir_name) + 1);

	DebugPrintf("  trying: %s\n", name_buf);

	if (FileExists(name_buf))
		return StringDup(name_buf);

	return NULL;
}


static const char * SearchForIWAD(const char *game)
{
	DebugPrintf("Searching for '%s' IWAD\n", game);

	static char dir_name[FL_PATH_MAX];

	// 1. look in ~/.eureka/iwads first

	snprintf(dir_name, FL_PATH_MAX, "%s/iwads", home_dir);
	dir_name[FL_PATH_MAX-1] = 0;

	const char * path = SearchDirForIWAD(dir_name, game);
	if (path)
		return path;

	// 2. look in $DOOMWADDIR

	/* TODO: support $DOOMWADPATH */

	const char *doomwaddir = getenv("DOOMWADDIR");
	if (doomwaddir)
	{
		path = SearchDirForIWAD(StringDup(doomwaddir), game);
		if (path)
			return path;
	}

	// 3. look in various standard places

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
		if (path)
			return path;
	}

	// 4. last resort : the current directory

	path = SearchDirForIWAD(".", game);
	if (path)
		return path;

	return NULL;  // not found
}


/*
 * search for iwads in various places
 */
void M_LookForIWADs()
{
	LogPrintf("Looking for IWADs....\n");

	string_list_t  game_list;

	M_CollectKnownDefs("games", game_list);

	for (unsigned int i ; i < game_list.size() ; i++)
	{
		const char *game = game_list[i];

		// already have it?
		if (M_QueryKnownIWAD(game))
			continue;

		const char *path = SearchForIWAD(game);

		if (path)
		{
			LogPrintf("Found '%s' IWAD file: %s\n", game, path);

			M_AddKnownIWAD(game, path);
		}
	}

	M_SaveRecent();
}


const char * M_PickDefaultIWAD()
{
	// guess either DOOM or DOOM 2 based on level names
	const char *default_game = "doom2";

	if (Level_name && toupper(Level_name[0]) == 'E')
	{
		default_game = "doom";
	}
	else if (edit_wad)
	{
		int lev = edit_wad->FindFirstLevel();
		const char *lev_name = "";

		if (lev >= 0)
		{
			lev_name = edit_wad->GetLump(lev)->Name();

			if (toupper(lev_name[0]) == 'E')
				default_game = "doom";
		}
	}

	DebugPrintf("pick default iwad, trying: '%s'\n", default_game);

	const char *result;
	
	if ((result = M_QueryKnownIWAD(default_game)))
		return StringDup(result);

	DebugPrintf("pick default iwad, trying: 'freedoom'\n");

	if ((result = M_QueryKnownIWAD("freedoom")))
		return StringDup(result);

	// try any known iwad

	DebugPrintf("pick default iwad, trying first known iwad...\n");

	std::map<std::string, std::string>::iterator KI;

	KI = known_iwads.begin();

	if (KI != known_iwads.end())
		return StringDup(KI->second.c_str());

	// nothing left to try
	DebugPrintf("pick default iwad failed.\n");

	return NULL;
}


void M_ParseEurekaLump(Wad_file *wad)
{
	LogPrintf("Parsing '%s' lump\n", EUREKA_LUMP);

	Lump_c * lump = wad->FindLump(EUREKA_LUMP);

	if (! lump)
	{
		LogPrintf("--> does not exist.\n");
		return;
	}
	
	if (! lump->Seek())
	{
		LogPrintf("--> error seeking.\n");
		return;
	}

	ResourceWads.clear();

	static char line[FL_PATH_MAX];

	// skip comment on first line
	lump->GetLine(line, sizeof(line));

	while (lump->GetLine(line, sizeof(line)))
	{
		StringRemoveCRLF(line);

		char *pos = strchr(line, ' ');
		if (! pos)
		{
			// FIXME warning
			continue;
		}

		*pos++ = 0;

		if (strcmp(line, "iwad") == 0)
		{
			if (FileExists(pos))
				Iwad_name = StringDup(pos);
			else
				LogPrintf("  no longer exists: %s\n", pos);
		}
		else if (strcmp(line, "resource") == 0)
		{
			if (FileExists(pos))
				ResourceWads.push_back(StringDup(pos));
			else
				LogPrintf("  no longer exists: %s\n", pos);
		}
		else if (strcmp(line, "port") == 0)
		{
			Port_name = StringDup(pos);
		}
		else
		{
			// FIXME: warning
			continue;
		}
	}
}


void M_WriteEurekaLump(Wad_file *wad)
{
	LogPrintf("Writing '%s' lump\n", EUREKA_LUMP);

	int oldie = wad->FindLumpNum(EUREKA_LUMP);
	if (oldie >= 0)
		wad->RemoveLumps(oldie, 1);
	
	Lump_c *lump = wad->AddLump(EUREKA_LUMP);

	lump->Printf("# Eureka project info\n");

	if (Iwad_name)
		lump->Printf("iwad %s\n", Iwad_name);

	if (Port_name)
		lump->Printf("port %s\n", Port_name);

	for (unsigned int i = 0 ; i < ResourceWads.size() ; i++)
		lump->Printf("resource %s\n", ResourceWads[i]);

	lump->Finish();
}


//------------------------------------------------------------------------


class UI_RecentFiles : public Fl_Double_Window
{
private:
	Fl_Button * cancel;

	bool want_close;

	int picked_num;

	static UI_RecentFiles * _instance;  // meh!

private:
	static void close_callback(Fl_Widget *w, void *data)
	{
		_instance->picked_num = -1;
		_instance->want_close = true;
	}

	static void button_callback(Fl_Widget *w, void *data)
	{
		_instance->picked_num = (int) data;
		_instance->want_close = true;
	}

	static int need_h()
	{
		return recent_files.getSize() * 28 + 144;
	}

public:
	UI_RecentFiles() : Fl_Double_Window(320, need_h(), "Recent Files"),
		want_close(false), picked_num(-1)
	{
		int W = w();
		int H = h();

		int cy = 10;

		_instance = this;

		color(WINDOW_BG, WINDOW_BG);
		callback(close_callback, this);

		int total_num = recent_files.getSize();

		Fl_Box *title = new Fl_Box(10, cy, W - 20, 44, "Select the recent file and map");
		title->labelsize(16);
		title->labelfont(FL_HELVETICA_BOLD);
		title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

		if (total_num == 0)
			title->label("  There are no recent files");

		cy += title->h() + 10;

		for (int i = 0 ; i < total_num ; i++)
		{
			char number[64];
			sprintf(number, "%d.", 1 + i);

			Fl_Box * num_box = new Fl_Box(FL_NO_BOX, 10, cy, 35, 25, "");
			num_box->copy_label(number);
			num_box->labelfont(FL_HELVETICA_BOLD);

			char name_buf[256];

			recent_files.Format(name_buf, i);

			Fl_Button * but = new Fl_Button(50, cy, W - 70, 24);
			but->copy_label(name_buf);
			but->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
			but->callback(button_callback, (void *) i);

			cy += 28;
		}

		cancel = new Fl_Button(W / 2 - 45, H - 50, 90, 35, "Cancel");
		cancel->callback(close_callback, this);

		if (total_num == 0)
			cancel->label("OK");

		end();

		resizable(NULL);
	}

	~UI_RecentFiles()
	{
		_instance = NULL;
	}

	int Run()
	{
		set_modal();

		show();

		while (! want_close)
			Fl::wait(0.2);

		return picked_num;
	}
};


UI_RecentFiles * UI_RecentFiles::_instance = NULL;


void M_RecentDialog(const char ** file_v, const char ** map_v)
{
	UI_RecentFiles * dialog = new UI_RecentFiles();

	int picked = dialog->Run();

	delete dialog;

	if (picked >= 0)
	{
		recent_files.Lookup(picked, file_v, map_v);
	}
	else
	{
		*file_v = NULL;
		*map_v  = NULL;
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
