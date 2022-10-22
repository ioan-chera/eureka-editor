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
#include "main.h"

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
		gLog.printf("File check error: %s", e.what());
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

static size_t FindBaseName(const SString &filename)
{
	// Find the base name of the file (i.e. without any path).
	// The result always points within the given string.
	//
	// Example:  "C:\Foo\Bar.wad"  ->  "Bar.wad"

#ifdef WIN32
	size_t s = filename.find_last_of("/\\");
#else
	size_t s = filename.rfind('/');
#endif
	if(s != std::string::npos)
		return s + 1;
	return 0;
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

bool FilenameIsBare(const SString &filename)
{
	return filename.find_first_of("./\\:") == std::string::npos;
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
// takes the basename in 'filename' and prepends the path from 'othername'.
// returns a newly allocated string.
//
SString FilenameReposition(const SString &cfilename, const SString &othername)
{
	SString filename = GetBaseName(cfilename.get()).u8string();

	size_t otherBaseNameLoc = FindBaseName(othername);

	if (otherBaseNameLoc == 0 || otherBaseNameLoc == SString::npos)
		return filename;

	SString result = othername;
	result.erase(otherBaseNameLoc, SString::npos);
	result += filename;
	return result;
}

//
// Get path
//
SString FilenameGetPath(const SString &filename)
{
	size_t baseNamePosition = FindBaseName(filename);

	SString directory = filename;
	directory.erase(baseNamePosition, SString::npos);

	if(directory.empty())
		return ".";

#ifdef _WIN32
	directory.trimTrailingSet("/\\");
#else
	directory.trimTrailingSet("/");
#endif
	if(directory.empty())
		return DIR_SEP_STR;

#ifdef _WIN32
	if(directory.length() == 2 && directory[1] == ':' && isalpha(directory[0]))
	{
		directory.push_back(DIR_SEP_CH);
	}
#endif

	return directory;
}

//
// Safe wrapper around fl_filename_absolute
//
SString GetAbsolutePath(const SString &path)
{
	size_t sz = 64;
	std::vector<char> stringBuffer;

	do
	{
		sz *= 2;
		stringBuffer.resize(sz);
		fl_filename_absolute(stringBuffer.data(), (int)stringBuffer.size(), path.c_str());
	} while(stringBuffer.back() == '\0' && stringBuffer[stringBuffer.size() - 2] != 0);
	// repeat until we know it's large enough

	return stringBuffer.data();
}

bool FileCopy(const SString &src_name, const SString &dest_name)
{
	char buffer[1024];

	FILE *src = fopen(src_name.c_str(), "rb");
	if (! src)
		return false;

	FILE *dest = fopen(dest_name.c_str(), "wb");
	if (! dest)
	{
		fclose(src);
		return false;
	}

	while (true)
	{
		size_t rlen = fread(buffer, 1, sizeof(buffer), src);
		if (rlen == 0)
			break;

		size_t wlen = fwrite(buffer, 1, rlen, dest);
		if (wlen != rlen)
			break;
	}

	bool was_OK = !ferror(src) && !ferror(dest);

	fclose(dest);
	fclose(src);

	return was_OK;
}


bool FileDelete(const SString &filename)
{
#ifdef WIN32
	// TODO: set wide character here
	return (::DeleteFile(filename.c_str()) != 0);

#else // UNIX or MACOSX

	return (remove(filename.c_str()) == 0);
#endif
}


bool FileChangeDir(const SString &dir_name)
{
#ifdef WIN32
	// TODO: set wide character here
	return (::SetCurrentDirectory(dir_name.c_str()) != 0);

#else // UNIX or MACOSX

	return (chdir(dir_name.c_str()) == 0);
#endif
}


bool FileMakeDir(const SString &dir_name)
{
#ifdef WIN32
	return (::CreateDirectory(dir_name.c_str(), NULL) != 0);

#else // UNIX or MACOSX

	return (mkdir(dir_name.c_str(), 0775) == 0);
#endif
}

bool FileLoad(const SString &filename, std::vector<u8_t> &data)
{
	struct stat filestat = {};
	int n = stat(filename.c_str(), &filestat);
	if(n == -1)
		return false;
	if(filestat.st_mode & S_IFDIR || !(filestat.st_mode & S_IFREG))
		return false;	// reject directories and unusual files

	FILE *fp = fopen(filename.c_str(), "rb");

	if(!fp)
		return false;

	long length;

	// determine size of file (via seeking)
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(ferror(fp) || length < 0)
	{
		fclose(fp);
		return false;
	}

	data.resize(length);

	if(length > 0 && 1 != fread(data.data(), length, 1, fp))
	{
		data.clear();
		fclose(fp);
		return false;
	}

	fclose(fp);

	return true;
}

//------------------------------------------------------------------------

//
// Scan a directory
//
int ScanDirectory(const SString &path, const std::function<void(const SString &, int)> &func)
{
	SString actualPath = path;
	if(actualPath.empty())
		actualPath = ".";
	int count = 0;

#ifdef WIN32

	DWORD attributes = GetFileAttributesA(path.c_str());
	if(attributes == INVALID_FILE_ATTRIBUTES)
		return SCAN_ERR_NoExist;
	if(!(attributes & FILE_ATTRIBUTE_DIRECTORY))
		return SCAN_ERR_NotDir;
	SString pattern;
	if(actualPath.back() == '/' || actualPath.back() == DIR_SEP_CH)
		pattern = actualPath + "*";
	else
		pattern = actualPath + DIR_SEP_STR "*";

	WIN32_FIND_DATA fdata;

	HANDLE handle = FindFirstFile(pattern.c_str(), &fdata);
	if (handle == INVALID_HANDLE_VALUE)
		return 0;  //??? (GetLastError() == ERROR_FILE_NOT_FOUND) ? 0 : SCAN_ERROR;

	do
	{
		int flags = 0;

		if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			flags |= SCAN_F_IsDir;

		if (fdata.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			flags |= SCAN_F_ReadOnly;

		if (fdata.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			flags |= SCAN_F_Hidden;

		// minor kludge for consistency with Unix
		if (fdata.cFileName[0] == '.' && isalpha(fdata.cFileName[1]))
			flags |= SCAN_F_Hidden;

		if (strcmp(fdata.cFileName, ".")  == 0 ||
				strcmp(fdata.cFileName, "..") == 0)
		{
			// skip the funky "." and ".." dirs
		}
		else
		{
			func(fdata.cFileName, flags);

			count++;
		}
	}
	while (FindNextFile(handle, &fdata) != FALSE);

	FindClose(handle);


#else // ---- UNIX ------------------------------------------------

	DIR *handle = opendir(path.c_str());
	if (handle == NULL)
		return errno == ENOTDIR ? SCAN_ERR_NotDir : SCAN_ERR_NoExist;

	for (;;)
	{
		const struct dirent *fdata = readdir(handle);
		if (fdata == NULL)
			break;

		if (strlen(fdata->d_name) == 0)
			continue;

		// skip the funky "." and ".." dirs
		if (strcmp(fdata->d_name, ".")  == 0 ||
				strcmp(fdata->d_name, "..") == 0)
			continue;

		SString full_name = path + "/" + fdata->d_name;

		struct stat finfo;

		if (stat(full_name.c_str(), &finfo) != 0)
		{
			gLog.debugPrintf(".... stat failed: %s\n", GetErrorMessage(errno).c_str());
			continue;
		}


		int flags = 0;

		if (S_ISDIR(finfo.st_mode))
			flags |= SCAN_F_IsDir;

		if ((finfo.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
			flags |= SCAN_F_ReadOnly;

		if (fdata->d_name[0] == '.' && isalpha(fdata->d_name[1]))
			flags |= SCAN_F_Hidden;

		func(fdata->d_name, flags);

		count++;
	}

	closedir(handle);
#endif

	return count;
}

int ScanDirectory(const SString &path, directory_iter_f func, void *priv_dat)
{
	return ScanDirectory(path, [func, priv_dat](const SString &name, int flags)
						 {
		func(name, flags, priv_dat);
	});
}

//------------------------------------------------------------------------

SString GetExecutablePath(const char *argv0)
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
			return retpath;
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
			return rawpath;
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
		return rawpath;
	}
#endif

	// fallback method: use argv[0]
	path = argv0;

#ifdef __APPLE__
	// FIXME: check if _inside_ the .app folder
#endif

	FilenameStripBase(path);
	return path;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
