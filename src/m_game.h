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

class Instance;

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


// line <number> <group> <description>  [ arg1 .. arg5 ]
struct linetype_t
{
	char group;
	SString desc;
	SString args[5];
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
	int  sky_color = 0;
	SString sky_flat;

	int  wall_colors[2] = {};
	int floor_colors[2] = {};
	int invis_colors[2] = {};

	int missing_color = 0;
	int unknown_tex = 0;
	int unknown_flat = 0;
	int unknown_thing = 0;

	int player_r = 0;
	int player_h = 0;
	int view_height = 0;

	int min_dm_starts = 0;
	int max_dm_starts = 0;
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

	GameInfo(const SString &name): name(name)
	{
	}

	operator bool() const
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
	PortInfo_c(SString _name) : name(_name)
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

	SString keywords[MAX_GEN_FIELD_KEYWORDS];
	int num_keywords;
};


struct generalized_linetype_t
{
	char key;

	int base;
	int length;

	SString name;

	generalized_field_t fields[MAX_GEN_NUM_FIELDS];
	int num_fields;
};

//------------------------------------------------------------------------

bool M_CanLoadDefinitions(const SString &folder, const SString &name);

enum parse_purpose_e
{
	PURPOSE_Normal = 0,		// normal loading, update everything
	PURPOSE_Resource,		// as a resource file
	PURPOSE_GameInfo,		// load a GameInfo
	PURPOSE_PortInfo,		// load a PortInfo_c
};

void M_ParseDefinitionFile(Instance &inst,
						   parse_purpose_e purpose,
						   void *purposeTarget,
						   const SString &filename,
						   const SString &folder = NULL,
						   const SString &prettyname = NULL,
                           int include_level = 0);

PortInfo_c * M_LoadPortInfo(Instance &inst, const SString &port);

std::vector<SString> M_CollectKnownDefs(const char *folder);

bool M_CheckPortSupportsGame(Instance &inst, const SString &base_game,
							 const SString &port);

SString M_CollectPortsForMenu(Instance &inst, const char *base_game, int *exist_val, const char *exist_name);

SString M_GetBaseGame(Instance &inst, const SString &game);

map_format_bitset_t M_DetermineMapFormats(Instance &inst, const char *game,
										  const char *port);

bool is_null_tex(const SString &tex);		// the "-" texture
bool is_special_tex(const SString &tex);	// begins with "#"

#endif  /* __EUREKA_M_GAME_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
