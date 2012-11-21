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
#include "e_loadsave.h"
#include "m_config.h"
#include "r_misc.h"
#include "r_grid.h"
#include "r_render.h"
#include "levels.h"

#include "ui_window.h"  // meh!


// config item variables

extern bool leave_offsets_alone;
extern bool new_islands_are_void;
extern bool same_mode_clears_selection;


//------------------------------------------------------------------------

/*
 *  Description of the command line arguments and config file keywords
 */
typedef enum
{
	// End of the options description
	OPT_END = 0,

	// Boolean (toggle)
	// Receptacle is of type: bool
	OPT_BOOLEAN,

	// "yes", "no", "ask"
	// Receptacle is of type: confirm_t
	OPT_CONFIRM,

	// Integer number,
	// Receptacle is of type: int
	OPT_INTEGER,

	// String
	// Receptacle is of type: char[9]
	OPT_STRINGBUF8,

	// String
	// Receptacle is of type: const char *
	OPT_STRING,

	// List of strings
	// Receptacle is of type: std::vector< const char * >
	OPT_STRING_LIST,
}
opt_type_t;


typedef struct
{
	const char *long_name;  // Command line arg. or keyword
	const char *short_name; // Abbreviated command line argument
	opt_type_t opt_type;    // Type of this option
	const char *flags;    // Flags for this option :
	// "1" : process only on pass 1 of parse_command_line_options()
	// "h" : show in help, i.e. in dump_command_line_options()
	// "f" : string is a filename
	// "<" : print extra newline after this option (when dumping)
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

	{	"help",
		"h",
		OPT_BOOLEAN,
		"1h",
		"Show usage summary",
		NULL,
		&show_help
	},

	{	"version",
		0,
		OPT_BOOLEAN,
		"1h",
		"Show version info",
		NULL,
		&show_version
	},

	{	"debug",
		"d",
		OPT_BOOLEAN,
		"1h",
		"Enable debugging messages",
		NULL,
		&Debugging
	},

	{	"quiet",
		"q",
		OPT_BOOLEAN,
		"1h<",
		"Quiet mode (no messages on stdout)",
		NULL,
		&Quiet
	},


	{	"home",
		0,
		OPT_STRING,
		"1h",
		"Home directory",
		"<dir>",
		&home_dir
	},

	{	"install",
		0,
		OPT_STRING,
		"1h",
		"Installation directory",
		"<dir>",
		&install_dir
	},

	{	"log",
		0,
		OPT_STRING,
		"1h",
		"Log messages to file (instead of stdout)",
		"<file>",
		&log_file
	},

	{	"config",
		0,
		OPT_STRING,
		"1h<",
		"Config file to load",
		"<file>",
		&config_file
	},

	//
	// Normal options from here on....
	//

	{	"iwad",
		"i",
		OPT_STRING,
		"h",
		"The name of the IWAD",
		"<file>",
		&Iwad
	},

	{	"file",
		"f",
		OPT_STRING,
		"h",
		"Patch wad file to edit",
		"<file>",
		&Pwad
	},

// TODO
#if 0
	{	"merge",
		0,
		OPT_STRING_LIST,
		"h",
		"Resource file to load",
		"<file>",
		&PatchWads
	},
#endif

	{	"port",
		"p",
		OPT_STRING,
		"h",
		"Port (engine) name",
		"<name>",
		&Port_name
	},

// TODO
#if 0
	{	"mod",
		0,
		OPT_STRING,
		"h",
		"Mod name",
		"<name>",
		&Mod_name
	},
#endif

	{	"warp",
		"w",
		OPT_STRING,
		"h",
		"Select level to edit",
		"<map>",
		&Level_name
	},


	{	"copy_linedef_reuse_sidedefs",
		0,
		OPT_BOOLEAN,
		"",
		"Use same sidedefs as original linedef",
		NULL,
		&copy_linedef_reuse_sidedefs
	},

	{	"default_ceiling_height",
		0,
		OPT_INTEGER,
		"",
		"Default ceiling height",
		NULL,
		&default_ceiling_height
	},

	{	"default_ceiling_texture",
		0,
		OPT_STRINGBUF8,
		"",
		"Default ceiling texture",
		NULL,
		default_ceiling_texture
	},

	{	"default_floor_height",
		0,
		OPT_INTEGER,
		"",
		"Default floor height",
		NULL,
		&default_floor_height
	},

	{	"default_floor_texture",
		0,
		OPT_STRINGBUF8,
		"",
		"Default floor texture",
		NULL,
		default_floor_texture
	},

	{	"default_light_level",
		0,
		OPT_INTEGER,
		"",
		"Default light level",
		NULL,
		&default_light_level
	},

	{	"default_lower_texture",
		0,
		OPT_STRINGBUF8,
		"",
		"Default lower texture",
		NULL,
		default_lower_texture
	},

	{	"default_middle_texture",
		0,
		OPT_STRINGBUF8,
		"",
		"Default middle texture",
		NULL,
		default_middle_texture
	},

	{	"default_upper_texture",
		0,
		OPT_STRINGBUF8,
		"",
		"Default upper texture",
		NULL,
		default_upper_texture
	},

	{	"leave_offsets_alone",
		0,
		OPT_BOOLEAN,
		"",
		"Do not adjust offsets when splitting lines (etc)",
		NULL,
		&leave_offsets_alone
	},

	{	"new_islands_are_void",
		0,
		OPT_BOOLEAN,
		"",
		"Islands created inside a sector will get a void interior",
		NULL,
		&new_islands_are_void
	},

	{	"same_mode_clears_selection",
		0,
		OPT_BOOLEAN,
		"",
		"Clear the selection when entering the same mode",
		NULL,
		&same_mode_clears_selection
	},

	{	"scroll_less",
		0,
		OPT_INTEGER,
		"",
		"Amp. of scrolling (% of screen size)",
		NULL,
		&scroll_less
	},

	{	"scroll_more",
		0,
		OPT_INTEGER,
		"",
		"Amp. of scrolling (% of screen size)",
		NULL,
		&scroll_more
	},

	{	"sprite_scale",
		0,
		OPT_INTEGER,
		"",
		"Relative scale of sprites",
		NULL,
		&sprite_scale
	},

// -AJA- this may be better done with a 'mouse_button_order' string,
//       where the default is "123", and user could have "321"
//       or "132" or any other combination.
#if 0
	{	"swap_buttons",
		0,
		OPT_BOOLEAN,
		"",
		"Swap mouse buttons",
		NULL,
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
		0,
		0
	}
};


static confirm_t confirm_e2i (const char *external);
static const char *confirm_i2e (confirm_t internal);


/*
 *  try to parse a config file by pathname.
 *
 *  Return 0 on success, negative value on failure.
 */
static int parse_config_file(FILE *fp, const char *filename)
{
	char line[1024];

	const char *basename = FindBaseName(filename);

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
				p[len - 1] = 0;
		}

		// Skip empty lines
		if (*p == 0)
			continue;

		// Make <name> point on the <name> field
		name = p;
		while (y_isident (*p))
			p++;
		if (*p == 0)
		{
			LogPrintf("WARNING: %s(%u): missing '=', skipping\n", basename, lnum);
			goto next_line;
		}
		if (*p == '=')
		{
			// Mark the end of the option name
			*p = 0;
		}
		else
		{
			// Mark the end of the option name
			*p = 0;
			p++;
			// Skip blanks after the option name
			while (isspace ((unsigned char) *p))
				p++;
			if (*p != '=')
			{
				LogPrintf("WARNING: %s(%u): missing '=', skipping\n",
						  basename, lnum);
				goto next_line;
			}
		}
		p++;

		// First parameter : <value> points on the first character.
		// Put a NUL at the end. If there is a second parameter, holler.
		while (isspace ((unsigned char) *p))
			p++;
		value = p;

		{
			unsigned char *p2 = (unsigned char *) value;
			while (*p2 != 0 && ! isspace (*p2))
				p2++;
			if (*p2 != 0)  // There's trailing whitespace after 1st parameter
			{
				for (unsigned char *p3 = p2; *p3 != 0; p3++)
					if (! isspace (*p3))
					{
						LogPrintf("WARNING: %s(%u): extraneous argument\n",
								  basename, lnum);
						break;
					}
			}
			*p2 = 0;
		}

		for (const opt_desc_t *o = options; ; o++)
		{
			if (o->opt_type == OPT_END)
			{
				LogPrintf("WARNING: %s(%u): invalid option '%s', skipping\n",
						  basename, lnum, name);
				goto next_line;
			}
			if (! o->long_name || strcmp(name, o->long_name) != 0)
				continue;

			if (strchr(o->flags, '1'))
				break;

			switch (o->opt_type)
			{
				case OPT_BOOLEAN:
					if (y_stricmp(value, "no")    == 0 ||
					    y_stricmp(value, "false") == 0 ||
						y_stricmp(value, "off")   == 0 ||
						y_stricmp(value, "0")     == 0)
					{
						*((bool *) (o->data_ptr)) = false;
					}
					else  // anything else is TRUE
					{
						*((bool *) (o->data_ptr)) = true;
					}
					break;

				case OPT_CONFIRM:
					*((confirm_t *) o->data_ptr) = confirm_e2i (value);
					break;

				case OPT_INTEGER:
					*((int *) (o->data_ptr)) = atoi (value);
					break;

				case OPT_STRINGBUF8:
					strncpy ((char *) o->data_ptr, value, 8);
					((char *) o->data_ptr)[8] = 0;
					break;

				case OPT_STRING:
					*((char **) o->data_ptr) = StringDup(value);
					break;

				case OPT_STRING_LIST:
					while (*value != 0)
					{
						char *v = value;
						while (*v != 0 && ! isspace ((unsigned char) *v))
							v++;

						string_list_t * list = (string_list_t *)o->data_ptr;
						list->push_back(StringDup(value, v - value));

						while (isspace (*v))
							v++;
						value = v;
					}
					break;

				default:
					{
						BugError("INTERNAL ERROR: unknown option type %d", (int) o->opt_type);
						return -1;
					}
			}
			break;
		}
next_line: ;
	}

	return 0;  // OK
}


/*
 *  parses a user-specified config file
 *
 *  Return 0 on success, negative value on error.
 */
int parse_config_file_user(const char *filename)
{
	FILE * fp = fopen(filename, "r");

	LogPrintf("Read config file: %s\n", filename);

	if (fp == NULL)
	{
		LogPrintf("--> %s\n", strerror(errno));
		return -1;
	}

	int rc = parse_config_file(fp, filename);

	fclose(fp);

	return rc;
}


/*
 *  parse the default config file (if any)
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


static void parse_loose_file(const char *filename)
{
	// too simplistic??

	Pwad = StringDup(filename);
}


/*
 *  parses the command line options
 *
 *  If <pass> is set to 1, ignores all options except those
 *  that have the "1" flag.
 *  Else, ignores all options that have the "1" flag.
 *  If an error occurs, report it with LogPrintf().
 *  and returns non-zero. Else, returns 0.
 */
int parse_command_line_options (int argc, const char *const *argv, int pass)
{
	const opt_desc_t *o;

	while (argc > 0)
	{
		bool ignore;

		// is it actually an option?
		if (argv[0][0] != '-')
		{
			// this is a loose file, handle it now
			parse_loose_file(argv[0]);

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
				return 1;
			}

			if ( (o->short_name && strcmp (argv[0]+1, o->short_name) == 0) ||
				 (o->long_name  && strcmp (argv[0]+1, o->long_name ) == 0) ||
				 (o->long_name  && argv[0][1] == '-' && strcmp (argv[0]+2, o->long_name ) == 0) )
				break;
		}

		// If this option has the "1" flag but pass is not 1
		// or it doesn't but pass is 1, ignore it.
		ignore = (strchr(o->flags, '1') != NULL) != (pass == 1);

		switch (o->opt_type)
		{
			case OPT_BOOLEAN:
				// -AJA- permit a following value
				if (argc >= 1 && argv[1][0] != '-')
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

			case OPT_CONFIRM:
				if (argc <= 1)
				{
					FatalError("missing argument after '%s'\n", argv[0]);
					return 1;
				}

				argv++;
				argc--;

				if (! ignore)
				{
					*((confirm_t *) o->data_ptr) = confirm_e2i (argv[0]);
				}
				break;

			case OPT_INTEGER:
				if (argc <= 1)
				{
					FatalError("missing argument after '%s'\n", argv[0]);
					return 1;
				}

				argv++;
				argc--;

				if (! ignore)
				{
					*((int *) o->data_ptr) = atoi (argv[0]);
				}
				break;

			case OPT_STRINGBUF8:
				if (argc <= 1)
				{
					FatalError("missing argument after '%s'\n", argv[0]);
					return 1;
				}

				argv++;
				argc--;

				if (! ignore)
				{
					strncpy ((char *) o->data_ptr, argv[0], 8);
					((char *) o->data_ptr)[8] = 0;
				}
				break;

			case OPT_STRING:
				if (argc <= 1)
				{
					FatalError("missing argument after '%s'\n", argv[0]);
					return 1;
				}

				argv++;
				argc--;

				if (! ignore)
				{
					*((const char **) o->data_ptr) = StringDup(argv[0]);
				}
				break;

			case OPT_STRING_LIST:
				if (argc <= 1)
				{
					FatalError("missing argument after '%s'\n", argv[0]);
					return 1;
				}
				while (argc > 1 && argv[1][0] != '-' && argv[1][0] != '+')
				{
					argv++;
					argc--;

					if (! ignore)
					{
						string_list_t * list = (string_list_t *) o->data_ptr;
						list->push_back(StringDup(argv[0]));
					}
				}
				break;

			default:
				{
					BugError("INTERNAL ERROR: unknown option type (%d)", (int) o->opt_type);
					return 1;
				}
		}

		argv++;
		argc--;
	}
	return 0;
}


/*
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
		
		fprintf (fp, "%-*s  %-*s  ", name_maxlen, o->long_name, desc_maxlen, o->desc);

		if (o->opt_type == OPT_BOOLEAN)
			fprintf (fp, "%s", *((bool *) o->data_ptr) ? "enabled" : "disabled");
		else if (o->opt_type == OPT_CONFIRM)
			fputs (confirm_i2e (*((confirm_t *) o->data_ptr)), fp);
		else if (o->opt_type == OPT_STRINGBUF8)
			fprintf (fp, "'%s'", (char *) o->data_ptr);
		else if (o->opt_type == OPT_STRING)
			fprintf (fp, "'%s'", *((char **) o->data_ptr));
		else if (o->opt_type == OPT_INTEGER)
			fprintf (fp, "%d", *((int *) o->data_ptr));
		else if (o->opt_type == OPT_STRING_LIST)
		{
			string_list_t *list = (string_list_t *)o->data_ptr;

			if (list->empty())
				fprintf(fp, "--none--");
			else for (unsigned int i = 0 ; i < list->size() ; i++)
				fprintf(fp, "'%s' ", list->at(i));
		}
		fputc ('\n', fp);
	}
}


/*
 *  Print a list of all command line options (usage message).
 */
void dump_command_line_options(FILE *fp)
{
	const opt_desc_t *o;
	int name_maxlen = 0;
	int  arg_maxlen = 0;

	for (o = options; o->opt_type != OPT_END; o++)
	{
		int len;

		if (! strchr(o->flags, 'h'))
			continue;

		if (o->long_name)
		{
			len = strlen (o->long_name);
			name_maxlen = MAX(name_maxlen, len);
		}

		if (o->arg_desc)
		{
			len = strlen (o->arg_desc);
			arg_maxlen = MAX(arg_maxlen, len);
		}
	}

	for (o = options; o->opt_type != OPT_END; o++)
	{
		if (! strchr(o->flags, 'h'))
			continue;

		if (o->short_name)
			fprintf (fp, "  -%-3s ", o->short_name);
		else
			fprintf (fp, "       ");

		if (o->long_name)
			fprintf (fp, "-%-*s   ", name_maxlen, o->long_name);
		else
			fprintf (fp, "%*s  ", name_maxlen + 2, "");

		if (o->arg_desc)
			fprintf (fp, "%-12s", o->arg_desc);
		else switch (o->opt_type)
		{
			case OPT_BOOLEAN:       fprintf (fp, "            "); break;
			case OPT_CONFIRM:       fprintf (fp, "yes|no|ask  "); break;
			case OPT_INTEGER:       fprintf (fp, "<value>     "); break;

			case OPT_STRINGBUF8:
			case OPT_STRING:        fprintf (fp, "<string>    "); break;
			case OPT_STRING_LIST:   fprintf (fp, "<string> ..."); break;
			case OPT_END: ;  // This line is here only to silence a GCC warning.
		}

		fprintf (fp, " %s\n", o->desc);

		if (strchr(o->flags, '<'))
			fprintf (fp, "\n");
	}
}


/*
 *  confirm_e2i
 *  Convert the external representation of a confirmation
 *  flag ("yes", "no", "ask", "ask_once") to the internal
 *  representation (YC_YES, YC_NO, YC_ASK, YC_ASK_ONCE or
 *  0 if none).
 */
static confirm_t confirm_e2i (const char *external)
{
	if (external != NULL)
	{
		if (! y_stricmp (external, "yes"))
			return YC_YES;
		if (! y_stricmp (external, "no"))
			return YC_NO;
		if (! y_stricmp (external, "ask"))
			return YC_ASK;
		if (! y_stricmp (external, "ask_once"))
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
		if (*iptr == '\n' || *iptr == 0)
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



char * PersistFilename(const crc32_c& crc)
{
	static char filename[FL_PATH_MAX];

	sprintf(filename, "%s/cache/%08X%08X.dat", home_dir,
	        crc.extra, crc.raw);

	return filename;
}


bool M_LoadUserState()
{
	crc32_c crc;

	BA_LevelChecksum(crc);


	char *filename = PersistFilename(crc);

	LogPrintf("Load user state from: %s\n", filename);

	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		LogPrintf("--> %s\n", strerror(errno));
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

		if (num_tok == 0)
			continue;

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


bool M_SaveUserState()
{
	crc32_c crc;

	BA_LevelChecksum(crc);


	char *filename = PersistFilename(crc);

	LogPrintf("Save user state to: %s\n", filename);

	FILE *fp = fopen(filename, "w");

	if (! fp)
	{
		LogPrintf("--> FAILED! (%s)\n", strerror(errno));
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
