//------------------------------------------------------------------------
//  OBJECT STUFF
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

#ifndef __EUREKA_OBJ_ID_H__
#define __EUREKA_OBJ_ID_H__

#include "e_basis.h"


// Special object numbers
typedef s16_t  obj_no_t;

#define OBJ_NO_NONE    -1

#define is_obj(n)   ((n) >= 0)

class Objid
{
public:
    obj_type_e type;
    obj_no_t   num;

    Objid () { num = -1; type = OBJ_NONE; }
    Objid (obj_type_e t, obj_no_t n) { type = t; num = n; }

    void clear () { num = -1; type = OBJ_NONE; }

    bool operator== (const Objid& other) const
    {
      return (other.type == type) && (other.num == num);
    }
    bool is_nil     () const { return num <  0 || type == OBJ_NONE; }
    bool operator() () const { return num >= 0 && type != OBJ_NONE; } 
};

#endif  /* __EUREKA_OBJ_ID_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
