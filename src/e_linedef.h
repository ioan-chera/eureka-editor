//------------------------------------------------------------------------
//  LINEDEF OPERATIONS
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

#ifndef __EUREKA_E_LINEDEF_H__
#define __EUREKA_E_LINEDEF_H__

void FlipLineDef(int ld);
void FlipLineDefGroup(selection_c *flip);


void LineDefs_SetLength(int new_len);

bool LineDefAlreadyExists(int v1, int v2);
bool LineDefWouldOverlap(int v1, double x2, double y2);

int SplitLineDefAtVertex(int ld, int v_idx);

void MoveCoordOntoLineDef(int ld, double *x, double *y);

void LD_AddSecondSideDef(int ld, int new_sd, int other_sd);
void LD_RemoveSideDef(int ld, int ld_side);
void LD_FixForLostSide(int ld);

double LD_AngleBetweenLines(int A, int B, int C);

bool LD_GetTwoNeighbors(int new_ld, int v1, int v2,
						int *ld1, int *side1,
						int *ld2, int *side2);

bool LD_RailHeights(int& z1, int& z2, const LineDef *L, const SideDef *sd,
					const Sector *front, const Sector *back);

typedef enum
{
	LINALIGN_X		= (1 << 0),		// align the X offset
	LINALIGN_Y		= (1 << 1),		// align the Y offset
	LINALIGN_Clear	= (1 << 2),		// clear the offset(s), instead of aligning
	LINALIGN_Unpeg	= (1 << 3),		// change the unpegging flags
	LINALIGN_Right	= (1 << 4)		// align with sidedef on RIGHT of this one [ otherwise do LEFT ]
}
linedef_align_flag_e;

bool Line_AlignOffsets(const Objid& obj, int align_flags);

void Line_AlignGroup(std::vector<Objid> & group, int align_flags);


/* commands */

void CMD_LIN_Flip();
void CMD_LIN_SwapSides();
void CMD_LIN_Align();
void CMD_LIN_MergeTwo();
void CMD_LIN_SplitHalf();

#endif  /* __EUREKA_E_LINEDEF_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
