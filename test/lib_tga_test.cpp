//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2023 Ioan Chera
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

#include "gtest/gtest.h"

#include "lib_tga.h"

TEST(TGA, DecodeImage)
{
    static const uint8_t image[] = {
        17, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 3, 0, 32, 8, 16, 67, 114, 101, 97, 116, 101, 100,
        32, 98, 121, 32, 80, 105, 110, 116, 97, 255, 255, 127, 255, 255, 255, 199, 255, 255, 160,
        143, 255, 255, 38, 0, 255, 192, 192, 199, 255, 238, 238, 246, 255, 254, 232, 238, 255, 247,
        146, 192, 255, 0, 0, 255, 255, 143, 143, 255, 255, 247, 199, 255, 255, 237, 127, 255, 255,
    };

    int width = 0;
    int height = 0;
    rgba_color_t *rgba = TGA_DecodeImage(image, sizeof(image), width, height);

    ASSERT_TRUE(rgba);
    ASSERT_EQ(width, 4);
    ASSERT_EQ(height, 3);

    TGA_FreeImage(rgba);
}

TEST(TGA, DecodeImageBroken)
{
    static const uint8_t image[] = {
        17, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 32, 8, 16, 67, 114, 101, 97, 116, 101, 100,
        32, 98, 121, 32, 80, 105, 110, 116, 97, 255, 255, 127, 255, 255, 255, 199, 255, 255, 160,
        143, 255, 255, 38, 0, 255, 192, 192, 199, 255, 238, 238, 246, 255, 254, 232, 238, 255, 247,
        146, 192, 255, 0, 0, 255, 255, 143, 143, 255, 255, 247, 199, 255, 255, 237, 127, 255, 255,
    };

    int width = 0;
    int height = 0;
    rgba_color_t *rgba = TGA_DecodeImage(image, sizeof(image), width, height);

    ASSERT_FALSE(rgba);
}

TEST(TGA, DecodeImageMalformed)
{
    static const uint8_t image[] = {
        23,
    };

    int width;
    int height;
    rgba_color_t *rgba = TGA_DecodeImage(image, sizeof(image), width, height);
    ASSERT_FALSE(rgba);
}