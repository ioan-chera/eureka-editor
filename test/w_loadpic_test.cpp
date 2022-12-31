//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022 Ioan Chera
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

#include "w_loadpic.h"
#include "w_wad.h"
#include "gtest/gtest.h"

//
// Helper fixture for DetectImageFormat
//
class DetectImageFormat : public ::testing::Test
{
protected:
	void SetUp() override;

	Lump_c *lump;
	byte header[20] = {};
private:
	std::shared_ptr<Wad_file> wad;
};

//
// Sets up the lump
//
void DetectImageFormat::SetUp()
{
	wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	lump = wad->AddLump("PIC");
	ASSERT_TRUE(lump);
}

TEST_F(DetectImageFormat, ShortFile)
{
	// Empty lump give error
	ASSERT_EQ(W_DetectImageFormat(lump), ImageFormat::unrecognized);

	// Write too little data error
	lump->Write(header, (int)(sizeof(header) - 1));
	ASSERT_EQ(W_DetectImageFormat(lump), ImageFormat::unrecognized);

	// Blank data error
	lump->clearData();
	lump->Write(header, (int)sizeof(header));
	ASSERT_EQ(W_DetectImageFormat(lump), ImageFormat::unrecognized);
}

TEST_F(DetectImageFormat, PNG)
{
	header[0] = 0x89;
	strncpy(reinterpret_cast<char *>(header + 1), "PNG\r\n", 5);
	lump->Write(header, (int)sizeof(header));
	ASSERT_EQ(W_DetectImageFormat(lump), ImageFormat::png);
}

TEST_F(DetectImageFormat, JPEG)
{
	const char *tags[] = { "JF", "Ex" };
	for(int value = 0xe0; value < 0x100; ++value)
	{
		header[0] = 0xff;
		header[1] = 0xd8;
		header[2] = 0xff;
		header[3] = (byte)value;
		for(const char *tag : tags)
		{
			strncpy(reinterpret_cast<char *>(header + 6), tag, 2);
			lump->clearData();
			lump->Write(header, (int)sizeof(header));
			ASSERT_EQ(W_DetectImageFormat(lump), ImageFormat::jpeg);
		}
	}
}

TEST_F(DetectImageFormat, GIF)
{
	const char *tags[] = { "GIF87a", "GIF88a", "GIF89a" };
	for(const char *tag : tags)
	{
		strncpy(reinterpret_cast<char *>(header), tag, 6);
		lump->clearData();
		lump->Write(header, (int)sizeof(header));
		ASSERT_EQ(W_DetectImageFormat(lump), ImageFormat::gif);
	}
}

TEST_F(DetectImageFormat, DDS)
{
	static const char tag[] = "DDS |";
	memcpy(header, tag, 5);
	lump->Write(header, (int)sizeof(header));
	ASSERT_EQ(W_DetectImageFormat(lump), ImageFormat::dds);
}

TEST_F(DetectImageFormat, TGA)
{
	// TODO: targa
}
