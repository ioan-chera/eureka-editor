//------------------------------------------------------------------------
//  GAME DEFINITION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
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

#include "Instance.h"
#include "m_streams.h"

#define UNKNOWN_THING_RADIUS  16
#define UNKNOWN_THING_COLOR   fl_rgb_color(0,255,255)

namespace global
{
	// all the game and port definitions and previously loaded
	static std::unordered_map<SString, GameInfo> sLoadedGameDefs;
	static std::map<SString, PortInfo_c *> loaded_port_defs;

	// the information being loaded for PURPOSE_GameInfo / PortInfo
	// TODO : move into parser_state_c
	static PortInfo_c *loading_Port;
}

void PortInfo_c::AddSupportedGame(const SString &game)
{
	if (! SupportsGame(game))
		supported_games.push_back(game);
}

bool PortInfo_c::SupportsGame(const SString &game) const
{
	for (const SString &supportedGame : supported_games)
		if (supportedGame.noCaseEqual(game))
			return true;

	return false;
}


void Instance::M_FreeAllDefinitions()
{
	// TODO: prevent memory leak, delete contents of these maps

    line_groups.clear();
    line_types.clear();
    sector_types.clear();

    thing_groups.clear();
    thing_types.clear();

	texture_groups.clear();
	texture_categories.clear();
	flat_categories.clear();
}


//
//  this is called each time the full set of definitions
//  (game, port, resource files) are loaded.
//
void Instance::M_ClearAllDefinitions()
{
	M_FreeAllDefinitions();

	Misc_info = misc_info_t();
	// TODO: #58
	Features = {};

	Misc_info.player_r = 16;
	Misc_info.player_h = 56;
	Misc_info.view_height = 41;
	Misc_info.min_dm_starts = 4;
	Misc_info.max_dm_starts = 10;

	// reset generalized types
	for(generalized_linetype_t &type : gen_linetypes)
		type = {};
	num_gen_linetypes = 0;
}


void Instance::M_PrepareConfigVariables()
{
	parse_vars.clear();

	switch (Level_format)
	{
		case MapFormat::doom:
			parse_vars["$MAP_FORMAT"] = "DOOM";
			break;

		case MapFormat::hexen:
			parse_vars["$MAP_FORMAT"] = "HEXEN";
			break;

		case MapFormat::udmf:
			parse_vars["$MAP_FORMAT"] = "UDMF";
			break;

		default:
			break;
	}

	if (!Udmf_namespace.empty())
	{
		parse_vars["$UDMF_NAMESPACE"] = Udmf_namespace;
	}

	if (!Game_name.empty())
	{
		parse_vars["$GAME_NAME"] = Game_name;

		if (M_CanLoadDefinitions("games", Game_name))
		{
			SString base_game = M_GetBaseGame(*this, Game_name);
			parse_vars["$BASE_GAME"] = base_game;
		}
	}

	if (!Port_name.empty())
	{
		parse_vars["$PORT_NAME"] = Port_name;
	}
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


static void ParseColorDef(Instance &inst, char ** argv, int argc)
{
	if (y_stricmp(argv[0], "sky") == 0)
	{
		inst.Misc_info.sky_color = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "wall") == 0)
	{
		inst.Misc_info.wall_colors[0] = atoi(argv[1]);
		inst.Misc_info.wall_colors[1] = atoi(argv[(argc < 2) ? 1 : 2]);
	}
	else if (y_stricmp(argv[0], "floor") == 0)
	{
		inst.Misc_info.floor_colors[0] = atoi(argv[1]);
		inst.Misc_info.floor_colors[1] = atoi(argv[(argc < 2) ? 1 : 2]);
	}
	else if (y_stricmp(argv[0], "invis") == 0)
	{
		inst.Misc_info.invis_colors[0] = atoi(argv[1]);
		inst.Misc_info.invis_colors[1] = atoi(argv[(argc < 2) ? 1 : 2]);
	}
	else if (y_stricmp(argv[0], "missing") == 0)
	{
		inst.Misc_info.missing_color = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "unknown_tex") == 0)
	{
		inst.Misc_info.unknown_tex = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "unknown_flat") == 0)
	{
		inst.Misc_info.unknown_flat = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "unknown_thing") == 0)
	{
		inst.Misc_info.unknown_thing = atoi(argv[1]);
	}
	else
	{
		gLog.printf("unknown color keyword: '%s'\n", argv[0]);
	}
}


static map_format_bitset_t ParseMapFormats(char ** argv, int argc)
{
	map_format_bitset_t result = 0;

	for ( ; argc > 0 ; argv++, argc--)
	{
		if (y_stricmp(argv[0], "DOOM") == 0)
			result |= (1 << static_cast<int>(MapFormat::doom));

		else if (y_stricmp(argv[0], "HEXEN") == 0)
			result |= (1 << static_cast<int>(MapFormat::hexen));

		else if (y_stricmp(argv[0], "UDMF") == 0)
			result |= (1 << static_cast<int>(MapFormat::udmf));

		else
			ThrowException("Unknown map format '%s' in definition file.\n", argv[0]);
	}

	return result;
}


static void ParseClearKeywords(Instance &inst, char ** argv, int argc)
{
	for ( ; argc > 0 ; argv++, argc--)
	{
		if (y_stricmp(argv[0], "lines") == 0)
		{
			inst.line_groups.clear();
			inst.line_types.clear();
		}
		else if (y_stricmp(argv[0], "sectors") == 0)
		{
			inst.sector_types.clear();
		}
		else if (y_stricmp(argv[0], "things") == 0)
		{
			inst.thing_groups.clear();
			inst.thing_types.clear();
		}
		else if (y_stricmp(argv[0], "textures") == 0)
		{
			inst.texture_groups.clear();
			inst.texture_categories.clear();

			inst.flat_categories.clear();
		}
		else
			ThrowException("Unknown clear keyword '%s' in definition file.\n", argv[0]);
	}
}

//
// Mapping from UGH to value
//
struct FeatureMapping
{
	const char *const name;
	int port_features_t::*field;
	int (*converter)(const char *);
};

//
// Text
//
static int GenSectorConvert(const char *text)
{
	if(!y_stricmp(text, "boom"))
		return static_cast<int>(GenSectorFamily::boom);
	if(!y_stricmp(text, "zdoom"))
		return static_cast<int>(GenSectorFamily::zdoom);
	return static_cast<int>(GenSectorFamily::none);
}

//
// The available feature mappings
//
static const FeatureMapping skFeatureMappings[] =
{
#define MAPPING(a) { #a, &port_features_t::a }
	MAPPING(gen_types),
	{ "gen_sectors", reinterpret_cast<int port_features_t::*>(&port_features_t::gen_sectors), GenSectorConvert },
	MAPPING(img_png),
	MAPPING(tx_start),
	MAPPING(coop_dm_flags),
	MAPPING(friend_flag),
	MAPPING(pass_through),
	{ "3d_midtex", &port_features_t::midtex_3d },
	MAPPING(strife_flags),
	MAPPING(medusa_fixed),
	MAPPING(lax_sprites),
	MAPPING(no_need_players),
	MAPPING(tag_666),
	MAPPING(mix_textures_flats),
	MAPPING(neg_patch_offsets),
	MAPPING(extra_floors),
	MAPPING(slopes),
#undef MAPPING
};

//
// Parses features
//
static void ParseFeatureDef(Instance &inst, char **argv, int argc)
{
	bool found = false;
	for(const FeatureMapping &mapping : skFeatureMappings)
		if(!y_stricmp(argv[0], mapping.name))
		{
			char *endptr = nullptr;
			long val = strtol(argv[1], &endptr, 10);
			if(endptr == argv[1] && mapping.converter)
				inst.Features.*mapping.field = mapping.converter(argv[1]);
			else
				inst.Features.*mapping.field = static_cast<int>(val);
			found = true;
			break;
		}
	if(!found)
		gLog.printf("unknown feature keyword: '%s'\n", argv[0]);
}


static SString FindDefinitionFile(const SString &folder, const SString &name)
{
	SYS_ASSERT(folder.good() && name.good());
	for (int pass = 0 ; pass < 2 ; pass++)
	{
		const SString &base_dir = (pass == 0) ? global::home_dir :
				global::install_dir;

		if (base_dir.empty())
			continue;

		SString filename = base_dir + "/" + folder + "/" + name + ".ugh";

		gLog.debugPrintf("  trying: %s\n", filename.c_str());

		if (FileExists(filename))
			return filename;
	}

	return "";
}


bool M_CanLoadDefinitions(const SString &folder, const SString &name)
{
	SString filename = FindDefinitionFile(folder, name);

	return !filename.empty();
}


//
// Loads a definition file.  The ".ugh" extension is added.
// Will try the "common" folder if not found in the given one.
//
// Examples: "games" + "doom2"
//           "ports" + "edge"
//
void Instance::M_LoadDefinitions(const SString &folder, const SString &name)
		noexcept(false)
{
	// this is for error messages & debugging
	SString prettyname = folder + "/" + name + ".ugh";

	gLog.printf("Loading Definitions : %s\n", prettyname.c_str());

	SString filename = FindDefinitionFile(folder, name);

	if (filename.empty())
		ThrowException("Cannot find definition file: %s\n", prettyname.c_str());

	gLog.debugPrintf("  found at: %s\n", filename.c_str());

	M_ParseDefinitionFile(*this, PURPOSE_Normal, nullptr, filename, folder,
						  prettyname);
}


enum parsing_condition_e
{
	PCOND_NONE		= 0,
	PCOND_Reading	= 1,
	PCOND_Skipping	= 2
};

struct parsing_cond_state_t
{
	parsing_condition_e cond;
	int start_line;

	void Toggle()
	{
		cond = (cond == PCOND_Reading) ? PCOND_Skipping : PCOND_Reading;
	}
};


#define MAX_TOKENS  30   /* tokens per line */

#define MAX_INCLUDE_LEVEL  10

class parser_state_c
{
public:
	// current line number
	int lineno = 0;

	// filename for error messages (lacks the directory)
	SString fname = nullptr;

	// buffer containing the raw line
	SString readstring;

	// buffer storing the tokens
	char tokenbuf[512];

	// the line parsed into tokens
	int    argc = 0;
	char * argv[MAX_TOKENS] = {};

	// state for handling if/else/endif
	std::vector<parsing_cond_state_t> cond_stack;

	// BOOM generalized linedef stuff
	int current_gen_line = -1;

public:
	bool HaveAnySkipping() const
	{
		for (size_t i = 0 ; i < cond_stack.size() ; i++)
			if (cond_stack[i].cond == PCOND_Skipping)
				return true;

		return false;
	}
};


static const char *const bad_arg_count =
		"%s(%d): directive \"%s\" takes %d parameters\n";


static void M_TokenizeLine(parser_state_c *pst)
{
	// break the line into whitespace-separated tokens.
	// whitespace can be enclosed in double quotes.

	size_t srcpos = 0;
	char		*dest = pst->tokenbuf;

	bool		in_token = false;
	bool		quoted   = false;

	pst->argc = 0;

	for ( ; ; srcpos++)
	{
		char srcc = pst->readstring[srcpos];
		if (srcc == 0 || srcc == '\n')
		{
			if (in_token)
				*dest = 0;
			break;
		}

		if (srcc == '"')
		{
			quoted = !quoted;
			continue;
		}

		// found a comment?   [ we allow # in the middle of a token ]
		if (srcc == '#' && !in_token && !quoted)
			break;

		// beginning a new token?
		if (!in_token && (quoted || !isspace(srcc)))
		{
			if (pst->argc >= MAX_TOKENS)
				ThrowException("%s(%d): more than %d tokens on the line\n",
							   pst->fname.c_str(), pst->lineno, MAX_TOKENS);

			in_token = true;

			pst->argv[pst->argc++] = dest;

			*dest++ = srcc;
			continue;
		}

		// whitespace will end a token (unless quoted)
		if (isspace(srcc) && in_token && !quoted)
		{
			in_token = false;
			*dest++  = 0;
			continue;
		}

		// normal token character?
		if (in_token)
			*dest++ = srcc;
	}

	if (quoted)
		ThrowException("%s(%d): unmatched double quote\n", pst->fname.c_str(), pst->lineno);
}


static void M_ParseNormalLine(Instance &inst, parser_state_c *pst)
{
	char **argv  = pst->argv;
	int    nargs = pst->argc - 1;


	// these are handled by other passes
	if (y_stricmp(argv[0], "base_game") == 0 ||
		y_stricmp(argv[0], "map_formats") == 0 ||
		y_stricmp(argv[0], "supported_games") == 0 ||
		y_stricmp(argv[0], "udmf_namespace") == 0)
	{
		return;
	}


	if (y_stricmp(argv[0], "player_size") == 0)
	{
		if (nargs != 3)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 1);

		inst.Misc_info.player_r    = atoi(argv[1]);
		inst.Misc_info.player_h    = atoi(argv[2]);
		inst.Misc_info.view_height = atoi(argv[3]);
	}
	else if (y_stricmp(argv[0], "sky_color") == 0)  // back compat
	{
		if (nargs != 1)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 1);

		inst.Misc_info.sky_color = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "sky_flat") == 0)
	{
		if (nargs != 1)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 1);

		inst.Misc_info.sky_flat = argv[1];
	}
	else if (y_stricmp(argv[0], "color") == 0)
	{
		if (nargs < 2)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 2);

		ParseColorDef(inst, pst->argv + 1, nargs);
	}
	else if (y_stricmp(argv[0], "feature") == 0)
	{
		if (nargs < 2)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 2);

		ParseFeatureDef(inst, pst->argv + 1, nargs);
	}
	else if (y_stricmp(argv[0], "default_textures") == 0)
	{
		if (nargs != 3)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 3);

		inst.default_wall_tex	= argv[1];
		inst.default_floor_tex	= argv[2];
		inst.default_ceil_tex	= argv[3];
	}
	else if (y_stricmp(argv[0], "default_thing") == 0)
	{
		if (nargs != 1)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 1);

		inst.default_thing = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "linegroup") == 0 ||
			 y_stricmp(argv[0], "spec_group") == 0)
	{
		if (nargs != 2)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 2);

		linegroup_t lg = {};

		lg.group = argv[1][0];
		lg.desc  = argv[2];

		inst.line_groups[lg.group] = lg;
	}

	else if (y_stricmp(argv[0], "line") == 0 ||
			 y_stricmp(argv[0], "special") == 0)
	{
		if (nargs < 3)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 3);

		linetype_t info = {};

		int number = atoi(argv[1]);

		info.group = argv[2][0];
		info.desc  = argv[3];

		int arg_count = std::min(nargs - 3, 5);
		for (int i = 0 ; i < arg_count ; i++)
		{
			if (argv[4 + i][0] != '-')
				info.args[i] = argv[4 + i];
		}

		if (inst.line_groups.find( info.group) == inst.line_groups.end())
		{
			gLog.printf("%s(%d): unknown line group '%c'\n",
					  pst->fname.c_str(), pst->lineno,  info.group);
		}
		else
			inst.line_types[number] = info;
	}

	else if (y_stricmp(argv[0], "sector") == 0)
	{
		if (nargs != 2)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 2);

		int number = atoi(argv[1]);

		sectortype_t info = {};

		info.desc = argv[2];

		inst.sector_types[number] = info;
	}

	else if (y_stricmp(argv[0], "thinggroup") == 0)
	{
		if (nargs != 3)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 3);

		thinggroup_t tg = {};

		tg.group = argv[1][0];
		tg.color = ParseColor(argv[2]);
		tg.desc  = argv[3];

		inst.thing_groups[tg.group] = tg;
	}

	else if (y_stricmp(argv[0], "thing") == 0)
	{
		if (nargs < 6)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 6);

		thingtype_t info = {};

		int number = atoi(argv[1]);

		info.group  = argv[2][0];
		info.flags  = ParseThingdefFlags(argv[3]);
		info.radius = static_cast<short>(atoi(argv[4]));
		info.sprite = argv[5];
		info.desc   = argv[6];
		info.scale  = static_cast<float>((nargs >= 7) ? atof(argv[7]) : 1.0);

		int arg_count = std::min(nargs - 7, 5);
		for (int i = 0 ; i < arg_count ; i++)
		{
			if (argv[8 + i][0] != '-')
				info.args[i] = argv[8 + i];
		}

		if (inst.thing_groups.find(info.group) == inst.thing_groups.end())
		{
			gLog.printf("%s(%d): unknown thing group '%c'\n",
					  pst->fname.c_str(), pst->lineno, info.group);
		}
		else
		{
			info.color = inst.thing_groups[info.group].color;

			inst.thing_types[number] = info;
		}
	}

	else if (y_stricmp(argv[0], "texturegroup") == 0)
	{
		if (nargs != 2)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 2);

		texturegroup_t tg = {};

		tg.group = argv[1][0];
		tg.desc  = argv[2];

		inst.texture_groups[tg.group] = tg;
	}

	else if (y_stricmp(argv[0], "texture") == 0)
	{
		if (nargs != 2)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 2);

		char group = argv[1][0];
		SString name = SString(argv[2]);

		if (inst.texture_groups.find((char)tolower(group)) == inst.texture_groups.end())
		{
			gLog.printf("%s(%d): unknown texture group '%c'\n",
					  pst->fname.c_str(), pst->lineno, group);
		}
		else
			inst.texture_categories[name] = group;
	}

	else if (y_stricmp(argv[0], "flat") == 0)
	{
		if (nargs != 2)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 2);

		char group = argv[1][0];
		SString name = SString(argv[2]);

		if (inst.texture_groups.find((char)tolower(group)) == inst.texture_groups.end())
		{
			gLog.printf("%s(%d): unknown texture group '%c'\n",
					  pst->fname.c_str(), pst->lineno, group);
		}
		else
			inst.flat_categories[name] = group;
	}

	else if (y_stricmp(argv[0], "gen_line") == 0)
	{
		if (nargs != 4)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 4);

		pst->current_gen_line = inst.num_gen_linetypes;
		inst.num_gen_linetypes++;

		if (inst.num_gen_linetypes > MAX_GEN_NUM_TYPES)
			ThrowException("%s(%d): too many gen_line definitions\n", pst->fname.c_str(), pst->lineno);

		generalized_linetype_t *def = &inst.gen_linetypes[pst->current_gen_line];

		def->key = argv[1][0];

		// use strtol() to support "0x" notation
		def->base   = static_cast<int>(strtol(argv[2], NULL, 0));
		def->length = static_cast<int>(strtol(argv[3], NULL, 0));

		def->name = argv[4];
		def->num_fields = 0;
	}

	else if (y_stricmp(argv[0], "gen_field") == 0)
	{
		if (nargs < 5)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 5);

		if (pst->current_gen_line < 0)
			ThrowException("%s(%d): gen_field used outside of a gen_line definition\n", pst->fname.c_str(), pst->lineno);

		generalized_linetype_t *def = &inst.gen_linetypes[pst->current_gen_line];

		generalized_field_t *field = &def->fields[def->num_fields];

		def->num_fields++;
		if (def->num_fields > MAX_GEN_NUM_FIELDS)
			ThrowException("%s(%d): too many fields in gen_line definition\n", pst->fname.c_str(), pst->lineno);

		field->bits  = atoi(argv[1]);
		field->shift = atoi(argv[2]);

		field->mask  = ((1 << field->bits) - 1) << field->shift;

		field->default_val = atoi(argv[3]);

		field->name = argv[4];

		// grab the keywords
		field->num_keywords = std::min(nargs - 4, MAX_GEN_FIELD_KEYWORDS);

		for (int i = 0 ; i < field->num_keywords ; i++)
		{
			field->keywords[i] = argv[5 + i];
		}
	}
	else if (y_stricmp(argv[0], "clear") == 0)
	{
		if (nargs < 1)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 2);

		ParseClearKeywords(inst, pst->argv + 1, nargs);
	}

/*  FIXME

	else
	{
		FatalError("%s(%d): unknown directive: %.32s\n",
				   pst->fname, pst->lineno, argv[0]);
	}
*/
}


static void M_ParseGameInfoLine(parser_state_c *pst, GameInfo &loadingGame)
{
	char **argv  = pst->argv;
	int    nargs = pst->argc - 1;

	if (y_stricmp(argv[0], "map_formats") == 0 ||
		y_stricmp(argv[0], "supported_games") == 0 ||
		y_stricmp(argv[0], "udmf_namespace") == 0)
	{
		ThrowException("%s(%d): %s can only be used in port definitions\n",
					   pst->fname.c_str(), pst->lineno, argv[0]);
	}

	if (y_stricmp(argv[0], "base_game") == 0)
	{
		if (nargs < 1)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 1);

		loadingGame.baseGame = SString(argv[1]).asLower();
	}
}


static void M_ParsePortInfoLine(parser_state_c *pst)
{
	char **argv  = pst->argv;
	int    nargs = pst->argc - 1;

	if (y_stricmp(argv[0], "base_game") == 0)
	{
		ThrowException("%s(%d): %s can only be used in game definitions\n",
					   pst->fname.c_str(), pst->lineno, argv[0]);
	}

	if (y_stricmp(argv[0], "supported_games") == 0)
	{
		if (nargs < 1)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 1);

		for (argv++ ; nargs > 0 ; argv++, nargs--)
			global::loading_Port->AddSupportedGame(SString(*argv).asLower());
	}
	else if (y_stricmp(argv[0], "map_formats") == 0)
	{
		if (nargs < 1)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 1);

		global::loading_Port->formats = ParseMapFormats(argv + 1, nargs);
	}
	else if (y_stricmp(argv[0], "udmf_namespace") == 0)
	{
		if (nargs != 1)
			ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, argv[0], 1);

		// want to preserve the case here
		global::loading_Port->udmf_namespace = argv[1];
	}
}


static bool M_ParseConditional(const Instance &inst, parser_state_c *pst)
{
	// returns the result of the "IF" test, true or false.

	char **argv  = pst->argv + 1;
	int    nargs = pst->argc - 1;

	bool op_is  = (nargs >= 3 && y_stricmp(argv[1], "is")  == 0);
	bool op_not = (nargs >= 3 && y_stricmp(argv[1], "not") == 0);

	if (op_is || op_not)
	{
		if (strlen(argv[0]) < 2 || argv[0][0] != '$')
			ThrowException("%s(%d): expected variable in if statement\n",
						   pst->fname.c_str(), pst->lineno);

		// tokens are stored in pst->tokenbuf, so this is OK
		y_strupr(argv[0]);

		auto it = inst.parse_vars.find(argv[0]);
		if(it != inst.parse_vars.end())
		{
			const SString &var_value = it->second;

			// test multiple values, only need one to succeed
			for (int i = 2 ; i < nargs ; i++)
				if (var_value.noCaseEqual(argv[i]))
					return op_is;
		}

		return op_not;
	}

	ThrowException("%s(%d): syntax error in if statement\n", pst->fname.c_str(),
				   pst->lineno);
	return false;
}


static void M_ParseSetVar(Instance &inst, parser_state_c *pst)
{
	char **argv  = pst->argv + 1;
	int    nargs = pst->argc - 1;

	if (nargs != 2)
		ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno,
					   pst->argv[0], 1);

	if (strlen(argv[0]) < 2 || argv[0][0] != '$')
		ThrowException("%s(%d): variable name too short or lacks '$' prefix\n",
					   pst->fname.c_str(), pst->lineno);

	// tokens are stored in pst->tokenbuf, so this is OK
	y_strupr(argv[0]);

	inst.parse_vars[argv[0]] = argv[1];
}


//
//  this is main function for parsing a definition file.
//
//  when purpose is PURPOSE_GameInfo or PURPOSE_PortInfo, then
//  only minimal parsing occurs, in particular the "include", "set"
//  and "if".."endif" directives are NOT handled.
//
void M_ParseDefinitionFile(Instance &inst,
						   const parse_purpose_e purpose,
						   void *target,
						   const SString &filename,
						   const SString &cfolder,
						   const SString &cprettyname,
						   int include_level)
{
	SYS_ASSERT(filename.good());

	SString folder = cfolder;
	if (folder.empty())
		folder = "common";

	SString prettyname = cprettyname;
	if (prettyname.empty())
		prettyname = fl_filename_name(filename.c_str());

	parser_state_c parser_state;

	// this is a bit silly, but makes it easier to move code around
	parser_state_c *const pst = &parser_state;

	pst->fname = prettyname;

	// read the definition file, line by line

	LineFile file(filename);
	if (! file)
		ThrowException("Cannot open %s: %s\n", filename.c_str(), GetErrorMessage(errno).c_str());

	while (file.readLine(pst->readstring))
	{
		pst->lineno += 1;

		M_TokenizeLine(pst);

		// skip empty lines and comments
		if (pst->argc == 0)
			continue;

		int nargs = pst->argc - 1;


		// handle conditionals: if...else...endif

		if (y_stricmp(pst->argv[0], "if") == 0)
		{
			parsing_cond_state_t cst;

			cst.cond = M_ParseConditional(inst, pst) ? PCOND_Reading :
					PCOND_Skipping;
			cst.start_line = pst->lineno;

			pst->cond_stack.push_back(cst);
			continue;
		}
		if (y_stricmp(pst->argv[0], "else") == 0)
		{
			if (pst->cond_stack.empty())
				ThrowException("%s(%d): else without if\n", pst->fname.c_str(), pst->lineno);

			// toggle the mode
			pst->cond_stack.back().Toggle();
			continue;
		}
		if (y_stricmp(pst->argv[0], "endif") == 0)
		{
			if (pst->cond_stack.empty())
				ThrowException("%s(%d): endif without if\n", pst->fname.c_str(), pst->lineno);

			pst->cond_stack.pop_back();
			continue;
		}


		// skip lines when ANY if statement is in skip mode
		if (pst->HaveAnySkipping())
			continue;


		// handle setting variables
		if (y_stricmp(pst->argv[0], "set") == 0)
		{
			M_ParseSetVar(inst, pst);
			continue;
		}


		// handle includes
		if (y_stricmp(pst->argv[0], "include") == 0)
		{
			if (nargs != 1)
				ThrowException(bad_arg_count, pst->fname.c_str(), pst->lineno, pst->argv[0], 1);

			if (include_level >= MAX_INCLUDE_LEVEL)
				ThrowException("%s(%d): Too many includes (check for a loop)\n",
							   pst->fname.c_str(), pst->lineno);

			SString new_folder = folder;
			SString new_name = FindDefinitionFile(new_folder, pst->argv[1]);

			// if not found, check the common/ folder
			if (new_name.empty() && folder != "common")
			{
				new_folder = "common";
				new_name = FindDefinitionFile(new_folder, pst->argv[1]);
			}

			if (new_name.empty())
				ThrowException("%s(%d): Cannot find include file: %s.ugh\n",
							   pst->fname.c_str(), pst->lineno, pst->argv[1]);

			M_ParseDefinitionFile(inst, purpose, target, new_name, new_folder,
								  NULL /* prettyname */,
								  include_level + 1);
			continue;
		}


		if (purpose == PURPOSE_GameInfo)
		{
			M_ParseGameInfoLine(pst, *static_cast<GameInfo *>(target));
			continue;
		}
		if (purpose == PURPOSE_PortInfo)
		{
			M_ParsePortInfoLine(pst);
			continue;
		}

		// handle everything else
		M_ParseNormalLine(inst, pst);
	}

	// check for an unterminated conditional
	if (! pst->cond_stack.empty())
	{
		ThrowException("%s(%d): Missing endif statement\n", pst->fname.c_str(),
			pst->cond_stack.back().start_line);
	}
}

//
// Load game info
//
static GameInfo M_LoadGameInfo(Instance &inst, const SString &game)
		noexcept(false)
{
	// already loaded?
	auto it = global::sLoadedGameDefs.find(game);
	if(it != global::sLoadedGameDefs.end())
		return it->second;

	SString filename = FindDefinitionFile("games", game);
	if(filename.empty())
		return {};
	GameInfo loadingGame = GameInfo(game);
	M_ParseDefinitionFile(inst, PURPOSE_GameInfo, &loadingGame, filename,
						  "games", nullptr);
	if(loadingGame.baseGame.empty())
		ThrowException("Game definition for '%s' does not set base_game\n",
					   game.c_str());

	global::sLoadedGameDefs[game] = loadingGame;
	return loadingGame;
}


PortInfo_c * M_LoadPortInfo(Instance &inst, const SString &port)
{
	std::map<SString, PortInfo_c *>::iterator IT;
	IT = global::loaded_port_defs.find(port);

	if (IT != global::loaded_port_defs.end())
		return IT->second;

	SString filename = FindDefinitionFile("ports", port);
	if (filename.empty())
		return NULL;

	global::loading_Port = new PortInfo_c(port);

	M_ParseDefinitionFile(inst, PURPOSE_PortInfo, nullptr, filename, "ports",
						  NULL);

	// default is to support both Doom and Doom2
	if (global::loading_Port->supported_games.empty())
	{
		global::loading_Port->supported_games.push_back("doom");
		global::loading_Port->supported_games.push_back("doom2");
	}

	global::loaded_port_defs[port] = global::loading_Port;
	return global::loading_Port;
}


//------------------------------------------------------------------------

//
// Collect known definitions from folder
//
std::vector<SString> M_CollectKnownDefs(const char *folder)
{
	SYS_ASSERT(!!folder);

	std::vector<SString> temp_list;
	SString path;

	//	gLog.debugPrintf("M_CollectKnownDefs for: %d\n", folder);
	auto scanner_add_file = [&temp_list](const SString &name, int flags)
	{
		if (flags & (SCAN_F_IsDir | SCAN_F_Hidden))
			return;
		if (! MatchExtension(name, "ugh"))
			return;
		temp_list.push_back(ReplaceExtension(name, NULL));
	};
	path = global::install_dir + "/" + folder;
	ScanDirectory(path, scanner_add_file);
	path = global::home_dir + "/" + folder;
	ScanDirectory(path, scanner_add_file);

	std::sort(temp_list.begin(), temp_list.end(), [](const SString &a, const SString &b)
			  {
		return a.noCaseCompare(b) < 0;
	});

	// transfer to passed list, removing duplicates as we go
	size_t pos;

	// FIXME: use some <algorithm> thing here
	std::vector<SString> list;
	list.reserve(temp_list.size());
	for (pos = 0 ; pos < temp_list.size() ; pos++)
	{
		if (pos + 1 < temp_list.size() && temp_list[pos].noCaseEqual(temp_list[pos + 1]))
		{
			continue;
		}

		list.push_back(temp_list[pos]);
	}
	return list;

}

SString M_GetBaseGame(Instance &inst, const SString &game)
{
	GameInfo ginfo = M_LoadGameInfo(inst, game);
	SYS_ASSERT(ginfo);

	return ginfo.baseGame;
}


map_format_bitset_t M_DetermineMapFormats(Instance &inst, const char *game,
										  const char *port)
{
	PortInfo_c *pinfo = M_LoadPortInfo(inst, port);
	if (pinfo && pinfo->formats != 0)
		return pinfo->formats;

	// a bit hacky, maybe should do it a better way...
	if (strcmp(game, "hexen") == 0)
		return (1 << static_cast<int>(MapFormat::hexen));

	return (1 << static_cast<int>(MapFormat::doom));
}


bool M_CheckPortSupportsGame(Instance &inst, const SString &base_game,
							 const SString &port)
{
	if (port == "vanilla")
	{
		// Vanilla means the engine that comes with the game, hence
		// it supports everything.
		return true;
	}

	PortInfo_c *pinfo = M_LoadPortInfo(inst, port);
	if (! pinfo)
		return false;

	return pinfo->SupportsGame(base_game);
}


// find all the ports which support the given base game..
//
// result will be '|' separated (ready for Fl_Choice::add)
// returns the empty string when nothing found.
// The result should be freed with StringFree().
//
// will also find an existing name, storing its index in 'exist_val'
// (when not found, the value in 'exist_val' is not changed at all)

SString M_CollectPortsForMenu(Instance &inst, const char *base_game,
							  int *exist_val, const char *exist_name)
{
	std::vector<SString> list = M_CollectKnownDefs("ports");

	if (list.empty())
		return "";

	// determine final length
	int length = 2 + (int)list.size();
	unsigned int i;

	for (i = 0 ; i < list.size() ; i++)
		length += list[i].length();

	SString result;
	result.reserve(length);

	int entry_id = 0;

	for (i = 0 ; i < list.size() ; i++)
	{
		if (! M_CheckPortSupportsGame(inst, base_game, list[i]))
			continue;

		if (result[0])
			result += '|';

		result += list[i];

		if (list[i].noCaseEqual(exist_name))
			*exist_val = entry_id;

		entry_id++;
	}

//	gLog.debugPrintf( "RESULT = '%s'\n", result);

	return result;
}


//------------------------------------------------------------------------

// is this flat a sky?
bool Instance::is_sky(const SString &flat) const
{
	return flat.noCaseEqual(Misc_info.sky_flat);
}

bool is_null_tex(const SString &tex)
{
	return tex.good() && tex[0] == '-';
}

bool is_special_tex(const SString &tex)
{
	return tex.good() && tex[0] == '#';
}


const sectortype_t &Instance::M_GetSectorType(int type) const
{
	std::map<int, sectortype_t>::const_iterator SI;

	SI = sector_types.find(type);

	if (SI != sector_types.end())
		return SI->second;

	static sectortype_t dummy_type =
	{
		"UNKNOWN TYPE"
	};

	return dummy_type;
}


const linetype_t &Instance::M_GetLineType(int type) const
{
	std::map<int, linetype_t>::const_iterator LI;

	LI = line_types.find(type);

	if (LI != line_types.end())
		return LI->second;

	static linetype_t dummy_type =
	{
		0, "UNKNOWN TYPE"
	};

	return dummy_type;
}


const thingtype_t &Instance::M_GetThingType(int type) const
{
	auto TI = thing_types.find(type);

	if (TI != thing_types.end())
		return TI->second;

	static thingtype_t dummy_type =
	{
		0, 0, UNKNOWN_THING_RADIUS, 1.0f,
		"UNKNOWN TYPE", "NULL",
		UNKNOWN_THING_COLOR
	};

	return dummy_type;
}


char Instance::M_GetTextureType(const SString &name) const
{
	std::map<SString, char>::const_iterator TI;

	TI = texture_categories.find(name);

	if (TI != texture_categories.end())
		return TI->second;

	return '-';  // the OTHER category
}


char Instance::M_GetFlatType(const SString &name) const
{
	std::map<SString, char>::const_iterator TI;

	TI = flat_categories.find(name);

	if (TI != flat_categories.end())
		return TI->second;

	return '-';  // the OTHER category
}

//
// Generic for types
//
template<typename T>
static bool Category_IsUsed(const std::map<int, T> &types, char group)
{
	for (const auto &type : types)
		if(type.second.group == group)
			return true;
	return false;
}
static bool Category_IsUsed(const std::map<SString, char> &categories, char group)
{
	for (const auto &category : categories)
		if(category.second == group)
			return true;
	return false;
}

//
// Produces the category menu string and its associated letters
//
template<typename Group, typename Categories>
static SString M_CategoryString(SString &letters, bool recent, const std::map<char, Group> &groups, const Categories &categories)
{
	SString buffer;
	buffer.reserve(2000);

	// the "ALL" category is always first
	buffer = "ALL";
	letters = "*";
	letters.reserve(64);
	if(recent)
	{
		buffer += "|RECENT";
		letters.push_back('^');
	}

	typename std::map<char, Group>::const_iterator IT;

	for (IT = groups.begin() ; IT != groups.end() ; IT++)
	{
		const Group &G = IT->second;

		// the "Other" category is always at the end
		if (G.group == '-')
			continue;

		if (! Category_IsUsed(categories, G.group))
			continue;

		// FIXME: potential for buffer overflow here
		buffer += '|';
		buffer += G.desc;

		letters.push_back(IT->first);
	}

	buffer += "|Other";

	letters.push_back('-');

	return buffer;
}

SString Instance::M_LineCategoryString(SString &letters) const
{
	return M_CategoryString(letters, false, line_groups, line_types);
}


SString Instance::M_ThingCategoryString(SString &letters) const
{
	return M_CategoryString(letters, true, thing_groups, thing_types);
}


SString Instance::M_TextureCategoryString(SString &letters, bool do_flats) const
{
	return M_CategoryString(letters, true, texture_groups, do_flats ? flat_categories : texture_categories);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
