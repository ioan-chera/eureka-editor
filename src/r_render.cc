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
#include "LineDef.h"
#include "m_config.h"
#include "m_game.h"
#include "m_events.h"
#include "m_vector.h"
#include "r_render.h"
#include "r_subdiv.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "ui_window.h"
#include "Vertex.h"

// config items
rgb_color_t config::transparent_col = rgbMake(0, 255, 255);

bool config::render_high_detail    = false;
bool config::render_lock_gravity   = false;
bool config::render_missing_bright = true;
bool config::render_unknown_bright = true;

int  config::render_far_clip = 32768;

// in original DOOM pixels were 20% taller than wide, giving 0.83
// as the pixel aspect ratio.
int  config::render_pixel_aspect = 83;  //  100 * width / height

bool global::use_npot_textures;

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
		invalid_low  = std::min(invalid_low,  th);
		invalid_high = std::max(invalid_high, th);
	}

	void InvalidateAll(const Document &doc)
	{
		invalid_low  = 0;
		invalid_high = doc.numThings() - 1;
	}

	void Update(Instance &inst);
};

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

		Objid o = hover::getNearestSector(inst.level, { test_x, test_y });

		if (o.num >= 0)
		{
			double z = inst.level.sectors[o.num]->floorh;
			{
				sector_3dfloors_c *ex = inst.Subdiv_3DFloorsForSector(o.num);
				if (ex->f_plane.sloped)
					z = ex->f_plane.SlopeZ(test_x, test_y);
			}

			max_floor = std::max(max_floor, z);
			hit_something = true;
		}
	}

	if (hit_something)
		z = max_floor + inst.conf.miscInfo.view_height;
}

void Render_View_t::CalcAspect()
{
	aspect_sw = static_cast<float>(screen_w);	 // things break if these are different

	aspect_sh = screen_w / (config::render_pixel_aspect / 100.0f);
}

double Render_View_t::DistToViewPlane(v2double_t map)
{
	map.x -= x;
	map.y -= y;

	return map.x * Cos + map.y * Sin;
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


static const Thing *FindPlayer(const Document &doc, int typenum)
{
	// need to search backwards (to handle Voodoo dolls properly)

	for ( int i = doc.numThings()-1 ; i >= 0 ; i--)
		if (doc.things[i]->type == typenum)
			return doc.things[i].get();

	return nullptr;  // not found
}


//------------------------------------------------------------------------

namespace thing_sec_cache
{
	void Update(Instance &inst)
	{
		// guarantee that thing_sectors has the correct size.
		// [ prevent a potential crash ]
		if (inst.level.numThings() != (int)inst.r_view.thing_sectors.size())
		{
			inst.r_view.thing_sectors.resize(inst.level.numThings());
			thing_sec_cache::InvalidateAll(inst.level);
		}

		// nothing changed?
		if (invalid_low > invalid_high)
			return;

		for (int i = invalid_low ; i <= invalid_high ; i++)
		{
			Objid obj = hover::getNearestSector(inst.level, inst.level.things[i]->xy());

			inst.r_view.thing_sectors[i] = obj.num;
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

int Render3D_CalcRotation(double viewAngle_rad, int thingAngle_deg)
{
	// thingAngle(deg) - viewAngle(deg)
	
	// 1: front. 45 degrees around 180 difference.  157d30': 202d30'
	// 2: front-left                                112d30': 157d30'
	// 3: left                                       67d30': 112d30'
	// 4: back-left                                  22d30':  67d30'
	// 5: back                                      -22d30':  22d30'
	// 6: back-right                                -67d30': -22d30'
	// 7: right                                    -112d30': -67d30'
	// 8: front-right.                             -157d30':-112d30'
	
	double thingAngle_rad = M_PI / 180.0 * thingAngle_deg;
	double angleDelta_rad = thingAngle_rad - viewAngle_rad;
	while(angleDelta_rad > 202.5 * M_PI / 180.0)
		angleDelta_rad -= 2 * M_PI;
	while(angleDelta_rad < -157.5 * M_PI / 180.0)
		angleDelta_rad += 2 * M_PI;
	
	return clamp((int)floor((202.5 * M_PI / 180.0 - angleDelta_rad) / (M_PI / 4.0) + 1.0), 1, 8);
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

	void ApplyToBasis(EditOperation &op, int field, int delta)
	{
		for (size_t i = 0 ; i < fields.size() ; i++)
		{
			if (fields[i].field == field)
			{
				op.change(type, fields[i].obj, (byte)field, fields[i].value + delta);
			}
		}
	}

private:
	int * RawObjPointer(int objnum)
	{
		switch (type)
		{
			case ObjType::things:
				return reinterpret_cast<int*>(inst.level.things[objnum].get());

			case ObjType::vertices:
				return reinterpret_cast<int *>(inst.level.vertices[objnum].get());

			case ObjType::sectors:
				return reinterpret_cast<int *>(inst.level.sectors[objnum].get());

			case ObjType::sidedefs:
				return reinterpret_cast<int *>(inst.level.sidedefs[objnum].get());

			case ObjType::linedefs:
				return reinterpret_cast<int *>(inst.level.linedefs[objnum].get());

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
	const auto L = inst.level.linedefs[ld_num];

	float lx1 = static_cast<float>(inst.level.getStart(*L).x());
	float ly1 = static_cast<float>(inst.level.getStart(*L).y());
	float lx2 = static_cast<float>(inst.level.getEnd(*L).x());
	float ly2 = static_cast<float>(inst.level.getEnd(*L).y());

	if (lx1 > lx2) std::swap(lx1, lx2);
	if (ly1 > ly2) std::swap(ly1, ly2);

	inst.edit.adjust_bbox.x1 = std::min(inst.edit.adjust_bbox.x1, lx1);
	inst.edit.adjust_bbox.y1 = std::min(inst.edit.adjust_bbox.y1, ly1);
	inst.edit.adjust_bbox.x2 = std::max(inst.edit.adjust_bbox.x2, lx2);
	inst.edit.adjust_bbox.y2 = std::max(inst.edit.adjust_bbox.y2, ly2);
}

static void AdjustOfs_CalcDistFactor(const Instance &inst, float& dx_factor, float& dy_factor)
{
	// this computes how far to move the offsets for each screen pixel
	// the mouse moves.  we want it to appear as though each texture
	// is being dragged by the mouse, e.g. if you click on the middle
	// of a switch, that switch follows the mouse pointer around.
	// such an effect can only be approximate though.

	float dx = static_cast<float>((inst.r_view.x < inst.edit.adjust_bbox.x1) ? (inst.edit.adjust_bbox.x1 - inst.r_view.x) :
			   (inst.r_view.x > inst.edit.adjust_bbox.x2) ? (inst.r_view.x - inst.edit.adjust_bbox.x2) : 0);

	float dy = static_cast<float>((inst.r_view.y < inst.edit.adjust_bbox.y1) ? (inst.edit.adjust_bbox.y1 - inst.r_view.y) :
			   (inst.r_view.y > inst.edit.adjust_bbox.y2) ? (inst.r_view.y - inst.edit.adjust_bbox.y2) : 0);

	float dist = hypot(dx, dy);

	dist = clamp(20.f, dist, 1000.f);

	dx_factor = dist / inst.r_view.aspect_sw;
	dy_factor = dist / inst.r_view.aspect_sh;
}

static void AdjustOfs_Add(Instance &inst, int ld_num, int part)
{
	if (! inst.edit.adjust_bucket)
		return;

	const auto L = inst.level.linedefs[ld_num];

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
	inst.edit.adjust_lax = inst.Exec_HasFlag("/LAX");

	int total_lines = 0;

	// we will compute the bbox of selected lines
	inst.edit.adjust_bbox.x1 = inst.edit.adjust_bbox.y1 = static_cast<float>(+9e9);
	inst.edit.adjust_bbox.x2 = inst.edit.adjust_bbox.y2 = static_cast<float>(-9e9);

	// find the sidedefs to adjust
	if (! inst.edit.Selected->empty())
	{
		for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
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
		byte parts  = static_cast<byte>(inst.edit.highlight.parts);

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

	inst.edit.adjust_lax = inst.Exec_HasFlag("/LAX");

	inst.Editor_SetAction(EditorAction::adjustOfs);
}

static void AdjustOfs_Finish(Instance &inst)
{
	if (! inst.edit.adjust_bucket)
	{
		inst.Editor_ClearAction();
		return;
	}

	int dx = iround(inst.edit.adjust_dx);
	int dy = iround(inst.edit.adjust_dy);

	if (dx || dy)
	{
		EditOperation op(inst.level.basis);
		op.setMessage("adjusted offsets");

		inst.edit.adjust_bucket->ApplyToBasis(op, SideDef::F_X_OFFSET, dx);
		inst.edit.adjust_bucket->ApplyToBasis(op, SideDef::F_Y_OFFSET, dy);
	}

	delete inst.edit.adjust_bucket;
	inst.edit.adjust_bucket = NULL;

	inst.Editor_ClearAction();
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

	inst.RedrawMap();
}


static void AdjustOfs_RenderAnte(const Instance &inst)
{
	if (inst.edit.action == EditorAction::adjustOfs && inst.edit.adjust_bucket)
	{
		int dx = iround(inst.edit.adjust_dx);
		int dy = iround(inst.edit.adjust_dy);

		// change it temporarily (just for the render)
		inst.edit.adjust_bucket->ApplyTemp(SideDef::F_X_OFFSET, dx);
		inst.edit.adjust_bucket->ApplyTemp(SideDef::F_Y_OFFSET, dy);
	}
}

static void AdjustOfs_RenderPost(const Instance &inst)
{
	if (inst.edit.action == EditorAction::adjustOfs && inst.edit.adjust_bucket)
	{
		inst.edit.adjust_bucket->RestoreAll();
	}
}


//------------------------------------------------------------------------

void Render3D_Draw(Instance &inst, int ox, int oy, int ow, int oh)
{
	inst.r_view.PrepareToRender(ow, oh);

	AdjustOfs_RenderAnte(inst);

#ifdef NO_OPENGL
	inst.SW_RenderWorld(ox, oy, ow, oh);
#else
	RGL_RenderWorld(inst, ox, oy, ow, oh);
#endif

	AdjustOfs_RenderPost(inst);
}


static bool Render3D_Query(Instance &inst, Objid& hl, int sx, int sy)
{
	int ow = inst.main_win->canvas->w();
	int oh = inst.main_win->canvas->h();

#ifdef NO_OPENGL
	// in OpenGL mode, UI_Canvas is a window and that means the
	// event X/Y values are relative to *it* and not the main window.
	// hence the following is only needed in software mode.
	int ox = inst.main_win->canvas->x();
	int oy = inst.main_win->canvas->y();

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

	inst.r_view.PrepareToRender(ow, oh);

	return inst.SW_QueryPoint(hl, sx, sy);
}


void Instance::Render3D_Setup()
{
	thing_sec_cache::InvalidateAll(level);
	r_view.thing_sectors.resize(0);

	if (! r_view.p_type)
	{
		r_view.p_type = THING_PLAYER1;
		r_view.px = 99999;
	}

	const Thing *player = FindPlayer(level, r_view.p_type);

	if (! player)
	{
		if (r_view.p_type != THING_DEATHMATCH)
			r_view.p_type = THING_DEATHMATCH;

		player = FindPlayer(level, r_view.p_type);
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
	inst.Editor_ClearAction();

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
		inst.main_win->info_bar->SetMouse();

		// TODO: ideally query this, like code in PointerPos
		inst.r_view.mouse_x = inst.r_view.mouse_y = -1;
	}
	else
	{
		inst.main_win->canvas->PointerPos();
		inst.main_win->info_bar->SetMouse();
	}

	inst.RedrawMap();
}


void Render3D_ScrollMap(Instance &inst, v2int_t dpos, keycode_t mod)
{
	// we separate the movement into either turning or moving up/down
	// (never both at the same time : CONFIG IT THOUGH).

	bool force_one_dir = true;

	if (force_one_dir)
	{
		if (abs(dpos.x) >= abs(dpos.y))
			dpos.y = 0;
		else
			dpos.x = 0;
	}

	bool is_strafe = (mod & EMOD_ALT) ? true : false;

	float mod_factor = 1.0;
	if (mod & EMOD_SHIFT)   mod_factor = 0.4f;
	if (mod & EMOD_COMMAND) mod_factor = 2.5f;

	float speed = inst.edit.panning_speed * mod_factor;

	if (is_strafe)
	{
		inst.r_view.x += inst.r_view.Sin * dpos.x * mod_factor;
		inst.r_view.y -= inst.r_view.Cos * dpos.x * mod_factor;
	}
	else  // turn camera
	{
		double d_ang = dpos.x * speed * M_PI / 480.0;

		inst.r_view.SetAngle(static_cast<float>(inst.r_view.angle - d_ang));
	}

	dpos.y = -dpos.y;  //TODO CONFIG ITEM

	if (is_strafe)
	{
		inst.r_view.x += inst.r_view.Cos * dpos.y * mod_factor;
		inst.r_view.y += inst.r_view.Sin * dpos.y * mod_factor;
	}
	else if (! (config::render_lock_gravity && inst.r_view.gravity))
	{
		inst.r_view.z += dpos.y * speed * 0.75;

		inst.r_view.gravity = false;
	}

	inst.main_win->info_bar->SetMouse();
	inst.RedrawMap();
}


static void DragSectors_Update(Instance &inst)
{
	float ow = static_cast<float>(inst.main_win->canvas->w());
	float x_slope = 100.0f / config::render_pixel_aspect;

	float factor = static_cast<float>(clamp(20.f, inst.edit.drag_point_dist, 1000.f) / (ow * x_slope * 0.5));
	float map_dz = -inst.edit.drag_screen_dpos.y * factor;

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
	int dz = iround(inst.edit.drag_sector_dz);
	if (dz == 0)
		return;

	EditOperation op(inst.level.basis);

	if (dz > 0)
		op.setMessage("raised sectors");
	else
		op.setMessage("lowered sectors");

	if (inst.edit.dragged.valid())
	{
		int parts = inst.edit.dragged.parts;

		inst.level.secmod.safeRaiseLower(op, inst.edit.dragged.num, parts, dz);
	}
	else
	{
		for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
		{
			int parts = inst.edit.Selected->get_ext(*it);
			parts &= ~1;

			inst.level.secmod.safeRaiseLower(op, *it, parts, dz);
		}
	}
}


static void DragThings_Update(Instance &inst)
{
	float ow = static_cast<float>(inst.main_win->canvas->w());
//	float oh = main_win->canvas->h();

	float x_slope = 100.0f / config::render_pixel_aspect;
//	float y_slope = (float)oh / (float)ow;

	float dist = clamp(20.f, inst.edit.drag_point_dist, 1000.f);

	float x_factor = dist / (ow * 0.5f);
	float y_factor = dist / (ow * x_slope * 0.5f);

	if (inst.edit.drag_thing_up_down)
	{
		// vertical positioning in Hexen and UDMF formats
		float map_dz = -inst.edit.drag_screen_dpos.y * y_factor;

		// final result is in drag_cur_x/y/z
		inst.edit.drag_cur.x = inst.edit.drag_start.x;
		inst.edit.drag_cur.y = inst.edit.drag_start.y;
		inst.edit.drag_cur.z = inst.edit.drag_start.z + map_dz;
		return;
	}

	/* move thing around XY plane */

	inst.edit.drag_cur.z = inst.edit.drag_start.z;

	// vectors for view camera
	double fwd_vx = inst.r_view.Cos;
	double fwd_vy = inst.r_view.Sin;

	double side_vx =  fwd_vy;
	double side_vy = -fwd_vx;

	double dx =  inst.edit.drag_screen_dpos.x * static_cast<double>(x_factor);
	double dy = -inst.edit.drag_screen_dpos.y * y_factor * 2.0;

	// this usually won't happen, but is a reasonable fallback...
	if (inst.edit.drag_thing_num < 0)
	{
		inst.edit.drag_cur.x = inst.edit.drag_start.x + dx * side_vx + dy * fwd_vx;
		inst.edit.drag_cur.y = inst.edit.drag_start.y + dx * side_vy + dy * fwd_vy;
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

	const auto T = inst.level.things[inst.edit.drag_thing_num];

	float old_x = static_cast<float>(T->x());
	float old_y = static_cast<float>(T->y());

	float new_x = static_cast<float>(old_x + dx * side_vx);
	float new_y = static_cast<float>(old_y + dx * side_vy);

	// recompute forward/back vector
	fwd_vx = new_x - inst.r_view.x;
	fwd_vy = new_y - inst.r_view.y;

	double fwd_len = hypot(fwd_vx, fwd_vy);
	if (fwd_len < 1)
		fwd_len = 1;

	new_x = static_cast<float>(new_x + dy * fwd_vx / fwd_len);
	new_y = static_cast<float>(new_y + dy * fwd_vy / fwd_len);

	// handle a change in floor height
	Objid old_sec = hover::getNearestSector(inst.level, { old_x, old_y });

	Objid new_sec = hover::getNearestSector(inst.level, { new_x, new_y });

	if (old_sec.valid() && new_sec.valid())
	{
		float old_z = static_cast<float>(inst.level.sectors[old_sec.num]->floorh);
		float new_z = static_cast<float>(inst.level.sectors[new_sec.num]->floorh);

		// intent here is to show proper position, NOT raise/lower things.
		// [ perhaps add a new variable? ]
		inst.edit.drag_cur.z += (new_z - old_z);
	}

	inst.edit.drag_cur.x = inst.edit.drag_start.x + new_x - old_x;
	inst.edit.drag_cur.y = inst.edit.drag_start.y + new_y - old_y;
}

void Render3D_DragThings(Instance &inst)
{
	v3double_t delta;
	delta.x = inst.edit.drag_cur.x - inst.edit.drag_start.x;
	delta.y = inst.edit.drag_cur.y - inst.edit.drag_start.y;
	delta.z = inst.edit.drag_cur.z - inst.edit.drag_start.z;

	// for movement in XY plane, ensure we don't raise/lower things
	if (! inst.edit.drag_thing_up_down)
		delta.z = 0.0;

	if (inst.edit.dragged.valid())
	{
		selection_c sel(ObjType::things);
		sel.set(inst.edit.dragged.num);

		inst.level.objects.move(sel, delta);
	}
	else
	{
		inst.level.objects.move(*inst.edit.Selected, delta);
	}

	inst.RedrawMap();
}


void Instance::Render3D_MouseMotion(v2int_t pos, keycode_t mod, v2int_t dpos)
{
	edit.pointer_in_window = true;

	// save position for Render3D_UpdateHighlight
	r_view.mouse_x = pos.x;
	r_view.mouse_y = pos.y;

	if (edit.is_panning)
	{
		Editor_ScrollMap(0, dpos, mod);
		return;
	}
	else if (edit.action == EditorAction::adjustOfs)
	{
		AdjustOfs_Delta(*this, dpos.x, dpos.y);
		return;
	}

	if (edit.action == EditorAction::click)
	{
		CheckBeginDrag();
	}
	else if (edit.action == EditorAction::drag)
	{
		// get the latest map_x/y/z coordinates
		Objid unused_hl;
		Render3D_Query(*this, unused_hl, pos.x, pos.y);

		edit.drag_screen_dpos = pos - edit.click_screen_pos;

		edit.drag_cur.x = edit.map.x;
		edit.drag_cur.y = edit.map.y;

		if (edit.mode == ObjType::sectors)
			DragSectors_Update(*this);

		if (edit.mode == ObjType::things)
			DragThings_Update(*this);

		main_win->canvas->redraw();
		main_win->status_bar->redraw();
		return;
	}

	UpdateHighlight();
}


void Instance::Render3D_UpdateHighlight()
{
	edit.highlight.clear();
	edit.split_line.clear();

	if (edit.pointer_in_window && r_view.mouse_x >= 0 &&
		edit.action != EditorAction::drag)
	{
		Objid current_hl;

		// this also updates inst.edit.map_x/y/z
		Render3D_Query(*this, current_hl, r_view.mouse_x, r_view.mouse_y);

		if (current_hl.type == edit.mode)
			edit.highlight = current_hl;
	}

	main_win->canvas->UpdateHighlight();
	main_win->canvas->redraw();

	main_win->status_bar->redraw();
}


void Instance::Render3D_Navigate()
{
	float delay_ms = static_cast<float>(Nav_TimeDiff());

	delay_ms = delay_ms / 1000.0f;

	keycode_t mod = 0;

	if (edit.nav.lax)
		mod = M_ReadLaxModifiers();

	float mod_factor = 1.0;
	if (mod & EMOD_SHIFT)   mod_factor = 0.5;
	if (mod & EMOD_COMMAND) mod_factor = 2.0;

	if (edit.nav.fwd || edit.nav.back || edit.nav.right || edit.nav.left)
	{
		float fwd   = edit.nav.fwd   - edit.nav.back;
		float right = edit.nav.right - edit.nav.left;

		float dx = static_cast<float>(r_view.Cos * fwd + r_view.Sin * right);
		float dy = static_cast<float>(r_view.Sin * fwd - r_view.Cos * right);

		dx = dx * mod_factor * mod_factor;
		dy = dy * mod_factor * mod_factor;

		r_view.x += static_cast<double>(dx) * delay_ms;
		r_view.y += static_cast<double>(dy) * delay_ms;
	}

	if (edit.nav.up || edit.nav.down)
	{
		float dz = (edit.nav.up - edit.nav.down);

		r_view.z += static_cast<double>(dz) * mod_factor * delay_ms;
	}

	if (edit.nav.turn_L || edit.nav.turn_R)
	{
		float dang = (edit.nav.turn_L - edit.nav.turn_R);

		dang = dang * mod_factor * delay_ms;
		dang = clamp(-90.f, dang, 90.f);

		r_view.SetAngle(static_cast<float>(r_view.angle + dang));
	}

	main_win->info_bar->SetMouse();
	RedrawMap();
}


// returns -1 if nothing in selection or highlight, -2 if multiple
// things are selected and they have different types.
int Instance::GrabSelectedThing()
{
	int result = -1;

	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("no things for copy/cut type");
			return -1;
		}

		result = level.things[edit.highlight.num]->type;
	}
	else
	{
		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			const auto T = level.things[*it];
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


void Instance::StoreSelectedThing(int new_type)
{
	// this code is similar to code in UI_Thing::type_callback(),
	// but here we must handle a highlighted object.

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("no things for paste type");
		return;
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("pasted type of", *edit.Selected);

		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			op.changeThing(*it, Thing::F_TYPE, new_type);
		}
	}

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);

	Status_Set("pasted type %d", new_type);
}


static StringID SEC_GrabFlat(const Sector *S, int part)
{
	if (part & PART_CEIL)
		return S->ceil_tex;

	if (part & PART_FLOOR)
		return S->floor_tex;

	return S->floor_tex;
}


// returns -1 if nothing in selection or highlight, -2 if multiple
// sectors are selected and they have different flats.
StringID Instance::GrabSelectedFlat()
{
	StringID result(-1);

	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("no sectors for copy/cut flat");
			return StringID(-1);
		}

		const auto S = level.sectors[edit.highlight.num];

		result = SEC_GrabFlat(S.get(), edit.highlight.parts);
	}
	else
	{
		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			const auto S = level.sectors[*it];
			byte parts = edit.Selected->get_ext(*it);

			StringID tex = SEC_GrabFlat(S.get(), parts & ~1);

			if (result.isValid() && tex != result)
			{
				Beep("multiple flats present");
				return StringID(-2);
			}

			result = tex;
		}
	}

	if (result.isValid())
		Status_Set("copied %s", BA_GetString(result).c_str());

	return result;
}


void Instance::StoreSelectedFlat(StringID new_tex)
{
	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("no sectors for paste flat");
		return;
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("pasted flat to", *edit.Selected);

		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			byte parts = edit.Selected->get_ext(*it);

			if (parts == 1 || (parts & PART_FLOOR))
				op.changeSector(*it, Sector::F_FLOOR_TEX, new_tex);

			if (parts == 1 || (parts & PART_CEIL))
				op.changeSector(*it, Sector::F_CEIL_TEX, new_tex);
		}
	}

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);

	Status_Set("pasted %s", BA_GetString(new_tex).c_str());
}


void Instance::StoreDefaultedFlats()
{
	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("no sectors for default");
		return;
	}

	StringID floor_tex = BA_InternaliseString(conf.default_floor_tex);
	StringID ceil_tex  = BA_InternaliseString(conf.default_ceil_tex);

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("defaulted flat in", *edit.Selected);

		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			byte parts = edit.Selected->get_ext(*it);

			if (parts == 1 || (parts & PART_FLOOR))
				op.changeSector(*it, Sector::F_FLOOR_TEX, floor_tex);

			if (parts == 1 || (parts & PART_CEIL))
				op.changeSector(*it, Sector::F_CEIL_TEX, ceil_tex);
		}
	}

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);

	Status_Set("defaulted flats");
}


StringID Instance::LD_GrabTex(const LineDef *L, int part) const
{
	if (L->NoSided())
		return BA_InternaliseString(conf.default_wall_tex);

	if (L->OneSided())
		return level.getRight(*L)->mid_tex;

	if (part & PART_RT_LOWER) return level.getRight(*L)->lower_tex;
	if (part & PART_RT_UPPER) return level.getRight(*L)->upper_tex;

	if (part & PART_LF_LOWER) return level.getLeft(*L)->lower_tex;
	if (part & PART_LF_UPPER) return level.getLeft(*L)->upper_tex;

	if (part & PART_RT_RAIL)  return level.getRight(*L)->mid_tex;
	if (part & PART_LF_RAIL)  return level.getLeft(*L) ->mid_tex;

	// pick something reasonable for a simply selected line
	if (level.getSector(*level.getLeft(*L)).floorh > level.getSector(*level.getRight(*L)).floorh)
		return level.getRight(*L)->lower_tex;

	if (level.getSector(*level.getLeft(*L)).ceilh < level.getSector(*level.getRight(*L)).ceilh)
		return level.getRight(*L)->upper_tex;

	if (level.getSector(*level.getLeft(*L)).floorh < level.getSector(*level.getRight(*L)).floorh)
		return level.getLeft(*L)->lower_tex;

	if (level.getSector(*level.getLeft(*L)).ceilh > level.getSector(*level.getRight(*L)).ceilh)
		return level.getLeft(*L)->upper_tex;

	// emergency fallback
	return level.getRight(*L)->lower_tex;
}


// returns -1 if nothing in selection or highlight, -2 if multiple
// linedefs are selected and they have different textures.
StringID Instance::GrabSelectedTexture()
{
	StringID result = StringID(-1);

	if (edit.Selected->empty())
	{
		if (edit.highlight.is_nil())
		{
			Beep("no linedefs for copy/cut tex");
			return StringID(-1);
		}

		const auto L = level.linedefs[edit.highlight.num];

		result = LD_GrabTex(L.get(), edit.highlight.parts);
	}
	else
	{
		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			const auto L = level.linedefs[*it];
			byte parts = edit.Selected->get_ext(*it);

			StringID tex = LD_GrabTex(L.get(), parts & ~1);

			if (result.isValid() && tex != result)
			{
				Beep("multiple textures present");
				return StringID(-2);
			}

			result = tex;
		}
	}

	if (result.isValid())
		Status_Set("copied %s", BA_GetString(result).c_str());

	return result;
}


void Instance::StoreSelectedTexture(StringID new_tex)
{
	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("no linedefs for paste tex");
		return;
	}

	{
		EditOperation op(level.basis);
		op.setMessageForSelection("pasted tex to", *edit.Selected);

		for (sel_iter_c it(*edit.Selected) ; !it.done() ; it.next())
		{
			const auto L = level.linedefs[*it];
			byte parts = edit.Selected->get_ext(*it);

			if (L->NoSided())
				continue;

			if (L->OneSided())
			{
				op.changeSidedef(L->right, SideDef::F_MID_TEX, new_tex);
				continue;
			}

			/* right side */
			if (parts == 1 || (parts & PART_RT_LOWER))
				op.changeSidedef(L->right, SideDef::F_LOWER_TEX, new_tex);

			if (parts == 1 || (parts & PART_RT_UPPER))
				op.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_tex);

			if (parts & PART_RT_RAIL)
				op.changeSidedef(L->right, SideDef::F_MID_TEX, new_tex);

			/* left side */
			if (parts == 1 || (parts & PART_LF_LOWER))
				op.changeSidedef(L->left, SideDef::F_LOWER_TEX, new_tex);

			if (parts == 1 || (parts & PART_LF_UPPER))
				op.changeSidedef(L->left, SideDef::F_UPPER_TEX, new_tex);

			if (parts & PART_LF_RAIL)
				op.changeSidedef(L->left, SideDef::F_MID_TEX, new_tex);
		}

	}

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);

	Status_Set("pasted %s", BA_GetString(new_tex).c_str());
}


void Instance::Render3D_CB_Copy()
{
	StringID num;
	int thingnum;

	switch (edit.mode)
	{
	case ObjType::things:
		thingnum = GrabSelectedThing();
		if (thingnum >= 0)
			Texboard_SetThing(thingnum);
		break;

	case ObjType::sectors:
		num = GrabSelectedFlat();
		if (num.isValid())
			Texboard_SetFlat(BA_GetString(num), conf);
		break;

	case ObjType::linedefs:
		num = GrabSelectedTexture();
		if (num.isValid())
			Texboard_SetTex(BA_GetString(num), conf);
		break;

	default:
		break;
	}
}


void Instance::Render3D_CB_Paste()
{
	switch (edit.mode)
	{
	case ObjType::things:
		StoreSelectedThing(Texboard_GetThing(conf));
		break;

	case ObjType::sectors:
		StoreSelectedFlat(Texboard_GetFlatNum(conf));
		break;

	case ObjType::linedefs:
		StoreSelectedTexture(Texboard_GetTexNum(conf));
		break;

	default:
		break;
	}
}


void Instance::Render3D_CB_Cut()
{
	// this is repurposed to set the default texture/thing

	switch (edit.mode)
	{
	case ObjType::things:
		StoreSelectedThing(conf.default_thing);
		break;

	case ObjType::sectors:
		StoreDefaultedFlats();
		break;

	case ObjType::linedefs:
		StoreSelectedTexture(BA_InternaliseString(conf.default_wall_tex));
		break;

	default:
		break;
	}
}


void Instance::Render3D_SetCameraPos(const v2double_t &newpos)
{
	r_view.x = newpos.x;
	r_view.y = newpos.y;

	r_view.FindGroundZ();
}


void Instance::Render3D_GetCameraPos(v2double_t &pos, float *angle) const
{
	pos.x = r_view.x;
	pos.y = r_view.y;

	// convert angle from radians to degrees
	*angle = static_cast<float>(r_view.angle * 180.0 / M_PI);
}


bool Instance::Render3D_ParseUser(const std::vector<SString> &tokens)
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
		config::usegamma = std::max(0, atoi(tokens[1])) % 5;

		wad.palette.updateGamma(config::usegamma, config::panel_gamma);
		return true;
	}

	return false;
}

void Instance::Render3D_WriteUser(std::ostream &os) const
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

void Instance::R3D_Forward()
{
	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.x += r_view.Cos * dist;
	r_view.y += r_view.Sin * dist;

	main_win->info_bar->SetMouse();
	RedrawMap();
}

void Instance::R3D_Backward()
{
	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.x -= r_view.Cos * dist;
	r_view.y -= r_view.Sin * dist;

	main_win->info_bar->SetMouse();
	RedrawMap();
}

void Instance::R3D_Left()
{
	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.x -= r_view.Sin * dist;
	r_view.y += r_view.Cos * dist;

	main_win->info_bar->SetMouse();
	RedrawMap();
}

void Instance::R3D_Right()
{
	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.x += r_view.Sin * dist;
	r_view.y -= r_view.Cos * dist;

	main_win->info_bar->SetMouse();
	RedrawMap();
}

void Instance::R3D_Up()
{
	if (r_view.gravity && config::render_lock_gravity)
	{
		Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.z += dist;

	RedrawMap();
}

void Instance::R3D_Down()
{
	if (r_view.gravity && config::render_lock_gravity)
	{
		Beep("Gravity is on");
		return;
	}

	r_view.gravity = false;

	float dist = static_cast<float>(atof(EXEC_Param[0]));

	r_view.z -= dist;

	RedrawMap();
}


void Instance::R3D_Turn()
{
	float angle = static_cast<float>(atof(EXEC_Param[0]));

	// convert to radians
	angle = static_cast<float>(angle * M_PI / 180.0);

	r_view.SetAngle(static_cast<float>(r_view.angle + angle));

	RedrawMap();
}


void Instance::R3D_DropToFloor()
{
	r_view.FindGroundZ();

	RedrawMap();
}


void Instance::R3D_NAV_Forward_release()
{
	edit.nav.fwd = 0;
}

//
// For 3D movement control
//
void Instance::navigation3DMove(float *editNav, nav_release_func_t func, bool fly)
{
	if(!EXEC_CurKey)
		return;

	if(fly)
	{
		if(r_view.gravity && config::render_lock_gravity)
		{
			Beep("Gravity is on");
			return;
		}

		r_view.gravity = false;
	}

	if(!edit.is_navigating)
		edit.clearNav();

	*editNav = static_cast<float>(atof(EXEC_Param[0]));

	Nav_SetKey(EXEC_CurKey, func);
}

//
// Applies turning adjustment
//
void Instance::navigation3DTurn(float *editNav, nav_release_func_t func)
{
	navigation3DMove(editNav, func, false);
	*editNav = static_cast<float>(*editNav * M_PI / 180.0f);	// convert it to radians now
}


void Instance::R3D_NAV_Forward()
{
	navigation3DMove(&edit.nav.fwd, &Instance::R3D_NAV_Forward_release, false);
}


void Instance::R3D_NAV_Back_release()
{
	edit.nav.back = 0;
}

void Instance::R3D_NAV_Back()
{
	navigation3DMove(&edit.nav.back, &Instance::R3D_NAV_Back_release, false);
}


void Instance::R3D_NAV_Right_release()
{
	edit.nav.right = 0;
}

void Instance::R3D_NAV_Right()
{
	navigation3DMove(&edit.nav.right, &Instance::R3D_NAV_Right_release, false);
}


void Instance::R3D_NAV_Left_release()
{
	edit.nav.left = 0;
}

void Instance::R3D_NAV_Left()
{
	navigation3DMove(&edit.nav.left, &Instance::R3D_NAV_Left_release, false);
}


void Instance::R3D_NAV_Up_release()
{
	edit.nav.up = 0;
}

void Instance::R3D_NAV_Up()
{
	navigation3DMove(&edit.nav.up, &Instance::R3D_NAV_Up_release, true);
}


void Instance::R3D_NAV_Down_release()
{
	edit.nav.down = 0;
}

void Instance::R3D_NAV_Down()
{
	navigation3DMove(&edit.nav.down, &Instance::R3D_NAV_Down_release, true);
}

void Instance::R3D_NAV_TurnLeft_release()
{
	edit.nav.turn_L = 0;
}

void Instance::R3D_NAV_TurnLeft()
{
	navigation3DTurn(&edit.nav.turn_L, &Instance::R3D_NAV_TurnLeft_release);
}


void Instance::R3D_NAV_TurnRight_release()
{
	edit.nav.turn_R = 0;
}

void Instance::R3D_NAV_TurnRight()
{
	navigation3DTurn(&edit.nav.turn_R, &Instance::R3D_NAV_TurnRight_release);
}


void Instance::ACT_AdjustOfs_release()
{
	// check if cancelled or overridden
	if (edit.action != EditorAction::adjustOfs)
		return;

	AdjustOfs_Finish(*this);
}

void Instance::R3D_ACT_AdjustOfs()
{
	if (! EXEC_CurKey)
		return;

	if (! Nav_ActionKey(EXEC_CurKey, &Instance::ACT_AdjustOfs_release))
		return;

	if (edit.mode != ObjType::linedefs)
	{
		Beep("not in linedef mode");
		return;
	}

	AdjustOfs_Begin(*this);
}


void Instance::R3D_Set()
{
	SString var_name = EXEC_Param[0];
	SString value    = EXEC_Param[1];

	if (var_name.empty())
	{
		Beep("3D_Set: missing var name");
		return;
	}

	if (value.empty())
	{
		Beep("3D_Set: missing value");
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
		Beep("3D_Set: unknown var: %s", var_name.c_str());
		return;
	}

	RedrawMap();
}


void Instance::R3D_Toggle()
{
	SString var_name = EXEC_Param[0];

	if (var_name.empty())
	{
		Beep("3D_Toggle: missing var name");
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
		Beep("3D_Toggle: unknown var: %s", var_name.c_str());
		return;
	}

	RedrawMap();
}


void Instance::R3D_WHEEL_Move()
{
	float dx = static_cast<float>(Fl::event_dx());
	float dy = static_cast<float>(Fl::event_dy());

	dy = 0 - dy;

	float speed = static_cast<float>(atof(EXEC_Param[0]));

	if (Exec_HasFlag("/LAX"))
	{
		keycode_t mod = Fl::event_state() & EMOD_ALL_MASK;

		if (mod == EMOD_SHIFT)
			speed /= 4.0f;
		else if (mod == EMOD_COMMAND)
			speed *= 4.0f;
	}

	r_view.x += speed * (r_view.Cos * dy + r_view.Sin * dx);
	r_view.y += speed * (r_view.Sin * dy - r_view.Cos * dx);

	main_win->info_bar->SetMouse();
	RedrawMap();
}


//------------------------------------------------------------------------

static editor_command_t  render_commands[] =
{
	{	"3D_Set", NULL,
		&Instance::R3D_Set,
		/* flags */ NULL,
		/* keywords */ "tex obj light grav"
	},

	{	"3D_Toggle", NULL,
		&Instance::R3D_Toggle,
		/* flags */ NULL,
		/* keywords */ "tex obj light grav"
	},

	{	"3D_Forward", NULL,
		&Instance::R3D_Forward
	},

	{	"3D_Backward", NULL,
		&Instance::R3D_Backward
	},

	{	"3D_Left", NULL,
		&Instance::R3D_Left
	},

	{	"3D_Right", NULL,
		&Instance::R3D_Right
	},

	{	"3D_Up", NULL,
		&Instance::R3D_Up
	},

	{	"3D_Down", NULL,
		&Instance::R3D_Down
	},

	{	"3D_Turn", NULL,
		&Instance::R3D_Turn
	},

	{	"3D_DropToFloor", NULL,
		&Instance::R3D_DropToFloor
	},

	{	"3D_ACT_AdjustOfs", NULL,
		&Instance::R3D_ACT_AdjustOfs
	},

	{	"3D_WHEEL_Move", NULL,
		&Instance::R3D_WHEEL_Move
	},

	{	"3D_NAV_Forward", NULL,
		&Instance::R3D_NAV_Forward
	},

	{	"3D_NAV_Back", NULL,
		&Instance::R3D_NAV_Back
	},

	{	"3D_NAV_Right", NULL,
		&Instance::R3D_NAV_Right
	},

	{	"3D_NAV_Left", NULL,
		&Instance::R3D_NAV_Left
	},

	{	"3D_NAV_Up", NULL,
		&Instance::R3D_NAV_Up
	},

	{	"3D_NAV_Down", NULL,
		&Instance::R3D_NAV_Down
	},

	{	"3D_NAV_TurnLeft", NULL,
		&Instance::R3D_NAV_TurnLeft
	},

	{	"3D_NAV_TurnRight", NULL,
		&Instance::R3D_NAV_TurnRight
	},

	// backwards compatibility.
	// [ we cannot remap this in FindEditorCommand, it fails the context check ]
	{	"3D_Align", NULL,
		&Instance::CMD_LIN_Align,
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
