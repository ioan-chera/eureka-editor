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

#ifndef __EUREKA_R_MISC_H__
#define __EUREKA_R_MISC_H__


/* Width and height of font cell. Those figures are not meant to
   represent the actual size of the character but rather the step
   between two characters. For example, for the original 8x8 BGI
   font, FONTW was 8 and FONTH was 10. DrawScreenText() is supposed
   to draw characters properly _centered_ within that space, taking
   into account the optional underscoring. */
extern unsigned FONTW;
extern unsigned FONTH;
extern int font_xofs;
extern int font_yofs;


/* Parameters set by command line args and configuration file */


/* Global variables */
extern int   ScrMaxX;
extern int   ScrMaxY;

/* gfx.cc */
void SetWindowSize (int width, int height);

void DrawScreenText (int, int, const char *, ...);
void DrawScreenString (int, int, const char *);


// OLD CRUD : REMOVE
#define  set_colour(X)  do{}while(0)
#define push_colour(X)  do{}while(0)
#define  pop_colour(X)  do{}while(0)


#endif  /* __EUREKA_R_MISC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
