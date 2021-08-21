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

TEST_F(WadFileTest, Open)
{
	std::shared_ptr<Wad_file> wad;

	wad = Wad_file::Open(getChildPath("inexistent.wad"), WadOpenMode::read);
	ASSERT_FALSE(wad);

	wad = Wad_file::Open(getChildPath("newwad.wad"), WadOpenMode::write);
	ASSERT_TRUE(wad);
	// File won't get created per se

	wad = Wad_file::Open(getChildPath("appendwad.wad"), WadOpenMode::append);
	ASSERT_TRUE(wad);	// File needs to exist? Not necessarily
	// File won't get created per se
}

TEST_F(WadFileTest, WriteIntegrity)
{
	std::shared_ptr<Wad_file> wad;

	// Prepare something
	SString path = getChildPath("newwad.wad");
	wad = Wad_file::Open(path, WadOpenMode::write);
	ASSERT_TRUE(wad);

	// Write it lumpless
	wad->writeToDisk();
	mDeleteList.push(path);

	// Now add some data to it
	Lump_c *lump = wad->AddLump("Hello");
	ASSERT_NE(lump, nullptr);
	ASSERT_EQ(lump->Name(), "HELLO");

	// Check that it doesn't get written right off the bat

	// Check its content
	FILE *f = fopen(path.c_str(), "rb");
	ASSERT_NE(f, nullptr);

	uint8_t data[16];
	size_t r = fread(data, 1, 12, f);
	ASSERT_EQ(r, 12);
	r = fread(data, 1, 1, f);
	ASSERT_EQ(r, 0);
	ASSERT_TRUE(feof(f));

	int rc = fclose(f);
	ASSERT_EQ(rc, 0);
	ASSERT_FALSE(memcmp("PWAD", data, 4));
	auto d32 = reinterpret_cast<const int32_t *>(data);
	ASSERT_EQ(d32[1], 0);
	ASSERT_EQ(d32[2], 12);

	// Now write it with the new name
	wad->writeToDisk();
	f = fopen(path.c_str(), "rb");
	ASSERT_NE(f, nullptr);

	// header
	r = fread(data, 1, 4, f);
	ASSERT_EQ(r, 4);
	ASSERT_FALSE(memcmp("PWAD", data, 4));
	int32_t i32;
	// numlumps
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 1);
	// infotableofs
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 12);
	// Blank lump follows
	// filepos
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 12);

	// Size
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 0);

	// name
	r = fread(data, 1, 8, f);
	ASSERT_EQ(r, 8);
	ASSERT_FALSE(memcmp("HELLO\0\0\0", data, 8));

	// eof
	r = fread(data, 1, 1, f);
	ASSERT_EQ(r, 0);
	rc = fclose(f);
	ASSERT_EQ(rc, 0);

	// Now add data to that lump
	lump->Printf("Hello world, %d", 3);
	wad->writeToDisk();

	// Check again
	f = fopen(path.c_str(), "rb");
	ASSERT_NE(f, nullptr);

	// header
	r = fread(data, 1, 4, f);
	ASSERT_EQ(r, 4);
	ASSERT_FALSE(memcmp("PWAD", data, 4));
	// numlumps
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 1);
	// infotableofs
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 12 + strlen("Hello world, 3"));

	// The lump
	r = fread(data, 1, strlen("Hello world, 3"), f);
	ASSERT_FALSE(memcmp(data, "Hello world, 3", strlen("Hello world, 3")));

	// Now the directory
	// filepos
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 12);

	// Size
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, strlen("Hello world, 3"));

	// name
	r = fread(data, 1, 8, f);
	ASSERT_EQ(r, 8);
	ASSERT_FALSE(memcmp("HELLO\0\0\0", data, 8));

	// eof
	r = fread(data, 1, 1, f);
	ASSERT_EQ(r, 0);
	rc = fclose(f);
	ASSERT_EQ(rc, 0);

	// Now add yet another lump
	lump = wad->AddLump("LUMPLUMP");
	ASSERT_NE(lump, nullptr);
	lump->Printf("Ah");
	wad->writeToDisk();

	// Check again
	f = fopen(path.c_str(), "rb");
	ASSERT_NE(f, nullptr);

	// header
	r = fread(data, 1, 4, f);
	ASSERT_EQ(r, 4);
	ASSERT_FALSE(memcmp("PWAD", data, 4));
	// numlumps
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 2);
	// infotableofs
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 12 + strlen("Hello world, 3Ah"));

	// The lumps
	r = fread(data, 1, strlen("Hello world, 3Ah"), f);
	ASSERT_FALSE(memcmp(data, "Hello world, 3Ah", strlen("Hello world, 3Ah")));

	// Now the directory
	// filepos
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 12);

	// Size
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, strlen("Hello world, 3"));

	// name
	r = fread(data, 1, 8, f);
	ASSERT_EQ(r, 8);
	ASSERT_FALSE(memcmp("HELLO\0\0\0", data, 8));

	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, 12 + strlen("Hello world, 3"));

	// Size
	r = fread(&i32, 4, 1, f);
	ASSERT_EQ(r, 1);
	ASSERT_EQ(i32, strlen("Ah"));

	// name
	r = fread(data, 1, 8, f);
	ASSERT_EQ(r, 8);
	ASSERT_FALSE(memcmp("LUMPLUMP", data, 8));

	// eof
	r = fread(data, 1, 1, f);
	ASSERT_EQ(r, 0);
	rc = fclose(f);
	ASSERT_EQ(rc, 0);
}

TEST_F(WadFileTest, WriteRead)
{
	// Same as above, but just reads the wad
	std::shared_ptr<Wad_file> wad;
	std::shared_ptr<Wad_file> read;

	// Prepare something
	SString path = getChildPath("newwad.wad");

	wad = Wad_file::Open(path, WadOpenMode::write);
	ASSERT_TRUE(wad);

	// Write it lumpless
	wad->writeToDisk();
	mDeleteList.push(path);

	// Now add some data to it
	Lump_c *lump = wad->AddLump("Hello");
	ASSERT_NE(lump, nullptr);
	ASSERT_EQ(lump->Name(), "HELLO");

	// Check that it's read as such
	read = Wad_file::Open(path, WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 0);

	wad->writeToDisk();
	read = Wad_file::Open(path, WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 1);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLO");

	// Now add data to that lump
	lump->Printf("Hello world, %d", 3);
	wad->writeToDisk();

	read = Wad_file::Open(path, WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 1);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLO");
	std::vector<byte> vdat;

	W_LoadLumpData(read->GetLump(0), vdat);
	ASSERT_STREQ((const char *)vdat.data(), "Hello world, 3");

	wad->AddLump("LUMPLUMP")->Printf("Ah");
	wad->writeToDisk();
	read = Wad_file::Open(path, WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 2);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLO");
	ASSERT_EQ(read->GetLump(1)->Name(), "LUMPLUMP");
	W_LoadLumpData(read->GetLump(0), vdat);
	ASSERT_STREQ((const char *)vdat.data(), "Hello world, 3");
	W_LoadLumpData(read->GetLump(1), vdat);
	ASSERT_STREQ((const char *)vdat.data(), "Ah");

	// Also check TotalSize
	ASSERT_EQ(wad->TotalSize(), 12 + strlen("Hello world, 3Ah") + 32);
	ASSERT_EQ(read->TotalSize(), 12 + strlen("Hello world, 3Ah") + 32);
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
