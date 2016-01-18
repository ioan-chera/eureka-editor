//------------------------------------------------------------------------
//  SECTOR PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
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

#include "levels.h"
#include "e_checks.h"
#include "e_sector.h"
#include "e_things.h"
#include "m_game.h"
#include "w_rawdef.h"


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
	0, 72, 96, 128, 192, 256
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


	type = new Fl_Int_Input(X+70, Y, 64, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);
	type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	choose = new Fl_Button(X+W/2+24, Y, 80, 24, "Choose");
	choose->callback(button_callback, this);

	Y += type->h() + 4;


	desc = new Fl_Output(X+70, Y, W-78, 24, "Desc: ");
	desc->align(FL_ALIGN_LEFT);

	Y += desc->h() + 12;


	tag = new Fl_Int_Input(X+70, Y, 64, 24, "Tag: ");
	tag->align(FL_ALIGN_LEFT);
	tag->callback(tag_callback, this);
	tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

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


	c_tex = new Fl_Input(X+70, Y, 108, 24, "Ceiling: ");
	c_tex->align(FL_ALIGN_LEFT);
	c_tex->callback(tex_callback, this);
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


	f_tex = new Fl_Input(X+70, Y, 108, 24, "Floor:   ");
	f_tex->align(FL_ALIGN_LEFT);
	f_tex->callback(tex_callback, this);
	f_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += f_tex->h() + 28;


	headroom = new Fl_Int_Input(X+100, Y, 56, 24, "Headroom: ");
	headroom->align(FL_ALIGN_LEFT);
	headroom->callback(room_callback, this);
	headroom->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	for (int i = 0 ; i < HEADROOM_BUTTONS ; i++)
	{
		int hx = (i < 2) ? (X + 170 + i * 60) : (X + 50 + (i-2) * 60);
		int hy = Y + 28 * ((i < 2) ? 0 : 1);

		hd_buttons[i] = new Fl_Button(hx, hy+1, 45, 22);
		hd_buttons[i]->copy_label(Int_TmpStr(headroom_presets[i]));
		hd_buttons[i]->callback(room_callback, this);
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

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
		
		for (list.begin(&it); !it.at_end(); ++it)
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

void UI_SectorBox::room_callback(Fl_Widget *w, void *data)
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

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
		
		for (list.begin(&it); !it.at_end(); ++it)
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

	if (Fl::event_button() != 3 &&
		(w == box->f_pic || w == box->c_pic))
	{
		UI_Pic * pic = (UI_Pic *) w;

		pic->Selected(! pic->Selected());
		pic->redraw();

		if (pic->Selected())
			main_win->ShowBrowser('F');
		return;
	}

	int new_tex;

	// right click sets to default value
	if (Fl::event_button() == 3)
	{
		if (w == box->f_pic)
			new_tex = BA_InternaliseString(default_floor_tex);
		else
			new_tex = BA_InternaliseString(default_ceil_tex);
	}
	else
	{
		if (w == box->f_tex)
			new_tex = FlatFromWidget(box->f_tex);
		else
			new_tex = FlatFromWidget(box->c_tex);
	}

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it); !it.at_end(); ++it)
		{
			if (w == box->f_tex || w == box->f_pic)
				BA_ChangeSEC(*it, Sector::F_FLOOR_TEX, new_tex);
			else
				BA_ChangeSEC(*it, Sector::F_CEIL_TEX, new_tex);
		}

		BA_End();

		box->UpdateField();
	}
}


void UI_SectorBox::SetFlat(const char *name, int e_state)
{
	if (obj < 0)
		return;

	int sel_pics = GetSelectedPics();

	if (sel_pics == 0)
		sel_pics = (e_state & FL_BUTTON3) ? 2 : 1;

	if (sel_pics & 1)
	{
		f_tex->value(name);
		f_tex->do_callback();
	}

	if (sel_pics & 2)
	{
		c_tex->value(name);
		c_tex->do_callback();
	}
}


void UI_SectorBox::type_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int mask  = 65535;
	int value = atoi(box->type->value());

	if (game_info.gen_types)
	{
		// Boom generalized sectors

		if (w == box->bm_damage)
		{
			mask = 0x60;
			value = box->bm_damage->value() << 5;
		}
		else if (w == box->bm_secret)
		{
			mask = 0x80;
			value = box->bm_secret->value() << 7;
		}
		else if (w == box->bm_friction)
		{
			mask = 0x100;
			value = box->bm_friction->value() << 8;
		}
		else if (w == box->bm_wind)
		{
			mask = 0x200;
			value = box->bm_wind->value() << 9;
		}
		else
		{
			mask  = 31;
			// already got the value
		}
	}

	value &= mask;

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it) ; !it.at_end() ; ++it)
		{
			int old_type = Sectors[*it]->type;

			BA_ChangeSEC(*it, Sector::F_TYPE, (old_type & ~mask) | value);
		}

		BA_End();
	}

	// update the description
	const sectortype_t * info = M_GetSectorType(atoi(box->type->value()));
	box->desc->value(info->desc);
}


void UI_SectorBox::SetSectorType(int new_type)
{
	if (obj < 0)
		return;

	char buffer[64];

	sprintf(buffer, "%d", new_type);

	type->value(buffer);
	type->do_callback();
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

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it); !it.at_end(); ++it)
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

	Tags_ApplyNewValue(new_tag);
}


void UI_SectorBox::button_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	if (w == box->choose)
	{
		main_win->ShowBrowser('S');
		return;
	}

	int lt_step;

	if (Fl::event_shift())
		lt_step = light_bump_small;
	else if (Fl::event_ctrl())
		lt_step = light_bump_large;
	else
		lt_step = light_bump_medium;

	if (w == box->lt_up)
	{
		CMD_AdjustLight(+lt_step);
		return;
	}
	else if (w == box->lt_down)
	{
		CMD_AdjustLight(-lt_step);
		return;
	}

	int mv_step;

	if (Fl::event_shift())
		mv_step = floor_bump_small;
	else if (Fl::event_ctrl())
		mv_step = floor_bump_large;
	else
		mv_step = floor_bump_medium;

	if (w == box->ce_up)
	{
		EXEC_Param[0] = Int_TmpStr(+mv_step);
		SEC_Ceil();
		return;
	}
	else if (w == box->ce_down)
	{
		EXEC_Param[0] = Int_TmpStr(-mv_step);
		SEC_Ceil();
		return;
	}

	if (w == box->fl_up)
	{
		EXEC_Param[0] = Int_TmpStr(+mv_step);
		SEC_Floor();
		return;
	}
	else if (w == box->fl_down)
	{
		EXEC_Param[0] = Int_TmpStr(-mv_step);
		SEC_Floor();
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
		}
		else
		{
			f_tex->value("");
			c_tex->value("");

			f_pic->Clear();
			c_pic->Clear();
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
			int mask  = game_info.gen_types ? 31 : 65535;

			type->value(Int_TmpStr(value & mask));

			const sectortype_t *info = M_GetSectorType(value & mask);
			desc->value(info->desc);

			if (game_info.gen_types)
			{
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
	return	(f_pic->Selected() ? 1 : 0) |
			(c_pic->Selected() ? 2 : 0);
}


void UI_SectorBox::UnselectPics()
{
	f_pic->Selected(false);
	c_pic->Selected(false);
}


// FIXME: make a method of Sector class
void UI_SectorBox::AdjustHeight(s16_t *h, int delta)
{
	// prevent overflow
	if (delta > 0 && (int(*h) + delta) > 32767)
	{
		*h = 32767; return;
	}
	else if (delta < 0 && (int(*h) + delta) < -32767)
	{
		*h = -32767; return;
	}

	*h = *h + delta;
}


// FIXME: make a method of Sector class
void UI_SectorBox::AdjustLight(s16_t *L, int delta)
{
	if (abs(delta) > 16)
	{
		*L = (delta > 0) ? 255 : 0;
		return;
	}

	if (abs(delta) < 4)
		*L += delta;
	else
	{
		if (delta > 0)
			*L = (*L | 15) + 1;
		else
			*L = (*L - 1) & ~15;
	}

	if (*L < 0)
		*L = 0;
	else if (*L > 255)
		*L = 255;
}


int UI_SectorBox::FlatFromWidget(Fl_Input *w)
{
	char name[WAD_FLAT_NAME+1];

	memset(name, 0, sizeof(name));

	strncpy(name, w->value(), WAD_FLAT_NAME);

	for (int i = 0 ; i < WAD_FLAT_NAME ; i++)
		name[i] = toupper(name[i]);

	return BA_InternaliseString(name);
}


void UI_SectorBox::UpdateTotal()
{
	which->SetTotal(NumSectors);
}


void UI_SectorBox::UpdateGameInfo()
{
	if (game_info.gen_types)
	{
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
