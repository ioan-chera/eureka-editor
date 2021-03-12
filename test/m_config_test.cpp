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

#include "m_config.h"

#include "testUtils/FatalHandler.hpp"
#include "testUtils/TempDirContext.hpp"
#include "Instance.h"
#include "m_strings.h"
#include "gtest/gtest.h"

//
// Fixture
//
class MConfig : public TempDirContext
{
protected:
    void SetUp() override
    {
        TempDirContext::SetUp();
        global::home_dir = mTempDir;
    }

    void TearDown() override
    {
        global::config_file.clear();
        global::home_dir.clear();
        TempDirContext::TearDown();
    }
};

TEST(MConfigBlank, MParseConfigFileNoPath)
{
    // We don't have the location set yet? Error out
    ASSERT_DEATH(Fatal([]{ M_ParseConfigFile(); }), "Home directory");
}

TEST_F(MConfig, MParseConfigFileNotFound)
{
    // Get an error result if we don't have the file itself
    ASSERT_EQ(M_ParseConfigFile(), -1);
}

TEST_F(MConfig, MParseConfigFile)
{
    SString path = getChildPath("config.cfg");
    std::ofstream os(path.get(), std::ios::trunc);
    ASSERT_TRUE(os.is_open());
    mDeleteList.push(path);
    os.close();

    // Check empty file is ok
    ASSERT_EQ(M_ParseConfigFile(), 0);

    os.open(path.get());
    ASSERT_TRUE(os.is_open());
    os << "\n";
    os << "#\n";
    os << "   # Test comment\n";
    os << "Single\n";   // Safe to skip
    os << "Single \n";   // Safe to skip
    os << "home michael\n";  // Home must NOT be changed
    os << "help 1\n";   // Also unchanged
    os << "file file1 file2 file3\n";
    os << "iwad doom 3.wad\n";
    os << "udmftest yes\n";
    os << "backup_max_files -999\n";
    os << "bsp_on_save off\n";  // test "off"
    os << "bsp_gl_nodes no\n";  // test "no"
    os << "grid_snap_indicator false\n";    // test "false"
    os << "leave_offsets_alone 0\n";    // test 0
    os << "dotty_axis_col 123 45 67\n"; // NOTE: this format isn't the real one
    os.close();

    ASSERT_EQ(M_ParseConfigFile(), 0);
    ASSERT_EQ(global::home_dir, mTempDir);  // Not changed
    ASSERT_FALSE(global::show_help);

    ASSERT_EQ(global::Pwad_list.size(), 3);
    ASSERT_EQ(global::Pwad_list[0], "file1");
    ASSERT_EQ(global::Pwad_list[1], "file2");
    ASSERT_EQ(global::Pwad_list[2], "file3");
    global::Pwad_list.clear();

    ASSERT_EQ(gInstance.Iwad_name, "doom 3.wad");   // spaces get included
    gInstance.Iwad_name.clear();

    ASSERT_TRUE(global::udmf_testing);
    global::udmf_testing = false;

    ASSERT_EQ(config::backup_max_files, -999);
    config::backup_max_files = 30;

    ASSERT_FALSE(config::bsp_on_save);
    config::bsp_on_save = true;

    ASSERT_FALSE(config::bsp_gl_nodes);
    config::bsp_gl_nodes = true;

    ASSERT_EQ(config::dotty_axis_col, RGB_MAKE(123, 45, 67));
    config::dotty_axis_col = RGB_MAKE(0, 128, 255);

    ASSERT_FALSE(config::grid_snap_indicator);
    config::grid_snap_indicator = true;

    ASSERT_FALSE(config::leave_offsets_alone);
    config::leave_offsets_alone = true;
}

//========================================================================
//
// Mockups
//
//========================================================================

static const int LINFO_Length = 1;

int  config::grid_style;  // 0 = squares, 1 = dotty
int config::gui_scheme    = 1;  // gtk+
bool config::bsp_on_save    = true;
int config::panel_gamma = 2;
bool config::bsp_force_v5        = false;
bool config::bsp_gl_nodes        = true;
bool config::bsp_warnings    = false;
SString config::default_port = "vanilla";
int config::gui_color_set = 1;  // bright
rgb_color_t config::gui_custom_bg = RGB_MAKE(0xCC, 0xD5, 0xDD);
rgb_color_t config::gui_custom_ig = RGB_MAKE(255, 255, 255);
rgb_color_t config::gui_custom_fg = RGB_MAKE(0, 0, 0);
bool config::swap_sidedefs = false;
bool config::bsp_compressed        = false;
rgb_color_t config::dotty_axis_col  = RGB_MAKE(0, 128, 255);
int  config::grid_ratio_low  = 1;  // (low must be > 0)
bool config::begin_maximized  = false;
bool config::bsp_force_zdoom    = false;
rgb_color_t config::dotty_major_col = RGB_MAKE(0, 0, 238);
rgb_color_t config::dotty_minor_col = RGB_MAKE(0, 0, 187);
rgb_color_t config::dotty_point_col = RGB_MAKE(0, 0, 255);
int  config::grid_ratio_high = 3;  // custom ratio (high must be >= low)
bool config::map_scroll_bars  = true;
int  config::new_sector_size = 128;
rgb_color_t config::normal_axis_col  = RGB_MAKE(0, 128, 255);
rgb_color_t config::normal_main_col  = RGB_MAKE(0, 0, 238);
rgb_color_t config::normal_flat_col  = RGB_MAKE(60, 60, 120);
int  config::render_far_clip = 32768;
rgb_color_t config::transparent_col = RGB_MAKE(0, 255, 255);
bool config::auto_load_recent = false;
int config::backup_max_files = 30;
int config::backup_max_space = 60;  // MB
int  config::bsp_split_factor    = DEFAULT_FACTOR;
int config::floor_bump_medium = 8;
int config::floor_bump_large  = 64;
int config::floor_bump_small  = 1;
int config::light_bump_small  = 4;
int config::light_bump_medium = 16;
int config::light_bump_large  = 64;
rgb_color_t config::normal_small_col = RGB_MAKE(60, 60, 120);
bool config::browser_small_tex = false;
int config::default_edit_mode = 3;  // Vertices
int  config::grid_default_size = 64;
bool config::grid_default_snap = false;
int  config::grid_default_mode = 0;  // off
bool config::render_high_detail    = false;
bool config::browser_combine_tex = false;
bool config::grid_snap_indicator = true;
int config::highlight_line_info = (int)LINFO_Length;
bool config::leave_offsets_alone = true;
int config::minimum_drag_pixels = 5;
bool config::render_lock_gravity   = false;
int  config::render_pixel_aspect = 83;  //  100 * width / height
bool config::show_full_one_sided = false;
int  config::thing_render_default = 1;
bool config::render_missing_bright = true;
bool config::render_unknown_bright = true;
int config::sector_render_default = (int)SREND_Floor;
bool config::grid_hide_in_free_mode = false;
bool config::sidedef_add_del_buttons = false;
bool config::same_mode_clears_selection = false;
bool config::bsp_fast        = false;
int config::usegamma = 2;
SString global::config_file;
SString global::install_dir;
int global::show_version  = 0;
bool global::udmf_testing = false;
SString global::home_dir;
SString global::log_file;
std::vector<SString> global::Pwad_list;
SString global::cache_dir;
int global::show_help     = 0;

Instance gInstance;

bool Browser_ParseUser(Instance &inst, const std::vector<SString> &tokens)
{
    return true;
}

void Grid_State_c::Init()
{
}

bool Props_ParseUser(Instance &inst, const std::vector<SString> &tokens)
{
    return true;
}

void Props_LoadValues(const Instance &inst)
{
}

//
// Parse color mockup
//
rgb_color_t ParseColor(const SString &cstr)
{
    // Use some independent example
    std::istringstream ss(cstr.get());
    int r, g, b;
    ss >> r >> g >> b;
    return RGB_MAKE(r, g, b);
}

void Instance::ZoomWholeMap()
{
}

bool Instance::Grid_ParseUser(const std::vector<SString> &tokens)
{
    return false;
}

void Instance::Render3D_Setup()
{
}

bool Instance::Editor_ParseUser(const std::vector<SString> &tokens)
{
    return false;
}

bool Instance::RecUsed_ParseUser(const std::vector<SString> &tokens)
{
    return false;
}

bool Instance::Render3D_ParseUser(const std::vector<SString> &tokens)
{
    return false;
}

void Instance::Editor_DefaultState()
{
}

void Document::getLevelChecksum(crc32_c &crc) const
{
}

void Instance::Grid_WriteUser(std::ostream &os) const
{
}

void Instance::Props_WriteUser(std::ostream &os) const
{
}

void Instance::Editor_WriteUser(std::ostream &os) const
{
}

void Instance::Browser_WriteUser(std::ostream &os) const
{
}

void Instance::RecUsed_WriteUser(std::ostream &os) const
{
}

void Instance::Render3D_WriteUser(std::ostream &os) const
{
}

Grid_State_c::Grid_State_c(Instance &inst) : inst(inst)
{
}

Grid_State_c::~Grid_State_c()
{
}

Recently_used::Recently_used(Instance &inst) : inst(inst)
{
}

Recently_used::~Recently_used()
{
}

Render_View_t::Render_View_t(Instance &inst) : inst(inst)
{
}

Render_View_t::~Render_View_t()
{
}

DocumentModule::DocumentModule(Document &doc) : inst(doc.inst), doc(doc)
{
}

sector_3dfloors_c::sector_3dfloors_c()
{
}

sector_3dfloors_c::~sector_3dfloors_c()
{
}

void Basis::EditOperation::destroy()
{
}

extrafloor_c::~extrafloor_c()
{
}

slope_plane_c::slope_plane_c()
{
}

slope_plane_c::~slope_plane_c()
{
}

sector_subdivision_c::~sector_subdivision_c()
{
}
