//------------------------------------------------------------------------
//  CONFIG FILE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2012 Andrew Apted
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
#include "editloop.h"
#include "m_config.h"
#include "r_misc.h"
#include "r_grid.h"
#include "r_render.h"
#include "levels.h"

#include "ui_window.h"  // meh!


/*
 *  Description of the command line arguments and config file keywords
 */
typedef enum
{
	// Boolean (toggle)
	// Receptacle is of type bool
	// data_ptr is of type (bool *)
	OPT_BOOLEAN,

	// "yes", "no", "ask"
	// Receptacle is of type confirm_t
	// data_ptr is of type confirm_t
	OPT_CONFIRM,

	// Integer number,
	// Receptacle is of type int
	// data_ptr is of type (int *)
	OPT_INTEGER,

	// Unsigned long integer
	// Receptacle is of type unsigned
	// data_ptr is of type (unsigned long *)
	OPT_UNSIGNED,

	// String
	// Receptacle is of type (char[9])
	// data_ptr is of type (char *)
	OPT_STRINGBUF8,

	// String
	// Receptacle is of type (const char *)
	// data_ptr is of type (const char **)
	OPT_STRINGPTR,

	// String, but store in a list
	// Receptacle is of type ??
	// data_ptr is of type ??
	OPT_STRINGPTRACC,

	// List of strings
	// Receptacle is of type (const char *[])
	// data_ptr is of type (const char ***)
	OPT_STRINGPTRLIST,

	// End of the options description
	OPT_END
}
opt_type_t;


typedef struct
{
	const char *long_name;  // Command line arg. or keyword
	const char *short_name; // Abbreviated command line argument
	opt_type_t opt_type;    // Type of this option
	const char *flags;    // Flags for this option :
	// "1" = process only on pass 1 of
	//       parse_command_line_options()
	const char *desc;   // Description of the option
	void *data_ptr;   // Pointer to the data
}
opt_desc_t;


static const opt_desc_t options[] =
{
	//
	// A few options must be handled in an early pass
	//

	{	"help",
		"h",
		OPT_BOOLEAN,
		"1",
		"Show usage summary",
		&show_help
	},

	{	"version",
		0,
		OPT_BOOLEAN,
		"1",
		"Show version info",
		&show_version
	},

	{	"config",
		0,
		OPT_STRINGPTR,
		"1",
		"Config file",
		&config_file
	},

	{	"home",
		0,
		OPT_STRINGPTR,
		"1",
		"Home directory",
		&home_dir
	},

	{	"install",
		0,
		OPT_STRINGPTR,
		"1",
		"Installation directory",
		&install_dir
	},

	{	"log",
		0,
		OPT_STRINGPTR,
		"1",
		"Log messages to file (instead of stdout)",
		&log_file
	},

	{	"quiet",
		"q",
		OPT_BOOLEAN,
		"1",
		"Quiet mode (no messages on stdout)",
		&Quiet
	},

	{	"debug",
		"d",
		OPT_BOOLEAN,
		"1",
		"Enable debugging messages",
		&Debugging
	},

	//
	// Normal options from here on....
	//

	{	"file",
		"f",
		OPT_STRINGPTR,
		0,
		"Patch wad file",
		&Pwad
	},

	{	"iwad",
		"i",
		OPT_STRINGPTR,
		0,
		"The name of the IWAD",
		&Iwad
	},

	{	"port",
		"p",
		OPT_STRINGPTR,
		0,
		"Port (engine) name",
		&Port_name
	},

	{	"warp",
		"w",
		OPT_STRINGPTR,
		0,
		"Warp map",
		&Level_name
	},

// TODO
#if 0
	{	"merge",
		0,
		OPT_STRINGPTRACC,
		0,
		"Resource file to load",
		&PatchWads
	},

	{	"mod",
		0,
		OPT_STRINGPTR,
		0,
		"Mod name",
		&Mod_name
	},
#endif

	{	"copy_linedef_reuse_sidedefs",
		0,
		OPT_BOOLEAN,
		0,
		"Use same sidedefs as original linedef",
		&copy_linedef_reuse_sidedefs
	},

	{	"default_ceiling_height",
		0,
		OPT_INTEGER,
		0,
		"Default ceiling height",
		&default_ceiling_height
	},

	{	"default_ceiling_texture",
		0,
		OPT_STRINGBUF8,
		0,
		"Default ceiling texture",
		default_ceiling_texture
	},

	{	"default_floor_height",
		0,
		OPT_INTEGER,
		0,
		"Default floor height",
		&default_floor_height
	},

	{	"default_floor_texture",
		0,
		OPT_STRINGBUF8,
		0,
		"Default floor texture",
		default_floor_texture
	},

	{	"default_light_level",
		0,
		OPT_INTEGER,
		0,
		"Default light level",
		&default_light_level
	},

	{	"default_lower_texture",
		0,
		OPT_STRINGBUF8,
		0,
		"Default lower texture",
		default_lower_texture
	},

	{	"default_middle_texture",
		0,
		OPT_STRINGBUF8,
		0,
		"Default middle texture",
		default_middle_texture
	},

	{	"default_upper_texture",
		0,
		OPT_STRINGBUF8,
		0,
		"Default upper texture",
		default_upper_texture
	},

	{	"scroll_less",
		0,
		OPT_UNSIGNED,
		0,
		"Amp. of scrolling (% of screen size)",
		&scroll_less
	},

	{	"scroll_more",
		0,
		OPT_UNSIGNED,
		0,
		"Amp. of scrolling (% of screen size)",
		&scroll_more
	},

	{	"sprite_scale",
		0,
		OPT_INTEGER,
		0,
		"Relative scale of sprites",
		&sprite_scale
	},

// -AJA- this may be better as a 'mouse_button_order' string,
//       where the default is "123", and user could have "321"
//       or "132" or any other combination.
#if 0
	{	"swap_buttons",
		0,
		OPT_BOOLEAN,
		0,
		"Swap mouse buttons",
		&swap_buttons
	},
#endif

	//
	// That's all there is
	//

	{	0,
		0,
		OPT_END,
		0,
		0,
		0
	}
};


static void append_item_to_list (const char ***list, const char *item);
static confirm_t confirm_e2i (const char *external);
static const char *confirm_i2e (confirm_t internal);


/*
 *  parse_config_file - try to parse a config file by pathname.
 *
 *  Return 0 on success, negative value on failure.
 */
static int parse_config_file(FILE *fp, const char *filename)
{
	char line[1024];

	// Execute one line on each iteration
	for (unsigned lnum = 1; fgets (line, sizeof line, fp) != NULL; lnum++)
	{
		char *name  = 0;
		char *value = 0;
		char *p     = line;

		// Skip leading whitespace
		while (isspace (*p))
			p++;

		// Skip comments
		if (*p == '#')
			continue;

		// Remove trailing newline
		{
			size_t len = strlen (p);
			if (len >= 1 && p[len - 1] == '\n')
				p[len - 1] = '\0';
		}

		// Skip empty lines
		if (*p == '\0')
			continue;

		// Make <name> point on the <name> field
		name = p;
		while (y_isident (*p))
			p++;
		if (*p == '\0')
		{
			warn ("%s(%u): expected an \"=\", skipping\n", filename, lnum);
			goto next_line;
		}
		if (*p == '=')
		{
			// Mark the end of the option name
			*p = '\0';
		}
		else
		{
			// Mark the end of the option name
			*p = '\0';
			p++;
			// Skip blanks after the option name
			while (isspace ((unsigned char) *p))
				p++;
			if (*p != '=')
			{
				warn ("%s(%u,%d): expected an \"=\", skipping\n",
						filename, lnum, 1 + (int) (p - line));
				goto next_line;
			}
		}
		p++;

		/* First parameter : <value> points on the first character.
		   Put a NUL at the end. If there is a second parameter,
		   holler. */
		while (isspace ((unsigned char) *p))
			p++;
		value = p;
		{
			unsigned char *p2 = (unsigned char *) value;
			while (*p2 != '\0' && ! isspace (*p2))
				p2++;
			if (*p2 != '\0')  // There's trailing whitespace after 1st parameter
			{
				for (unsigned char *p3 = p2; *p3 != '\0'; p3++)
					if (! isspace (*p3))
					{
						err ("%s(%u,%d): extraneous argument",
								filename, lnum, 1 + (int) ((char *) p3 - line));
						return -1;
					}
			}
			*p2 = '\0';
		}

		for (const opt_desc_t *o = options; ; o++)
		{
			if (o->opt_type == OPT_END)
			{
				warn ("%s(%u): invalid variable \"%s\", skipping\n",
						filename, lnum, name);
				goto next_line;
			}
			if (! o->long_name || strcmp (name, o->long_name) != 0)
				continue;

			if (o->flags != NULL && strchr (o->flags, '1'))
				break;
			switch (o->opt_type)
			{
				case OPT_BOOLEAN:
					if (! strcmp (value, "yes") || ! strcmp (value, "true")
							|| ! strcmp (value, "on") || ! strcmp (value, "1"))
					{
						if (o->data_ptr)
							*((bool *) (o->data_ptr)) = true;
					}
					else if (! strcmp (value, "no") || ! strcmp (value, "false")
							|| ! strcmp (value, "off") || ! strcmp (value, "0"))
					{
						if (o->data_ptr)
							*((bool *) (o->data_ptr)) = false;
					}
					else
					{
						err ("%s(%u): invalid value for option %s: \"%s\"",
								filename, lnum, name, value);
						return -1;
					}
					break;

				case OPT_CONFIRM:
					if (o->data_ptr)
						*((confirm_t *) o->data_ptr) = confirm_e2i (value);
					break;

				case OPT_INTEGER:
					if (o->data_ptr)
						*((int *) (o->data_ptr)) = atoi (value);
					break;

				case OPT_UNSIGNED:
					if (o->data_ptr)
					{
						if (*value == '\0')
						{
							err ("%s(%u,%d): missing argument",
									filename, lnum, 1 + (int) (value - line));
							return -1;
						}
						bool neg = false;
						if (value[0] == '-')
							neg = true;
						char *endptr;
						errno = 0;
						*((unsigned long *) (o->data_ptr)) = strtoul (value, &endptr, 0);
						if (*endptr != '\0' && ! isspace (*endptr))
						{
							err ("%s(%u,%d): illegal character in unsigned integer",
									filename, lnum, 1 + (int) (endptr - line));
							return -1;
						}
						/* strtoul() sets errno to ERANGE if overflow. In
						   addition, we don't want any non-zero negative
						   numbers. In terms of regexp, /^(0x)?0*$/i. */
						if
							(
							 errno != 0
							 || neg
							 && !
							 (
							  strspn (value + 1, "0") == strlen (value + 1)
							  || value[1] == '0'
							  && tolower (value[2]) == 'x'
							  && strspn (value + 3, "0") == strlen (value + 3)
							 )
							)
							{
								err ("%s(%u,%d): unsigned integer out of range",
										filename, lnum, 1 + (int) (value - line));
								return -1;
							}
					}
					break;

				case OPT_STRINGBUF8:
					if (o->data_ptr)
						strncpy ((char *) o->data_ptr, value, 8);
					((char *) o->data_ptr)[8] = 0;
					break;

				case OPT_STRINGPTR:
					{
						char *dup = (char *) GetMemory (strlen (value) + 1);
						strcpy (dup, value);
						if (o->data_ptr)
							*((char **) (o->data_ptr)) = dup;
						break;
					}

				case OPT_STRINGPTRACC:
					{
						char *dup = (char *) GetMemory (strlen (value) + 1);
						strcpy (dup, value);
						if (o->data_ptr)
							append_item_to_list ((const char ***) o->data_ptr, dup);
						break;
					}

				case OPT_STRINGPTRLIST:
					while (*value != '\0')
					{
						char *v = value;
						while (*v != '\0' && ! isspace ((unsigned char) *v))
							v++;
						char *dup = (char *) GetMemory (v - value + 1);
						memcpy (dup, value, v - value);
						dup[v - value] = '\0';
						if (o->data_ptr)
							append_item_to_list ((const char ***) o->data_ptr, dup);
						while (isspace (*v))
							v++;
						value = v;
					}
					break;

				default:
					{
						BugError("%s(%u): unknown option type %d",
								filename, lnum, (int) o->opt_type);
						return -1;
					}
			}
			break;
		}
next_line:;
	}

	return 0;  // OK
}


/*
 *  parse_config_file_user - parse a user-specified config file
 *
 *  Return 0 on success, negative value on error.
 */
int parse_config_file_user(const char *filename)
{
	FILE * fp = fopen(filename, "r");

	if (fp == NULL)
	{
		err ("Cannot open config file \"%s\" (%s)", filename, strerror(errno));
		return -1;
	}

	LogPrintf("Reading config file: %s\n", filename);

	int rc = parse_config_file(fp, filename);

	fclose(fp);

	return rc;
}


/*
 *  parse_config_file_default - parse the default config file(s)
 *
 *  Return 0 on success, negative value on failure.
 *  It is OK if this file does not exist.
 */
int parse_config_file_default()
{
	char filename[FL_PATH_MAX];

	if (! home_dir)
		return 0;

	sprintf(filename, "%s/config.cfg", home_dir);

	if (! FileExists(filename))
		return 0;

	return parse_config_file_user(filename);
}


/*
 *  parse_command_line_options
 *  If <pass> is set to 1, ignores all options except those
 *  that have the "1" flag.
 *  Else, ignores all options that have the "1" flag.
 *  If an error occurs, report it with err()
 *  and returns non-zero. Else, returns 0.
 */
int parse_command_line_options (int argc, const char *const *argv, int pass)
{
	const opt_desc_t *o;

	while (argc > 0)
	{
		int ignore;

		// Which option is this ?
		if (argv[0][0] != '-')
		{
			// this is a loose file, handle it now
			Pwad = StringDup(argv[0]);

			argc++;
			argv--;
			continue;
		}

		{
			for (o = options; ; o++)
			{
				if (o->opt_type == OPT_END)
				{
					err ("invalid option: \"%s\"", argv[0]);
					return 1;
				}
				if ( (o->short_name && strcmp (argv[0]+1, o->short_name) == 0) ||
					 (o->long_name  && strcmp (argv[0]+1, o->long_name ) == 0) ||
					 (o->long_name  && argv[0][1] == '-' && strcmp (argv[0]+2, o->long_name ) == 0) )
					break;
			}
		}

		// If this option has the "1" flag but pass is not 1
		// or it doesn't but pass is 1, ignore it.
		ignore = (o->flags != NULL && strchr (o->flags, '1')) != (pass == 1);

		switch (o->opt_type)
		{
			case OPT_BOOLEAN:
				if (argv[0][0] == '-')
				{
					if (o->data_ptr && ! ignore)
						*((bool *) (o->data_ptr)) = true;
				}
				else
				{
					if (o->data_ptr && ! ignore)
						*((bool *) (o->data_ptr)) = false;
				}
				break;

			case OPT_CONFIRM:
				if (argc <= 1)
				{
					err ("missing argument after \"%s\"", argv[0]);
					return 1;
				}
				argv++;
				argc--;
				if (o->data_ptr && ! ignore)
					*((confirm_t *) o->data_ptr) = confirm_e2i (argv[0]);
				break;

			case OPT_INTEGER:
				if (argc <= 1)
				{
					err ("missing argument after \"%s\"", argv[0]);
					return 1;
				}
				argv++;
				argc--;
				if (o->data_ptr && ! ignore)
					*((int *) (o->data_ptr)) = atoi (argv[0]);
				break;

			case OPT_UNSIGNED:
				if (argc <= 1)
				{
					err ("missing argument after \"%s\"", argv[0]);
					return 1;
				}
				argv++;
				argc--;
				if (o->data_ptr && ! ignore)
				{
					const char *value = argv[0];
					if (*value == '\0')
					{
						err ("not an unsigned integer \"%s\"", value);
						return 1;
					}
					bool neg = false;
					if (*value == '-')
						neg = true;
					char *endptr;
					errno = 0;
					*((unsigned long *) (o->data_ptr)) = strtoul (value, &endptr, 0);
					while (*endptr != '\0' && isspace (*endptr))
						endptr++;
					if (*endptr != '\0')
					{
						err ("illegal characters in unsigned int \"%s\"", endptr);
						return 1;
					}
					/* strtoul() sets errno to ERANGE if overflow. In
					   addition, we don't want any non-zero negative
					   numbers. In terms of regexp, /^(0x)?0*$/i. */
					if
						(
						 errno != 0
						 || neg
						 && !
						 (
						  strspn (value + 1, "0") == strlen (value + 1)
						  || value[1] == '0'
						  && tolower (value[2]) == 'x'
						  && strspn (value + 3, "0") == strlen (value + 3)
						 )
						)
						{
							err ("unsigned integer out of range \"%s\"", value);
							return 1;
						}
				}
				break;

			case OPT_STRINGBUF8:
				if (argc <= 1)
				{
					err ("missing argument after \"%s\"", argv[0]);
					return 1;
				}
				argv++;
				argc--;
				if (o->data_ptr && ! ignore)
					strncpy ((char *) o->data_ptr, argv[0], 8);
				((char *) o->data_ptr)[8] = 0;
				break;

			case OPT_STRINGPTR:
				if (argc <= 1)
				{
					err ("missing argument after \"%s\"", argv[0]);
					return 1;
				}
				argv++;
				argc--;
				if (o->data_ptr && ! ignore)
					*((const char **) (o->data_ptr)) = argv[0];
				break;

			case OPT_STRINGPTRACC:
				if (argc <= 1)
				{
					err ("missing argument after \"%s\"", argv[0]);
					return 1;
				}
				argv++;
				argc--;
				if (o->data_ptr && ! ignore)
					append_item_to_list ((const char ***) o->data_ptr, argv[0]);
				break;

			case OPT_STRINGPTRLIST:
				if (argc <= 1)
				{
					err ("missing argument after \"%s\"", argv[0]);
					return 1;
				}
				while (argc > 1 && argv[1][0] != '-' && argv[1][0] != '+')
				{
					argv++;
					argc--;
					if (o->data_ptr && ! ignore)
						append_item_to_list ((const char ***) o->data_ptr, argv[0]);
				}
				break;

			default:
				{
					BugError("unknown option type (%d)", (int) o->opt_type);
					return 1;
				}
		}

		argv++;
		argc--;
	}
	return 0;
}


/*
 *  dump_parameters
 *  Print a list of the parameters with their current value.
 */
void dump_parameters(FILE *fp)
{
	const opt_desc_t *o;
	int desc_maxlen = 0;
	int name_maxlen = 0;

	for (o = options; o->opt_type != OPT_END; o++)
	{
		int len = strlen (o->desc);
		desc_maxlen = MAX(desc_maxlen, len);
		if (o->long_name)
		{
			len = strlen (o->long_name);
			name_maxlen = MAX(name_maxlen, len);
		}
	}

	for (o = options; o->opt_type != OPT_END; o++)
	{
		if (! o->long_name)
			continue;
		fprintf (fp, "%-*s  %-*s  ",name_maxlen, o->long_name, desc_maxlen, o->desc);
		if (o->opt_type == OPT_BOOLEAN)
			fprintf (fp, "%s", *((bool *) o->data_ptr) ? "enabled" : "disabled");
		else if (o->opt_type == OPT_CONFIRM)
			fputs (confirm_i2e (*((confirm_t *) o->data_ptr)), fp);
		else if (o->opt_type == OPT_STRINGBUF8)
			fprintf (fp, "\"%s\"", (char *) o->data_ptr);
		else if (o->opt_type == OPT_STRINGPTR)
		{
			if (o->data_ptr)
				fprintf (fp, "\"%s\"", *((char **) o->data_ptr));
			else
				fprintf (fp, "--none--");
		}
		else if (o->opt_type == OPT_INTEGER)
			fprintf (fp, "%d", *((int *) o->data_ptr));
		else if (o->opt_type == OPT_UNSIGNED)
			fprintf (fp, "%lu", *((unsigned long *) o->data_ptr));
		else if (o->opt_type == OPT_STRINGPTRACC
				|| o->opt_type == OPT_STRINGPTRLIST)
		{
			if (o->data_ptr)
			{
				char **list;
				for (list = *((char ***) o->data_ptr); list && *list; list++)
					fprintf (fp, "\"%s\" ", *list);
				if (list == *((char ***) o->data_ptr))
					fprintf (fp, "--none--");
			}
			else
				fprintf (fp, "--none--");
		}
		fputc ('\n', fp);
	}
}


/*
 *  dump_command_line_options
 *  Print a list of all command line options (usage message).
 */
void dump_command_line_options(FILE *fp)
{
	const opt_desc_t *o;
	int desc_maxlen = 0;
	int name_maxlen = 0;

	for (o = options; o->opt_type != OPT_END; o++)
	{
		int len;
		if (! o->short_name)
			continue;
		len = strlen (o->desc);
		desc_maxlen = MAX(desc_maxlen, len);
		if (o->long_name)
		{
			len = strlen (o->long_name);
			name_maxlen = MAX(name_maxlen, len);
		}
	}

	for (o = options; o->opt_type != OPT_END; o++)
	{
		if (! o->short_name)
			continue;
		if (o->short_name)
			fprintf (fp, " -%-3s ", o->short_name);
		else
			fprintf (fp, "      ");
		if (o->long_name)
			fprintf (fp, "-%-*s ", name_maxlen, o->long_name);
		else
			fprintf (fp, "%*s", name_maxlen + 2, "");
		switch (o->opt_type)
		{
			case OPT_BOOLEAN:       fprintf (fp, "            "); break;
			case OPT_CONFIRM:       fprintf (fp, "yes|no|ask  "); break;
			case OPT_STRINGBUF8:
			case OPT_STRINGPTR:
			case OPT_STRINGPTRACC:  fprintf (fp, "<string>    "); break;
			case OPT_INTEGER:       fprintf (fp, "<integer>   "); break;
			case OPT_UNSIGNED:      fprintf (fp, "<unsigned>  "); break;
			case OPT_STRINGPTRLIST: fprintf (fp, "<string> ..."); break;
			case OPT_END: ;  // This line is here only to silence a GCC warning.
		}
		fprintf (fp, " %s\n", o->desc);
	}
}


/*
 *  confirm_e2i
 *  Convert the external representation of a confirmation
 *  flag ("yes", "no", "ask", "ask_once") to the internal
 *  representation (YC_YES, YC_NO, YC_ASK, YC_ASK_ONCE or
 *  '\0' if none).
 */
static confirm_t confirm_e2i (const char *external)
{
	if (external != NULL)
	{
		if (! strcmp (external, "yes"))
			return YC_YES;
		if (! strcmp (external, "no"))
			return YC_NO;
		if (! strcmp (external, "ask"))
			return YC_ASK;
		if (! strcmp (external, "ask_once"))
			return YC_ASK_ONCE;
	}
	return YC_ASK;
}


/*
 *  confirm_i2e
 *  Convert the internal representation of a confirmation
 *  flag (YC_YES, YC_NO, YC_ASK, YC_ASK_ONCE) to the external
 *  representation ("yes", "no", "ask", "ask_once" or "?").
 */
static const char *confirm_i2e (confirm_t internal)
{
	if (internal == YC_YES)
		return "yes";
	if (internal == YC_NO)
		return "no";
	if (internal == YC_ASK)
		return "ask";
	if (internal == YC_ASK_ONCE)
		return "ask_once";
	return "?";
}


/*
 *  append_item_to_list
 *  Append a string to a null-terminated string list
 */
static void append_item_to_list (const char ***list, const char *item)
{
	int i;

	i = 0;
	if (*list != 0)
	{
		// Count the number of elements in the list (last = null)
		while ((*list)[i] != 0)
			i++;
		// Expand the list
		*list = (const char **) ResizeMemory (*list, (i + 2) * sizeof **list);
	}
	else
	{
		// Create a new list
		*list = (const char **) GetMemory (2 * sizeof **list);
	}
	// Append the new element
	(*list)[i] = item;
	(*list)[i + 1] = 0;
}


#include <string>
#include <vector>

const size_t MAX_TOKENS = 10;

/*
 *  word_splitting - perform word splitting on a string
 */
int word_splitting (std::vector<std::string>& tokens, const char *string)
{
	size_t      ntokens     = 0;
	const char *iptr        = string;
	const char *token_start = 0;
	bool        in_token    = false;
	bool        quoted      = false;

	/* break the line into whitespace-separated tokens.
	   whitespace can be enclosed in double quotes. */
	for (; ; iptr++)
	{
		if (*iptr == '\n' || *iptr == '\0')
			break;

		else if (*iptr == '"')
			quoted = ! quoted;

		// "#" at the beginning of a token
		else if (! in_token && ! quoted && *iptr == '#')
			break;

		// First character of token
		else if (! in_token && (quoted || ! isspace (*iptr)))
		{
			ntokens++;
			if (ntokens > MAX_TOKENS)
				return 2;  // Too many tokens
			in_token = true;
		}

		// First space between two tokens
		else if (in_token && ! quoted && isspace (*iptr))
		{
			tokens.push_back (std::string (token_start, iptr - token_start));
			in_token = false;
		}
	}

	if (in_token)
		tokens.push_back (std::string (token_start, iptr - token_start));

	if (quoted)
		return 1;  // Unmatched double quote

	return 0;
}


//------------------------------------------------------------------------


int M_ParseLine(const char *line, const char ** tokens, int max_tok)
{
	int num_tok = 0;

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
			if (ch == '"')
			{
				in_string = true;
				continue;
			}

			// begin a normal token
			tokenbuf[tokenlen++] = ch;
			continue;
		}

		if (ch == '"' && in_string)
		{
			// end of string
			in_string = false;
		}
		else if (ch == 0 || ch == '\n' || isspace(ch))
		{
			// end of token
		}
		else
		{
			tokenbuf[tokenlen++] = ch;
			continue;
		}

		if (num_tok >= max_tok)  // ERROR: too many tokens
			return -2;

		if (in_string)  // ERROR: non-terminated string
			return -3;

		tokenbuf[tokenlen] = 0;
		tokenlen = -1;

		tokens[num_tok++] = strdup(tokenbuf);

		// end of line?  if yes, break out of for loop
		if (ch == 0 || ch == '\n')
			break;
	}

	return num_tok;
}


void M_FreeLine(const char ** tokens, int num_tok)
{
	for (int i = 0 ; i < num_tok ; i++)
	{
		free((void *) tokens[i]);

		tokens[i] = NULL;
	}
}



char * PersistFilename(const crc32_c *crc)
{
	static char filename[FL_PATH_MAX];

	// FIXME !!! proper base directory
	const char *base_dir = ".";

	sprintf(filename, "%s/cache/%08X%08X.dat", base_dir,
	        crc->extra, crc->raw);

	return filename;
}


bool M_LoadUserState(const crc32_c *crc)
{
	char *filename = PersistFilename(crc);

	LogPrintf("Load User State from: %s\n", filename);

	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		LogPrintf("--> No such file.\n");
		return false;
	}

	static char line_buf[FL_PATH_MAX];

	const char * tokens[MAX_TOKENS];

	while (! feof(fp))
	{
		char *line = fgets(line_buf, FL_PATH_MAX, fp);

		if (! line)
			break;

		int num_tok = M_ParseLine(line, tokens, MAX_TOKENS);

		if (num_tok < 0)
		{
			LogPrintf("Error in persistent data: %s\n", line);
			continue;
		}

		if (  Editor_ParseUser(tokens, num_tok) ||
		        Grid_ParseUser(tokens, num_tok) ||
		    Render3D_ParseUser(tokens, num_tok) ||
		     Browser_ParseUser(tokens, num_tok))
		{
			// Ok
		}
		else
		{
			LogPrintf("Unknown persistent data: %s\n", line);
		}
	}

	fclose(fp);

	return true;
}


bool M_SaveUserState(const crc32_c *crc)
{
	char *filename = PersistFilename(crc);

	LogPrintf("Save User State to: %s\n", filename);

	FILE *fp = fopen(filename, "w");

	if (! fp)
	{
		LogPrintf("--> FAILED!\n");
		return false;
	}

	  Editor_WriteUser(fp);
	    Grid_WriteUser(fp);
	Render3D_WriteUser(fp);
	 Browser_WriteUser(fp);

#if 0
	Browser_WriteUser(fp);
#endif

	fclose(fp);

	return true;
}


void M_DefaultUserState()
{
	grid.Init();

	CMD_ZoomWholeMap();

    Render3D_Init();

	if (main_win)
		main_win->NewEditMode('t');

}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
