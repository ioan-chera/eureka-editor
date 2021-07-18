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

#include "gtest/gtest.h"

#include "Instance.h"
#include "m_game.h"

//=============================================================================
//
// MOCKUPS
//
//=============================================================================
namespace global
{
    SString install_dir;
    SString home_dir;
}

void Clipboard_ClearLocals()
{
}

void Clipboard_NotifyBegin()
{
}

void Clipboard_NotifyChange(ObjType type, int objnum, int field)
{
}

void Clipboard_NotifyDelete(ObjType type, int objnum)
{
}

void Clipboard_NotifyEnd()
{
}

void Clipboard_NotifyInsert(const Document &doc, ObjType type, int objnum)
{
}

DocumentModule::DocumentModule(Document &doc) : inst(doc.inst), doc(doc)
{
}

void Instance::MapStuff_NotifyBegin()
{
}

void Instance::MapStuff_NotifyChange(ObjType type, int objnum, int field)
{
}

void Instance::MapStuff_NotifyDelete(ObjType type, int objnum)
{
}

void Instance::MapStuff_NotifyEnd()
{
}

void Instance::MapStuff_NotifyInsert(ObjType type, int objnum)
{
}

void Instance::ObjectBox_NotifyBegin()
{
}

void Instance::ObjectBox_NotifyChange(ObjType type, int objnum, int field)
{
}

void Instance::ObjectBox_NotifyDelete(ObjType type, int objnum)
{
}

void Instance::ObjectBox_NotifyEnd() const
{
}

void Instance::ObjectBox_NotifyInsert(ObjType type, int objnum)
{
}

void Instance::RedrawMap()
{
}

rgb_color_t ParseColor(const SString &cstr)
{
    // Use some independent example
    return (rgb_color_t)strtol(cstr.c_str(), nullptr, 16) << 8;
}

void Render3D_NotifyBegin()
{
}

void Render3D_NotifyChange(ObjType type, int objnum, int field)
{
}

void Render3D_NotifyDelete(const Document &doc, ObjType type, int objnum)
{
}

void Render3D_NotifyEnd(Instance &inst)
{
}

void Render3D_NotifyInsert(ObjType type, int objnum)
{
}


void Instance::Selection_Clear(bool no_save)
{
}

void Instance::Selection_NotifyBegin()
{
}

void Selection_NotifyChange(ObjType type, int objnum, int field)
{
    // field changes never affect the current selection
}

void Instance::Selection_NotifyDelete(ObjType type, int objnum)
{
}

void Instance::Selection_NotifyEnd()
{
}

void Instance::Selection_NotifyInsert(ObjType type, int objnum)
{
}

void Recently_used::insert(const SString &name)
{
}

void Recently_used::insert_number(int val)
{
}

void Instance::Status_Set(const char *fmt, ...) const
{
}

//=============================================================================
//
// TESTS
//
//=============================================================================

TEST(MGame, MClearAllDefinitions)
{
    Instance instance;

    instance.line_groups['a'] = linegroup_t();
    instance.line_types[0] = linetype_t();
    instance.sector_types[0] = sectortype_t();

    instance.thing_groups['a'] = thinggroup_t();
    instance.thing_types[0] = thingtype_t();

    instance.texture_groups['a'] = texturegroup_t();
    instance.texture_categories["a"] = 'a';
    instance.flat_categories["a"] = 'a';

    instance.Misc_info.sky_color = 1;
    instance.Misc_info.sky_flat = "a";
    instance.Misc_info.wall_colors[0] = 1;
    instance.Misc_info.floor_colors[0] = 1;
    instance.Misc_info.invis_colors[0] = 1;

    instance.Misc_info.missing_color = 1;
    instance.Misc_info.unknown_tex = 1;
    instance.Misc_info.unknown_flat = 1;
    instance.Misc_info.unknown_thing = 1;

    instance.Misc_info.player_r = 1;
    instance.Misc_info.player_h = 1;
    instance.Misc_info.view_height = 1;
    
    instance.Misc_info.min_dm_starts = 1;
    instance.Misc_info.max_dm_starts = 1;

    instance.Features.gen_types = 1;
    instance.Features.gen_sectors = GenSectorFamily::boom;
    instance.Features.tx_start = 1;
    instance.Features.img_png = 1;
    instance.Features.coop_dm_flags = 1;
    instance.Features.friend_flag = 1;
    instance.Features.pass_through = 1;
    instance.Features.midtex_3d = 1;
    instance.Features.strife_flags = 1;
    instance.Features.medusa_fixed = 1;
    instance.Features.lax_sprites = 1;
    instance.Features.mix_textures_flats = 1;
    instance.Features.neg_patch_offsets = 1;
    instance.Features.no_need_players = 1;
    instance.Features.tag_666 = Tag666Rules::doom;
    instance.Features.extra_floors = 1;
    instance.Features.slopes = 1;

    instance.gen_linetypes[0].base = 1;

    instance.num_gen_linetypes = 1;

    instance.M_ClearAllDefinitions();

    ASSERT_TRUE(instance.line_groups.empty());
    ASSERT_TRUE(instance.line_types.empty());
    ASSERT_TRUE(instance.sector_types.empty());

    ASSERT_TRUE(instance.thing_groups.empty());
    ASSERT_TRUE(instance.thing_types.empty());

    ASSERT_TRUE(instance.texture_groups.empty());
    ASSERT_TRUE(instance.texture_categories.empty());
    ASSERT_TRUE(instance.flat_categories.empty());

    misc_info_t emptyInfo = {};

    ASSERT_EQ(instance.Misc_info.sky_color, emptyInfo.sky_color);
    ASSERT_EQ(instance.Misc_info.sky_flat, emptyInfo.sky_flat);
    ASSERT_EQ(instance.Misc_info.wall_colors[0], emptyInfo.wall_colors[0]);
    ASSERT_EQ(instance.Misc_info.floor_colors[0], emptyInfo.floor_colors[0]);
    ASSERT_EQ(instance.Misc_info.invis_colors[0], emptyInfo.invis_colors[0]);

    ASSERT_EQ(instance.Misc_info.missing_color, emptyInfo.missing_color);
    ASSERT_EQ(instance.Misc_info.unknown_tex, emptyInfo.unknown_tex);
    ASSERT_EQ(instance.Misc_info.unknown_flat, emptyInfo.unknown_flat);
    ASSERT_EQ(instance.Misc_info.unknown_thing, emptyInfo.unknown_thing);

    ASSERT_EQ(instance.Misc_info.player_r, emptyInfo.player_r);
    ASSERT_EQ(instance.Misc_info.player_h, emptyInfo.player_h);
    ASSERT_EQ(instance.Misc_info.player_h, misc_info_t::DEFAULT_PLAYER_HEIGHT); // also check this
    ASSERT_EQ(instance.Misc_info.view_height, emptyInfo.view_height);

    ASSERT_EQ(instance.Misc_info.min_dm_starts, emptyInfo.min_dm_starts);
    ASSERT_EQ(instance.Misc_info.max_dm_starts, emptyInfo.max_dm_starts);
    ASSERT_EQ(instance.Misc_info.max_dm_starts, misc_info_t::DEFAULT_MAXIMUM_DEATHMATCH_STARTS); // also check this

    port_features_t emptyFeats = {};

    ASSERT_EQ(instance.Features.gen_types, emptyFeats.gen_types);
    ASSERT_EQ(instance.Features.gen_sectors, emptyFeats.gen_sectors);
    ASSERT_EQ(instance.Features.tx_start, emptyFeats.tx_start);
    ASSERT_EQ(instance.Features.img_png, emptyFeats.img_png);
    ASSERT_EQ(instance.Features.coop_dm_flags, emptyFeats.coop_dm_flags);
    ASSERT_EQ(instance.Features.friend_flag, emptyFeats.friend_flag);
    ASSERT_EQ(instance.Features.pass_through, emptyFeats.pass_through);
    ASSERT_EQ(instance.Features.midtex_3d, emptyFeats.midtex_3d);
    ASSERT_EQ(instance.Features.strife_flags, emptyFeats.strife_flags);
    ASSERT_EQ(instance.Features.medusa_fixed, emptyFeats.medusa_fixed);
    ASSERT_EQ(instance.Features.lax_sprites, emptyFeats.lax_sprites);
    ASSERT_EQ(instance.Features.mix_textures_flats, emptyFeats.mix_textures_flats);
    ASSERT_EQ(instance.Features.neg_patch_offsets, emptyFeats.neg_patch_offsets);
    ASSERT_EQ(instance.Features.no_need_players, emptyFeats.no_need_players);
    ASSERT_EQ(instance.Features.tag_666, emptyFeats.tag_666);
    ASSERT_EQ(instance.Features.extra_floors, emptyFeats.extra_floors);
    ASSERT_EQ(instance.Features.slopes, emptyFeats.slopes);

    ASSERT_EQ(instance.gen_linetypes[0].base, 0);

    ASSERT_EQ(instance.num_gen_linetypes, 0);
}
