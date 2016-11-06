//------------------------------------------------------------------------
//  GAME DEFINITION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
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

#include <map>
#include <algorithm>

#include "im_color.h"
#include "m_game.h"
#include "levels.h"
#include "e_things.h"


#define UNKNOWN_THING_RADIUS  16
#define UNKNOWN_THING_COLOR   fl_rgb_color(0,255,255)


std::map<char, linegroup_t *>    line_groups;
std::map<char, thinggroup_t *>   thing_groups;
std::map<char, texturegroup_t *> texture_groups;

std::map<int, linetype_t *>   line_types;
std::map<int, sectortype_t *> sector_types;
std::map<int, thingtype_t *>  thing_types;

std::map<std::string, char> texture_assigns;
std::map<std::string, char> flat_assigns;


game_info_t  game_info;  // contains sky_color (etc)


generalized_linetype_t gen_linetypes[MAX_GEN_NUM_TYPES];

int num_gen_linetypes;


/*
 *  Create empty lists for game definitions
 */
void M_InitDefinitions()
{
	// FIXME: delete the contents

    line_groups.clear();
    line_types.clear();
    sector_types.clear();

    thing_groups.clear();
    thing_types.clear();

	texture_groups.clear();
	texture_assigns.clear();
	flat_assigns.clear();

	// reset game information
	memset(&game_info, 0, sizeof(game_info));

	// FIXME: ability to parse from a game definition file
	game_info.player_h = 56;
	game_info.min_dm_starts = 4;
	game_info.max_dm_starts = 10;

	// reset generalized types
	memset(&gen_linetypes, 0, sizeof(gen_linetypes));
	num_gen_linetypes = 0;
}


static short ParseThingdefFlags(const char *s)
{
	short flags = 0;

	if (strchr(s, 'i')) flags |= THINGDEF_INVIS;
	if (strchr(s, 'c')) flags |= THINGDEF_CEIL;
	if (strchr(s, 'l')) flags |= THINGDEF_LIT;
	if (strchr(s, 'n')) flags |= THINGDEF_PASS;
	if (strchr(s, 'v')) flags |= THINGDEF_VOID;
	if (strchr(s, 't')) flags |= THINGDEF_TELEPT;

	return flags;
}


static void ParseColorDef(char ** argv, int argc)
{
	if (y_stricmp(argv[0], "sky") == 0)
	{
		game_info.sky_color = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "wall") == 0)
	{
		game_info.wall_colors[0] = atoi(argv[1]);

		if (argc < 2)
			game_info.wall_colors[1] = game_info.wall_colors[0];
		else
			game_info.wall_colors[1] = atoi(argv[2]);

	}
	else if (y_stricmp(argv[0], "floor") == 0)
	{
		game_info.floor_colors[0] = atoi(argv[1]);

		if (argc < 2)
			game_info.floor_colors[1] = game_info.floor_colors[0];
		else
			game_info.floor_colors[1] = atoi(argv[2]);
	}
	else if (y_stricmp(argv[0], "missing") == 0)
	{
		game_info.missing_color = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "unknown_tex") == 0)
	{
		game_info.unknown_tex = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "unknown_flat") == 0)
	{
		game_info.unknown_flat = atoi(argv[1]);
	}
	else
	{
		LogPrintf("unknown color keyword: '%s'\n", argv[0]);
	}
}


static map_format_bitset_t ParseMapFormats(char ** argv, int argc)
{
	map_format_bitset_t result = 0;

	for ( ; argc > 0 ; argv++, argc--)
	{
		if (y_stricmp(argv[0], "DOOM") == 0)
			result |= (1 << MAPF_Doom);

		else if (y_stricmp(argv[0], "HEXEN") == 0)
			result |= (1 << MAPF_Hexen);

		else
			FatalError("Unknown map format '%s' in definition file.\n", argv[0]);
	}

	return result;
}


static void ParseFeatureDef(char ** argv, int argc)
{
	if (y_stricmp(argv[0], "gen_types") == 0)
	{
		game_info.gen_types = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "img_png") == 0)
	{
		game_info.img_png = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "tx_start") == 0)
	{
		game_info.tx_start = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "coop_dm_flags") == 0)
	{
		game_info.coop_dm_flags = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "friend_flag") == 0)
	{
		game_info.friend_flag = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "pass_through") == 0)
	{
		game_info.pass_through = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "3d_midtex") == 0)
	{
		game_info.midtex_3d = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "medusa_bug") == 0)
	{
		game_info.medusa_bug = atoi(argv[1]);
	}
	else
	{
		LogPrintf("unknown feature keyword: '%s'\n", argv[0]);
	}
}


static const char * FindDefinitionFile(const char *folder, const char *name)
{
	static char filename[FL_PATH_MAX];

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		const char *base_dir = (pass == 0) ? home_dir : install_dir;

		if (! base_dir)
			continue;

		sprintf(filename, "%s/%s/%s.ugh", base_dir, folder, name);

		DebugPrintf("  trying: %s\n", filename);

		if (FileExists(filename))
			return filename;
	}

	return NULL;
}


bool M_CanLoadDefinitions(const char *folder, const char *name)
{
	const char * filename = FindDefinitionFile(folder, name);

	return (filename != NULL);
}


/*
 *  Loads a definition file.  The ".ugh" extension is added.
 *  Will try the "common" folder if not found in the given one.
 *
 *  Examples: "games" + "doom2"
 *            "ports" + "edge"
 *            "mods"  + "qdoom"
 */
void M_LoadDefinitions(const char *folder, const char *name)
{
	// this is for error messages & debugging
	char prettyname[256];

	sprintf(prettyname, "%s/%s.ugh", folder, name);

	LogPrintf("Loading Definitions : %s\n", prettyname);

	const char * filename = FindDefinitionFile(folder, name);

	if (! filename)
		FatalError("Cannot find definition file: %s\n", prettyname);

	DebugPrintf("  found at: %s\n", filename);

	if (strcmp(folder, "mods") == 0)
		M_ParseDefinitionFile(PURPOSE_Resource, filename, folder, prettyname);
	else
		M_ParseDefinitionFile(PURPOSE_Normal, filename, folder, prettyname);
}


void M_ParseDefinitionFile(parse_purpose_e purpose,
						   const char *filename,
						   const char *folder,
						   const char *prettyname,
						   parse_check_info_t * check_info,
						   int include_level)
{
	if (! folder)
		folder = "common";
	
	if (! prettyname)
		prettyname = fl_filename_name(filename);


#define YGD_BUF  512   /* max. line length + 2 */
	char readbuf[YGD_BUF];    /* buffer the line is read into */

#define MAX_TOKENS  30   /* tokens per line */
	int lineno;     /* current line of file */

#define MAX_INCLUDE_LEVEL  10

	int current_gen_line = -1;



	FILE *fp = fopen(filename, "r");
	if (! fp)
		FatalError("Cannot open %s: %s\n", filename, strerror(errno));

	/* Read the game definition file, line by line. */

	for (lineno = 2 ; fgets(readbuf, sizeof readbuf, fp) ; lineno++)
	{
		int         ntoks;
		char       *token[MAX_TOKENS];
		int         quoted;
		int         in_token;
		const char *iptr;
		char       *optr;
		char       *buf;

		const char *const bad_arg_count =
			"%s(%d): directive \"%s\" takes %d parameters\n";

		// create a buffer to contain the tokens [Note: never freed]
		buf = StringNew((int)strlen(readbuf) + 1);

		/* break the line into whitespace-separated tokens.
		   whitespace can be enclosed in double quotes. */
		for (in_token = 0, quoted = 0, iptr = readbuf, optr = buf, ntoks = 0 ; ; iptr++)
		{
			if (*iptr == '\n' || *iptr == '\0')
			{
				if (in_token)
					*optr = '\0';
				break;
			}

			else if (*iptr == '"')
				quoted ^= 1;

			// "#" at the beginning of a token
			else if (! in_token && ! quoted && *iptr == '#')
				break;

			// First character of token
			else if (! in_token && (quoted || ! isspace(*iptr)))
			{
				if (ntoks >= (int) (sizeof token / sizeof *token))
					FatalError("%s(%d): more than %d tokens\n",
							prettyname, lineno, sizeof token / sizeof *token);
				token[ntoks] = optr;
				ntoks++;
				in_token = 1;
				*optr++ = *iptr;
			}

			// First space between two tokens
			else if (in_token && ! quoted && isspace(*iptr))
			{
				*optr++ = '\0';
				in_token = 0;
			}

			// Character in the middle of a token
			else if (in_token)
				*optr++ = *iptr;
		}

		if (quoted)
			FatalError("%s(%d): unmatched double quote\n", prettyname, lineno);

		/* process the line */

		int nargs = ntoks - 1;

		// skip empty lines
		if (ntoks == 0)
		{
			continue;
		}

		// handle includes
		if (y_stricmp(token[0], "include") == 0)
		{
			if (nargs != 1)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 1);

			if (include_level >= MAX_INCLUDE_LEVEL)
				FatalError("%s(%d): Too many includes (check for a loop)\n", prettyname, lineno);

			const char * new_folder = folder;
			const char * new_name = FindDefinitionFile(new_folder, token[1]);

			// if not found, check the common/ folder
			if (! new_name && strcmp(folder, "common") != 0)
			{
				new_folder = "common";
				new_name = FindDefinitionFile(new_folder, token[1]);
			}

			if (! new_name)
				FatalError("%s(%d): Cannot find include file: %s.ugh\n", prettyname, lineno, token[1]);

			M_ParseDefinitionFile(purpose, new_name, new_folder,
								  NULL /* prettyname */,
								  check_info, include_level + 1);
			continue;
		}

		// handle a game-checking parse
		if (purpose == PURPOSE_GameCheck)
		{
			SYS_ASSERT(check_info);

			if (y_stricmp(token[0], "supported_game") == 0)
				FatalError("%s(%d): supported_game can only be used in port definitions\n", prettyname, lineno);

			else if (y_stricmp(token[0], "variant_of") == 0)
			{
				// TODO
			}

			else if (y_stricmp(token[0], "map_formats") == 0)
			{
				if (nargs < 1)
					FatalError(bad_arg_count, prettyname, lineno, token[0], 1);

				check_info->formats = ParseMapFormats(token + 1, nargs);
			}

			continue;
		}

		// handle a port-checking parse
		if (purpose == PURPOSE_PortCheck)
		{
			SYS_ASSERT(check_info);

			if (y_stricmp(token[0], "variant_of") == 0)
				FatalError("%s(%d): variant_of can only be used in game definitions\n", prettyname, lineno);

			else if (y_stricmp(token[0], "supported_game") == 0)
			{
				// TODO
			}

			else if (y_stricmp(token[0], "map_formats") == 0)
			{
				if (nargs < 1)
					FatalError(bad_arg_count, prettyname, lineno, token[0], 1);

				check_info->formats = ParseMapFormats(token + 1, nargs);
			}

			continue;
		}

		// handle a normal parse

		if (y_stricmp(token[0], "variant_of") == 0 ||
			y_stricmp(token[0], "supported_games") == 0 ||
			y_stricmp(token[0], "map_formats") == 0)
		{
			/* these are handled above, ignored in a normal parse */
		}

		else if (y_stricmp(token[0], "level_name") == 0)
		{
			/* ignored for backwards compability */
		}

		else if (y_stricmp(token[0], "sky_color") == 0)  // back compat
		{
			if (nargs != 1)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 1);

			game_info.sky_color = atoi(token[1]);
		}

		else if (y_stricmp(token[0], "sky_flat") == 0)
		{
			if (nargs != 1)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 1);

			if (strlen(token[1]) >= sizeof(game_info.sky_flat))
				FatalError("%s(%d): sky_flat name is too long\n", prettyname, lineno);

			strcpy(game_info.sky_flat, token[1]);
		}

		else if (y_stricmp(token[0], "color") == 0)
		{
			if (nargs < 2)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 2);

			ParseColorDef(token + 1, nargs);
		}

		else if (y_stricmp(token[0], "feature") == 0)
		{
			if (nargs < 2)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 2);

			ParseFeatureDef(token + 1, nargs);
		}

		else if (y_stricmp(token[0], "default_port") == 0)
		{
			/* ignored for backwards compability */
		}

		else if (y_stricmp(token[0], "default_textures") == 0)
		{
			if (nargs != 3)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 3);

			default_wall_tex	= token[1];
			default_floor_tex	= token[2];
			default_ceil_tex	= token[3];
		}

		else if (y_stricmp(token[0], "default_thing") == 0)
		{
			if (nargs != 1)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 1);

			default_thing = atoi(token[1]);
		}

		else if (y_stricmp(token[0], "linegroup") == 0)
		{
			if (nargs != 2)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 2);

			linegroup_t * lg = new linegroup_t;

			lg->group = token[1][0];
			lg->desc  = token[2];

			line_groups[lg->group] = lg;
		}

		else if (y_stricmp(token[0], "line") == 0 ||
				 y_stricmp(token[0], "special") == 0)
		{
			if (nargs < 3)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 3);

			linetype_t * info = new linetype_t;

			memset(info->args, 0, sizeof(info->args));

			int number = atoi(token[1]);

			info->group = token[2][0];
			info->desc  = token[3];

			int arg_count = MIN(nargs - 3, 5);

			for (int i = 0 ; i < arg_count ; i++)
			{
				if (token[4 + i][0] != '-')
					info->args[i] = token[4 + i];
			}

			// FIXME : have separate tables for "special"

			if (line_groups.find( info->group) == line_groups.end())
			{
				LogPrintf("%s(%d): unknown line group '%c'\n",
						prettyname, lineno,  info->group);
			}
			else
				line_types[number] = info;
		}

		else if (y_stricmp(token[0], "sector") == 0)
		{
			if (nargs != 2)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 2);

			int number = atoi(token[1]);

			sectortype_t *info = new sectortype_t;

			info->desc = token[2];

			sector_types[number] = info;
		}

		else if (y_stricmp(token[0], "thinggroup") == 0)
		{
			if (nargs != 3)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 3);

			thinggroup_t * tg = new thinggroup_t;

			tg->group = token[1][0];
			tg->color = ParseColor(token[2]);
			tg->desc  = token[3];

			thing_groups[tg->group] = tg;
		}

		else if (y_stricmp(token[0], "thing") == 0)
		{
			if (nargs != 6)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 6);

			thingtype_t * info = new thingtype_t;

			int number = atoi(token[1]);

			info->group  = token[2][0];
			info->flags  = ParseThingdefFlags(token[3]);
			info->radius = atoi(token[4]);
			info->sprite = token[5];
			info->desc   = token[6];

			if (thing_groups.find(info->group) == thing_groups.end())
			{
				LogPrintf("%s(%d): unknown thing group '%c'\n",
						prettyname, lineno, info->group);
			}
			else
			{	
				info->color = thing_groups[info->group]->color;

				thing_types[number] = info;
			}
		}

		else if (y_stricmp(token[0], "texturegroup") == 0)
		{
			if (nargs != 2)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 2);

			texturegroup_t * tg = new texturegroup_t;

			tg->group = token[1][0];
			tg->desc  = token[2];

			texture_groups[tg->group] = tg;
		}

		else if (y_stricmp(token[0], "texture") == 0)
		{
			if (nargs != 2)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 2);

			char group = token[1][0];
			std::string name = std::string(token[2]);

			if (texture_groups.find(tolower(group)) == texture_groups.end())
			{
				LogPrintf("%s(%d): unknown texture group '%c'\n",
						  prettyname, lineno, group);
			}
			else
				texture_assigns[name] = group;
		}

		else if (y_stricmp(token[0], "flat") == 0)
		{
			if (nargs != 2)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 2);

			char group = token[1][0];
			std::string name = std::string(token[2]);

			if (texture_groups.find(tolower(group)) == texture_groups.end())
			{
				LogPrintf("%s(%d): unknown texture group '%c'\n",
						prettyname, lineno, group);
			}
			else
				flat_assigns[name] = group;
		}

		else if (y_stricmp(token[0], "gen_line") == 0)
		{
			if (nargs != 4)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 4);

			current_gen_line = num_gen_linetypes;
			num_gen_linetypes++;

			if (num_gen_linetypes > MAX_GEN_NUM_TYPES)
				FatalError("%s(%d): too many gen_line definitions\n", prettyname, lineno);

			generalized_linetype_t *def = &gen_linetypes[current_gen_line];

			def->key = token[1][0];
	
			// use strtol() to support "0x" notation
			def->base   = strtol(token[2], NULL, 0);
			def->length = strtol(token[3], NULL, 0);

			def->name = token[4];
			def->num_fields = 0;
		}

		else if (y_stricmp(token[0], "gen_field") == 0)
		{
			if (nargs < 5)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 5);

			if (current_gen_line < 0)
				FatalError("%s(%d): gen_field used outside of a gen_line definition\n", prettyname, lineno);
			
			generalized_linetype_t *def = &gen_linetypes[current_gen_line];

			generalized_field_t *field = &def->fields[def->num_fields];

			def->num_fields++;
			if (def->num_fields > MAX_GEN_NUM_FIELDS)
				FatalError("%s(%d): too many fields in gen_line definition\n", prettyname, lineno);

			field->bits  = atoi(token[1]);
			field->shift = atoi(token[2]);

			field->mask  = ((1 << field->bits) - 1) << field->shift;

			field->default_val = atoi(token[3]);

			field->name = token[4];

			// grab the keywords
			field->num_keywords = MIN(nargs - 4, MAX_GEN_FIELD_KEYWORDS);

			for (int i = 0 ; i < field->num_keywords ; i++)
			{
				field->keywords[i] = token[5 + i]; 
			}
		}

		else if (y_stricmp(token[0], "exclude_game") == 0)
		{
			if (nargs != 1)
				FatalError(bad_arg_count, prettyname, lineno, token[0], 1);

			if (Game_name && y_stricmp(token[1], Game_name) == 0)
			{
				LogPrintf("WARNING: skipping %s -- not compatible with %s\n",
						  prettyname, Game_name);

				// do not process any more of the file
				fclose(fp);
				return;
			}
		}

		else
		{
			FatalError("%s(%d): unknown directive: %.32s\n",
					   prettyname, lineno, token[0]);
		}
	}

	fclose(fp);
}


/*
 *  Free all memory allocated to game definitions
 */
void M_FreeDefinitions()
{
}


static void scanner_add_file(const char *name, int flags, void *priv_dat)
{
	std::vector<const char*> * list = (std::vector<const char*> *) priv_dat;

//	DebugPrintf("  file [%s] flags:%d\n", name, flags);

	if (flags & (SCAN_F_IsDir | SCAN_F_Hidden))
		return;

	if (! MatchExtension(name, "ugh"))
		return;

	list->push_back(ReplaceExtension(name, NULL));
}


struct DefName_CMP_pred
{
	inline bool operator() (const char *A, const char *B) const
	{
		return y_stricmp(A, B) < 0;
	}
};

void M_CollectKnownDefs(const char *folder, std::vector<const char *> & list)
{
	std::vector<const char *> temp_list;

	static char path[FL_PATH_MAX];

//	DebugPrintf("M_CollectKnownDefs for: %d\n", folder);

	sprintf(path, "%s/%s", install_dir, folder);
	ScanDirectory(path, scanner_add_file, & temp_list);

	sprintf(path, "%s/%s", home_dir, folder);
	ScanDirectory(path, scanner_add_file, & temp_list);

	std::sort(temp_list.begin(), temp_list.end(), DefName_CMP_pred());

	// transfer to passed list, removing duplicates as we go
	unsigned int pos;

	for (pos = 0 ; pos < temp_list.size() ; pos++)
	{
		if (pos + 1 < temp_list.size() &&
			y_stricmp(temp_list[pos], temp_list[pos + 1]) == 0)
		{
			StringFree(temp_list[pos]);
			continue;
		}

		list.push_back(temp_list[pos]);
	}
}


const char * M_VariantForGame(const char *game)
{
	// static so we can return the char[] inside it
	static parse_check_info_t info;

	// when no "variant_of" lines exist, result is just input name
	strcpy(info.variant_name, game);

	const char * filename = FindDefinitionFile("games", game);
	SYS_ASSERT(filename);

	M_ParseDefinitionFile(PURPOSE_GameCheck, filename, "games",
						  NULL /* prettyname */, &info);

	SYS_ASSERT(info.variant_name[0]);

	return info.variant_name;
}


map_format_bitset_t M_DetermineMapFormats(const char *game, const char *port)
{
	parse_check_info_t info;

	info.formats = (1 << MAPF_Doom);

	const char * filename = FindDefinitionFile("games", game);
	SYS_ASSERT(filename);

	M_ParseDefinitionFile(PURPOSE_GameCheck, filename, "games",
						  NULL /* prettyname */, &info);

	filename = FindDefinitionFile("ports", port);
	SYS_ASSERT(filename);

	M_ParseDefinitionFile(PURPOSE_PortCheck, filename, "ports",
						  NULL /* prettyname */, &info);

	return info.formats;
}


static bool M_CheckPortSupportsGame(const char *var_game, const char *port)
{
	parse_check_info_t info;

	snprintf(info.variant_name, sizeof(info.variant_name), "%s", var_game);

	info.supports_game = 0;

	const char *filename = FindDefinitionFile("ports", port);

	if (! filename)
		return false;

	M_ParseDefinitionFile(PURPOSE_PortCheck, filename, "ports",
						  NULL /* prettyname */, &info);

	return (info.supports_game > 0);
}


// find all the ports which support the given game variant.
//
// result will be '|' separated (ready for Fl_Choice::add)
// returns the empty string when nothing found.
// The result should be freed with StringFree().
//
// will also find an existing name, storing its index in 'exist_val'
// (when not found, the value in 'exist_val' is not changed at all)

const char * M_CollectPortsForMenu(const char *var_game, int *exist_val, const char *exist_name)
{
	std::vector<const char *> list;

	M_CollectKnownDefs("ports", list);

	if (list.empty())
		return StringDup("");

	// determine final length
	int length = 2 + (int)list.size();
	unsigned int i;

	for (i = 0 ; i < list.size() ; i++)
		length += strlen(list[i]);

	char * result = StringNew(length);
	result[0] = 0;

	for (i = 0 ; i < list.size() ; i++)
	{
		if (! M_CheckPortSupportsGame(var_game, list[i]))
			continue;

		strcat(result, list[i]);

		if (i + 1 < list.size())
			strcat(result, "|");

		if (y_stricmp(list[i], exist_name) == 0)
			*exist_val = i;
	}

//	DebugPrintf( "RESULT = '%s'\n", result);

	return result;
}


//------------------------------------------------------------------------


bool is_sky(const char *flat)
{
	return (y_stricmp(game_info.sky_flat, flat) == 0);
}

bool is_null_tex(const char *tex)
{
	return tex[0] == '-';
}

bool is_missing_tex(const char *tex)
{
	return (tex[0] == 0 || tex[0] == '-');
}



const sectortype_t * M_GetSectorType(int type)
{
	std::map<int, sectortype_t *>::iterator SI;

	SI = sector_types.find(type);

	if (SI != sector_types.end())
		return SI->second;

	static sectortype_t dummy_type =
	{
		"UNKNOWN TYPE"
	};

	return &dummy_type;
}


const linetype_t * M_GetLineType(int type)
{
	std::map<int, linetype_t *>::iterator LI;

	LI = line_types.find(type);

	if (LI != line_types.end())
		return LI->second;

	static linetype_t dummy_type =
	{
		0, "UNKNOWN TYPE"
	};

	return &dummy_type;
}


const thingtype_t * M_GetThingType(int type)
{
	std::map<int, thingtype_t *>::iterator TI;

	TI = thing_types.find(type);

	if (TI != thing_types.end())
		return TI->second;

	static thingtype_t dummy_type =
	{
		0, 0, UNKNOWN_THING_RADIUS,
		"UNKNOWN TYPE", "NULL",
		UNKNOWN_THING_COLOR
	};

	return &dummy_type;
}


char M_GetTextureType(const char *name)
{
	std::map<std::string, char>::iterator TI;

	TI = texture_assigns.find(name);

	if (TI != texture_assigns.end())
		return TI->second;

	return '-';  // the OTHER category
}


char M_GetFlatType(const char *name)
{
	std::map<std::string, char>::iterator TI;

	TI = flat_assigns.find(name);

	if (TI != flat_assigns.end())
		return TI->second;

	return '-';  // the OTHER category
}


static bool LineCategory_IsUsed(char group)
{
	std::map<int, linetype_t *>::iterator IT;

	for (IT = line_types.begin() ; IT != line_types.end() ; IT++)
	{
		linetype_t *info = IT->second;
		if (info->group == group)
			return true;
	}

	return false;
}


static bool ThingCategory_IsUsed(char group)
{
	std::map<int, thingtype_t *>::iterator IT;

	for (IT = thing_types.begin() ; IT != thing_types.end() ; IT++)
	{
		thingtype_t *info = IT->second;
		if (info->group == group)
			return true;
	}

	return false;
}


static bool TextureCategory_IsUsed(char group)
{
	std::map<std::string, char>::iterator IT;

	for (IT = texture_assigns.begin() ; IT != texture_assigns.end() ; IT++)
		if (IT->second == group)
			return true;

	return false;
}


static bool FlatCategory_IsUsed(char group)
{
	std::map<std::string, char>::iterator IT;

	for (IT = flat_assigns.begin() ; IT != flat_assigns.end() ; IT++)
		if (IT->second == group)
			return true;

	return false;
}


const char *M_LineCategoryString(char *letters)
{
	static char buffer[2000];

	int L_index = 0;

	// the "ALL" category is always first
	strcpy(buffer, "ALL");
	letters[L_index++] = '*';

	std::map<char, linegroup_t *>::iterator IT;

	for (IT = line_groups.begin() ; IT != line_groups.end() ; IT++)
	{
		linegroup_t *G = IT->second;

		// the "Other" category is always at the end
		if (G->group == '-')
			continue;

		if (! LineCategory_IsUsed(G->group))
			continue;

		// FIXME: potential for buffer overflow here
		strcat(buffer, "|");
		strcat(buffer, G->desc);

		letters[L_index++] = IT->first;
	}

	strcat(buffer, "|Other");

	letters[L_index++] = '-';
	letters[L_index++] = 0;

	return buffer;
}


const char *M_ThingCategoryString(char *letters)
{
	static char buffer[2000];

	int L_index = 0;

	// these common categories are always first
	strcpy(buffer, "ALL|RECENT");
	letters[L_index++] = '*';
	letters[L_index++] = '^';

	std::map<char, thinggroup_t *>::iterator IT;

	for (IT = thing_groups.begin() ; IT != thing_groups.end() ; IT++)
	{
		thinggroup_t *G = IT->second;

		// the "Other" category is always at the end
		if (G->group == '-')
			continue;

		if (! ThingCategory_IsUsed(G->group))
			continue;

		// FIXME: potential for buffer overflow here
		strcat(buffer, "|");
		strcat(buffer, G->desc);

		letters[L_index++] = IT->first;
	}

	strcat(buffer, "|Other");

	letters[L_index++] = '-';
	letters[L_index++] = 0;

	return buffer;
}


const char *M_TextureCategoryString(char *letters, bool do_flats)
{
	static char buffer[2000];

	int L_index = 0;

	// these common categories are always first
	strcpy(buffer, "ALL|RECENT");
	letters[L_index++] = '*';
	letters[L_index++] = '^';

	std::map<char, texturegroup_t *>::iterator IT;

	for (IT = texture_groups.begin() ; IT != texture_groups.end() ; IT++)
	{
		texturegroup_t *G = IT->second;

		// the "Other" category is always at the end
		if (G->group == '-')
			continue;

		if (do_flats && !FlatCategory_IsUsed(G->group))
			continue;

		if (!do_flats && !TextureCategory_IsUsed(G->group))
			continue;

		// FIXME: potential for buffer overflow here
		strcat(buffer, "|");
		strcat(buffer, G->desc);

		letters[L_index++] = IT->first;
	}

	strcat(buffer, "|Other");

	letters[L_index++] = '-';
	letters[L_index++] = 0;

	return buffer;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
