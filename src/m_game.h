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

#ifndef __EUREKA_M_GAME_H__
#define __EUREKA_M_GAME_H__

#include "im_color.h"
#include "m_strings.h"
#include <initializer_list>
#include <map>
#include <unordered_map>

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

struct ConfigData;
class Instance;
struct LoadingData;

// for this, set/clear/test bits using (1 << MAPF_xxx)
typedef int map_format_bitset_t;


/*
 *  Data structures for game definition data
 */

// linegroup <letter> <description>
struct linegroup_t
{
	char group;
	SString desc;
};

//
// Special arg type
//
enum class SpecialArgType
{
    generic,
    tag,
    tag_hi,
    line_id,
    self_line_id,
    self_line_id_hi,
    tid,
    po
};

//
// Argument type
//
struct SpecialArg
{
    SString name;
    SpecialArgType type = SpecialArgType::generic;
};

// line <number> <group> <description>  [ arg1 .. arg5 ]
struct linetype_t
{
    bool isPolyObjectSpecial() const;

	char group;
	SString desc;
	SpecialArg args[5];
};


// sector <number> <description>
struct sectortype_t
{
	SString desc;
};


// thinggroup <letter> <colour> <description>
struct thinggroup_t
{
	char group;         // group letter
	rgb_color_t color;  // RGB colour
	SString desc;   // Description of thing group
};


// thing <number> <group> <flags> <radius> <description> [<sprite>]  [ arg1..arg5 ]
struct thingtype_t
{
	char group;      // group letter
	short flags;     // flags (THINGDEF_XXX)
	short radius;    // radius of thing
	float scale;	 // scaling (1.0 is normal)

	SString desc;    // short description of the thing
	SString sprite;  // name of sprite (frame and rot are optional)
	rgb_color_t color;   // RGB color (from group)
	SString args[5]; // args used when spawned (Hexen)
};


enum thingdef_flags_e
{
	THINGDEF_INVIS   = (1 << 0),  // partially invisible
	THINGDEF_CEIL    = (1 << 1),  // hangs from ceiling
	THINGDEF_LIT     = (1 << 2),  // always bright
	THINGDEF_PASS    = (1 << 3),  // non-blocking
	THINGDEF_VOID    = (1 << 4),  // can exist in the void
	THINGDEF_TELEPT  = (1 << 5),  // teleport dest, can overlap certain things
    THINGDEF_POLYSPOT= (1 << 6),  // polyobject spawn spot, tagging & BSP
};


// texturegroup <letter> <description>
struct texturegroup_t
{
	char group;
	SString desc;
};


/*
 *  Global variables that contain game definition data
 */

struct misc_info_t
{
	enum
	{
		DEFAULT_PLAYER_RADIUS = 16,
		DEFAULT_PLAYER_HEIGHT = 56,
		DEFAULT_PLAYER_VIEW_HEIGHT = 41,
		DEFAULT_MINIMUM_DEATHMATCH_STARTS = 4,
		DEFAULT_MAXIMUM_DEATHMATCH_STARTS = 10
	};

	int  sky_color = 0;
	SString sky_flat;

	int  wall_colors[2] = {};
	int floor_colors[2] = {};
	int invis_colors[2] = {};

	int missing_color = 0;
	int unknown_tex = 0;
	int unknown_flat = 0;
	int unknown_thing = 0;

	int player_r = DEFAULT_PLAYER_RADIUS;
	int player_h = DEFAULT_PLAYER_HEIGHT;
	int view_height = DEFAULT_PLAYER_VIEW_HEIGHT;

	int min_dm_starts = DEFAULT_MINIMUM_DEATHMATCH_STARTS;
	int max_dm_starts = DEFAULT_MAXIMUM_DEATHMATCH_STARTS;
};

//
// Type of generalized sectors for port
//
enum class GenSectorFamily : int
{
	none,	// not set
	boom,	// original Boom generalized sectors
	zdoom	// ZDoom shifted Boom generalized sectors
};

//
// Tag 666 rules. Mapped from the UGH file.
//
enum class Tag666Rules : int
{
	disabled = 0,
	doom = 1,
	heretic = 2,
};

struct port_features_t
{
	// NOTE: values here are generally 0 or 1, but some can be higher

	int gen_types;		// BOOM generalized linedef types
	GenSectorFamily gen_sectors;	// BOOM and ZDoom sector flags (damage, secret, ...)

	int tx_start;		// textures in TX_START .. TX_END
	int img_png;		// PNG format for various graphics

	int coop_dm_flags;	// MTF_NOT_COOP and MTF_NOT_DM for things
	int friend_flag;	// MTF_FRIEND thing flag from MBF

	int pass_through;	// Boom's MTF_PASSTHRU line flag
	int midtex_3d;		// Eternity's ML_3DMIDTEX line flag
	int strife_flags;	// Strife flags

	int medusa_fixed;	// the Medusa Effect has been fixed (cannot occur)
	int tuttifrutti_fixed;	// the tutti-frutti effect has been fixed (Boom and modern ZDoom)
	int lax_sprites;	// sprites can be found outside of S_START..S_END
	int mix_textures_flats;	// allow mixing textures and flats (adv. ports)
	int neg_patch_offsets;	// honors negative patch offsets in textures (ZDoom)

	int no_need_players;	// having no players is OK (Things checker)
	union					// game uses tag 666 and 667 for special FX (type Tag666Rules)
	{
		int tag_666raw;
		Tag666Rules tag_666;
	};
	int extra_floors;		// bitmask: +1 EDGE, +2 Legacy, +4 for ZDoom in Hexen format
	int slopes;				// bitmask: +1 EDGE, +2 Eternity, +4 Odamex,
							//          +8 for ZDoom in Hexen format, +16 ZDoom things
};

//
// Game info
//
struct GameInfo
{
	SString name;
	SString baseGame;

	GameInfo() = default;

	explicit GameInfo(const SString &name): name(name)
	{
	}

	bool isSet() const
	{
		return name.good() && baseGame.good();
	}
};

class PortInfo_c
{
public:
	SString name;

	map_format_bitset_t formats = 0; // 0 if not specified

	std::vector<SString> supported_games;
	SString udmf_namespace;

public:
	PortInfo_c() = default;

	explicit PortInfo_c(SString _name) : name(_name)
	{
	}

	bool SupportsGame(const SString &game) const;

	void AddSupportedGame(const SString &game);
};


//------------------------------------------------------------------------

/* Boom generalized types */

#define MAX_GEN_FIELD_BITS	4
#define MAX_GEN_FIELD_KEYWORDS	(1 << MAX_GEN_FIELD_BITS)

#define MAX_GEN_NUM_FIELDS	16
#define MAX_GEN_NUM_TYPES	16

struct generalized_field_t
{
	int bits;	//
	int mask;	//	the bit-field info
	int shift;	//

	int default_val;

	SString name;

	std::vector<SString> keywords;
};


struct generalized_linetype_t
{
	char key;

	int base;
	int length;

	SString name;

	std::vector<generalized_field_t> fields;
};

//------------------------------------------------------------------------

// Standard directory names
static const char GAMES_DIR[] = "games";
static const char PORTS_DIR[] = "ports";

bool M_CanLoadDefinitions(const fs::path &home_dir, const fs::path &install_dir, const fs::path &folder, const SString &name);
void readConfiguration(std::unordered_map<SString, SString> &parse_vars,
					   const fs::path &folder, const SString &name,
					   ConfigData &config) noexcept(false);

enum class ParsePurpose
{
	normal,		// normal loading, update everything
	resource,	// as a resource file
	gameInfo,	// load a GameInfo
	portInfo	// load a PortInfo_c
};

//
// Exception throwable by M_ParseDefinitionFile. Meant to be caught by parties 
// who don't want the app to terminate suddenly.
//
class ParseException : public std::runtime_error
{
public:
	ParseException(const SString &msg) : std::runtime_error(msg.c_str())
	{
	}
};

//
// Target for M_ParseDefinitionFile
//
struct ConfigData
{
	misc_info_t miscInfo = {};
	port_features_t features = {};

	SString default_wall_tex = "GRAY1";
	SString default_floor_tex = "FLAT1";
	SString default_ceil_tex = "FLAT1";
	int default_thing = 2001;

	std::map<SString, char> flat_categories;
	std::map<char, linegroup_t> line_groups;
	std::map<int, linetype_t> line_types;
	std::map<int, sectortype_t> sector_types;
	std::map<SString, char> texture_categories;
	std::map<char, texturegroup_t> texture_groups;
	std::map<char, thinggroup_t> thing_groups;
	std::map<int, thingtype_t> thing_types;

	int num_gen_linetypes = 0;
	generalized_linetype_t gen_linetypes[MAX_GEN_NUM_TYPES] = {};	// BOOM Generalized Lines

	const thingtype_t &getThingType(int type) const;
	const linetype_t &getLineType(int type) const;
};

//
// Generic parse target, depending on ParsePurpose
//
union ParseTarget
{
	ParseTarget(GameInfo *game) : game(game)
	{
	}

	ParseTarget(PortInfo_c *port) : port(port)
	{
	}

	ParseTarget(ConfigData *config) : config(config)
	{
	}

	ParseTarget() = default;

	GameInfo *game;
	PortInfo_c *port;
	ConfigData *config;
};

void M_ParseDefinitionFile(std::unordered_map<SString, SString> &parse_vars,
						   ParsePurpose purpose,
						   ParseTarget target,
						   const fs::path &filename,
						   const fs::path &folder = "",
						   const fs::path &prettyname = "",
                           int include_level = 0);

const PortInfo_c * M_LoadPortInfo(const SString &port) noexcept(false);

std::vector<SString> M_CollectKnownDefs(const std::initializer_list<fs::path> &dirList,
										const fs::path &folder);

bool M_CheckPortSupportsGame(const SString &base_game,
							 const SString &port) noexcept(false);

SString M_CollectPortsForMenu(const char *base_game, int *exist_val, const char *exist_name) noexcept(false);

SString M_GetBaseGame(const SString &game) noexcept(false);

map_format_bitset_t M_DetermineMapFormats(const char *game, const char *port);

bool is_null_tex(const SString &tex);		// the "-" texture
bool is_special_tex(const SString &tex);	// begins with "#"

#endif  /* __EUREKA_M_GAME_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
