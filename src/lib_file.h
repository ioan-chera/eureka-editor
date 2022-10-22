//------------------------------------------------------------------------
//  File Utilities
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2012 Andrew Apted
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

#ifndef __LIB_FILE_H__
#define __LIB_FILE_H__

#include <stdint.h>
#include <functional>
#include <vector>

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

#ifdef WIN32
#define DIR_SEP_CH   '\\'
#define DIR_SEP_STR  "\\"
#else
#define DIR_SEP_CH   '/'
#define DIR_SEP_STR  "/"
#endif

class SString;

// filename functions
bool HasExtension(const SString &filename);
bool MatchExtension(const SString &filename, const SString &ext);
SString ReplaceExtension(const SString &filename, const SString &ext);
SString GetBaseName(const SString &path);
bool FilenameIsBare(const SString &filename);
SString FilenameReposition(const SString &filename, const SString &othername);
SString FilenameGetPath(const SString &filename);
SString GetAbsolutePath(const SString &path);

// file utilities
bool FileExists(const fs::path &filename);
bool FileCopy(const SString &src_name, const SString &dest_name);
bool FileDelete(const SString &filename);
bool FileChangeDir(const SString &dir_name);
bool FileMakeDir(const SString &dir_name);

bool FileLoad(const SString &filename, std::vector<uint8_t> &data);

// miscellaneous
SString GetExecutablePath(const char *argv0);

//------------------------------------------------------------------------

enum scan_flags_e
{
	SCAN_F_IsDir    = (1 << 0),
	SCAN_F_Hidden   = (1 << 1),
	SCAN_F_ReadOnly = (1 << 2),
};

enum scan_error_e
{
	SCAN_ERROR = -1,  // general catch-all

	SCAN_ERR_NoExist  = -2,  // could not find given path
	SCAN_ERR_NotDir   = -3,  // path was not a directory
};

typedef void (* directory_iter_f)(const SString &name, int flags, void *priv_dat);

int ScanDirectory(const SString &path, directory_iter_f func, void *priv_dat);
int ScanDirectory(const SString &path, const std::function<void(const SString &, int)> &func);
// scan the directory with the given path and call the given
// function (passing the private data pointer to it) for each
// entry in the directory.  Returns the total number of entries,
// or a negative value on error (SCAN_ERR_xx value).

#endif /* __LIB_FILE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
