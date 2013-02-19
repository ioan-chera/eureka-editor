//------------------------------------------------------------------------
//  Debugging support
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2009 Andrew Apted
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


bool Quiet = false;
bool Debugging = false;

static FILE * log_fp;


// hack here to avoid bringing in ui_window.h and FLTK headers
extern void LogViewer_AddLine(const char *str);


void LogOpen(const char *filename)
{
	if (filename)
	{
		log_fp = fopen(filename, "w");

		LogPrintf("======= START OF LOGS =======\n");
	}
}


void LogClose(void)
{
	if (log_fp)
	{
		LogPrintf("\n======== END OF LOGS ========\n");

		fclose(log_fp);

		log_fp = NULL;
	}
}


void LogPrintf(const char *str, ...)
{
	static char buffer[MSG_BUF_LEN];

	va_list args;

	va_start(args, str);
	vsnprintf(buffer, MSG_BUF_LEN, str, args);
	va_end(args);

	buffer[MSG_BUF_LEN-1] = 0;

	LogViewer_AddLine(buffer);

/* FIXME
	if (log_fp)
	{
		va_list args;

		va_start(args, str);
		vfprintf(log_fp, str, args);
		va_end(args);

		fflush(log_fp);
	}
	else if (! Quiet)
	{
		va_list args;

		va_start(args, str);
		vfprintf(stdout, str, args);
		va_end(args);

		fflush(stdout);
	}
*/
}


void DebugPrintf(const char *str, ...)
{
	if (Debugging)
	{
		static char buffer[MSG_BUF_LEN];

		va_list args;

		va_start(args, str);
		vsnprintf(buffer, MSG_BUF_LEN-1, str, args);
		va_end(args);

		buffer[MSG_BUF_LEN-2] = 0;

		// prefix each debugging line with a special symbol

		char *pos = buffer;
		char *next;

		while (pos && *pos)
		{
			next = strchr(pos, '\n');

			if (next) *next++ = 0;

			if (log_fp)
			{
				fprintf(log_fp, "# %s\n", pos);
				fflush(log_fp);
			}
			else
				fprintf(stderr, "# %s\n", pos);

			pos = next;
		}
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
