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

#include "im_img.h"
#include "w_wad.h"
#include "testUtils/Utility.hpp"
#include "gtest/gtest.h"

class ImageFixture : public ::testing::Test
{
protected:
	void SetUp() override;

	Palette palette;
};

void ImageFixture::SetUp()
{
	auto wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	Lump_c *lump = wad->AddLump("PALETTE");
	ASSERT_TRUE(lump);
	std::vector<uint8_t> data = makeGrayscale();
	lump->Write(data.data(), (int)data.size());
	palette.loadPalette(*lump, 2, 2);
}

TEST_F(ImageFixture, CreateLightSprite)
{
	std::unique_ptr<Img_c> image = Img_c::createLightSprite(palette);
	ASSERT_TRUE(image);
	ASSERT_EQ(image->width(), 11);
	ASSERT_EQ(image->height(), 11);

	// All images must rise in intensity, then go down
	int mode = 0;
	img_pixel_t prevpix = TRANS_PIXEL;
	for(int i = 0; i < 11; ++i)
	{
		img_pixel_t pix = image->buf()[i + 5 * 11];
		byte r, g, b;
		byte pr, pg, pb;
		switch(mode)
		{
			case 0:	// trans edge
				if(pix == TRANS_PIXEL)
					continue;
				mode = 1;
				break;
			case 3:
				ASSERT_EQ(pix, TRANS_PIXEL);
				break;
			case 1:		// start to visit
			case 2:		// start to fade
				palette.decodePixel(pix, r, g, b);
				if(i > 5)
					mode = 2;
				if(prevpix != TRANS_PIXEL)
				{
					if(mode == 1)
					{
						ASSERT_GE(r, pr);
						ASSERT_GE(g, pg);
						ASSERT_GE(b, pb);
					}
					else
					{
						if(pix != TRANS_PIXEL)
						{
							ASSERT_LE(r, pr);
							ASSERT_LE(g, pg);
							ASSERT_LE(b, pb);
						}
						else
							mode = 3;
					}
				}
				prevpix = pix;
				pr = r;
				pg = g;
				pb = b;
				break;
		}
	}


}
