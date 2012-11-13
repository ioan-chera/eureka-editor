//------------------------------------------------------------------------
//  SECTOR OPERATIONS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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

#ifndef __EUREKA_E_SECTOR_H__
#define __EUREKA_E_SECTOR_H__

#include "selectn.h"

bool CMD_MoveFloors(int diff);
bool CMD_MoveCeilings(int diff);
bool CMD_AdjustLight(int delta);

bool CMD_SwapFlats();

void CMD_CorrectSector();


void centre_of_sector (obj_no_t s, int *x, int *y);

void MakeDoorFromSector (int);
void MakeLiftFromSector (int);

#if 0
void MergeSectors (SelPtr *);
void DeleteLineDefsJoinSectors (SelPtr *);
#endif


class bitvec_c;

bitvec_c *linedefs_of_sector (obj_no_t s);
bitvec_c *linedefs_of_sectors (selection_c * list);
int linedefs_of_sector (obj_no_t s, obj_no_t *&array);

bitvec_c *bv_vertices_of_sector (obj_no_t s);
bitvec_c *bv_vertices_of_sectors (selection_c * list);

#endif  /* __EUREKA_E_SECTOR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
