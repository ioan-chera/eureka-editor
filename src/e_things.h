//------------------------------------------------------------------------
//  THING OPERATIONS
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

#ifndef __EUREKA_E_THINGS_H__
#define __EUREKA_E_THINGS_H__

#include "im_color.h"


/* starting areas */
#define THING_PLAYER1         1
#define THING_PLAYER2         2
#define THING_PLAYER3         3
#define THING_PLAYER4         4
#define THING_DEATHMATCH      11


#define MAX_RADIUS  128

class Instance;


/*
 *  angle_to_direction - convert angle to direction (0-7)
 *
 *  Return a value that is guaranteed to be within [0-7].
 */
inline int angle_to_direction (int angle)
{
  return ((unsigned) angle / 45) % 8;
}


int calc_new_angle(int angle, int diff);


/* commands */

void CMD_TH_Disconnect(Instance &inst);
void CMD_TH_Merge(Instance &inst);


#endif  /* __EUREKA_E_THINGS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
