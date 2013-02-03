//------------------------------------------------------------------------
//  Thing Panel
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2013 Andrew Apted
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

#include "editloop.h"
#include "levels.h"
#include "m_game.h"
#include "w_rawdef.h"


class thing_opt_CB_data_c
{
public:
	UI_ThingBox *parent;

	int mask;

public:
	thing_opt_CB_data_c(UI_ThingBox *_parent, int _mask) :
		parent(_parent), mask(_mask)
	{ }

	~thing_opt_CB_data_c()
	{ }
};


extern const char * arrow_0_xpm[];
extern const char * arrow_45_xpm[];
extern const char * arrow_90_xpm[];
extern const char * arrow_135_xpm[];
extern const char * arrow_180_xpm[];
extern const char * arrow_225_xpm[];
extern const char * arrow_270_xpm[];
extern const char * arrow_315_xpm[];

static const char ** arrow_pixmaps[8] =
{
	arrow_0_xpm,   arrow_45_xpm,
	arrow_90_xpm,  arrow_135_xpm,
	arrow_180_xpm, arrow_225_xpm,
	arrow_270_xpm, arrow_315_xpm
};


//
// UI_ThingBox Constructor
//
UI_ThingBox::UI_ThingBox(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    obj(-1), count(0)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX);

	X += 6;
	Y += 5;

	W -= 12;
	H -= 10;

	int MX = X + W/2;


	which = new UI_Nombre(X, Y, W-10, 28, "Thing");

	add(which);

	Y += which->h() + 4;


	type = new Fl_Int_Input(X+70, Y, 64, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);
	type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(type);

	choose = new Fl_Button(X+W/2+24, Y, 80, 24, "Choose");
	choose->callback(button_callback, this);

	add(choose);


	Y += type->h() + 3;

	desc = new Fl_Output(X+70, Y, W-76, 24, "Desc: ");
	desc->align(FL_ALIGN_LEFT);

	add(desc);

	Y += desc->h() + 3;


	pos_x = new Fl_Int_Input(X +70, Y, 70, 24, "x: ");
	pos_y = new Fl_Int_Input(MX+38, Y, 70, 24, "y: ");

	pos_x->align(FL_ALIGN_LEFT);
	pos_y->align(FL_ALIGN_LEFT);

	pos_x->callback(x_callback, this);
	pos_y->callback(y_callback, this);

	pos_x->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	pos_y->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(pos_x); add(pos_y);

	Y += pos_x->h() + 4;



	angle = new Fl_Int_Input(X+70, Y, 64, 24, "Angle: ");
	angle->align(FL_ALIGN_LEFT);
	angle->callback(angle_callback, this);
	angle->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(angle);


	int ang_mx = X + W - 70;
	int ang_my = Y + 40;

	for (int i = 0 ; i < 8 ; i++)
	{
		int x = ang_mx + 30 * cos(i * 45 * M_PI / 180.0);
		int y = ang_my - 30 * sin(i * 45 * M_PI / 180.0);

		ang_buts[i] = new Fl_Button(x - 9, y - 9, 20, 20, 0);

		ang_buts[i]->image(new Fl_Pixmap(arrow_pixmaps[i]));
		ang_buts[i]->align(FL_ALIGN_CENTER);
		ang_buts[i]->clear_visible_focus();
     	ang_buts[i]->callback(button_callback, this);

		add(ang_buts[i]);
	}


	Y += 30;

	exfloor = new Fl_Int_Input(X+70, Y, 64, 24, "ExFloor: ");
	exfloor->align(FL_ALIGN_LEFT);
	exfloor->callback(option_callback, new thing_opt_CB_data_c(this, MTF_EXFLOOR_MASK));
	exfloor->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(exfloor);

	efl_down = new Fl_Button(X+W-100, Y+1, 30, 22, "-");
	efl_up   = new Fl_Button(X+W- 60, Y+1, 30, 22, "+");

	efl_down->labelfont(FL_HELVETICA_BOLD);
	efl_up  ->labelfont(FL_HELVETICA_BOLD);
	efl_down->labelsize(16);
	efl_up  ->labelsize(16);

	efl_down->callback(button_callback, this);
	efl_up  ->callback(button_callback, this);

	add(efl_down); add(efl_up);

#if 0	
	Y += exfloor->h() + 10;
#else
	exfloor->hide();
	efl_down->hide();
	efl_up->hide();
#endif


	Y += 40;

	Fl_Box *opt_lab = new Fl_Box(X, Y, W, 22, "Options:");
	opt_lab->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

	add(opt_lab);

	Y += opt_lab->h() + 2;


	// when appear: two rows of three on/off buttons
	int AX = X+W/3+4;
	int BX = X+2*W/3-16;
	int FW = W/3 - 12;

	int AY = Y+22;
	int BY = Y+22*2;

	o_easy   = new Fl_Check_Button( X+12,  Y, FW, 22, "easy");
	o_medium = new Fl_Check_Button( X+12, AY, FW, 22, "medium");
	o_hard   = new Fl_Check_Button( X+12, BY, FW, 22, "hard");

	o_sp     = new Fl_Check_Button(AX+12,  Y, FW, 22, "sp");
	o_coop   = new Fl_Check_Button(AX+12, AY, FW, 22, "coop");
	o_dm     = new Fl_Check_Button(AX+12, BY, FW, 22, "dm");

	o_easy  ->value(1);  o_sp  ->value(1);
	o_medium->value(1);  o_coop->value(1);
	o_hard  ->value(1);  o_dm  ->value(1);

#if 0
	o_easy  ->labelsize(12);  o_sp  ->labelsize(12);
	o_medium->labelsize(12);  o_coop->labelsize(12);
	o_hard  ->labelsize(12);  o_dm  ->labelsize(12);
#endif

	o_easy  ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Easy));
	o_medium->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Medium));
	o_hard  ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Hard));

	o_sp    ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Not_SP));
	o_coop  ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Not_COOP));
	o_dm    ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Not_DM));

	add(o_easy); add(o_medium); add(o_hard);
	add(o_sp);   add(o_coop);   add(o_dm);


	// Hexen class flags
	o_fighter = new Fl_Check_Button(BX+12,  Y, FW, 22, "fighter");
	o_cleric  = new Fl_Check_Button(BX+12, AY, FW, 22, "cleric");
	o_mage    = new Fl_Check_Button(BX+12, BY, FW, 22, "mage");

	add(o_fighter); add(o_cleric); add(o_mage);

	o_fighter->hide();
	o_cleric->hide();
	o_mage->hide();


	Y = BY + 35;


	o_ambush = new Fl_Check_Button( X+12, Y, FW, 22, "ambush");
	o_friend = new Fl_Check_Button(AX+12, Y, FW, 22, "friend");

	o_ambush->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Ambush));
	o_friend->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Friend));

	add(o_ambush); add(o_friend);

	Y += 40;


	sprite = new UI_Pic(X + (W-100)/2, Y, 100,100);
	sprite->callback(button_callback, this);

	add(sprite);


	resizable(NULL);
}

//
// UI_ThingBox Destructor
//
UI_ThingBox::~UI_ThingBox()
{
}


void UI_ThingBox::SetObj(int _index, int _count)
{
	if (obj == _index && count == _count)
		return;

	obj   = _index;
	count = _count;

	which->SetIndex(obj);
	which->SetSelected(count);

	UpdateField();

	redraw();
}


void UI_ThingBox::type_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_type = atoi(box->type->value());

	const thingtype_t *info = M_GetThingType(new_type);

	box->desc->value(info->desc);
	box->sprite->GetSprite(new_type);

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
	
		for (list.begin(&it) ; !it.at_end() ; ++it)
		{
			BA_ChangeTH(*it, Thing::F_TYPE, new_type);
		}

		BA_End();

		MarkChanges();
	}
}


void UI_ThingBox::SetThingType(int new_type)
{
	if (obj < 0)
		return;

	char buffer[64];

	sprintf(buffer, "%d", new_type);

	type->value(buffer);
	type->do_callback();
}


void UI_ThingBox::angle_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_ang = atoi(box->angle->value());

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
	
		for (list.begin(&it); !it.at_end(); ++it)
		{
			BA_ChangeTH(*it, Thing::F_ANGLE, new_ang);
		}

		BA_End();

		MarkChanges();
	}
}


void UI_ThingBox::x_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_x = atoi(box->pos_x->value());

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
		
		for (list.begin(&it); !it.at_end(); ++it)
			BA_ChangeTH(*it, Thing::F_X, new_x);

		BA_End();
		MarkChanges();
	}
}

void UI_ThingBox::y_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_y = atoi(box->pos_y->value());

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
	
		for (list.begin(&it); !it.at_end(); ++it)
			BA_ChangeTH(*it, Thing::F_Y, new_y);

		BA_End();
		MarkChanges();
	}
}


void UI_ThingBox::option_callback(Fl_Widget *w, void *data)
{
	thing_opt_CB_data_c *o_c_b = (thing_opt_CB_data_c *)data;

	UI_ThingBox *box = o_c_b->parent;

	int mask = o_c_b->mask;
	int new_opts = box->CalcOptions();

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
	
		for (list.begin(&it); !it.at_end(); ++it)
		{
			const Thing *T = Things[*it];

			// only change the bits specified in 'mask'.
			// this is important when multiple things are selected.
			BA_ChangeTH(*it, Thing::F_OPTIONS, (T->options & ~mask) | (new_opts & mask));
		}

		BA_End();

		MarkChanges();
	}
}


void UI_ThingBox::button_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	if (w == box->efl_down)
		box->AdjustExtraFloor(-1);

	if (w == box->efl_up)
		box->AdjustExtraFloor(+1);

	if (w == box->choose || w == box->sprite)
		main_win->ShowBrowser('O');

	// check for the angle buttons
	for (int i = 0 ; i < 8 ; i++)
	{
		if (w == box->ang_buts[i])
		{
			char buffer[20];

			sprintf(buffer, "%d", i * 45);

			box->angle->value(buffer);

			angle_callback(box->angle, box);
		}
	}
}


void UI_ThingBox::AdjustExtraFloor(int dir)
{
	if (! is_thing(obj))
		return;

	int old_fl = atoi(exfloor->value());
	int new_fl = (old_fl + dir) & 15;

	if (new_fl)
		exfloor->value(Int_TmpStr(new_fl));
	else
		exfloor->value("");

	thing_opt_CB_data_c o_c_b(this, MTF_EXFLOOR_MASK);

	option_callback(this, &o_c_b);
}


void UI_ThingBox::OptionsFromInt(int options)
{
	o_easy  ->value((options & MTF_Easy)   ? 1 : 0);
	o_medium->value((options & MTF_Medium) ? 1 : 0);
	o_hard  ->value((options & MTF_Hard)   ? 1 : 0);

	o_sp  ->value((options & MTF_Not_SP)   ? 0 : 1);
	o_coop->value((options & MTF_Not_COOP) ? 0 : 1);
	o_dm  ->value((options & MTF_Not_DM)   ? 0 : 1);

	o_ambush->value((options & MTF_Ambush) ? 1 : 0);
	o_friend->value((options & MTF_Friend) ? 1 : 0);

	if (options & MTF_EXFLOOR_MASK)
		exfloor->value(Int_TmpStr((options & MTF_EXFLOOR_MASK) >> MTF_EXFLOOR_SHIFT));
	else
		exfloor->value("");
}


int UI_ThingBox::CalcOptions() const
{
	int options = 0;

	if (o_easy  ->value()) options |= MTF_Easy;
	if (o_medium->value()) options |= MTF_Medium;
	if (o_hard  ->value()) options |= MTF_Hard;

	if (0 == o_sp  ->value()) options |= MTF_Not_SP;
	if (0 == o_coop->value()) options |= MTF_Not_COOP;
	if (0 == o_dm  ->value()) options |= MTF_Not_DM;

	if (o_ambush->value()) options |= MTF_Ambush;
	if (o_friend->value()) options |= MTF_Friend;

	int exfl_num = atoi(exfloor->value());

	if (exfl_num > 0)
	{
		options |= (exfl_num << MTF_EXFLOOR_SHIFT) & MTF_EXFLOOR_MASK;
	}

	return options;
}


void UI_ThingBox::UpdateField(int field)
{
	if (field < 0 || field == Thing::F_X || field == Thing::F_Y)
	{
		if (is_thing(obj))
		{
			pos_x->value(Int_TmpStr(Things[obj]->x));
			pos_y->value(Int_TmpStr(Things[obj]->y));
		}
		else
		{
			pos_x->value("");
			pos_y->value("");
		}
	}

	if (field < 0 || field == Thing::F_ANGLE)
	{
		if (is_thing(obj))
			angle->value(Int_TmpStr(Things[obj]->angle));
		else
			angle->value("");
	}

	if (field < 0 || field == Thing::F_TYPE)
	{
		if (is_thing(obj))
		{
			const thingtype_t *info = M_GetThingType(Things[obj]->type);
			desc->value(info->desc);
			type->value(Int_TmpStr(Things[obj]->type));
			sprite->GetSprite(Things[obj]->type);
		}
		else
		{
			type ->value("");
			desc ->value("");
			sprite->Clear();
		}
	}

	if (field < 0 || field == Thing::F_OPTIONS)
	{
		if (is_thing(obj))
			OptionsFromInt(Things[obj]->options);
		else
			OptionsFromInt(0);
	}
}


void UI_ThingBox::UpdateTotal()
{
	which->SetTotal(NumThings);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
