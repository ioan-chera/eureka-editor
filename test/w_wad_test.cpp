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
#include "WadData.h"
#include "testUtils/TempDirContext.hpp"
#ifdef None	// fix pollution
#undef None
#endif
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
static void readFromPath(const fs::path &path, std::vector<uint8_t> &data)
{
	FILE *f = fopen(path.u8string().c_str(), "rb");
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
	wad = attempt(Wad_file::Open(getChildPath("inexistent.wad"), WadOpenMode::read));
	ASSERT_FALSE(wad);

	// Opening for writing or appending should be fine.
	wad = attempt(Wad_file::Open(getChildPath("newwad.wad"), WadOpenMode::write));
	ASSERT_TRUE(wad);
	// File won't get created per se, so don't add it to the delete list.
	// Zero lumps for the new file.
	ASSERT_EQ(wad->NumLumps(), 0);

	wad = attempt(Wad_file::Open(getChildPath("appendwad.wad"), WadOpenMode::append));
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
	fs::path path = getChildPath("newwad.wad");
	wad = attempt(Wad_file::Open(path, WadOpenMode::write));
	ASSERT_EQ(wad->PathName(), path);
	ASSERT_FALSE(wad->IsReadOnly());
	ASSERT_FALSE(wad->IsIWAD());
	ASSERT_TRUE(wad);

	// Write it lumpless
	attempt(wad->writeToDisk());
	mDeleteList.push(path);

	// Right now add some data to it. Since writeToDisk won't be called yet,
	// the lump should only be there in memory.
	Lump_c *lump = &wad->AddLump("HelloWorld");
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
	read = attempt(Wad_file::Open(path, WadOpenMode::read));
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 0);

	// Now write it with the new lump.
	attempt(wad->writeToDisk());
	readFromPath(path, data);
	ASSERT_EQ(data.size(), 12 + 16);	// header and one dir entry
	assertVecString(data, "PWAD\x01\0\0\0\x0c\0\0\0"
					"\x0c\0\0\0\0\0\0\0HELLOWOR");
	// And the wad
	read = attempt(Wad_file::Open(path, WadOpenMode::read));
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 1);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLOWOR");
	ASSERT_EQ(read->GetLump(0)->Length(), 0);

	// Now add some data to that lump
	lump->Printf("Hello, world!");
	attempt(wad->writeToDisk());	// and update the disk entry

	// Check again
	readFromPath(path, data);
	ASSERT_EQ(data.size(), 12 + 16 + 13);	// header, dir entry and content
	// Info table ofs: 12 + 13 = 25 (0x19)
	assertVecString(data, "PWAD\x01\0\0\0\x19\0\0\0"
					"Hello, world!"
					"\x0c\0\0\0\x0d\0\0\0HELLOWOR");

	// And the wad
	read = attempt(Wad_file::Open(path, WadOpenMode::read));
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 1);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLOWOR");
	ASSERT_EQ(read->GetLump(0)->Length(), 13);
	wadReadData = read->GetLump(0)->getData();
	assertVecString(wadReadData, "Hello, world!");

	// Now add yet another lump at the end
	lump = &wad->AddLump("LUMPLUMP");
	ASSERT_NE(lump, nullptr);
	lump->Printf("Ah");

	// Also add a lump in the middle. Test the insertion point feature
	wad->InsertPoint(1);
	lump = &wad->AddLump("MIDLump");
	ASSERT_NE(lump, nullptr);
	lump->Printf("Doom");

	ASSERT_EQ(wad->TotalSize(), 12 + 13 + 4 + 2 + 48);
	attempt(wad->writeToDisk());

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
	read = attempt(Wad_file::Open(path, WadOpenMode::read));
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 3);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLOWOR");
	ASSERT_EQ(read->GetLump(0)->Length(), 13);
	wadReadData = read->GetLump(0)->getData();
	assertVecString(wadReadData, "Hello, world!");
	ASSERT_EQ(read->GetLump(1)->Name(), "MIDLUMP");
	ASSERT_EQ(read->GetLump(1)->Length(), 4);
	wadReadData = read->GetLump(1)->getData();
	assertVecString(wadReadData, "Doom");
	ASSERT_EQ(read->GetLump(2)->Name(), "LUMPLUMP");
	ASSERT_EQ(read->GetLump(2)->Length(), 2);
	wadReadData = read->GetLump(2)->getData();
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
	ASSERT_EQ(read->LevelFindFirst(), -1);

	// Test renaming
	read->RenameLump(read->FindLumpNum("LumpLump"), "BaronOfHell");
	ASSERT_EQ(read->GetLump(2)->Name(), "BARONOFH");

	// Test name8
	int64_t nm8 = read->GetLump(2)->getName8();
	char buf[9] = {};
	memcpy(buf, &nm8, 8);
	ASSERT_STREQ(buf, "BARONOFH");
}

TEST_F(WadFileTest, Validate)
{
	// Inexistent path should fail validation
	ASSERT_FALSE(Wad_file::Validate(getChildPath("None.wad")));

	fs::path path = getChildPath("wad.wad");

	FILE *f = fopen(path.u8string().c_str(), "wb");
	ASSERT_NE(f, nullptr);
	mDeleteList.push(path);
	fprintf(f, "abcdefghijklmnopq");
	ASSERT_EQ(fclose(f), 0);

	// Will not work: no WAD signature
	ASSERT_FALSE(Wad_file::Validate(path));

	f = fopen(path.u8string().c_str(), "r+b");
	ASSERT_NE(f, nullptr);
	ASSERT_EQ(fseek(f, 1, SEEK_SET), 0);
	fprintf(f, "WAD");	// put WAD on the second char
	ASSERT_EQ(fclose(f), 0);

	// Will be validated due to the WAD signature
	ASSERT_TRUE(Wad_file::Validate(path));

	// Now have it
	f = fopen(path.u8string().c_str(), "wb");
	ASSERT_NE(f, nullptr);
	fprintf(f, "%s", "PWAD\0\0\01");
	ASSERT_EQ(fclose(f), 0);

	// Will not be validated due to length
	ASSERT_FALSE(Wad_file::Validate(path));
}

//
// Test FindLumpInNamespace
//
TEST_F(WadFileTest, FindLumpInNamespace)
{
	auto wad = attempt(Wad_file::Open("dummy.wad", WadOpenMode::write));
	ASSERT_TRUE(wad);

	// Print data for each new lump so it gets namespaced correctly
	wad->AddLump("F_START");	// 0
	wad->AddLump("LUMP");
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	wad->AddLump("F_END");
	wad->AddLump("FF_START");
	wad->AddLump("LUMP2");		// 4
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	wad->AddLump("F_END");
	wad->AddLump("FF_START");
	wad->AddLump("LUMP3");
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	wad->AddLump("FF_END");	// 8
	wad->AddLump("S_START");
	wad->AddLump("LUMP");
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	wad->AddLump("S_END");
	wad->AddLump("SS_START");	// 12
	wad->AddLump("LUMP2");
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	wad->AddLump("S_END");
	wad->AddLump("SS_START");
	wad->AddLump("LUMP3");		// 16
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	wad->AddLump("SS_END");
	wad->AddLump("TX_START");
	wad->AddLump("LUMP");
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	wad->AddLump("TX_END");	// 20
	wad->AddLump("LUMP");
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");

	ASSERT_EQ(wad->FindLumpInNamespace("LUMP", WadNamespace::Global),
			  wad->GetLump(21));
	ASSERT_EQ(wad->FindLumpInNamespace("LUMP", WadNamespace::Flats),
			  wad->GetLump(1));
	ASSERT_EQ(wad->FindLumpInNamespace("LUMP2", WadNamespace::Flats),
			  wad->GetLump(4));
	ASSERT_EQ(wad->FindLumpInNamespace("LUMP3", WadNamespace::Flats),
			  wad->GetLump(7));
	ASSERT_EQ(wad->FindLumpInNamespace("LUMP", WadNamespace::Sprites),
			  wad->GetLump(10));
	ASSERT_EQ(wad->FindLumpInNamespace("LUMP2", WadNamespace::Sprites),
			  wad->GetLump(13));
	ASSERT_EQ(wad->FindLumpInNamespace("LUMP3", WadNamespace::Sprites),
			  wad->GetLump(16));
	ASSERT_EQ(wad->FindLumpInNamespace("LUMP", WadNamespace::TextureLumps),
			  wad->GetLump(19));
}

//
// Query levels
//
TEST_F(WadFileTest, LevelQuery)
{
	fs::path path = getChildPath("wad.wad");
	auto wad = attempt(Wad_file::Open(path, WadOpenMode::write));
	ASSERT_TRUE(wad);

	// Classic Doom map. Give it a nonstandard name
	// NOTE: when adding, we must explicitly state if the lump is a level
	wad->AddLevel("MAP45");	// 0 (deliberately after UDMF)
	wad->AddLump("THINGS");
	wad->AddLump("LINEDEFS");
	wad->AddLump("SIDEDEFS");
	wad->AddLump("VERTEXES");	// 4
	wad->AddLump("SEGS");
	wad->AddLump("SSECTORS");
	wad->AddLump("NODES");
	wad->AddLump("SECTORS");	// 8
	wad->AddLump("REJECT");
	wad->AddLump("BLOCKMAP");
	wad->AddLump("BEHAVIOR");
	wad->AddLump("GL_JACK");	// 12, random name
	// UDMF map
	ASSERT_TRUE(wad->AddLevel("E2M4"));
	wad->AddLump("TEXTMAP");
	wad->AddLump("ZNODES");
	wad->AddLump("BLOCKMAP");	// 16, give these rarely-used lumps
	wad->AddLump("REJECT");
	wad->AddLump("ENDMAP");

	// Test the sorting...
	ASSERT_EQ(wad->LevelHeader(0), 0);
	ASSERT_EQ(wad->LevelHeader(1), 13);
	wad->SortLevels();
	ASSERT_EQ(wad->LevelHeader(0), 13);
	ASSERT_EQ(wad->LevelHeader(1), 0);

	attempt(wad->writeToDisk());
	mDeleteList.push(path);

	// For this test, let's assume UDMF is active
	global::udmf_testing = true;

	// Also check how a wad read from file behaves
	auto read = attempt(Wad_file::Open(path, WadOpenMode::read));
	ASSERT_TRUE(read);

	// Test both wads. The second one shall autodetect
	for(const std::shared_ptr<Wad_file> &w : { wad, read })
	{
		if(w.get() == wad.get())
			puts("Testing WRITTEN file");
		else
			puts("Testing READ file");
		// Check the simple queries
		ASSERT_EQ(w->LevelCount(), 2);
		// NOTE: check that they get sorted
		ASSERT_EQ(w->LevelHeader(0), 13);
		ASSERT_EQ(w->LevelLastLump(0), 18);
		ASSERT_EQ(w->LevelHeader(1), 0);
		ASSERT_EQ(w->LevelLastLump(1), 12);

		ASSERT_EQ(w->LevelFind("E2M4"), 0);
		ASSERT_EQ(w->LevelFind("MAP45"), 1);
		ASSERT_EQ(w->LevelFind("OTHER"), -1);

		ASSERT_EQ(w->LevelFindByNumber(24), 0);
		ASSERT_EQ(w->LevelFindByNumber(45), 1);
		ASSERT_EQ(w->LevelFindByNumber(23), -1);

		// the other kind is for no-level wads
		ASSERT_EQ(w->LevelFindFirst(), 0);

		ASSERT_EQ(w->LevelLookupLump(0, "BLOCKMAP"), 16);
		ASSERT_EQ(w->LevelLookupLump(0, "ENDMAP"), 18);
		ASSERT_EQ(w->LevelLookupLump(0, "ZNODES"), 15);
		ASSERT_EQ(w->LevelLookupLump(1, "BLOCKMAP"), 10);
		ASSERT_EQ(w->LevelLookupLump(1, "ZNODES"), -1);

		ASSERT_EQ(w->LevelFormat(0), MapFormat::udmf);
		ASSERT_EQ(w->LevelFormat(1), MapFormat::hexen);
	}

	// Now remove behaviour and check the new format and level indices
	wad->RemoveLumps(11, 2);	// Behavior and GL_jack
	ASSERT_EQ(wad->LevelFormat(0), MapFormat::udmf);
	ASSERT_EQ(wad->LevelFormat(1), MapFormat::doom);	// no behavior: doom
	ASSERT_EQ(wad->LevelHeader(0), 11);
	ASSERT_EQ(wad->LevelLastLump(0), 16);
	ASSERT_EQ(wad->LevelHeader(1), 0);
	ASSERT_EQ(wad->LevelLastLump(1), 10);

	// Now remove the second level (first from directory)
	wad->RemoveLevel(1);
	ASSERT_EQ(wad->LevelCount(), 1);
	ASSERT_EQ(wad->LevelFormat(0), MapFormat::udmf);
	ASSERT_EQ(wad->LevelHeader(0), 0);
	ASSERT_EQ(wad->LevelLastLump(0), 5);

	// Since we already removed stuff from the wad, we should now test the read
	read->RemoveGLNodes(0);	// also test levels lacking it
	read->RemoveGLNodes(1);
	ASSERT_EQ(read->LevelHeader(0), 12);
	ASSERT_EQ(read->LevelLastLump(0), 17);
	ASSERT_EQ(read->LevelHeader(1), 0);
	ASSERT_EQ(read->LevelLastLump(1), 11);
	read->RemoveZNodes(0);
	read->RemoveZNodes(1);
	ASSERT_EQ(read->LevelHeader(0), 12);
	ASSERT_EQ(read->LevelLastLump(0), 16);
	ASSERT_EQ(read->LevelHeader(1), 0);
	ASSERT_EQ(read->LevelLastLump(1), 11);
}

//
// Tests that the backup will write exactly like writeToDisk.
//
TEST_F(WadFileTest, Backup)
{
	fs::path path = getChildPath("wad.wad");
	fs::path path2 = getChildPath("wad2.wad");
	auto wad = attempt(Wad_file::Open(path, WadOpenMode::write));
	ASSERT_TRUE(wad);

	wad->AddLump("LUMP1");
	wad->GetLump(wad->NumLumps() - 1)->Printf("Hello, world!");
	wad->AddLump("LUMP2");
	wad->GetLump(wad->NumLumps() - 1)->Printf("Goodbye!");

	ASSERT_TRUE(wad->Backup(path2));
	mDeleteList.push(path2);
	attempt(wad->writeToDisk());
	mDeleteList.push(path);

	// Test it now
	std::vector<uint8_t> data, data2;
	readFromPath(path, data);
	readFromPath(path2, data2);
	ASSERT_FALSE(data.empty());
	ASSERT_EQ(data, data2);
}

TEST_F(WadFileTest, LumpIO)
{
	auto wad = attempt(Wad_file::Open("dummy.wad", WadOpenMode::write));
	Lump_c &lump = wad->AddLump("LUMP");

	lump.Printf("Hello, world!");
	LumpInputStream stream(lump);
	char data[20] = {};
	ASSERT_TRUE(stream.read(data, 12));
	ASSERT_STREQ(data, "Hello, world");
	// Won't accept post-EOF
	ASSERT_FALSE(stream.read(data, 2));
	// If we got false, we're now at EOF and must rewind
	ASSERT_FALSE(stream.read(data, 1));

	LumpInputStream stream2(lump);
	memset(data, 0, sizeof(data));
	ASSERT_TRUE(stream2.read(data, 5));
	ASSERT_TRUE(stream2.read(data + 5, 7));
	ASSERT_STREQ(data, "Hello, world");

	// Now test GetLine
	Lump_c &lump2 = wad->AddLump("LUMP2");
	lump2.Printf("Hello\nworld\nand you!");
	LumpInputStream stream3(lump2);
	SString line;
	ASSERT_TRUE(stream3.readLine(line));
	ASSERT_EQ(line, "Hello\n");
	ASSERT_TRUE(stream3.readLine(line));
	ASSERT_EQ(line, "world\n");
	ASSERT_TRUE(stream3.readLine(line));
	ASSERT_EQ(line, "and you!");
	ASSERT_FALSE(stream3.readLine(line));
}

TEST_F(WadFileTest, LumpFromFile)
{
	fs::path path = getChildPath("wad.wad");
	auto wad = attempt(Wad_file::Open(path, WadOpenMode::write));
	ASSERT_TRUE(wad);
	attempt(wad->writeToDisk());
	mDeleteList.push(path);

	Lump_c &lump = wad->AddLump("Test");

	FILE *f = fopen(path.u8string().c_str(), "rb");
	ASSERT_TRUE(f);
	ASSERT_EQ(lump.writeData(f, 32), 12);
	ASSERT_EQ(fclose(f), 0);

	ASSERT_EQ(lump.Length(), 12);
	// Test getData
	ASSERT_FALSE(memcmp(lump.getData().data(), "PWAD\0\0\0\0\x0c\0\0\0", 12));
}

TEST_F(WadFileTest, FindFirstSpriteLump)
{
	auto wad = attempt(Wad_file::Open("dummy.wad", WadOpenMode::write));
	ASSERT_TRUE(wad);
	wad->AddLump("POSSA1");
	wad->AddLump("S_START");
	Lump_c &possa1 = wad->AddLump("POSSA1");
	possa1.Printf("a");	// need to have content to be considered
	Lump_c &possa2d3 = wad->AddLump("POSSA2D3");
	possa2d3.Printf("a");
	Lump_c &trooc1 = wad->AddLump("TROOC1");
	trooc1.Printf("a");
	Lump_c &possa3d2 = wad->AddLump("POSSA3D2");
	possa3d2.Printf("a");
	Lump_c &troob1 = wad->AddLump("TROOB1");
	troob1.Printf("a");
	Lump_c &trood1 = wad->AddLump("TROOD1");
	trood1.Printf("a");
	wad->AddLump("S_END");

	ASSERT_EQ(wad->findFirstSpriteLump("POSS"), &possa1);
	ASSERT_EQ(wad->findFirstSpriteLump("POSSA"), &possa1);
	ASSERT_EQ(wad->findFirstSpriteLump("TROOC"), &trooc1);
	ASSERT_EQ(wad->findFirstSpriteLump("POSSD"), &possa2d3);
	ASSERT_EQ(wad->findFirstSpriteLump("POSSD2"), &possa3d2);
	ASSERT_EQ(wad->findFirstSpriteLump("POSSA3"), &possa3d2);
	ASSERT_EQ(wad->findFirstSpriteLump("TROO"), &troob1);
	ASSERT_EQ(wad->findFirstSpriteLump("POSSD4"), nullptr);
}
