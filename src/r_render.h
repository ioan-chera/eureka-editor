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
	int p_type = 0;
	double px = 0.0, py = 0.0;

	// view position.
	double x = 0.0, y = 0.0, z = 0.0;

	// view direction.  angle is in radians
	double angle = 0.0;
	double Sin = 0.0, Cos = 0.0;

	// screen buffer.
	int screen_w = 0, screen_h = 0;
	img_pixel_t *screen = nullptr;

	float aspect_sh;
	float aspect_sw;  // screen_w * aspect_ratio

	bool texturing = false;
	bool sprites = false;
	bool lighting = false;

	bool gravity = true;  // when true, walk on ground

	std::vector<int> thing_sectors;

	// current mouse coords (in window), invalid if -1
	int mouse_x = -1, mouse_y = -1;

private:
	Instance &inst;

public:
	explicit Render_View_t(Instance &inst) : inst(inst)
	{
	}

	void SetAngle(float new_ang);
	void FindGroundZ();
	void CalcAspect();

	void UpdateScreen(int ow, int oh);
	void PrepareToRender(int ow, int oh);

	double DistToViewPlane(v2double_t map);

	/* r_editing_info_t stuff */

	void AddAdjustSide(const Objid& obj);
	float AdjustDistFactor(float view_x, float view_y);
};

namespace global
{
	extern bool use_npot_textures;
};

void Render3D_RegisterCommands();

void Render3D_Enable(Instance &inst, bool _enable);

// this is basically the FLTK draw() method
void Render3D_Draw(Instance &inst, int ox, int oy, int ow, int oh);

// perform a query to see what the mouse pointer is over.
// returns true if something was hit, false otherwise.
// [ see the struct definition for more details... ]
bool Render3D_Query(Instance &inst, Objid& hl, int sx, int sy, int ox, int oy, int ow, int oh);

void Render3D_ScrollMap(Instance &inst, v2int_t dpos = {}, keycode_t mod = 0);

void Render3D_DragThings(Instance &inst);
void Render3D_DragSectors(Instance &inst);

void Render3D_NotifyBegin();
void Render3D_NotifyInsert(ObjType type, int objnum);
void Render3D_NotifyDelete(const Document &doc, ObjType type, int objnum);
void Render3D_NotifyChange(ObjType type, int objnum, int field);
void Render3D_NotifyEnd(Instance &inst);

int Render3D_CalcRotation(double viewAngle_rad, int thingAngle_deg);

/* API for rendering a scene (etc) */

void RGL_RenderWorld(Instance &inst, int ox, int oy, int ow, int oh);

#endif  /* __EUREKA_R_RENDER__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
