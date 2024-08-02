//------------------------------------------------------------------------
//  THING PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2018 Andrew Apted
//  Copyright (C)      2015 Ioan Chera
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
#include "m_config.h"
#include "m_game.h"
#include "Thing.h"
#include "w_rawdef.h"

#include <assert.h>

class thing_opt_CB_data_c
{
public:
	UI_ThingBox *parent;

	int mask;

public:
	thing_opt_CB_data_c(UI_ThingBox *_parent, int _mask) :
		parent(_parent), mask(_mask)
	{ }

};


extern const char *const arrow_0_xpm[];
extern const char *const arrow_45_xpm[];
extern const char *const arrow_90_xpm[];
extern const char *const arrow_135_xpm[];
extern const char *const arrow_180_xpm[];
extern const char *const arrow_225_xpm[];
extern const char *const arrow_270_xpm[];
extern const char *const arrow_315_xpm[];

extern const char *const *arrow_pixmaps[8];
const char *const *arrow_pixmaps[8] =
{
	arrow_0_xpm,   arrow_45_xpm,
	arrow_90_xpm,  arrow_135_xpm,
	arrow_180_xpm, arrow_225_xpm,
	arrow_270_xpm, arrow_315_xpm
};


//
// UI_ThingBox Constructor
//
UI_ThingBox::UI_ThingBox(Instance &inst, int X, int Y, int W, int H, const char *label) :
    MapItemBox(inst, X, Y, W, H, label)
{
	box(FL_FLAT_BOX);

	X += 6;
	Y += 6;

	W -= 12;
	H -= 10;


	which = new UI_Nombre(X+6, Y, W-12, 28, "Thing");

	Y = Y + which->h() + 4;


	type = new UI_DynInput(X+70, Y, 70, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);
	type->callback2(dyntype_callback, this);
	type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	type->type(FL_INT_INPUT);

	choose = new Fl_Button(X+W/2, Y, 80, 24, "Choose");
	choose->callback(button_callback, this);

	Y = Y + type->h() + 4;

	desc = new Fl_Output(X+70, Y, W-78, 24, "Desc: ");
	desc->align(FL_ALIGN_LEFT);

	Y = Y + desc->h() + 4;


	sprite = new UI_Pic(inst, X + W - 120, Y + 10, 100,100, "Sprite");
	sprite->callback(button_callback, this);


	Y = Y + 10;

	pos_x = new UI_DynIntInput(X+70, Y, 70, 24, "x: ");
	pos_y = new UI_DynIntInput(X+70, Y + 28, 70, 24, "y: ");
	pos_z = new UI_DynIntInput(X+70, Y + 28*2, 70, 24, "z: ");
	pos_z->hide();

	pos_x->align(FL_ALIGN_LEFT);
	pos_y->align(FL_ALIGN_LEFT);
	pos_z->align(FL_ALIGN_LEFT);

	pos_x->callback(x_callback, this);
	pos_y->callback(y_callback, this);
	pos_z->callback(z_callback, this);

	pos_x->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	pos_y->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	pos_z->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y = Y + 105;


	// IOANCH 9/2015: TID
	tid = new UI_DynIntInput(X+70, Y, 64, 24, "TID: ");
	tid->align(FL_ALIGN_LEFT);
	tid->callback(tid_callback, this);
	tid->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	Y = Y + tid->h() + 4;


	angle = new UI_DynIntInput(X+70, Y, 64, 24, "Angle: ");
	angle->align(FL_ALIGN_LEFT);
	angle->callback(angle_callback, this);
	angle->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	int ang_mx = X + W - 75;
	int ang_my = Y + 17;

	for (int i = 0 ; i < 8 ; i++)
	{
		int dist = (i == 2 || i == 6) ? 32 : 35;

		int x = static_cast<int>(ang_mx + dist * cos(i * 45 * M_PI / 180.0));
		int y = static_cast<int>(ang_my - dist * sin(i * 45 * M_PI / 180.0));

		ang_buts[i] = new Fl_Button(x - 9, y - 9, 24, 24, 0);

		ang_buts[i]->image(new Fl_Pixmap(arrow_pixmaps[i]));
		ang_buts[i]->align(FL_ALIGN_CENTER);
		ang_buts[i]->clear_visible_focus();
		ang_buts[i]->callback(button_callback, this);
	}

	Y = Y + 50;


	Fl_Box *opt_lab = new Fl_Box(X+10, Y, W, 22, "Options Flags:");
	opt_lab->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

	Y = Y + opt_lab->h() + 2;


	// when appear: two rows of three on/off buttons
	int AX = X+W/3+4;
	int BX = X+2*W/3-20;
	int FW = W/3 - 12;

	int AY = Y+22;
	int BY = Y+22*2;

	o_easy   = new Fl_Check_Button( X+28,  Y, FW, 22, "easy");
	o_medium = new Fl_Check_Button( X+28, AY, FW, 22, "medium");
	o_hard   = new Fl_Check_Button( X+28, BY, FW, 22, "hard");

	o_sp     = new Fl_Check_Button(AX+28,  Y, FW, 22, "sp");
	o_coop   = new Fl_Check_Button(AX+28, AY, FW, 22, "coop");
	o_dm     = new Fl_Check_Button(AX+28, BY, FW, 22, "dm");

	// this is shown only for Vanilla DOOM (instead of the above three).
	// it is also works differently, no negated like the above.
	o_vanilla_dm = new Fl_Check_Button(AX+28, BY, FW, 22, "dm");

	o_easy  ->value(1);  o_sp  ->value(1);
	o_medium->value(1);  o_coop->value(1);
	o_hard  ->value(1);  o_dm  ->value(1);

	o_vanilla_dm->value(0);

	o_easy  ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Easy));
	o_medium->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Medium));
	o_hard  ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Hard));

	o_sp    ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Not_SP));
	o_coop  ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Not_COOP));
	o_dm    ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Not_DM));

	o_vanilla_dm->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Not_SP));


	// Hexen class flags
	o_fight   = new Fl_Check_Button(BX+28,  Y, FW, 22, "Fighter");
	o_cleric  = new Fl_Check_Button(BX+28, AY, FW, 22, "Cleric");
	o_mage    = new Fl_Check_Button(BX+28, BY, FW, 22, "Mage");

	o_fight ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Hexen_Fighter));
	o_cleric->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Hexen_Cleric));
	o_mage  ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Hexen_Mage));

	o_fight ->hide();
	o_cleric->hide();
	o_mage  ->hide();


	// Strife flags
	o_sf_shadow  = new Fl_Check_Button(BX+28, AY, FW, 22, "shadow");
	o_sf_altvis  = new Fl_Check_Button(BX+28, BY, FW, 22, "alt-vis");

	o_sf_shadow->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Strife_Shadow));
	o_sf_altvis->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Strife_AltVis));

	o_sf_shadow->hide();
	o_sf_altvis->hide();


	Y = BY + 35;


	o_ambush  = new Fl_Check_Button( X+28, Y, FW, 22, "ambush");
	o_friend  = new Fl_Check_Button(AX+28, Y, FW, 22, "friend");
	o_dormant = new Fl_Check_Button(BX+28, Y, FW, 22, "dormant");

	o_ambush ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Ambush));
	o_friend ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Friend));
	o_dormant->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Hexen_Dormant));

	o_dormant->hide();

	o_sf_ambush = new Fl_Check_Button( X+28, Y, FW, 22, "ambush");
	o_sf_friend = new Fl_Check_Button(AX+28, Y, FW, 22, "friend");
	o_sf_stand  = new Fl_Check_Button( X+28, Y+22, FW, 22, "stand");

	o_sf_ambush->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Strife_Ambush));
	o_sf_friend->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Strife_Friend));
	o_sf_stand ->callback(option_callback, new thing_opt_CB_data_c(this, MTF_Strife_Stand));

	o_sf_ambush->hide();
	o_sf_friend->hide();
	o_sf_stand ->hide();

	Y = Y + 45;


	exfloor = new UI_DynIntInput(X+84, Y, 64, 24, "3D Floor: ");
	exfloor->align(FL_ALIGN_LEFT);
	exfloor->callback(option_callback, new thing_opt_CB_data_c(this, MTF_EXFLOOR_MASK));
	exfloor->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	efl_down = new Fl_Button(X+165, Y+1, 30, 22, "-");
	efl_up   = new Fl_Button(X+210, Y+1, 30, 22, "+");

	efl_down->labelfont(FL_HELVETICA_BOLD);
	efl_up  ->labelfont(FL_HELVETICA_BOLD);
	efl_down->labelsize(16);
	efl_up  ->labelsize(16);

	efl_down->callback(button_callback, this);
	efl_up  ->callback(button_callback, this);

#if 0
	Y = Y + exfloor->h() + 10;
#else
  	exfloor->hide();
  	efl_down->hide();
  	efl_up->hide();
#endif


	// Hexen thing specials

	spec_type = new UI_DynInput(X+74, Y, 64, 24, "Special: ");
	spec_type->align(FL_ALIGN_LEFT);
	spec_type->callback(spec_callback, this);
	spec_type->callback2(dynspec_callback, this);
	spec_type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	spec_type->type(FL_INT_INPUT);
	spec_type->hide();

	spec_choose = new Fl_Button(X+W/2+24, Y, 80, 24, "Choose");
	spec_choose->callback(button_callback, this);
	spec_choose->hide();

	Y = Y + spec_type->h() + 2;

	spec_desc = new Fl_Output(X+74, Y, W-86, 24, "Desc: ");
	spec_desc->align(FL_ALIGN_LEFT);
	spec_desc->hide();

	Y = Y + spec_desc->h() + 2;

	for (int a = 0 ; a < 5 ; a++)
	{
		args[a] = new UI_DynIntInput(X+74+43*a, Y, 39, 24);
		args[a]->callback(args_callback, new thing_opt_CB_data_c(this, a));
		args[a]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		args[a]->hide();
	}

	args[0]->label("Args: ");

	mFixUp.loadFields({type, angle, tid, exfloor, pos_x, pos_y, pos_z, spec_type,
					  args[0], args[1], args[2], args[3], args[4]});

	end();

	resizable(NULL);
}


void UI_ThingBox::type_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_type = atoi(box->type->value());

	const thingtype_t &info = box->inst.conf.getThingType(new_type);

	box->desc->value(info.desc.c_str());
	box->sprite->GetSprite(new_type, FL_DARK2);

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited type of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected) ; !it.done() ; it.next())
		{
			op.changeThing(*it, Thing::F_TYPE, new_type);
		}
	}
}


void UI_ThingBox::dyntype_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	if (box->obj < 0)
		return;

	int value = atoi(box->type->value());

	const thingtype_t &info = box->inst.conf.getThingType(value);

	box->desc->value(info.desc.c_str());
	box->sprite->GetSprite(value, FL_DARK2);
}


void UI_ThingBox::spec_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_type = atoi(box->spec_type->value());

	const linetype_t &info = box->inst.conf.getLineType(new_type);

	if (new_type == 0)
		box->spec_desc->value("");
	else
		box->spec_desc->value(info.desc.c_str());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited special of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected) ; !it.done() ; it.next())
		{
			op.changeThing(*it, Thing::F_SPECIAL, new_type);
		}
	}
}


void UI_ThingBox::dynspec_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	if (box->obj < 0 || box->inst.loaded.levelFormat == MapFormat::doom)
		return;

	int value = atoi(box->spec_type->value());

	if (value)
	{
		const linetype_t &info = box->inst.conf.getLineType(value);
		box->spec_desc->value(info.desc.c_str());
	}
	else
	{
		box->spec_desc->value("");
	}
}


void UI_ThingBox::SetThingType(int new_type)
{
	if (obj < 0)
		return;

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%d", new_type);

	mFixUp.checkDirtyFields();
	mFixUp.setInputValue(type, buffer);
	type->do_callback();
}


void UI_ThingBox::SetSpecialType(int new_type)
{
	if (obj < 0)
		return;

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%d", new_type);

	mFixUp.checkDirtyFields();
	mFixUp.setInputValue(spec_type, buffer);
	spec_type->do_callback();
}

//
// Thing box clipboard operation
//
bool UI_ThingBox::ClipboardOp(EditCommand op)
{
	return false;
}


void UI_ThingBox::BrowsedItem(BrowserMode kind, int number, const char *name, int e_state)
{
	if (kind == BrowserMode::things)
	{
		SetThingType(number);
	}
	else if (kind == BrowserMode::lineTypes && inst.loaded.levelFormat != MapFormat::doom)
	{
		SetSpecialType(number);
	}
}


void UI_ThingBox::angle_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_ang = atoi(box->angle->value());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited angle of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			op.changeThing(*it, Thing::F_ANGLE, new_ang);
		}
	}
}


// IOANCH 9/2015
void UI_ThingBox::tid_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_tid = atoi(box->tid->value());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited TID of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			op.changeThing(*it, Thing::F_TID, new_tid);
		}
	}
}


void UI_ThingBox::x_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_x = atoi(box->pos_x->value());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited X of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			op.changeThing(*it, Thing::F_X, MakeValidCoord(box->inst.loaded.levelFormat, new_x));

	}
}

void UI_ThingBox::y_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_y = atoi(box->pos_y->value());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited Y of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			op.changeThing(*it, Thing::F_Y, MakeValidCoord(box->inst.loaded.levelFormat, new_y));
	}
}

void UI_ThingBox::z_callback(Fl_Widget *w, void *data)
{
	UI_ThingBox *box = (UI_ThingBox *)data;

	int new_h = atoi(box->pos_z->value());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited Z of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			op.changeThing(*it, Thing::F_H, FFixedPoint(new_h));
	}
}


void UI_ThingBox::option_callback(Fl_Widget *w, void *data)
{
	thing_opt_CB_data_c *ocb = (thing_opt_CB_data_c *)data;

	UI_ThingBox *box = ocb->parent;

	int mask = ocb->mask;
	int new_opts = box->CalcOptions();

	if (!box->inst.edit.Selected->empty())
	{
		box->mFixUp.checkDirtyFields();
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited flags of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			const auto T = box->inst.level.things[*it];

			// only change the bits specified in 'mask'.
			// this is important when multiple things are selected.
			op.changeThing(*it, Thing::F_OPTIONS, (T->options & ~mask) | (new_opts & mask));
		}
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
		box->inst.main_win->BrowserMode(BrowserMode::things);

	if (w == box->spec_choose)
		box->inst.main_win->BrowserMode(BrowserMode::lineTypes);

	box->mFixUp.checkDirtyFields();

	// check for the angle buttons
	for (int i = 0 ; i < 8 ; i++)
	{
		if (w == box->ang_buts[i])
		{
			char buffer[64];
			snprintf(buffer, sizeof(buffer), "%d", i * 45);

			box->mFixUp.setInputValue(box->angle, buffer);

			angle_callback(box->angle, box);
		}
	}
}


void UI_ThingBox::args_callback(Fl_Widget *w, void *data)
{
	thing_opt_CB_data_c *ocb = (thing_opt_CB_data_c *)data;

	UI_ThingBox *box = ocb->parent;

	int arg_idx = ocb->mask;
	int new_value = atoi(box->args[arg_idx]->value());

	assert(arg_idx >= 0 && arg_idx <= 4);

	new_value = clamp(0, new_value, 255);

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited args of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			op.changeThing(*it, static_cast<Thing::IntAddress>(Thing::F_ARG1 + arg_idx),
                                              new_value);
		}
	}
}


void UI_ThingBox::AdjustExtraFloor(int dir)
{
	if (!inst.level.isThing(obj))
		return;

	int old_fl = atoi(exfloor->value());
	int new_fl = (old_fl + dir) & 15;

	if(new_fl)
		mFixUp.setInputValue(exfloor, SString(new_fl).c_str());
	else
		mFixUp.setInputValue(exfloor, "");

	thing_opt_CB_data_c ocb(this, MTF_EXFLOOR_MASK);

	option_callback(this, &ocb);
}


void UI_ThingBox::OptionsFromInt(int options)
{
	o_easy  ->value((options & MTF_Easy)   ? 1 : 0);
	o_medium->value((options & MTF_Medium) ? 1 : 0);
	o_hard  ->value((options & MTF_Hard)   ? 1 : 0);

	o_ambush->value((options & MTF_Ambush) ? 1 : 0);

	if (inst.loaded.levelFormat != MapFormat::doom)
	{
		o_sp  ->value((options & MTF_Hexen_SP)   ? 1 : 0);
		o_coop->value((options & MTF_Hexen_COOP) ? 1 : 0);
		o_dm  ->value((options & MTF_Hexen_DM)   ? 1 : 0);

		o_fight ->value((options & MTF_Hexen_Fighter) ? 1 : 0);
		o_cleric->value((options & MTF_Hexen_Cleric)  ? 1 : 0);
		o_mage  ->value((options & MTF_Hexen_Mage)    ? 1 : 0);

		o_dormant->value((options & MTF_Hexen_Dormant) ? 1 : 0);
	}
	else
	{
		o_sp  ->value((options & MTF_Not_SP)   ? 0 : 1);
		o_coop->value((options & MTF_Not_COOP) ? 0 : 1);
		o_dm  ->value((options & MTF_Not_DM)   ? 0 : 1);

		o_vanilla_dm->value((options & MTF_Not_SP) ? 1 : 0);
	}

	if (inst.loaded.levelFormat == MapFormat::doom)
	{
		o_friend->value((options & MTF_Friend) ? 1 : 0);

		if(options & MTF_EXFLOOR_MASK)
			mFixUp.setInputValue(exfloor, SString((options & MTF_EXFLOOR_MASK) >> MTF_EXFLOOR_SHIFT).c_str());
		else
			mFixUp.setInputValue(exfloor, "");
	}

	if (inst.conf.features.strife_flags)
	{
		o_sf_ambush->value((options & MTF_Strife_Ambush) ? 1 : 0);
		o_sf_friend->value((options & MTF_Strife_Friend) ? 1 : 0);
		o_sf_shadow->value((options & MTF_Strife_Shadow) ? 1 : 0);
		o_sf_altvis->value((options & MTF_Strife_AltVis) ? 1 : 0);
		o_sf_stand ->value((options & MTF_Strife_Stand) ? 1 : 0);
	}
}


int UI_ThingBox::CalcOptions() const
{
	int options = 0;

	if (o_easy  ->value()) options |= MTF_Easy;
	if (o_medium->value()) options |= MTF_Medium;
	if (o_hard  ->value()) options |= MTF_Hard;

	if (inst.conf.features.strife_flags)
	{
		if (o_sf_ambush->value()) options |= MTF_Strife_Ambush;
		if (o_sf_friend->value()) options |= MTF_Strife_Friend;
		if (o_sf_shadow->value()) options |= MTF_Strife_Shadow;
		if (o_sf_altvis->value()) options |= MTF_Strife_AltVis;
		if (o_sf_stand ->value()) options |= MTF_Strife_Stand;
	}
	else
	{
		if (o_ambush->value()) options |= MTF_Ambush;
	}

	if (inst.loaded.levelFormat != MapFormat::doom)
	{
		if (o_sp  ->value()) options |= MTF_Hexen_SP;
		if (o_coop->value()) options |= MTF_Hexen_COOP;
		if (o_dm  ->value()) options |= MTF_Hexen_DM;

		if (o_fight ->value()) options |= MTF_Hexen_Fighter;
		if (o_cleric->value()) options |= MTF_Hexen_Cleric;
		if (o_mage  ->value()) options |= MTF_Hexen_Mage;

		if (o_dormant->value()) options |= MTF_Hexen_Dormant;
	}
	else if (inst.conf.features.coop_dm_flags)
	{
		if (0 == o_sp  ->value()) options |= MTF_Not_SP;
		if (0 == o_coop->value()) options |= MTF_Not_COOP;
		if (0 == o_dm  ->value()) options |= MTF_Not_DM;
	}
	else
	{
		if (o_vanilla_dm->value()) options |= MTF_Not_SP;
	}

	if (inst.loaded.levelFormat == MapFormat::doom)
	{
		if (inst.conf.features.friend_flag && o_friend->value())
			options |= MTF_Friend;

#if 0
		int exfl_num = atoi(exfloor->value());

		if (exfl_num > 0)
		{
			options |= (exfl_num << MTF_EXFLOOR_SHIFT) & MTF_EXFLOOR_MASK;
		}
#endif
	}

	return options;
}


void UI_ThingBox::UpdateField(int field)
{
	if (field < 0 ||
		field == Thing::F_X ||
		field == Thing::F_Y ||
		field == Thing::F_H)
	{
		if (inst.level.isThing(obj))
		{
			const auto T = inst.level.things[obj];

			// @@ FIXME show decimals in UDMF
			mFixUp.setInputValue(pos_x, SString(static_cast<int>(T->x())).c_str());
			mFixUp.setInputValue(pos_y, SString(static_cast<int>(T->y())).c_str());
			mFixUp.setInputValue(pos_z, SString(static_cast<int>(T->h())).c_str());
		}
		else
		{
			mFixUp.setInputValue(pos_x, "");
			mFixUp.setInputValue(pos_y, "");
			mFixUp.setInputValue(pos_z, "");
		}
	}

	if (field < 0 || field == Thing::F_ANGLE)
	{
		if(inst.level.isThing(obj))
			mFixUp.setInputValue(angle, SString(inst.level.things[obj]->angle).c_str());
		else
			mFixUp.setInputValue(angle, "");
	}

	// IOANCH 9/2015
	if (field < 0 || field == Thing::F_TID)
	{
		if(inst.level.isThing(obj))
			mFixUp.setInputValue(tid, SString(inst.level.things[obj]->tid).c_str());
		else
			mFixUp.setInputValue(tid, "");
	}

	if (field < 0 || field == Thing::F_TYPE)
	{
		if (inst.level.isThing(obj))
		{
			const thingtype_t &info = inst.conf.getThingType(inst.level.things[obj]->type);
			desc->value(info.desc.c_str());
			mFixUp.setInputValue(type, SString(inst.level.things[obj]->type).c_str());
			sprite->GetSprite(inst.level.things[obj]->type, FL_DARK2);
		}
		else
		{
			mFixUp.setInputValue(type, "");
			desc ->value("");
			sprite->Clear();
		}
	}

	if (field < 0 || field == Thing::F_OPTIONS)
	{
		if (inst.level.isThing(obj))
			OptionsFromInt(inst.level.things[obj]->options);
		else
			OptionsFromInt(0);
	}

	if (inst.loaded.levelFormat == MapFormat::doom)
		return;

	if (field < 0 || field == Thing::F_SPECIAL)
	{
		if (inst.level.isThing(obj) && inst.level.things[obj]->special)
		{
			const linetype_t &info = inst.conf.getLineType(inst.level.things[obj]->special);
			spec_desc->value(info.desc.c_str());
			mFixUp.setInputValue(spec_type, SString(inst.level.things[obj]->special).c_str());
		}
		else
		{
			mFixUp.setInputValue(spec_type, "");
			spec_desc->value("");
		}
	}

	if (field < 0 || (field >= Thing::F_ARG1 && field <= Thing::F_ARG5))
	{
		for (int a = 0 ; a < 5 ; a++)
		{
			mFixUp.setInputValue(args[a], "");
			args[a]->tooltip(NULL);
			args[a]->textcolor(FL_BLACK);
		}

		if (inst.level.isThing(obj))
		{
			const auto T = inst.level.things[obj];

			const thingtype_t &info = inst.conf.getThingType(T->type);
			const linetype_t  &spec = inst.conf.getLineType (T->special);

			// set argument values and tooltips
			for (int a = 0 ; a < 5 ; a++)
			{
				int arg_val = T->Arg(1 + a);

				if (T->special)
				{
					mFixUp.setInputValue(args[a], SString(arg_val).c_str());

					if (!spec.args[a].name.empty())
						args[a]->copy_tooltip(spec.args[a].name.c_str());
					else
						args[a]->textcolor(fl_rgb_color(160,160,160));
				}
				else
				{
					// spawn arguments
					if(arg_val || !info.args[a].empty())
						mFixUp.setInputValue(args[a], SString(arg_val).c_str());

					if (!info.args[a].empty())
						args[a]->copy_tooltip(info.args[a].c_str());
					else
						args[a]->textcolor(fl_rgb_color(160,160,160));
				}
			}
		}
	}
}


void UI_ThingBox::UpdateTotal(const Document &doc) noexcept
{
	which->SetTotal(doc.numThings());
}


void UI_ThingBox::UpdateGameInfo(const LoadingData &loaded, const ConfigData &config)
{
	if (config.features.coop_dm_flags || loaded.levelFormat != MapFormat::doom)
	{
		o_sp  ->show();
		o_coop->show();
		o_dm  ->show();

		o_vanilla_dm->hide();
	}
	else
	{
		// this is appropriate for Strife too

		o_vanilla_dm->show();

		o_sp  ->hide();
		o_coop->hide();
		o_dm  ->hide();
	}

	if (config.features.friend_flag && !config.features.strife_flags)
		o_friend->show();
	else
		o_friend->hide();


	if (config.features.strife_flags)
	{
		o_ambush->hide();

		o_sf_ambush->show();
		o_sf_friend->show();
		o_sf_shadow->show();
		o_sf_altvis->show();
		o_sf_stand ->show();
	}
	else
	{
		o_ambush->show();

		o_sf_ambush->hide();
		o_sf_friend->hide();
		o_sf_shadow->hide();
		o_sf_altvis->hide();
		o_sf_stand ->hide();
	}


	/* map format stuff */

	thing_opt_CB_data_c *ocb;

	if (loaded.levelFormat != MapFormat::doom)
	{
		pos_z->show();

		tid->show();

		o_fight ->show();
		o_cleric->show();
		o_mage  ->show();

		o_dormant->show();

		spec_type  ->show();
		spec_choose->show();
		spec_desc  ->show();

		for (int a = 0 ; a < 5 ; a++)
			args[a]->show();

		// fix the masks for SP/COOP/DM
		ocb = (thing_opt_CB_data_c *) o_sp  ->user_data(); ocb->mask = MTF_Hexen_SP;
		ocb = (thing_opt_CB_data_c *) o_coop->user_data(); ocb->mask = MTF_Hexen_COOP;
		ocb = (thing_opt_CB_data_c *) o_dm  ->user_data(); ocb->mask = MTF_Hexen_DM;
	}
	else
	{
		pos_z->hide();

		tid->hide();

		o_fight ->hide();
		o_cleric->hide();
		o_mage  ->hide();

		o_dormant->hide();

		spec_type  ->hide();
		spec_choose->hide();
		spec_desc  ->hide();

		for (int a = 0 ; a < 5 ; a++)
			args[a]->hide();

		// fix the masks for SP/COOP/DM
		ocb = (thing_opt_CB_data_c *) o_sp  ->user_data(); ocb->mask = MTF_Not_SP;
		ocb = (thing_opt_CB_data_c *) o_coop->user_data(); ocb->mask = MTF_Not_COOP;
		ocb = (thing_opt_CB_data_c *) o_dm  ->user_data(); ocb->mask = MTF_Not_DM;
	}

	redraw();
}


void UI_ThingBox::UnselectPics()
{
	sprite->Unhighlight();
	sprite->Selected(false);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
