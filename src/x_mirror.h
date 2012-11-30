//------------------------------------------------------------------------
//  MIRROR / ROTATE / ETC OPS
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

#ifndef __EUREKA_X_MIRROR_H__
#define __EUREKA_X_MIRROR_H__


struct scale_param_t
{
	int mid_x, mid_y;
	float scale_x, scale_y;
	int rotate;  // 16 bits (65536 = 360 degrees)

	void Apply(int *x, int *y) const;
};


void Objs_CalcMiddle(selection_c * list, int *x, int *y);
void Objs_CalcBBox(selection_c * list, int *x1, int *y1, int *x2, int *y2);

void CMD_MirrorObjects(bool is_vert);
// FIXME: rename these two
void CMD_RotateObjects(bool anti_clockwise);  // 90 degrees
void CMD_ScaleObjects (bool is_half);

void CMD_ScaleObjects2(scale_param_t& param);

void CMD_ScaleObjectsByStr(const char *x_val, const char *y_val,
                           int x_origin, int y_origin);

void CMD_QuantizeObjects();


#if 0
int exchange_objects_numbers (int obj_type, SelPtr list, bool adjust);
#endif


#endif  /* __EUREKA_X_MIRROR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
