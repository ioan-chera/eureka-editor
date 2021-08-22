//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2021 Ioan Chera
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

#include "w_wad.h"
#include "testUtils/TempDirContext.hpp"
#include "gtest/gtest.h"

class WadFileTest : public TempDirContext
{
};

//
// Reads data from path.
//
// NOTE: Google Test doesn't allow its assertion macros to be called in non-void
// functions.
//
static void readFromPath(const SString &path, std::vector<uint8_t> &data)
{
	FILE *f = fopen(path.c_str(), "rb");
	ASSERT_NE(f, nullptr);
	uint8_t buffer[4096];
	data.clear();
	while(!feof(f))
	{
		size_t n = fread(buffer, 1, 4096, f);
		ASSERT_FALSE(ferror(f));
		if(n > 0)
			data.insert(data.end(), buffer, buffer + n);
	}
	ASSERT_EQ(fclose(f), 0);
}

//
// Convenience test of vector vs string
//
static void assertVecString(const std::vector<uint8_t> &data, const char *text)
{
	// WARNING: cannot assert the strlen, because the string can also have nul
	ASSERT_FALSE(memcmp(data.data(), text, data.size()));
}

TEST_F(WadFileTest, Open)
{
	std::shared_ptr<Wad_file> wad;

	// Opening a missing file for reading should fail
	wad = Wad_file::Open(getChildPath("inexistent.wad"), WadOpenMode::read);
	ASSERT_FALSE(wad);

	// Opening for writing or appending should be fine.
	wad = Wad_file::Open(getChildPath("newwad.wad"), WadOpenMode::write);
	ASSERT_TRUE(wad);
	// File won't get created per se, so don't add it to the delete list.
	// Zero lumps for the new file.
	ASSERT_EQ(wad->NumLumps(), 0);

	wad = Wad_file::Open(getChildPath("appendwad.wad"), WadOpenMode::append);
	ASSERT_TRUE(wad);
	// Same as with writing, no file created unless "writeToDisk". In addition,
	// check that it's empty, just like the writing mode.
	ASSERT_EQ(wad->NumLumps(), 0);
}

//
// Tests that whatever we write is formatted correctly, and that reading it will
// bring back the correct data.
//
TEST_F(WadFileTest, WriteRead)
{
	std::shared_ptr<Wad_file> wad;
	std::shared_ptr<Wad_file> read;

	// Prepare the test path
	SString path = getChildPath("newwad.wad");
	wad = Wad_file::Open(path, WadOpenMode::write);
	ASSERT_EQ(wad->PathName(), path);
	ASSERT_FALSE(wad->IsReadOnly());
	ASSERT_FALSE(wad->IsIWAD());
	ASSERT_TRUE(wad);

	// Write it lumpless
	wad->writeToDisk();
	mDeleteList.push(path);

	// Right now add some data to it. Since writeToDisk won't be called yet,
	// the lump should only be there in memory.
	Lump_c *lump = wad->AddLump("HelloWorld");
	ASSERT_NE(lump, nullptr);
	// Check that the name gets upper-cased and truncated.
	ASSERT_EQ(lump->Name(), "HELLOWOR");

	// Check that what we wrote was _before_ adding the lump.

	// Check the lump-less wad content
	std::vector<uint8_t> data;
	std::vector<uint8_t> wadReadData;	// this is when loading a Wad_file
	readFromPath(path, data);
	ASSERT_EQ(data.size(), 12);
	assertVecString(data, "PWAD\0\0\0\0\x0c\0\0\0");

	// The read wad
	read = Wad_file::Open(path, WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 0);

	// Now write it with the new lump.
	wad->writeToDisk();
	readFromPath(path, data);
	ASSERT_EQ(data.size(), 12 + 16);	// header and one dir entry
	assertVecString(data, "PWAD\x01\0\0\0\x0c\0\0\0"
					"\x0c\0\0\0\0\0\0\0HELLOWOR");
	// And the wad
	read = Wad_file::Open(path, WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 1);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLOWOR");
	ASSERT_EQ(read->GetLump(0)->Length(), 0);

	// Now add some data to that lump
	lump->Printf("Hello, world!");
	wad->writeToDisk();	// and update the disk entry

	// Check again
	readFromPath(path, data);
	ASSERT_EQ(data.size(), 12 + 16 + 13);	// header, dir entry and content
	// Info table ofs: 12 + 13 = 25 (0x19)
	assertVecString(data, "PWAD\x01\0\0\0\x19\0\0\0"
					"Hello, world!"
					"\x0c\0\0\0\x0d\0\0\0HELLOWOR");

	// And the wad
	read = Wad_file::Open(path, WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 1);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLOWOR");
	ASSERT_EQ(read->GetLump(0)->Length(), 13);
	ASSERT_EQ(W_LoadLumpData(read->GetLump(0), wadReadData), 13);
	assertVecString(wadReadData, "Hello, world!");

	// Now add yet another lump at the end
	lump = wad->AddLump("LUMPLUMP");
	ASSERT_NE(lump, nullptr);
	lump->Printf("Ah");

	// Also add a lump in the middle. Test the insertion point feature
	wad->InsertPoint(1);
	lump = wad->AddLump("MIDLump");
	ASSERT_NE(lump, nullptr);
	lump->Printf("Doom");

	ASSERT_EQ(wad->TotalSize(), 12 + 13 + 4 + 2 + 48);
	wad->writeToDisk();

	// Check
	readFromPath(path, data);
	ASSERT_EQ(data.size(), 12 + 13 + 4 + 2 + 48);
	// Info table ofs: 12 + 13 + 4 + 2 = 25 + 6 = 31 = 0x1f
	assertVecString(data, "PWAD\x03\0\0\0\x1f\0\0\0"
					"Hello, world!"
					   "Doom"
					   "Ah"
					   "\x0c\0\0\0\x0d\0\0\0HELLOWOR"
					   "\x19\0\0\0\x04\0\0\0MIDLUMP\0"
					"\x1d\0\0\0\x02\0\0\0LUMPLUMP");

	// And check the wad
	read = Wad_file::Open(path, WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 3);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLOWOR");
	ASSERT_EQ(read->GetLump(0)->Length(), 13);
	ASSERT_EQ(W_LoadLumpData(read->GetLump(0), wadReadData), 13);
	assertVecString(wadReadData, "Hello, world!");
	ASSERT_EQ(read->GetLump(1)->Name(), "MIDLUMP");
	ASSERT_EQ(read->GetLump(1)->Length(), 4);
	ASSERT_EQ(W_LoadLumpData(read->GetLump(1), wadReadData), 4);
	assertVecString(wadReadData, "Doom");
	ASSERT_EQ(read->GetLump(2)->Name(), "LUMPLUMP");
	ASSERT_EQ(read->GetLump(2)->Length(), 2);
	ASSERT_EQ(W_LoadLumpData(read->GetLump(2), wadReadData), 2);
	ASSERT_EQ(read->TotalSize(), 12 + 13 + 4 + 2 + 48);
	assertVecString(wadReadData, "Ah");

	// Lump lookup tests
	ASSERT_EQ(read->FindLump("MIDLuMP"), read->GetLump(1));
	ASSERT_EQ(read->FindLump("MIXLUMP"), nullptr);
	ASSERT_EQ(read->FindLumpNum("LUMPLump"), 2);
	ASSERT_EQ(read->FindLumpNum("LUMPLuma"), -1);
	ASSERT_EQ(read->FindLumpInNamespace("HelloWor", WadNamespace::Global),
			  read->GetLump(0));
	ASSERT_EQ(read->FindLumpInNamespace("HelloWor", WadNamespace::Flats),
			  nullptr);
	ASSERT_EQ(read->LevelCount(), 0);
}

TEST_F(WadFileTest, Validate)
{
	// Inexistent path should fail validation
	ASSERT_FALSE(Wad_file::Validate(getChildPath("None.wad")));

	SString path = getChildPath("wad.wad");

	FILE *f = fopen(path.c_str(), "wb");
	ASSERT_NE(f, nullptr);
	mDeleteList.push(path);
	fprintf(f, "abcdefghijklmnopq");
	ASSERT_EQ(fclose(f), 0);

	// Will not work: no WAD signature
	ASSERT_FALSE(Wad_file::Validate(path));

	f = fopen(path.c_str(), "r+b");
	ASSERT_NE(f, nullptr);
	ASSERT_EQ(fseek(f, 1, SEEK_SET), 0);
	fprintf(f, "WAD");	// put WAD on the second char
	ASSERT_EQ(fclose(f), 0);

	// Will be validated due to the WAD signature
	ASSERT_TRUE(Wad_file::Validate(path));

	// Now have it
	f = fopen(path.c_str(), "wb");
	ASSERT_NE(f, nullptr);
	fprintf(f, "%s", "PWAD\0\0\01");
	ASSERT_EQ(fclose(f), 0);

	// Will not be validated due to length
	ASSERT_FALSE(Wad_file::Validate(path));
}
