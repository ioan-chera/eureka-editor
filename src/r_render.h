//------------------------------------------------------------------------
//  3D RENDERING
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

#ifndef __EUREKA_R_RENDER__
#define __EUREKA_R_RENDER__

void Render3D_Setup();
void Render3D_RegisterCommands();

void Render3D_Draw(int ox, int oy, int ow, int oh);

void Render3D_Wheel(int delta, keycode_t mod);
void Render3D_RBScroll(int dx, int dy, keycode_t mod);

void Render3D_SetCameraPos(int new_x, int new_y);
void Render3D_GetCameraPos(int *x, int *y, float *angle);

bool Render3D_ParseUser(const char ** tokens, int num_tok);
void Render3D_WriteUser(FILE *fp);

#endif  /* __EUREKA_R_RENDER__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
