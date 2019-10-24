//------------------------------------------------------------------------
//  3D RENDERING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
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

#include "im_img.h"


struct Render_View_t
{
public:
	// player type and position.
	int p_type;
	double px, py;

	// view position.
	double x, y, z;

	// view direction.  angle is in radians
	double angle;
	double Sin, Cos;

	// screen buffer.
	int screen_w, screen_h;
	img_pixel_t *screen;

	float aspect_sh;
	float aspect_sw;  // screen_w * aspect_ratio

	bool texturing;
	bool sprites;
	bool lighting;

	bool gravity;  // when true, walk on ground

	std::vector<int> thing_sectors;
	int thsec_sector_num;
	bool thsec_invalidated;

	// navigation loop info
	bool is_scrolling;
	float scroll_speed;

	unsigned int nav_time;

	float nav_fwd, nav_back;
	float nav_left, nav_right;
	float nav_up, nav_down;
	float nav_turn_L, nav_turn_R;

	/* r_editing_info_t stuff */

	// current highlighted wotsit
	Objid current_hl;

	// state for adjusting offsets via the mouse
	std::vector<int> adjust_sides;
	std::vector<int> adjust_lines;

	float adjust_dx, adjust_dx_factor;
	float adjust_dy, adjust_dy_factor;

	std::vector<int> saved_x_offsets;
	std::vector<int> saved_y_offsets;

public:
	Render_View_t();
	~Render_View_t();

	void SetAngle(float new_ang);
	void FindGroundZ();
	void CalcAspect();
	void FindThingSectors();

	img_pixel_t DoomLightRemap(int light, float dist, img_pixel_t pixel);

	void UpdateScreen(int ow, int oh);
	void PrepareToRender(int ow, int oh);

	/* r_editing_info_t stuff */

	void AddAdjustSide(const Objid& obj);
	float AdjustDistFactor(float view_x, float view_y);

	void SaveOffsets();
	void RestoreOffsets();
};


extern Render_View_t r_view;


void Render3D_Setup();
void Render3D_RegisterCommands();

void Render3D_Enable(bool _enable);

// this is basically the FLTK draw() method
void Render3D_Draw(int ox, int oy, int ow, int oh);

// perform a query to see what the mouse pointer is over.
// returns true if something was hit, false otherwise.
// [ see the struct definition for more details... ]
bool Render3D_Query(Objid& hl, int sx, int sy, int ox, int oy, int ow, int oh);

void Render3D_MouseMotion(int x, int y, keycode_t mod, int dx, int dy);
void Render3D_AdjustOffsets(int mode, int dx = 0, int dy = 0);

void Render3D_Navigate();
void Render3D_ClearNav();

void Render3D_UpdateHighlight();
void Render3D_SaveHighlight();
void Render3D_RestoreHighlight();

void Render3D_CB_Cut();
void Render3D_CB_Copy();
void Render3D_CB_Paste();

void Render3D_SetCameraPos(double new_x, double new_y);
void Render3D_GetCameraPos(double *x, double *y, float *angle);

bool Render3D_ParseUser(const char ** tokens, int num_tok);
void Render3D_WriteUser(FILE *fp);


/* API for rendering a scene (etc) */

void SW_RenderWorld(int ox, int oy, int ow, int oh);
bool SW_QueryPoint(Objid& hl, int qx, int qy);

void RGL_RenderWorld(int ox, int oy, int ow, int oh);

#endif  /* __EUREKA_R_RENDER__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
