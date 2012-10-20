//------------------------------------------------------------------------
//  Radius Trigger Info
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2009 Andrew Apted
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


//
// UI_RadiusBox Constructor
//
UI_RadiusBox::UI_RadiusBox(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    view_RAD(NULL)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX);

	X += 6;
	Y += 5;

	W -= 12;
	H -= 10;

	int MX = X + W/2;


	which = new UI_Nombre(X, Y, W-10, 28, "RadTrig");

	add(which);

	Y += which->h() + 4;



	name = new Fl_Input(X+58, Y, W-60, 22, "Name:");
	name->align(FL_ALIGN_LEFT);
	name->callback(name_callback, this);
	name->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(name);

	Y += name->h() + 3;


	tag = new Fl_Int_Input(X+58, Y, 64, 22, "Tag:");
	tag->align(FL_ALIGN_LEFT);
	tag->callback(tag_callback, this);
	tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(tag);

	Y += tag->h() + 9;


#if 0
	Fl_Box *sh_lab = new Fl_Box(FL_NO_BOX, X, Y, W, 22, "Shape:");
	sh_lab->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	add(sh_lab);

	is_radius = new Fl_Round_Button(X+50, Y, 68, 22, "Radius");
	is_radius->type(FL_RADIO_BUTTON);
	is_radius->value(1);
	is_radius->callback(shape_callback, this);
	add(is_radius);

	is_rect = new Fl_Round_Button(X+120, Y, 60, 22, "Rect");
	is_rect->type(FL_RADIO_BUTTON);
	is_rect->value(0);
	is_rect->callback(shape_callback, this);
	add(is_rect);

	Y += is_rect->h() + 4;
#endif


	pos_x1 = new Fl_Float_Input(X +30, Y, 84, 24, "x1:");
	pos_x2 = new Fl_Float_Input(MX+30, Y, 84, 24, "x2:");

	pos_x1->align(FL_ALIGN_LEFT);
	pos_x2->align(FL_ALIGN_LEFT);

	pos_x1->callback(pos_callback, this);
	pos_x2->callback(pos_callback, this);

	pos_x1->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	pos_x2->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(pos_x1);
	add(pos_x2);

	Y += pos_x1->h() + 3;


	pos_y1 = new Fl_Float_Input(X +30, Y, 84, 24, "y1:");
	pos_y2 = new Fl_Float_Input(MX+30, Y, 84, 24, "y2:");

	pos_y1->align(FL_ALIGN_LEFT);
	pos_y2->align(FL_ALIGN_LEFT);

	pos_y1->callback(pos_callback, this);
	pos_y2->callback(pos_callback, this);

	pos_y1->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	pos_y2->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(pos_y1);
	add(pos_y2);

	Y += pos_y2->h() + 3;


	pos_z1 = new Fl_Float_Input( X+30, Y, 84, 24, "z2:");
	pos_z2 = new Fl_Float_Input(MX+30, Y, 84, 24, "z2:");

	pos_z1->align(FL_ALIGN_LEFT);
	pos_z2->align(FL_ALIGN_LEFT);

	pos_z1->callback(height_callback, this);
	pos_z2->callback(height_callback, this);

	pos_z1->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	pos_z2->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(pos_z1);
	add(pos_z2);

	Y += pos_z1->h() + 7;


	// when appear: two rows of three on/off buttons

	Fl_Box *when_lab = new Fl_Box(X, Y, W, 22, "When Appear:");
	when_lab->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

	add(when_lab);

	Y += when_lab->h() + 2;


	int AX = X+W/2;
//	int BX = X+2*W/3;
	int CW = W/3 - 12;

	int AY = Y+22;
	int BY = Y+22*2;

	wa_easy   = new Fl_Check_Button(X+24,  Y, CW, 22, "easy");
	wa_medium = new Fl_Check_Button(X+24, AY, CW, 22, "medium");
	wa_hard   = new Fl_Check_Button(X+24, BY, CW, 22, "hard");

	//  Y += wa_easy->h() + 2;

	wa_sp     = new Fl_Check_Button(AX+4,  Y, CW, 22, "sp");
	wa_coop   = new Fl_Check_Button(AX+4, AY, CW, 22, "coop");
	wa_dm     = new Fl_Check_Button(AX+4, BY, CW, 22, "dm");

	wa_easy  ->value(1);   wa_sp->value(1);
	wa_medium->value(1); wa_coop->value(1);
	wa_hard  ->value(1);   wa_dm->value(1);

	wa_easy  ->callback(whenapp_callback, this);
	wa_medium->callback(whenapp_callback, this);
	wa_hard  ->callback(whenapp_callback, this);

	wa_sp  ->callback(whenapp_callback, this);
	wa_coop->callback(whenapp_callback, this);
	wa_dm  ->callback(whenapp_callback, this);

	add(wa_easy); add(wa_medium); add(wa_hard);
	add(wa_sp);   add(wa_coop);   add(wa_dm);


	Y = wa_hard->y() + wa_hard->h() + 10;


	resizable(NULL);
}

//
// UI_RadiusBox Destructor
//
UI_RadiusBox::~UI_RadiusBox()
{
}


void UI_RadiusBox::SetViewRad(rad_trigger_c *rad)
{
#if 0 // TODO
	if (view_RAD == rad)
		return;

	view_RAD = rad;

	if (view_RAD)
	{
		main_win->panel->SetWhich(rad->rad_Index + 1, active_startmap->rad_Total);

		LoadData(view_RAD);
		activate();
	}
	else
	{
		main_win->panel->SetWhich(-1, -1);

		deactivate();
	}

	redraw();
#endif
}

void UI_RadiusBox::shape_callback(Fl_Widget *w, void *data)
{
#if 0 // TODO
	UI_RadiusBox *info = (UI_RadiusBox *)data;
	SYS_ASSERT(info);

	if (info->view_RAD)
	{
		if (w == info->is_radius)
			Edit_ChangeShape(info->view_RAD, 0);

		if (w == info->is_rect)
			Edit_ChangeShape(info->view_RAD, 1);
	}
#endif
}

void UI_RadiusBox::pos_callback(Fl_Widget *w, void *data)
{
#if 0 // TODO
	UI_RadiusBox *info = (UI_RadiusBox *)data;
	SYS_ASSERT(info);

	rad_trigger_c *RAD = info->view_RAD;
	if (! RAD)
		return;

	if (w == info->radius)
	{
		float r = Float_or_Unspec(info->radius->value());

		if (r == FLOAT_UNSPEC || r < 4)
			r = 4;

		Edit_ResizeRadiusOnly(RAD, r);
		return;
	}

	if (! RAD->is_rect)
	{
		if (w == info->pos_x1 || w == info->pos_y1)
		{
			Edit_MoveRad(RAD,
					atof(info->pos_x1->value()),
					atof(info->pos_y1->value()));
			return;
		}
	}

	float x1 = atof(info->pos_x1->value());
	float y1 = atof(info->pos_y1->value());
	float x2 = atof(info->pos_x2->value());
	float y2 = atof(info->pos_y2->value());

	Edit_ResizeRad(RAD, x1, y1, x2, y2);
#endif
}

void UI_RadiusBox::height_callback(Fl_Widget *w, void *data)
{
#if 0 // TODO
	UI_RadiusBox *info = (UI_RadiusBox *)data;
	SYS_ASSERT(info);

	rad_trigger_c *RAD = info->view_RAD;
	if (! RAD)
		return;

	if (w == info->pos_z1)
	{
		Edit_ChangeFloat(RAD, rad_trigger_c::F_Z1,
				Float_or_Unspec(info->pos_z1->value()));
	}
	else /* (w == info->pos_z2) */
	{
		Edit_ChangeFloat(RAD, rad_trigger_c::F_Z2,
				Float_or_Unspec(info->pos_z2->value()));
	}
#endif
}

void UI_RadiusBox::name_callback(Fl_Widget *w, void *data)
{
#if 0 // TODO
	UI_RadiusBox *info = (UI_RadiusBox *)data;
	SYS_ASSERT(info);

	rad_trigger_c *RAD = info->view_RAD;
	if (! RAD)
		return;

	Edit_ChangeString(RAD, rad_trigger_c::F_NAME,
			info->name->value());
#endif
}

void UI_RadiusBox::tag_callback(Fl_Widget *w, void *data)
{
#if 0 // TODO
	UI_RadiusBox *info = (UI_RadiusBox *)data;
	SYS_ASSERT(info);

	rad_trigger_c *RAD = info->view_RAD;
	if (! RAD)
		return;

	Edit_ChangeInt(RAD, rad_trigger_c::F_TAG,
			Int_or_Unspec(info->tag->value()));
#endif
}

void UI_RadiusBox::whenapp_callback(Fl_Widget *w, void *data)
{
#if 0 // TODO
	UI_RadiusBox *info = (UI_RadiusBox *)data;
	SYS_ASSERT(info);

	rad_trigger_c *RAD = info->view_RAD;
	if (! RAD)
		return;

	int when_appear = info->CalcWhenAppear();

	Edit_ChangeInt(RAD, rad_trigger_c::F_WHEN_APPEAR, when_appear);
#endif
}

int UI_RadiusBox::CalcWhenAppear()
{
	int when_appear = 0;

#if 0 // TODO
	if (wa_easy->value())   when_appear |= WNAP_Easy;
	if (wa_medium->value()) when_appear |= WNAP_Medium;
	if (wa_hard->value())   when_appear |= WNAP_Hard;

	if (wa_sp->value())     when_appear |= WNAP_SP;
	if (wa_coop->value())   when_appear |= WNAP_Coop;
	if (wa_dm->value())     when_appear |= WNAP_DM;
#endif

	return when_appear;
}


//------------------------------------------------------------------------

void UI_ListenRadTrig(rad_trigger_c *rad, int F)
{
#if 0 // TODO
	main_win->panel->script_box->ListenField(rad, F);
#endif
}

void UI_RadiusBox::ListenField(rad_trigger_c *rad, int F)
{
#if 0 // TODO
	if (rad != view_RAD)
		return;

	switch (F)
	{
		case rad_trigger_c::F_IS_RECT:
			update_Shape(rad);
			break;

		case rad_trigger_c::F_MX: case rad_trigger_c::F_MY:
		case rad_trigger_c::F_RX: case rad_trigger_c::F_RY:
			update_XY(rad);
			break;

		case rad_trigger_c::F_Z1: case rad_trigger_c::F_Z2:
			update_Z(rad);
			break;

		case rad_trigger_c::F_NAME:
			update_Name(rad);
			break;

		case rad_trigger_c::F_TAG:
			update_Tag(rad);
			break;

		case rad_trigger_c::F_WHEN_APPEAR:
			update_WhenAppear(rad);
			break;

		default: break;
	}

	main_win->grid->redraw();
#endif
}

void UI_RadiusBox::LoadData(rad_trigger_c *rad)
{
#if 0 // TODO
	update_Shape(rad);
	update_XY(rad);
	update_Z(rad);
	update_Name(rad);
	update_Tag(rad);
	update_WhenAppear(rad);
#endif
}

void UI_RadiusBox::update_Shape(rad_trigger_c *rad)
{
#if 0 // TODO
	if (rad->is_rect)
	{
		is_rect  ->value(1);
		is_radius->value(0);

		radius->hide();

		pos_x2->show();
		pos_y2->show();
	}
	else /* radius */
	{
		is_radius->value(1);
		is_rect  ->value(0);

		pos_x2->hide();
		pos_y2->hide();

		radius->show();
	}

	update_XY(rad);
#endif
}

void UI_RadiusBox::update_XY(rad_trigger_c *rad)
{
#if 0 // TODO
	if (is_radius->value())
	{
		pos_x1->value(Float_TmpStr(rad->mx, 1));
		pos_y1->value(Float_TmpStr(rad->my, 1));

		radius->value(Float_TmpStr(rad->rx, 1));
		return;
	}

	/* rectangle */

	pos_x1->value(Float_TmpStr(rad->mx - rad->rx, 1));
	pos_x2->value(Float_TmpStr(rad->mx + rad->rx, 1));

	pos_y1->value(Float_TmpStr(rad->my - rad->ry, 1));
	pos_y2->value(Float_TmpStr(rad->my + rad->ry, 1));
#endif
}

void UI_RadiusBox::update_Z(rad_trigger_c *rad)
{
#if 0 // TODO
	pos_z1->value(Float_TmpStr(rad->z1, 1));
	pos_z2->value(Float_TmpStr(rad->z2, 1));
#endif
}

void UI_RadiusBox::update_Name(rad_trigger_c *rad)
{
#if 0 // TODO
	name->value(rad->name.c_str());
#endif
}

void UI_RadiusBox::update_Tag(rad_trigger_c *rad)
{
#if 0 // TODO
	tag->value(Int_TmpStr(rad->tag));
#endif
}

void UI_RadiusBox::update_WhenAppear(rad_trigger_c *rad)
{
#if 0 // TODO
	wa_easy->value(   (rad->when_appear & WNAP_Easy)  ? 1 : 0);
	wa_medium->value( (rad->when_appear & WNAP_Medium)? 1 : 0);
	wa_hard->value(   (rad->when_appear & WNAP_Hard)  ? 1 : 0);

	wa_sp->value(   (rad->when_appear & WNAP_SP)  ? 1 : 0);
	wa_coop->value( (rad->when_appear & WNAP_Coop)? 1 : 0);
	wa_dm->value(   (rad->when_appear & WNAP_DM)  ? 1 : 0);
#endif
}


void UI_RadiusBox::UpdateTotal()
{
	which->SetTotal(NumRadTrigs);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
