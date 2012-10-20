//------------------------------------------------------------------------
//  WAD PIC LOADER
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

#ifndef __EUREKA_W_LOADPIC_H__
#define __EUREKA_W_LOADPIC_H__

#include "im_img.h"
#include "w_wad.h"


bool LoadPicture(Img& img,
	Lump_c *lump,           /* Lump containing picture */
	const char *pic_name,   /* Picture name, for messages */
	int pic_x_offset,    /* Coordinates of top left corner of picture */
	int pic_y_offset,    /* relative to top left corner of buffer. */
	int *pic_width = NULL,    /* To return the size of the picture */
	int *pic_height = NULL);  /* (can be NULL) */


#endif  /* __EUREKA_W_LOADPIC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
