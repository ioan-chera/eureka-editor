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
	lump = &wad->AddLump("PIC");
}

TEST_F(DetectImageFormat, ShortFile)
{
	// Empty lump give error
	ASSERT_EQ(W_DetectImageFormat(*lump), ImageFormat::unrecognized);

	// Write too little data error
	lump->Write(header, (int)(sizeof(header) - 1));
	ASSERT_EQ(W_DetectImageFormat(*lump), ImageFormat::unrecognized);

	// Blank data error
	lump->clearData();
	lump->Write(header, (int)sizeof(header));
	ASSERT_EQ(W_DetectImageFormat(*lump), ImageFormat::unrecognized);
}

TEST_F(DetectImageFormat, PNG)
{
	header[0] = 0x89;
	memcpy(header + 1, "PNG\r\n", 5);
	lump->Write(header, (int)sizeof(header));
	ASSERT_EQ(W_DetectImageFormat(*lump), ImageFormat::png);
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
			memcpy(header + 6, tag, 2);
			lump->clearData();
			lump->Write(header, (int)sizeof(header));
			ASSERT_EQ(W_DetectImageFormat(*lump), ImageFormat::jpeg);
		}
	}
}

TEST_F(DetectImageFormat, GIF)
{
	const char *tags[] = { "GIF87a", "GIF88a", "GIF89a" };
	for(const char *tag : tags)
	{
		memcpy(header, tag, 6);
		lump->clearData();
		lump->Write(header, (int)sizeof(header));
		ASSERT_EQ(W_DetectImageFormat(*lump), ImageFormat::gif);
	}
}

TEST_F(DetectImageFormat, DDS)
{
	static const char tag[] = "DDS |";
	memcpy(header, tag, 5);
	lump->Write(header, (int)sizeof(header));
	ASSERT_EQ(W_DetectImageFormat(*lump), ImageFormat::dds);
}

TEST_F(DetectImageFormat, TGA)
{
	int width = 512;
	int height = 1024;
	header[12] = (byte)(width & 0xff);
	header[13] = (byte)((width >> 8) & 0xff);
	header[14] = (byte)(height & 0xff);
	header[15] = (byte)((height >> 8) & 0xff);
	int cmap_type = 1;
	header[1] = (byte)cmap_type;
	int img_type = 10;
	header[2] = (byte)img_type;
	int depth = 24;
	header[16] = (byte)depth;
	lump->Write(header, (int)sizeof(header));
	ASSERT_EQ(W_DetectImageFormat(*lump), ImageFormat::tga);
}

TEST_F(DetectImageFormat, doom)
{
	int width = 64;
	int height = 128;
	header[0] = (byte)(width & 0xff);
	header[1] = (byte)((width >> 8) & 0xff);
	header[2] = (byte)(height & 0xff);
	header[3] = (byte)((height >> 8) & 0xff);
	int ofs_x = 32;
	int ofs_y = -32;
	header[4] = (byte)ofs_x;
	header[5] = 0;
	header[6] = (byte)ofs_y;
	lump->Write(header, (int)sizeof(header));
	std::vector<byte> padding;
	padding.resize(4 * width);
	lump->Write(padding.data(), (int)padding.size());
	ASSERT_EQ(W_DetectImageFormat(*lump), ImageFormat::doom);
}

//==================================================================================================

static auto prepareData(const std::vector<uint8_t> &data)
{
	auto wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
	EXPECT_TRUE(wad);
	auto &lump = wad->AddLump("LUMP");
	lump.Write(data.data(), (int)data.size());

	// IMPORTANT: must return pair because we must preserve the wad lifetime for the lump!
	return std::make_pair(wad, &lump);
}

static void assertImageValid(const std::optional<Img_c> &image)
{
	ASSERT_TRUE(image);
	ASSERT_FALSE(image->is_null());
	ASSERT_EQ(image->width(), 4);	// as per content of PNG data
	ASSERT_EQ(image->height(), 3);
	ASSERT_FALSE(image->has_transparent());
}

static const std::vector<uint8_t> pngData = {
	137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0, 0, 0, 4, 0, 0, 0, 3, 8, 6,
	0, 0, 0, 180, 244, 174, 198, 0, 0, 0, 4, 115, 66, 73, 84, 8, 8, 8, 8, 124, 8, 100, 136, 0,
	0, 0, 59, 73, 68, 65, 84, 8, 153, 5, 193, 49, 17, 192, 32, 16, 0, 193, 43, 50, 19, 1, 120,
	72, 9, 18, 145, 240, 22, 144, 66, 71, 234, 248, 160, 134, 161, 185, 236, 34, 104, 132, 142,
	165, 117, 122, 189, 189, 83, 114, 102, 159, 155, 47, 1, 85, 29, 106, 52, 229, 209, 31, 10,
	229, 30, 135, 131, 75, 13, 237, 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130,
};

TEST(LoadImage, PNG)
{
	// Prepare data
	auto data = prepareData(pngData);

	auto image = LoadImage_PNG(*data.second, "our image");
	assertImageValid(image);
}

TEST(LoadImage, PNGBroken)
{
	// 1 bit off
	auto brokenPNG = pngData;
	brokenPNG[brokenPNG.size() / 2] ^= 1;

	// Prepare data
	auto data = prepareData(brokenPNG);

	auto image = LoadImage_PNG(*data.second, "our image");
	ASSERT_FALSE(image);
}

static const std::vector<uint8_t> jpgData = {
	255, 216, 255, 224, 0, 16, 74, 70, 73, 70, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 255, 219, 0, 67, 0,
	5, 3, 4, 4, 4, 3, 5, 4, 4, 4, 5, 5, 5, 6, 7, 12, 8, 7, 7, 7, 7, 15, 11, 11, 9, 12, 17, 15,
	18, 18, 17, 15, 17, 17, 19, 22, 28, 23, 19, 20, 26, 21, 17, 17, 24, 33, 24, 26, 29, 29, 31,
	31, 31, 19, 23, 34, 36, 34, 30, 36, 28, 30, 31, 30, 255, 219, 0, 67, 1, 5, 5, 5, 7, 6, 7,
	14, 8, 8, 14, 30, 20, 17, 20, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 255, 192, 0, 17, 8, 0, 3, 0, 4, 3, 1, 34, 0,
	2, 17, 1, 3, 17, 1, 255, 196, 0, 31, 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181, 16, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4,
	0, 0, 1, 125, 1, 2, 3, 0, 4, 17, 5, 18, 33, 49, 65, 6, 19, 81, 97, 7, 34, 113, 20, 50, 129,
	145, 161, 8, 35, 66, 177, 193, 21, 82, 209, 240, 36, 51, 98, 114, 130, 9, 10, 22, 23, 24,
	25, 26, 37, 38, 39, 40, 41, 42, 52, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72, 73, 74,
	83, 84, 85, 86, 87, 88, 89, 90, 99, 100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118,
	119, 120, 121, 122, 131, 132, 133, 134, 135, 136, 137, 138, 146, 147, 148, 149, 150, 151,
	152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181, 182, 183,
	184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215,
	216, 217, 218, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 241, 242, 243, 244, 245,
	246, 247, 248, 249, 250, 255, 196, 0, 31, 1, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255, 196, 0, 181, 17, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4,
	4, 0, 1, 2, 119, 0, 1, 2, 3, 17, 4, 5, 33, 49, 6, 18, 65, 81, 7, 97, 113, 19, 34, 50, 129,
	8, 20, 66, 145, 161, 177, 193, 9, 35, 51, 82, 240, 21, 98, 114, 209, 10, 22, 36, 52, 225,
	37, 241, 23, 24, 25, 26, 38, 39, 40, 41, 42, 53, 54, 55, 56, 57, 58, 67, 68, 69, 70, 71, 72,
	73, 74, 83, 84, 85, 86, 87, 88, 89, 90, 99, 100, 101, 102, 103, 104, 105, 106, 115, 116,
	117, 118, 119, 120, 121, 122, 130, 131, 132, 133, 134, 135, 136, 137, 138, 146, 147, 148,
	149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180,
	181, 182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212,
	213, 214, 215, 216, 217, 218, 226, 227, 228, 229, 230, 231, 232, 233, 234, 242, 243, 244,
	245, 246, 247, 248, 249, 250, 255, 218, 0, 12, 3, 1, 0, 2, 17, 3, 17, 0, 63, 0, 79, 218, 39,
	197, 186, 255, 0, 133, 124, 119, 109, 162, 248, 122, 242, 61, 59, 78, 135, 76, 131, 203,
	183, 138, 218, 45, 171, 157, 221, 50, 166, 138, 40, 175, 233, 14, 30, 203, 48, 82, 202, 232,
	74, 84, 98, 223, 42, 251, 43, 252, 143, 147, 254, 212, 198, 217, 126, 250, 93, 62, 211, 255,
	0, 51, 255, 217
};

TEST(LoadImage, JPEG)
{
	auto data = prepareData(jpgData);
	auto image = LoadImage_JPEG(*data.second, "our jpg");
	assertImageValid(image);
}

TEST(LoadImage, JPEGBroken)
{
	auto brokenJPG = jpgData;
	brokenJPG[brokenJPG.size() - 1] ^= 1;
	auto data = prepareData(brokenJPG);
	auto image = LoadImage_JPEG(*data.second, "our jpg");
	ASSERT_FALSE(image);
}

TEST(LoadImage, TGA)
{
	std::vector<uint8_t> tgaData = {
		17, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 3, 0, 32, 8, 16, 67, 114, 101, 97, 116, 101, 100,
        32, 98, 121, 32, 80, 105, 110, 116, 97, 255, 255, 127, 255, 255, 255, 199, 255, 255, 160,
        143, 255, 255, 38, 0, 255, 192, 192, 199, 255, 238, 238, 246, 255, 254, 232, 238, 255, 247,
        146, 192, 255, 0, 0, 255, 255, 143, 143, 255, 255, 247, 199, 255, 255, 237, 127, 255, 255,
	};

	auto data = prepareData(tgaData);
	auto image = LoadImage_TGA(*data.second, "our tga");
	assertImageValid(image);
}
