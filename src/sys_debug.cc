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

#include "Errors.h"
#include "m_strings.h"
#include "sys_debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <vector>

bool global::Quiet = false;
bool global::Debugging = false;
bool global::in_fatal_error;

static FILE * log_fp;

// need to keep an in-memory copy of logs until the log viewer is open
static bool log_window_open;

static std::vector<SString> kept_messages;


// hack here to avoid bringing in ui_window.h and FLTK headers
extern void LogViewer_AddLine(const char *str);


void LogOpenFile(const char *filename)
{
	log_fp = fopen(filename, "w+");

	if (! log_fp)
		ThrowException("Cannot open log file: %s\n", filename);

	fprintf(log_fp, "======= START OF LOGS =======\n\n");

	// add all messages saved so far

	for (const SString &message : kept_messages)
		fputs(message.c_str(), log_fp);
}

//
// Open a file
//
bool Log::openFile(const SString &filename)
{
	log_fp = fopen(filename.c_str(), "w+");

	if (! log_fp)
		return false;

	fprintf(log_fp, "======= START OF LOGS =======\n\n");

	// add all messages saved so far

	for (const SString &message : kept_messages)
		fputs(message.c_str(), log_fp);
	return true;
}

void LogOpenWindow()
{
	log_window_open = true;

	// retrieve all messages saved so far

	for (const SString &message : kept_messages)
		LogViewer_AddLine(message.c_str());
}

//
// Open a window
//
void Log::openWindow()
{
	log_window_open = true;

	// retrieve all messages saved so far

	if(windowAdd)
		for (const SString &message : kept_messages)
			windowAdd(message, windowAddUserData);
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
    kept_messages.clear();
}

//
// Close the file
//
void Log::close()
{
	if (log_fp)
	{
		fprintf(log_fp, "\n\n======== END OF LOGS ========\n");

		fclose(log_fp);

		log_fp = nullptr;
	}

	log_window_open = false;
}

void LogPrintf(EUR_FORMAT_STRING(const char *str), ...)
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

	if (log_window_open && !global::in_fatal_error)
		LogViewer_AddLine(buffer);
    kept_messages.push_back(buffer);

	if (! global::Quiet)
	{
		fputs(buffer, stdout);
		fflush(stdout);
	}
}

//
// Printf to log
//
void Log::printf(EUR_FORMAT_STRING(const char *str), ...)
{
	va_list args;

	va_start(args, str);
	SString buffer = SString::vprintf(str, args);
	va_end(args);

	if (log_fp)
	{
		fputs(buffer.c_str(), log_fp);
		fflush(log_fp);
	}

	if (windowAdd && log_window_open && !inFatalError)
		windowAdd(buffer, windowAddUserData);
	else
		kept_messages.push_back(buffer);

	if (! global::Quiet)
	{
		fputs(buffer.c_str(), stdout);
		fflush(stdout);
	}
}

void DebugPrintf(EUR_FORMAT_STRING(const char *str), ...)
{
	if (global::Debugging && log_fp)
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

			if (! global::Quiet)
			{
				fprintf(stderr, "# %s\n", pos);
			}

			pos = next;
		}
	}
}

//
// Debug printf
//
void Log::debugPrintf(EUR_FORMAT_STRING(const char *str), ...)
{
	if (global::Debugging && log_fp)
	{
		va_list args;

		va_start(args, str);
		SString buffer = SString::vprintf(str, args);
		va_end(args);

		// prefix each debugging line with a special symbol

		size_t index = 0;
		size_t startpos = 0;
		while(index < buffer.length())
		{
			index = buffer.find('\n', startpos);
			if(index == SString::npos)
				index = buffer.length();
			SString fragment(buffer.c_str() + startpos, static_cast<int>(index - startpos));

			fprintf(log_fp, "# %s\n", fragment.c_str());
			fflush(log_fp);

			if (! global::Quiet)
				fprintf(stderr, "# %s\n", fragment.c_str());

			startpos = index + 1;
		}
	}
}

//
// Save the log so far to another file
//
void LogSaveTo(FILE *dest_fp)
{
    fprintf(dest_fp, "======= START OF LOGS =======\n\n");

    // add all messages saved so far

    for (const SString &message : kept_messages)
        fputs(message.c_str(), dest_fp);
}

void Log::saveTo(FILE *dest_fp) const
{
	uint8_t buffer[256];

	if (! log_fp)
	{
		fprintf(dest_fp, "No logs.\n");
		return;
	}

	// copy the log file

	rewind(log_fp);

	while (true)
	{
		size_t rlen = fread(buffer, 1, sizeof(buffer), log_fp);

		if (rlen == 0)
			break;

		fwrite(buffer, 1, rlen, dest_fp);
	}

	// restore write position for the log file

	fseek(log_fp, 0L, SEEK_END);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
