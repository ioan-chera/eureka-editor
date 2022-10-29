//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
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

#include "lib_file.h"
#include "m_strings.h"
#include "testUtils/TempDirContext.hpp"
#include "gtest/gtest.h"
#include "FL/filename.H"
#include <fstream>

class LibFileTempDir : public TempDirContext
{
};

TEST(LibFile, HasExtension)
{
	ASSERT_FALSE(HasExtension("man/doc."));
	ASSERT_TRUE(HasExtension("man/doom.wad"));
	ASSERT_FALSE(HasExtension("man.wad/doom"));
	ASSERT_TRUE(HasExtension("man.wad/doom.wad"));
	ASSERT_TRUE(HasExtension("man.wad/doom..wad"));
	ASSERT_FALSE(HasExtension(".okay"));
	ASSERT_FALSE(HasExtension("man/.okay"));
	ASSERT_TRUE(HasExtension("man/..okay"));
	ASSERT_FALSE(HasExtension("man/.okay."));
	ASSERT_TRUE(HasExtension("man/.okay.wad"));
	ASSERT_FALSE(HasExtension("/."));
	ASSERT_FALSE(HasExtension("."));
	ASSERT_FALSE(HasExtension(".."));
	ASSERT_FALSE(HasExtension("abc/.."));
	ASSERT_FALSE(HasExtension("../.."));
	ASSERT_FALSE(HasExtension(""));
}

TEST(LibFile, MatchExtension)
{
	ASSERT_TRUE(MatchExtensionNoCase("man/doc.", nullptr));
	ASSERT_TRUE(MatchExtensionNoCase("man/doc.", ""));
	ASSERT_FALSE(MatchExtensionNoCase("man/.doc.", ".doc"));
	ASSERT_TRUE(MatchExtensionNoCase("man/doc. ", ". "));
	ASSERT_TRUE(MatchExtensionNoCase("man.wad/doom", nullptr));
	ASSERT_FALSE(MatchExtensionNoCase("man.wad/doom", "doom"));
	ASSERT_TRUE(MatchExtensionNoCase("man.wad/doom.wad", ".WAD"));
	ASSERT_TRUE(MatchExtensionNoCase("man.wad/doom..wad", ".WAD"));
	ASSERT_TRUE(MatchExtensionNoCase(".okay", ""));
	ASSERT_FALSE(MatchExtensionNoCase(".okay", ".okay"));
	ASSERT_TRUE(MatchExtensionNoCase("man/.okay", ""));
	ASSERT_FALSE(MatchExtensionNoCase("man/.okay", ".okay"));
	ASSERT_TRUE(MatchExtensionNoCase("man/.okay.WAD", ".wad"));
	ASSERT_TRUE(MatchExtensionNoCase("/.", nullptr));
	ASSERT_TRUE(MatchExtensionNoCase(".", nullptr));
	ASSERT_TRUE(MatchExtensionNoCase("..", nullptr));
	ASSERT_FALSE(MatchExtensionNoCase("..", "."));
	ASSERT_TRUE(MatchExtensionNoCase("", nullptr));
}

TEST(LibFile, ReplaceExtension)
{
	ASSERT_EQ(ReplaceExtension("man/doc.", "wad"), "man/doc.wad");
	ASSERT_EQ(ReplaceExtension("man/doc.", "WAD"), "man/doc.WAD");
	ASSERT_EQ(ReplaceExtension("man/doc.", ""), "man/doc");
	ASSERT_EQ(ReplaceExtension("man/doc.", nullptr), "man/doc");
	ASSERT_EQ(ReplaceExtension("man/.doc", ""), "man/.doc");
	ASSERT_EQ(ReplaceExtension("man/.doc", nullptr), "man/.doc");
	ASSERT_EQ(ReplaceExtension("man/.doc", "wad"), "man/.doc.wad");
	ASSERT_EQ(ReplaceExtension("man.wad/doom", "waD"), "man.wad/doom.waD");
	ASSERT_EQ(ReplaceExtension("man.wad/doom.wad", ".txt"), "man.wad/doom.txt");
	ASSERT_EQ(ReplaceExtension("man.wad/doom.wad", ""), "man.wad/doom");
	ASSERT_EQ(ReplaceExtension("man.wad/doom.wad", nullptr), "man.wad/doom");
	ASSERT_EQ(ReplaceExtension("man.wad/doom..wad", nullptr), "man.wad/doom.");
	ASSERT_EQ(ReplaceExtension("man.wad/doom..wad", ""), "man.wad/doom.");
	ASSERT_EQ(ReplaceExtension("man.wad/doom..wad", "txt"), "man.wad/doom..txt");
	ASSERT_EQ(ReplaceExtension(".okay", "txt"), ".okay.txt");
	ASSERT_EQ(ReplaceExtension(".okay", ""), ".okay");
	ASSERT_EQ(ReplaceExtension(".okay", nullptr), ".okay");
	ASSERT_EQ(ReplaceExtension("/.", nullptr), "/.");
	ASSERT_EQ(ReplaceExtension("/.", "txt"), "/..txt");
	ASSERT_EQ(ReplaceExtension(".", "txt"), "..txt");
	ASSERT_EQ(ReplaceExtension("", "txt"), ".txt");
	ASSERT_EQ(ReplaceExtension(".", ""), ".");
	ASSERT_EQ(ReplaceExtension(".", nullptr), ".");
	ASSERT_EQ(ReplaceExtension("..", ""), "..");
	ASSERT_EQ(ReplaceExtension("..", nullptr), "..");
	ASSERT_EQ(ReplaceExtension("..", "txt"), "..txt");
	ASSERT_EQ(ReplaceExtension("..txt", "wad"), "..wad");
	ASSERT_EQ(ReplaceExtension("..txt", ""), ".");
	ASSERT_EQ(ReplaceExtension("..txt", nullptr), ".");
	ASSERT_EQ(ReplaceExtension("", ""), "");
	ASSERT_EQ(ReplaceExtension("", nullptr), "");
	ASSERT_EQ(ReplaceExtension("", "wad"), ".wad");
}

TEST(LibFile, FilenameGetPath)
{
    ASSERT_EQ(FilenameGetPath("path/to/file"), "path/to");
    ASSERT_EQ(FilenameGetPath("path/to" DIR_SEP_STR DIR_SEP_STR "file"), "path/to");
    ASSERT_EQ(FilenameGetPath("file"), ".");
	ASSERT_EQ(FilenameGetPath(""), ".");
    ASSERT_EQ(FilenameGetPath(DIR_SEP_STR "file"), DIR_SEP_STR);
	ASSERT_EQ(FilenameGetPath("/doom/"), "/doom");
	ASSERT_TRUE(FilenameGetPath("///doom.wad") == "/" || FilenameGetPath("///doom.wad") == "\\");
#ifdef _WIN32
    ASSERT_EQ(FilenameGetPath("C:" DIR_SEP_STR "file"), "C:" DIR_SEP_STR);
#endif
}

TEST(LibFile, GetBaseName)
{
	ASSERT_EQ(GetBaseName("path/to///fileA.wad"), "fileA.wad");
    ASSERT_EQ(GetBaseName("path/to/fileB"), "fileB");
    ASSERT_EQ(GetBaseName("/fileC.txt"), "fileC.txt");
	ASSERT_EQ(GetBaseName("/file"), "file");
	ASSERT_EQ(GetBaseName("//dir/file"), "file");
    ASSERT_EQ(GetBaseName("/file"), "file");
    ASSERT_EQ(GetBaseName("fil"), "fil");
	ASSERT_EQ(GetBaseName(""), "");
}

TEST(LibFile, FilenameIsBare)
{
	ASSERT_TRUE(FilenameIsBare(""));
	ASSERT_TRUE(FilenameIsBare("Doom"));
	ASSERT_TRUE(FilenameIsBare("DOOM"));
	ASSERT_TRUE(FilenameIsBare("doom"));
	ASSERT_FALSE(FilenameIsBare("/doom"));
	ASSERT_FALSE(FilenameIsBare("/doom.wad"));
	ASSERT_FALSE(FilenameIsBare("doom.wad"));
	ASSERT_FALSE(FilenameIsBare("C:\\doom"));
}

TEST(LibFile, GetAbsolutePath)
{
	char path[FL_PATH_MAX];
	int result = fl_filename_absolute(path, sizeof(path), "Hello");
	ASSERT_NE(result, 0);	// absolute path stays absolute

	SString stringResult = GetAbsolutePath("Hello").generic_u8string();
	ASSERT_STREQ(stringResult.c_str(), path);
}

TEST_F(LibFileTempDir, FileExists)
{
	fs::path path = getChildPath("hello");
	ASSERT_FALSE(FileExists(path));
	std::ofstream os(path);
	ASSERT_TRUE(os.is_open());
	os.close();
	mDeleteList.push(path.u8string());
	ASSERT_TRUE(FileExists(path));

	fs::remove(path);
	mDeleteList.pop();

	// Deleted, now must be back to false
	ASSERT_FALSE(FileExists(path));

	ASSERT_TRUE(FileMakeDir(path));
	mDeleteList.push(path.u8string());
	ASSERT_FALSE(FileExists(path));
}

TEST_F(LibFileTempDir, FileDelete)
{
	fs::path path = getChildPath("file");
	ASSERT_FALSE(FileDelete(path));	// can't delete what is not there

	std::ofstream os(path);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(path.u8string());
	os << "Hello";
	os.close();

	ASSERT_TRUE(FileDelete(path));
	mDeleteList.pop();

}

// NOTE: FileChangeDir modifies the process state

TEST_F(LibFileTempDir, FileMakeDir)
{
	fs::path path = getChildPath("dir");
	ASSERT_TRUE(FileMakeDir(path));
	mDeleteList.push(path.u8string());
	// Disallow overwriting
	ASSERT_FALSE(FileMakeDir(path));
	// Disallow inexistent intermediary paths
	ASSERT_FALSE(FileMakeDir(getChildPath(fs::path("dir2") / "dir3")));

}

TEST_F(LibFileTempDir, FileLoad)
{
	// Must test a sufficiently large file
	std::vector<char> randomData;
	randomData.reserve(40000);
	for(int i = 0; i < 40000; ++i)
		randomData.push_back(static_cast<char>(rand() & 0xff));

	fs::path path = getChildPath("file");
	std::ofstream os(path, std::ios::binary);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(path.u8string());
	os.write(randomData.data(), randomData.size());
	os.close();

	std::vector<uint8_t> result;
	ASSERT_TRUE(FileLoad(path, result));
	ASSERT_EQ(result.size(), 40000);
	ASSERT_EQ(memcmp(result.data(), randomData.data(), result.size()), 0);

	// Mustn't read dirs
	ASSERT_FALSE(FileLoad(mTempDir.get(), result));
	// Mustn't read inexistent files
	ASSERT_FALSE(FileLoad(getChildPath("file2"), result));
#ifndef _WIN32
	// Mustn't read special files
	ASSERT_FALSE(FileLoad("/dev/null", result));
	ASSERT_FALSE(FileLoad("/dev/urandom", result));
#endif
}

TEST_F(LibFileTempDir, ScanDirectory)
{
	// 1. Add a file, a "hidden" file, a folder and a "hidden" folder
	fs::path path = getChildPath("file");
	std::ofstream os(path);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(path.u8string());
	os << "hello";
	os.close();

	fs::path filePath = path;

	path = getChildPath(".file");
	os.open(path);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(path.u8string());
	os << "shadow";
	os.close();

	path = getChildPath("dir");
	ASSERT_TRUE(FileMakeDir(path));
	mDeleteList.push(path.u8string());
	path = getChildPath(".dir");
	ASSERT_TRUE(FileMakeDir(path));
	mDeleteList.push(path.u8string());

	fs::path dotDirPath = path;

	// Also nest another file, to check it doesn't get listed
	path = getChildPath(fs::path("dir") / "file2");
	os.open(path);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(path.u8string());
	os << "shadow2";
	os.close();

	int result = ScanDirectory(mTempDir, [](const SString &name, int flags)
		{
			if(name == "file")
				ASSERT_EQ(flags, 0);
			else if(name == ".file")
				ASSERT_TRUE(flags & SCAN_F_Hidden);
			else if(name == "dir")
				ASSERT_TRUE(flags & SCAN_F_IsDir);
			else if(name == ".dir")
				ASSERT_EQ(flags & (SCAN_F_IsDir | SCAN_F_Hidden), SCAN_F_IsDir | SCAN_F_Hidden);
			else
				ASSERT_FALSE(true);	// error if getting here
		});
	ASSERT_EQ(result, 4);	// scan no more

	// Now also try on some non-folder files
	result = ScanDirectory(filePath.u8string(), [](const SString &name, int flags)
		{
			ASSERT_FALSE(true);	// should not get here
		});
	ASSERT_EQ(result, SCAN_ERR_NotDir);

	result = ScanDirectory(getChildPath("illegal").u8string(), [](const SString &name, int flags)
		{
			ASSERT_FALSE(true);	// should not get here
		});
	ASSERT_EQ(result, SCAN_ERR_NoExist);

	// Also scan an empty dir
	result = ScanDirectory(dotDirPath.u8string(), [](const SString &name, int flags)
		{
			ASSERT_FALSE(true);	// should not get here
		});
	ASSERT_EQ(result, 0);
}
