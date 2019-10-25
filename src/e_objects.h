//------------------------------------------------------------------------
//  OBJECT STUFF
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

#ifndef __EUREKA_OBJECTS_H__
#define __EUREKA_OBJECTS_H__


void MoveObjects(selection_c * list, double delta_x, double delta_y, double delta_z = 0);
void DragSingleObject(Objid& obj, double delta_x, double delta_y, double delta_z = 0);

void DeleteObjects(selection_c * list);

bool LineTouchesBox(int ld, double x0, double y0, double x1, double y1);

void GetDragFocus(double *x, double *y, double map_x, double map_y);


struct transform_t
{
public:
	double mid_x, mid_y;
	double scale_x, scale_y;
	double skew_x, skew_y;

	int rotate;  // 16 bits (65536 = 360 degrees)

public:
	void Clear();

	void Apply(double *x, double *y) const;
};


typedef enum
{
	TRANS_K_Scale = 0,		// scale and keep aspect
	TRANS_K_Stretch,		// scale X and Y independently
	TRANS_K_Rotate,			// rotate
	TRANS_K_RotScale,		// rotate and scale at same time
	TRANS_K_Skew			// skew (shear) along an axis

} transform_keyword_e;


void Objs_CalcMiddle(selection_c * list, double *x, double *y);
void Objs_CalcBBox(selection_c * list, double *x1, double *y1, double *x2, double *y2);

void TransformObjects(transform_t& param);

void ScaleObjects3(double scale_x, double scale_y, double pos_x, double pos_y);
void ScaleObjects4(double scale_x, double scale_y, double scale_z,
                   double pos_x, double pos_y, double pos_z);

void RotateObjects3(double deg, double pos_x, double pos_y);


/* commands */

void CMD_Insert();

void CMD_CopyProperties();

void CMD_Mirror  ();
void CMD_Rotate90();
void CMD_Enlarge ();
void CMD_Shrink  ();
void CMD_Quantize();


#endif  /* __EUREKA_OBJECTS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
