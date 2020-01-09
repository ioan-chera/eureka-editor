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

#include "main.h"

#include <map>
#include <algorithm>

#ifndef NO_OPENGL
#include "FL/gl.h"
#endif

#include "e_basis.h"
#include "e_cutpaste.h"
#include "e_hover.h"
#include "e_linedef.h"
#include "e_sector.h"
#include "e_main.h"
#include "m_game.h"
#include "m_events.h"
#include "r_render.h"
#include "ui_window.h"


// config items
rgb_color_t transparent_col = RGB_MAKE(0, 255, 255);

bool render_high_detail    = false;
bool render_lock_gravity   = false;
bool render_missing_bright = true;
bool render_unknown_bright = true;

int  render_far_clip = 32768;

// in original DOOM pixels were 20% taller than wide, giving 0.83
// as the pixel aspect ratio.
int  render_pixel_aspect = 83;  //  100 * width / height


namespace thing_sec_cache
{
	int invalid_low, invalid_high;

	void ResetRange()
	{
		invalid_low  = 999999999;
		invalid_high = -1;
	}

	void InvalidateThing(int th)
	{
		invalid_low  = MIN(invalid_low,  th);
		invalid_high = MAX(invalid_high, th);
	}

	void InvalidateAll()
	{
		invalid_low  = 0;
		invalid_high = NumThings - 1;
	}

	void Update();
};


Render_View_t r_view;


Render_View_t::Render_View_t() :
	p_type(0), px(), py(),
	x(), y(), z(),
	angle(), Sin(), Cos(),
	screen_w(), screen_h(), screen(NULL),
	texturing(false), sprites(false), lighting(false),
	gravity(true),
	thing_sectors(),
	current_hl()
{ }

Render_View_t::~Render_View_t()
{ }

void Render_View_t::SetAngle(float new_ang)
{
	angle = new_ang;

	if (angle >= 2*M_PI)
		angle -= 2*M_PI;
	else if (angle < 0)
		angle += 2*M_PI;

	Sin = sin(angle);
	Cos = cos(angle);
}

void Render_View_t::FindGroundZ()
{
	// test a grid of points on the player's bounding box, and
	// use the maximum floor of all contacted sectors.

	int max_floor = INT_MIN;

	for (int dx = -2 ; dx <= 2 ; dx++)
	for (int dy = -2 ; dy <= 2 ; dy++)
	{
		Objid o;

		GetNearObject(o, OBJ_SECTORS, int(x + dx*8), int(y + dy*8));

		if (o.num >= 0)
			max_floor = MAX(max_floor, Sectors[o.num]->floorh);
	}

	if (max_floor != INT_MIN)
		z = max_floor + Misc_info.view_height;
}

void Render_View_t::CalcAspect()
{
	aspect_sw = screen_w;	 // things break if these are different

	aspect_sh = screen_w / (render_pixel_aspect / 100.0);
}

double Render_View_t::DistToViewPlane(double map_x, double map_y)
{
	map_x -= r_view.x;
	map_y -= r_view.y;

	return map_x * r_view.Cos + map_y * r_view.Sin;
}

void Render_View_t::UpdateScreen(int ow, int oh)
{
	// in low detail mode, setup size so that expansion always covers
	// our window (i.e. we draw a bit more than we need).

	int new_sw = render_high_detail ? ow : (ow + 1) / 2;
	int new_sh = render_high_detail ? oh : (oh + 1) / 2;

	if (!screen || screen_w != new_sw || screen_h != new_sh)
	{
		screen_w = new_sw;
		screen_h = new_sh;

		if (screen)
		{
			delete[] screen;
			screen = NULL;
		}
	}

	// we don't need a screen buffer when using OpenGL
#ifdef NO_OPENGL
	if (!screen)
		screen = new img_pixel_t [screen_w * screen_h];
#endif

	CalcAspect();
}

void Render_View_t::PrepareToRender(int ow, int oh)
{
	thing_sec_cache::Update();

	UpdateScreen(ow, oh);

	if (gravity)
		FindGroundZ();
}


static Thing *FindPlayer(int typenum)
{
	// need to search backwards (to handle Voodoo dolls properly)

	for (int i = NumThings-1 ; i >= 0 ; i--)
		if (Things[i]->type == typenum)
			return Things[i];

	return NULL;  // not found
}


//------------------------------------------------------------------------

namespace thing_sec_cache
{
	void Update()
	{
		// guarantee that thing_sectors has the correct size.
		// [ prevent a potential crash ]
		if (NumThings != (int)r_view.thing_sectors.size())
		{
			r_view.thing_sectors.resize(NumThings);
			thing_sec_cache::InvalidateAll();
		}

		// nothing changed?
		if (invalid_low > invalid_high)
			return;

		for (int i = invalid_low ; i <= invalid_high ; i++)
		{
			Objid obj;
			GetNearObject(obj, OBJ_SECTORS, Things[i]->x(), Things[i]->y());

			r_view.thing_sectors[i] = obj.num;
		}

		thing_sec_cache::ResetRange();
	}
}

void Render3D_NotifyBegin()
{
	thing_sec_cache::ResetRange();
}

void Render3D_NotifyInsert(obj_type_e type, int objnum)
{
	if (type == OBJ_THINGS)
		thing_sec_cache::InvalidateThing(objnum);
}

void Render3D_NotifyDelete(obj_type_e type, int objnum)
{
	if (type == OBJ_THINGS || type == OBJ_SECTORS)
		thing_sec_cache::InvalidateAll();
}

void Render3D_NotifyChange(obj_type_e type, int objnum, int field)
{
	if (type == OBJ_THINGS &&
		(field == Thing::F_X || field == Thing::F_Y))
	{
		thing_sec_cache::InvalidateThing(objnum);
	}
}

void Render3D_NotifyEnd()
{
	thing_sec_cache::Update();
}


//------------------------------------------------------------------------

class save_obj_field_c
{
public:
	int obj;    // object number (edit.mode is implicit type)
	int field;  // e.g. Thing::F_X
	int value;  // the saved value

public:
	save_obj_field_c(int _obj, int _field, int _value) :
		obj(_obj), field(_field), value(_value)
	{ }

	~save_obj_field_c()
	{ }
};


class SaveBucket_c
{
private:
	obj_type_e type;

	std::vector< save_obj_field_c > fields;

public:
	SaveBucket_c(obj_type_e _type) : type(_type), fields()
	{ }

	~SaveBucket_c()
	{ }

	void Clear()
	{
		fields.clear();
	}

	void Clear(obj_type_e _type)
	{
		type = _type;
		fields.clear();
	}

	void Save(int obj, int field)
	{
		// is it already saved?
		for (size_t i = 0 ; i < fields.size() ; i++)
			if (fields[i].obj == obj && fields[i].field == field)
				return;

		int value = RawGet(obj, field);

		fields.push_back(save_obj_field_c(obj, field, value));
	}

	void RestoreAll()
	{
		for (size_t i = 0 ; i < fields.size() ; i++)
		{
			RawSet(fields[i].obj, fields[i].field, fields[i].value);
		}
	}

	void ApplyTemp(int field, int delta)
	{
		for (size_t i = 0 ; i < fields.size() ; i++)
		{
			if (fields[i].field == field)
				RawSet(fields[i].obj, field, fields[i].value + delta);
		}
	}

	void ApplyToBasis(int field, int delta)
	{
		for (size_t i = 0 ; i < fields.size() ; i++)
		{
			if (fields[i].field == field)
			{
				BA_Change(type, fields[i].obj, (byte)field, fields[i].value + delta);
			}
		}
	}

private:
	int * RawObjPointer(int objnum)
	{
		switch (type)
		{
			case OBJ_THINGS:
				return (int*) Things[objnum];

			case OBJ_VERTICES:
				return (int*) Vertices[objnum];

			case OBJ_SECTORS:
				return (int*) Sectors[objnum];

			case OBJ_SIDEDEFS:
				return (int*) SideDefs[objnum];

			case OBJ_LINEDEFS:
				return (int*) LineDefs[objnum];

			default:
				BugError("SaveBucket with bad mode\n");
				return NULL; /* NOT REACHED */
		}
	}

	int RawGet(int objnum, int field)
	{
		int *ptr = RawObjPointer(objnum) + field;
		return *ptr;
	}

	void RawSet(int objnum, int field, int value)
	{
		int *ptr = RawObjPointer(objnum) + field;
		*ptr = value;
	}
};


static void AdjustOfs_UpdateBBox(int ld_num)
{
	const LineDef *L = LineDefs[ld_num];

	float lx1 = L->Start()->x();
	float ly1 = L->Start()->y();
	float lx2 = L->End()->x();
	float ly2 = L->End()->y();

	if (lx1 > lx2) std::swap(lx1, lx2);
	if (ly1 > ly2) std::swap(ly1, ly2);

	edit.adjust_bbox.x1 = MIN(edit.adjust_bbox.x1, lx1);
	edit.adjust_bbox.y1 = MIN(edit.adjust_bbox.y1, ly1);
	edit.adjust_bbox.x2 = MAX(edit.adjust_bbox.x2, lx2);
	edit.adjust_bbox.y2 = MAX(edit.adjust_bbox.y2, ly2);
}

void AdjustOfs_CalcDistFactor(float& dx_factor, float& dy_factor)
{
	// this computes how far to move the offsets for each screen pixel
	// the mouse moves.  we want it to appear as though each texture
	// is being dragged by the mouse, e.g. if you click on the middle
	// of a switch, that switch follows the mouse pointer around.
	// such an effect can only be approximate though.

	float dx = (r_view.x < edit.adjust_bbox.x1) ? (edit.adjust_bbox.x1 - r_view.x) :
			   (r_view.x > edit.adjust_bbox.x2) ? (r_view.x - edit.adjust_bbox.x2) : 0;

	float dy = (r_view.y < edit.adjust_bbox.y1) ? (edit.adjust_bbox.y1 - r_view.y) :
			   (r_view.y > edit.adjust_bbox.y2) ? (r_view.y - edit.adjust_bbox.y2) : 0;

	float dist = hypot(dx, dy);

	dist = CLAMP(20, dist, 1000);

	dx_factor = dist / r_view.aspect_sw;
	dy_factor = dist / r_view.aspect_sh;
}

static void AdjustOfs_Add(int ld_num, int part)
{
	if (! edit.adjust_bucket)
		return;

	const LineDef *L = LineDefs[ld_num];

	// ignore invalid sides (sanity check)
	int sd_num = (part & PART_LF_ALL) ? L->left : L->right;
	if (sd_num < 0)
		return;

	// TODO : UDMF ports can allow full control over each part

	edit.adjust_bucket->Save(sd_num, SideDef::F_X_OFFSET);
	edit.adjust_bucket->Save(sd_num, SideDef::F_Y_OFFSET);
}

static void AdjustOfs_Begin()
{
	if (edit.adjust_bucket)
		delete edit.adjust_bucket;

	edit.adjust_bucket = new SaveBucket_c(OBJ_SIDEDEFS);
	edit.adjust_lax = Exec_HasFlag("/LAX");

	int total_lines = 0;

	// we will compute the bbox of selected lines
	edit.adjust_bbox.x1 = edit.adjust_bbox.y1 = +9e9;
	edit.adjust_bbox.x2 = edit.adjust_bbox.y2 = -9e9;

	// find the sidedefs to adjust
	if (! edit.Selected->empty())
	{
		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			int ld_num = *it;
			byte parts = edit.Selected->get_ext(ld_num);

			// handle "simply selected" linedefs
			if (parts <= 1)
			{
				parts |= PART_RT_LOWER | PART_RT_UPPER;
				parts |= PART_LF_LOWER | PART_LF_UPPER;
			}

			total_lines++;
			AdjustOfs_UpdateBBox(ld_num);

			if (parts & PART_RT_LOWER) AdjustOfs_Add(ld_num, PART_RT_LOWER);
			if (parts & PART_RT_UPPER) AdjustOfs_Add(ld_num, PART_RT_UPPER);
			if (parts & PART_RT_RAIL)  AdjustOfs_Add(ld_num, PART_RT_RAIL);

			if (parts & PART_LF_LOWER) AdjustOfs_Add(ld_num, PART_LF_LOWER);
			if (parts & PART_LF_UPPER) AdjustOfs_Add(ld_num, PART_LF_UPPER);
			if (parts & PART_LF_RAIL)  AdjustOfs_Add(ld_num, PART_LF_RAIL);
		}
	}
	else if (edit.highlight.valid())
	{
		int  ld_num = edit.highlight.num;
		byte parts  = edit.highlight.parts;

		if (parts >= 2)
		{
			AdjustOfs_Add(ld_num, parts);
			AdjustOfs_UpdateBBox(ld_num);
			total_lines++;
		}
	}

	if (total_lines == 0)
	{
		Beep("nothing to adjust");
		return;
	}

	edit.adjust_dx = 0;
	edit.adjust_dy = 0;

	edit.adjust_lax = Exec_HasFlag("/LAX");

	Editor_SetAction(ACT_ADJUST_OFS);
}

static void AdjustOfs_Finish()
{
	if (! edit.adjust_bucket)
	{
		Editor_ClearAction();
		return;
	}

	int dx = I_ROUND(edit.adjust_dx);
	int dy = I_ROUND(edit.adjust_dy);

	if (dx || dy)
	{
		BA_Begin();
		BA_Message("adjusted offsets");

		edit.adjust_bucket->ApplyToBasis(SideDef::F_X_OFFSET, dx);
		edit.adjust_bucket->ApplyToBasis(SideDef::F_Y_OFFSET, dy);

		BA_End();
	}

	delete edit.adjust_bucket;
	edit.adjust_bucket = NULL;

	Editor_ClearAction();
}

static void AdjustOfs_Delta(int dx, int dy)
{
	if (! edit.adjust_bucket)
		return;

	if (dx == 0 && dy == 0)
		return;

	bool force_one_dir = true;

	if (force_one_dir)
	{
		if (abs(dx) >= abs(dy))
			dy = 0;
		else
			dx = 0;
	}

	keycode_t mod = edit.adjust_lax ? M_ReadLaxModifiers() : 0;

	float factor = (mod & MOD_SHIFT) ? 0.5 : 2.0;

	if (!render_high_detail)
		factor = factor * 0.5;

	float dx_factor, dy_factor;
	AdjustOfs_CalcDistFactor(dx_factor, dy_factor);

	edit.adjust_dx -= dx * factor * dx_factor;
	edit.adjust_dy -= dy * factor * dy_factor;

	RedrawMap();
}


void AdjustOfs_RenderAnte()
{
	if (edit.action == ACT_ADJUST_OFS && edit.adjust_bucket)
	{
		int dx = I_ROUND(edit.adjust_dx);
		int dy = I_ROUND(edit.adjust_dy);

		// change it temporarily (just for the render)
		edit.adjust_bucket->ApplyTemp(SideDef::F_X_OFFSET, dx);
		edit.adjust_bucket->ApplyTemp(SideDef::F_Y_OFFSET, dy);
	}
}

void AdjustOfs_RenderPost()
{
	if (edit.action == ACT_ADJUST_OFS && edit.adjust_bucket)
	{
		edit.adjust_bucket->RestoreAll();
	}
}


//------------------------------------------------------------------------

static Thing *player;

extern void CheckBeginDrag();


void Render3D_Draw(int ox, int oy, int ow, int oh)
{
	r_view.PrepareToRender(ow, oh);

	AdjustOfs_RenderAnte();

#ifdef NO_OPENGL
	SW_RenderWorld(ox, oy, ow, oh);
#else
	RGL_RenderWorld(ox, oy, ow, oh);
#endif

	AdjustOfs_RenderPost();
}


bool Render3D_Query(Objid& hl, int sx, int sy)
{
	int ow = main_win->canvas->w();
	int oh = main_win->canvas->h();

#ifdef NO_OPENGL
	// in OpenGL mode, UI_Canvas is a window and that means the
	// event X/Y values are relative to *it* and not the main window.
	// hence the following is only needed in software mode.
	int ox = main_win->canvas->x();
	int oy = main_win->canvas->y();

	sx -= ox;
	sy -= oy;
#endif

	// force high detail mode for OpenGL, so we don't lose
	// precision when performing the query.
#ifndef NO_OPENGL
	render_high_detail = true;
#endif

	hl.clear();

	if (! edit.pointer_in_window)
		return false;

	r_view.PrepareToRender(ow, oh);

	return SW_QueryPoint(hl, sx, sy);
}


void Render3D_Setup()
{
	thing_sec_cache::InvalidateAll();
	r_view.thing_sectors.resize(0);

	if (! r_view.p_type)
	{
		r_view.p_type = THING_PLAYER1;
		r_view.px = 99999;
	}

	player = FindPlayer(r_view.p_type);

	if (! player)
	{
		if (r_view.p_type != THING_DEATHMATCH)
			r_view.p_type = THING_DEATHMATCH;

		player = FindPlayer(r_view.p_type);
	}

	if (player && !(r_view.px == player->x() && r_view.py == player->y()))
	{
		// if player moved, re-create view parameters

		r_view.x = r_view.px = player->x();
		r_view.y = r_view.py = player->y();

		r_view.FindGroundZ();

		r_view.SetAngle(player->angle * M_PI / 180.0);
	}
	else
	{
		r_view.x = 0;
		r_view.y = 0;
		r_view.z = 64;

		r_view.SetAngle(0);
	}

	r_view.screen_w = -1;
	r_view.screen_h = -1;

	r_view.texturing  = true;
	r_view.sprites    = true;
	r_view.lighting   = true;
}


void Render3D_Enable(bool _enable)
{
	Editor_ClearAction();

	edit.render3d = _enable;

	edit.highlight.clear();
	edit.clicked.clear();
	edit.dragged.clear();

	// give keyboard focus to the appropriate large widget
	Fl::focus(main_win->canvas);

	main_win->scroll->UpdateRenderMode();
	main_win->info_bar->UpdateSecRend();

	if (edit.render3d)
	{
		main_win->info_bar->SetMouse(r_view.x, r_view.y);
	}
	else
	{
		main_win->canvas->PointerPos();
		main_win->info_bar->SetMouse(edit.map_x, edit.map_y);
	}

	RedrawMap();
}


void Render3D_ScrollMap(int dx, int dy, keycode_t mod)
{
	// we separate the movement into either turning or moving up/down
	// (never both at the same time : CONFIG IT THOUGH).

	bool force_one_dir = true;

	if (force_one_dir)
	{
		if (abs(dx) >= abs(dy))
			dy = 0;
		else
			dx = 0;
	}

	bool is_strafe = (mod & MOD_ALT) ? true : false;

	float mod_factor = 1.0;
	if (mod & MOD_SHIFT)   mod_factor = 0.4;
	if (mod & MOD_COMMAND) mod_factor = 2.5;

	float speed = edit.panning_speed * mod_factor;

	if (is_strafe)
	{
		r_view.x += r_view.Sin * dx * mod_factor;
		r_view.y -= r_view.Cos * dx * mod_factor;
	}
	else  // turn camera
	{
		double d_ang = dx * speed * M_PI / 480.0;

		r_view.SetAngle(r_view.angle - d_ang);
	}

	dy = -dy;  //TODO CONFIG ITEM

	if (is_strafe)
	{
		r_view.x += r_view.Cos * dy * mod_factor;
		r_view.y += r_view.Sin * dy * mod_factor;
	}
	else if (! (render_lock_gravity && r_view.gravity))
	{
		r_view.z += dy * speed * 0.75;

		r_view.gravity = false;
	}

	main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap();
}


static void DragSectors_Update()
{
	float ow = main_win->canvas->w();
	float x_slope = 100.0 / render_pixel_aspect;

	float factor = CLAMP(20, edit.drag_point_dist, 1000) / (ow * x_slope * 0.5);
	float map_dz = -edit.drag_screen_dy * factor;

	float step = 8.0;  // TODO config item

	if (map_dz > step*0.25)
		edit.drag_sector_dz = step * (int)ceil(map_dz / step);
	else if (map_dz < step*-0.25)
		edit.drag_sector_dz = step * (int)floor(map_dz / step);
	else
		edit.drag_sector_dz = 0;
}

void Render3D_DragSectors()
{
	int dz = I_ROUND(edit.drag_sector_dz);
	if (dz == 0)
		return;

	BA_Begin();

	if (dz > 0)
		BA_Message("raised sectors");
	else
		BA_Message("lowered sectors");

	if (edit.dragged.valid())
	{
		int parts = edit.dragged.parts;

		SEC_SafeRaiseLower(edit.dragged.num, parts, dz);
	}
	else
	{
		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			int parts = edit.Selected->get_ext(*it);
			parts &= ~1;

			SEC_SafeRaiseLower(*it, parts, dz);
		}
	}

	BA_End();
}


static void DragThings_Update()
{
	float ow = main_win->canvas->w();
//	float oh = main_win->canvas->h();

	float x_slope = 100.0 / render_pixel_aspect;
//	float y_slope = (float)oh / (float)ow;

	float dist = CLAMP(20, edit.drag_point_dist, 1000);

	float x_factor = dist / (ow * 0.5);
	float y_factor = dist / (ow * x_slope * 0.5);

	if (edit.drag_thing_up_down)
	{
		// vertical positioning in Hexen and UDMF formats
		float map_dz = -edit.drag_screen_dy * y_factor;

		// final result is in drag_cur_x/y/z
		edit.drag_cur_x = edit.drag_start_x;
		edit.drag_cur_y = edit.drag_start_y;
		edit.drag_cur_z = edit.drag_start_z + map_dz;
		return;
	}

	/* move thing around XY plane */

	edit.drag_cur_z = edit.drag_start_z;

	// vectors for view camera
	double fwd_vx = r_view.Cos;
	double fwd_vy = r_view.Sin;

	double side_vx =  fwd_vy;
	double side_vy = -fwd_vx;

	double dx =  edit.drag_screen_dx * x_factor;
	double dy = -edit.drag_screen_dy * y_factor * 2.0;

	// this usually won't happen, but is a reasonable fallback...
	if (edit.drag_thing_num < 0)
	{
		edit.drag_cur_x = edit.drag_start_x + dx * side_vx + dy * fwd_vx;
		edit.drag_cur_y = edit.drag_start_y + dx * side_vy + dy * fwd_vy;
		return;
	}

	// old code for depth calculation, works well in certain cases
	// but very poorly in other cases.
#if 0
	int sy1 = edit.click_screen_y;
	int sy2 = sy1 + edit.drag_screen_dy;

	if (sy1 >= oh/2 && sy2 >= oh/2)
	{
		double d1 = (edit.drag_thing_floorh - r_view.z) / (oh - sy1*2.0);
		double d2 = (edit.drag_thing_floorh - r_view.z) / (oh - sy2*2.0);

		d1 = d1 * ow;
		d2 = d2 * ow;

		dy = (d2 - d1) * 0.5;
	}
#endif

	const Thing *T = Things[edit.drag_thing_num];

	float old_x = T->x();
	float old_y = T->y();

	float new_x = old_x + dx * side_vx;
	float new_y = old_y + dx * side_vy;

	// recompute forward/back vector
	fwd_vx = new_x - r_view.x;
	fwd_vy = new_y - r_view.y;

	double fwd_len = hypot(fwd_vx, fwd_vy);
	if (fwd_len < 1)
		fwd_len = 1;

	new_x = new_x + dy * fwd_vx / fwd_len;
	new_y = new_y + dy * fwd_vy / fwd_len;

	// handle a change in floor height
	Objid old_sec;
	GetNearObject(old_sec, OBJ_SECTORS, old_x, old_y);

	Objid new_sec;
	GetNearObject(new_sec, OBJ_SECTORS, new_x, new_y);

	if (old_sec.valid() && new_sec.valid())
	{
		float old_z = Sectors[old_sec.num]->floorh;
		float new_z = Sectors[new_sec.num]->floorh;

		// intent here is to show proper position, NOT raise/lower things.
		// [ perhaps add a new variable? ]
		edit.drag_cur_z += (new_z - old_z);
	}

	edit.drag_cur_x = edit.drag_start_x + new_x - old_x;
	edit.drag_cur_y = edit.drag_start_y + new_y - old_y;
}

void Render3D_DragThings()
{
	double dx = edit.drag_cur_x - edit.drag_start_x;
	double dy = edit.drag_cur_y - edit.drag_start_y;
	double dz = edit.drag_cur_z - edit.drag_start_z;

	// for movement in XY plane, ensure we don't raise/lower things
	if (! edit.drag_thing_up_down)
		dz = 0.0;

	if (edit.dragged.valid())
	{
		selection_c sel(OBJ_THINGS);
		sel.set(edit.dragged.num);

		MoveObjects(&sel, dx, dy, dz);
	}
	else
	{
		MoveObjects(edit.Selected, dx, dy, dz);
	}

	RedrawMap();
}


void Render3D_MouseMotion(int x, int y, keycode_t mod, int dx, int dy)
{
	if (edit.is_panning)
	{
		Editor_ScrollMap(0, dx, dy, mod);
		return;
	}
	else if (edit.action == ACT_ADJUST_OFS)
	{
		AdjustOfs_Delta(dx, dy);
		return;
	}

	Objid old_hl(r_view.current_hl);

	// this also updates edit.map_x/y/z
	Render3D_Query(r_view.current_hl, x, y);

	if (edit.action == ACT_CLICK)
	{
		CheckBeginDrag();
	}
	else if (edit.action == ACT_DRAG)
	{
		edit.drag_screen_dx = x - edit.click_screen_x;
		edit.drag_screen_dy = y - edit.click_screen_y;

		edit.drag_cur_x = edit.map_x;
		edit.drag_cur_y = edit.map_y;

		if (edit.mode == OBJ_SECTORS)
			DragSectors_Update();

		if (edit.mode == OBJ_THINGS)
			DragThings_Update();

		main_win->canvas->redraw();
		main_win->status_bar->redraw();
		return;
	}

	if (! (r_view.current_hl == old_hl))
	{
		UpdateHighlight();
	}
}


void Render3D_UpdateHighlight()
{
	// TODO : REVIEW HOW/WHEN pointer_in_window is set

	edit.highlight.clear();
	edit.split_line.clear();

	if (r_view.current_hl.type == edit.mode &&
		edit.pointer_in_window &&
		edit.action != ACT_DRAG)
	{
		edit.highlight = r_view.current_hl;
	}

	main_win->canvas->redraw();
	main_win->status_bar->redraw();
}


void Render3D_Navigate()
{
	float delay_ms = Nav_TimeDiff();

	delay_ms = delay_ms / 1000.0;

	keycode_t mod = 0;

	if (edit.nav_lax)
		mod = M_ReadLaxModifiers();

	float mod_factor = 1.0;
	if (mod & MOD_SHIFT)   mod_factor = 0.5;
	if (mod & MOD_COMMAND) mod_factor = 2.0;

	if (edit.nav_fwd || edit.nav_back || edit.nav_right || edit.nav_left)
	{
		float fwd   = edit.nav_fwd   - edit.nav_back;
		float right = edit.nav_right - edit.nav_left;

		float dx = r_view.Cos * fwd + r_view.Sin * right;
		float dy = r_view.Sin * fwd - r_view.Cos * right;

		dx = dx * mod_factor * mod_factor;
		dy = dy * mod_factor * mod_factor;

		r_view.x += dx * delay_ms;
		r_view.y += dy * delay_ms;
	}

	if (edit.nav_up || edit.nav_down)
	{
		float dz = (edit.nav_up - edit.nav_down);

		r_view.z += dz * mod_factor * delay_ms;
	}

	if (edit.nav_turn_L || edit.nav_turn_R)
	{
		float dang = (edit.nav_turn_L - edit.nav_turn_R);

		dang = dang * mod_factor * delay_ms;
		dang = CLAMP(-90, dang, 90);

		r_view.SetAngle(r_view.angle + dang);
	}

	main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap();
}


// returns -1 if nothing in selection or highlight, -2 if multiple
// things are selected and they have different types.
static int GrabSelectedThing()
{
	int result = -1;

	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("no things for copy/cut type");
			return -1;
		}

		result = Things[edit.highlight.num]->type;
	}
	else
	{
		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			const Thing *T = Things[*it];
			if (result >= 0 && T->type != result)
			{
				Beep("multiple thing types");
				return -2;
			}

			result = T->type;
		}
	}

	Status_Set("copied type %d", result);

	return result;
}


static void StoreSelectedThing(int new_type)
{
	// this code is similar to code in UI_Thing::type_callback(),
	// but here we must handle a highlighted object.

	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("no things for paste type");
		return;
	}

	BA_Begin();
	BA_MessageForSel("pasted type of", edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		BA_ChangeTH(*it, Thing::F_TYPE, new_type);
	}

	BA_End();

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);

	Status_Set("pasted type %d", new_type);
}


static int SEC_GrabFlat(const Sector *S, int part)
{
	if (part & PART_CEIL)
		return S->ceil_tex;

	if (part & PART_FLOOR)
		return S->floor_tex;

	return S->floor_tex;
}


// returns -1 if nothing in selection or highlight, -2 if multiple
// sectors are selected and they have different flats.
static int GrabSelectedFlat()
{
	int result = -1;

	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("no sectors for copy/cut flat");
			return -1;
		}

		const Sector *S = Sectors[edit.highlight.num];

		result = SEC_GrabFlat(S, edit.highlight.parts);
	}
	else
	{
		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			const Sector *S = Sectors[*it];
			byte parts = edit.Selected->get_ext(*it);

			int tex = SEC_GrabFlat(S, parts & ~1);

			if (result >= 0 && tex != result)
			{
				Beep("multiple flats present");
				return -2;
			}

			result = tex;
		}
	}

	if (result >= 0)
		Status_Set("copied %s", BA_GetString(result));

	return result;
}


static void StoreSelectedFlat(int new_tex)
{
	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("no sectors for paste flat");
		return;
	}

	BA_Begin();
	BA_MessageForSel("pasted flat to", edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		byte parts = edit.Selected->get_ext(*it);

		if (parts == 1 || (parts & PART_FLOOR))
			BA_ChangeSEC(*it, Sector::F_FLOOR_TEX, new_tex);

		if (parts == 1 || (parts & PART_CEIL))
			BA_ChangeSEC(*it, Sector::F_CEIL_TEX, new_tex);
	}

	BA_End();

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);

	Status_Set("pasted %s", BA_GetString(new_tex));
}


static void StoreDefaultedFlats()
{
	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("no sectors for default");
		return;
	}

	int floor_tex = BA_InternaliseString(default_floor_tex);
	int ceil_tex  = BA_InternaliseString(default_ceil_tex);

	BA_Begin();
	BA_MessageForSel("defaulted flat in", edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		byte parts = edit.Selected->get_ext(*it);

		if (parts == 1 || (parts & PART_FLOOR))
			BA_ChangeSEC(*it, Sector::F_FLOOR_TEX, floor_tex);

		if (parts == 1 || (parts & PART_CEIL))
			BA_ChangeSEC(*it, Sector::F_CEIL_TEX, ceil_tex);
	}

	BA_End();

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);

	Status_Set("defaulted flats");
}


static int LD_GrabTex(const LineDef *L, int part)
{
	if (L->NoSided())
		return BA_InternaliseString(default_wall_tex);

	if (L->OneSided())
		return L->Right()->mid_tex;

	if (part & PART_RT_LOWER) return L->Right()->lower_tex;
	if (part & PART_RT_UPPER) return L->Right()->upper_tex;

	if (part & PART_LF_LOWER) return L->Left()->lower_tex;
	if (part & PART_LF_UPPER) return L->Left()->upper_tex;

	if (part & PART_RT_RAIL)  return L->Right()->mid_tex;
	if (part & PART_LF_RAIL)  return L->Left() ->mid_tex;

	// pick something reasonable for a simply selected line
	if (L->Left()->SecRef()->floorh > L->Right()->SecRef()->floorh)
		return L->Right()->lower_tex;

	if (L->Left()->SecRef()->ceilh < L->Right()->SecRef()->ceilh)
		return L->Right()->upper_tex;

	if (L->Left()->SecRef()->floorh < L->Right()->SecRef()->floorh)
		return L->Left()->lower_tex;

	if (L->Left()->SecRef()->ceilh > L->Right()->SecRef()->ceilh)
		return L->Left()->upper_tex;

	// emergency fallback
	return L->Right()->lower_tex;
}


// returns -1 if nothing in selection or highlight, -2 if multiple
// linedefs are selected and they have different textures.
static int GrabSelectedTexture()
{
	int result = -1;

	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("no linedefs for copy/cut tex");
			return -1;
		}

		const LineDef *L = LineDefs[edit.highlight.num];

		result = LD_GrabTex(L, edit.highlight.parts);
	}
	else
	{
		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			const LineDef *L = LineDefs[*it];
			byte parts = edit.Selected->get_ext(*it);

			int tex = LD_GrabTex(L, parts & ~1);

			if (result >= 0 && tex != result)
			{
				Beep("multiple textures present");
				return -2;
			}

			result = tex;
		}
	}

	if (result >= 0)
		Status_Set("copied %s", BA_GetString(result));

	return result;
}


static void StoreSelectedTexture(int new_tex)
{
	soh_type_e unselect = Selection_Or_Highlight();
	if (unselect == SOH_Empty)
	{
		Beep("no linedefs for paste tex");
		return;
	}

	BA_Begin();
	BA_MessageForSel("pasted tex to", edit.Selected);

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		const LineDef *L = LineDefs[*it];
		byte parts = edit.Selected->get_ext(*it);

		if (L->NoSided())
			continue;

		if (L->OneSided())
		{
			BA_ChangeSD(L->right, SideDef::F_MID_TEX, new_tex);
			continue;
		}

		/* right side */
		if (parts == 1 || (parts & PART_RT_LOWER))
			BA_ChangeSD(L->right, SideDef::F_LOWER_TEX, new_tex);

		if (parts == 1 || (parts & PART_RT_UPPER))
			BA_ChangeSD(L->right, SideDef::F_UPPER_TEX, new_tex);

		if (parts & PART_RT_RAIL)
			BA_ChangeSD(L->right, SideDef::F_MID_TEX, new_tex);

		/* left side */
		if (parts == 1 || (parts & PART_LF_LOWER))
			BA_ChangeSD(L->left, SideDef::F_LOWER_TEX, new_tex);

		if (parts == 1 || (parts & PART_LF_UPPER))
			BA_ChangeSD(L->left, SideDef::F_UPPER_TEX, new_tex);

		if (parts & PART_LF_RAIL)
			BA_ChangeSD(L->left, SideDef::F_MID_TEX, new_tex);
	}

	BA_End();

	if (unselect == SOH_Unselect)
		Selection_Clear(true /* nosave */);

	Status_Set("pasted %s", BA_GetString(new_tex));
}


void Render3D_CB_Copy()
{
	int num;

	switch (edit.mode)
	{
	case OBJ_THINGS:
		num = GrabSelectedThing();
		if (num >= 0)
			Texboard_SetThing(num);
		break;

	case OBJ_SECTORS:
		num = GrabSelectedFlat();
		if (num >= 0)
			Texboard_SetFlat(BA_GetString(num));
		break;

	case OBJ_LINEDEFS:
		num = GrabSelectedTexture();
		if (num >= 0)
			Texboard_SetTex(BA_GetString(num));
		break;

	default:
		break;
	}
}


void Render3D_CB_Paste()
{
	switch (edit.mode)
	{
	case OBJ_THINGS:
		StoreSelectedThing(Texboard_GetThing());
		break;

	case OBJ_SECTORS:
		StoreSelectedFlat(Texboard_GetFlatNum());
		break;

	case OBJ_LINEDEFS:
		StoreSelectedTexture(Texboard_GetTexNum());
		break;

	default:
		break;
	}
}


void Render3D_CB_Cut()
{
	// this is repurposed to set the default texture/thing

	switch (edit.mode)
	{
	case OBJ_THINGS:
		StoreSelectedThing(default_thing);
		break;

	case OBJ_SECTORS:
		StoreDefaultedFlats();
		break;

	case OBJ_LINEDEFS:
		StoreSelectedTexture(BA_InternaliseString(default_wall_tex));
		break;

	default:
		break;
	}
}


void Render3D_SetCameraPos(double new_x, double new_y)
{
	r_view.x = new_x;
	r_view.y = new_y;

	r_view.FindGroundZ();
}


void Render3D_GetCameraPos(double *x, double *y, float *angle)
{
	*x = r_view.x;
	*y = r_view.y;

	// convert angle from radians to degrees
	*angle = r_view.angle * 180.0 / M_PI;
}


bool Render3D_ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "camera") == 0 && num_tok >= 5)
	{
		r_view.x = atof(tokens[1]);
		r_view.y = atof(tokens[2]);
		r_view.z = atof(tokens[3]);

		r_view.SetAngle(atof(tokens[4]));
		return true;
	}

	if (strcmp(tokens[0], "r_modes") == 0 && num_tok >= 4)
	{
		r_view.texturing = atoi(tokens[1]) ? true : false;
		r_view.sprites   = atoi(tokens[2]) ? true : false;
		r_view.lighting  = atoi(tokens[3]) ? true : false;

		return true;
	}

	if (strcmp(tokens[0], "r_gravity") == 0 && num_tok >= 2)
	{
		r_view.gravity = atoi(tokens[1]) ? true : false;
		return true;
	}

	if (strcmp(tokens[0], "low_detail") == 0 && num_tok >= 2)
	{
		// ignored for compatibility
		return true;
	}

	if (strcmp(tokens[0], "gamma") == 0 && num_tok >= 2)
	{
		usegamma = MAX(0, atoi(tokens[1])) % 5;

		W_UpdateGamma();
		return true;
	}

	return false;
}


void Render3D_WriteUser(FILE *fp)
{
	fprintf(fp, "camera %1.2f %1.2f %1.2f %1.2f\n",
	        r_view.x, r_view.y, r_view.z, r_view.angle);

	fprintf(fp, "r_modes %d %d %d\n",
	        r_view.texturing  ? 1 : 0,
			r_view.sprites    ? 1 : 0,
			r_view.lighting   ? 1 : 0);

	fprintf(fp, "r_gravity %d\n",
	        r_view.gravity ? 1 : 0);

	fprintf(fp, "gamma %d\n",
	        usegamma);
}


//------------------------------------------------------------------------
//  COMMAND FUNCTIONS
//------------------------------------------------------------------------

void R3D_Forward()
{
	float dist = atof(EXEC_Param[0]);

	r_view.x += r_view.Cos * dist;
	r_view.y += r_view.Sin * dist;

	main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap();
}

void R3D_Backward()
{
	float dist = atof(EXEC_Param[0]);

	r_view.x -= r_view.Cos * dist;
	r_view.y -= r_view.Sin * dist;

	main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap();
}

void R3D_Left()
{
	float dist = atof(EXEC_Param[0]);

	r_view.x -= r_view.Sin * dist;
	r_view.y += r_view.Cos * dist;

	main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap();
}

void R3D_Right()
{
	float dist = atof(EXEC_Param[0]);

	r_view.x += r_view.Sin * dist;
	r_view.y -= r_view.Cos * dist;

	main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap();
}

void R3D_Up()
{
	if (r_view.gravity && render_lock_gravity)
	{
		Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	float dist = atof(EXEC_Param[0]);

	r_view.z += dist;

	RedrawMap();
}

void R3D_Down()
{
	if (r_view.gravity && render_lock_gravity)
	{
		Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	float dist = atof(EXEC_Param[0]);

	r_view.z -= dist;

	RedrawMap();
}


void R3D_Turn()
{
	float angle = atof(EXEC_Param[0]);

	// convert to radians
	angle = angle * M_PI / 180.0;

	r_view.SetAngle(r_view.angle + angle);

	RedrawMap();
}


void R3D_DropToFloor()
{
	r_view.FindGroundZ();

	RedrawMap();
}


static void R3D_NAV_Forward_release()
{
	edit.nav_fwd = 0;
}

void R3D_NAV_Forward()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	edit.nav_fwd = atof(EXEC_Param[0]);

	Nav_SetKey(EXEC_CurKey, &R3D_NAV_Forward_release);
}


static void R3D_NAV_Back_release(void)
{
	edit.nav_back = 0;
}

void R3D_NAV_Back()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	edit.nav_back = atof(EXEC_Param[0]);

	Nav_SetKey(EXEC_CurKey, &R3D_NAV_Back_release);
}


static void R3D_NAV_Right_release(void)
{
	edit.nav_right = 0;
}

void R3D_NAV_Right()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	edit.nav_right = atof(EXEC_Param[0]);

	Nav_SetKey(EXEC_CurKey, &R3D_NAV_Right_release);
}


static void R3D_NAV_Left_release(void)
{
	edit.nav_left = 0;
}

void R3D_NAV_Left()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	edit.nav_left = atof(EXEC_Param[0]);

	Nav_SetKey(EXEC_CurKey, &R3D_NAV_Left_release);
}


static void R3D_NAV_Up_release(void)
{
	edit.nav_up = 0;
}

void R3D_NAV_Up()
{
	if (! EXEC_CurKey)
		return;

	if (r_view.gravity && render_lock_gravity)
	{
		Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	if (! edit.is_navigating)
		Editor_ClearNav();

	edit.nav_up = atof(EXEC_Param[0]);

	Nav_SetKey(EXEC_CurKey, &R3D_NAV_Up_release);
}


static void R3D_NAV_Down_release(void)
{
	edit.nav_down = 0;
}

void R3D_NAV_Down()
{
	if (! EXEC_CurKey)
		return;

	if (r_view.gravity && render_lock_gravity)
	{
		Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	if (! edit.is_navigating)
		Editor_ClearNav();

	edit.nav_down = atof(EXEC_Param[0]);

	Nav_SetKey(EXEC_CurKey, &R3D_NAV_Down_release);
}


static void R3D_NAV_TurnLeft_release(void)
{
	edit.nav_turn_L = 0;
}

void R3D_NAV_TurnLeft()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float turn = atof(EXEC_Param[0]);

	// convert to radians
	edit.nav_turn_L = turn * M_PI / 180.0;

	Nav_SetKey(EXEC_CurKey, &R3D_NAV_TurnLeft_release);
}


static void R3D_NAV_TurnRight_release(void)
{
	edit.nav_turn_R = 0;
}

void R3D_NAV_TurnRight()
{
	if (! EXEC_CurKey)
		return;

	if (! edit.is_navigating)
		Editor_ClearNav();

	float turn = atof(EXEC_Param[0]);

	// convert to radians
	edit.nav_turn_R = turn * M_PI / 180.0;

	Nav_SetKey(EXEC_CurKey, &R3D_NAV_TurnRight_release);
}


static void ACT_AdjustOfs_release(void)
{
	// check if cancelled or overridden
	if (edit.action != ACT_ADJUST_OFS)
		return;

	AdjustOfs_Finish();
}

void R3D_ACT_AdjustOfs()
{
	if (! EXEC_CurKey)
		return;

	if (! Nav_ActionKey(EXEC_CurKey, &ACT_AdjustOfs_release))
		return;

	if (edit.mode != OBJ_LINEDEFS)
	{
		Beep("not in linedef mode");
		return;
	}

	AdjustOfs_Begin();
}


void R3D_Set()
{
	const char *var_name = EXEC_Param[0];
	const char *value    = EXEC_Param[1];

	if (! var_name[0])
	{
		Beep("3D_Set: missing var name");
		return;
	}

	if (! value[0])
	{
		Beep("3D_Set: missing value");
		return;
	}

	 int  int_val = atoi(value);
	bool bool_val = (int_val > 0);


	if (y_stricmp(var_name, "tex") == 0)
	{
		r_view.texturing = bool_val;
	}
	else if (y_stricmp(var_name, "obj") == 0)
	{
		r_view.sprites = bool_val;
	}
	else if (y_stricmp(var_name, "light") == 0)
	{
		r_view.lighting = bool_val;
	}
	else if (y_stricmp(var_name, "grav") == 0)
	{
		r_view.gravity = bool_val;
	}
	else
	{
		Beep("3D_Set: unknown var: %s", var_name);
		return;
	}

	RedrawMap();
}


void R3D_Toggle()
{
	const char *var_name = EXEC_Param[0];

	if (! var_name[0])
	{
		Beep("3D_Toggle: missing var name");
		return;
	}

	if (y_stricmp(var_name, "tex") == 0)
	{
		r_view.texturing = ! r_view.texturing;
	}
	else if (y_stricmp(var_name, "obj") == 0)
	{
		r_view.sprites = ! r_view.sprites;
	}
	else if (y_stricmp(var_name, "light") == 0)
	{
		r_view.lighting = ! r_view.lighting;
	}
	else if (y_stricmp(var_name, "grav") == 0)
	{
		r_view.gravity = ! r_view.gravity;
	}
	else
	{
		Beep("3D_Toggle: unknown var: %s", var_name);
		return;
	}

	RedrawMap();
}


void R3D_WHEEL_Move()
{
	float dx = Fl::event_dx();
	float dy = Fl::event_dy();

	dy = 0 - dy;

	float speed = atof(EXEC_Param[0]);

	if (Exec_HasFlag("/LAX"))
	{
		keycode_t mod = Fl::event_state() & MOD_ALL_MASK;

		if (mod == MOD_SHIFT)
			speed /= 4.0;
		else if (mod == MOD_COMMAND)
			speed *= 4.0;
	}

	r_view.x += speed * (r_view.Cos * dy + r_view.Sin * dx);
	r_view.y += speed * (r_view.Sin * dy - r_view.Cos * dx);

	main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap();
}


//------------------------------------------------------------------------

extern void CMD_ACT_Click();


static editor_command_t  render_commands[] =
{
	{	"3D_Set", NULL,
		&R3D_Set,
		/* flags */ NULL,
		/* keywords */ "tex obj light grav"
	},

	{	"3D_Toggle", NULL,
		&R3D_Toggle,
		/* flags */ NULL,
		/* keywords */ "tex obj light grav"
	},

	{	"3D_Forward", NULL,
		&R3D_Forward
	},

	{	"3D_Backward", NULL,
		&R3D_Backward
	},

	{	"3D_Left", NULL,
		&R3D_Left
	},

	{	"3D_Right", NULL,
		&R3D_Right
	},

	{	"3D_Up", NULL,
		&R3D_Up
	},

	{	"3D_Down", NULL,
		&R3D_Down
	},

	{	"3D_Turn", NULL,
		&R3D_Turn
	},

	{	"3D_DropToFloor", NULL,
		&R3D_DropToFloor
	},

	{	"3D_ACT_AdjustOfs", NULL,
		&R3D_ACT_AdjustOfs
	},

	{	"3D_WHEEL_Move", NULL,
		&R3D_WHEEL_Move
	},

	{	"3D_NAV_Forward", NULL,
		&R3D_NAV_Forward
	},

	{	"3D_NAV_Back", NULL,
		&R3D_NAV_Back
	},

	{	"3D_NAV_Right", NULL,
		&R3D_NAV_Right
	},

	{	"3D_NAV_Left", NULL,
		&R3D_NAV_Left
	},

	{	"3D_NAV_Up", NULL,
		&R3D_NAV_Up
	},

	{	"3D_NAV_Down", NULL,
		&R3D_NAV_Down
	},

	{	"3D_NAV_TurnLeft", NULL,
		&R3D_NAV_TurnLeft
	},

	{	"3D_NAV_TurnRight", NULL,
		&R3D_NAV_TurnRight
	},

	// backwards compatibility.
	// [ we cannot remap this in FindEditorCommand, it fails the context check ]
	{	"3D_Align", NULL,
		&CMD_LIN_Align,
		/* flags */ "/x /y /right /clear"
	},

	// end of command list
	{	NULL, NULL, 0, NULL  }
};


void Render3D_RegisterCommands()
{
	M_RegisterCommandList(render_commands);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
