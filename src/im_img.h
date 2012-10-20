//------------------------------------------------------------------------
//  IMAGES
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

#ifndef __EUREKA_IM_IMG_H__
#define __EUREKA_IM_IMG_H__

typedef byte  img_pixel_t;

class Img_priv;


/* The colour number used to represent transparent pixels in an Img.
   Any value will do but zero is probably best performance-wise.
 */
const img_pixel_t TRANS_PIXEL = 247;


class Img
{
  public :
    Img ();
    Img (int width, int height, bool opaque);
    ~Img ();
    bool               is_null    () const; // Is it a null image ?
    int          width      () const; // Return the width
    int          height     () const; // Return the height
    const img_pixel_t *buf        () const; // Return pointer on buffer
    img_pixel_t       *wbuf       ();   // Return pointer on buffer
    void               clear      ();
    void               set_opaque (bool opaque);
    void               resize     (int width, int height);

    Img * spectrify() const;

  private :
    Img            (const Img&);  // Too lazy to implement it
    Img& operator= (const Img&);  // Too lazy to implement it
    Img_priv *p;
};


// FIXME: methods of Img
void scale_img (const Img& img, double scale, Img& omg);


#endif  /* __EUREKA_IM_IMG_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
