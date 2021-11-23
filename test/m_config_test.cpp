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
        global::install_dir = getChildPath("install");
    }

    void TearDown() override
    {
        global::config_file.clear();
        global::install_dir.clear();
        global::home_dir.clear();
        TempDirContext::TearDown();
    }
};

TEST(MConfigBlank, MParseConfigFileNoPath)
{
    // We don't have the location set yet? Error out
    ASSERT_DEATH(Fatal([]{ M_ParseConfigFile(); }), "Home directory");
    ASSERT_EQ(M_ParseDefaultConfigFile(), -1);
}

TEST_F(MConfig, MParseConfigFileNotFound)
{
    // Get an error result if we don't have the file itself
    ASSERT_EQ(M_ParseConfigFile(), -1);
}

TEST_F(MConfig, MParseConfigFile)
{
    SString paths[2] = { getChildPath("config.cfg"), getChildPath("install/defaults.cfg" )};
    int (*funcs[2])() = { M_ParseConfigFile, M_ParseDefaultConfigFile };

    ASSERT_TRUE(FileMakeDir(getChildPath("install")));
    mDeleteList.push(getChildPath("install"));

    for(int i = 0; i < 2; ++i)
    {
        const SString &path = paths[i];
        std::ofstream os(path.get(), std::ios::trunc);
        ASSERT_TRUE(os.is_open());
        mDeleteList.push(path);
        os.close();

        // Check empty file is ok
        ASSERT_EQ(funcs[i](), 0);

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
        os << "dotty_axis_col 7b2d43\n"; // NOTE: # not supported
        os.close();

        ASSERT_EQ(funcs[i](), 0);
        ASSERT_EQ(global::home_dir, mTempDir);  // Not changed
        ASSERT_FALSE(global::show_help);

        ASSERT_EQ(global::Pwad_list.size(), 3);
        ASSERT_EQ(global::Pwad_list[0], "file1");
        ASSERT_EQ(global::Pwad_list[1], "file2");
        ASSERT_EQ(global::Pwad_list[2], "file3");
        global::Pwad_list.clear();

        ASSERT_EQ(gInstance.loaded.iwadName, "doom 3.wad");   // spaces get included
        gInstance.loaded.iwadName.clear();

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
}

TEST(MConfigBlank, MWriteConfigFileNotSetup)
{
    ASSERT_DEATH(Fatal([]{ M_WriteConfigFile(); }), "Configuration file");
}

TEST_F(MConfig, MWriteConfig)
{
    config::auto_load_recent = true;
    config::begin_maximized = true;
    config::backup_max_files = 777;
    config::default_port = "Doom \\ # Legacy    ";
    config::dotty_axis_col = RGB_MAKE(99, 90, 80);

    global::udmf_testing = true;    // this one shall not be saved
    gInstance.loaded.levelName = "NEW LEVEL";
    global::Pwad_list = { "file1", "file2 file3" };

    global::config_file = getChildPath("configx.cfg");  // pick any name

    ASSERT_EQ(M_WriteConfigFile(), 0);
    mDeleteList.push(global::config_file);

    // Now unset them
    config::auto_load_recent = false;
    config::begin_maximized = false;
    config::backup_max_files = 30;
    config::default_port = "vanilla";
    config::dotty_axis_col = RGB_MAKE(0, 128, 255);

    global::udmf_testing = false;
    gInstance.loaded.levelName.clear();
    global::Pwad_list.clear();

    // Now read config back

    ASSERT_EQ(M_ParseConfigFile(), 0);

    ASSERT_TRUE(config::auto_load_recent);
    ASSERT_TRUE(config::begin_maximized);
    ASSERT_EQ(config::backup_max_files, 777);
    ASSERT_EQ(config::default_port, "Doom \\ # Legacy");
    ASSERT_EQ(config::dotty_axis_col, RGB_MAKE(99, 90, 80));

    ASSERT_FALSE(global::udmf_testing); // still false
    ASSERT_TRUE(gInstance.loaded.levelName.empty());
    ASSERT_TRUE(global::Pwad_list.empty());

    // Now unset them
    config::auto_load_recent = false;
    config::begin_maximized = false;
    config::backup_max_files = 30;
    config::default_port = "vanilla";
    config::dotty_axis_col = RGB_MAKE(0, 128, 255);
}

TEST(MConfigArgs, MParseCommandLine)
{
	// NOTE: check that both - and -- work and short name too

    // Test loose files
    std::vector<const char *> argv;
    argv = { "file1", "file 2", "" };
    // First pass doesn't use it
    M_ParseCommandLine(3, argv.data(), CommandLinePass::early);
    ASSERT_TRUE(global::Pwad_list.empty());
    // Second pass uses it
    M_ParseCommandLine(3, argv.data(), CommandLinePass::normal);
    ASSERT_EQ(global::Pwad_list.size(), 3);
    ASSERT_EQ(global::Pwad_list[0], "file1");
    ASSERT_EQ(global::Pwad_list[1], "file 2");
    ASSERT_EQ(global::Pwad_list[2], "");
    global::Pwad_list.clear();

    // Test illegal argument
    argv = { "file", "-unknown-abcxyz" };
	// Both passes reject invalid settings
	ASSERT_DEATH(Fatal([&argv]{ M_ParseCommandLine(2, argv.data(),
												   CommandLinePass::early); }),
				 "unknown option");
    ASSERT_DEATH(Fatal([&argv]{ M_ParseCommandLine(2, argv.data(),
												   CommandLinePass::normal); }),
				 "unknown option");
    global::Pwad_list.clear();

    // Test file AND valid arg
	// Pass 1
	argv = { "file3", "-home", "home1" };
	M_ParseCommandLine(3, argv.data(), CommandLinePass::early);
	ASSERT_TRUE(global::Pwad_list.empty());
	ASSERT_EQ(global::home_dir, "home1");
	global::home_dir.clear();
	// Pass 2: ignore the -home arg but populate loose files
	M_ParseCommandLine(3, argv.data(), CommandLinePass::normal);
	ASSERT_EQ(global::Pwad_list.size(), 1);
	ASSERT_EQ(global::Pwad_list[0], "file3");
	ASSERT_TRUE(global::home_dir.empty());
	global::Pwad_list.clear();

	// Test that missing argument shows error
	// First pass actively looks for -home
	argv = { "file4", "-home" };
	ASSERT_DEATH(Fatal([&argv]{ M_ParseCommandLine(2, argv.data(),
												   CommandLinePass::early); }),
				 "missing argument");
	global::Pwad_list.clear();
	// Second pass still cares about integrity
	ASSERT_DEATH(Fatal([&argv]{ M_ParseCommandLine(2, argv.data(),
												   CommandLinePass::normal); }),
				 "missing argument");
	global::Pwad_list.clear();

	// Test boolean assignment
	argv = { "-v" };
	M_ParseCommandLine(1, argv.data(), CommandLinePass::early);
	ASSERT_TRUE(global::show_version);
	global::show_version = false;

	// Reject double-dash short options
	argv = { "--v" };
	ASSERT_DEATH(Fatal([&argv]{ M_ParseCommandLine(1, argv.data(),
												   CommandLinePass::early); }),
				 "unknown option");

	// Test that anything means true
	argv = { "--version", "yeah" };
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early);
	ASSERT_TRUE(global::show_version);
	global::show_version = false;

	// Test that second-pass options get ignored in pass 1.
	// Also combine with loose stuff
	argv = { "loose file.wad", "", "--file", "doom.wad", "landing page.wad" };
	M_ParseCommandLine(5, argv.data(), CommandLinePass::early);
	ASSERT_TRUE(global::Pwad_list.empty());
	// On pass 2, they get combined
	M_ParseCommandLine(5, argv.data(), CommandLinePass::normal);
	ASSERT_EQ(global::Pwad_list.size(), 4);
	ASSERT_EQ(global::Pwad_list[0], "loose file.wad");
	ASSERT_EQ(global::Pwad_list[1], "");
	ASSERT_EQ(global::Pwad_list[2], "doom.wad");
	ASSERT_EQ(global::Pwad_list[3], "landing page.wad");
	global::Pwad_list.clear();
	argv = { "loose file.wad", "", "-f", "doom.wad", "landing page.wad",
		"--merge", "res1.wad", "res2.wad"
	};
	M_ParseCommandLine(8, argv.data(), CommandLinePass::normal);
	ASSERT_EQ(global::Pwad_list.size(), 4);
	ASSERT_EQ(global::Pwad_list[0], "loose file.wad");
	ASSERT_EQ(global::Pwad_list[1], "");
	ASSERT_EQ(global::Pwad_list[2], "doom.wad");
	ASSERT_EQ(global::Pwad_list[3], "landing page.wad");
	global::Pwad_list.clear();
	ASSERT_EQ(gInstance.loaded.resourceList.size(), 2);
	ASSERT_EQ(gInstance.loaded.resourceList[0], "res1.wad");
	ASSERT_EQ(gInstance.loaded.resourceList[1], "res2.wad");
	gInstance.loaded.resourceList.clear();

	// Test integer stuff
	argv = { "-backup_max_files", "-23", "--default_gamma", "25" };
	M_ParseCommandLine(4, argv.data(), CommandLinePass::normal);
	ASSERT_EQ(config::backup_max_files, -23);
	ASSERT_EQ(config::usegamma, 25);
	config::backup_max_files = 30;
	config::usegamma = 2;

	// Test the boolean off options
	argv = { "-quiet", "off" };
	global::Quiet = true;
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early);
	ASSERT_FALSE(global::Quiet);
	argv = { "-q", "no" };
	global::Quiet = true;
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early);
	ASSERT_FALSE(global::Quiet);
	argv = { "--quiet", "false" };
	global::Quiet = true;
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early);
	ASSERT_FALSE(global::Quiet);
	argv = { "-quiet", "0" };
	global::Quiet = true;
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early);
	ASSERT_FALSE(global::Quiet);

	// Test the special warp behaviour
	argv = { "-warp", "map01" };
	M_ParseCommandLine(2, argv.data(), CommandLinePass::normal);
	ASSERT_EQ(gInstance.loaded.levelName, "map01");	// gets uppercase
	gInstance.loaded.levelName.clear();
	// Check that one argument is valid
	argv = { "--warp", "09" };
	M_ParseCommandLine(2, argv.data(), CommandLinePass::normal);
	ASSERT_EQ(gInstance.loaded.levelName, "09");
	gInstance.loaded.levelName.clear();

	// Check that loose files can be located within
	argv = { "file1", "-warp", "map01", "file2", "--merge", "res1", "res2" };
	M_ParseCommandLine(7, argv.data(), CommandLinePass::normal);
	ASSERT_EQ(global::Pwad_list.size(), 2);
	ASSERT_EQ(global::Pwad_list[0], "file1");
	ASSERT_EQ(global::Pwad_list[1], "file2");
	ASSERT_EQ(gInstance.loaded.levelName, "map01");
	ASSERT_EQ(gInstance.loaded.resourceList.size(), 2);
	ASSERT_EQ(gInstance.loaded.resourceList[0], "res1");
	ASSERT_EQ(gInstance.loaded.resourceList[1], "res2");
	global::Pwad_list.clear();
	gInstance.loaded.levelName.clear();
	gInstance.loaded.resourceList.clear();

	// Check that loose files can be located within and arguments can be redone
	argv = { "file1", "--warp", "map01", "file2", "-warp", "1", "3", "file3" };
	M_ParseCommandLine(8, argv.data(), CommandLinePass::normal);
	ASSERT_EQ(global::Pwad_list.size(), 3);
	ASSERT_EQ(global::Pwad_list[0], "file1");
	ASSERT_EQ(global::Pwad_list[1], "file2");
	ASSERT_EQ(global::Pwad_list[2], "file3");
	ASSERT_EQ(gInstance.loaded.levelName, "13");
	global::Pwad_list.clear();
	gInstance.loaded.levelName.clear();

	// Check that stringList options can be repeatedly argumented
	argv = { "file1", "-file", "file2", "-merge", "res1", "--file", "file3",
		"file4", "-merge", "res2", "res3" };
	M_ParseCommandLine(11, argv.data(), CommandLinePass::normal);
	ASSERT_EQ(global::Pwad_list.size(), 4);
	ASSERT_EQ(global::Pwad_list[0], "file1");
	ASSERT_EQ(global::Pwad_list[1], "file2");
	ASSERT_EQ(global::Pwad_list[2], "file3");
	ASSERT_EQ(global::Pwad_list[3], "file4");
	ASSERT_EQ(gInstance.loaded.resourceList.size(), 3);
	ASSERT_EQ(gInstance.loaded.resourceList[0], "res1");
	ASSERT_EQ(gInstance.loaded.resourceList[1], "res2");
	ASSERT_EQ(gInstance.loaded.resourceList[2], "res3");
	global::Pwad_list.clear();
	gInstance.loaded.resourceList.clear();
}

// M_PrintCommandLineOptions is tested in system testing

static std::unordered_map<SString, std::vector<SString>> sUnitTokens;

TEST_F(MConfig, InstanceMLoadUserState)
{
	Instance inst;
	// crc32_c raw=1 extra=0
	// Resulted filenme is cache_dir + "/cache/%08X%08X.dat"
	// crc.extra, crc.raw
	global::cache_dir = mTempDir;
	ASSERT_TRUE(FileMakeDir(getChildPath("cache")));
	mDeleteList.push(getChildPath("cache"));

	// Try with no file: should fail
	ASSERT_FALSE(inst.M_LoadUserState());

	// Prepare cache file
	std::ofstream os(getChildPath("cache/0000000000000001.dat").get(),
					 std::ios::trunc);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(getChildPath("cache/0000000000000001.dat"));
	os << " # Stuff\n";
	os << "\n";
	os << "editor \"hello world\" again\n";
	os << "whatever is this\n";	// unhandled by any of these
	os << "recused\n";
	os << "render3d image\n";
	os << "browser preferred\n";
	os << "grid high resolution\n";
	os << "props properties \"\"\n";
	os << "render3d again another rendering\n";
	os << "grid low key\n";
	os << " # Stuff\n";
	os.close();

	// We use the mockup handlers below to add to a map, which is global
	// Now it should work
	ASSERT_TRUE(inst.M_LoadUserState());

	// TODO: check sUnitTokens
	ASSERT_EQ(sUnitTokens["editor"].size(), 3);
	ASSERT_EQ(sUnitTokens["editor"][0], "editor_editor");
	ASSERT_EQ(sUnitTokens["editor"][1], "editor_hello world");
	ASSERT_EQ(sUnitTokens["editor"][2], "editor_again");
	ASSERT_EQ(sUnitTokens["recused"].size(), 1);
	ASSERT_EQ(sUnitTokens["recused"][0], "recused_recused");
	ASSERT_EQ(sUnitTokens["render3d"].size(), 6);
	ASSERT_EQ(sUnitTokens["render3d"][0], "render3d_render3d");
	ASSERT_EQ(sUnitTokens["render3d"][1], "render3d_image");
	ASSERT_EQ(sUnitTokens["render3d"][2], "render3d_render3d");
	ASSERT_EQ(sUnitTokens["render3d"][3], "render3d_again");
	ASSERT_EQ(sUnitTokens["render3d"][4], "render3d_another");
	ASSERT_EQ(sUnitTokens["render3d"][5], "render3d_rendering");
	ASSERT_EQ(sUnitTokens["browser"].size(), 2);
	ASSERT_EQ(sUnitTokens["browser"][0], "browser_browser");
	ASSERT_EQ(sUnitTokens["browser"][1], "browser_preferred");
	ASSERT_EQ(sUnitTokens["grid"].size(), 6);
	ASSERT_EQ(sUnitTokens["grid"][0], "grid_grid");
	ASSERT_EQ(sUnitTokens["grid"][1], "grid_high");
	ASSERT_EQ(sUnitTokens["grid"][2], "grid_resolution");
	ASSERT_EQ(sUnitTokens["grid"][3], "grid_grid");
	ASSERT_EQ(sUnitTokens["grid"][4], "grid_low");
	ASSERT_EQ(sUnitTokens["grid"][5], "grid_key");
	// Keep in mind the last value gets called too
	ASSERT_EQ(sUnitTokens["props"].size(), 4);
	ASSERT_EQ(sUnitTokens["props"][0], "props_props");
	ASSERT_EQ(sUnitTokens["props"][1], "props_properties");
	ASSERT_EQ(sUnitTokens["props"][2], "props_");
	ASSERT_EQ(sUnitTokens.size(), 6);	// only these got stuff

	ASSERT_EQ(sUnitTokens["props"].size(), 4);	// check it got called
	ASSERT_EQ(sUnitTokens["props"][3], "LoadValues");

	global::cache_dir.clear();
	sUnitTokens.clear();
}

TEST_F(MConfig, InstanceMSaveUserState)
{
	Instance inst;
	ASSERT_FALSE(inst.M_SaveUserState());	// can't save if no location
	sUnitTokens.clear();

	global::cache_dir = mTempDir;
	ASSERT_TRUE(FileMakeDir(getChildPath("cache")));
	mDeleteList.push(getChildPath("cache"));

	ASSERT_TRUE(inst.M_SaveUserState());
	mDeleteList.push(getChildPath("cache/0000000000000001.dat"));
	global::cache_dir.clear();

	ASSERT_EQ(sUnitTokens["WriteUser"].size(), 6);
	ASSERT_EQ(sUnitTokens["WriteUser"][0], "editor");
	ASSERT_EQ(sUnitTokens["WriteUser"][1], "grid");
	ASSERT_EQ(sUnitTokens["WriteUser"][2], "render3d");
	ASSERT_EQ(sUnitTokens["WriteUser"][3], "browser");
	ASSERT_EQ(sUnitTokens["WriteUser"][4], "props");
	ASSERT_EQ(sUnitTokens["WriteUser"][5], "recused");
	sUnitTokens.clear();
}

TEST(MConfigBlank, InstanceMDefaultUserState)
{
	sUnitTokens.clear();

	Instance().M_DefaultUserState();

	ASSERT_EQ(sUnitTokens["default"].size(), 4);
	ASSERT_EQ(sUnitTokens["default"][0], "gridInit");
	ASSERT_EQ(sUnitTokens["default"][1], "zoomWholeMap");
	ASSERT_EQ(sUnitTokens["default"][2], "renderSetup");
	ASSERT_EQ(sUnitTokens["default"][3], "editorDefaultState");
	sUnitTokens.clear();
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
	if(tokens.empty() || tokens[0] != "browser")
		return false;
	for(const SString &token : tokens)
		sUnitTokens["browser"].push_back("browser_" + token);
	return true;
}

void Grid_State_c::Init()
{
	sUnitTokens["default"].push_back("gridInit");
}

bool Props_ParseUser(Instance &inst, const std::vector<SString> &tokens)
{
	if(tokens.empty() || tokens[0] != "props")
		return false;
	for(const SString &token : tokens)
		sUnitTokens["props"].push_back("props_" + token);
	return true;
}

void Props_LoadValues(const Instance &inst)
{
	sUnitTokens["props"].push_back("LoadValues");
}

//
// Parse color mockup
//
rgb_color_t ParseColor(const SString &cstr)
{
    // Use some independent example
    return (rgb_color_t)strtol(cstr.c_str(), nullptr, 16) << 8;
}

void Instance::ZoomWholeMap()
{
	sUnitTokens["default"].push_back("zoomWholeMap");
}

bool Instance::Grid_ParseUser(const std::vector<SString> &tokens)
{
	if(tokens.empty() || tokens[0] != "grid")
		return false;
	for(const SString &token : tokens)
		sUnitTokens["grid"].push_back("grid_" + token);
	return true;
}

void Instance::Render3D_Setup()
{
	sUnitTokens["default"].push_back("renderSetup");
}

bool Instance::Editor_ParseUser(const std::vector<SString> &tokens)
{
	if(tokens.empty() || tokens[0] != "editor")
		return false;
	for(const SString &token : tokens)
		sUnitTokens["editor"].push_back("editor_" + token);
	return true;
}

bool Instance::RecUsed_ParseUser(const std::vector<SString> &tokens)
{
	if(tokens.empty() || tokens[0] != "recused")
		return false;
	for(const SString &token : tokens)
		sUnitTokens["recused"].push_back("recused_" + token);
	return true;
}

bool Instance::Render3D_ParseUser(const std::vector<SString> &tokens)
{
	if(tokens.empty() || tokens[0] != "render3d")
		return false;
	for(const SString &token : tokens)
		sUnitTokens["render3d"].push_back("render3d_" + token);
	return true;
}

void Instance::Editor_DefaultState()
{
	sUnitTokens["default"].push_back("editorDefaultState");
}

void Document::getLevelChecksum(crc32_c &crc) const
{
}

void Instance::Grid_WriteUser(std::ostream &os) const
{
	sUnitTokens["WriteUser"].push_back("grid");
}

void Instance::Props_WriteUser(std::ostream &os) const
{
	sUnitTokens["WriteUser"].push_back("props");
}

void Instance::Editor_WriteUser(std::ostream &os) const
{
	sUnitTokens["WriteUser"].push_back("editor");
}

void Instance::Browser_WriteUser(std::ostream &os) const
{
	sUnitTokens["WriteUser"].push_back("browser");
}

void Instance::RecUsed_WriteUser(std::ostream &os) const
{
	sUnitTokens["WriteUser"].push_back("recused");
}

void Instance::Render3D_WriteUser(std::ostream &os) const
{
	sUnitTokens["WriteUser"].push_back("render3d");
}

DocumentModule::DocumentModule(Document &doc) : inst(doc.inst), doc(doc)
{
}

void Basis::EditUnit::destroy()
{
}
