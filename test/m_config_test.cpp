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

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

//
// Option end
//
static const opt_desc_t kEnd =
{
	nullptr, nullptr, 0, nullptr, nullptr, nullptr
};

//
// Returns an opt_desc_t using only the fields we need for config file
//
template<typename T>
static opt_desc_t configOption(const char *long_name, unsigned flags,
							   T *data_ptr)
{
	return {long_name, nullptr, flags, nullptr, nullptr, data_ptr};
}
template<typename T>
static opt_desc_t configOption(const char *long_name, const char *shortname, unsigned flags,
							   T *data_ptr)
{
	return {long_name, shortname, flags, nullptr, nullptr, data_ptr};
}

//
// Fixture
//
class MConfig : public TempDirContext
{
protected:
	//
	// Demo testing configuration data
	//
	struct Configuration
	{
		SString home_dir;
		SString loadedIwadName;
		bool show_help = false;
		bool udmf_testing = false;
		bool bsp_on_save = false;
		bool bsp_gl_nodes = false;
		bool grid_snap_indicator = false;
		bool leave_offsets_alone = false;
		std::vector<fs::path> Pwad_list;
		int backup_max_files = 0;
		rgb_color_t dotty_axis_col;

		bool auto_load_recent = false;
		bool begin_maximized = false;
		SString default_port;
		SString loadedLevelName;
		fs::path config_file;

		bool show_version = false;
		std::vector<SString> loadedResourceList;
		int usegamma = 0;
		bool Quiet = false;

		std::vector<fs::path> paths;
		fs::path onepath;
	};

	void SetUp() override;
	//
	// Const getter
	//
	const std::vector<opt_desc_t> &options() const
	{
		return mOptions;
	}

	Configuration config;

private:
	void add(const opt_desc_t &option);

	std::vector<opt_desc_t> mOptions = {kEnd};
};

//
// Set up
//
void MConfig::SetUp()
{
	TempDirContext::SetUp();

	add(configOption("auto_load_recent", OptFlag_preference,
					 &config.auto_load_recent));
	add(configOption("backup_max_files", OptFlag_preference,
					 &config.backup_max_files));
	add(configOption("begin_maximized", OptFlag_preference,
					 &config.begin_maximized));
	add(configOption("bsp_gl_nodes", OptFlag_preference, &config.bsp_gl_nodes));
	add(configOption("bsp_on_save", OptFlag_preference, &config.bsp_on_save));
	add(configOption("config", OptFlag_pass1 | OptFlag_helpNewline,
					 &config.config_file));
	add(configOption("default_gamma", OptFlag_preference, &config.usegamma));
	add(configOption("default_port", OptFlag_preference, &config.default_port));
	add(configOption("dotty_axis_col", OptFlag_preference, &config.dotty_axis_col));
	add(configOption("file", "f", 0, &config.Pwad_list));
	add(configOption("grid_snap_indicator", OptFlag_preference,
					 &config.grid_snap_indicator));
	add(configOption("help", OptFlag_pass1, &config.show_help));
	add(configOption("home", OptFlag_pass1, &config.home_dir));
	add(configOption("iwad", 0, &config.loadedIwadName));
	add(configOption("leave_offsets_alone", OptFlag_preference,
					 &config.leave_offsets_alone));
	add(configOption("merge", "m", 0, &config.loadedResourceList));
	add(configOption("quiet", "q", OptFlag_pass1, &config.Quiet));
	add(configOption("udmftest", OptFlag_hide, &config.udmf_testing));
	add(configOption("version", "v", OptFlag_pass1, &config.show_version));
	add(configOption("warp", OptFlag_warp | OptFlag_helpNewline,
					 &config.loadedLevelName));
	// Path type
	add(configOption("path", OptFlag_preference, &config.paths));
	add(configOption("onepath", OptFlag_preference, &config.onepath));
}

//
// Adds a new option, ensuring null termination
//
void MConfig::add(const opt_desc_t &option)
{
	mOptions.back() = option;
	mOptions.push_back(kEnd);
}

TEST_F(MConfig, MParseConfigFileNotFound)
{
    // Get an error result if we don't have the file itself
    ASSERT_EQ(M_ParseConfigFile(getChildPath("nothing.cfg"), options().data()), -1);
}

TEST_F(MConfig, MParseConfigEmptyFile)
{
	fs::path path = getChildPath("something.cfg");
	FILE *f = fopen(path.u8string().c_str(), "wb");
	ASSERT_TRUE(f);
	mDeleteList.push(path);
	fclose(f);
	ASSERT_EQ(M_ParseConfigFile(path, options().data()), 0);
}

TEST_F(MConfig, MParseConfigFile)
{
    fs::path path = getChildPath("config.cfg");

	std::ofstream os(path, std::ios::trunc);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(path);
	os.close();

	os.open(path);
	ASSERT_TRUE(os.is_open());
	os << "\n";
	os << "#\n";
	os << "   # Test comment\n";
	os << "Single\n";   // Safe to skip
	os << "Single \n";   // Safe to skip
	os << "home michael\n";  // Home must NOT be changed
	os << "help 1\n";   // Also unchanged
	os << "file file1 \"file 2\" file3\n";
	os << "iwad \"doom 3.wad\"\n";
	os << "udmftest yes\n";
	os << "backup_max_files -999\n";
	os << "bsp_on_save off\n";  // test "off"
	os << "bsp_gl_nodes no\n";  // test "no"
	os << "grid_snap_indicator false\n";    // test "false"
	os << "leave_offsets_alone 0\n";    // test 0
	os << "dotty_axis_col 7b2d43\n"; // NOTE: # not supported
	os.close();

	// Prepare some prereqs
	config.home_dir = "jackson";
	config.bsp_on_save = true;
	config.bsp_gl_nodes = true;
	config.grid_snap_indicator = true;
	config.leave_offsets_alone = true;

	ASSERT_EQ(M_ParseConfigFile(path, options().data()), 0);
	ASSERT_EQ(config.home_dir, "jackson");	// unchanged
	ASSERT_FALSE(config.show_help);	// unchanged

	ASSERT_EQ(config.Pwad_list.size(), 3);
	ASSERT_EQ(config.Pwad_list[0], "file1");
	ASSERT_EQ(config.Pwad_list[1], "file 2");
	ASSERT_EQ(config.Pwad_list[2], "file3");

	ASSERT_EQ(config.loadedIwadName, "doom 3.wad");   // spaces get included

	ASSERT_TRUE(config.udmf_testing);

	ASSERT_EQ(config.backup_max_files, -999);
	config::backup_max_files = 30;

	ASSERT_FALSE(config.bsp_on_save);

	ASSERT_FALSE(config.bsp_gl_nodes);

	ASSERT_EQ(config.dotty_axis_col, rgbMake(123, 45, 67));

	ASSERT_FALSE(config.grid_snap_indicator);

	ASSERT_FALSE(config.leave_offsets_alone);
}

TEST_F(MConfig, ParsePathList)
{
	fs::path path = getChildPath("config.cfg");

	std::ofstream os(path, std::ios::trunc);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(path);
	os.close();

	os.open(path);
	ASSERT_TRUE(os.is_open());
	// Use a wild combo of spaces, non-spaces and quotes
	os << "path \"/michael/jack \"\"Ripper\"\" son\". .. here/next\"here/../here\"here\n";
	os << "onepath \"jacob's \"\"ladder\"\"/norg.txt\"";
	os.close();

	ASSERT_EQ(M_ParseConfigFile(path, options().data()), 0);

	ASSERT_EQ(config.paths.size(), 6);
	ASSERT_EQ(config.paths[0], fs::path("/") / "michael" / "jack \"Ripper\" son");
	ASSERT_EQ(config.paths[1], fs::path("."));
	ASSERT_EQ(config.paths[2], fs::path(".."));
	ASSERT_EQ(config.paths[3], fs::path("here") / "next");
	ASSERT_EQ(config.paths[4].lexically_normal(), fs::path("here"));
	ASSERT_EQ(config.paths[5], fs::path("here"));

	ASSERT_EQ(config.onepath, fs::path("jacob's \"ladder\"") / "norg.txt");
}

TEST_F(MConfig, ParseEmptyList)
{
	fs::path path = getChildPath("config.cfg");

	std::ofstream os(path, std::ios::trunc);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(path);
	os.close();

	os.open(path);
	ASSERT_TRUE(os.is_open());
	// Use a wild combo of spaces, non-spaces and quotes
	os << "path {}\n";
	os << "file {}\n";
	os.close();

	ASSERT_EQ(M_ParseConfigFile(path, options().data()), 0);

	ASSERT_TRUE(config.paths.empty());
	ASSERT_TRUE(config.Pwad_list.empty());
}

TEST_F(MConfig, MWriteConfig)
{
    config.auto_load_recent = true;
    config.begin_maximized = true;
    config.backup_max_files = 777;
    config.default_port = "Doom \\ # Legacy    ";
    config.dotty_axis_col = rgbMake(99, 90, 80);

    config.udmf_testing = true;    // this one shall not be saved
    config.loadedLevelName = "NEW LEVEL";
    config.Pwad_list = { "file1", "file2 file3" };

    config.config_file = getChildPath("configx.cfg");  // pick any name

    ASSERT_EQ(M_WriteConfigFile(config.config_file, options().data()), 0);
    mDeleteList.push(config.config_file);

    // Now unset them
    config.auto_load_recent = false;
    config.begin_maximized = false;
    config.backup_max_files = 30;
    config.default_port = "vanilla";
    config.dotty_axis_col = rgbMake(0, 128, 255);

    config.udmf_testing = false;
    config.loadedLevelName.clear();
    config.Pwad_list.clear();

    // Now read config back

    ASSERT_EQ(M_ParseConfigFile(config.config_file, options().data()), 0);

    ASSERT_TRUE(config.auto_load_recent);
    ASSERT_TRUE(config.begin_maximized);
    ASSERT_EQ(config.backup_max_files, 777);
    ASSERT_EQ(config.default_port, "Doom \\ # Legacy    ");
    ASSERT_EQ(config.dotty_axis_col, rgbMake(99, 90, 80));

    ASSERT_FALSE(config.udmf_testing); // still false
    ASSERT_TRUE(config.loadedLevelName.empty());
    ASSERT_TRUE(config.Pwad_list.empty());
}

TEST_F(MConfig, MWriteConfigPathList)
{
	config.paths = {
		"", 
		".", 
		"/michael/jack \"Ripper\" son", 
		".", 
		"..", 
		"here/n\"ext", 
		"here/../here", 
		"here", 
		"\"\""
	};
	config.onepath = "bi\"g\"/dog";

	config.config_file = getChildPath("configx.cfg");  // pick any name
	ASSERT_EQ(M_WriteConfigFile(config.config_file, options().data()), 0);
	mDeleteList.push(config.config_file);

	config.paths.clear();

	// Now read config back
	ASSERT_EQ(M_ParseConfigFile(config.config_file, options().data()), 0);
	ASSERT_EQ(config.paths.size(), 9);
	ASSERT_EQ(config.paths[0], fs::path(""));
	ASSERT_EQ(config.paths[1], fs::path("."));
	ASSERT_EQ(config.paths[2], fs::path("/") / "michael" / "jack \"Ripper\" son");
	ASSERT_EQ(config.paths[3], fs::path("."));
	ASSERT_EQ(config.paths[4], fs::path(".."));
	ASSERT_EQ(config.paths[5], fs::path("here") / "n\"ext");
	ASSERT_EQ(config.paths[6].lexically_normal(), fs::path("here"));
	ASSERT_EQ(config.paths[7], fs::path("here"));
	ASSERT_EQ(config.paths[8], fs::path("\"\""));

	ASSERT_EQ(config.onepath, "bi\"g\"/dog");
}

TEST_F(MConfig, MParseCommandLine)
{
	// NOTE: check that both - and -- work and short name too

    // Test loose files
    std::vector<const char *> argv;
    argv = { "file1", "file 2", "" };
	std::vector<fs::path> &Pwad_list = config.Pwad_list;
    // First pass doesn't use it
    M_ParseCommandLine(3, argv.data(), CommandLinePass::early, Pwad_list, options().data());
    ASSERT_TRUE(Pwad_list.empty());
    // Second pass uses it
    M_ParseCommandLine(3, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
    ASSERT_EQ(Pwad_list.size(), 3);
    ASSERT_EQ(Pwad_list[0], "file1");
    ASSERT_EQ(Pwad_list[1], "file 2");
    ASSERT_EQ(Pwad_list[2], "");
    Pwad_list.clear();

    // Test illegal argument
    argv = { "file", "-unknown-abcxyz" };
	// Both passes reject invalid settings
	ASSERT_DEATH(Fatal([&argv, &Pwad_list, this]{
		M_ParseCommandLine(2, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	}), "unknown option");
    ASSERT_DEATH(Fatal([&argv, &Pwad_list, this]{
		M_ParseCommandLine(2, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	}), "unknown option");
    Pwad_list.clear();

    // Test file AND valid arg
	// Pass 1
	argv = { "file3", "-home", "home1" };
	M_ParseCommandLine(3, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	ASSERT_TRUE(Pwad_list.empty());
	ASSERT_EQ(config.home_dir, "home1");
	config.home_dir.clear();
	// Pass 2: ignore the -home arg but populate loose files
	M_ParseCommandLine(3, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	ASSERT_EQ(Pwad_list.size(), 1);
	ASSERT_EQ(Pwad_list[0], "file3");
	ASSERT_TRUE(config.home_dir.empty());
	Pwad_list.clear();

	// Test that missing argument shows error
	// First pass actively looks for -home
	argv = { "file4", "-home" };
	ASSERT_DEATH(Fatal([&]{
		M_ParseCommandLine(2, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	}), "missing argument");
	Pwad_list.clear();
	// Second pass still cares about integrity
	ASSERT_DEATH(Fatal([&]{
		M_ParseCommandLine(2, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	}), "missing argument");
	Pwad_list.clear();

	// Test boolean assignment
	argv = { "-v" };
	M_ParseCommandLine(1, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	ASSERT_TRUE(config.show_version);
	config.show_version = false;

	// Reject double-dash short options
	argv = { "--v" };
	ASSERT_DEATH(Fatal([&]{
		M_ParseCommandLine(1, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	}), "unknown option");

	// Test that anything means true
	argv = { "--version", "yeah" };
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	ASSERT_TRUE(config.show_version);
	config.show_version = false;

	// Test that second-pass options get ignored in pass 1.
	// Also combine with loose stuff
	argv = { "loose file.wad", "", "--file", "doom.wad", "landing page.wad" };
	M_ParseCommandLine(5, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	ASSERT_TRUE(Pwad_list.empty());
	// On pass 2, they get combined
	M_ParseCommandLine(5, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	ASSERT_EQ(Pwad_list.size(), 4);
	ASSERT_EQ(Pwad_list[0], "loose file.wad");
	ASSERT_EQ(Pwad_list[1], "");
	ASSERT_EQ(Pwad_list[2], "doom.wad");
	ASSERT_EQ(Pwad_list[3], "landing page.wad");
	Pwad_list.clear();
	argv = { "loose file.wad", "", "-f", "doom.wad", "landing page.wad",
		"--merge", "res1.wad", "res2.wad"
	};
	M_ParseCommandLine(8, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	ASSERT_EQ(Pwad_list.size(), 4);
	ASSERT_EQ(Pwad_list[0], "loose file.wad");
	ASSERT_EQ(Pwad_list[1], "");
	ASSERT_EQ(Pwad_list[2], "doom.wad");
	ASSERT_EQ(Pwad_list[3], "landing page.wad");
	Pwad_list.clear();
	ASSERT_EQ(config.loadedResourceList.size(), 2);
	ASSERT_EQ(config.loadedResourceList[0], "res1.wad");
	ASSERT_EQ(config.loadedResourceList[1], "res2.wad");
	config.loadedResourceList.clear();

	// Test integer stuff
	argv = { "-backup_max_files", "-23", "--default_gamma", "25" };
	M_ParseCommandLine(4, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	ASSERT_EQ(config.backup_max_files, -23);
	ASSERT_EQ(config.usegamma, 25);
	config.backup_max_files = 30;
	config.usegamma = 2;

	// Test the boolean off options
	argv = { "-quiet", "off" };
	config.Quiet = true;
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	ASSERT_FALSE(config.Quiet);
	argv = { "-q", "no" };
	config.Quiet = true;
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	ASSERT_FALSE(config.Quiet);
	argv = { "--quiet", "false" };
	config.Quiet = true;
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	ASSERT_FALSE(config.Quiet);
	argv = { "-quiet", "0" };
	config.Quiet = true;
	M_ParseCommandLine(2, argv.data(), CommandLinePass::early, Pwad_list, options().data());
	ASSERT_FALSE(config.Quiet);

	// Test the special warp behaviour
	argv = { "-warp", "map01" };
	M_ParseCommandLine(2, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	ASSERT_EQ(config.loadedLevelName, "map01");	// gets uppercase
	config.loadedLevelName.clear();
	// Check that one argument is valid
	argv = { "--warp", "09" };
	M_ParseCommandLine(2, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	ASSERT_EQ(config.loadedLevelName, "09");
	config.loadedLevelName.clear();

	// Check that loose files can be located within
	argv = { "file1", "-warp", "map01", "file2", "--merge", "res1", "res2" };
	M_ParseCommandLine(7, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	ASSERT_EQ(Pwad_list.size(), 2);
	ASSERT_EQ(Pwad_list[0], "file1");
	ASSERT_EQ(Pwad_list[1], "file2");
	ASSERT_EQ(config.loadedLevelName, "map01");
	ASSERT_EQ(config.loadedResourceList.size(), 2);
	ASSERT_EQ(config.loadedResourceList[0], "res1");
	ASSERT_EQ(config.loadedResourceList[1], "res2");
	Pwad_list.clear();
	config.loadedLevelName.clear();
	config.loadedResourceList.clear();

	// Check that loose files can be located within and arguments can be redone
	argv = { "file1", "--warp", "map01", "file2", "-warp", "1", "3", "file3" };
	M_ParseCommandLine(8, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	ASSERT_EQ(Pwad_list.size(), 3);
	ASSERT_EQ(Pwad_list[0], "file1");
	ASSERT_EQ(Pwad_list[1], "file2");
	ASSERT_EQ(Pwad_list[2], "file3");
	ASSERT_EQ(config.loadedLevelName, "13");
	Pwad_list.clear();
	config.loadedLevelName.clear();

	// Check that stringList options can be repeatedly argumented
	argv = { "file1", "-file", "file2", "-merge", "res1", "--file", "file3",
		"file4", "-merge", "res2", "res3" };
	M_ParseCommandLine(11, argv.data(), CommandLinePass::normal, Pwad_list, options().data());
	ASSERT_EQ(Pwad_list.size(), 4);
	ASSERT_EQ(Pwad_list[0], "file1");
	ASSERT_EQ(Pwad_list[1], "file2");
	ASSERT_EQ(Pwad_list[2], "file3");
	ASSERT_EQ(Pwad_list[3], "file4");
	ASSERT_EQ(config.loadedResourceList.size(), 3);
	ASSERT_EQ(config.loadedResourceList[0], "res1");
	ASSERT_EQ(config.loadedResourceList[1], "res2");
	ASSERT_EQ(config.loadedResourceList[2], "res3");
	Pwad_list.clear();
	config.loadedResourceList.clear();
}

TEST_F(MConfig, MParsePathListCommandLine)
{
	std::vector<const char *> argv;
	argv = { "--path", "", "/michael/jackson", ".", "..", "here/next", "here/../here", "here", "--onepath", "big guy" };
	std::vector<fs::path> &Pwad_list = config.Pwad_list;

	M_ParseCommandLine((int)argv.size(), argv.data(), CommandLinePass::normal, Pwad_list,
					   options().data());

	ASSERT_EQ(config.paths.size(), 7);
	ASSERT_EQ(config.paths[0], fs::path(""));
	ASSERT_EQ(config.paths[1], fs::path("/") / "michael" / "jackson");
	ASSERT_EQ(config.paths[2], fs::path("."));
	ASSERT_EQ(config.paths[3], fs::path(".."));
	ASSERT_EQ(config.paths[4], fs::path("here") / "next");
	ASSERT_EQ(config.paths[5].lexically_normal(), fs::path("here"));
	ASSERT_EQ(config.paths[6], fs::path("here"));

	ASSERT_EQ(config.onepath, fs::path("big guy"));
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
	std::ofstream os(getChildPath(fs::path("cache") / "0000000000000001.dat"),
					 std::ios::trunc);
	ASSERT_TRUE(os.is_open());
	mDeleteList.push(getChildPath(fs::path("cache") / "0000000000000001.dat"));
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
	ASSERT_EQ(sUnitTokens["props"].size(), 3);
	ASSERT_EQ(sUnitTokens["props"][0], "props_props");
	ASSERT_EQ(sUnitTokens["props"][1], "props_properties");
	ASSERT_EQ(sUnitTokens["props"][2], "props_");
	ASSERT_EQ(sUnitTokens.size(), 6);	// only these got stuff

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
	mDeleteList.push(getChildPath(fs::path("cache") / "0000000000000001.dat"));
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
bool config::bsp_force_v5        = false;
bool config::bsp_gl_nodes        = true;
bool config::bsp_warnings    = false;
SString config::default_port = "vanilla";
int config::gui_color_set = 1;  // bright
rgb_color_t config::gui_custom_bg = rgbMake(0xCC, 0xD5, 0xDD);
rgb_color_t config::gui_custom_ig = rgbMake(255, 255, 255);
rgb_color_t config::gui_custom_fg = rgbMake(0, 0, 0);
bool config::swap_sidedefs = false;
bool config::bsp_compressed        = false;
rgb_color_t config::dotty_axis_col  = rgbMake(0, 128, 255);
int  config::grid_ratio_low  = 1;  // (low must be > 0)
bool config::begin_maximized  = false;
bool config::bsp_force_zdoom    = false;
rgb_color_t config::dotty_major_col = rgbMake(0, 0, 238);
rgb_color_t config::dotty_minor_col = rgbMake(0, 0, 187);
rgb_color_t config::dotty_point_col = rgbMake(0, 0, 255);
int  config::grid_ratio_high = 3;  // custom ratio (high must be >= low)
bool config::map_scroll_bars  = true;
int  config::new_sector_size = 128;
rgb_color_t config::normal_axis_col  = rgbMake(0, 128, 255);
rgb_color_t config::normal_main_col  = rgbMake(0, 0, 238);
rgb_color_t config::normal_flat_col  = rgbMake(60, 60, 120);
int  config::render_far_clip = 32768;
rgb_color_t config::transparent_col = rgbMake(0, 255, 255);
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
rgb_color_t config::normal_small_col = rgbMake(60, 60, 120);
bool config::browser_small_tex = false;
int config::default_edit_mode = 3;  // Vertices
int  config::grid_default_size = 64;
bool config::grid_default_snap = false;
bool config::grid_default_mode = false;
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
fs::path global::config_file;
fs::path global::install_dir;
bool global::show_version;
fs::path global::home_dir;
fs::path global::log_file;
std::vector<fs::path> global::Pwad_list;
fs::path global::cache_dir;
bool global::show_help;

Instance gInstance;

bool Browser_ParseUser(Instance &inst, const std::vector<SString> &tokens)
{
	if(tokens.empty() || tokens[0] != "browser")
		return false;
	for(const SString &token : tokens)
		sUnitTokens["browser"].push_back("browser_" + token);
	return true;
}

void grid::State::Init()
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

void UI_DefaultProps::LoadValues()
{
	sUnitTokens["props"].push_back("LoadValues");
}

void Instance::ZoomWholeMap()
{
	sUnitTokens["default"].push_back("zoomWholeMap");
}

bool grid::State::parseUser(const std::vector<SString> &tokens)
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

void Editor_State_t::defaultState()
{
	sUnitTokens["default"].push_back("editorDefaultState");
}

void Document::getLevelChecksum(crc32_c &crc) const
{
}

void grid::State::writeUser(std::ostream &os) const
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


// UI_NodeDialog
UI_NodeDialog::UI_NodeDialog() : Fl_Double_Window(100, 100)
{
}

int UI_NodeDialog::handle(int event)
{
	return 0;
}

void UI_Canvas::PointerPos(bool in_event)
{
}

void UI_InfoBar::SetScale(double new_scale)
{
}

void UI_InfoBar::SetGrid(int new_step)
{
}

void UI_InfoBar::UpdateSnap()
{
}

void UI_InfoBar::UpdateRatio()
{
}

void UI_CanvasScroll::AdjustPos()
{
}

void Instance::RedrawMap()
{
}
