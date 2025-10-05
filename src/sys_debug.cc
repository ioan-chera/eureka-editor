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

// FIXME: make this a local entity passed to interested parties
Log gLog;

// hack here to avoid bringing in ui_window.h and FLTK headers
extern void LogViewer_AddLine(const char *str);

//
// Open a file
//
bool Log::openFile(const fs::path &filename)
{
	log_fp = UTF8_fopen(filename.u8string().c_str(), "w+");

	if (! log_fp)
		return false;

	fprintf(log_fp, "======= START OF LOGS =======\n\n");

	// add all messages saved so far

	for (const SString &message : kept_messages)
		fputs(message.c_str(), log_fp);
	return true;
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
void Log::openWindow(WindowAddCallback callback, void *userData)
{
    setWindowAddCallback(callback, userData);
    openWindow();
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
    kept_messages.clear();
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
    kept_messages.push_back(buffer);

	if (! global::Quiet)
	{
		fputs(buffer.c_str(), stdout);
		fflush(stdout);
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
            if(index == buffer.length() && startpos == index)
                break;
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
void Log::saveTo(std::ostream &os) const
{
    os << "======= START OF LOGS =======\n\n";

    // add all messages saved so far

    for (const SString &message : kept_messages)
        os << message;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
