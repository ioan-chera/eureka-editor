//------------------------------------------------------------------------
//  File Utilities
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2016 Andrew Apted
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
#include "lib_file.h"
#include "sys_debug.h"

#ifdef WIN32
#include <io.h>
#include "m_strings.h"
#else // UNIX or MACOSX
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __APPLE__
#include <sys/param.h>
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif

#ifndef PATH_MAX
#define PATH_MAX  2048
#endif


bool FileExists(const fs::path &filename)
{
	try
	{
		return fs::is_regular_file(filename);
	}
	catch(const fs::filesystem_error &e)
	{
		// Let's print any errors before we throw anything
		gLog.printf("File check error: %s\n", e.what());
		return false;
	}
}


bool HasExtension(const fs::path &filename)
{
	fs::path extension = filename.extension();
	return extension != "." && !extension.empty();
}

//
// MatchExtension
//
// When ext is NULL, checks if the file has no extension.
//
bool MatchExtensionNoCase(const fs::path &filename, const char *extension)
{
	if (!extension || !*extension)
		return ! HasExtension(filename);
	if(!HasExtension(filename))
		return false;

	return SString(filename.extension().u8string()).noCaseEqual(extension);
}


//
// ReplaceExtension
//
// When ext is NULL, any existing extension is removed.
//
// Returned string is a COPY.
//
fs::path ReplaceExtension(const fs::path &filename, const char *extension)
{
	if(filename.filename() == ".." && (!extension || !*extension))
		return filename;
	fs::path result(filename);
	result.replace_extension(extension ? extension : "");
	return result;
}

//
// Get the basename of a path
//
fs::path GetBaseName(const fs::path &path)
{
	// Find the base name of the file (i.e. without any path).
	// The result always points within the given string.
	//
	// Example:  "C:\Foo\Bar.wad"  ->  "Bar.wad"
	return path.filename();
}

bool FilenameIsBare(const fs::path &filename)
{
	return !filename.has_extension() && filename == filename.filename();
}


static void FilenameStripBase(char *buffer)
{
	if(!*buffer)
	{
		return;	// empty buffer, can't do much
	}
	char *pos = buffer + strlen(buffer) - 1;

	for (; pos > buffer ; pos--)
	{
		if (*pos == '/')
			break;

#ifdef WIN32
		if (*pos == '\\')
			break;

		if (*pos == ':')
		{
			pos[1] = 0;
			return;
		}
#endif
	}

	if (pos > buffer)
		*pos = 0;
	else
	{
		// At this point it's guaranteed not to be empty
		buffer[0] = '.';
		buffer[1] = 0;
	}
}

//
// Clears the basename
//
static void FilenameStripBase(SString &path)
{
	if(path.empty())
	{
		path = ".";
		return;
	}

#ifdef _WIN32
	size_t seppos = path.find_last_of("\\/");
	size_t colonpos = path.rfind(':');
	if(seppos != SString::npos)
	{
		if(colonpos != SString::npos && colonpos > seppos)
		{
			if(colonpos < path.size() - 1)
				path.erase(colonpos + 1, SString::npos);
			return;
		}
		if(seppos == 0)
		{
			path = "\\";
			return;
		}
		path.erase(seppos, SString::npos);
		return;
	}
	path = ".";
	return;
#else
	size_t seppos = path.find_last_of("/");
	if(seppos != SString::npos)
	{
		if(seppos == 0)
		{
			path = "/";
			return;
		}
		path.erase(seppos, SString::npos);
		return;
	}
	path = ".";
	return;
#endif
}

//
// Get path
//
fs::path FilenameGetPath(const fs::path &filename)
{
	fs::path parent(filename.parent_path());
	return parent.empty() ? "." : parent;
}

//
// Safe wrapper around fl_filename_absolute
//
fs::path GetAbsolutePath(const fs::path &path)
{
	return fs::absolute(path);
}

bool FileDelete(const fs::path &filename)
{
#ifdef WIN32
	// TODO: set wide character here
	return (::DeleteFileW(filename.c_str()) != 0);

#else // UNIX or MACOSX

	return (remove(filename.c_str()) == 0);
#endif
}


bool FileChangeDir(const fs::path &dir_name)
{
	try
	{
		fs::current_path(dir_name);
	}
	catch(const fs::filesystem_error &e)
	{
		gLog.printf("Error changing directory to %s: %s\n", dir_name.u8string().c_str(), e.what());
		return false;
	}
	return true;
}


bool FileMakeDir(const fs::path &dir_name)
{
	try
	{
		return fs::create_directory(dir_name);
	}
	catch(const fs::filesystem_error &e)
	{
		gLog.printf("Error creating directory %s: %s\n", dir_name.u8string().c_str(), e.what());
		return false;
	}
}

bool FileLoad(const fs::path &filename, std::vector<uint8_t> &data)
{
	try
	{
		// Don't try to read unusual files
		if(!fs::is_regular_file(filename))
			return false;

		uintmax_t size = fs::file_size(filename);

		std::ifstream stream(filename, std::ios::binary);
		if(!stream.is_open())
			return false;

		std::vector<uint8_t> trydata;
		trydata.resize(size);

		size_t pos = 0;

		while(!stream.eof() && pos < size)
		{
			if(!stream.read(reinterpret_cast<char *>(trydata.data() + pos), size - pos))
				return false;
			pos += stream.gcount();
		}

		data = std::move(trydata);
	}
	catch(const fs::filesystem_error &e)
	{
		gLog.printf("Error loading file %s: %s\n", filename.u8string().c_str(), e.what());
		return false;
	}
	return true;
}

//------------------------------------------------------------------------

//
// Scan a directory
//
int ScanDirectory(const fs::path &path, const std::function<void(const fs::path &, int)> &func)
{
	try
	{
		int count = 0;
		for(const auto &dir_entry : fs::directory_iterator(path))
		{
			int flags = 0;
			if(fs::is_directory(dir_entry))
				flags |= SCAN_F_IsDir;
			if((fs::status(dir_entry).permissions() & (fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write)) == fs::perms::none)
			{
				flags |= SCAN_F_ReadOnly;
			}
			fs::path entry_name = dir_entry.path().filename();
			SString entry_string = entry_name.u8string();
			if(entry_string.length() >= 2 && entry_string[0] == '.' && isalnum(entry_string[1]))
			{
				flags |= SCAN_F_Hidden;
			}

			func(entry_name, flags);
			++count;
		}
		return count;
	}
	catch(const fs::filesystem_error &e)
	{
		gLog.printf("%s (%d)\n", e.what(), e.code().value());
		if(!fs::exists(path))
			return SCAN_ERR_NoExist;
		if(!fs::is_directory(path))
			return SCAN_ERR_NotDir;
		return SCAN_ERROR;
	}
}

int ScanDirectory(const fs::path &path, directory_iter_f func, void *priv_dat)
{
	return ScanDirectory(path, [func, priv_dat](const fs::path &name, int flags)
						 {
		func(name, flags, priv_dat);
	});
}

//------------------------------------------------------------------------

fs::path GetExecutablePath(const char *argv0)
{
	SString path;

#ifdef WIN32
	wchar_t wpath[PATH_MAX / 2 + 1];
	DWORD length = GetModuleFileNameW(GetModuleHandleW(nullptr), wpath, _countof(wpath));
	if(length > 0 && length < PATH_MAX / 2)
	{
		if(_waccess(wpath, 0) == 0)
		{
			SString retpath = WideToUTF8(wpath);
			FilenameStripBase(retpath);
			return fs::u8path(retpath.get());
		}
	}

#elif !defined(__APPLE__) // UNIX
	char rawpath[PATH_MAX+2];

	int length = readlink("/proc/self/exe", rawpath, PATH_MAX);

	if (length > 0)
	{
		rawpath[length] = 0; // add the missing NUL

		if (access(rawpath, 0) == 0)  // sanity check
		{
			FilenameStripBase(rawpath);
			return fs::u8path(rawpath);
		}
	}

#else
	/*
	   from http://www.hmug.org/man/3/NSModule.html

	   extern int _NSGetExecutablePath(char *buf, uint32_t *bufsize);

	   _NSGetExecutablePath copies the path of the executable
	   into the buffer and returns 0 if the path was successfully
	   copied in the provided buffer. If the buffer is not large
	   enough, -1 is returned and the expected buffer size is
	   copied in *bufsize.
	 */
	uint32_t pathlen = PATH_MAX * 2;
	char rawpath[2 * PATH_MAX + 2];

	if (0 == _NSGetExecutablePath(rawpath, &pathlen))
	{
		// FIXME: will this be _inside_ the .app folder???
		FilenameStripBase(rawpath);
		return fs::u8path(rawpath);
	}
#endif

	// fallback method: use argv[0]
	path = argv0;

#ifdef __APPLE__
	// FIXME: check if _inside_ the .app folder
#endif

	FilenameStripBase(path);
	return fs::u8path(path.get());
}

//
// Helper to escape path for writing. It adds quotes if needed, and any internal quotes are doubled
// (like in MS-DOS)
//
SString escape(const fs::path &path)
{
	std::string str = path.generic_u8string();
	return SString(str).spaceEscape();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
