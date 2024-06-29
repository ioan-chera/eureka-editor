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
#include "m_parse.h"
#include "m_streams.h"
#include <assert.h>

#define MAX_TOKENS  30   /* tokens per line */

#define UNKNOWN_THING_RADIUS  16
#define UNKNOWN_THING_COLOR   fl_rgb_color(0,255,255)

namespace global
{
	// all the game and port definitions and previously loaded
	static std::unordered_map<SString, GameInfo> sLoadedGameDefs;
	static std::map<SString, PortInfo_c> loaded_port_defs;

	// the information being loaded for PURPOSE_GameInfo / PortInfo
	// TODO : move into parser_state_c
	static PortInfo_c loading_Port;
}

enum class ParsingCondition
{
	none,
	reading,
	skipping
};

struct parsing_cond_state_t
{
	ParsingCondition cond;
	int start_line;

	void Toggle()
	{
		cond = (cond == ParsingCondition::reading) ? ParsingCondition::skipping : ParsingCondition::reading;
	}
};

class parser_state_c
{
public:

	// the line parsed into tokens
	int    argc = 0;
	const char * argv[MAX_TOKENS] = {};

	// state for handling if/else/endif
	std::vector<parsing_cond_state_t> cond_stack;

	// BOOM generalized linedef stuff
	int current_gen_line = -1;

	explicit parser_state_c(const fs::path &filename) : fname(filename)
	{
	}

	bool HaveAnySkipping() const
	{
		for (size_t i = 0 ; i < cond_stack.size() ; i++)
			if (cond_stack[i].cond == ParsingCondition::skipping)
				return true;

		return false;
	}

	const fs::path &file() const
	{
		return fname;
	}

	bool readLine(LineFile &file)
	{
		bool result = file.readLine(readstring);
		if(result)
			++lineno;
		return result;
	}

	void tokenize();

	int line() const
	{
		return lineno;
	}

	void fail(EUR_FORMAT_STRING(const char *format), ...) const EUR_PRINTF(2, 3);

private:
	// filename for error messages (lacks the directory)
	const fs::path fname;

	// buffer containing the raw line
	SString readstring;

	// current line number
	int lineno = 0;

	std::vector<SString> args;
};

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


//
// Called only from Main_LoadResources
//
std::unordered_map<SString, SString> LoadingData::prepareConfigVariables() const
{
	std::unordered_map<SString, SString> parse_vars;

	switch (levelFormat)
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

	if (!udmfNamespace.empty())
	{
		parse_vars["$UDMF_NAMESPACE"] = udmfNamespace;
	}

	if (!gameName.empty())
	{
		parse_vars["$GAME_NAME"] = gameName;

		if (M_CanLoadDefinitions(global::home_dir, global::install_dir, GAMES_DIR, gameName))
		{
			SString base_game = M_GetBaseGame(gameName);
			parse_vars["$BASE_GAME"] = base_game;
		}
	}

	if (!portName.empty())
	{
		parse_vars["$PORT_NAME"] = portName;
	}
	return parse_vars;
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
    if (strchr(s, 'p')) flags |= THINGDEF_POLYSPOT;

	return flags;
}


static void ParseColorDef(ConfigData &config, const char ** argv, int argc)
{
	if (y_stricmp(argv[0], "sky") == 0)
	{
		config.miscInfo.sky_color = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "wall") == 0)
	{
		config.miscInfo.wall_colors[0] = atoi(argv[1]);
		config.miscInfo.wall_colors[1] = atoi(argv[(argc < 2) ? 1 : 2]);
	}
	else if (y_stricmp(argv[0], "floor") == 0)
	{
		config.miscInfo.floor_colors[0] = atoi(argv[1]);
		config.miscInfo.floor_colors[1] = atoi(argv[(argc < 2) ? 1 : 2]);
	}
	else if (y_stricmp(argv[0], "invis") == 0)
	{
		config.miscInfo.invis_colors[0] = atoi(argv[1]);
		config.miscInfo.invis_colors[1] = atoi(argv[(argc < 2) ? 1 : 2]);
	}
	else if (y_stricmp(argv[0], "missing") == 0)
	{
		config.miscInfo.missing_color = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "unknown_tex") == 0)
	{
		config.miscInfo.unknown_tex = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "unknown_flat") == 0)
	{
		config.miscInfo.unknown_flat = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "unknown_thing") == 0)
	{
		config.miscInfo.unknown_thing = atoi(argv[1]);
	}
	else
	{
		gLog.printf("unknown color keyword: '%s'\n", argv[0]);
	}
}


static map_format_bitset_t ParseMapFormats(parser_state_c *pst, const char ** argv,
										   int argc)
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
			pst->fail("Unknown map format '%s' in definition file.", argv[0]);
	}

	return result;
}


static void ParseClearKeywords(ConfigData &config, const char ** argv, int argc, parser_state_c *pst)
{
	for ( ; argc > 0 ; argv++, argc--)
	{
		if (y_stricmp(argv[0], "lines") == 0)
		{
			config.line_groups.clear();
			config.line_types.clear();
		}
		else if (y_stricmp(argv[0], "sectors") == 0)
		{
			config.sector_types.clear();
		}
		else if (y_stricmp(argv[0], "things") == 0)
		{
			config.thing_groups.clear();
			config.thing_types.clear();
		}
		else if (y_stricmp(argv[0], "textures") == 0)
		{
			config.texture_groups.clear();
			config.texture_categories.clear();

			config.flat_categories.clear();
		}
		else
			pst->fail("Unknown clear keyword '%s' in definition file.", argv[0]);
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
	MAPPING(tuttifrutti_fixed),
	MAPPING(lax_sprites),
	MAPPING(no_need_players),
	{ "tag_666", &port_features_t::tag_666raw },
	MAPPING(mix_textures_flats),
	MAPPING(neg_patch_offsets),
	MAPPING(extra_floors),
	MAPPING(slopes),
#undef MAPPING
};

//
// Parses features
//
static void ParseFeatureDef(ConfigData &config, const char **argv, int argc)
{
	bool found = false;
	for(const FeatureMapping &mapping : skFeatureMappings)
		if(!y_stricmp(argv[0], mapping.name))
		{
			char *endptr = nullptr;
			long val = strtol(argv[1], &endptr, 10);
			if(endptr == argv[1] && mapping.converter)
				config.features.*mapping.field = mapping.converter(argv[1]);
			else
				config.features.*mapping.field = static_cast<int>(val);
			found = true;
			break;
		}
	if(!found)
		gLog.printf("unknown feature keyword: '%s'\n", argv[0]);
}

//
// Finds a definition file in a given subpath under either the home or system install folder
// Both strings are required
// Returns "" if not found.
//
static tl::optional<fs::path> FindDefinitionFile(const fs::path &home_dir, const fs::path &install_dir, const fs::path &folder, const SString &name)
{
	SYS_ASSERT(!folder.empty() && name.good());
	const fs::path lookupDirs[] = { home_dir, install_dir };
	for (const fs::path &base_dir : lookupDirs)
	{
		if (base_dir.empty())
			continue;

		fs::path filename = base_dir / folder / (name + ".ugh").get();

		gLog.debugPrintf("  trying: %s\n", filename.u8string().c_str());

		if (FileExists(filename))
			return filename;
	}

	return {};
}


bool M_CanLoadDefinitions(const fs::path &home_dir, const fs::path &install_dir, const fs::path &folder, const SString &name)
{
	tl::optional<fs::path> filename = FindDefinitionFile(home_dir, install_dir, folder, name);

	return filename.has_value();
}

//
// Returns true if this is a polyobject source line
//
bool linetype_t::isPolyObjectSpecial() const
{
    for(const SpecialArg &arg : args)
        if(arg.type == SpecialArgType::po)
            return true;
    return false;
}


//
// Loads a definition file.  The ".ugh" extension is added.
// Will try the "common" folder if not found in the given one.
//
// Examples: "games" + "doom2"
//           "ports" + "edge"
//
void readConfiguration(std::unordered_map<SString, SString> &parse_vars,
					   const fs::path &folder, const SString &name,
					   ConfigData &config) noexcept(false)
{
	// this is for error messages & debugging
	fs::path prettyname = folder / fs::u8path((name + ".ugh").get());

	gLog.printf("Loading Definitions : %s\n", prettyname.u8string().c_str());

	tl::optional<fs::path> filename = FindDefinitionFile(global::home_dir, global::install_dir, folder, name);

	if (!filename)
	{
		throw ParseException(SString::printf("Cannot find definition file: %s",
											 prettyname.u8string().c_str()));
	}

	gLog.debugPrintf("  found at: %s\n", filename->u8string().c_str());

	M_ParseDefinitionFile(parse_vars, ParsePurpose::normal, &config, *filename,
						  folder, prettyname);
}

#define MAX_INCLUDE_LEVEL  10

//
// Fails with a ParseException
//
void parser_state_c::fail(EUR_FORMAT_STRING(const char *format), ...) const
{
	va_list ap;
	va_start(ap, format);
	SString ss = SString::vprintf(format, ap);
	va_end(ap);

	SString prefix = SString::printf("%s(%d): ", file().u8string().c_str(), line());
	throw ParseException(prefix + ss);
}

static const char *const bad_arg_count_fail = "directive \"%s\" takes %d parameters";


void parser_state_c::tokenize()
{
	// break the line into whitespace-separated tokens.
	// whitespace can be enclosed in double quotes.
	
	TokenWordParse parse(readstring, true);
	SString word;
	args.clear();
	argc = 0;
	while(parse.getNext(word))
	{
		if(args.size() >= MAX_TOKENS)
			fail("more than %d tokens on the line", MAX_TOKENS);
		args.push_back(word);
	}
	argc = (int)args.size();
	for(int i = 0; i < argc; ++i)
		argv[i] = args[i].c_str();	// safe if we lock the std::vector
}

//
// Parses an arg string. Will return true and populate arg if successful
//
static bool parseArg(const char *text, SpecialArg &arg)
{
    assert(!!text);
    const char *pos = strchr(text, ':');
    if(!pos)
    {
        arg.name = text;
        arg.type = SpecialArgType::generic;
        return true;
    }
    const char *type = pos + 1;
    // Either use whatever's before the colon, or the type name if empty.
    arg.name = pos == text ? type : SString(text, (int)(pos - text));
    if(!strcmp(type, "tag"))
        arg.type = SpecialArgType::tag;
    else if(!strcmp(type, "tag_hi"))
        arg.type = SpecialArgType::tag_hi;
    else if(!strcmp(type, "line_id"))
        arg.type = SpecialArgType::line_id;
    else if(!strcmp(type, "self_line_id"))
        arg.type = SpecialArgType::self_line_id;
    else if(!strcmp(type, "self_line_id_hi"))
        arg.type = SpecialArgType::self_line_id_hi;
    else if(!strcmp(type, "tid"))
        arg.type = SpecialArgType::tid;
    else if(!strcmp(type, "po"))
        arg.type = SpecialArgType::po;
    else    // error
        return false;
    return true;
}

static void M_ParseNormalLine(parser_state_c *pst, ConfigData &config)
{
	const char **argv  = pst->argv;
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
			pst->fail(bad_arg_count_fail, argv[0], 1);

		config.miscInfo.player_r    = atoi(argv[1]);
		config.miscInfo.player_h    = atoi(argv[2]);
		config.miscInfo.view_height = atoi(argv[3]);
	}
	else if (y_stricmp(argv[0], "sky_color") == 0)  // back compat
	{
		if (nargs != 1)
			pst->fail(bad_arg_count_fail, argv[0], 1);

		config.miscInfo.sky_color = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "sky_flat") == 0)
	{
		if (nargs != 1)
			pst->fail(bad_arg_count_fail, argv[0], 1);

		config.miscInfo.sky_flat = argv[1];
	}
	else if (y_stricmp(argv[0], "color") == 0)
	{
		if (nargs < 2)
			pst->fail(bad_arg_count_fail, argv[0], 2);

		ParseColorDef(config, pst->argv + 1, nargs);
	}
	else if (y_stricmp(argv[0], "feature") == 0)
	{
		if (nargs < 2)
			pst->fail(bad_arg_count_fail, argv[0], 2);

		ParseFeatureDef(config, pst->argv + 1, nargs);
	}
	else if (y_stricmp(argv[0], "default_textures") == 0)
	{
		if (nargs != 3)
			pst->fail(bad_arg_count_fail, argv[0], 3);

		config.default_wall_tex	= argv[1];
		config.default_floor_tex	= argv[2];
		config.default_ceil_tex	= argv[3];
	}
	else if (y_stricmp(argv[0], "default_thing") == 0)
	{
		if (nargs != 1)
			pst->fail(bad_arg_count_fail, argv[0], 1);

		config.default_thing = atoi(argv[1]);
	}
	else if (y_stricmp(argv[0], "linegroup") == 0 ||
			 y_stricmp(argv[0], "spec_group") == 0)
	{
		if (nargs != 2)
			pst->fail(bad_arg_count_fail, argv[0], 2);

		linegroup_t lg = {};

		lg.group = argv[1][0];
		lg.desc  = argv[2];

		config.line_groups[lg.group] = lg;
	}

	else if (y_stricmp(argv[0], "line") == 0 ||
			 y_stricmp(argv[0], "special") == 0)
	{
		if (nargs < 3)
			pst->fail(bad_arg_count_fail, argv[0], 3);

		linetype_t info = {};

		int number = atoi(argv[1]);

		info.group = argv[2][0];
		info.desc  = argv[3];

		int arg_count = std::min(nargs - 3, 5);
		for (int i = 0 ; i < arg_count ; i++)
		{
			if (argv[4 + i][0] != '-')
            {
                if(!parseArg(argv[4 + i], info.args[i]))
                    pst->fail("invalid arg type \"%s\"", argv[4 + i]);
            }
		}

		if (config.line_groups.find( info.group) == config.line_groups.end())
		{
			gLog.printf("%s(%d): unknown line group '%c'\n",
					  pst->file().u8string().c_str(), pst->line(),  info.group);
		}
		else
			config.line_types[number] = info;
	}

	else if (y_stricmp(argv[0], "sector") == 0)
	{
		if (nargs != 2)
			pst->fail(bad_arg_count_fail, argv[0], 2);

		int number = atoi(argv[1]);

		sectortype_t info = {};

		info.desc = argv[2];

		config.sector_types[number] = info;
	}

	else if (y_stricmp(argv[0], "thinggroup") == 0)
	{
		if (nargs != 3)
			pst->fail(bad_arg_count_fail, argv[0], 3);

		thinggroup_t tg = {};

		tg.group = argv[1][0];
		tg.color = ParseColor(argv[2]);
		tg.desc  = argv[3];

		config.thing_groups[tg.group] = tg;
	}

	else if (y_stricmp(argv[0], "thing") == 0)
	{
		if (nargs < 6)
			pst->fail(bad_arg_count_fail, argv[0], 6);

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

		if (config.thing_groups.find(info.group) == config.thing_groups.end())
		{
			gLog.printf("%s(%d): unknown thing group '%c'\n",
					  pst->file().u8string().c_str(), pst->line(), info.group);
		}
		else
		{
			info.color = config.thing_groups[info.group].color;

			config.thing_types[number] = info;
		}
	}

	else if (y_stricmp(argv[0], "texturegroup") == 0)
	{
		if (nargs != 2)
			pst->fail(bad_arg_count_fail, argv[0], 2);

		texturegroup_t tg = {};

		tg.group = argv[1][0];
		tg.desc  = argv[2];

		config.texture_groups[tg.group] = tg;
	}

	else if (y_stricmp(argv[0], "texture") == 0)
	{
		if (nargs != 2)
			pst->fail(bad_arg_count_fail, argv[0], 2);

		char group = argv[1][0];
		SString name = SString(argv[2]);

		if (config.texture_groups.find((char)tolower(group)) == config.texture_groups.end())
		{
			gLog.printf("%s(%d): unknown texture group '%c'\n",
					  pst->file().u8string().c_str(), pst->line(), group);
		}
		else
			config.texture_categories[name] = group;
	}

	else if (y_stricmp(argv[0], "flat") == 0)
	{
		if (nargs != 2)
			pst->fail(bad_arg_count_fail, argv[0], 2);

		char group = argv[1][0];
		SString name = SString(argv[2]);

		if (config.texture_groups.find((char)tolower(group)) == config.texture_groups.end())
		{
			gLog.printf("%s(%d): unknown texture group '%c'\n",
					  pst->file().u8string().c_str(), pst->line(), group);
		}
		else
			config.flat_categories[name] = group;
	}

	else if (y_stricmp(argv[0], "gen_line") == 0)
	{
		if (nargs != 4)
			pst->fail(bad_arg_count_fail, argv[0], 4);

		pst->current_gen_line = config.num_gen_linetypes;
		config.num_gen_linetypes++;

		if (config.num_gen_linetypes > MAX_GEN_NUM_TYPES)
			pst->fail("too many gen_line definitions");

		generalized_linetype_t *def = &config.gen_linetypes[pst->current_gen_line];

		def->key = argv[1][0];

		// use strtol() to support "0x" notation
		def->base   = static_cast<int>(strtol(argv[2], NULL, 0));
		def->length = static_cast<int>(strtol(argv[3], NULL, 0));

		def->name = argv[4];
		def->fields.clear();
	}

	else if (y_stricmp(argv[0], "gen_field") == 0)
	{
		if (nargs < 5)
			pst->fail(bad_arg_count_fail, argv[0], 5);

		if (pst->current_gen_line < 0)
			pst->fail("gen_field used outside of a gen_line definition");

		generalized_linetype_t *def = &config.gen_linetypes[pst->current_gen_line];

		def->fields.push_back({});
		generalized_field_t *field = &def->fields.back();

		if (def->fields.size() > MAX_GEN_NUM_FIELDS)
			pst->fail("too many fields in gen_line definition");

		field->bits  = atoi(argv[1]);
		field->shift = atoi(argv[2]);

		field->mask  = ((1 << field->bits) - 1) << field->shift;

		field->default_val = atoi(argv[3]);

		field->name = argv[4];

		// grab the keywords
		int num_keywords = std::min(nargs - 4, MAX_GEN_FIELD_KEYWORDS);
		field->keywords.reserve(num_keywords);

		for (int i = 0 ; i < num_keywords ; i++)
		{
			field->keywords.push_back(argv[5 + i]);
		}
	}
	else if (y_stricmp(argv[0], "clear") == 0)
	{
		if (nargs < 1)
			pst->fail(bad_arg_count_fail, argv[0], 2);

		ParseClearKeywords(config, pst->argv + 1, nargs, pst);
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
	const char **argv  = pst->argv;
	int    nargs = pst->argc - 1;

	if (y_stricmp(argv[0], "map_formats") == 0 ||
		y_stricmp(argv[0], "supported_games") == 0 ||
		y_stricmp(argv[0], "udmf_namespace") == 0)
	{
		pst->fail("%s can only be used in port definitions", argv[0]);
	}

	if (y_stricmp(argv[0], "base_game") == 0)
	{
		if (nargs < 1)
			pst->fail(bad_arg_count_fail, argv[0], 1);

		loadingGame.baseGame = SString(argv[1]).asLower();
	}
}


static void M_ParsePortInfoLine(parser_state_c *pst, PortInfo_c &port)
{
	const char **argv  = pst->argv;
	int    nargs = pst->argc - 1;

	if (y_stricmp(argv[0], "base_game") == 0)
		pst->fail("%s can only be used in game definitions", argv[0]);

	if (y_stricmp(argv[0], "supported_games") == 0)
	{
		if (nargs < 1)
			pst->fail(bad_arg_count_fail, argv[0], 1);

		for (argv++ ; nargs > 0 ; argv++, nargs--)
			port.AddSupportedGame(SString(*argv).asLower());
	}
	else if (y_stricmp(argv[0], "map_formats") == 0)
	{
		if (nargs < 1)
			pst->fail(bad_arg_count_fail, argv[0], 1);

		port.formats = ParseMapFormats(pst, argv + 1, nargs);
	}
	else if (y_stricmp(argv[0], "udmf_namespace") == 0)
	{
		if (nargs != 1)
			pst->fail(bad_arg_count_fail, argv[0], 1);

		// want to preserve the case here
		port.udmf_namespace = argv[1];
	}
}


static ParsingCondition M_ParseConditional(const std::unordered_map<SString, SString> &parse_vars, parser_state_c *pst)
{
	// returns the result of the "IF" test, true or false.

	const char **argv  = pst->argv + 1;
	int    nargs = pst->argc - 1;

	bool op_is  = (nargs >= 3 && y_stricmp(argv[1], "is")  == 0);
	bool op_not = (nargs >= 3 && y_stricmp(argv[1], "not") == 0);

	if (op_is || op_not)
	{
		if (strlen(argv[0]) < 2 || argv[0][0] != '$')
			pst->fail("expected variable in if statement");

		// tokens are stored in pst->tokenbuf, so this is OK
		SString argvUpper = argv[0];
		argvUpper = argvUpper.asUpper();

		auto it = parse_vars.find(argvUpper);
		if(it != parse_vars.end())
		{
			const SString &var_value = it->second;

			// test multiple values, only need one to succeed
			for (int i = 2 ; i < nargs ; i++)
				if (var_value.noCaseEqual(argv[i]))
					return op_is ? ParsingCondition::reading : ParsingCondition::skipping;
		}

		return op_not ? ParsingCondition::reading : ParsingCondition::skipping;
	}

	pst->fail("syntax error in if statement");
	return ParsingCondition::skipping;
}


static void M_ParseSetVar(std::unordered_map<SString, SString> &parse_vars, parser_state_c *pst)
{
	const char **argv  = pst->argv + 1;
	int    nargs = pst->argc - 1;

	if (nargs != 2)
		pst->fail(bad_arg_count_fail, pst->argv[0], 1);

	if (strlen(argv[0]) < 2 || argv[0][0] != '$')
		pst->fail("variable name too short or lacks '$' prefix");

	SString argvUpper = SString(argv[0]).asUpper();

	parse_vars[argvUpper] = argv[1];
}

//
//  this is main function for parsing a definition file.
//
//  when purpose is PURPOSE_GameInfo or PURPOSE_PortInfo, then
//  only minimal parsing occurs, in particular the "include", "set"
//  and "if".."endif" directives are NOT handled.
//
void M_ParseDefinitionFile(std::unordered_map<SString, SString> &parse_vars,
						   const ParsePurpose purpose,
						   ParseTarget target,
						   const fs::path &filename,
						   const fs::path &cfolder,
						   const fs::path &cprettyname,
						   int include_level)
{
	SYS_ASSERT(!filename.empty());

	fs::path folder = cfolder;
	if (folder.empty())
		folder = "common";

	fs::path prettyname = cprettyname;
	if (prettyname.empty())
		prettyname = filename.filename();

	parser_state_c parser_state(prettyname);

	// this is a bit silly, but makes it easier to move code around
	parser_state_c *const pst = &parser_state;

	// read the definition file, line by line

	LineFile file(filename);
	if (! file.isOpen())
		throw ParseException(SString::printf("Cannot open %s: %s", filename.u8string().c_str(), GetErrorMessage(errno).c_str()));

	while (pst->readLine(file))
	{
		pst->tokenize();

		// skip empty lines and comments
		if (pst->argc == 0)
			continue;

		int nargs = pst->argc - 1;


		// handle conditionals: if...else...endif

		if (y_stricmp(pst->argv[0], "if") == 0)
		{
			parsing_cond_state_t cst;

			cst.cond = M_ParseConditional(parse_vars, pst);
			cst.start_line = pst->line();

			pst->cond_stack.push_back(cst);
			continue;
		}
		if (y_stricmp(pst->argv[0], "else") == 0)
		{
			if (pst->cond_stack.empty())
				pst->fail("else without if");

			// toggle the mode
			pst->cond_stack.back().Toggle();
			continue;
		}
		if (y_stricmp(pst->argv[0], "endif") == 0)
		{
			if (pst->cond_stack.empty())
				pst->fail("endif without if");

			pst->cond_stack.pop_back();
			continue;
		}


		// skip lines when ANY if statement is in skip mode
		if (pst->HaveAnySkipping())
			continue;


		// handle setting variables
		if (y_stricmp(pst->argv[0], "set") == 0)
		{
			M_ParseSetVar(parse_vars, pst);
			continue;
		}


		// handle includes
		if (y_stricmp(pst->argv[0], "include") == 0)
		{
			if (nargs != 1)
				pst->fail(bad_arg_count_fail, pst->argv[0], 1);

			if (include_level >= MAX_INCLUDE_LEVEL)
				pst->fail("Too many includes (check for a loop)");

			fs::path new_folder = folder;
			tl::optional<fs::path> new_name = FindDefinitionFile(global::home_dir, global::install_dir, new_folder, pst->argv[1]);

			// if not found, check the common/ folder
			if (!new_name && folder != "common")
			{
				new_folder = "common";
				new_name = FindDefinitionFile(global::home_dir, global::install_dir, new_folder, pst->argv[1]);
			}

			if (!new_name)
				pst->fail("Cannot find include file: %s.ugh", pst->argv[1]);

			M_ParseDefinitionFile(parse_vars, purpose, target, *new_name, new_folder,
								  "" /* prettyname */,
								  include_level + 1);
			continue;
		}


		if (purpose == ParsePurpose::gameInfo)
		{
			M_ParseGameInfoLine(pst, *target.game);
			continue;
		}
		if (purpose == ParsePurpose::portInfo)
		{
			M_ParsePortInfoLine(pst, *target.port);
			continue;
		}

		// handle everything else
		M_ParseNormalLine(pst, *target.config);
	}

	// check for an unterminated conditional
	if (! pst->cond_stack.empty())
		pst->fail("Missing endif statement");
}

//
// Load game info
//
static GameInfo M_LoadGameInfo(const SString &game)
		noexcept(false)
{
	// already loaded?
	auto it = global::sLoadedGameDefs.find(game);
	if(it != global::sLoadedGameDefs.end())
		return it->second;

	tl::optional<fs::path> filename = FindDefinitionFile(global::home_dir, global::install_dir, GAMES_DIR, game);
	if(!filename)
		return {};
	GameInfo loadingGame = GameInfo(game);
	std::unordered_map<SString, SString> empty_vars;
	M_ParseDefinitionFile(empty_vars, ParsePurpose::gameInfo, &loadingGame,
						  *filename, "games", "");
	if(loadingGame.baseGame.empty())
		throw ParseException(SString::printf("Game definition for '%s' does "
											 "not set base_game\n",
											 game.c_str()));

	global::sLoadedGameDefs[game] = loadingGame;
	return loadingGame;
}


const PortInfo_c * M_LoadPortInfo(const SString &port) noexcept(false)
{
	std::map<SString, PortInfo_c>::iterator IT;
	IT = global::loaded_port_defs.find(port);

	if (IT != global::loaded_port_defs.end())
		return &IT->second;

	tl::optional<fs::path> filename = FindDefinitionFile(global::home_dir, global::install_dir, PORTS_DIR, port);
	if (!filename)
		return NULL;

	global::loading_Port = PortInfo_c(port);

	std::unordered_map<SString, SString> empty_vars;
	M_ParseDefinitionFile(empty_vars, ParsePurpose::portInfo, &global::loading_Port,
						  *filename, "ports", "");

	// default is to support both Doom and Doom2
	if (global::loading_Port.supported_games.empty())
	{
		global::loading_Port.supported_games.push_back("doom");
		global::loading_Port.supported_games.push_back("doom2");
	}

	global::loaded_port_defs[port] = global::loading_Port;
	return &global::loaded_port_defs[port];
}


//------------------------------------------------------------------------

//
// Collect known definitions from folder
//
std::vector<SString> M_CollectKnownDefs(const std::initializer_list<fs::path> &dirList,
										const fs::path &folder)
{
	std::vector<SString> temp_list;

	//	gLog.debugPrintf("M_CollectKnownDefs for: %d\n", folder);
	auto scanner_add_file = [&temp_list](const fs::path &name, int flags)
	{
		if (flags & (SCAN_F_IsDir | SCAN_F_Hidden))
			return;
		if (! MatchExtensionNoCase(name, ".ugh"))
			return;
		temp_list.push_back(ReplaceExtension(name, NULL).u8string());
	};
	for(const fs::path &parentPath : dirList)
	{
		fs::path path = parentPath / folder;
		ScanDirectory(path, scanner_add_file);
	}

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

SString M_GetBaseGame(const SString &game) noexcept(false)
{
	GameInfo ginfo = M_LoadGameInfo(game);
	SYS_ASSERT(ginfo.isSet());

	return ginfo.baseGame;
}


map_format_bitset_t M_DetermineMapFormats(const char *game, const char *port)
{
	const PortInfo_c *pinfo = M_LoadPortInfo(port);
	if (pinfo && pinfo->formats != 0)
		return pinfo->formats;

	// a bit hacky, maybe should do it a better way...
	if (strcmp(game, "hexen") == 0)
		return (1 << static_cast<int>(MapFormat::hexen));

	return (1 << static_cast<int>(MapFormat::doom));
}


bool M_CheckPortSupportsGame(const SString &base_game,
							 const SString &port) noexcept(false)
{
	if (port == "vanilla")
	{
		// Vanilla means the engine that comes with the game, hence
		// it supports everything.
		return true;
	}

	const PortInfo_c *pinfo = M_LoadPortInfo(port);
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

SString M_CollectPortsForMenu(const char *base_game,
							  int *exist_val, const char *exist_name) noexcept(false)
{
	std::vector<SString> list = M_CollectKnownDefs({global::install_dir, global::home_dir}, "ports");

	if (list.empty())
		return "";

	// determine final length
	int length = 2 + (int)list.size();
	unsigned int i;

	for (i = 0 ; i < list.size() ; i++)
		length += static_cast<int>(list[i].length());

	SString result;
	result.reserve(length);

	int entry_id = 0;

	for (i = 0 ; i < list.size() ; i++)
	{
		if (! M_CheckPortSupportsGame(base_game, list[i]))
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
	return flat.noCaseEqual(conf.miscInfo.sky_flat);
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

	SI = conf.sector_types.find(type);

	if (SI != conf.sector_types.end())
		return SI->second;

	static sectortype_t dummy_type =
	{
		"UNKNOWN TYPE"
	};

	return dummy_type;
}


const linetype_t &ConfigData::getLineType(int type) const
{
	std::map<int, linetype_t>::const_iterator LI;

	LI = line_types.find(type);

	if (LI != line_types.end())
		return LI->second;

	static const linetype_t dummy_type =
	{
		0, "UNKNOWN TYPE"
	};

	return dummy_type;
}


const thingtype_t &ConfigData::getThingType(int type) const
{
	const thingtype_t *existing = get(thing_types, type);
	if(existing)
		return *existing;

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

	TI = conf.texture_categories.find(name);

	if (TI != conf.texture_categories.end())
		return TI->second;

	return '-';  // the OTHER category
}


char Instance::M_GetFlatType(const SString &name) const
{
	std::map<SString, char>::const_iterator TI;

	TI = conf.flat_categories.find(name);

	if (TI != conf.flat_categories.end())
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
	return M_CategoryString(letters, false, conf.line_groups, conf.line_types);
}


SString Instance::M_ThingCategoryString(SString &letters) const
{
	return M_CategoryString(letters, true, conf.thing_groups, conf.thing_types);
}


SString Instance::M_TextureCategoryString(SString &letters, bool do_flats) const
{
	return M_CategoryString(letters, true, conf.texture_groups, do_flats ? conf.flat_categories : conf.texture_categories);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
