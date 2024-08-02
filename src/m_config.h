//------------------------------------------------------------------------
//  CONFIG FILE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2018 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __EUREKA_M_CONFIG_H__
#define __EUREKA_M_CONFIG_H__

#include "im_color.h"
#include <variant>
#include <vector>

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

struct LoadingData;

/* ==== CONFIG VARIABLES ==================== */

enum
{
	OptFlag_pass1 = 1 << 0,
	OptFlag_helpNewline = 1 << 1,
	OptFlag_preference = 1 << 2,
	OptFlag_warp = 1 << 3,
	OptFlag_hide = 1 << 4,
};

typedef std::variant<bool *, int *, rgb_color_t *, SString *, fs::path *, std::vector<SString> *, 
					 std::vector<fs::path> *, std::nullptr_t> ArgData;

struct opt_desc_t
{
	const char *long_name;  // Command line arg. or keyword
	const char *short_name; // Abbreviated command line argument

	unsigned flags;    // Flags for this option :
	// '1' : process only on pass 1 of parse_command_line_options()
	// '<' : print extra newline after this option (when dumping)
	// 'v' : a real variable (preference setting)
	// 'w' : warp hack -- accept two numeric args
	// 'H' : hide option from --help display

	const char *desc;   // Description of the option
	const char *arg_desc;  // Description of the argument (NULL --> none or default)

	// Pointer to the data
	ArgData data_ptr;
};


namespace config
{
extern SString default_port;
extern int default_edit_mode;

extern bool auto_load_recent;
extern bool begin_maximized;

extern bool map_scroll_bars;

extern bool leave_offsets_alone;
extern bool same_mode_clears_selection;

extern bool swap_sidedefs;
extern bool show_full_one_sided;
extern bool sidedef_add_del_buttons;

extern int gui_scheme;
extern int gui_color_set;
extern rgb_color_t gui_custom_bg;
extern rgb_color_t gui_custom_ig;
extern rgb_color_t gui_custom_fg;

extern int panel_gamma;

extern int  minimum_drag_pixels;
extern int  highlight_line_info;
extern int  new_sector_size;
extern int  sector_render_default;
extern int   thing_render_default;

extern int  grid_style;
extern bool grid_default_mode;
extern bool grid_default_snap;
extern int  grid_default_size;
extern bool grid_hide_in_free_mode;
extern bool grid_snap_indicator;
extern int  grid_ratio_high;
extern int  grid_ratio_low;

extern rgb_color_t dotty_axis_col;
extern rgb_color_t dotty_major_col;
extern rgb_color_t dotty_minor_col;
extern rgb_color_t dotty_point_col;

extern rgb_color_t normal_axis_col;
extern rgb_color_t normal_main_col;
extern rgb_color_t normal_flat_col;
extern rgb_color_t normal_small_col;

extern int backup_max_files;
extern int backup_max_space;

extern bool browser_small_tex;
extern bool browser_combine_tex;

extern int floor_bump_small;
extern int floor_bump_medium;
extern int floor_bump_large;

extern int light_bump_small;
extern int light_bump_medium;
extern int light_bump_large;

extern int  render_pixel_aspect;
extern int  render_far_clip;
extern bool render_high_detail;
extern bool render_lock_gravity;
extern bool render_missing_bright;
extern bool render_unknown_bright;

extern rgb_color_t transparent_col;

extern bool bsp_on_save;
extern bool bsp_fast;
extern bool bsp_warnings;
extern int  bsp_split_factor;

extern bool bsp_gl_nodes;
extern bool bsp_force_v5;
extern bool bsp_force_zdoom;
extern bool bsp_compressed;

extern LoadingData preloading;
}

extern const opt_desc_t options[];

enum class CommandLinePass
{
	early,
	normal
};


/* ==== FUNCTIONS ==================== */

int M_ParseConfigFile(const fs::path &path, const opt_desc_t *options);
int M_WriteConfigFile(const fs::path &path, const opt_desc_t *options);

void M_ParseEnvironmentVars();
void M_ParseCommandLine(int argc, const char *const *argv,
						CommandLinePass pass, std::vector<fs::path> &Pwad_list, const opt_desc_t *options);

void M_PrintCommandLineOptions();

#endif  /* __EUREKA_M_CONFIG_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
