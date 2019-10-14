//------------------------------------------------------------------------
//  VERTEX STUFF
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

#ifndef __EUREKA_E_VERTEX_H__
#define __EUREKA_E_VERTEX_H__

int Vertex_FindExact(fixcoord_t fx, fixcoord_t fy);
int Vertex_HowManyLineDefs(int v_num);

void Vertex_MergeList(selection_c *list);
bool Vertex_TryFixDangler(int v_num);

/* commands */

void CMD_VT_Merge(void);

void CMD_VT_Disconnect(void);

void CMD_LIN_Disconnect(void);
void CMD_SEC_Disconnect(void);

void CMD_VT_ShapeLine(void);
void CMD_VT_ShapeArc (void);

#endif  /* __EUREKA_E_VERTEX_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
