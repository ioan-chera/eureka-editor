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

#include "WadData.h"
#include "m_game.h"
#include "m_loadsave.h"
#include "w_wad.h"
#include "gtest/gtest.h"

TEST(Texture, WadDataGetSpriteDetectsNonstandardRotations)
{
    ConfigData config;

    thingtype_t type = {};

    type.group = 'C';
    type.flags = 0;
    type.radius = 20;
    type.scale = 1.0;
    type.desc = "Description";
    type.sprite = "POSS";
    type.color = 0xff000000;
    config.thing_types[3004] = type;

    auto wad = Wad_file::Open("dummy.wad", WadOpenMode::write);
    ASSERT_TRUE(wad);
    wad->AddLump("S_START");
    auto &spritelump = wad->AddLump("POSSA3D7");

    static const uint8_t tga[] = {
        17, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 3, 0, 32, 8, 16, 67, 114, 101, 97, 116, 101, 100,
        32, 98, 121, 32, 80, 105, 110, 116, 97, 255, 255, 127, 255, 255, 255, 199, 255, 255, 160,
        143, 255, 255, 38, 0, 255, 192, 192, 199, 255, 238, 238, 246, 255, 254, 232, 238, 255, 247,
        146, 192, 255, 0, 0, 255, 255, 143, 143, 255, 255, 247, 199, 255, 255, 237, 127, 255, 255,
    };

    spritelump.Write(tga, sizeof(tga));
    wad->AddLump("S_END");

    WadData wadData;
    wadData.master.setGameWad(wad);

    LoadingData loading;

    auto image = wadData.getSprite(config, 3004, loading, 3);
    ASSERT_TRUE(image);
}

TEST(Texture, WadDataGetNullSprite)
{
    ConfigData config;
    thingtype_t type = {};
    type.desc = "UNKNOWN";
    config.thing_types[1234] = type;

    LoadingData loading;

    WadData wadData;
    auto image = wadData.getSprite(config, 1234, loading, 1);
    ASSERT_FALSE(image);
    // Try twice to make sure we don't crash (happened before)
    image = wadData.getSprite(config, 1234, loading, 1);
    ASSERT_FALSE(image);
}
