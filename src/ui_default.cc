//------------------------------------------------------------------------
//  Default Properties
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2015 Andrew Apted
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
#include "m_game.h"
#include "w_rawdef.h"


UI_DefaultProps::UI_DefaultProps(int X, int Y, int W, int H) :
	Fl_Group(X, Y, W, H, NULL)
{
	box(FL_FLAT_BOX);


	Fl_Box *title = new Fl_Box(X + 50, Y + 10, W - 60, 30, "Default Properties");
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	title->labelsize(18+KF*4);

	Y += 50;
	H -= 50;

	X += 6;
	W -= 12;

	int MX = X + W/2;


	// ---- LINEDEF TEXTURES ------------

	Fl_Box *lower_tit = new Fl_Box(X + 5,      Y, 70, 30, "Lower");
	Fl_Box *  mid_tit = new Fl_Box(MX - 35,    Y, 70, 30, "Mid (1S)");
	Fl_Box *upper_tit = new Fl_Box(X + W - 75, Y, 70, 30, "Upper");

	lower_tit->labelcolor(fl_gray_ramp(2));
	  mid_tit->labelcolor(fl_gray_ramp(2));
	upper_tit->labelcolor(fl_gray_ramp(2));

	Y += 32;


	l_pic = new UI_Pic(X+8,      Y, 64, 64);
	m_pic = new UI_Pic(MX-32,    Y, 64, 64);
	u_pic = new UI_Pic(X+W-64-8, Y, 64, 64);

	l_pic->callback(tex_callback, this);
	m_pic->callback(tex_callback, this);
	u_pic->callback(tex_callback, this);

	Y += 65;


	l_tex = new Fl_Input(X,      Y, 80, 20);
	m_tex = new Fl_Input(MX-40,  Y, 80, 20);
	u_tex = new Fl_Input(X+W-80, Y, 80, 20);

	l_tex->textsize(12);
	m_tex->textsize(12);
	u_tex->textsize(12);

	l_tex->callback(tex_callback, this);
	m_tex->callback(tex_callback, this);
	u_tex->callback(tex_callback, this);

	l_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	m_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	u_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += l_tex->h() + 34;


	// ---- SECTOR PROPS --------------

#if 0
	Fl_Box *sec_tit = new Fl_Box(X, Y, W, 30, "Sector props:");
	sec_tit->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	Y += sec_tit->h() + 2;
#endif

	c_pic = new UI_Pic(X+W-76, Y+2,  64, 64);
	f_pic = new UI_Pic(X+W-76, Y+78, 64, 64);

	c_pic->callback(flat_callback, this);
	f_pic->callback(flat_callback, this);


	c_tex = new Fl_Input(X+68, Y, 108, 24, "Ceiling: ");
	c_tex->align(FL_ALIGN_LEFT);
	c_tex->callback(flat_callback, this);
	c_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += c_tex->h() + 3;


	ceil_h = new Fl_Int_Input(X+68, Y, 64, 24, "");
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

	floor_h = new Fl_Int_Input(X+68, Y, 64, 24, "");
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


	f_tex = new Fl_Input(X+68, Y, 108, 24, "Floor:   ");
	f_tex->align(FL_ALIGN_LEFT);
	f_tex->callback(flat_callback, this);
	f_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += f_tex->h() + 8;


	light = new Fl_Int_Input(X+68, Y, 64, 24, "Light:   ");
	light->align(FL_ALIGN_LEFT);
	light->callback(height_callback, this);
	light->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y += light->h() + 40;


	// ---- THING PROPS --------------

	thing = new Fl_Int_Input(X+54, Y, 64, 24, "Thing: ");
	thing->align(FL_ALIGN_LEFT);
	thing->callback(thing_callback, this);
	thing->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	th_desc = new Fl_Output(thing->x() + thing->w() + 10, Y, 144, 24);


	resizable(NULL);

	end();
}


UI_DefaultProps::~UI_DefaultProps()
{ }


void UI_DefaultProps::SetIntVal(Fl_Int_Input *w, int value)
{
	char buffer[64];
	sprintf(buffer, "%d", value);
	w->value(buffer);
}

void UI_DefaultProps::UpdateThingDesc()
{
	const thingtype_t *info = M_GetThingType(default_thing);

	th_desc->value(info->desc);
}

void UI_DefaultProps::SetTexture(const char *name, int e_state)
{
	int sel_pics =	(l_pic->Selected() ? 1 : 0) |
					(m_pic->Selected() ? 2 : 0) |
					(u_pic->Selected() ? 4 : 0);

	if (sel_pics == 0)
	{
//??		bool low_mid_same = (strcmp(default_lower_tex, default_mid_tex) == 0);
//??		bool low_upp_same = (strcmp(default_lower_tex, default_upper_tex) == 0);

		if (e_state & FL_BUTTON2)
			sel_pics = 2;
		else if (e_state & FL_BUTTON3)
			sel_pics = 4;
		else
			sel_pics = 1;  //??  | (low_mid_same ? 2 : 0) | (low_upp_same ? 4 : 0);
	}

	if (sel_pics & 1)
	{
		l_tex->value(name);
		l_tex->do_callback();
	}
	if (sel_pics & 2)
	{
		m_tex->value(name);
		m_tex->do_callback();
	}
	if (sel_pics & 4)
	{
		u_tex->value(name);
		u_tex->do_callback();
	}
}

void UI_DefaultProps::SetFlat(const char *name, int e_state)
{
	// same logic as in UI_SectorBox::SetFlat()

	int sel_pics =	(f_pic->Selected() ? 1 : 0) |
					(c_pic->Selected() ? 2 : 0);

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

void UI_DefaultProps::SetThing(int number)
{
	default_thing = number;

	SetIntVal(thing, default_thing);

	UpdateThingDesc();
}

void UI_DefaultProps::UnselectPicSet(char what /* 'f' or 't' */)
{
	if (what == 'f')
	{
		f_pic->Selected(false);
		c_pic->Selected(false);
	}

	if (what == 't')
	{
		l_pic->Selected(false);
		m_pic->Selected(false);
		u_pic->Selected(false);
	}
}


const char * UI_DefaultProps::NormalizeTex_and_Dup(Fl_Input *w)
{
	char name[WAD_TEX_NAME + 1];

	memset(name, 0, sizeof(name));

	strncpy(name, w->value(), WAD_TEX_NAME);

	for (int i = 0 ; i < WAD_TEX_NAME ; i++)
		name[i] = toupper(name[i]);

	w->value(name);

	return StringDup(name);
}


void UI_DefaultProps::tex_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	if (w == box->l_pic ||
		w == box->m_pic ||
		w == box->u_pic)
	{
		UI_Pic * pic = (UI_Pic *) w;

		pic->Selected(! pic->Selected());
		pic->redraw();

		if (pic->Selected())
		{
			box->UnselectPicSet('f');
			main_win->ShowBrowser('T');
		}
		return;
	}

	if (w == box->l_tex)
		default_lower_tex = NormalizeTex_and_Dup(box->l_tex);

	if (w == box->m_tex)
		default_mid_tex = NormalizeTex_and_Dup(box->m_tex);

	if (w == box->u_tex)
		default_upper_tex = NormalizeTex_and_Dup(box->u_tex);

	box->l_pic->GetTex(box->l_tex->value());
	box->m_pic->GetTex(box->m_tex->value());
	box->u_pic->GetTex(box->u_tex->value());
}

void UI_DefaultProps::flat_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	if (w == box->f_pic ||
		w == box->c_pic)
	{
		UI_Pic * pic = (UI_Pic *) w;

		pic->Selected(! pic->Selected());
		pic->redraw();

		if (pic->Selected())
		{
			box->UnselectPicSet('t');
			main_win->ShowBrowser('F');
		}
		return;
	}

	if (w == box->f_tex)
		default_floor_tex = NormalizeTex_and_Dup(box->f_tex);

	if (w == box->c_tex)
		default_ceil_tex = NormalizeTex_and_Dup(box->c_tex);

	box->f_pic->GetFlat(box->f_tex->value());
	box->c_pic->GetFlat(box->c_tex->value());
}

void UI_DefaultProps::button_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	int diff = 8;
	if (Fl::event_shift())
		diff = 1;
	else if (Fl::event_ctrl())
		diff = 64;

	if (w == box->fl_up)
		default_floor_h += diff;

	if (w == box->fl_down)
		default_floor_h -= diff;

	if (w == box->ce_up)
		default_ceil_h += diff;

	if (w == box->ce_down)
		default_ceil_h -= diff;

	box->SetIntVal(box->floor_h, default_floor_h);
	box->SetIntVal(box-> ceil_h, default_ceil_h);
}

void UI_DefaultProps::height_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	default_floor_h = atoi(box->floor_h->value());
	default_ceil_h  = atoi(box-> ceil_h->value());
	default_light_level = atoi(box->light->value());
}

void UI_DefaultProps::thing_callback(Fl_Widget *w, void *data)
{
	UI_DefaultProps *box = (UI_DefaultProps *)data;

	default_thing = atoi(box->thing->value());

	box->UpdateThingDesc();
}

void UI_DefaultProps::LoadValues()
{
	l_tex->value(default_lower_tex);
	m_tex->value(default_mid_tex);
	u_tex->value(default_upper_tex);

	l_pic->GetTex(l_tex->value());
	m_pic->GetTex(m_tex->value());
	u_pic->GetTex(u_tex->value());

	f_tex->value(default_floor_tex);
	c_tex->value(default_ceil_tex);

	f_pic->GetFlat(f_tex->value());
	c_pic->GetFlat(c_tex->value());

	SetIntVal(floor_h, default_floor_h);
	SetIntVal( ceil_h, default_ceil_h);
	SetIntVal(  light, default_light_level);
	SetIntVal(  thing, default_thing);

	UpdateThingDesc();
}


void UI_DefaultProps::BrowsedItem(char kind, int number, const char *name, int e_state)
{
	if (! visible())
	{
		fl_beep();
		return;
	}

	switch (kind)
	{
		case 'T': SetTexture(name, e_state); break;
		case 'F': SetFlat   (name, e_state); break;
		case 'O': SetThing(number); break;

		default:
			fl_beep();
			break;
	}
}


void UI_DefaultProps::UnselectPics()
{
	UnselectPicSet('f');
	UnselectPicSet('t');
}


//------------------------------------------------------------------------


bool Props_ParseUser(const char ** tokens, int num_tok)
{
	// syntax is:  default  <prop>  <value>
	if (num_tok < 3)
		return false;

	if (strcmp(tokens[0], "default") != 0)
		return false;

	if (strcmp(tokens[1], "is_shown") == 0)
	{ /* ignored for backwards compat */ }

	if (strcmp(tokens[1], "floor_h") == 0)
		default_floor_h = atoi(tokens[2]);

	if (strcmp(tokens[1], "ceil_h") == 0)
		default_ceil_h = atoi(tokens[2]);

	if (strcmp(tokens[1], "light_level") == 0)
		default_light_level = atoi(tokens[2]);

	if (strcmp(tokens[1], "thing") == 0)
		default_thing = atoi(tokens[2]);

	if (strcmp(tokens[1], "floor_tex") == 0)
		default_floor_tex = StringDup(tokens[2]);

	if (strcmp(tokens[1], "ceil_tex") == 0)
		default_ceil_tex = StringDup(tokens[2]);

	if (strcmp(tokens[1], "lower_tex") == 0)
		default_lower_tex = StringDup(tokens[2]);

	if (strcmp(tokens[1], "mid_tex") == 0)
		default_mid_tex = StringDup(tokens[2]);

	if (strcmp(tokens[1], "upper_tex") == 0)
		default_upper_tex = StringDup(tokens[2]);

	return true;
}


void Props_WriteUser(FILE *fp)
{
	fprintf(fp, "\n");

	fprintf(fp, "default floor_h %d\n", default_floor_h);
	fprintf(fp, "default ceil_h %d\n",  default_ceil_h);
	fprintf(fp, "default light_level %d\n",  default_light_level);
	fprintf(fp, "default thing %d\n",  default_thing);

	fprintf(fp, "default floor_tex \"%s\"\n", StringTidy(default_floor_tex, "\""));
	fprintf(fp, "default ceil_tex \"%s\"\n",  StringTidy(default_ceil_tex,  "\""));
	fprintf(fp, "default lower_tex \"%s\"\n", StringTidy(default_lower_tex, "\""));
	fprintf(fp, "default mid_tex \"%s\"\n",   StringTidy(default_mid_tex,   "\""));
	fprintf(fp, "default upper_tex \"%s\"\n", StringTidy(default_upper_tex, "\""));
}


void Props_LoadValues()
{
	if (main_win)
		main_win->props_box->LoadValues();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
