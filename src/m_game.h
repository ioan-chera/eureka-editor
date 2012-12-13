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
	THINGDEF_INVIS   = (1 << 0),
	THINGDEF_CEIL    = (1 << 1),
	THINGDEF_LIT     = (1 << 2),
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

typedef enum { YGLN__, YGLN_E1M10, YGLN_E1M1, YGLN_MAP01 } ygln_t;

extern ygln_t yg_level_name;

extern int g_sky_color;

extern std::string g_sky_flat;


// FIXME: M_ prefix
void InitDefinitions();
void LoadDefinitions(const char *folder, const char *name,
                     int include_level = 0);
bool CanLoadDefinitions(const char *folder, const char *name);
void FreeDefinitions();

void M_CollectKnownDefs(const char *folder, std::vector<const char *> & list);

const char * M_CollectDefsForMenu(const char *folder, int *exist_val, const char *exist_name);


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
