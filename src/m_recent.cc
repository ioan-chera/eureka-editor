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
	{ }

	~RecentFiles_c()
	{ }

	void clear()
	{
		for (int k = 0 ; k < size ; k++)
		{
			StringFree(filenames[k]);
			StringFree(map_names[k]);

			filenames[k] = NULL;
			map_names[k] = 0;
		}

		size = 0;
	}

	int find(const char *file)
	{
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
		StringFree(filenames[idx]);
		StringFree(map_names[idx]);

		size--;

		for ( ; index < size ; index++)
		{
			filenames[index] = filenames[index + 1];
			map_names[index] = map_names[index + 1];
		}

		filenames[size] = NULL;
		map_names[size] = NULL;
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
		// TODO
	}

	void WriteFile(FILE * fp)
	{
		// file is in opposite order, newest at the end

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
	// FIXME
}


void M_SaveRecent()
{
	// FIXME
}


void M_AddRecent(const char *filename, const char *map_name)
{
	recent_files.push_front(filename, map_name);
}


//------------------------------------------------------------------------


void M_RecentFilesDialog()
{
	// FIXME
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
