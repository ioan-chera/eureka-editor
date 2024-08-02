//------------------------------------------------------------------------
//  DEFAULT PROPERTIES
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

#include "Instance.h"
#include "main.h"
#include "ui_misc.h"
#include "ui_window.h"

#include "e_main.h"
#include "e_cutpaste.h"
#include "m_config.h"	// gui_scheme
#include "m_game.h"
#include "r_render.h"
#include "w_rawdef.h"
#include "w_texture.h"


#define HIDE_BG  (config::gui_scheme == 2 ? FL_DARK3 : FL_DARK1)


UI_DefaultProps::UI_DefaultProps(Instance &inst, int X, int Y, int W, int H) :
	Fl_Group(X, Y, W, H, NULL), inst(inst)
{
	box(FL_FLAT_BOX);


	Fl_Button *hide_button = new Fl_Button(X + 14, Y + 14, 22, 22, "X");
	hide_button->color(HIDE_BG, HIDE_BG);
	hide_button->labelsize(14);
	hide_button->callback(hide_callback, this);


	Fl_Box *title = new Fl_Box(X + 60, Y + 10, W - 70, 30, "Default Properties");
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	title->labelsize(22);

	Y += 35; H -= 35;
	X += 6;  W -= 12;


	// ---- LINEDEF TEXTURES ------------

	Y += 32;

	w_pic = new UI_Pic(inst, X+W-76,   Y, 64, 64);
	w_pic->callback(tex_callback, this);
	w_pic->AllowHighlight(true);

	Y += 20;

	w_tex = new UI_DynInput(X+68,   Y, 108, 24, "Wall: ");
	w_tex->callback(tex_callback, this);
	w_tex->callback2(dyntex_callback, this);
	w_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += w_tex->h() + 50;


	// ---- SECTOR PROPS --------------

	c_pic = new UI_Pic(inst, X+W-76, Y+2,  64, 64);
	f_pic = new UI_Pic(inst, X+W-76, Y+78, 64, 64);

	c_pic->callback(flat_callback, this);
	f_pic->callback(flat_callback, this);

	f_pic->AllowHighlight(true);
	c_pic->AllowHighlight(true);


	c_tex = new UI_DynInput(X+68, Y, 108, 24, "Ceiling: ");
	c_tex->align(FL_ALIGN_LEFT);
	c_tex->callback(flat_callback, this);
	c_tex->callback2(dyntex_callback, this);
	c_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += c_tex->h() + 3;


	ceil_h = new UI_DynIntInput(X+68, Y, 64, 24, "");
	ceil_h->align(FL_ALIGN_LEFT);
	ceil_h->callback(height_callback, this);
	ceil_h->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);



	ce_down = new Fl_Button(X+24,    Y+1, 30, 22, "-");
	ce_up   = new Fl_Button(X+68+68, Y+1, 30, 22, "+");

	ce_down->labelfont(FL_HELVETICA_BOLD);
	ce_up  ->labelfont(FL_HELVETICA_BOLD);
	ce_down->labelsize(16);
	ce_up  ->labelsize(16);

	ce_down->callback(button_callback, this);
	ce_up  ->callback(button_callback, this);


	Y += ceil_h->h() + 8;

	floor_h = new UI_DynIntInput(X+68, Y, 64, 24, "");
	floor_h->align(FL_ALIGN_LEFT);
	floor_h->callback(height_callback, this);
	floor_h->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	fl_down = new Fl_Button(X+24,    Y+1, 30, 22, "-");
	fl_up   = new Fl_Button(X+68+68, Y+1, 30, 22, "+");

	fl_down->labelfont(FL_HELVETICA_BOLD);
	fl_up  ->labelfont(FL_HELVETICA_BOLD);
	fl_down->labelsize(16);
	fl_up  ->labelsize(16);

	fl_down->callback(button_callback, this);
	fl_up  ->callback(button_callback, this);

	Y += floor_h->h() + 3;


	f_tex = new UI_DynInput(X+68, Y, 108, 24, "Floor:   ");
	f_tex->align(FL_ALIGN_LEFT);
	f_tex->callback(flat_callback, this);
	f_tex->callback2(dyntex_callback, this);
	f_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += f_tex->h() + 8;


	light = new UI_DynIntInput(X+68, Y, 64, 24, "Light:   ");
	light->align(FL_ALIGN_LEFT);
	light->callback(height_callback, this);
	light->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += light->h() + 40;


	// ---- THING PROPS --------------

	thing = new UI_DynInput(X+60, Y+20, 64, 24, "Thing: ");
	thing->align(FL_ALIGN_LEFT);
	thing->callback(thing_callback, this);
	thing->callback2(dynthing_callback, this);
	thing->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	thing->type(FL_INT_INPUT);

	th_desc = new Fl_Output(X+60, Y+80-26, 122, 24);

	th_sprite = new UI_Pic(inst, X+W-90, Y, 80,80, "Sprite");
	th_sprite->callback(thing_callback, this);


	resizable(NULL);

	mFixUp.loadFields({ w_tex, ceil_h, light, floor_h, c_tex, f_tex, thing });

	end();
}


UI_DefaultProps::~UI_DefaultProps()
{ }


void UI_DefaultProps::hide_callback(Fl_Widget *w, void *data)
{
	auto props = static_cast<UI_DefaultProps *>(data);
	props->inst.main_win->HideSpecialPanel();
}


void UI_DefaultProps::SetIntVal(Fl_Int_Input *w, int value)
{
	w->value(std::to_string(value).c_str());
}


void UI_DefaultProps::UpdateThingDesc()
{
	const thingtype_t &info = inst.conf.getThingType(inst.conf.default_thing);

	th_desc->value(info.desc.c_str());
	th_sprite->GetSprite(inst.conf.default_thing, FL_DARK2);
}


void UI_DefaultProps::SetThing(int number)
{
	inst.conf.default_thing = number;

	mFixUp.setInputValue(thing, SString(inst.conf.default_thing).c_str());

	UpdateThingDesc();
}


SString UI_DefaultProps::Normalize_and_Dup(UI_DynInput *w)
{
	SString normalized = NormalizeTex(w->value());

	mFixUp.setInputValue(w, normalized.c_str());

	return normalized;
}


void UI_DefaultProps::tex_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	if (w == box->w_pic)
	{
		UI_Pic * pic = (UI_Pic *) w;

		pic->Selected(! pic->Selected());

		if (pic->Selected())
			box->inst.main_win->BrowserMode(BrowserMode::textures);

		return;
	}

	if (w == box->w_tex)
	{
		box->inst.conf.default_wall_tex = box->Normalize_and_Dup(box->w_tex);
	}

	box->w_pic->GetTex(box->w_tex->value());
}


void UI_DefaultProps::flat_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	if (w == box->f_pic ||
		w == box->c_pic)
	{
		UI_Pic * pic = (UI_Pic *) w;

		pic->Selected(! pic->Selected());

		if (pic->Selected())
			box->inst.main_win->BrowserMode(BrowserMode::flats);

		return;
	}

	if (w == box->f_tex)
		box->inst.conf.default_floor_tex = box->Normalize_and_Dup(box->f_tex);

	if (w == box->c_tex)
		box->inst.conf.default_ceil_tex = box->Normalize_and_Dup(box->c_tex);

	box->f_pic->GetFlat(box->f_tex->value());
	box->c_pic->GetFlat(box->c_tex->value());
}


void UI_DefaultProps::dyntex_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	if (w == box->w_tex)
	{
		box->w_pic->GetTex(box->w_tex->value());
	}
	else if (w == box->f_tex)
	{
		box->f_pic->GetFlat(box->f_tex->value());
	}
	else if (w == box->c_tex)
	{
		box->c_pic->GetFlat(box->c_tex->value());
	}
}


void UI_DefaultProps::button_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	keycode_t mod = Fl::event_state() & EMOD_ALL_MASK;

	int diff = 8;

	if (mod & EMOD_SHIFT)
		diff = 1;
	else if (mod & EMOD_COMMAND)
		diff = 64;

	box->mFixUp.checkDirtyFields();

	if (w == box->fl_up)
		global::default_floor_h += diff;

	if (w == box->fl_down)
		global::default_floor_h -= diff;

	if (w == box->ce_up)
		global::default_ceil_h += diff;

	if (w == box->ce_down)
		global::default_ceil_h -= diff;

	box->SetIntVal(box->floor_h, global::default_floor_h);
	box->SetIntVal(box-> ceil_h, global::default_ceil_h);
}


void UI_DefaultProps::height_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	global::default_floor_h = atoi(box->floor_h->value());
	global::default_ceil_h  = atoi(box-> ceil_h->value());
	global::default_light_level = atoi(box->light->value());
}


void UI_DefaultProps::thing_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	if (w == box->th_sprite)
	{
		box->inst.main_win->BrowserMode(BrowserMode::things);
		return;
	}

	box->inst.conf.default_thing = atoi(box->thing->value());

	box->UpdateThingDesc();
}


void UI_DefaultProps::dynthing_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	int value = atoi(box->thing->value());

	const thingtype_t &info = box->inst.conf.getThingType(value);

	box->th_desc->value(info.desc.c_str());
	box->th_sprite->GetSprite(value, FL_DARK2);
}


void UI_DefaultProps::LoadValues()
{
	mFixUp.setInputValue(w_tex, inst.conf.default_wall_tex.c_str());
	mFixUp.setInputValue(f_tex, inst.conf.default_floor_tex.c_str());
	mFixUp.setInputValue(c_tex, inst.conf.default_ceil_tex.c_str());

	w_pic->GetTex (w_tex->value());
	f_pic->GetFlat(f_tex->value());
	c_pic->GetFlat(c_tex->value());

	SetIntVal(floor_h, global::default_floor_h);
	SetIntVal( ceil_h, global::default_ceil_h);
	SetIntVal(  light, global::default_light_level);

	mFixUp.setInputValue(thing, SString(inst.conf.default_thing).c_str());

	UpdateThingDesc();
}


void UI_DefaultProps::CB_Copy(int sel_pics)
{
	const char *name = NULL;

	switch (sel_pics)
	{
		case 1: name = f_tex->value(); break;
		case 2: name = c_tex->value(); break;
		case 4: name = w_tex->value(); break;

		default:
			inst.Beep("multiple textures");
			return;
	}

	if (sel_pics & 4)
		Texboard_SetTex(name, inst.conf);
	else
		Texboard_SetFlat(name, inst.conf);
}


void UI_DefaultProps::CB_Paste(int sel_pics)
{
	if (sel_pics & 1)
	{
		mFixUp.setInputValue(f_tex, 
							 BA_GetString(Texboard_GetFlatNum(inst.conf)).c_str());
		f_tex->do_callback();
	}

	if (sel_pics & 2)
	{
		mFixUp.setInputValue(c_tex, 
							 BA_GetString(Texboard_GetFlatNum(inst.conf)).c_str());
		c_tex->do_callback();
	}

	if (sel_pics & 4)
	{
		mFixUp.setInputValue(w_tex, 
							 BA_GetString(Texboard_GetTexNum(inst.conf)).c_str());
		w_tex->do_callback();
	}
}


void UI_DefaultProps::CB_Delete(int sel_pics)
{
	// we abuse the delete function to turn sector ceilings into sky

	if (sel_pics & 1)
	{
		mFixUp.setInputValue(f_tex, inst.conf.miscInfo.sky_flat.c_str());
		f_tex->do_callback();
	}

	if (sel_pics & 2)
	{
		mFixUp.setInputValue(c_tex, inst.conf.miscInfo.sky_flat.c_str());
		c_tex->do_callback();
	}
}

//
// Clipboard operation
//
bool UI_DefaultProps::ClipboardOp(EditCommand op)
{
	int sel_pics =	(f_pic->Selected() ? 1 : 0) |
					(c_pic->Selected() ? 2 : 0) |
					(w_pic->Selected() ? 4 : 0);

	if (sel_pics == 0)
	{
		sel_pics =	(f_pic->Highlighted() ? 1 : 0) |
					(c_pic->Highlighted() ? 2 : 0) |
					(w_pic->Highlighted() ? 4 : 0);
	}

	if(sel_pics == 0)
		return false;

	switch(op)
	{
		case EditCommand::copy:
			CB_Copy(sel_pics);
			break;

		case EditCommand::paste:
			CB_Paste(sel_pics);
			break;

		case EditCommand::cut:
			inst.Beep("cannot cut that");
			break;

		case EditCommand::del:
			CB_Delete(sel_pics);
			break;
	}

	return true;
}

void UI_DefaultProps::BrowsedItem(BrowserMode kind, int number, const char *name, int e_state)
{
	if (kind == BrowserMode::things)
	{
		SetThing(number);
		return;
	}

	if (! (kind == BrowserMode::textures || kind == BrowserMode::flats))
		return;

	int sel_pics =	(f_pic->Selected() ? 1 : 0) |
					(c_pic->Selected() ? 2 : 0) |
					(w_pic->Selected() ? 4 : 0);

	if (sel_pics == 0)
	{
		if (kind == BrowserMode::textures)
			sel_pics = 4;
		else
			sel_pics = (e_state & FL_BUTTON3) ? 2 : 1;
	}


	if (sel_pics & 1)
	{
		mFixUp.setInputValue(f_tex, name);
		f_tex->do_callback();
	}
	if (sel_pics & 2)
	{
		mFixUp.setInputValue(c_tex, name);
		c_tex->do_callback();
	}
	if (sel_pics & 4)
	{
		mFixUp.setInputValue(w_tex, name);
		w_tex->do_callback();
	}
}


void UI_DefaultProps::UnselectPics()
{
	w_pic->Unhighlight();
	f_pic->Unhighlight();
	c_pic->Unhighlight();

	w_pic->Selected(false);
	f_pic->Selected(false);
	c_pic->Selected(false);
}


//------------------------------------------------------------------------


bool Props_ParseUser(Instance &inst, const std::vector<SString> &tokens)
{
	// syntax is:  default  <prop>  <value>
	if (tokens.size() < 3)
		return false;

	if (tokens[0] != "default")
		return false;

	if (tokens[1] == "floor_h")
		global::default_floor_h = atoi(tokens[2]);

	if (tokens[1] == "ceil_h")
		global::default_ceil_h = atoi(tokens[2]);

	if (tokens[1] == "light_level")
		global::default_light_level = atoi(tokens[2]);

	if (tokens[1] == "thing")
		inst.conf.default_thing = atoi(tokens[2]);

	if (tokens[1] == "floor_tex")
		inst.conf.default_floor_tex = tokens[2];

	if (tokens[1] == "ceil_tex")
		inst.conf.default_ceil_tex = tokens[2];

	if (tokens[1] == "mid_tex")
		inst.conf.default_wall_tex = tokens[2];

	return true;
}

void Instance::Props_WriteUser(std::ostream &os) const
{
	os << '\n';

	os << "default floor_h " << global::default_floor_h << '\n';
	os << "default ceil_h " << global::default_ceil_h << '\n';
	os << "default light_level " << global::default_light_level << '\n';
	os << "default thing " << conf.default_thing << '\n';
	
	os << "default mid_tex \"" << conf.default_wall_tex.getTidy("\"") << "\"\n";
	os << "default floor_tex \"" << conf.default_floor_tex.getTidy("\"") << "\"\n";
	os << "default ceil_tex \"" << conf.default_ceil_tex.getTidy("\"") << "\"\n";
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
