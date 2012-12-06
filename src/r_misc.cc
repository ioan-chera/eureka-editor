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




// Shared by DrawScreenText() and DrawScreenString()

/* Coordinates of first character of last string drawn with
   DrawScreenText() or DrawScreenString(). */
static int lastx0;
static int lasty0;

// Where the last DrawScreen{Text,String}() left the "cursor".
static int lastxcur;
static int lastycur;


/*
 *  DrawScreenText - format and display a string
 *
 *  Write text to the screen in printf() fashion.
 *  The top left corner of the first character is at (<scrx>, <scry>)
 *  If <scrx> == -1, the text is printed at the same abscissa
 *  as the last text printed with this function.
 *  If <scry> == -1, the text is printed one line (FONTH pixels)
 *  below the last text printed with this function.
 *  If <msg> == NULL, no text is printed. Useful to set the
 *  coordinates for the next time.
 */
void DrawScreenText (int scrx, int scry, const char *msg, ...)
{
	char temp[120];

	va_list args;

	// <msg> == NULL: print nothing, just set the coordinates.
	if (msg == NULL)
	{
		if (scrx != -1 && scrx != -2)
		{
			lastx0   = scrx;
			lastxcur = scrx;
		}
		if (scry != -1 && scry != -2)
		{
			lasty0   = scry;  // Note: no "+ FONTH"
			lastycur = scry;
		}
		return;
	}

	va_start (args, msg);
	y_vsnprintf (temp, sizeof temp, msg, args);
	DrawScreenString (scrx, scry, temp);
}


/*
 *  DrawScreenString - display a string
 *
 *  Same thing as DrawScreenText() except that the string is
 *  printed verbatim (no formatting or conversion).
 *
 *  A "\1" in the string is not displayed but causes
 *  subsequent characters to be displayed in WINLABEL (or
 *  WINLABEL_DIM if the current colour before the function
 *  was called was WINFG_DIM).
 *
 *  A "\2" in the string is not displayed but causes
 *  subsequent characters to be displayed in the same colour
 *  that was active before the function was called.
 *
 *  The string can contain any number of "\1" and "\2".
 *  Regardless, upon return from the function, the current
 *  colour is restored to what it was before the function
 *  was called.
 *
 *  This colour switching business was hacked in a hurry.
 *  Feel free to improve it.
 */
void DrawScreenString (int scrx, int scry, const char *str)
{
	int x; int y;

	/* FIXME originally, the test was "< 0". Because it broke
	   when the screen was too small, I changed it to a more
	   specific "== -1". A quick and very dirty hack ! */
	if (scrx == -1)
		x = lastx0;
	else if (scrx == -2)
		x = lastxcur;
	else
		x = scrx;
	if (scry == -1)
		y = lasty0;
	else if (scry == -2)
		y = lastycur;
	else
		y = scry;
	size_t len = strlen (str);

	if (strchr (str, '\1') == 0)
	{
		fl_draw (str, len, x - font_xofs, y + font_yofs);
	}
	else
	{
		int xx = x;
		len = 0;
		for (const char *p = str; *p != '\0';)
		{
			int i;
			for (i = 0; p[i] != '\0' && p[i] != '\1' && p[i] != '\2'; i++)
				;
			len += i;
			if (i > 0)
			{
				fl_draw (p, i, xx - font_xofs, y + font_yofs);
				xx += i * FONTW;
			}
			if (p[i] == '\0')
				break;
//			if (p[i] == '\1')
//				set_colour (save == WINFG_DIM ? WINLABEL_DIM : WINLABEL);
//			else if (p[i] == '\2')
//				set_colour (save);
			i++;
			p += i;
		}
//		set_colour (save);
	}

	lastxcur = x + FONTW * len;
	lastycur = y;
	if (scrx != -2)
		lastx0 = x;
	if (scry != -2)
		lasty0 = y + FONTH;
}



//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
