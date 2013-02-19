//------------------------------------------------------------------------
//  Debugging support
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2013 Andrew Apted
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

// need to keep an in-memory copy of logs until the log viewer is open
static bool log_window_open;

static std::vector<const char *> kept_messages;


// hack here to avoid bringing in ui_window.h and FLTK headers
extern void LogViewer_AddLine(const char *str);


void LogOpenFile(const char *filename)
{
	log_fp = fopen(filename, "w");

	if (! log_fp)
		FatalError("Cannot open log file: %s\n", filename);

	fprintf(log_fp, "======= START OF LOGS =======\n\n");

	// add all messages saved so far

	for (unsigned int i = 0 ; i < kept_messages.size() ; i++)
		fputs(kept_messages[i], log_fp);
}


void LogOpenWindow()
{
	log_window_open = true;

	// retrieve all messages saved so far

	for (unsigned int i = 0 ; i < kept_messages.size() ; i++)
		LogViewer_AddLine(kept_messages[i]);
}


void LogClose()
{
	if (log_fp)
	{
		fprintf(log_fp, "\n\n======== END OF LOGS ========\n");

		fclose(log_fp);

		log_fp = NULL;
	}

	log_window_open = false;
}


void LogPrintf(const char *str, ...)
{
	static char buffer[MSG_BUF_LEN];

	va_list args;

	va_start(args, str);
	vsnprintf(buffer, MSG_BUF_LEN, str, args);
	va_end(args);

	buffer[MSG_BUF_LEN-1] = 0;

	if (log_fp)
	{
		fputs(buffer, log_fp);
		fflush(log_fp);
	}

	if (log_window_open)
		LogViewer_AddLine(buffer);
	else
		kept_messages.push_back(StringDup(buffer));

	if (! Quiet)
	{
		fputs(buffer, stdout);
		fflush(stdout);
	}
}


void DebugPrintf(const char *str, ...)
{
	if (Debugging && log_fp)
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

			fprintf(log_fp, "# %s\n", pos);
			fflush(log_fp);

			if (! Quiet)
			{
				fprintf(stderr, "# %s\n", pos);
			}

			pos = next;
		}
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
