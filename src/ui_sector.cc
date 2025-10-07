//------------------------------------------------------------------------
//  SECTOR PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2019 Andrew Apted
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

#include "Instance.h"
#include "main.h"
#include "ui_misc.h"
#include "ui_window.h"

#include "e_checks.h"
#include "e_cutpaste.h"
#include "e_main.h"
#include "e_sector.h"
#include "e_things.h"
#include "m_config.h"
#include "m_game.h"
#include "r_render.h"
#include "Sector.h"
#include "w_rawdef.h"
#include "w_texture.h"


// config items
int config::floor_bump_small  = 1;
int config::floor_bump_medium = 8;
int config::floor_bump_large  = 64;

int config::light_bump_small  = 4;
int config::light_bump_medium = 16;
int config::light_bump_large  = 64;


//TODO make these configurable
static const int headroom_presets[UI_SectorBox::HEADROOM_BUTTONS] =
{
	0, 64, 96, 128, 192, 256
};


//
// UI_SectorBox Constructor
//
UI_SectorBox::UI_SectorBox(Instance &inst, int X, int Y, int W, int H, const char *label) :
    MapItemBox(inst, X, Y, W, H, label)
{
	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);

	const int Y0 = Y;
	const int X0 = X;

	X += 6;
	Y += 6;

	W -= 12;
	H -= 10;


	which = new UI_Nombre(X+NOMBRE_INSET, Y, W-2*NOMBRE_INSET, NOMBRE_HEIGHT, "Sector");

	Y += which->h() + 4;


	type = new UI_DynInput(X+70, Y, 70, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);
	type->callback2(dyntype_callback, this);
	type->type(FL_INT_INPUT);
	type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	choose = new Fl_Button(X+W/2, Y, 80, 24, "Choose");
	choose->callback(button_callback, this);

	Y += type->h() + 4;


	desc = new Fl_Output(X+70, Y, W-78, 24, "Desc: ");
	desc->align(FL_ALIGN_LEFT);

	Y += desc->h() + 12;


	tag = new UI_DynIntInput(X+70, Y, 64, 24, "Tag: ");
	tag->align(FL_ALIGN_LEFT);
	tag->callback(tag_callback, this);
	tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	fresh_tag = new Fl_Button(tag->x() + tag->w() + 20, Y+1, 64, 22, "fresh");
	fresh_tag->callback(button_callback, this);

	Y += tag->h() + 4;


	light = new UI_DynIntInput(X+70, Y, 64, 24, "Light: ");
	light->align(FL_ALIGN_LEFT);
	light->callback(light_callback, this);
	light->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	lt_down = new Fl_Button(light->x() + light->w() + 20, Y+1, 30, 22, "-");
	lt_up   = new Fl_Button(light->x() + light->w() + 60, Y+1, 30, 22, "+");

	lt_down->labelfont(FL_HELVETICA_BOLD);
	lt_up  ->labelfont(FL_HELVETICA_BOLD);
	lt_down->labelsize(16);
	lt_up  ->labelsize(16);

	lt_down->callback(button_callback, this);
	lt_up  ->callback(button_callback, this);

	Y += light->h() + 20;


	c_pic = new UI_Pic(inst, X+W-82, Y-2,  64, 64, "Ceil");
	f_pic = new UI_Pic(inst, X+W-82, Y+74, 64, 64, "Floor");

	c_pic->callback(tex_callback, this);
	f_pic->callback(tex_callback, this);

	Y += 10;


	c_tex = new UI_DynInput(X+70, Y-5, 108, 24, "Ceiling: ");
	c_tex->align(FL_ALIGN_LEFT);
	c_tex->callback(tex_callback, this);
	c_tex->callback2(dyntex_callback, this);
	c_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += c_tex->h() + 3;


	ceil_h = new UI_DynIntInput(X+70, Y, 64, 24, "");
	ceil_h->align(FL_ALIGN_LEFT);
	ceil_h->callback(height_callback, this);
	ceil_h->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	ce_down = new Fl_Button(X+28,  Y+1, 30, 22, "-");
	ce_up   = new Fl_Button(X+145, Y+1, 30, 22, "+");

	ce_down->labelfont(FL_HELVETICA_BOLD);
	ce_up  ->labelfont(FL_HELVETICA_BOLD);
	ce_down->labelsize(16);
	ce_up  ->labelsize(16);

	ce_down->callback(button_callback, this);
	ce_up  ->callback(button_callback, this);

	Y += ceil_h->h() + 10;


	floor_h = new UI_DynIntInput(X+70, Y, 64, 24, "");
	floor_h->align(FL_ALIGN_LEFT);
	floor_h->callback(height_callback, this);
	floor_h->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	fl_down = new Fl_Button(X+28,  Y+1, 30, 22, "-");
	fl_up   = new Fl_Button(X+145, Y+1, 30, 22, "+");

	fl_down->labelfont(FL_HELVETICA_BOLD);
	fl_up  ->labelfont(FL_HELVETICA_BOLD);
	fl_down->labelsize(16);
	fl_up  ->labelsize(16);

	fl_down->callback(button_callback, this);
	fl_up  ->callback(button_callback, this);

	Y += floor_h->h() + 3;


	f_tex = new UI_DynInput(X+70, Y+5, 108, 24, "Floor:   ");
	f_tex->align(FL_ALIGN_LEFT);
	f_tex->callback(tex_callback, this);
	f_tex->callback2(dyntex_callback, this);
	f_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += f_tex->h() + 33;


	headroom = new UI_DynIntInput(X+100, Y, 56, 24, "Headroom: ");
	headroom->align(FL_ALIGN_LEFT);
	headroom->callback(headroom_callback, this);
	headroom->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	for (int i = 0 ; i < HEADROOM_BUTTONS ; i++)
	{
		int hx = (i < 2) ? (X + 170 + i * 60) : (X + 50 + (i-2) * 60);
		int hy = Y + 28 * ((i < 2) ? 0 : 1);

		hd_buttons[i] = new Fl_Button(hx, hy+1, 45, 22);
		hd_buttons[i]->copy_label(SString(headroom_presets[i]).c_str());
		hd_buttons[i]->callback(headroom_callback, this);
	}

	Y += headroom->h() + 50;


	// generalized sector stuff

	bm_title = new Fl_Box(FL_NO_BOX, X+10, Y, 100, 24, "Sector flags:");
	bm_title->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

	Y += 28;

	genStartX = X - X0;
	genStartY = Y - Y0;

	mFixUp.loadFields({ type, light, tag, ceil_h, floor_h, c_tex, f_tex, headroom });

	end();

	resizable(NULL);
}


//------------------------------------------------------------------------

void UI_SectorBox::height_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int f_h = atoi(box->floor_h->value());
	int c_h = atoi(box->ceil_h->value());

	f_h = clamp(-32767, f_h, 32767);
	c_h = clamp(-32767, c_h, 32767);

	if (! box->inst.edit.Selected->empty())
	{
		{
			EditOperation op(box->inst.level.basis);

			if (w == box->floor_h)
				op.setMessageForSelection("edited floor of", *box->inst.edit.Selected);
			else
				op.setMessageForSelection("edited ceiling of", *box->inst.edit.Selected);

			for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			{
				if (w == box->floor_h)
					op.changeSector(*it, Sector::F_FLOORH, f_h);
				else
					op.changeSector(*it, Sector::F_CEILH, c_h);
			}
		}

		box->mFixUp.setInputValue(box->floor_h, SString(f_h).c_str());
		box->mFixUp.setInputValue(box->ceil_h, SString(c_h).c_str());
		box->mFixUp.setInputValue(box->headroom, SString(c_h - f_h).c_str());
	}
}

void UI_SectorBox::headroom_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int room = atoi(box->headroom->value());

	// handle the shortcut buttons
	for (int i = 0 ; i < HEADROOM_BUTTONS ; i++)
	{
		if (w == box->hd_buttons[i])
		{
			room = atoi(w->label());
		}
	}

	if (!box->inst.edit.Selected->empty())
	{
		box->mFixUp.checkDirtyFields();

		{
			EditOperation op(box->inst.level.basis);
			op.setMessageForSelection("edited headroom of", *box->inst.edit.Selected);

			for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			{
				int new_h = box->inst.level.sectors[*it]->floorh + room;

				new_h = clamp(-32767, new_h, 32767);

				op.changeSector(*it, Sector::F_CEILH, new_h);
			}
		}

		box->UpdateField();
	}
}


void UI_SectorBox::tex_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	if (box->obj < 0)
		return;

	bool is_pic   = (w == box->f_pic || w == box->c_pic);
	bool is_floor = (w == box->f_pic || w == box->f_tex);

	// MMB on ceiling flat image sets to sky
	if (w == box->c_pic && Fl::event_button() == FL_MIDDLE_MOUSE)
	{
		box->SetFlat(box->inst.conf.miscInfo.sky_flat, PART_CEIL);
		return;
	}

	// LMB on the flat image just selects/unselects it (red border)
	if (is_pic && Fl::event_button() != FL_RIGHT_MOUSE)
	{
		auto pic = static_cast<UI_Pic *>(w);

		pic->Selected(! pic->Selected());

		if (pic->Selected())
			box->inst.main_win->BrowserMode(BrowserMode::flats);
		return;
	}

	SString new_flat;

	// right click sets to default value
	// [ Note the 'is_pic' check prevents a bug when using RMB in browser ]
	if (is_pic && Fl::event_button() == FL_RIGHT_MOUSE)
		new_flat = is_floor ? box->inst.conf.default_floor_tex : box->inst.conf.default_ceil_tex;
	else if (is_floor)
		new_flat = NormalizeTex(box->f_tex->value());
	else
		new_flat = NormalizeTex(box->c_tex->value());

	box->InstallFlat(new_flat, is_floor ? PART_FLOOR : PART_CEIL);
}


void UI_SectorBox::InstallFlat(const SString &name, int filter_parts)
{
	StringID tex_num = BA_InternaliseString(name);

	if (! inst.edit.Selected->empty())
	{
		mFixUp.checkDirtyFields();

		EditOperation op(inst.level.basis);
		op.setMessageForSelection("edited texture on", *inst.edit.Selected);

		for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
		{
			int parts = inst.edit.Selected->get_ext(*it);
			if (parts == 1)
				parts = filter_parts;

			if (parts & filter_parts & PART_FLOOR)
				op.changeSector(*it, Sector::F_FLOOR_TEX, tex_num);

			if (parts & filter_parts & parts & PART_CEIL)
				op.changeSector(*it, Sector::F_CEIL_TEX, tex_num);
		}
	}

	UpdateField();
}


void UI_SectorBox::dyntex_callback(Fl_Widget *w, void *data)
{
	// change picture to match the input, BUT does not change the map

	UI_SectorBox *box = (UI_SectorBox *)data;

	if (box->obj < 0)
		return;

	if (w == box->f_tex)
	{
		box->f_pic->GetFlat(box->f_tex->value());
	}
	else if (w == box->c_tex)
	{
		box->c_pic->GetFlat(box->c_tex->value());
	}
}


void UI_SectorBox::SetFlat(const SString &name, int parts)
{
	if(parts & PART_FLOOR)
		mFixUp.setInputValue(f_tex, name.c_str());

	if(parts & PART_CEIL)
		mFixUp.setInputValue(c_tex, name.c_str());

	InstallFlat(name, parts);
}


void UI_SectorBox::type_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int mask  = 65535;
	int value = atoi(box->type->value());

	int gen_mask = box->basicSectorMask;

	// when generalize sectors active, typing a low value (which does
	// not touch any bitflags) should just update the TYPE part, and
	// leave the existing bitflags alone.

	if (w == box->type && value > gen_mask)
	{
		// the value is too large and can affect the bitflags, so we
		// update the WHOLE sector type.  If generalized sectors are
		// active, then the panel will reinterpret the typed value.
	}
	else if (!box->inst.conf.gen_sectors.empty())
	{
		// Boom and ZDoom generalized sectors

		mask = 0;
		for(const SectorFlagButton &button : box->bm_buttons)
		{
			if(w == button.button.get())
			{
				mask = button.info->value;
				value = button.button->value() ? mask : 0;
				break;
			}
			if(w == button.choice.get())
			{
				int idx = button.choice->value();
				mask = button.info->value;
				value = button.info->options[idx].value;
				break;
			}
		}

		if (mask == 0)
		{
			mask = gen_mask;
		}
	}

	box->InstallSectorType(mask, value);
}


void UI_SectorBox::InstallSectorType(int mask, int value)
{
	value &= mask;

	if (! inst.edit.Selected->empty())
	{
		mFixUp.checkDirtyFields();
		EditOperation op(inst.level.basis);
		op.setMessageForSelection("edited type of", *inst.edit.Selected);

		for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
		{
			int old_type = inst.level.sectors[*it]->type;

			op.changeSector(*it, Sector::F_TYPE, (old_type & ~mask) | value);
		}
	}

	// update the description
	UpdateField(Sector::F_TYPE);
}


void UI_SectorBox::dyntype_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	if (box->obj < 0)
		return;

	int value = atoi(box->type->value());

	// when generalize sectors in effect, the name should just
	// show the TYPE part of the sector type.

	if (!box->inst.conf.gen_sectors.empty())
	{
		int gen_mask = box->basicSectorMask;

		value &= gen_mask;
	}

	const sectortype_t &info = box->inst.M_GetSectorType(value);

	box->desc->value(info.desc.c_str());
}


void UI_SectorBox::SetSectorType(int new_type)
{
	if (obj < 0)
		return;


	auto buffer = SString(new_type);
	mFixUp.setInputValue(type, buffer.c_str());	// was updated by this

	InstallSectorType(basicSectorMask, new_type);
}


void UI_SectorBox::light_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int new_lt = atoi(box->light->value());

	// we allow 256 if explicitly typed, otherwise limit to 255
	if (new_lt > 256)
		new_lt = 255;

	if (new_lt < 0)
		new_lt = 0;

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited light of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			op.changeSector(*it, Sector::F_LIGHT, new_lt);
		}
	}
}


void UI_SectorBox::tag_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int new_tag = atoi(box->tag->value());

	new_tag = clamp(-32767, new_tag, 32767);

	if (!box->inst.edit.Selected->empty())
		box->inst.level.checks.tagsApplyNewValue(new_tag);
}

void UI_SectorBox::FreshTag()
{
	mFixUp.checkDirtyFields();

	int new_tag = findFreeTag(inst, true);

	if (new_tag > 32767)
	{
		inst.Beep("Out of tag numbers");
		return;
	}

	if(!inst.edit.Selected->empty())
	{
		inst.level.checks.tagsApplyNewValue(new_tag);
	}
}


void UI_SectorBox::button_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	box->mFixUp.checkDirtyFields();

	if (w == box->choose)
	{
		box->inst.main_win->BrowserMode(BrowserMode::sectorTypes);
		return;
	}

	if (w == box->fresh_tag)
	{
		box->FreshTag();
		return;
	}

	keycode_t mod = Fl::event_state() & EMOD_ALL_MASK;

	int lt_step = config::light_bump_medium;

	if (mod & EMOD_SHIFT)
		lt_step = config::light_bump_small;
	else if (mod & EMOD_COMMAND)
		lt_step = config::light_bump_large;

	if (w == box->lt_up)
	{
		box->inst.level.secmod.sectorsAdjustLight(+lt_step);
		return;
	}
	else if (w == box->lt_down)
	{
		box->inst.level.secmod.sectorsAdjustLight(-lt_step);
		return;
	}

	int mv_step = config::floor_bump_medium;

	if (mod & EMOD_SHIFT)
		mv_step = config::floor_bump_small;
	else if (mod & EMOD_COMMAND)
		mv_step = config::floor_bump_large;

	if (w == box->ce_up)
	{
		box->inst.ExecuteCommand("SEC_Ceil", SString(+mv_step));
		return;
	}
	else if (w == box->ce_down)
	{
		box->inst.ExecuteCommand("SEC_Ceil", SString(-mv_step));
		return;
	}

	if (w == box->fl_up)
	{
		box->inst.ExecuteCommand("SEC_Floor", SString(+mv_step));
		return;
	}
	else if (w == box->fl_down)
	{
		box->inst.ExecuteCommand("SEC_Floor", SString(-mv_step));
		return;
	}
}


//------------------------------------------------------------------------

void UI_SectorBox::UpdateField(int field)
{
	const Sector *sector = inst.level.isSector(obj) ? inst.level.sectors[obj].get() : nullptr;
	if (field < 0 || field == Sector::F_FLOORH || field == Sector::F_CEILH)
	{
		if (inst.level.isSector(obj))
		{
			mFixUp.setInputValue(floor_h, SString(sector->floorh).c_str());
			mFixUp.setInputValue(ceil_h, SString(sector->ceilh).c_str());
			mFixUp.setInputValue(headroom, SString(sector->HeadRoom()).c_str());
		}
		else
		{
			mFixUp.setInputValue(floor_h, "");
			mFixUp.setInputValue(ceil_h, "");
			mFixUp.setInputValue(headroom, "");
		}
	}

	if (field < 0 || field == Sector::F_FLOOR_TEX || field == Sector::F_CEIL_TEX)
	{
		if (inst.level.isSector(obj))
		{
			mFixUp.setInputValue(f_tex, sector->FloorTex().c_str());
			mFixUp.setInputValue(c_tex, sector->CeilTex().c_str());

			f_pic->GetFlat(sector->FloorTex());
			c_pic->GetFlat(sector->CeilTex());

			f_pic->AllowHighlight(true);
			c_pic->AllowHighlight(true);
		}
		else
		{
			mFixUp.setInputValue(f_tex, "");
			mFixUp.setInputValue(c_tex, "");

			f_pic->Clear();
			c_pic->Clear();

			f_pic->AllowHighlight(false);
			c_pic->AllowHighlight(false);
		}
	}

	if (field < 0 || field == Sector::F_TYPE)
	{
		for(const SectorFlagButton &button : bm_buttons)
		{
			if(button.button)
				button.button->value(0);
			else if(button.choice)
				button.choice->value(0);
		}

		if (inst.level.isSector(obj))
		{
			int value = sector->type;
			int mask  = basicSectorMask;

			mFixUp.setInputValue(type, SString(value & mask).c_str());

			const sectortype_t &info = inst.M_GetSectorType(value & mask);
			desc->value(info.desc.c_str());

			for(const SectorFlagButton &button : bm_buttons)
			{
				if(button.button)
					button.button->value((value & button.info->value) ? 1 : 0);
				else if(button.choice)
				{
					int masked = value & button.info->value;
					for(size_t i = 0; i < button.info->options.size(); ++i)
					{
						if(masked == button.info->options[i].value)
						{
							button.choice->value((int)i);
							break;
						}
					}
				}
			}
		}
		else
		{
			mFixUp.setInputValue(type, "");
			desc->value("");
		}
	}

	if (field < 0 || field == Sector::F_LIGHT || field == Sector::F_TAG)
	{
		if (inst.level.isSector(obj))
		{
			mFixUp.setInputValue(light, SString(sector->light).c_str());
			mFixUp.setInputValue(tag, SString(sector->tag).c_str());
		}
		else
		{
			mFixUp.setInputValue(light, "");
			mFixUp.setInputValue(tag, "");
		}
	}
}


int UI_SectorBox::GetSelectedPics() const
{
	return	(f_pic->Selected() ? PART_FLOOR : 0) |
			(c_pic->Selected() ? PART_CEIL  : 0);
}

int UI_SectorBox::GetHighlightedPics() const
{
	return	(f_pic->Highlighted() ? PART_FLOOR : 0) |
			(c_pic->Highlighted() ? PART_CEIL  : 0);
}


void UI_SectorBox::CB_Copy(int parts)
{
	if (parts == (PART_FLOOR | PART_CEIL))
	{
		inst.Beep("multiple textures");
		return;
	}

	const char *name = NULL;

	if (parts == PART_CEIL)
		name = c_tex->value();
	else
		name = f_tex->value();

	Texboard_SetFlat(name, inst.conf);

	inst.Status_Set("copied %s", name);
}


void UI_SectorBox::CB_Paste(int parts, StringID new_tex)
{
	if (inst.edit.Selected->empty())
		return;

	mFixUp.checkDirtyFields();

	{
		EditOperation op(inst.level.basis);
		op.setMessage("pasted %s", BA_GetString(new_tex).c_str());

		for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
		{
			if (parts & PART_FLOOR) op.changeSector(*it, Sector::F_FLOOR_TEX, new_tex);
			if (parts & PART_CEIL)  op.changeSector(*it, Sector::F_CEIL_TEX,  new_tex);
		}
	}

	UpdateField();
}


void UI_SectorBox::CB_Cut(int parts)
{
	StringID new_floor = BA_InternaliseString(inst.conf.default_floor_tex);
	StringID new_ceil  = BA_InternaliseString(inst.conf.default_ceil_tex);

	if (! inst.edit.Selected->empty())
	{
		mFixUp.checkDirtyFields();

		{
			EditOperation op(inst.level.basis);
			op.setMessageForSelection("cut texture on", *inst.edit.Selected);

			for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
			{
				if (parts & PART_FLOOR) op.changeSector(*it, Sector::F_FLOOR_TEX, new_floor);
				if (parts & PART_CEIL)  op.changeSector(*it, Sector::F_CEIL_TEX,  new_ceil);
			}
		}

		UpdateField();
	}
}

//
// Sector box clipboard operation
//
bool UI_SectorBox::ClipboardOp(EditCommand op)
{
	if (obj < 0)
		return false;

	int parts = GetSelectedPics();

	if (parts == 0)
		parts = GetHighlightedPics();

	if (parts == 0)
		return false;

	switch (op)
	{
		case EditCommand::copy:
			CB_Copy(parts);
			break;

		case EditCommand::paste:
			CB_Paste(parts, Texboard_GetFlatNum(inst.conf));
			break;

		case EditCommand::cut:
			CB_Cut(parts);
			break;

		case EditCommand::del:
			// abuse the delete function to turn sector ceilings into sky
			CB_Paste(parts, BA_InternaliseString(inst.conf.miscInfo.sky_flat));
			break;
	}

	return true;
}


void UI_SectorBox::BrowsedItem(BrowserMode kind, int number, const char *name, int e_state)
{
	if (obj < 0)
		return;

	if (kind == BrowserMode::flats || kind == BrowserMode::textures)
	{
		int parts = GetSelectedPics();

		if (parts == 0)
		{
			if (inst.edit.render3d)
				parts = PART_FLOOR | PART_CEIL;
			else if (inst.edit.sector_render_mode == SREND_Ceiling)
				parts = PART_CEIL;
			else if (e_state & FL_BUTTON3)
				parts = PART_CEIL;
			else
				parts = PART_FLOOR;
		}

		SetFlat(name, parts);
	}
	else if (kind == BrowserMode::sectorTypes)
	{
		SetSectorType(number);
	}
}


void UI_SectorBox::UnselectPics()
{
	f_pic->Unhighlight();
	c_pic->Unhighlight();

	f_pic->Selected(false);
	c_pic->Selected(false);
}


// FIXME: make a method of Sector class
void UI_SectorBox::UpdateTotal(const Document &doc) noexcept
{
	which->SetTotal(doc.numSectors());
}


void UI_SectorBox::UpdateGameInfo(const LoadingData &loaded, const ConfigData &config)
{
	for(const SectorFlagButton &button : bm_buttons)
	{
		if(button.button)
			this->remove(button.button.get());
		else if(button.choice)
			this->remove(button.choice.get());
	}
	bm_buttons.clear();

	int Y = y() + genStartY;
	int X = x() + genStartX;
	int Ycheck = Y;
	int Ychoice = Y;
	begin();
	for(const gensector_t &gensec : config.gen_sectors)
	{
		SectorFlagButton button = {};
		if(gensec.options.empty())
		{
			// Simple flag
			button.button = std::make_unique<Fl_Check_Button>(X + 28, Ycheck, 94, 20, "");
			button.button->copy_label(gensec.label.c_str());
			button.button->labelsize(12);
			button.button->callback(type_callback, this);
			Ycheck += 20;
		}
		else
		{
			// Choice
			button.choice = std::make_unique<Fl_Choice>(X + w() - 95, Ychoice, 80, 24, "");
			button.choice->copy_label((gensec.label + ": ").c_str());
			for(const gensector_t::option_t &opt : gensec.options)
				button.choice->add(opt.label.c_str(), 0, nullptr);
			button.choice->value(0);
			button.choice->callback(type_callback, this);
			Ychoice += 24;
		}
		button.info = &gensec;
		bm_buttons.push_back(std::move(button));
	}
	end();
	if(config.gen_sectors.empty())
		bm_title->hide();
	else
		bm_title->show();
	basicSectorMask = M_CalcSectorTypeMask(config);

	UpdateField();

	redraw();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
