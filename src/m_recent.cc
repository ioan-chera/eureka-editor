//------------------------------------------------------------------------
//  Recently Edited Files
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
#include "w_wad.h"

#include "ui_window.h"


#define MAX_RECENT  10


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

	int find(const char *file)
	{
		// ignore the path when matching filenames
		const char *A = fl_filename_name(file);

		for (int k = 0 ; k < size ; k++)
		{
			const char *B = fl_filename_name(filenames[k]);

			if (y_stricmp(A, B) == 0)
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

	void ParseFile(FILE * fp)
	{
		static char name[1024];
		static char map [64];

		// skip comment on first line
		fgets(name, sizeof(name), fp);

		while (fgets(name, sizeof(name), fp) != NULL &&
		       fgets(map,  sizeof(map),  fp) != NULL)
		{
			StringRemoveCRLF(name);
			StringRemoveCRLF(map);

			insert(name, map);
		}
	}

	void WriteFile(FILE * fp)
	{
		// file is in opposite order, newest at the end
		// (this allows the parser to merely insert() items in the
		//  order they are read).

		fprintf(fp, "# Eureka recent file list\n");

		for (int k = size - 1 ; k >= 0 ; k--)
		{
			fprintf(fp, "%s\n", filenames[k]);
			fprintf(fp, "%s\n", map_names[k]);
		}
	}
};


static RecentFiles_c  recent_files;


void M_LoadRecent()
{
	static char filename[FL_PATH_MAX];

	sprintf(filename, "%s/recent.cfg", home_dir);

	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		LogPrintf("No recent list at: %s\n", filename);
		return;
	}

	LogPrintf("Reading recent list from: %s\n", filename);

	recent_files.clear();
	recent_files.ParseFile(fp);

	fclose(fp);
}


void M_SaveRecent()
{
	static char filename[FL_PATH_MAX];

	sprintf(filename, "%s/recent.cfg", home_dir);

	FILE *fp = fopen(filename, "w");

	if (! fp)
	{
		LogPrintf("Failed to save recent list to: %s\n", filename);
		return;
	}

	LogPrintf("Writing recent list to: %s\n", filename);

	recent_files.WriteFile(fp);

	fclose(fp);
}


void M_AddRecent(const char *filename, const char *map_name)
{
	recent_files.insert(filename, map_name);

	M_SaveRecent();  // why wait?
}


//------------------------------------------------------------------------


class UI_RecentFiles : public Fl_Double_Window
{
private:
	Fl_Button * name_but[MAX_RECENT];

	Fl_Button * cancel;

	bool want_close;

public:
	UI_RecentFiles() : Fl_Double_Window(320, 400, "Recent Maps"),
		want_close(false)
	{
		int W = w();
		int H = h();

		int cy = 10;

		color(WINDOW_BG, WINDOW_BG);

		Fl_Box *title = new Fl_Box(10, cy, W - 20, 44, "Select recent file and map:");
		title->labelsize(16);
		title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

		cy += title->h() + 12;

	  { Fl_Button* o = new Fl_Button(10, cy, 295, 20, "klog2.wad : MAP01");
		  o->box(FL_ROUND_UP_BOX);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		  cy += 30;
	  } // Fl_Button* o
	  { Fl_Button* o = new Fl_Button(10, cy, 295, 20, "her_boss3.wad : E1M1");
		  o->box(FL_ROUND_UP_BOX);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		  cy += 30;
	  } // Fl_Button* o
	  { Fl_Button* o = new Fl_Button(10, cy, 295, 20, "foobie.wad : MAP07");
		  o->box(FL_ROUND_UP_BOX);
		  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		  cy += 30;
	  } // Fl_Button* o

		cancel = new Fl_Button(W / 2 - 45, H - 60, 90, 35, "Cancel");
		cancel->box(FL_ROUNDED_BOX);

		end();

		resizable(NULL);
	}

	~UI_RecentFiles()
	{ }

	void Run()
	{
		set_modal();

		show();

		while (! want_close)
			Fl::wait(0.2);
	}
};


void M_RecentFilesDialog()
{
	// FIXME

	UI_RecentFiles * dialog = new UI_RecentFiles();

	dialog->Run();

	delete dialog;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
