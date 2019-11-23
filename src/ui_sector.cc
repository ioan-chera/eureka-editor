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

#include "main.h"
#include "ui_window.h"

#include "e_checks.h"
#include "e_cutpaste.h"
#include "e_main.h"
#include "e_sector.h"
#include "e_things.h"
#include "m_game.h"
#include "r_render.h"
#include "w_rawdef.h"
#include "w_texture.h"


// config items
int floor_bump_small  = 1;
int floor_bump_medium = 8;
int floor_bump_large  = 64;

int light_bump_small  = 4;
int light_bump_medium = 16;
int light_bump_large  = 64;


//TODO make these configurable
int headroom_presets[UI_SectorBox::HEADROOM_BUTTONS] =
{
	0, 64, 96, 128, 192, 256
};


//
// UI_SectorBox Constructor
//
UI_SectorBox::UI_SectorBox(int X, int Y, int W, int H, const char *label) :
    Fl_Group(X, Y, W, H, label),
    obj(-1), count(0)
{
	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);


	X += 6;
	Y += 6;

	W -= 12;
	H -= 10;


	which = new UI_Nombre(X+6, Y, W-12, 28, "Sector");

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


	tag = new Fl_Int_Input(X+70, Y, 64, 24, "Tag: ");
	tag->align(FL_ALIGN_LEFT);
	tag->callback(tag_callback, this);
	tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	fresh_tag = new Fl_Button(tag->x() + tag->w() + 20, Y+1, 64, 22, "fresh");
	fresh_tag->callback(button_callback, this);

	Y += tag->h() + 4;


	light = new Fl_Int_Input(X+70, Y, 64, 24, "Light: ");
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


	c_pic = new UI_Pic(X+W-82, Y-2,  64, 64, "Ceil");
	f_pic = new UI_Pic(X+W-82, Y+74, 64, 64, "Floor");

	c_pic->callback(tex_callback, this);
	f_pic->callback(tex_callback, this);

	Y += 10;


	c_tex = new UI_DynInput(X+70, Y-5, 108, 24, "Ceiling: ");
	c_tex->align(FL_ALIGN_LEFT);
	c_tex->callback(tex_callback, this);
	c_tex->callback2(dyntex_callback, this);
	c_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += c_tex->h() + 3;


	ceil_h = new Fl_Int_Input(X+70, Y, 64, 24, "");
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


	floor_h = new Fl_Int_Input(X+70, Y, 64, 24, "");
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


	headroom = new Fl_Int_Input(X+100, Y, 56, 24, "Headroom: ");
	headroom->align(FL_ALIGN_LEFT);
	headroom->callback(headroom_callback, this);
	headroom->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	for (int i = 0 ; i < HEADROOM_BUTTONS ; i++)
	{
		int hx = (i < 2) ? (X + 170 + i * 60) : (X + 50 + (i-2) * 60);
		int hy = Y + 28 * ((i < 2) ? 0 : 1);

		hd_buttons[i] = new Fl_Button(hx, hy+1, 45, 22);
		hd_buttons[i]->copy_label(Int_TmpStr(headroom_presets[i]));
		hd_buttons[i]->callback(headroom_callback, this);
	}

	Y += headroom->h() + 50;


	// generalized sector stuff

	bm_title = new Fl_Box(FL_NO_BOX, X+10, Y, 100, 24, "Boom flags:");
	bm_title->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

	Y += 28;

	bm_damage = new Fl_Choice(X+W - 95, Y, 80, 24, "Damage: ");
	bm_damage->add("NONE|5 hp|10 hp|20 hp");
	bm_damage->value(0);
	bm_damage->callback(type_callback, this);

	bm_secret = new Fl_Check_Button(X+28, Y, 94, 20, "Secret");
	bm_secret->labelsize(12);
	bm_secret->callback(type_callback, this);

	bm_friction = new Fl_Check_Button(X+28, Y+20, 94, 20, "Friction");
	bm_friction->labelsize(12);
	bm_friction->callback(type_callback, this);

	bm_wind = new Fl_Check_Button(X+28, Y+40, 94, 20, "Wind");
	bm_wind->labelsize(12);
	bm_wind->callback(type_callback, this);


	end();

	resizable(NULL);
}


//
// UI_SectorBox Destructor
//
UI_SectorBox::~UI_SectorBox()
{
}


//------------------------------------------------------------------------

void UI_SectorBox::height_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int f_h = atoi(box->floor_h->value());
	int c_h = atoi(box->ceil_h->value());

	f_h = CLAMP(-32767, f_h, 32767);
	c_h = CLAMP(-32767, c_h, 32767);

	if (! edit.Selected->empty())
	{
		BA_Begin();

		if (w == box->floor_h)
			BA_MessageForSel("edited floor of", edit.Selected);
		else
			BA_MessageForSel("edited ceiling of", edit.Selected);

		for (sel_iter_c it(edit.Selected); !it.done(); it.next())
		{
			if (w == box->floor_h)
				BA_ChangeSEC(*it, Sector::F_FLOORH, f_h);
			else
				BA_ChangeSEC(*it, Sector::F_CEILH, c_h);
		}

		BA_End();

		box-> floor_h->value(Int_TmpStr(f_h));
		box->  ceil_h->value(Int_TmpStr(c_h));
		box->headroom->value(Int_TmpStr(c_h - f_h));
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

	if (! edit.Selected->empty())
	{
		BA_Begin();
		BA_MessageForSel("edited headroom of", edit.Selected);

		for (sel_iter_c it(edit.Selected); !it.done(); it.next())
		{
			int new_h = Sectors[*it]->floorh + room;

			new_h = CLAMP(-32767, new_h, 32767);

			BA_ChangeSEC(*it, Sector::F_CEILH, new_h);
		}

		BA_End();

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
	if (w == box->c_pic && Fl::event_button() == 2)
	{
		box->SetFlat(Misc_info.sky_flat, PART_CEIL);
		return;
	}

	// LMB on the flat image just selects/unselects it (red border)
	if (is_pic && Fl::event_button() != 3)
	{
		UI_Pic * pic = (UI_Pic *) w;

		pic->Selected(! pic->Selected());

		if (pic->Selected())
			main_win->BrowserMode('F');
		return;
	}

	const char *new_flat;

	// right click sets to default value
	// [ Note the 'is_pic' check prevents a bug when using RMB in browser ]
	if (is_pic && Fl::event_button() == 3)
		new_flat = is_floor ? default_floor_tex : default_ceil_tex;
	else if (is_floor)
		new_flat = NormalizeTex(box->f_tex->value());
	else
		new_flat = NormalizeTex(box->c_tex->value());

	box->InstallFlat(new_flat, is_floor ? PART_FLOOR : PART_CEIL);
}


void UI_SectorBox::InstallFlat(const char *name, int filter_parts)
{
	int tex_num = BA_InternaliseString(name);

	if (! edit.Selected->empty())
	{
		BA_Begin();
		BA_MessageForSel("edited texture on", edit.Selected);

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			int parts = edit.Selected->get_ext(*it);
			if (parts == 1)
				parts = filter_parts;

			if (parts & filter_parts & PART_FLOOR)
				BA_ChangeSEC(*it, Sector::F_FLOOR_TEX, tex_num);

			if (parts & filter_parts & parts & PART_CEIL)
				BA_ChangeSEC(*it, Sector::F_CEIL_TEX, tex_num);
		}

		BA_End();
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


void UI_SectorBox::SetFlat(const char *name, int parts)
{
	if (parts & PART_FLOOR)
		f_tex->value(name);

	if (parts & PART_CEIL)
		c_tex->value(name);

	InstallFlat(name, parts);
}


void UI_SectorBox::type_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int mask  = 65535;
	int value = atoi(box->type->value());

	int gen_mask = (Features.gen_sectors == 2) ? 255 : 31;

	// when generalize sectors active, typing a low value (which does
	// not touch any bitflags) should just update the TYPE part, and
	// leave the existing bitflags alone.

	if (w == box->type && value > gen_mask)
	{
		// the value is too large and can affect the bitflags, so we
		// update the WHOLE sector type.  If generalized sectors are
		// active, then the panel will reinterpret the typed value.
	}
	else if (Features.gen_sectors)
	{
		// Boom and ZDoom generalized sectors

		mask = 0;

		if (w == box->bm_damage)
		{
			mask  = BoomSF_DamageMask;
			value = box->bm_damage->value() << 5;
		}
		else if (w == box->bm_secret)
		{
			mask  = BoomSF_Secret;
			value = box->bm_secret->value() << 7;
		}
		else if (w == box->bm_friction)
		{
			mask  = BoomSF_Friction;
			value = box->bm_friction->value() << 8;
		}
		else if (w == box->bm_wind)
		{
			mask  = BoomSF_Wind;
			value = box->bm_wind->value() << 9;
		}

		if (mask == 0)
		{
			mask = gen_mask;
		}
		else if (Features.gen_sectors == 2)
		{
			// for ZDoom in Hexen mode, shift up 3 bits
			mask  <<= 3;
			value <<= 3;
		}
	}

	box->InstallSectorType(mask, value);
}


void UI_SectorBox::InstallSectorType(int mask, int value)
{
	value &= mask;

	if (! edit.Selected->empty())
	{
		BA_Begin();
		BA_MessageForSel("edited type of", edit.Selected);

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			int old_type = Sectors[*it]->type;

			BA_ChangeSEC(*it, Sector::F_TYPE, (old_type & ~mask) | value);
		}

		BA_End();
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

	if (Features.gen_sectors)
	{
		int gen_mask = (Features.gen_sectors == 2) ? 255 : 31;

		value &= gen_mask;
	}

	const sectortype_t *info = M_GetSectorType(value);

	box->desc->value(info->desc);
}


void UI_SectorBox::SetSectorType(int new_type)
{
	if (obj < 0)
		return;

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%d", new_type);

	type->value(buffer);

	int mask = (Features.gen_sectors == 2) ? 255 :
			   (Features.gen_sectors == 1) ? 31  : 65535;

	InstallSectorType(mask, new_type);
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

	if (! edit.Selected->empty())
	{
		BA_Begin();
		BA_MessageForSel("edited light of", edit.Selected);

		for (sel_iter_c it(edit.Selected); !it.done(); it.next())
		{
			BA_ChangeSEC(*it, Sector::F_LIGHT, new_lt);
		}

		BA_End();
	}
}


void UI_SectorBox::tag_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int new_tag = atoi(box->tag->value());

	new_tag = CLAMP(-32767, new_tag, 32767);

	if (! edit.Selected->empty())
		Tags_ApplyNewValue(new_tag);
}


void UI_SectorBox::FreshTag()
{
	int min_tag, max_tag;
	Tags_UsedRange(&min_tag, &max_tag);

	int new_tag = max_tag + 1;

	// skip some special tag numbers
	if (new_tag == 666)
		new_tag = 670;

	if (new_tag > 32767)
	{
		Beep("Out of tag numbers");
		return;
	}

	if (! edit.Selected->empty())
		Tags_ApplyNewValue(new_tag);
}


void UI_SectorBox::button_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	if (w == box->choose)
	{
		main_win->BrowserMode('S');
		return;
	}

	if (w == box->fresh_tag)
	{
		box->FreshTag();
		return;
	}

	keycode_t mod = Fl::event_state() & MOD_ALL_MASK;

	int lt_step = light_bump_medium;

	if (mod & MOD_SHIFT)
		lt_step = light_bump_small;
	else if (mod & MOD_COMMAND)
		lt_step = light_bump_large;

	if (w == box->lt_up)
	{
		SectorsAdjustLight(+lt_step);
		return;
	}
	else if (w == box->lt_down)
	{
		SectorsAdjustLight(-lt_step);
		return;
	}

	int mv_step = floor_bump_medium;

	if (mod & MOD_SHIFT)
		mv_step = floor_bump_small;
	else if (mod & MOD_COMMAND)
		mv_step = floor_bump_large;

	if (w == box->ce_up)
	{
		ExecuteCommand("SEC_Ceil", Int_TmpStr(+mv_step));
		return;
	}
	else if (w == box->ce_down)
	{
		ExecuteCommand("SEC_Ceil", Int_TmpStr(-mv_step));
		return;
	}

	if (w == box->fl_up)
	{
		ExecuteCommand("SEC_Floor", Int_TmpStr(+mv_step));
		return;
	}
	else if (w == box->fl_down)
	{
		ExecuteCommand("SEC_Floor", Int_TmpStr(-mv_step));
		return;
	}
}


//------------------------------------------------------------------------

void UI_SectorBox::SetObj(int _index, int _count)
{
	if (obj == _index && count == _count)
		return;

	obj   = _index;
	count = _count;

	which->SetIndex(obj);
	which->SetSelected(count);

	UpdateField();

	if (obj < 0)
		UnselectPics();

	redraw();
}

void UI_SectorBox::UpdateField(int field)
{
	if (field < 0 || field == Sector::F_FLOORH || field == Sector::F_CEILH)
	{
		if (is_sector(obj))
		{
			floor_h->value(Int_TmpStr(Sectors[obj]->floorh));
			ceil_h->value(Int_TmpStr(Sectors[obj]->ceilh));
			headroom->value(Int_TmpStr(Sectors[obj]->HeadRoom()));
		}
		else
		{
			floor_h->value("");
			ceil_h->value("");
			headroom->value("");
		}
	}

	if (field < 0 || field == Sector::F_FLOOR_TEX || field == Sector::F_CEIL_TEX)
	{
		if (is_sector(obj))
		{
			f_tex->value(Sectors[obj]->FloorTex());
			c_tex->value(Sectors[obj]->CeilTex());

			f_pic->GetFlat(Sectors[obj]->FloorTex());
			c_pic->GetFlat(Sectors[obj]->CeilTex());

			f_pic->AllowHighlight(true);
			c_pic->AllowHighlight(true);
		}
		else
		{
			f_tex->value("");
			c_tex->value("");

			f_pic->Clear();
			c_pic->Clear();

			f_pic->AllowHighlight(false);
			c_pic->AllowHighlight(false);
		}
	}

	if (field < 0 || field == Sector::F_TYPE)
	{
		bm_damage->value(0);
		bm_secret->value(0);
		bm_friction->value(0);
		bm_wind->value(0);

		if (is_sector(obj))
		{
			int value = Sectors[obj]->type;
			int mask  = (Features.gen_sectors == 2) ? 255 :
						(Features.gen_sectors) ? 31 : 65535;

			type->value(Int_TmpStr(value & mask));

			const sectortype_t *info = M_GetSectorType(value & mask);
			desc->value(info->desc);

			if (Features.gen_sectors)
			{
				if (Features.gen_sectors == 2)
					value >>= 3;

				bm_damage->value((value >> 5) & 3);
				bm_secret->value((value >> 7) & 1);

				bm_friction->value((value >> 8) & 1);
				bm_wind    ->value((value >> 9) & 1);
			}
		}
		else
		{
			type->value("");
			desc->value("");
		}
	}

	if (field < 0 || field == Sector::F_LIGHT || field == Sector::F_TAG)
	{
		if (is_sector(obj))
		{
			light->value(Int_TmpStr(Sectors[obj]->light));
			tag->value(Int_TmpStr(Sectors[obj]->tag));
		}
		else
		{
			light->value("");
			tag->value("");
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


void UI_SectorBox::CB_Copy()
{
	if (GetSelectedPics() == (PART_FLOOR | PART_CEIL))
	{
		Beep("multiple textures");
		return;
	}

	const char *name = NULL;

	if (GetSelectedPics() == PART_CEIL ||
		(GetSelectedPics() == 0 && c_pic->Highlighted()))
	{
		name = c_tex->value();
	}
	else
	{
		name = f_tex->value();
	}

	Texboard_SetFlat(name);

	Status_Set("copied %s", name);
}


void UI_SectorBox::CB_Paste(int new_tex)
{
	int parts = GetSelectedPics();

	if (parts == 0)
		parts = GetHighlightedPics();

	if (! edit.Selected->empty())
	{
		BA_Begin();
		BA_Message("pasted %s", BA_GetString(new_tex));

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			if (parts & PART_FLOOR) BA_ChangeSEC(*it, Sector::F_FLOOR_TEX, new_tex);
			if (parts & PART_CEIL)  BA_ChangeSEC(*it, Sector::F_CEIL_TEX,  new_tex);
		}

		BA_End();

		UpdateField();
	}
}


void UI_SectorBox::CB_Cut()
{
	int parts = GetSelectedPics();

	if (parts == 0)
		parts = GetHighlightedPics();

	int new_floor = BA_InternaliseString(default_floor_tex);
	int new_ceil  = BA_InternaliseString(default_ceil_tex);

	if (! edit.Selected->empty())
	{
		BA_Begin();
		BA_MessageForSel("cut texture on", edit.Selected);

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			if (parts & PART_FLOOR) BA_ChangeSEC(*it, Sector::F_FLOOR_TEX, new_floor);
			if (parts & PART_CEIL)  BA_ChangeSEC(*it, Sector::F_CEIL_TEX,  new_ceil);
		}

		BA_End();

		UpdateField();
	}
}


bool UI_SectorBox::ClipboardOp(char op)
{
	if (obj < 0)
		return false;

	if (!  (f_pic->Selected() || f_pic->Highlighted() ||
			c_pic->Selected() || c_pic->Highlighted()))
	{
		return false;
	}

	switch (op)
	{
		case 'c':
			CB_Copy();
			break;

		case 'v':
			CB_Paste(Texboard_GetFlatNum());
			break;

		case 'x':
			CB_Cut();
			break;

		case 'd':
			// we abuse the delete function to turn sector ceilings into sky
			CB_Paste(BA_InternaliseString(Misc_info.sky_flat));
			break;
	}

	return true;
}


void UI_SectorBox::BrowsedItem(char kind, int number, const char *name, int e_state)
{
	if (obj < 0)
		return;

	if (kind == 'F' || kind == 'T')
	{
		int parts = GetSelectedPics();

		if (parts == 0)
		{
			if (edit.render3d)
				parts = PART_FLOOR | PART_CEIL;
			else if (edit.sector_render_mode == SREND_Ceiling)
				parts = PART_CEIL;
			else if (e_state & FL_BUTTON3)
				parts = PART_CEIL;
			else
				parts = PART_FLOOR;
		}

		SetFlat(name, parts);
	}
	else if (kind == 'S')
	{
		SetSectorType(number);
	}
}


void UI_SectorBox::UnselectPics()
{
	f_pic->Selected(false);
	c_pic->Selected(false);
}


// FIXME: make a method of Sector class
void UI_SectorBox::UpdateTotal()
{
	which->SetTotal(NumSectors);
}


void UI_SectorBox::UpdateGameInfo()
{
	if (Features.gen_sectors)
	{
		if (Features.gen_sectors == 2)
			bm_title->label("ZDoom flags:");
		else
			bm_title->label("Boom flags:");

		bm_title->show();

		bm_damage->show();
		bm_secret->show();
		bm_friction->show();
		bm_wind->show();
	}
	else
	{
		bm_title->hide();

		bm_damage->hide();
		bm_secret->hide();
		bm_friction->hide();
		bm_wind->hide();
	}

	UpdateField();

	redraw();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
