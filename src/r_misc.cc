//------------------------------------------------------------------------
//  GRAPHICS ROUTINES
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

#include "main.h"

#include "im_color.h"
#include "r_misc.h"
#include "r_grid.h"
#include "levels.h"  /* Level */
#include "r_render.h"

#include "ui_window.h"



int KF;
int KF_fonth;


int ScrMaxX;    // Maximum display X-coord of screen/window
int ScrMaxY;    // Maximum display Y-coord of screen/window

unsigned FONTH = 16;
unsigned FONTW =  9;
int font_xofs = 0;
int font_yofs = 12;


/*
 *  Prototypes
 */


/*
 *  SetWindowSize - set the size of the edit window
 */
void SetWindowSize (int width, int height)
{
  // Am I called uselessly ?
  if (width == ScrMaxX + 1 && height == ScrMaxY + 1)
    return;

  ScrMaxX = width - 1;
  ScrMaxY = height - 1;
}



//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
