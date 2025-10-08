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

#include "e_basis.h"
#include "m_game.h"
#include "Sector.h"
#include "gtest/gtest.h"

TEST(Sector, HeadRoom)
{
	Sector sector;
	ASSERT_EQ(sector.HeadRoom(), 0);
	sector.floorh = 100;
	sector.ceilh = 345;
	ASSERT_EQ(sector.HeadRoom(), 245);
}

TEST(Sector, Texture)
{
	Sector sector;
	sector.floor_tex = BA_InternaliseString("TEXONE");
	sector.ceil_tex = BA_InternaliseString("TEXB");

	ASSERT_EQ(sector.FloorTex(), "TEXONE");
	ASSERT_EQ(sector.CeilTex(), "TEXB");
}

TEST(Sector, SetDefaults)
{
	// Save original global values
	auto original_floor_h = global::default_floor_h;
	auto original_ceil_h = global::default_ceil_h;
	auto original_light_level = global::default_light_level;

	ConfigData config;
	config.default_floor_tex = "DEFFLOOR";
	config.default_ceil_tex = "DEFCEIL";

	global::default_floor_h = 123;
	global::default_ceil_h = 456;
	global::default_light_level = 122;

	Sector sector;
	sector.SetDefaults(config);

	ASSERT_EQ(sector.floorh, 123);
	ASSERT_EQ(sector.ceilh, 456);
	ASSERT_EQ(sector.FloorTex(), "DEFFLOOR");
	ASSERT_EQ(sector.CeilTex(), "DEFCEIL");
	ASSERT_EQ(sector.light, 122);

	// Restore original global values
	global::default_floor_h = original_floor_h;
	global::default_ceil_h = original_ceil_h;
	global::default_light_level = original_light_level;
}
