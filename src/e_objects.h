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


void SelectObjectsInBox(selection_c *list, int, int, int, int, int);
void HighlightObject(int, int, int);

void MoveObjects(selection_c * list, int delta_x, int delta_y, int delta_z = 0);
void DragSingleObject(int obj_num, int delta_x, int delta_y, int delta_z = 0);

void DeleteObjects(selection_c * list);

bool LineTouchesBox(int, int, int, int, int);

void GetDragFocus(int *x, int *y, int map_x, int map_y);

void Insert_Vertex(bool force_continue, bool no_fill, bool is_button = false);
void Insert_Vertex_split(int split_ld, int new_x, int new_y);


struct transform_t
{
public:
	int mid_x, mid_y;

	float scale_x, scale_y;

	float skew_x, skew_y;

	int rotate;  // 16 bits (65536 = 360 degrees)

public:
	void Clear();

	void Apply(int *x, int *y) const;
};


typedef enum
{
	TRANS_K_Scale = 0,		// scale and keep aspect
	TRANS_K_Stretch,		// scale X and Y independently
	TRANS_K_Rotate,			// rotate
	TRANS_K_RotScale,		// rotate and scale at same time
	TRANS_K_Skew			// skew (shear) along an axis

} transform_keyword_e;


void Objs_CalcMiddle(selection_c * list, int *x, int *y);
void Objs_CalcBBox(selection_c * list, int *x1, int *y1, int *x2, int *y2);

void TransformObjects(transform_t& param);

void ScaleObjects3(double scale_x, double scale_y, int pos_x, int pos_y);
void ScaleObjects4(double scale_x, double scale_y, double scale_z,
                   int pos_x, int pos_y, int pos_z);

void RotateObjects3(double deg, int pos_x, int pos_y);


/* commands */

void CMD_Insert(void);

void CMD_CopyProperties(void);

void CMD_Mirror  (void);
void CMD_Rotate90(void);
void CMD_Enlarge (void);
void CMD_Shrink  (void);
void CMD_Quantize(void);


#endif  /* __EUREKA_OBJECTS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
