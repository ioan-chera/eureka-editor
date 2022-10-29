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
	wad = Wad_file::Open(getChildPath("inexistent.wad").u8string(), WadOpenMode::read);
	ASSERT_FALSE(wad);

	// Opening for writing or appending should be fine.
	wad = Wad_file::Open(getChildPath("newwad.wad").u8string(), WadOpenMode::write);
	ASSERT_TRUE(wad);
	// File won't get created per se, so don't add it to the delete list.
	// Zero lumps for the new file.
	ASSERT_EQ(wad->NumLumps(), 0);

	wad = Wad_file::Open(getChildPath("appendwad.wad").u8string(), WadOpenMode::append);
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
	wad = Wad_file::Open(path.u8string(), WadOpenMode::write);
	ASSERT_EQ(wad->PathName(), path.u8string());
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
	readFromPath(path.u8string(), data);
	ASSERT_EQ(data.size(), 12);
	assertVecString(data, "PWAD\0\0\0\0\x0c\0\0\0");

	// The read wad
	read = Wad_file::Open(path.u8string(), WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 0);

	// Now write it with the new lump.
	wad->writeToDisk();
	readFromPath(path.u8string(), data);
	ASSERT_EQ(data.size(), 12 + 16);	// header and one dir entry
	assertVecString(data, "PWAD\x01\0\0\0\x0c\0\0\0"
					"\x0c\0\0\0\0\0\0\0HELLOWOR");
	// And the wad
	read = Wad_file::Open(path.u8string(), WadOpenMode::read);
	ASSERT_TRUE(read);
	ASSERT_EQ(read->NumLumps(), 1);
	ASSERT_EQ(read->GetLump(0)->Name(), "HELLOWOR");
	ASSERT_EQ(read->GetLump(0)->Length(), 0);

	// Now add some data to that lump
	lump->Printf("Hello, world!");
	wad->writeToDisk();	// and update the disk entry

	// Check again
	readFromPath(path.u8string(), data);
	ASSERT_EQ(data.size(), 12 + 16 + 13);	// header, dir entry and content
	// Info table ofs: 12 + 13 = 25 (0x19)
	assertVecString(data, "PWAD\x01\0\0\0\x19\0\0\0"
					"Hello, world!"
					"\x0c\0\0\0\x0d\0\0\0HELLOWOR");

	// And the wad
	read = Wad_file::Open(path.u8string(), WadOpenMode::read);
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
	readFromPath(path.u8string(), data);
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
	read = Wad_file::Open(path.u8string(), WadOpenMode::read);
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
	auto wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);

	// Print data for each new lump so it gets namespaced correctly
	ASSERT_TRUE(wad->AddLump("F_START"));	// 0
	ASSERT_TRUE(wad->AddLump("LUMP"));
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	ASSERT_TRUE(wad->AddLump("F_END"));
	ASSERT_TRUE(wad->AddLump("FF_START"));
	ASSERT_TRUE(wad->AddLump("LUMP2"));		// 4
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	ASSERT_TRUE(wad->AddLump("F_END"));
	ASSERT_TRUE(wad->AddLump("FF_START"));
	ASSERT_TRUE(wad->AddLump("LUMP3"));
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	ASSERT_TRUE(wad->AddLump("FF_END"));	// 8
	ASSERT_TRUE(wad->AddLump("S_START"));
	ASSERT_TRUE(wad->AddLump("LUMP"));
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	ASSERT_TRUE(wad->AddLump("S_END"));
	ASSERT_TRUE(wad->AddLump("SS_START"));	// 12
	ASSERT_TRUE(wad->AddLump("LUMP2"));
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	ASSERT_TRUE(wad->AddLump("S_END"));
	ASSERT_TRUE(wad->AddLump("SS_START"));
	ASSERT_TRUE(wad->AddLump("LUMP3"));		// 16
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	ASSERT_TRUE(wad->AddLump("SS_END"));
	ASSERT_TRUE(wad->AddLump("TX_START"));
	ASSERT_TRUE(wad->AddLump("LUMP"));
	wad->GetLump(wad->NumLumps() - 1)->Printf("data");
	ASSERT_TRUE(wad->AddLump("TX_END"));	// 20
	ASSERT_TRUE(wad->AddLump("LUMP"));
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
	auto wad = Wad_file::Open(path.u8string(), WadOpenMode::write);
	ASSERT_TRUE(wad);

	// Classic Doom map. Give it a nonstandard name
	// NOTE: when adding, we must explicitly state if the lump is a level
	ASSERT_TRUE(wad->AddLevel("MAP45"));	// 0 (deliberately after UDMF)
	ASSERT_TRUE(wad->AddLump("THINGS"));
	ASSERT_TRUE(wad->AddLump("LINEDEFS"));
	ASSERT_TRUE(wad->AddLump("SIDEDEFS"));
	ASSERT_TRUE(wad->AddLump("VERTEXES"));	// 4
	ASSERT_TRUE(wad->AddLump("SEGS"));
	ASSERT_TRUE(wad->AddLump("SSECTORS"));
	ASSERT_TRUE(wad->AddLump("NODES"));
	ASSERT_TRUE(wad->AddLump("SECTORS"));	// 8
	ASSERT_TRUE(wad->AddLump("REJECT"));
	ASSERT_TRUE(wad->AddLump("BLOCKMAP"));
	ASSERT_TRUE(wad->AddLump("BEHAVIOR"));
	ASSERT_TRUE(wad->AddLump("GL_JACK"));	// 12, random name
	// UDMF map
	ASSERT_TRUE(wad->AddLevel("E2M4"));
	ASSERT_TRUE(wad->AddLump("TEXTMAP"));
	ASSERT_TRUE(wad->AddLump("ZNODES"));
	ASSERT_TRUE(wad->AddLump("BLOCKMAP"));	// 16, give these rarely-used lumps
	ASSERT_TRUE(wad->AddLump("REJECT"));
	ASSERT_TRUE(wad->AddLump("ENDMAP"));

	// Test the sorting...
	ASSERT_EQ(wad->LevelHeader(0), 0);
	ASSERT_EQ(wad->LevelHeader(1), 13);
	wad->SortLevels();
	ASSERT_EQ(wad->LevelHeader(0), 13);
	ASSERT_EQ(wad->LevelHeader(1), 0);

	wad->writeToDisk();
	mDeleteList.push(path);

	// For this test, let's assume UDMF is active
	global::udmf_testing = true;

	// Also check how a wad read from file behaves
	auto read = Wad_file::Open(path.u8string(), WadOpenMode::read);
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
	auto wad = Wad_file::Open(path.u8string(), WadOpenMode::write);
	ASSERT_TRUE(wad);

	ASSERT_TRUE(wad->AddLump("LUMP1"));
	wad->GetLump(wad->NumLumps() - 1)->Printf("Hello, world!");
	ASSERT_TRUE(wad->AddLump("LUMP2"));
	wad->GetLump(wad->NumLumps() - 1)->Printf("Goodbye!");

	ASSERT_TRUE(wad->Backup(path2.u8string().c_str()));
	mDeleteList.push(path2);
	wad->writeToDisk();
	mDeleteList.push(path);

	// Test it now
	std::vector<uint8_t> data, data2;
	readFromPath(path.u8string(), data);
	readFromPath(path2.u8string(), data2);
	ASSERT_FALSE(data.empty());
	ASSERT_EQ(data, data2);
}

TEST_F(WadFileTest, LumpIO)
{
	auto wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
	Lump_c *lump = wad->AddLump("LUMP");
	ASSERT_TRUE(lump);

	lump->Printf("Hello, world!");
	lump->Seek(7);
	char data[20] = {};
	ASSERT_TRUE(lump->Read(data, 5));
	ASSERT_STREQ(data, "world");
	// Won't accept post-EOF
	ASSERT_FALSE(lump->Read(data, 2));
	// If we got false, we're now at EOF and must rewind
	ASSERT_FALSE(lump->Read(data, 1));

	lump->Seek();
	memset(data, 0, sizeof(data));
	ASSERT_TRUE(lump->Read(data, 5));
	ASSERT_TRUE(lump->Read(data + 5, 7));
	ASSERT_STREQ(data, "Hello, world");

	// Now test GetLine
	Lump_c *lump2 = wad->AddLump("LUMP2");
	ASSERT_TRUE(lump2);
	lump2->Printf("Hello\nworld\nand you!");
	lump2->Seek();
	SString line;
	ASSERT_TRUE(lump2->GetLine(line));
	ASSERT_EQ(line, "Hello\n");
	ASSERT_TRUE(lump2->GetLine(line));
	ASSERT_EQ(line, "world\n");
	ASSERT_TRUE(lump2->GetLine(line));
	ASSERT_EQ(line, "and you!");
	ASSERT_FALSE(lump2->GetLine(line));
}

TEST_F(WadFileTest, LumpFromFile)
{
	fs::path path = getChildPath("wad.wad");
	auto wad = Wad_file::Open(path.u8string(), WadOpenMode::write);
	ASSERT_TRUE(wad);
	wad->writeToDisk();
	mDeleteList.push(path);

	Lump_c *lump = wad->AddLump("Test");
	ASSERT_TRUE(lump);

	FILE *f = fopen(path.u8string().c_str(), "rb");
	ASSERT_TRUE(f);
	ASSERT_EQ(lump->writeData(f, 32), 12);
	ASSERT_EQ(fclose(f), 0);

	ASSERT_EQ(lump->Length(), 12);
	// Test getData
	ASSERT_FALSE(memcmp(lump->getData(), "PWAD\0\0\0\0\x0c\0\0\0", 12));
}
