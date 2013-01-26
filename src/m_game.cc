//------------------------------------------------------------------------
//  GAME DEFINITION
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


ygln_t yg_level_name = YGLN__;

std::string g_sky_flat;

int g_sky_color;


/*
 *  Create empty lists for game definitions
 */
void InitDefinitions()
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
}


static short ParseThingdefFlags(const char *s)
{
	short flags = 0;

	if (strchr(s, 'i')) flags |= THINGDEF_INVIS;
	if (strchr(s, 'c')) flags |= THINGDEF_CEIL;
	if (strchr(s, 'l')) flags |= THINGDEF_LIT;

	return flags;
}


static const char * FindDefinitionFile(
	const char *base_dir, const char *folder, const char *name)
{
	static char filename[FL_PATH_MAX];

	if (! base_dir)
		return NULL;

	sprintf(filename, "%s/%s/%s.ugh", base_dir, folder, name);

DebugPrintf("  trying: %s\n", filename);

	if (FileExists(filename))
		return filename;
	
	return NULL;
}


bool CanLoadDefinitions(const char *folder, const char *name)
{
	const char * filename;

	filename = FindDefinitionFile(home_dir, folder, name);

	if (! filename)
		filename = FindDefinitionFile(install_dir, folder, name);

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
void LoadDefinitions(const char *folder, const char *name, int include_level)
{
	// this is for error messages & debugging
	char basename[256];

	sprintf(basename, "%s/%s.ugh", folder, name);

	LogPrintf("Loading Definitions : %s\n", basename);


	const char * filename;

	filename = FindDefinitionFile(home_dir, folder, name);

	if (! filename)
		filename = FindDefinitionFile(install_dir, folder, name);

	// look in common/ folder as last resort
	if (! filename && strcmp(folder, "common") != 0)
	{
		folder = "common";

		filename = FindDefinitionFile(home_dir, folder, name);

		if (! filename)
			filename = FindDefinitionFile(install_dir, folder, name);
	}

	if (! filename)
		FatalError("Cannot find definition file: %s\n", basename);


	DebugPrintf("  found at: %s\n", filename);


#define YGD_BUF 200   /* max. line length + 2 */
	char readbuf[YGD_BUF];    /* buffer the line is read into */

#define MAX_TOKENS 10   /* tokens per line */
	int lineno;     /* current line of file */

#define MAX_INCLUDE_LEVEL  10


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
							basename, lineno, sizeof token / sizeof *token);
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
			FatalError("%s(%d): unmatched double quote\n", basename, lineno);

		/* process the line */

		if (ntoks == 0)
		{
			continue;
		}

		else if (y_stricmp(token[0], "include") == 0)
		{
			if (ntoks != 2)
				FatalError(bad_arg_count, basename, lineno, token[0], 1);

			if (include_level >= MAX_INCLUDE_LEVEL)
				FatalError("%s(%d): Too many includes (check for a loop)\n", basename, lineno);

			// TODO: validate filename

			LoadDefinitions(folder, token[1], include_level + 1);
		}

		else if (y_stricmp(token[0], "level_name") == 0)
		{
			if (ntoks != 2)
				FatalError(bad_arg_count, basename, lineno, token[0], 1);

			if (! strcmp(token[1], "e1m1"))
				yg_level_name = YGLN_E1M1;
			else if (! strcmp(token[1], "e1m10"))
				yg_level_name = YGLN_E1M10;
			else if (! strcmp(token[1], "map01"))
				yg_level_name = YGLN_MAP01;
			else
				FatalError("%s(%d): invalid argument \"%.32s\" (e1m1|e1m10|map01)\n",
						   basename, lineno, token[1]);
		}

		else if (y_stricmp(token[0], "sky_color") == 0)
		{
			if (ntoks != 2)
				FatalError(bad_arg_count, basename, lineno, token[0], 1);

			g_sky_color = atoi(token[1]);
		}

		else if (y_stricmp(token[0], "sky_flat") == 0)
		{
			if (ntoks != 2)
				FatalError(bad_arg_count, basename, lineno, token[0], 1);

			g_sky_flat = token[1];
		}

		else if (y_stricmp(token[0], "default_port") == 0)
		{
			if (ntoks != 2)
				FatalError(bad_arg_count, basename, lineno, token[0], 1);

			if (! Port_name)
				Port_name = token[1];
		}

		else if (y_stricmp(token[0], "default_textures") == 0)
		{
			if (ntoks != 4)
				FatalError(bad_arg_count, basename, lineno, token[0], 1);

			default_mid_tex		= token[1];
			default_floor_tex	= token[2];
			default_ceil_tex	= token[3];

			default_upper_tex = default_mid_tex;
			default_lower_tex = default_mid_tex;
		}

		else if (y_stricmp(token[0], "default_thing") == 0)
		{
			if (ntoks != 2)
				FatalError(bad_arg_count, basename, lineno, token[0], 1);

			default_thing = atoi(token[1]);
		}

		else if (y_stricmp(token[0], "linegroup") == 0)
		{
			if (ntoks != 3)
				FatalError(bad_arg_count, basename, lineno, token[0], 2);

			linegroup_t * lg = new linegroup_t;

			lg->group = token[1][0];
			lg->desc  = token[2];

			line_groups[lg->group] = lg;
		}

		else if (y_stricmp(token[0], "line") == 0)
		{
			linetype_t * info = new linetype_t;

			if (ntoks != 4)
				FatalError(bad_arg_count, basename, lineno, token[0], 3);

			int number = atoi(token[1]);

			 info->group = token[2][0];
			 info->desc  = token[3];

			if (line_groups.find( info->group) == line_groups.end())
			{
				LogPrintf("%s(%d): unknown line group '%c'\n",
						basename, lineno,  info->group);
			}
			else
				line_types[number] = info;
		}

		else if (y_stricmp(token[0], "sector") == 0)
		{
			if (ntoks != 3)
				FatalError(bad_arg_count, basename, lineno, token[0], 2);

			int number = atoi(token[1]);

			sectortype_t *info = new sectortype_t;

			info->desc = token[2];

			sector_types[number] = info;
		}

		else if (y_stricmp(token[0], "thinggroup") == 0)
		{
			if (ntoks != 4)
				FatalError(bad_arg_count, basename, lineno, token[0], 3);

			thinggroup_t * tg = new thinggroup_t;

			tg->group = token[1][0];
			tg->color = ParseColor(token[2]);
			tg->desc  = token[3];

			thing_groups[tg->group] = tg;
		}

		else if (y_stricmp(token[0], "thing") == 0)
		{
			if (ntoks != 7)
				FatalError(bad_arg_count, basename, lineno, token[0], 7);

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
						basename, lineno, info->group);
			}
			else
			{	
				info->color = thing_groups[info->group]->color;

				thing_types[number] = info;
			}
		}

		else if (y_stricmp(token[0], "texturegroup") == 0)
		{
			if (ntoks != 3)
				FatalError(bad_arg_count, basename, lineno, token[0], 2);

			texturegroup_t * tg = new texturegroup_t;

			tg->group = token[1][0];
			tg->desc  = token[2];

			texture_groups[tg->group] = tg;
		}

		else if (y_stricmp(token[0], "texture") == 0)
		{
			if (ntoks != 3)
				FatalError(bad_arg_count, basename, lineno, token[0], 3);

			char group = token[1][0];
			std::string name = std::string(token[2]);

			if (texture_groups.find(tolower(group)) == texture_groups.end())
			{
				LogPrintf("%s(%d): unknown texture group '%c'\n",
						  basename, lineno, group);
			}
			else
				texture_assigns[name] = group;
		}

		else if (y_stricmp(token[0], "flat") == 0)
		{
			if (ntoks != 3)
				FatalError(bad_arg_count, basename, lineno, token[0], 3);

			char group = token[1][0];
			std::string name = std::string(token[2]);

			if (texture_groups.find(tolower(group)) == texture_groups.end())
			{
				LogPrintf("%s(%d): unknown texture group '%c'\n",
						basename, lineno, group);
			}
			else
				flat_assigns[name] = group;
		}

		else
		{
			FatalError("%s(%d): unknown directive: %.32s\n",
					   basename, lineno, token[0]);
		}
	}

	fclose(fp);

	/* Verify that all the mandatory directives are present. */

	if (yg_level_name == YGLN__)
		yg_level_name = YGLN_MAP01;
}


/*
 *  Free all memory allocated to game definitions
 */
void FreeDefinitions()
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


// result will be '|' separated (ready for Fl_Choice::add)
// returns the empty string when nothing found.
// The result should be freed with StringFree().
//
// will also find an existing name, storing its index in 'exist_val'
// (when not found, the value in 'exist_val' is not changed at all)

const char * M_CollectDefsForMenu(const char *folder, int *exist_val, const char *exist_name)
{
	std::vector<const char *> list;

	M_CollectKnownDefs(folder, list);

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

		// FIXME: potential for buffer overflow here
		strcat(buffer, "|");
		strcat(buffer, G->desc);

		letters[L_index++] = IT->first;
	}

	strcat(buffer, "|OTHER");

	letters[L_index++] = '-';
	letters[L_index++] = 0;

	return buffer;
}


const char *M_ThingCategoryString(char *letters)
{
	static char buffer[2000];

	int L_index = 0;

	// the "ALL" category is always first
	strcpy(buffer, "ALL");
	letters[L_index++] = '*';

	std::map<char, thinggroup_t *>::iterator IT;

	for (IT = thing_groups.begin() ; IT != thing_groups.end() ; IT++)
	{
		thinggroup_t *G = IT->second;

		// the "Other" category is always at the end
		if (G->group == '-')
			continue;

		// FIXME: potential for buffer overflow here
		strcat(buffer, "|");
		strcat(buffer, G->desc);

		letters[L_index++] = IT->first;
	}

	strcat(buffer, "|OTHER");

	letters[L_index++] = '-';
	letters[L_index++] = 0;

	return buffer;
}


const char *M_TextureCategoryString(char *letters)
{
	static char buffer[2000];

	int L_index = 0;

	// the "ALL" category is always first
	strcpy(buffer, "ALL");
	letters[L_index++] = '*';

	std::map<char, texturegroup_t *>::iterator IT;

	for (IT = texture_groups.begin() ; IT != texture_groups.end() ; IT++)
	{
		texturegroup_t *G = IT->second;

		// the "Other" category is always at the end
		if (G->group == '-')
			continue;

		// FIXME: potential for buffer overflow here
		strcat(buffer, "|");
		strcat(buffer, G->desc);

		letters[L_index++] = IT->first;
	}

	strcat(buffer, "|OTHER");

	letters[L_index++] = '-';
	letters[L_index++] = 0;

	return buffer;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
