//------------------------------------------------------------------------
//  GAME DEFINITION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2013 Andrew Apted
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


// line <number> <group> <shortdesc> <longdesc>
typedef struct
{
	char group;
	const char *desc;
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
	std::string sky_flat;

	int wall_colors[2];
	int floor_colors[2];

	int missing_color;
	int unknown_tex;
	int unknown_flat;

	int player_h;
	int min_dm_starts;
	int max_dm_starts;

} game_info_t;

extern game_info_t  game_info;


void M_InitDefinitions();
void M_LoadDefinitions(const char *folder, const char *name,
                       int include_level = 0);
bool M_CanLoadDefinitions(const char *folder, const char *name);
void M_ParseDefinitionFile(const char *filename, const char *folder = NULL,
						   const char *basename = NULL,
                           int include_level = 0);
void M_FreeDefinitions();

void M_CollectKnownDefs(const char *folder, std::vector<const char *> & list);

const char * M_CollectDefsForMenu(const char *folder, int *exist_val, const char *exist_name);


// is this flat a sky?
bool is_sky(const char *flat);


const sectortype_t * M_GetSectorType(int type);
const linetype_t   * M_GetLineType(int type);
const thingtype_t  * M_GetThingType(int type);

char M_GetTextureType(const char *name);
char M_GetFlatType(const char *name);

const char *M_LineCategoryString(char *letters);
const char *M_ThingCategoryString(char *letters);
const char *M_TextureCategoryString(char *letters);

#endif  /* __EUREKA_M_GAME_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
