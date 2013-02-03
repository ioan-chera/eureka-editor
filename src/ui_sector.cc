//------------------------------------------------------------------------
//  Sector Information Panel
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2012 Andrew Apted
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
#include "e_sector.h"
#include "e_things.h"
#include "m_game.h"
#include "w_rawdef.h"


//
// UI_SectorBox Constructor
//
UI_SectorBox::UI_SectorBox(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    obj(-1), count(0)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);


	X += 6;
	Y += 5;

	W -= 12;
	H -= 10;


	which = new UI_Nombre(X, Y, W-10, 28, "Sector");

	add(which);

	Y += which->h() + 4;


	type = new Fl_Int_Input(X+58, Y, 64, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);
	type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(type);

	choose = new Fl_Button(X+W/2+24, Y, 80, 24, "Choose");
	choose->callback(button_callback, this);

	add(choose);

	Y += type->h() + 3;


	desc = new Fl_Output(X+58, Y, W-64, 24, "Desc: ");
	desc->align(FL_ALIGN_LEFT);

	add(desc);

	Y += desc->h() + 3;


	tag = new Fl_Int_Input(X+58, Y, 64, 24, "Tag: ");
	tag->align(FL_ALIGN_LEFT);
	tag->callback(tag_callback, this);
	tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(tag);

	Y += tag->h() + 3;


	light = new Fl_Int_Input(X+58, Y, 64, 24, "Light: ");
	light->align(FL_ALIGN_LEFT);
	light->callback(light_callback, this);
	light->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(light);

	lt_down = new Fl_Button(X+W/2+20, Y+1, 30, 22, "-");
	lt_up   = new Fl_Button(X+W/2+60, Y+1, 30, 22, "+");

	lt_down->labelfont(FL_HELVETICA_BOLD);
	lt_up  ->labelfont(FL_HELVETICA_BOLD);
	lt_down->labelsize(16);
	lt_up  ->labelsize(16);

	lt_down->callback(button_callback, this);
	lt_up  ->callback(button_callback, this);

	add(lt_down); add(lt_up);


	Y += light->h() + 3;

	Y += 14;


	c_pic = new UI_Pic(X+W-76, Y+2,  64, 64);
	f_pic = new UI_Pic(X+W-76, Y+78, 64, 64);

	c_pic->callback(tex_callback, this);
	f_pic->callback(tex_callback, this);

	add(f_pic);
	add(c_pic);


	c_tex = new Fl_Input(X+68, Y, 108, 24, "Ceiling: ");
	c_tex->align(FL_ALIGN_LEFT);
	c_tex->callback(tex_callback, this);
	c_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(c_tex);

	Y += c_tex->h() + 3;


	ceil_h = new Fl_Int_Input(X+68, Y, 64, 24, "");
	ceil_h->align(FL_ALIGN_LEFT);
	ceil_h->callback(height_callback, this);
	ceil_h->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(ceil_h);


	ce_down = new Fl_Button(X+24,    Y+1, 30, 22, "-");
	ce_up   = new Fl_Button(X+68+68, Y+1, 30, 22, "+");

	ce_down->labelfont(FL_HELVETICA_BOLD);
	ce_up  ->labelfont(FL_HELVETICA_BOLD);
	ce_down->labelsize(16);
	ce_up  ->labelsize(16);

	ce_down->callback(button_callback, this);
	ce_up  ->callback(button_callback, this);

	add(ce_down); add(ce_up);


	Y += ceil_h->h() + 3;


	Y += 5;

	headroom = new Fl_Int_Input(X+100, Y, 56, 24, "Headroom: ");
	headroom->align(FL_ALIGN_LEFT);
	headroom->callback(room_callback, this);
	headroom->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(headroom);

	Y += headroom->h() + 3;

	Y += 5;


	floor_h = new Fl_Int_Input(X+68, Y, 64, 24, "");
	floor_h->align(FL_ALIGN_LEFT);
	floor_h->callback(height_callback, this);
	floor_h->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(floor_h);


	fl_down = new Fl_Button(X+24,    Y+1, 30, 22, "-");
	fl_up   = new Fl_Button(X+68+68, Y+1, 30, 22, "+");

	fl_down->labelfont(FL_HELVETICA_BOLD);
	fl_up  ->labelfont(FL_HELVETICA_BOLD);
	fl_down->labelsize(16);
	fl_up  ->labelsize(16);

	fl_down->callback(button_callback, this);
	fl_up  ->callback(button_callback, this);

	add(fl_down); add(fl_up);


	Y += floor_h->h() + 3;


	f_tex = new Fl_Input(X+68, Y, 108, 24, "Floor:   ");
	f_tex->align(FL_ALIGN_LEFT);
	f_tex->callback(tex_callback, this);
	f_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(f_tex);

	Y += f_tex->h() + 3;


	Y += 29;

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
		MarkChanges();

		box-> floor_h->value(Int_TmpStr(f_h));
		box->  ceil_h->value(Int_TmpStr(c_h));
		box->headroom->value(Int_TmpStr(c_h - f_h));
	}
}

void UI_SectorBox::room_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int room = atoi(box->headroom->value());

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
		MarkChanges();

		box->UpdateField();
	}
}


void UI_SectorBox::tex_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	if (box->obj < 0)
		return;

	if (w == box->f_pic || w == box->c_pic)
	{
		UI_Pic * pic = (UI_Pic *) w;

		pic->Selected(! pic->Selected());
		pic->redraw();

		if (pic->Selected())
			main_win->ShowBrowser('F');
		return;
	}

	int new_tex;
	if (w == box->f_tex)
		new_tex = box->FlatFromWidget(box->f_tex);
	else
		new_tex = box->FlatFromWidget(box->c_tex);

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it); !it.at_end(); ++it)
		{
			if (w == box->f_tex)
				BA_ChangeSEC(*it, Sector::F_FLOOR_TEX, new_tex);
			else
				BA_ChangeSEC(*it, Sector::F_CEIL_TEX, new_tex);
		}

		BA_End();
		MarkChanges();

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

	int new_type = atoi(box->type->value());

	const sectortype_t * info = M_GetSectorType(new_type);
	box->desc->value(info->desc);

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it) ; !it.at_end() ; ++it)
		{
			BA_ChangeSEC(*it, Sector::F_TYPE, new_type);
		}

		BA_End();
		MarkChanges();
	}
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
		MarkChanges();
	}
}


void UI_SectorBox::tag_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	int new_tag = atoi(box->tag->value());

	new_tag = CLAMP(-32767, new_tag, 32767);

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it); !it.at_end(); ++it)
		{
			BA_ChangeSEC(*it, Sector::F_TAG, new_tag);
		}

		BA_End();
		MarkChanges();
	}
}


void UI_SectorBox::button_callback(Fl_Widget *w, void *data)
{
	UI_SectorBox *box = (UI_SectorBox *)data;

	if (w == box->choose)
	{
		main_win->ShowBrowser('S');
		return;
	}


	int diff = 8;
	if (Fl::event_shift())
		diff = 1;
	else if (Fl::event_ctrl())
		diff = 64;

	if (w == box->lt_up)
	{
		CMD_AdjustLight(+diff);
		return;
	}
	else if (w == box->lt_down)
	{
		CMD_AdjustLight(-diff);
		return;
	}

	if (w == box->ce_up)
	{
		EXEC_Param[0] = Int_TmpStr(+diff);
		SEC_Ceil();
		return;
	}
	else if (w == box->ce_down)
	{
		EXEC_Param[0] = Int_TmpStr(-diff);
		SEC_Ceil();
		return;
	}

	if (w == box->fl_up)
	{
		EXEC_Param[0] = Int_TmpStr(+diff);
		SEC_Floor();
		return;
	}
	else if (w == box->fl_down)
	{
		EXEC_Param[0] = Int_TmpStr(-diff);
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
		if (is_sector(obj))
		{
			const sectortype_t *info = M_GetSectorType(Sectors[obj]->type);

			type->value(Int_TmpStr(Sectors[obj]->type));
			desc->value(info->desc);
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

//??	if (name[0] == 0)
//??		strcpy(name, "-");

	return BA_InternaliseString(name);
}


void UI_SectorBox::UpdateTotal()
{
	which->SetTotal(NumSectors);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
