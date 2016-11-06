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

#ifndef __EUREKA_M_GAME_H__
#define __EUREKA_M_GAME_H__

#include "im_color.h"

#include <string>


/*
 *  Data structures for game definition data
 */

// linegroup <letter> <description>
typedef struct
{
	char group;
	const char *desc;
}
linegroup_t;


// line <number> <group> <description>  [ arg1 .. arg5 ]
typedef struct
{
	char group;
	const char *desc;
	const char *args[5];
}
linetype_t;


// sector <number> <description>
typedef struct
{
	const char *desc;
}
sectortype_t;


// thinggroup <letter> <colour> <description>
typedef struct
{
	char group;         // group letter
	rgb_color_t color;  // RGB colour
	const char *desc;   // Description of thing group
}
thinggroup_t;


// thing <number> <group> <flags> <radius> <description> [<sprite>]
typedef struct
{
	char group;      // Thing group
	short flags;     // Flags
	short radius;    // Radius of thing
	const char *desc;  // Short description of thing
	const char *sprite;  // Root of name of sprite for thing
	rgb_color_t color;   // RGB color (from group)
}
thingtype_t;


typedef enum
{
	THINGDEF_INVIS   = (1 << 0),  // partially invisible
	THINGDEF_CEIL    = (1 << 1),  // hangs from ceiling
	THINGDEF_LIT     = (1 << 2),  // always bright
	THINGDEF_PASS    = (1 << 3),  // non-blocking
	THINGDEF_VOID    = (1 << 4),  // can exist in the void
	THINGDEF_TELEPT  = (1 << 5),  // teleport dest, can overlap certain things
}
thingdef_flags_e;


// texturegroup <letter> <description>
typedef struct
{
	char group;
	const char *desc;
}
texturegroup_t;


/*
 *  Global variables that contain game definition data
 */

typedef struct
{
	int sky_color;
	char sky_flat[16];

	int wall_colors[2];
	int floor_colors[2];

	int missing_color;
	int unknown_tex;
	int unknown_flat;

	int player_h;
	int min_dm_starts;
	int max_dm_starts;

	/* port features */

	int gen_types;		// BOOM generalized linedefs and sectors

	int tx_start;		// textures in TX_START .. TX_END
	int img_png;		// PNG format for various graphics

	int coop_dm_flags;	// MTF_NOT_COOP and MTF_NOT_DM for things
	int friend_flag;	// MTF_FRIEND thing flag from MBF

	int pass_through;	// Boom's MTF_PASSTHRU line flag
	int midtex_3d;		// Eternity's ML_3DMIDTEX line flag

	int medusa_bug;		// used for Vanilla, prone to the Medusa Effect

} game_info_t;

extern game_info_t  game_info;


/* Boom generalized types */

#define MAX_GEN_FIELD_BITS	4
#define MAX_GEN_FIELD_KEYWORDS	(1 << MAX_GEN_FIELD_BITS)

#define MAX_GEN_NUM_FIELDS	16
#define MAX_GEN_NUM_TYPES	16

typedef struct
{
	int bits;	//
	int mask;	//	the bit-field info
	int shift;	//

	int default_val;

	const char *name;

	const char *keywords[MAX_GEN_FIELD_KEYWORDS];
	int num_keywords;
}
generalized_field_t;


typedef struct
{
	char key;

	int base;
	int length;

	const char *name;

	generalized_field_t fields[MAX_GEN_NUM_FIELDS];
	int num_fields;
}
generalized_linetype_t;


extern generalized_linetype_t gen_linetypes[MAX_GEN_NUM_TYPES];
extern int num_gen_linetypes;


//------------------------------------------------------------------------

void M_InitDefinitions();
void M_LoadDefinitions(const char *folder, const char *name);
bool M_CanLoadDefinitions(const char *folder, const char *name);

typedef enum
{
	PURPOSE_Normal = 0,		// normal loading
	PURPOSE_Resource,		// as a resource file
	PURPOSE_GameCheck,		// check game's variant name and map formats
	PURPOSE_PortCheck,		// check if port supports game

} parse_purpose_e;

typedef struct
{
	// set when "map_formats" is found, otherwise left unchanged
	map_format_bitset_t formats;

	// set when "variant_of" is found, otherwise left unchanged
	char variant_name[256];

	// when "supported_games" is found, check if variant_name is in
	// the list and set this to 0 or 1, otherwise left unchanged
	int supports_game;

} parse_check_info_t;

void M_ParseDefinitionFile(parse_purpose_e purpose,
						   const char *filename,
						   const char *folder = NULL,
						   const char *prettyname = NULL,
						   parse_check_info_t *check_info = NULL,
                           int include_level = 0);
void M_FreeDefinitions();

void M_CollectKnownDefs(const char *folder, std::vector<const char *> & list);

const char * M_CollectPortsForMenu(const char *var_game, int *exist_val, const char *exist_name);

const char * M_VariantForGame(const char *game);

map_format_bitset_t M_DetermineMapFormats(const char *game, const char *port);


// is this flat a sky?
bool is_sky(const char *flat);

bool is_null_tex(const char *tex);
bool is_missing_tex(const char *tex);


const sectortype_t * M_GetSectorType(int type);
const linetype_t   * M_GetLineType(int type);
const thingtype_t  * M_GetThingType(int type);

char M_GetTextureType(const char *name);
char M_GetFlatType(const char *name);

const char *M_LineCategoryString(char *letters);
const char *M_ThingCategoryString(char *letters);
const char *M_TextureCategoryString(char *letters, bool do_flats);

#endif  /* __EUREKA_M_GAME_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
