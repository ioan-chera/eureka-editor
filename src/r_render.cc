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

#include "Errors.h"
#include "Instance.h"

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
#include "m_config.h"
#include "m_game.h"
#include "m_events.h"
#include "r_render.h"
#include "r_subdiv.h"
#include "ui_window.h"


// config items
rgb_color_t config::transparent_col = RGB_MAKE(0, 255, 255);

bool config::render_high_detail    = false;
bool config::render_lock_gravity   = false;
bool config::render_missing_bright = true;
bool config::render_unknown_bright = true;

int  config::render_far_clip = 32768;

// in original DOOM pixels were 20% taller than wide, giving 0.83
// as the pixel aspect ratio.
int  config::render_pixel_aspect = 83;  //  100 * width / height

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

	void InvalidateAll(const Document &doc)
	{
		invalid_low  = 0;
		invalid_high = doc.numThings() - 1;
	}

	void Update(Instance &inst);
};


// TODO: make it per instance
Render_View_t r_view(gInstance);


Render_View_t::Render_View_t(Instance &inst) :
	p_type(0), px(), py(),
	x(), y(), z(),
	angle(), Sin(), Cos(),
	screen_w(), screen_h(), screen(NULL),
	texturing(false), sprites(false), lighting(false),
	gravity(true),
	thing_sectors(),
	mouse_x(-1), mouse_y(-1), inst(inst)
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

	double max_floor = -9e9;
	bool hit_something = false;

	for (int dx = -2 ; dx <= 2 ; dx++)
	for (int dy = -2 ; dy <= 2 ; dy++)
	{
		double test_x = x + dx * 8;
		double test_y = y + dy * 8;

		Objid o = inst.level.hover.getNearbyObject(ObjType::sectors, test_x, test_y);

		if (o.num >= 0)
		{
			double z = inst.level.sectors[o.num]->floorh;
			{
				sector_3dfloors_c *ex = Subdiv_3DFloorsForSector(o.num);
				if (ex->f_plane.sloped)
					z = ex->f_plane.SlopeZ(test_x, test_y);
			}

			max_floor = MAX(max_floor, z);
			hit_something = true;
		}
	}

	if (hit_something)
		z = max_floor + Misc_info.view_height;
}

void Render_View_t::CalcAspect()
{
	aspect_sw = static_cast<float>(screen_w);	 // things break if these are different

	aspect_sh = screen_w / (config::render_pixel_aspect / 100.0f);
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

	int new_sw = config::render_high_detail ? ow : (ow + 1) / 2;
	int new_sh = config::render_high_detail ? oh : (oh + 1) / 2;

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
	thing_sec_cache::Update(inst);

	UpdateScreen(ow, oh);

	if (gravity)
		FindGroundZ();
}


static Thing *FindPlayer(const Document &doc, int typenum)
{
	// need to search backwards (to handle Voodoo dolls properly)

	for ( int i = doc.numThings()-1 ; i >= 0 ; i--)
		if (doc.things[i]->type == typenum)
			return doc.things[i];

	return NULL;  // not found
}


//------------------------------------------------------------------------

namespace thing_sec_cache
{
	void Update(Instance &inst)
	{
		// guarantee that thing_sectors has the correct size.
		// [ prevent a potential crash ]
		if (inst.level.numThings() != (int)r_view.thing_sectors.size())
		{
			r_view.thing_sectors.resize(inst.level.numThings());
			thing_sec_cache::InvalidateAll(inst.level);
		}

		// nothing changed?
		if (invalid_low > invalid_high)
			return;

		for (int i = invalid_low ; i <= invalid_high ; i++)
		{
			Objid obj = inst.level.hover.getNearbyObject(ObjType::sectors, inst.level.things[i]->x(), inst.level.things[i]->y());

			r_view.thing_sectors[i] = obj.num;
		}

		thing_sec_cache::ResetRange();
	}
}

void Render3D_NotifyBegin()
{
	thing_sec_cache::ResetRange();
}

void Render3D_NotifyInsert(ObjType type, int objnum)
{
	if (type == ObjType::things)
		thing_sec_cache::InvalidateThing(objnum);
}

void Render3D_NotifyDelete(const Document &doc, ObjType type, int objnum)
{
	if (type == ObjType::things || type == ObjType::sectors)
		thing_sec_cache::InvalidateAll(doc);
}

void Render3D_NotifyChange(ObjType type, int objnum, int field)
{
	if (type == ObjType::things &&
		(field == Thing::F_X || field == Thing::F_Y))
	{
		thing_sec_cache::InvalidateThing(objnum);
	}
}

void Render3D_NotifyEnd(Instance &inst)
{
	thing_sec_cache::Update(inst);
}


//------------------------------------------------------------------------

class save_obj_field_c
{
public:
	int obj;    // object number (SaveBucket::type is the type)
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
	ObjType type;

	std::vector< save_obj_field_c > fields;
	Instance &inst;

public:
	SaveBucket_c(ObjType type, Instance &inst) : type(type), inst(inst)
	{
	}

	~SaveBucket_c()
	{ }

	void Clear()
	{
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
				inst.level.basis.change(type, fields[i].obj, (byte)field, fields[i].value + delta);
			}
		}
	}

private:
	int * RawObjPointer(int objnum)
	{
		switch (type)
		{
			case ObjType::things:
				return reinterpret_cast<int*>(inst.level.things[objnum]);

			case ObjType::vertices:
				return reinterpret_cast<int *>(inst.level.vertices[objnum]);

			case ObjType::sectors:
				return reinterpret_cast<int *>(inst.level.sectors[objnum]);

			case ObjType::sidedefs:
				return reinterpret_cast<int *>(inst.level.sidedefs[objnum]);

			case ObjType::linedefs:
				return reinterpret_cast<int *>(inst.level.linedefs[objnum]);

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


static void AdjustOfs_UpdateBBox(Instance &inst, int ld_num)
{
	const LineDef *L = inst.level.linedefs[ld_num];

	float lx1 = static_cast<float>(L->Start(inst.level)->x());
	float ly1 = static_cast<float>(L->Start(inst.level)->y());
	float lx2 = static_cast<float>(L->End(inst.level)->x());
	float ly2 = static_cast<float>(L->End(inst.level)->y());

	if (lx1 > lx2) std::swap(lx1, lx2);
	if (ly1 > ly2) std::swap(ly1, ly2);

	inst.edit.adjust_bbox.x1 = MIN(inst.edit.adjust_bbox.x1, lx1);
	inst.edit.adjust_bbox.y1 = MIN(inst.edit.adjust_bbox.y1, ly1);
	inst.edit.adjust_bbox.x2 = MAX(inst.edit.adjust_bbox.x2, lx2);
	inst.edit.adjust_bbox.y2 = MAX(inst.edit.adjust_bbox.y2, ly2);
}

static void AdjustOfs_CalcDistFactor(const Instance &inst, float& dx_factor, float& dy_factor)
{
	// this computes how far to move the offsets for each screen pixel
	// the mouse moves.  we want it to appear as though each texture
	// is being dragged by the mouse, e.g. if you click on the middle
	// of a switch, that switch follows the mouse pointer around.
	// such an effect can only be approximate though.

	float dx = static_cast<float>((r_view.x < inst.edit.adjust_bbox.x1) ? (inst.edit.adjust_bbox.x1 - r_view.x) :
			   (r_view.x > inst.edit.adjust_bbox.x2) ? (r_view.x - inst.edit.adjust_bbox.x2) : 0);

	float dy = static_cast<float>((r_view.y < inst.edit.adjust_bbox.y1) ? (inst.edit.adjust_bbox.y1 - r_view.y) :
			   (r_view.y > inst.edit.adjust_bbox.y2) ? (r_view.y - inst.edit.adjust_bbox.y2) : 0);

	float dist = hypot(dx, dy);

	dist = CLAMP(20, dist, 1000);

	dx_factor = dist / r_view.aspect_sw;
	dy_factor = dist / r_view.aspect_sh;
}

static void AdjustOfs_Add(Instance &inst, int ld_num, int part)
{
	if (! inst.edit.adjust_bucket)
		return;

	const LineDef *L = inst.level.linedefs[ld_num];

	// ignore invalid sides (sanity check)
	int sd_num = (part & PART_LF_ALL) ? L->left : L->right;
	if (sd_num < 0)
		return;

	// TODO : UDMF ports can allow full control over each part

	inst.edit.adjust_bucket->Save(sd_num, SideDef::F_X_OFFSET);
	inst.edit.adjust_bucket->Save(sd_num, SideDef::F_Y_OFFSET);
}

static void AdjustOfs_Begin(Instance &inst)
{
	if (inst.edit.adjust_bucket)
		delete inst.edit.adjust_bucket;

	inst.edit.adjust_bucket = new SaveBucket_c(ObjType::sidedefs, inst);
	inst.edit.adjust_lax = Exec_HasFlag("/LAX");

	int total_lines = 0;

	// we will compute the bbox of selected lines
	inst.edit.adjust_bbox.x1 = inst.edit.adjust_bbox.y1 = static_cast<float>(+9e9);
	inst.edit.adjust_bbox.x2 = inst.edit.adjust_bbox.y2 = static_cast<float>(-9e9);

	// find the sidedefs to adjust
	if (! inst.edit.Selected->empty())
	{
		for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
		{
			int ld_num = *it;
			byte parts = inst.edit.Selected->get_ext(ld_num);

			// handle "simply selected" linedefs
			if (parts <= 1)
			{
				parts |= PART_RT_LOWER | PART_RT_UPPER;
				parts |= PART_LF_LOWER | PART_LF_UPPER;
			}

			total_lines++;
			AdjustOfs_UpdateBBox(inst, ld_num);

			if (parts & PART_RT_LOWER) AdjustOfs_Add(inst, ld_num, PART_RT_LOWER);
			if (parts & PART_RT_UPPER) AdjustOfs_Add(inst, ld_num, PART_RT_UPPER);
			if (parts & PART_RT_RAIL)  AdjustOfs_Add(inst, ld_num, PART_RT_RAIL);

			if (parts & PART_LF_LOWER) AdjustOfs_Add(inst, ld_num, PART_LF_LOWER);
			if (parts & PART_LF_UPPER) AdjustOfs_Add(inst, ld_num, PART_LF_UPPER);
			if (parts & PART_LF_RAIL)  AdjustOfs_Add(inst, ld_num, PART_LF_RAIL);
		}
	}
	else if (inst.edit.highlight.valid())
	{
		int  ld_num = inst.edit.highlight.num;
		byte parts  = inst.edit.highlight.parts;

		if (parts >= 2)
		{
			AdjustOfs_Add(inst, ld_num, parts);
			AdjustOfs_UpdateBBox(inst, ld_num);
			total_lines++;
		}
	}

	if (total_lines == 0)
	{
		inst.Beep("nothing to adjust");
		return;
	}

	inst.edit.adjust_dx = 0;
	inst.edit.adjust_dy = 0;

	inst.edit.adjust_lax = Exec_HasFlag("/LAX");

	Editor_SetAction(inst, ACT_ADJUST_OFS);
}

static void AdjustOfs_Finish(Instance &inst)
{
	if (! inst.edit.adjust_bucket)
	{
		Editor_ClearAction(inst);
		return;
	}

	int dx = I_ROUND(inst.edit.adjust_dx);
	int dy = I_ROUND(inst.edit.adjust_dy);

	if (dx || dy)
	{
		inst.level.basis.begin();
		inst.level.basis.setMessage("adjusted offsets");

		inst.edit.adjust_bucket->ApplyToBasis(SideDef::F_X_OFFSET, dx);
		inst.edit.adjust_bucket->ApplyToBasis(SideDef::F_Y_OFFSET, dy);

		inst.level.basis.end();
	}

	delete inst.edit.adjust_bucket;
	inst.edit.adjust_bucket = NULL;

	Editor_ClearAction(inst);
}

static void AdjustOfs_Delta(Instance &inst, int dx, int dy)
{
	if (! inst.edit.adjust_bucket)
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

	keycode_t mod = inst.edit.adjust_lax ? M_ReadLaxModifiers() : 0;

	float factor = (mod & EMOD_SHIFT) ? 0.5f : 2.0f;

	if (!config::render_high_detail)
		factor = factor * 0.5f;

	float dx_factor, dy_factor;
	AdjustOfs_CalcDistFactor(inst, dx_factor, dy_factor);

	inst.edit.adjust_dx -= dx * factor * dx_factor;
	inst.edit.adjust_dy -= dy * factor * dy_factor;

	RedrawMap(inst);
}


static void AdjustOfs_RenderAnte(const Instance &inst)
{
	if (inst.edit.action == ACT_ADJUST_OFS && inst.edit.adjust_bucket)
	{
		int dx = I_ROUND(inst.edit.adjust_dx);
		int dy = I_ROUND(inst.edit.adjust_dy);

		// change it temporarily (just for the render)
		inst.edit.adjust_bucket->ApplyTemp(SideDef::F_X_OFFSET, dx);
		inst.edit.adjust_bucket->ApplyTemp(SideDef::F_Y_OFFSET, dy);
	}
}

static void AdjustOfs_RenderPost(const Instance &inst)
{
	if (inst.edit.action == ACT_ADJUST_OFS && inst.edit.adjust_bucket)
	{
		inst.edit.adjust_bucket->RestoreAll();
	}
}


//------------------------------------------------------------------------

static Thing *player;

extern void CheckBeginDrag(Instance &inst);


void Render3D_Draw(Instance &inst, int ox, int oy, int ow, int oh)
{
	r_view.PrepareToRender(ow, oh);

	AdjustOfs_RenderAnte(inst);

#ifdef NO_OPENGL
	SW_RenderWorld(inst, ox, oy, ow, oh);
#else
	RGL_RenderWorld(inst, ox, oy, ow, oh);
#endif

	AdjustOfs_RenderPost(inst);
}


bool Render3D_Query(Instance &inst, Objid& hl, int sx, int sy)
{
	int ow = inst.main_win->canvas->w();
	int oh = inst.main_win->canvas->h();

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
	config::render_high_detail = true;
#endif

	hl.clear();

	if (! inst.edit.pointer_in_window)
		return false;

	r_view.PrepareToRender(ow, oh);

	return SW_QueryPoint(inst, hl, sx, sy);
}


void Render3D_Setup(Instance &inst)
{
	thing_sec_cache::InvalidateAll(inst.level);
	r_view.thing_sectors.resize(0);

	if (! r_view.p_type)
	{
		r_view.p_type = THING_PLAYER1;
		r_view.px = 99999;
	}

	player = FindPlayer(inst.level, r_view.p_type);

	if (! player)
	{
		if (r_view.p_type != THING_DEATHMATCH)
			r_view.p_type = THING_DEATHMATCH;

		player = FindPlayer(inst.level, r_view.p_type);
	}

	if (player && !(r_view.px == player->x() && r_view.py == player->y()))
	{
		// if player moved, re-create view parameters

		r_view.x = r_view.px = player->x();
		r_view.y = r_view.py = player->y();

		r_view.FindGroundZ();

		r_view.SetAngle(static_cast<float>(player->angle * M_PI / 180.0));
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


void Render3D_Enable(Instance &inst, bool _enable)
{
	Editor_ClearAction(inst);

	inst.edit.render3d = _enable;

	inst.edit.highlight.clear();
	inst.edit.clicked.clear();
	inst.edit.dragged.clear();

	// give keyboard focus to the appropriate large widget
	Fl::focus(inst.main_win->canvas);

	inst.main_win->scroll->UpdateRenderMode();
	inst.main_win->info_bar->UpdateSecRend();

	if (inst.edit.render3d)
	{
		inst.main_win->info_bar->SetMouse(r_view.x, r_view.y);

		// TODO: ideally query this, like code in PointerPos
		r_view.mouse_x = r_view.mouse_y = -1;
	}
	else
	{
		inst.main_win->canvas->PointerPos();
		inst.main_win->info_bar->SetMouse(inst.edit.map_x, inst.edit.map_y);
	}

	RedrawMap(inst);
}


void Render3D_ScrollMap(Instance &inst, int dx, int dy, keycode_t mod)
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

	bool is_strafe = (mod & EMOD_ALT) ? true : false;

	float mod_factor = 1.0;
	if (mod & EMOD_SHIFT)   mod_factor = 0.4f;
	if (mod & EMOD_COMMAND) mod_factor = 2.5f;

	float speed = inst.edit.panning_speed * mod_factor;

	if (is_strafe)
	{
		r_view.x += r_view.Sin * dx * mod_factor;
		r_view.y -= r_view.Cos * dx * mod_factor;
	}
	else  // turn camera
	{
		double d_ang = dx * speed * M_PI / 480.0;

		r_view.SetAngle(static_cast<float>(r_view.angle - d_ang));
	}

	dy = -dy;  //TODO CONFIG ITEM

	if (is_strafe)
	{
		r_view.x += r_view.Cos * dy * mod_factor;
		r_view.y += r_view.Sin * dy * mod_factor;
	}
	else if (! (config::render_lock_gravity && r_view.gravity))
	{
		r_view.z += dy * speed * 0.75;

		r_view.gravity = false;
	}

	inst.main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap(inst);
}


static void DragSectors_Update(Instance &inst)
{
	float ow = static_cast<float>(inst.main_win->canvas->w());
	float x_slope = 100.0f / config::render_pixel_aspect;

	float factor = static_cast<float>(CLAMP(20, inst.edit.drag_point_dist, 1000) / (ow * x_slope * 0.5));
	float map_dz = -inst.edit.drag_screen_dy * factor;

	float step = 8.0;  // TODO config item

	if (map_dz > step*0.25)
		inst.edit.drag_sector_dz = step * (int)ceil(map_dz / step);
	else if (map_dz < step*-0.25)
		inst.edit.drag_sector_dz = step * (int)floor(map_dz / step);
	else
		inst.edit.drag_sector_dz = 0;
}

void Render3D_DragSectors(Instance &inst)
{
	int dz = I_ROUND(inst.edit.drag_sector_dz);
	if (dz == 0)
		return;

	inst.level.basis.begin();

	if (dz > 0)
		inst.level.basis.setMessage("raised sectors");
	else
		inst.level.basis.setMessage("lowered sectors");

	if (inst.edit.dragged.valid())
	{
		int parts = inst.edit.dragged.parts;

		inst.level.secmod.safeRaiseLower(inst.edit.dragged.num, parts, dz);
	}
	else
	{
		for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
		{
			int parts = inst.edit.Selected->get_ext(*it);
			parts &= ~1;

			inst.level.secmod.safeRaiseLower(*it, parts, dz);
		}
	}

	inst.level.basis.end();
}


static void DragThings_Update(Instance &inst)
{
	float ow = static_cast<float>(inst.main_win->canvas->w());
//	float oh = main_win->canvas->h();

	float x_slope = 100.0f / config::render_pixel_aspect;
//	float y_slope = (float)oh / (float)ow;

	float dist = CLAMP(20, inst.edit.drag_point_dist, 1000);

	float x_factor = dist / (ow * 0.5f);
	float y_factor = dist / (ow * x_slope * 0.5f);

	if (inst.edit.drag_thing_up_down)
	{
		// vertical positioning in Hexen and UDMF formats
		float map_dz = -inst.edit.drag_screen_dy * y_factor;

		// final result is in drag_cur_x/y/z
		inst.edit.drag_cur_x = inst.edit.drag_start_x;
		inst.edit.drag_cur_y = inst.edit.drag_start_y;
		inst.edit.drag_cur_z = inst.edit.drag_start_z + map_dz;
		return;
	}

	/* move thing around XY plane */

	inst.edit.drag_cur_z = inst.edit.drag_start_z;

	// vectors for view camera
	double fwd_vx = r_view.Cos;
	double fwd_vy = r_view.Sin;

	double side_vx =  fwd_vy;
	double side_vy = -fwd_vx;

	double dx =  inst.edit.drag_screen_dx * x_factor;
	double dy = -inst.edit.drag_screen_dy * y_factor * 2.0;

	// this usually won't happen, but is a reasonable fallback...
	if (inst.edit.drag_thing_num < 0)
	{
		inst.edit.drag_cur_x = inst.edit.drag_start_x + dx * side_vx + dy * fwd_vx;
		inst.edit.drag_cur_y = inst.edit.drag_start_y + dx * side_vy + dy * fwd_vy;
		return;
	}

	// old code for depth calculation, works well in certain cases
	// but very poorly in other cases.
#if 0
	int sy1 = inst.edit.click_screen_y;
	int sy2 = sy1 + inst.edit.drag_screen_dy;

	if (sy1 >= oh/2 && sy2 >= oh/2)
	{
		double d1 = (inst.edit.drag_thing_floorh - r_view.z) / (oh - sy1*2.0);
		double d2 = (inst.edit.drag_thing_floorh - r_view.z) / (oh - sy2*2.0);

		d1 = d1 * ow;
		d2 = d2 * ow;

		dy = (d2 - d1) * 0.5;
	}
#endif

	const Thing *T = inst.level.things[inst.edit.drag_thing_num];

	float old_x = static_cast<float>(T->x());
	float old_y = static_cast<float>(T->y());

	float new_x = static_cast<float>(old_x + dx * side_vx);
	float new_y = static_cast<float>(old_y + dx * side_vy);

	// recompute forward/back vector
	fwd_vx = new_x - r_view.x;
	fwd_vy = new_y - r_view.y;

	double fwd_len = hypot(fwd_vx, fwd_vy);
	if (fwd_len < 1)
		fwd_len = 1;

	new_x = static_cast<float>(new_x + dy * fwd_vx / fwd_len);
	new_y = static_cast<float>(new_y + dy * fwd_vy / fwd_len);

	// handle a change in floor height
	Objid old_sec = inst.level.hover.getNearbyObject(ObjType::sectors, old_x, old_y);

	Objid new_sec = inst.level.hover.getNearbyObject(ObjType::sectors, new_x, new_y);

	if (old_sec.valid() && new_sec.valid())
	{
		float old_z = static_cast<float>(inst.level.sectors[old_sec.num]->floorh);
		float new_z = static_cast<float>(inst.level.sectors[new_sec.num]->floorh);

		// intent here is to show proper position, NOT raise/lower things.
		// [ perhaps add a new variable? ]
		inst.edit.drag_cur_z += (new_z - old_z);
	}

	inst.edit.drag_cur_x = inst.edit.drag_start_x + new_x - old_x;
	inst.edit.drag_cur_y = inst.edit.drag_start_y + new_y - old_y;
}

void Render3D_DragThings(Instance &inst)
{
	double dx = inst.edit.drag_cur_x - inst.edit.drag_start_x;
	double dy = inst.edit.drag_cur_y - inst.edit.drag_start_y;
	double dz = inst.edit.drag_cur_z - inst.edit.drag_start_z;

	// for movement in XY plane, ensure we don't raise/lower things
	if (! inst.edit.drag_thing_up_down)
		dz = 0.0;

	if (inst.edit.dragged.valid())
	{
		selection_c sel(ObjType::things);
		sel.set(inst.edit.dragged.num);

		inst.level.objects.move(&sel, dx, dy, dz);
	}
	else
	{
		inst.level.objects.move(inst.edit.Selected, dx, dy, dz);
	}

	RedrawMap(inst);
}


void Render3D_MouseMotion(Instance &inst, int x, int y, keycode_t mod, int dx, int dy)
{
	inst.edit.pointer_in_window = true;

	// save position for Render3D_UpdateHighlight
	r_view.mouse_x = x;
	r_view.mouse_y = y;

	if (inst.edit.is_panning)
	{
		Editor_ScrollMap(inst, 0, dx, dy, mod);
		return;
	}
	else if (inst.edit.action == ACT_ADJUST_OFS)
	{
		AdjustOfs_Delta(inst, dx, dy);
		return;
	}

	if (inst.edit.action == ACT_CLICK)
	{
		CheckBeginDrag(inst);
	}
	else if (inst.edit.action == ACT_DRAG)
	{
		// get the latest map_x/y/z coordinates
		Objid unused_hl;
		Render3D_Query(inst, unused_hl, x, y);

		inst.edit.drag_screen_dx = x - inst.edit.click_screen_x;
		inst.edit.drag_screen_dy = y - inst.edit.click_screen_y;

		inst.edit.drag_cur_x = inst.edit.map_x;
		inst.edit.drag_cur_y = inst.edit.map_y;

		if (inst.edit.mode == ObjType::sectors)
			DragSectors_Update(inst);

		if (inst.edit.mode == ObjType::things)
			DragThings_Update(inst);

		inst.main_win->canvas->redraw();
		inst.main_win->status_bar->redraw();
		return;
	}

	UpdateHighlight(inst);
}


void Render3D_UpdateHighlight(Instance &inst)
{
	inst.edit.highlight.clear();
	inst.edit.split_line.clear();

	if (inst.edit.pointer_in_window && r_view.mouse_x >= 0 &&
		inst.edit.action != ACT_DRAG)
	{
		Objid current_hl;

		// this also updates inst.edit.map_x/y/z
		Render3D_Query(inst, current_hl, r_view.mouse_x, r_view.mouse_y);

		if (current_hl.type == inst.edit.mode)
			inst.edit.highlight = current_hl;
	}

	inst.main_win->canvas->UpdateHighlight();
	inst.main_win->canvas->redraw();

	inst.main_win->status_bar->redraw();
}


void Render3D_Navigate(Instance &inst)
{
	float delay_ms = static_cast<float>(Nav_TimeDiff());

	delay_ms = delay_ms / 1000.0f;

	keycode_t mod = 0;

	if (inst.edit.nav_lax)
		mod = M_ReadLaxModifiers();

	float mod_factor = 1.0;
	if (mod & EMOD_SHIFT)   mod_factor = 0.5;
	if (mod & EMOD_COMMAND) mod_factor = 2.0;

	if (inst.edit.nav_fwd || inst.edit.nav_back || inst.edit.nav_right || inst.edit.nav_left)
	{
		float fwd   = inst.edit.nav_fwd   - inst.edit.nav_back;
		float right = inst.edit.nav_right - inst.edit.nav_left;

		float dx = static_cast<float>(r_view.Cos * fwd + r_view.Sin * right);
		float dy = static_cast<float>(r_view.Sin * fwd - r_view.Cos * right);

		dx = dx * mod_factor * mod_factor;
		dy = dy * mod_factor * mod_factor;

		r_view.x += dx * delay_ms;
		r_view.y += dy * delay_ms;
	}

	if (inst.edit.nav_up || inst.edit.nav_down)
	{
		float dz = (inst.edit.nav_up - inst.edit.nav_down);

		r_view.z += dz * mod_factor * delay_ms;
	}

	if (inst.edit.nav_turn_L || inst.edit.nav_turn_R)
	{
		float dang = (inst.edit.nav_turn_L - inst.edit.nav_turn_R);

		dang = dang * mod_factor * delay_ms;
		dang = CLAMP(-90, dang, 90);

		r_view.SetAngle(static_cast<float>(r_view.angle + dang));
	}

	inst.main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap(inst);
}


// returns -1 if nothing in selection or highlight, -2 if multiple
// things are selected and they have different types.
static int GrabSelectedThing(Instance &inst)
{
	int result = -1;

	if (inst.edit.Selected->empty())
	{
		if (inst.edit.highlight.is_nil())
		{
			inst.Beep("no things for copy/cut type");
			return -1;
		}

		result = inst.level.things[inst.edit.highlight.num]->type;
	}
	else
	{
		for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
		{
			const Thing *T = inst.level.things[*it];
			if (result >= 0 && T->type != result)
			{
				inst.Beep("multiple thing types");
				return -2;
			}

			result = T->type;
		}
	}

	inst.Status_Set("copied type %d", result);

	return result;
}


static void StoreSelectedThing(Instance &inst, int new_type)
{
	// this code is similar to code in UI_Thing::type_callback(),
	// but here we must handle a highlighted object.

	SelectHighlight unselect = inst.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("no things for paste type");
		return;
	}

	inst.level.basis.begin();
	inst.level.basis.setMessageForSelection("pasted type of", *inst.edit.Selected);

	for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
	{
		inst.level.basis.changeThing(*it, Thing::F_TYPE, new_type);
	}

	inst.level.basis.end();

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(inst, true /* nosave */);

	inst.Status_Set("pasted type %d", new_type);
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
static int GrabSelectedFlat(Instance &inst)
{
	int result = -1;

	if (inst.edit.Selected->empty())
	{
		if (inst.edit.highlight.is_nil())
		{
			inst.Beep("no sectors for copy/cut flat");
			return -1;
		}

		const Sector *S = inst.level.sectors[inst.edit.highlight.num];

		result = SEC_GrabFlat(S, inst.edit.highlight.parts);
	}
	else
	{
		for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
		{
			const Sector *S = inst.level.sectors[*it];
			byte parts = inst.edit.Selected->get_ext(*it);

			int tex = SEC_GrabFlat(S, parts & ~1);

			if (result >= 0 && tex != result)
			{
				inst.Beep("multiple flats present");
				return -2;
			}

			result = tex;
		}
	}

	if (result >= 0)
		inst.Status_Set("copied %s", BA_GetString(result).c_str());

	return result;
}


static void StoreSelectedFlat(Instance &inst, int new_tex)
{
	SelectHighlight unselect = inst.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("no sectors for paste flat");
		return;
	}

	inst.level.basis.begin();
	inst.level.basis.setMessageForSelection("pasted flat to", *inst.edit.Selected);

	for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
	{
		byte parts = inst.edit.Selected->get_ext(*it);

		if (parts == 1 || (parts & PART_FLOOR))
			inst.level.basis.changeSector(*it, Sector::F_FLOOR_TEX, new_tex);

		if (parts == 1 || (parts & PART_CEIL))
			inst.level.basis.changeSector(*it, Sector::F_CEIL_TEX, new_tex);
	}

	inst.level.basis.end();

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(inst, true /* nosave */);

	inst.Status_Set("pasted %s", BA_GetString(new_tex).c_str());
}


static void StoreDefaultedFlats(Instance &inst)
{
	SelectHighlight unselect = inst.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("no sectors for default");
		return;
	}

	int floor_tex = BA_InternaliseString(inst.default_floor_tex);
	int ceil_tex  = BA_InternaliseString(inst.default_ceil_tex);

	inst.level.basis.begin();
	inst.level.basis.setMessageForSelection("defaulted flat in", *inst.edit.Selected);

	for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
	{
		byte parts = inst.edit.Selected->get_ext(*it);

		if (parts == 1 || (parts & PART_FLOOR))
			inst.level.basis.changeSector(*it, Sector::F_FLOOR_TEX, floor_tex);

		if (parts == 1 || (parts & PART_CEIL))
			inst.level.basis.changeSector(*it, Sector::F_CEIL_TEX, ceil_tex);
	}

	inst.level.basis.end();

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(inst, true /* nosave */);

	inst.Status_Set("defaulted flats");
}


static int LD_GrabTex(const Instance &inst, const LineDef *L, int part)
{
	if (L->NoSided())
		return BA_InternaliseString(inst.default_wall_tex);

	if (L->OneSided())
		return L->Right(inst.level)->mid_tex;

	if (part & PART_RT_LOWER) return L->Right(inst.level)->lower_tex;
	if (part & PART_RT_UPPER) return L->Right(inst.level)->upper_tex;

	if (part & PART_LF_LOWER) return L->Left(inst.level)->lower_tex;
	if (part & PART_LF_UPPER) return L->Left(inst.level)->upper_tex;

	if (part & PART_RT_RAIL)  return L->Right(inst.level)->mid_tex;
	if (part & PART_LF_RAIL)  return L->Left(inst.level) ->mid_tex;

	// pick something reasonable for a simply selected line
	if (L->Left(inst.level)->SecRef(inst.level)->floorh > L->Right(inst.level)->SecRef(inst.level)->floorh)
		return L->Right(inst.level)->lower_tex;

	if (L->Left(inst.level)->SecRef(inst.level)->ceilh < L->Right(inst.level)->SecRef(inst.level)->ceilh)
		return L->Right(inst.level)->upper_tex;

	if (L->Left(inst.level)->SecRef(inst.level)->floorh < L->Right(inst.level)->SecRef(inst.level)->floorh)
		return L->Left(inst.level)->lower_tex;

	if (L->Left(inst.level)->SecRef(inst.level)->ceilh > L->Right(inst.level)->SecRef(inst.level)->ceilh)
		return L->Left(inst.level)->upper_tex;

	// emergency fallback
	return L->Right(inst.level)->lower_tex;
}


// returns -1 if nothing in selection or highlight, -2 if multiple
// linedefs are selected and they have different textures.
static int GrabSelectedTexture(Instance &inst)
{
	int result = -1;

	if (inst.edit.Selected->empty())
	{
		if (inst.edit.highlight.is_nil())
		{
			inst.Beep("no linedefs for copy/cut tex");
			return -1;
		}

		const LineDef *L = inst.level.linedefs[inst.edit.highlight.num];

		result = LD_GrabTex(inst, L, inst.edit.highlight.parts);
	}
	else
	{
		for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
		{
			const LineDef *L = inst.level.linedefs[*it];
			byte parts = inst.edit.Selected->get_ext(*it);

			int tex = LD_GrabTex(inst, L, parts & ~1);

			if (result >= 0 && tex != result)
			{
				inst.Beep("multiple textures present");
				return -2;
			}

			result = tex;
		}
	}

	if (result >= 0)
		inst.Status_Set("copied %s", BA_GetString(result).c_str());

	return result;
}


static void StoreSelectedTexture(Instance &inst, int new_tex)
{
	SelectHighlight unselect = inst.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		inst.Beep("no linedefs for paste tex");
		return;
	}

	inst.level.basis.begin();
	inst.level.basis.setMessageForSelection("pasted tex to", *inst.edit.Selected);

	for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
	{
		const LineDef *L = inst.level.linedefs[*it];
		byte parts = inst.edit.Selected->get_ext(*it);

		if (L->NoSided())
			continue;

		if (L->OneSided())
		{
			inst.level.basis.changeSidedef(L->right, SideDef::F_MID_TEX, new_tex);
			continue;
		}

		/* right side */
		if (parts == 1 || (parts & PART_RT_LOWER))
			inst.level.basis.changeSidedef(L->right, SideDef::F_LOWER_TEX, new_tex);

		if (parts == 1 || (parts & PART_RT_UPPER))
			inst.level.basis.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_tex);

		if (parts & PART_RT_RAIL)
			inst.level.basis.changeSidedef(L->right, SideDef::F_MID_TEX, new_tex);

		/* left side */
		if (parts == 1 || (parts & PART_LF_LOWER))
			inst.level.basis.changeSidedef(L->left, SideDef::F_LOWER_TEX, new_tex);

		if (parts == 1 || (parts & PART_LF_UPPER))
			inst.level.basis.changeSidedef(L->left, SideDef::F_UPPER_TEX, new_tex);

		if (parts & PART_LF_RAIL)
			inst.level.basis.changeSidedef(L->left, SideDef::F_MID_TEX, new_tex);
	}

	inst.level.basis.end();

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(inst, true /* nosave */);

	inst.Status_Set("pasted %s", BA_GetString(new_tex).c_str());
}


void Render3D_CB_Copy(Instance &inst)
{
	int num;

	switch (inst.edit.mode)
	{
	case ObjType::things:
		num = GrabSelectedThing(inst);
		if (num >= 0)
			Texboard_SetThing(num);
		break;

	case ObjType::sectors:
		num = GrabSelectedFlat(inst);
		if (num >= 0)
			Texboard_SetFlat(BA_GetString(num));
		break;

	case ObjType::linedefs:
		num = GrabSelectedTexture(inst);
		if (num >= 0)
			Texboard_SetTex(BA_GetString(num));
		break;

	default:
		break;
	}
}


void Render3D_CB_Paste(Instance &inst)
{
	switch (inst.edit.mode)
	{
	case ObjType::things:
		StoreSelectedThing(inst, Texboard_GetThing(inst));
		break;

	case ObjType::sectors:
		StoreSelectedFlat(inst, Texboard_GetFlatNum(inst));
		break;

	case ObjType::linedefs:
		StoreSelectedTexture(inst, Texboard_GetTexNum(inst));
		break;

	default:
		break;
	}
}


void Render3D_CB_Cut(Instance &inst)
{
	// this is repurposed to set the default texture/thing

	switch (inst.edit.mode)
	{
	case ObjType::things:
		StoreSelectedThing(inst, inst.default_thing);
		break;

	case ObjType::sectors:
		StoreDefaultedFlats(inst);
		break;

	case ObjType::linedefs:
			StoreSelectedTexture(inst, BA_InternaliseString(inst.default_wall_tex));
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
	*angle = static_cast<float>(r_view.angle * 180.0 / M_PI);
}


bool Render3D_ParseUser(const std::vector<SString> &tokens)
{
	if (tokens[0] == "camera" && tokens.size() >= 5)
	{
		r_view.x = atof(tokens[1]);
		r_view.y = atof(tokens[2]);
		r_view.z = atof(tokens[3]);

		r_view.SetAngle(static_cast<float>(atof(tokens[4])));
		return true;
	}

	if (tokens[0] == "r_modes" && tokens.size() >= 4)
	{
		r_view.texturing = atoi(tokens[1]) ? true : false;
		r_view.sprites   = atoi(tokens[2]) ? true : false;
		r_view.lighting  = atoi(tokens[3]) ? true : false;

		return true;
	}

	if (tokens[0] == "r_gravity" && tokens.size() >= 2)
	{
		r_view.gravity = atoi(tokens[1]) ? true : false;
		return true;
	}

	if (tokens[0] == "low_detail" && tokens.size() >= 2)
	{
		// ignored for compatibility
		return true;
	}

	if (tokens[0] == "gamma" && tokens.size() >= 2)
	{
		config::usegamma = MAX(0, atoi(tokens[1])) % 5;

		W_UpdateGamma();
		return true;
	}

	return false;
}

void Render3D_WriteUser(std::ostream &os)
{
	os << "camera " << SString::printf("%1.2f %1.2f %1.2f %1.2f", r_view.x, r_view.y, r_view.z,
									   r_view.angle) << '\n';
	os << "r_modes " << (r_view.texturing ? 1 : 0) << ' ' << (r_view.sprites ? 1 : 0) << ' ' <<
		(r_view.lighting ? 1 : 0) << '\n';
	os << "r_gravity " << (r_view.gravity ? 1 : 0) << '\n';
	os << "gamma " << config::usegamma << '\n';
}


//------------------------------------------------------------------------
//  COMMAND FUNCTIONS
//------------------------------------------------------------------------

static void R3D_Forward(Instance &inst)
{
	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.x += r_view.Cos * dist;
	r_view.y += r_view.Sin * dist;

	inst.main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap(inst);
}

static void R3D_Backward(Instance &inst)
{
	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.x -= r_view.Cos * dist;
	r_view.y -= r_view.Sin * dist;

	inst.main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap(inst);
}

static void R3D_Left(Instance &inst)
{
	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.x -= r_view.Sin * dist;
	r_view.y += r_view.Cos * dist;

	inst.main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap(inst);
}

static void R3D_Right(Instance &inst)
{
	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.x += r_view.Sin * dist;
	r_view.y -= r_view.Cos * dist;

	inst.main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap(inst);
}

static void R3D_Up(Instance &inst)
{
	if (r_view.gravity && config::render_lock_gravity)
	{
		inst.Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.z += dist;

	RedrawMap(inst);
}

static void R3D_Down(Instance &inst)
{
	if (r_view.gravity && config::render_lock_gravity)
	{
		inst.Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.z -= dist;

	RedrawMap(inst);
}


static void R3D_Turn(Instance &inst)
{
	float angle = static_cast<float>(atof(EXEC_Param[0]));

	// convert to radians
	angle = static_cast<float>(angle * M_PI / 180.0);

	r_view.SetAngle(static_cast<float>(r_view.angle + angle));

	RedrawMap(inst);
}


static void R3D_DropToFloor(Instance &inst)
{
	r_view.FindGroundZ();

	RedrawMap(inst);
}


static void R3D_NAV_Forward_release(Instance &inst)
{
	inst.edit.nav_fwd = 0;
}

static void R3D_NAV_Forward(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	inst.edit.nav_fwd = static_cast<float>(atof(EXEC_Param[0]));

	Nav_SetKey(inst, EXEC_CurKey, &R3D_NAV_Forward_release);
}


static void R3D_NAV_Back_release(Instance &inst)
{
	inst.edit.nav_back = 0;
}

static void R3D_NAV_Back(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	inst.edit.nav_back = static_cast<float>(atof(EXEC_Param[0]));

	Nav_SetKey(inst, EXEC_CurKey, &R3D_NAV_Back_release);
}


static void R3D_NAV_Right_release(Instance &inst)
{
	inst.edit.nav_right = 0;
}

static void R3D_NAV_Right(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	inst.edit.nav_right = static_cast<float>(atof(EXEC_Param[0]));

	Nav_SetKey(inst, EXEC_CurKey, &R3D_NAV_Right_release);
}


static void R3D_NAV_Left_release(Instance &inst)
{
	inst.edit.nav_left = 0;
}

static void R3D_NAV_Left(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	inst.edit.nav_left = static_cast<float>(atof(EXEC_Param[0]));

	Nav_SetKey(inst, EXEC_CurKey, &R3D_NAV_Left_release);
}


static void R3D_NAV_Up_release(Instance &inst)
{
	inst.edit.nav_up = 0;
}

static void R3D_NAV_Up(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (r_view.gravity && config::render_lock_gravity)
	{
		inst.Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	inst.edit.nav_up = static_cast<float>(atof(EXEC_Param[0]));

	Nav_SetKey(inst, EXEC_CurKey, &R3D_NAV_Up_release);
}


static void R3D_NAV_Down_release(Instance &inst)
{
	inst.edit.nav_down = 0;
}

static void R3D_NAV_Down(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (r_view.gravity && config::render_lock_gravity)
	{
		inst.Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	inst.edit.nav_down = static_cast<float>(atof(EXEC_Param[0]));

	Nav_SetKey(inst, EXEC_CurKey, &R3D_NAV_Down_release);
}


static void R3D_NAV_TurnLeft_release(Instance &inst)
{
	inst.edit.nav_turn_L = 0;
}

static void R3D_NAV_TurnLeft(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	float turn = static_cast<float>(atof(EXEC_Param[0]));

	// convert to radians
	inst.edit.nav_turn_L = static_cast<float>(turn * M_PI / 180.0);

	Nav_SetKey(inst, EXEC_CurKey, &R3D_NAV_TurnLeft_release);
}


static void R3D_NAV_TurnRight_release(Instance &inst)
{
	inst.edit.nav_turn_R = 0;
}

static void R3D_NAV_TurnRight(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! inst.edit.is_navigating)
		inst.Editor_ClearNav();

	float turn = static_cast<float>(atof(EXEC_Param[0]));

	// convert to radians
	inst.edit.nav_turn_R = static_cast<float>(turn * M_PI / 180.0);

	Nav_SetKey(inst, EXEC_CurKey, &R3D_NAV_TurnRight_release);
}


static void ACT_AdjustOfs_release(Instance &inst)
{
	// check if cancelled or overridden
	if (inst.edit.action != ACT_ADJUST_OFS)
		return;

	AdjustOfs_Finish(inst);
}

static void R3D_ACT_AdjustOfs(Instance &inst)
{
	if (! EXEC_CurKey)
		return;

	if (! Nav_ActionKey(inst, EXEC_CurKey, &ACT_AdjustOfs_release))
		return;

	if (inst.edit.mode != ObjType::linedefs)
	{
		inst.Beep("not in linedef mode");
		return;
	}

	AdjustOfs_Begin(inst);
}


static void R3D_Set(Instance &inst)
{
	SString var_name = EXEC_Param[0];
	SString value    = EXEC_Param[1];

	if (var_name.empty())
	{
		inst.Beep("3D_Set: missing var name");
		return;
	}

	if (value.empty())
	{
		inst.Beep("3D_Set: missing value");
		return;
	}

	 int  int_val = atoi(value);
	bool bool_val = (int_val > 0);


	if (var_name.noCaseEqual("tex"))
	{
		r_view.texturing = bool_val;
	}
	else if (var_name.noCaseEqual("obj"))
	{
		r_view.sprites = bool_val;
	}
	else if (var_name.noCaseEqual("light"))
	{
		r_view.lighting = bool_val;
	}
	else if (var_name.noCaseEqual("grav"))
	{
		r_view.gravity = bool_val;
	}
	else
	{
		inst.Beep("3D_Set: unknown var: %s", var_name.c_str());
		return;
	}

	RedrawMap(inst);
}


static void R3D_Toggle(Instance &inst)
{
	SString var_name = EXEC_Param[0];

	if (var_name.empty())
	{
		inst.Beep("3D_Toggle: missing var name");
		return;
	}

	if (var_name.noCaseEqual("tex"))
	{
		r_view.texturing = ! r_view.texturing;
	}
	else if (var_name.noCaseEqual("obj"))
	{
		r_view.sprites = ! r_view.sprites;
	}
	else if (var_name.noCaseEqual("light"))
	{
		r_view.lighting = ! r_view.lighting;
	}
	else if (var_name.noCaseEqual("grav"))
	{
		r_view.gravity = ! r_view.gravity;
	}
	else
	{
		inst.Beep("3D_Toggle: unknown var: %s", var_name.c_str());
		return;
	}

	RedrawMap(inst);
}


static void R3D_WHEEL_Move(Instance &inst)
{
	float dx = static_cast<float>(Fl::event_dx());
	float dy = static_cast<float>(Fl::event_dy());

	dy = 0 - dy;

	float speed = static_cast<float>(atof(EXEC_Param[0]));

	if (Exec_HasFlag("/LAX"))
	{
		keycode_t mod = Fl::event_state() & EMOD_ALL_MASK;

		if (mod == EMOD_SHIFT)
			speed /= 4.0;
		else if (mod == EMOD_COMMAND)
			speed *= 4.0;
	}

	r_view.x += speed * (r_view.Cos * dy + r_view.Sin * dx);
	r_view.y += speed * (r_view.Sin * dy - r_view.Cos * dx);

	inst.main_win->info_bar->SetMouse(r_view.x, r_view.y);
	RedrawMap(inst);
}


//------------------------------------------------------------------------

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
		&LinedefModule::commandAlign,
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
