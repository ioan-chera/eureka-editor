//------------------------------------------------------------------------
//  3D RENDERING
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

#ifndef __EUREKA_R_RENDER__
#define __EUREKA_R_RENDER__


class UI_Render3D : public Fl_Widget
{
public:
	UI_Render3D(int X, int Y, int W, int H);

	virtual ~UI_Render3D();

	// FLTK virtual methods for drawing / event handling
	void draw();

	int handle(int event);

	// perform a query to see what the mouse pointer is over.
	// returns true if something was hit, false otherwise.
	// [ see the struct definition for more details... ]
	bool query(Obj3d_t& hl, int sx, int sy);

private:
	void BlitLores(int ox, int oy, int ow, int oh);
	void BlitHires(int ox, int oy, int ow, int oh);

	void DrawInfoBar();

	void IB_Number   (int& cx, int& cy, const char *label, int value, int size);
	void IB_Flag     (int& cx, int& cy, bool value, const char *label_on, const char *label_off);
	void IB_Highlight(int& cx, int& cy);
};


void Render3D_Setup();
void Render3D_RegisterCommands();

void Render3D_Enable(bool _enable);

void Render3D_MouseMotion(int x, int y, keycode_t mod, int dx, int dy);
void Render3D_AdjustOffsets(int mode, int dx = 0, int dy = 0);

void Render3D_Navigate();
void Render3D_ClearNav();
void Render3D_UpdateHighlight();

void Render3D_ClearSelection();
bool Render3D_ClipboardOp(char what);
bool Render3D_BrowsedItem(char kind, int number, const char *name, int e_state);

void Render3D_SetCameraPos(int new_x, int new_y);
void Render3D_GetCameraPos(int *x, int *y, float *angle);

bool Render3D_ParseUser(const char ** tokens, int num_tok);
void Render3D_WriteUser(FILE *fp);

#endif  /* __EUREKA_R_RENDER__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
