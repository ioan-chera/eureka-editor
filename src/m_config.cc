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

#include "main.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "lib_adler.h"
#include "im_color.h"
#include "m_config.h"
#include "m_loadsave.h"
#include "r_grid.h"
#include "r_render.h"
#include "e_main.h"	  // RecUsed_xxx

#include "ui_window.h"  // Browser_xxx, Props_xxx



//------------------------------------------------------------------------

//
//  Structures for command line arguments and config settings
//
typedef enum
{
	// End of the options description
	OPT_END = 0,

	// Boolean (toggle)
	// Receptacle is of type: bool
	OPT_BOOLEAN,

	// Integer number,
	// Receptacle is of type: int
	OPT_INTEGER,

	// A color value
	// Receptacle is of type: rgb_color_t
	OPT_COLOR,

	// String
	// Receptacle is of type: const char *
	OPT_STRING,

	// List of strings
	// Receptacle is of type: std::vector< const char * >
	OPT_STRING_LIST
}
opt_type_t;


typedef struct
{
	const char *long_name;  // Command line arg. or keyword
	const char *short_name; // Abbreviated command line argument

	opt_type_t opt_type;    // Type of this option
	const char *flags;    // Flags for this option :
	// '1' : process only on pass 1 of parse_command_line_options()
	// 'f' : string is a filename
	// '<' : print extra newline after this option (when dumping)
	// 'v' : a real variable (preference setting)
	// 'w' : warp hack -- accept two numeric args

	const char *desc;   // Description of the option
	const char *arg_desc;  // Description of the argument (NULL --> none or default)

	void *data_ptr;   // Pointer to the data
}
opt_desc_t;


static const opt_desc_t options[] =
{
	//
	// A few options must be handled in an early pass
	//

	{	"home",
		0,
		OPT_STRING,
		"1",
		"Home directory",
		"<dir>",
		&config::home_dir
	},

	{	"install",
		0,
		OPT_STRING,
		"1",
		"Installation directory",
		"<dir>",
		&config::install_dir
	},

	{	"log",
		0,
		OPT_STRING,
		"1",
		"Log messages to specified file",
		"<file>",
		&config::log_file
	},

	{	"config",
		0,
		OPT_STRING,
		"1<",
		"Config file to load / save",
		"<file>",
		&config::config_file
	},

	{	"help",
		"h",
		OPT_BOOLEAN,
		"1",
		"Show usage summary",
		NULL,
		&config::show_help
	},

	{	"version",
		"v",
		OPT_BOOLEAN,
		"1",
		"Show the version",
		NULL,
		&config::show_version
	},

	{	"debug",
		"d",
		OPT_BOOLEAN,
		"1",
		"Enable debugging messages",
		NULL,
		&config::Debugging
	},

	{	"quiet",
		"q",
		OPT_BOOLEAN,
		"1",
		"Quiet mode (no messages on stdout)",
		NULL,
		&config::Quiet
	},

	//
	// Normal options from here on....
	//

	{	"file",
		"f",
		OPT_STRING_LIST,
		"",
		"Wad file(s) to edit",
		"<file>...",
		&config::Pwad_list
	},

	{	"merge",
		"m",
		OPT_STRING_LIST,
		"",
		"Resource file(s) to load",
		"<file>...",
		&config::Resource_list
	},

	{	"iwad",
		"i",
		OPT_STRING,
		"",
		"The name of the IWAD (game data)",
		"<file>",
		&config::Iwad_name
	},

	{	"port",
		"p",
		OPT_STRING,
		"",
		"Port (engine) name",
		"<name>",
		&config::Port_name
	},

	{	"warp",
		"w",
		OPT_STRING,
		"w<",
		"Select level to edit",
		"<map>",
		&config::Level_name
	},

	/* ------------ Preferences ------------ */

	{	"auto_load_recent",
		0,
		OPT_BOOLEAN,
		"v",
		"When no given files, load the most recent one saved",
		NULL,
		&config::auto_load_recent
	},

	{	"backup_max_files",
		0,
		OPT_INTEGER,
		"v",
		"Maximum copies to make when backing up a wad",
		NULL,
		&config::backup_max_files
	},

	{	"backup_max_space",
		0,
		OPT_INTEGER,
		"v",
		"Maximum space to use (in MB) when backing up a wad",
		NULL,
		&config::backup_max_space
	},

	{	"browser_small_tex",
		0,
		OPT_BOOLEAN,
		"v",
		"Show smaller (more compact) textures in the browser",
		NULL,
		&config::browser_small_tex
	},

	{	"bsp_fast",
		0,
		OPT_BOOLEAN,
		"v",
		"Node building: enable fast mode (may be lower quality)",
		NULL,
		&config::bsp_fast
	},

	{	"bsp_warnings",
		0,
		OPT_BOOLEAN,
		"v",
		"Node building: show all warning messages",
		NULL,
		&config::bsp_warnings
	},

	{	"bsp_split_factor",
		0,
		OPT_INTEGER,
		"v",
		"Node building: seg splitting factor",
		NULL,
		&config::bsp_split_factor
	},

	{	"bsp_gl_nodes",
		0,
		OPT_BOOLEAN,
		"v",
		"Node building: build GL-Nodes",
		NULL,
		&config::bsp_gl_nodes
	},

	{	"bsp_force_v5",
		0,
		OPT_BOOLEAN,
		"v",
		"Node building: force V5 of GL-Nodes",
		NULL,
		&config::bsp_force_v5
	},

	{	"bsp_force_zdoom",
		0,
		OPT_BOOLEAN,
		"v",
		"Node building: force ZDoom format for normal nodes",
		NULL,
		&config::bsp_force_zdoom
	},

	{	"bsp_compressed",
		0,
		OPT_BOOLEAN,
		"v",
		"Node building: force zlib compression of ZDoom nodes",
		NULL,
		&config::bsp_compressed
	},

	{	"default_gamma",
		0,
		OPT_INTEGER,
		"v",
		"Default gamma for images and 3D view (0..4)",
		NULL,
		&config::usegamma
	},

	{	"default_edit_mode",
		0,
		OPT_INTEGER,
		"v",
		"Default editing mode: 0..3 = Th / Lin / Sec / Vt",
		NULL,
		&config::default_edit_mode
	},

	{	"default_port",
		0,
		OPT_STRING,
		"v",
		"Default port (engine) name",
		NULL,
		&config::default_port
	},

	{	"dotty_axis_col",
		0,
		OPT_COLOR,
		"v",
		"axis color for the dotty style grid",
		NULL,
		&config::dotty_axis_col
	},

	{	"dotty_major_col",
		0,
		OPT_COLOR,
		"v",
		"major color for the dotty style grid",
		NULL,
		&config::dotty_major_col
	},

	{	"dotty_minor_col",
		0,
		OPT_COLOR,
		"v",
		"minor color for the dotty style grid",
		NULL,
		&config::dotty_minor_col
	},

	{	"dotty_point_col",
		0,
		OPT_COLOR,
		"v",
		"point color for the dotty style grid",
		NULL,
		&config::dotty_point_col
	},

	{	"floor_bump_small",
		0,
		OPT_INTEGER,
		"v",
		"distance for '+' and '-' buttons in sector panel while SHIFT is pressed",
		NULL,
		&config::floor_bump_small
	},

	{	"floor_bump_medium",
		0,
		OPT_INTEGER,
		"v",
		"distance for '+' and '-' buttons in sector panel without any modifier keys",
		NULL,
		&config::floor_bump_medium
	},

	{	"floor_bump_large",
		0,
		OPT_INTEGER,
		"v",
		"distance for '+' and '-' buttons in sector panel while CTRL is pressed",
		NULL,
		&config::floor_bump_large
	},

	{	"grid_default_mode",
		0,
		OPT_BOOLEAN,
		"v",
		"Default grid mode: 0 = OFF, 1 = dotty",
		NULL,
		&config::grid_default_mode
	},

	{	"grid_default_size",
		0,
		OPT_INTEGER,
		"v",
		"Default grid size",
		NULL,
		&config::grid_default_size
	},

	{	"grid_default_snap",
		0,
		OPT_BOOLEAN,
		"v",
		"Default grid snapping",
		NULL,
		&config::grid_default_snap
	},

	{	"grid_hide_in_free_mode",
		0,
		OPT_BOOLEAN,
		"v",
		"hide the grid in FREE mode",
		NULL,
		&config::grid_hide_in_free_mode
	},

	{	"grid_style",
		0,
		OPT_INTEGER,
		"v",
		"grid style : 0 = squares, 1 = dotty",
		NULL,
		&config::grid_style
	},

	{	"gui_theme",
		0,
		OPT_INTEGER,
		"v",
		"GUI widget theme: 0 = fltk, 1 = gtk+, 2 = plastic",
		NULL,
		&config::gui_scheme
	},

	{	"gui_color_set",
		0,
		OPT_INTEGER,
		"v",
		"GUI color set: 0 = fltk default, 1 = bright, 2 = custom",
		NULL,
		&config::gui_color_set
	},

	{	"gui_custom_bg",
		0,
		OPT_COLOR,
		"v",
		"GUI custom background color",
		NULL,
		&config::gui_custom_bg
	},

	{	"gui_custom_ig",
		0,
		OPT_COLOR,
		"v",
		"GUI custom input color",
		NULL,
		&config::gui_custom_ig
	},

	{	"gui_custom_fg",
		0,
		OPT_COLOR,
		"v",
		"GUI custom foreground (text) color",
		NULL,
		&config::gui_custom_fg
	},

	{	"leave_offsets_alone",
		0,
		OPT_BOOLEAN,
		"v",
		"Do not adjust offsets when splitting lines (etc)",
		NULL,
		&config::leave_offsets_alone
	},

	{	"light_bump_small",
		0,
		OPT_INTEGER,
		"v",
		"light step for '+' and '-' buttons in sector panel while SHIFT is pressed",
		NULL,
		&config::light_bump_small
	},

	{	"light_bump_medium",
		0,
		OPT_INTEGER,
		"v",
		"light step for '+' and '-' buttons in sector panel without any modifier keys",
		NULL,
		&config::light_bump_medium
	},

	{	"light_bump_large",
		0,
		OPT_INTEGER,
		"v",
		"light step for '+' and '-' buttons in sector panel while CTRL is pressed",
		NULL,
		&config::light_bump_large
	},

	{	"map_scroll_bars",
		0,
		OPT_BOOLEAN,
		"v",
		"Enable scroll-bars for the map view",
		NULL,
		&config::map_scroll_bars
	},

	{	"minimum_drag_pixels",
		0,
		OPT_INTEGER,
		"v",
		"Minimum distance to move mouse to drag an object (in pixels)",
		NULL,
		&config::minimum_drag_pixels
	},

	{	"new_islands_are_void",
		0,
		OPT_BOOLEAN,
		"v",
		"Islands created inside a sector will get a void interior",
		NULL,
		&config::new_islands_are_void
	},

	{	"new_sector_size",
		0,
		OPT_INTEGER,
		"v",
		"Size of sector rectangles created outside of the map",
		NULL,
		&config::new_sector_size
	},

	{	"normal_axis_col",
		0,
		OPT_COLOR,
		"v",
		"axis color for the normal grid",
		NULL,
		&config::normal_axis_col
	},

	{	"normal_main_col",
		0,
		OPT_COLOR,
		"v",
		"main color for the normal grid",
		NULL,
		&config::normal_main_col
	},

	{	"normal_flat_col",
		0,
		OPT_COLOR,
		"v",
		"flat color for the normal grid",
		NULL,
		&config::normal_flat_col
	},

	{	"normal_small_col",
		0,
		OPT_COLOR,
		"v",
		"small color for the normal grid",
		NULL,
		&config::normal_small_col
	},

	{	"panel_gamma",
		0,
		OPT_INTEGER,
		"v",
		"Gamma for images in the panels and the browser (0..4)",
		NULL,
		&config::panel_gamma
	},

	{	"render_pix_aspect",
		0,
		OPT_INTEGER,
		"v",
		"Aspect ratio of pixels for 3D view (100 * width / height)",
		NULL,
		&config::render_pixel_aspect
	},

	{	"render_high_detail",
		0,
		OPT_BOOLEAN,
		"v",
		"Use highest detail when rendering the 3D view",
		NULL,
		&config::render_high_detail
	},

	{	"render_lock_gravity",
		0,
		OPT_BOOLEAN,
		"v",
		"Locked gravity in 3D view -- cannot move up or down",
		NULL,
		&config::render_lock_gravity
	},

	{	"render_missing_bright",
		0,
		OPT_BOOLEAN,
		"v",
		"Render the missing texture as fullbright",
		NULL,
		&config::render_missing_bright
	},

	{	"render_unknown_bright",
		0,
		OPT_BOOLEAN,
		"v",
		"Render the unknown texture as fullbright",
		NULL,
		&config::render_unknown_bright
	},

	{	"same_mode_clears_selection",
		0,
		OPT_BOOLEAN,
		"v",
		"Clear the selection when entering the same mode",
		NULL,
		&config::same_mode_clears_selection
	},

	{	"sector_render_default",
		0,
		OPT_INTEGER,
		"v",
		"Default sector rendering mode: 0 = NONE, 1 = floor, 2 = ceiling",
		NULL,
		&config::sector_render_default
	},

	{	"thing_render_default",
		0,
		OPT_INTEGER,
		"v",
		"Default thing rendering mode: 0 = boxes, 1 = sprites",
		NULL,
		&config::thing_render_default
	},

	{	"transparent_col",
		0,
		OPT_COLOR,
		"v",
		"color used to represent transparent pixels in textures",
		NULL,
		&config::transparent_col
	},

	//
	// That's all there is
	//

	{	0,
		0,
		OPT_END,
		0,
		0,
		0,
		0
	}
};


//------------------------------------------------------------------------

// this automatically strips CR/LF from the line.
// returns true if ok, false on EOF or error.
bool M_ReadTextLine(char *buf, size_t size, FILE *fp)
{
	if (! fgets(buf, static_cast<int>(size), fp))
	{
		buf[0] = 0;
		return false;
	}

	// remove a Unicode BOM (byte-order mark)
	if ((byte)buf[0] == 0xEF &&
		(byte)buf[1] == 0xBB &&
		(byte)buf[2] == 0xBF)
	{
		size_t len = strlen(buf) - 3;

		memmove(buf, buf+3, len);

		buf[len] = 0;
	}

	StringRemoveCRLF(buf);

	return true;
}


static int parse_config_line_from_file(char *p, const char *basename, int lnum)
{
	char *name  = NULL;
	char *value = NULL;

	// skip leading whitespace
	while (isspace (*p))
		p++;

	// skip comments
	if (*p == '#')
		return 0;

	// remove trailing newline and whitespace
	{
		int len = (int)strlen(p);

		while (len > 0 && isspace(p[len - 1]))
		{
			p[len - 1] = 0;
			len--;
		}
	}

	// skip empty lines
	if (*p == 0)
		return 0;

	// grab the name
	name = p;
	while (y_isident (*p))
		p++;

	if (! isspace(*p))
	{
		LogPrintf("WARNING: %s(%u): bad line, no space after keyword.\n", basename, lnum);
		return 0;
	}

	*p++ = 0;

	// find the option value (occupies rest of the line)
	while (isspace(*p))
		p++;

	if (*p == 0)
	{
		LogPrintf("WARNING: %s(%u): bad line, missing option value.\n", basename, lnum);
		return 0;
	}

	value = p;

	// find the option keyword
	const opt_desc_t * opt;

	for (opt = options ; ; opt++)
	{
		if (opt->opt_type == OPT_END)
		{
			LogPrintf("WARNING: %s(%u): invalid option '%s', skipping\n",
					  basename, lnum, name);
			return 0;
		}

		if (! opt->long_name || strcmp(name, opt->long_name) != 0)
			continue;

		// pre-pass options (like --help) don't make sense in a config file
		if (strchr(opt->flags, '1'))
		{
			LogPrintf("WARNING: %s(%u): cannot use option '%s' in config files.\n",
			          basename, lnum, name);
			return 0;
		}

		// found it
		break;
	}

	switch (opt->opt_type)
	{
		case OPT_BOOLEAN:
			if (y_stricmp(value, "no")    == 0 ||
				y_stricmp(value, "false") == 0 ||
				y_stricmp(value, "off")   == 0 ||
				y_stricmp(value, "0")     == 0)
			{
				*((bool *) (opt->data_ptr)) = false;
			}
			else  // anything else is TRUE
			{
				*((bool *) (opt->data_ptr)) = true;
			}
			break;

		case OPT_INTEGER:
			*((int *) opt->data_ptr) = atoi(value);
			break;

		case OPT_COLOR:
			*((rgb_color_t *) opt->data_ptr) = ParseColor(value);
			break;

		case OPT_STRING:
			// handle empty string
			if (strcmp(value, "''") == 0)
				*value = 0;

			*static_cast<std::string *>(opt->data_ptr) = value;
			break;

		case OPT_STRING_LIST:
			while (*value != 0)
			{
				char *v = value;
				while (*v != 0 && ! isspace ((unsigned char) *v))
					v++;

				string_list_t * list = (string_list_t *)opt->data_ptr;

				list->push_back(std::string(value, (int)(v - value)));

				while (isspace (*v))
					v++;
				value = v;
			}
			break;

		default:
			BugError("INTERNAL ERROR: unknown option type %d\n", (int) opt->opt_type);
			return -1;
	}

	return 0;  // OK
}


//
//  try to parse a config file by pathname.
//
//  Return 0 on success, negative value on failure.
//
static int parse_a_config_file(FILE *fp, const char *filename)
{
	static char line[1024];

	const char *basename = FindBaseName(filename);

	// handle one line on each iteration
	for (int lnum = 1 ; M_ReadTextLine(line, sizeof(line), fp) ; lnum++)
	{
		int ret = parse_config_line_from_file(line, basename, lnum);

		if (ret != 0)
			return ret;
	}

	return 0;  // OK
}


inline static std::string default_config_file()
{
	SYS_ASSERT(!config::home_dir.empty());
	return config::home_dir + "/config.cfg";
}


//
//  parses the config file (either a user-specific one or the default one).
//
//  return 0 on success, negative value on error.
//
int M_ParseConfigFile()
{
	if (config::config_file.empty())
	{
		config::config_file = default_config_file();
	}

	FILE * fp = FileOpen(config::config_file.c_str(), "r");

	LogPrintf("Reading config file: %s\n", config::config_file.c_str());

	if (fp == NULL)
	{
		LogPrintf("--> %s\n", strerror(errno));
		return -1;
	}

	int rc = parse_a_config_file(fp, config::config_file.c_str());

	fclose(fp);

	return rc;
}


int M_ParseDefaultConfigFile()
{
	static char filename[FL_PATH_MAX];

	snprintf(filename, sizeof(filename), "%s/defaults.cfg", config::install_dir.c_str());

	FILE * fp = FileOpen(filename, "r");

	LogPrintf("Reading config file: %s\n", filename);

	if (fp == NULL)
	{
		LogPrintf("--> %s\n", strerror(errno));
		return -1;
	}

	int rc = parse_a_config_file(fp, filename);

	fclose(fp);

	return rc;
}


//
// check certain environment variables...
//
void M_ParseEnvironmentVars()
{
#if 0
	char *value;

	value = getenv ("EUREKA_GAME");
	if (value != NULL)
		Game = value;
#endif
}


void M_AddPwadName(const char *filename)
{
	config::Pwad_list.push_back(filename);
}


//
// parses the command line options
//
// If <pass> is set to 1, ignores all options except those
// that have the "1" flag (the "early" options).
//
// Otherwise, ignores all options that have the "1" flag.
//
void M_ParseCommandLine(int argc, const char *const *argv, int pass)
{
	const opt_desc_t *o;

	while (argc > 0)
	{
		bool ignore;

		// is it actually an option?
		if (argv[0][0] != '-')
		{
			// this is a loose file, handle it now
			if (pass != 1)
				M_AddPwadName(argv[0]);

			argv++;
			argc--;
			continue;
		}

		// Which option is this?
		for (o = options; ; o++)
		{
			if (o->opt_type == OPT_END)
			{
				FatalError("unknown option: '%s'\n", argv[0]);
				/* NOT REACHED */
			}

			if ( (o->short_name && strcmp (argv[0]+1, o->short_name) == 0) ||
				 (o->long_name  && strcmp (argv[0]+1, o->long_name ) == 0) ||
				 (o->long_name  && argv[0][1] == '-' && strcmp (argv[0]+2, o->long_name ) == 0) )
				break;
		}

		// ignore options which are not meant for this pass
		ignore = (strchr(o->flags, '1') != NULL) != (pass == 1);

		switch (o->opt_type)
		{
			case OPT_BOOLEAN:
				// -AJA- permit a following value
				if (argc >= 2 && argv[1][0] != '-')
				{
					argv++;
					argc--;

					if (ignore)
						break;

					if (y_stricmp(argv[0], "no")    == 0 ||
					    y_stricmp(argv[0], "false") == 0 ||
						y_stricmp(argv[0], "off")   == 0 ||
						y_stricmp(argv[0], "0")     == 0)
					{
						*((bool *) (o->data_ptr)) = false;
					}
					else  // anything else is TRUE
					{
						*((bool *) (o->data_ptr)) = true;
					}
				}
				else if (! ignore)
				{
					*((bool *) o->data_ptr) = true;
				}
				break;

			case OPT_INTEGER:
				if (argc < 2)
				{
					FatalError("missing argument after '%s'\n", argv[0]);
					/* NOT REACHED */
				}

				argv++;
				argc--;

				if (! ignore)
				{
					*((int *) o->data_ptr) = atoi(argv[0]);
				}
				break;

			case OPT_COLOR:
				if (argc < 2)
				{
					FatalError("missing argument after '%s'\n", argv[0]);
					/* NOT REACHED */
				}

				argv++;
				argc--;

				if (! ignore)
				{
					*((rgb_color_t *) o->data_ptr) = ParseColor(argv[0]);
				}
				break;

			case OPT_STRING:
				if (argc < 2)
				{
					FatalError("missing argument after '%s'\n", argv[0]);
					/* NOT REACHED */
				}

				argv++;
				argc--;

				if (! ignore)
				{
					*static_cast<std::string *>(o->data_ptr) = argv[0];
				}

				// support two numeric values after -warp
				if (strchr(o->flags, 'w') && isdigit(argv[0][0]) &&
					argc > 1 && isdigit(argv[1][0]))
				{
					if (! ignore)
					{
						*static_cast<std::string *>(o->data_ptr) = StringPrintf("%s%s", argv[0], argv[1]);
					}

					argv++;
					argc--;
				}

				break;

			case OPT_STRING_LIST:
				if (argc < 2)
				{
					FatalError("missing argument after '%s'\n", argv[0]);
					/* NOT REACHED */
				}
				while (argc > 1 && argv[1][0] != '-' && argv[1][0] != '+')
				{
					argv++;
					argc--;

					if (! ignore)
					{
						string_list_t * list = (string_list_t *) o->data_ptr;
						list->push_back(argv[0]);
					}
				}
				break;

			default:
				BugError("INTERNAL ERROR: unknown option type (%d)\n", (int) o->opt_type);
				/* NOT REACHED */
		}

		argv++;
		argc--;
	}
}


//
// print a list of all command line options (usage message).
//
void M_PrintCommandLineOptions(FILE *fp)
{
	const opt_desc_t *o;
	int name_maxlen = 0;
	int  arg_maxlen = 0;

	for (o = options; o->opt_type != OPT_END; o++)
	{
		int len;

		if (strchr(o->flags, 'v'))
			continue;

		if (o->long_name)
		{
			len = (int)strlen (o->long_name);
			name_maxlen = MAX(name_maxlen, len);
		}

		if (o->arg_desc)
		{
			len = (int)strlen (o->arg_desc);
			arg_maxlen = MAX(arg_maxlen, len);
		}
	}

	for (int pass = 0 ; pass < 2 ; pass++)
	for (o = options; o->opt_type != OPT_END; o++)
	{
		if (strchr(o->flags, 'v'))
			continue;

		if ((strchr(o->flags, '1') ? 1 : 0) != pass)
			continue;

		if (o->short_name)
			fprintf (fp, "  -%-3s ", o->short_name);
		else
			fprintf (fp, "       ");

		if (o->long_name)
			fprintf (fp, "--%-*s   ", name_maxlen, o->long_name);
		else
			fprintf (fp, "%*s  ", name_maxlen + 2, "");

		if (o->arg_desc)
			fprintf (fp, "%-12s", o->arg_desc);
		else switch (o->opt_type)
		{
			case OPT_BOOLEAN:       fprintf (fp, "            "); break;
			case OPT_INTEGER:       fprintf (fp, "<value>     "); break;
			case OPT_COLOR:         fprintf (fp, "<color>     "); break;

			case OPT_STRING:        fprintf (fp, "<string>    "); break;
			case OPT_STRING_LIST:   fprintf (fp, "<string> ..."); break;
			case OPT_END: ;  // This line is here only to silence a GCC warning.
		}

		fprintf (fp, " %s\n", o->desc);

		if (strchr(o->flags, '<'))
			fprintf (fp, "\n");
	}
}


int M_WriteConfigFile()
{
	SYS_ASSERT(!config::config_file.empty());

	LogPrintf("Writing config file: %s\n", config::config_file.c_str());

	FILE * fp = FileOpen(config::config_file.c_str(), "w");

	if (! fp)
	{
		LogPrintf("--> %s\n", strerror(errno));
		return -1;
	}

	fprintf(fp, "# Eureka configuration (local)\n");

	const opt_desc_t *o;

	for (o = options; o->opt_type != OPT_END; o++)
	{
		if (! strchr(o->flags, 'v'))
			continue;

		if (! o->long_name)
			continue;

		fprintf(fp, "%s ", o->long_name);

		switch (o->opt_type)
		{
			case OPT_BOOLEAN:
				fprintf(fp, "%s", *((bool *) o->data_ptr) ? "1" : "0");
				break;

			case OPT_STRING:
			{
				const std::string *str = static_cast<std::string *>(o->data_ptr);
				fprintf(fp, "%s", !str->empty() ? str->c_str() : "''");
				break;
			}

			case OPT_INTEGER:
				fprintf(fp, "%d", *((int *) o->data_ptr));
				break;

			case OPT_COLOR:
				fprintf(fp, "%06x", *((rgb_color_t *) o->data_ptr) >> 8);
				break;

			case OPT_STRING_LIST:
			{
				string_list_t *list = (string_list_t *)o->data_ptr;

				if (list->empty())
					fprintf(fp, "{}");
				else for (unsigned int i = 0 ; i < list->size() ; i++)
					fprintf(fp, "%s ", list->at(i).c_str());
			}

			default:
				break;
		}

		fprintf(fp, "\n");
	}

	fflush(fp);
	fclose(fp);

	return 0;  // OK
}


//------------------------------------------------------------------------
//   USER STATE HANDLING
//------------------------------------------------------------------------


int M_ParseLine(const char *line, std::vector<std::string> &tokens, bool do_strings)
{
	char tokenbuf[256];
	int  tokenlen = -1;

	bool in_string = false;

	// skip leading whitespace
	while (isspace(*line))
		line++;

	// blank line or comment line?
	if (*line == 0 || *line == '\n' || *line == '#')
		return 0;

	for (;;)
	{
		if (tokenlen > 250)  // ERROR: token too long
			return -1;

		int ch = *line++;

		if (tokenlen < 0)  // looking for a new token
		{
			SYS_ASSERT(!in_string);

			// end of line?  if yes, break out of for loop
			if (ch == 0 || ch == '\n')
				break;

			if (isspace(ch))
				continue;

			tokenlen = 0;

			// begin a string?
			if (ch == '"' && do_strings)
			{
				in_string = true;
				continue;
			}

			// begin a normal token
			tokenbuf[tokenlen++] = ch;
			continue;
		}

		if (in_string && ch == '"')
		{
			// end of string
			in_string = false;
		}
		else if (ch == 0 || ch == '\n')
		{
			// end of line
		}
		else if (! in_string && isspace(ch))
		{
			// end of token
		}
		else
		{
			tokenbuf[tokenlen++] = ch;
			continue;
		}

		if (in_string)  // ERROR: non-terminated string
			return -3;

		tokenbuf[tokenlen] = 0;
		tokenlen = -1;

		tokens.push_back(tokenbuf);

		// end of line?  if yes, we are done
		if (ch == 0 || ch == '\n')
			break;
	}

	return static_cast<int>(tokens.size());
}

char * PersistFilename(const crc32_c& crc)
{
	static char filename[FL_PATH_MAX];

	snprintf(filename, sizeof(filename), "%s/cache/%08X%08X.dat", cache_dir.c_str(),
	        crc.extra, crc.raw);

	return filename;
}


#define MAX_TOKENS  10


bool M_LoadUserState()
{
	crc32_c crc;

	BA_LevelChecksum(crc);

	char *filename = PersistFilename(crc);

	FILE *fp = FileOpen(filename, "r");

	if (! fp)
		return false;

	LogPrintf("Loading user state from: %s\n", filename);


	static char line[FL_PATH_MAX];

	std::vector<std::string> tokens;
	tokens.reserve(MAX_TOKENS);

	while (M_ReadTextLine(line, sizeof(line), fp))
	{
		tokens.clear();
		int num_tok = M_ParseLine(line, tokens, true /* do_strings */);

		if (num_tok == 0)
			continue;

		if (num_tok < 0)
		{
			LogPrintf("Error in persistent data: %s\n", line);
			continue;
		}

		if (  Editor_ParseUser(tokens) ||
		        Grid_ParseUser(tokens) ||
		    Render3D_ParseUser(tokens) ||
		     Browser_ParseUser(tokens) ||
		       Props_ParseUser(tokens) ||
		     RecUsed_ParseUser(tokens))
		{
			// Ok
		}
		else
		{
			LogPrintf("Unknown persistent data: %s\n", line);
		}
	}

	fclose(fp);

	Props_LoadValues();

	return true;
}


bool M_SaveUserState()
{
	crc32_c crc;

	BA_LevelChecksum(crc);


	char *filename = PersistFilename(crc);

	LogPrintf("Save user state to: %s\n", filename);

	FILE *fp = FileOpen(filename, "w");

	if (! fp)
	{
		LogPrintf("--> FAILED! (%s)\n", strerror(errno));
		return false;
	}

	  Editor_WriteUser(fp);
	    Grid_WriteUser(fp);
	Render3D_WriteUser(fp);
	 Browser_WriteUser(fp);
	   Props_WriteUser(fp);
	 RecUsed_WriteUser(fp);

	fclose(fp);

	return true;
}


void M_DefaultUserState()
{
	grid.Init();

	ZoomWholeMap();

	Render3D_Setup();

	Editor_DefaultState();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
