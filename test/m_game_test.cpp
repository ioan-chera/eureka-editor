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

#include "testUtils/TempDirContext.hpp"

#include "gtest/gtest.h"

#include "Instance.h"
#include "m_game.h"

class MGameFixture : public TempDirContext
{
};

//=============================================================================
//
// TESTS
//
//=============================================================================

TEST(MGame, MClearAllDefinitions)
{
    Instance instance;

    instance.conf.line_groups['a'] = linegroup_t();
    instance.conf.line_types[0] = linetype_t();
    instance.conf.sector_types[0] = sectortype_t();

    instance.conf.thing_groups['a'] = thinggroup_t();
    instance.conf.thing_types[0] = thingtype_t();

    instance.conf.texture_groups['a'] = texturegroup_t();
    instance.conf.texture_categories["a"] = 'a';
    instance.conf.flat_categories["a"] = 'a';

    instance.conf.miscInfo.sky_color = 1;
    instance.conf.miscInfo.sky_flat = "a";
    instance.conf.miscInfo.wall_colors[0] = 1;
    instance.conf.miscInfo.floor_colors[0] = 1;
    instance.conf.miscInfo.invis_colors[0] = 1;

    instance.conf.miscInfo.missing_color = 1;
    instance.conf.miscInfo.unknown_tex = 1;
    instance.conf.miscInfo.unknown_flat = 1;
    instance.conf.miscInfo.unknown_thing = 1;

    instance.conf.miscInfo.player_r = 1;
    instance.conf.miscInfo.player_h = 1;
    instance.conf.miscInfo.view_height = 1;
    
    instance.conf.miscInfo.min_dm_starts = 1;
    instance.conf.miscInfo.max_dm_starts = 1;

    instance.conf.features.gen_types = 1;
    instance.conf.features.gen_sectors = GenSectorFamily::boom;
    instance.conf.features.tx_start = 1;
    instance.conf.features.img_png = 1;
    instance.conf.features.coop_dm_flags = 1;
    instance.conf.features.friend_flag = 1;
    instance.conf.features.pass_through = 1;
    instance.conf.features.midtex_3d = 1;
    instance.conf.features.strife_flags = 1;
    instance.conf.features.medusa_fixed = 1;
    instance.conf.features.lax_sprites = 1;
    instance.conf.features.mix_textures_flats = 1;
    instance.conf.features.neg_patch_offsets = 1;
    instance.conf.features.no_need_players = 1;
    instance.conf.features.tag_666 = Tag666Rules::doom;
    instance.conf.features.extra_floors = 1;
    instance.conf.features.slopes = 1;

    instance.conf.gen_linetypes[0].base = 1;

    instance.conf.num_gen_linetypes = 1;

    instance.conf.clearExceptDefaults();

    ASSERT_TRUE(instance.conf.line_groups.empty());
    ASSERT_TRUE(instance.conf.line_types.empty());
    ASSERT_TRUE(instance.conf.sector_types.empty());

    ASSERT_TRUE(instance.conf.thing_groups.empty());
    ASSERT_TRUE(instance.conf.thing_types.empty());

    ASSERT_TRUE(instance.conf.texture_groups.empty());
    ASSERT_TRUE(instance.conf.texture_categories.empty());
    ASSERT_TRUE(instance.conf.flat_categories.empty());

    misc_info_t emptyInfo = {};

    ASSERT_EQ(instance.conf.miscInfo.sky_color, emptyInfo.sky_color);
    ASSERT_EQ(instance.conf.miscInfo.sky_flat, emptyInfo.sky_flat);
    ASSERT_EQ(instance.conf.miscInfo.wall_colors[0], emptyInfo.wall_colors[0]);
    ASSERT_EQ(instance.conf.miscInfo.floor_colors[0], emptyInfo.floor_colors[0]);
    ASSERT_EQ(instance.conf.miscInfo.invis_colors[0], emptyInfo.invis_colors[0]);

    ASSERT_EQ(instance.conf.miscInfo.missing_color, emptyInfo.missing_color);
    ASSERT_EQ(instance.conf.miscInfo.unknown_tex, emptyInfo.unknown_tex);
    ASSERT_EQ(instance.conf.miscInfo.unknown_flat, emptyInfo.unknown_flat);
    ASSERT_EQ(instance.conf.miscInfo.unknown_thing, emptyInfo.unknown_thing);

    ASSERT_EQ(instance.conf.miscInfo.player_r, emptyInfo.player_r);
    ASSERT_EQ(instance.conf.miscInfo.player_h, emptyInfo.player_h);
    ASSERT_EQ(instance.conf.miscInfo.player_h, misc_info_t::DEFAULT_PLAYER_HEIGHT); // also check this
    ASSERT_EQ(instance.conf.miscInfo.view_height, emptyInfo.view_height);

    ASSERT_EQ(instance.conf.miscInfo.min_dm_starts, emptyInfo.min_dm_starts);
    ASSERT_EQ(instance.conf.miscInfo.max_dm_starts, emptyInfo.max_dm_starts);
    ASSERT_EQ(instance.conf.miscInfo.max_dm_starts, misc_info_t::DEFAULT_MAXIMUM_DEATHMATCH_STARTS); // also check this

    port_features_t emptyFeats = {};

    ASSERT_EQ(instance.conf.features.gen_types, emptyFeats.gen_types);
    ASSERT_EQ(instance.conf.features.gen_sectors, emptyFeats.gen_sectors);
    ASSERT_EQ(instance.conf.features.tx_start, emptyFeats.tx_start);
    ASSERT_EQ(instance.conf.features.img_png, emptyFeats.img_png);
    ASSERT_EQ(instance.conf.features.coop_dm_flags, emptyFeats.coop_dm_flags);
    ASSERT_EQ(instance.conf.features.friend_flag, emptyFeats.friend_flag);
    ASSERT_EQ(instance.conf.features.pass_through, emptyFeats.pass_through);
    ASSERT_EQ(instance.conf.features.midtex_3d, emptyFeats.midtex_3d);
    ASSERT_EQ(instance.conf.features.strife_flags, emptyFeats.strife_flags);
    ASSERT_EQ(instance.conf.features.medusa_fixed, emptyFeats.medusa_fixed);
    ASSERT_EQ(instance.conf.features.lax_sprites, emptyFeats.lax_sprites);
    ASSERT_EQ(instance.conf.features.mix_textures_flats, emptyFeats.mix_textures_flats);
    ASSERT_EQ(instance.conf.features.neg_patch_offsets, emptyFeats.neg_patch_offsets);
    ASSERT_EQ(instance.conf.features.no_need_players, emptyFeats.no_need_players);
    ASSERT_EQ(instance.conf.features.tag_666, emptyFeats.tag_666);
    ASSERT_EQ(instance.conf.features.extra_floors, emptyFeats.extra_floors);
    ASSERT_EQ(instance.conf.features.slopes, emptyFeats.slopes);

    ASSERT_EQ(instance.conf.gen_linetypes[0].base, 0);

    ASSERT_EQ(instance.conf.num_gen_linetypes, 0);
}

//
// Convenience operator
//
static bool operator == (const thingtype_t &type1, const thingtype_t &type2)
{
	return type1.group == type2.group && type1.flags == type2.flags &&
	type1.radius == type2.radius && type1.scale == type2.scale && type1.desc == type2.desc &&
	type1.sprite == type2.sprite && type1.color == type2.color && type1.args[0] == type2.args[0] &&
	type1.args[1] == type2.args[1] && type1.args[2] == type2.args[2] &&
	type1.args[3] == type2.args[3] && type1.args[4] == type2.args[4];
}

TEST(MGame, ConfigDataGetThingType)
{
	ConfigData config = {};
	thingtype_t type = {};
	type.group = 'a';
	type.flags = THINGDEF_LIT;
	type.radius = 16;
	type.scale = 1.0f;
	type.desc = "Bright spot";
	type.sprite = "BRIG";
	type.color = rgbMake(128, 0, 0);

	config.thing_types[111] = type;

	thingtype_t type2 = {};

	type2.group = 'b';
	type2.flags = 0;
	type2.radius = 16;
	type2.scale = 1.0f;
	type2.desc = "Dark spot";
	type2.sprite = "DARK";
	type2.color = rgbMake(0, 0, 128);

	config.thing_types[222] = type2;

	ASSERT_EQ(config.getThingType(111), type);
	ASSERT_EQ(config.getThingType(222), type2);
	ASSERT_EQ(config.getThingType(1).desc, "UNKNOWN TYPE");
	ASSERT_EQ(config.getThingType(0).desc, "UNKNOWN TYPE");
	ASSERT_EQ(config.getThingType(-1).desc, "UNKNOWN TYPE");
}

TEST_F(MGameFixture, MCollectKnownDefs)
{
	//
	// Helper to make dir and put it to stack
	//
	auto makeDir = [this](const fs::path &path)
	{
		ASSERT_TRUE(FileMakeDir(path));
		mDeleteList.push(path);
	};

	//
	// Helper to create empty file, complete with check
	//
	auto makeFile = [this](const fs::path &path)
	{
		std::ofstream os(path);
		ASSERT_TRUE(os.is_open());
		mDeleteList.push(path);
	};

	// We need the install and home dirs for this test
	const fs::path install_dir = getChildPath("install");
	makeDir(install_dir);

	const fs::path home_dir = getChildPath("home");
	makeDir(home_dir);

	// Now add some other folder inside both of them
	const fs::path folder = "conf";
	const fs::path home = home_dir / folder;
	const fs::path install = install_dir / folder;
	makeDir(install);
	makeDir(home);

	// Now add unrelated folders in each
	fs::path unrelated[2] = {install_dir / "unrel", home_dir / "ated"};
	for(const fs::path &unrel : unrelated)
		makeDir(unrel);

	// Now produce all sorts of files
	makeFile(install / "empty");
	makeFile(install / "hasugh.ugh");
	makeFile(install / "DOOM.ugh");
	makeFile(install / "Config.cfg");
	makeFile(install / "MooD.uGH");
	makeFile(home / "doom.UGH");
	makeFile(home / "Heretic.Ugh");
	makeFile(home / "Junk");
	makeFile(home / "extra.cfg");
	makeFile(home / ".hidden.ugh");	// skip for being hidden
	makeDir(home / "directory.ugh");	// skip for being dir
	makeFile(home / "directory.ugh" / "subfile.ugh");	// skip for being under dir
	makeDir(home / "folder");
	makeFile(home / "folder" / "subfile2.ugh");	// make sure to always skip subdir
	makeFile(home / "Extra.ugh");
	makeFile(unrelated[0] / "bandit.ugh");
	makeFile(unrelated[0] / "brigand.ugh");
	makeFile(unrelated[1] / "jackson.ugh");
	makeFile(unrelated[1] / "jordan.cfg");

	// Requirement: the last entry takes precedence and it's sorted.
	// Hidden and dirs are skipped
	auto expected = std::vector<SString>{"doom", "Extra", "hasugh", "Heretic", "MooD"};
	ASSERT_EQ(M_CollectKnownDefs({install_dir, home_dir}, folder), expected);
}
