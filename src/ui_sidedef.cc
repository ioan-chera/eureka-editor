//------------------------------------------------------------------------
//  SIDEDEF INFORMATION
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

#include "Document.h"
#include "main.h"
#include "ui_window.h"

#include "e_hover.h"	// OppositeSector
#include "e_linedef.h"
#include "e_main.h"
#include "m_config.h"
#include "m_game.h"
#include "r_render.h"
#include "w_rawdef.h"
#include "w_texture.h"

enum
{
	TEXTURE_TILE_OUTSET = 8
};


// config item
bool config::swap_sidedefs = false;
bool config::show_full_one_sided = false;
bool config::sidedef_add_del_buttons = false;

//
// Constructor
//
UI_SideBox::UI_SideBox(int X, int Y, int W, int H, int _side) :
	Fl_Group(X, Y, W, H),
	obj(SETOBJ_NO_LINE), is_front(_side == 0),
	on_2S_line(false)
{
	box(FL_FLAT_BOX); // FL_UP_BOX

	align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_LEFT);

	if (is_front)
		labelcolor(FL_BLUE);
	else
		labelcolor(fl_rgb_color(224,64,0));


	add_button = new Fl_Button(X + W - 120, Y, 50, 20, "ADD");
	add_button->labelcolor(labelcolor());
	add_button->callback(add_callback, this);

	del_button = new Fl_Button(X + W - 65, Y, 50, 20, "DEL");
	del_button->labelcolor(labelcolor());
	del_button->callback(delete_callback, this);


	X += 6;
	Y += 6 + 16;  // space for label

	W -= 12;
	H -= 12;

	int MX = X + W/2;


	x_ofs = new Fl_Int_Input(X+28,   Y, 52, 24, "x:");
	y_ofs = new Fl_Int_Input(MX-20,  Y, 52, 24, "y:");
	sec = new Fl_Int_Input(X+W-59, Y, 52, 24, "sec:");

	x_ofs->align(FL_ALIGN_LEFT);
	y_ofs->align(FL_ALIGN_LEFT);
	sec  ->align(FL_ALIGN_LEFT);

	x_ofs->callback(offset_callback, this);
	y_ofs->callback(offset_callback, this);
	sec  ->callback(sector_callback, this);

	x_ofs->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	y_ofs->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	sec  ->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	Y += x_ofs->h() + 6;

	int LX = X+16;
	int UX = X+W-64-16;
		MX = MX-32;

	if (config::swap_sidedefs)
	{
		std::swap(UX, LX);
	}

	m_x_tile_positions[0] = std::min(LX, UX);
	m_x_tile_positions[1] = MX;

	l_pic = new UI_Pic(LX, Y, 64, 64, "Lower");
	u_pic = new UI_Pic(UX, Y, 64, 64, "Upper");
	r_pic = new UI_Pic(MX, Y, 64, 64, "Rail");

	l_pic->callback(tex_callback, this);
	u_pic->callback(tex_callback, this);
	r_pic->callback(tex_callback, this);

	Y += 65;

	l_tex = new UI_DynInput(LX - TEXTURE_TILE_OUTSET, Y, 80, 20);
	u_tex = new UI_DynInput(UX - TEXTURE_TILE_OUTSET, Y, 80, 20);
	r_tex = new UI_DynInput(MX - TEXTURE_TILE_OUTSET, Y, 80, 20);

	l_tex->textsize(12);
	u_tex->textsize(12);
	r_tex->textsize(12);

	l_tex->callback(tex_callback, this);
	u_tex->callback(tex_callback, this);
	r_tex->callback(tex_callback, this);

	l_tex->callback2(dyntex_callback, this);
	u_tex->callback2(dyntex_callback, this);
	r_tex->callback2(dyntex_callback, this);

	l_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	u_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	r_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	end();


	UpdateHiding();
	UpdateLabel();
	UpdateAddDel();
}


//
// Destructor
//
UI_SideBox::~UI_SideBox()
{ }


void UI_SideBox::tex_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	if (box->obj < 0)
		return;

	if (Fl::event_button() != 3 &&
		(w == box->l_pic || w == box->u_pic || w == box->r_pic))
	{
		UI_Pic * pic = (UI_Pic *)w;

		pic->Selected(! pic->Selected());

		if (pic->Selected())
			main_win->BrowserMode('T');
		return;
	}

	int new_tex;

	// right click sets to default value, "-" for rail
	if (Fl::event_button() == 3)
	{
		if (w == box->r_pic)
			new_tex = BA_InternaliseString("-");
		else
			new_tex = BA_InternaliseString(default_wall_tex);
	}
	else
	{
		if (w == box->l_tex)
			new_tex = BA_InternaliseString(NormalizeTex(box->l_tex->value()));
		else if (w == box->u_tex)
			new_tex = BA_InternaliseString(NormalizeTex(box->u_tex->value()));
		else
			new_tex = BA_InternaliseString(NormalizeTex(box->r_tex->value()));
	}

	// iterate over selected linedefs
	if (! edit.Selected->empty())
	{
		gDocument.basis.begin();
		gDocument.basis.setMessageForSelection("edited texture on", *edit.Selected);

		for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
		{
			const LineDef *L = gDocument.linedefs[*it];

			int sd = box->is_front ? L->right : L->left;

			if (is_sidedef(sd))
			{
				bool lower = (w == box->l_tex || w == box->l_pic);
				bool upper = (w == box->u_tex || w == box->u_pic);
				bool rail  = (w == box->r_tex || w == box->r_pic);


				if (lower)
				{
					gDocument.basis.changeSidedef(sd, SideDef::F_LOWER_TEX, new_tex);
				}
				else if (upper)
				{
					gDocument.basis.changeSidedef(sd, SideDef::F_UPPER_TEX, new_tex);
				}
				else if (rail)
				{
					gDocument.basis.changeSidedef(sd, SideDef::F_MID_TEX,   new_tex);
				}
			}
		}

		gDocument.basis.end();

		box->UpdateField();
	}
}


void UI_SideBox::dyntex_callback(Fl_Widget *w, void *data)
{
	// change picture to match the input, BUT does not change the map

	UI_SideBox *box = (UI_SideBox *)data;

	if (box->obj < 0)
		return;

	if (w == box->l_tex)
	{
		box->l_pic->GetTex(box->l_tex->value());
	}
	else if (w == box->u_tex)
	{
		box->u_pic->GetTex(box->u_tex->value());
	}
	else if (w == box->r_tex)
	{
		box->r_pic->GetTex(box->r_tex->value());
	}
}


void UI_SideBox::add_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	if (box->obj >= 0)
		return;

	if (edit.Selected->empty())
		return;

	// iterate over selected linedefs

	int field = box->is_front ? LineDef::F_RIGHT : LineDef::F_LEFT;

	gDocument.basis.begin();

	// make sure we have a fallback sector to use
	if (NumSectors == 0)
	{
		int new_sec = gDocument.basis.addNew(ObjType::sectors);

		gDocument.sectors[new_sec]->SetDefaults();
	}

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		const LineDef *L = gDocument.linedefs[*it];

		int sd    = box->is_front ? L->right : L->left;
		int other = box->is_front ? L->left : L->right;

		// skip lines which already have this sidedef
		if (is_sidedef(sd))
			continue;

		// determine what sector to use
		int new_sec = OppositeSector(*it, box->is_front ? Side::right : Side::left);

		if (new_sec < 0)
			new_sec = NumSectors - 1;

		// create the new sidedef
		sd = gDocument.basis.addNew(ObjType::sidedefs);

		gDocument.sidedefs[sd]->SetDefaults(other >= 0);
		gDocument.sidedefs[sd]->sector = new_sec;

		gDocument.basis.changeLinedef(*it, field, sd);

		if (other >= 0)
			LD_AddSecondSideDef(*it, sd, other);
	}

	gDocument.basis.setMessageForSelection("added sidedef to", *edit.Selected);
	gDocument.basis.end();

	main_win->line_box->UpdateField();
	main_win->line_box->UpdateSides();
}


void UI_SideBox::delete_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	if (box->obj < 0)
		return;

	if (edit.Selected->empty())
		return;

	// iterate over selected linedefs
	gDocument.basis.begin();

	for (sel_iter_c it(edit.Selected) ; !it.done() ; it.next())
	{
		const LineDef *L = gDocument.linedefs[*it];

		int sd = box->is_front ? L->right : L->left;

		if (sd < 0)
			continue;

		// NOTE WELL: the actual sidedef is not deleted (it might be shared)

		LD_RemoveSideDef(*it, box->is_front ? Side::right : Side::left);
	}

	gDocument.basis.setMessageForSelection("deleted sidedef from", *edit.Selected);
	gDocument.basis.end();

	main_win->line_box->UpdateField();
	main_win->line_box->UpdateSides();
}


void UI_SideBox::offset_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	int new_x_ofs = atoi(box->x_ofs->value());
	int new_y_ofs = atoi(box->y_ofs->value());

	// iterate over selected linedefs
	if (! edit.Selected->empty())
	{
		gDocument.basis.begin();

		if (w == box->x_ofs)
			gDocument.basis.setMessageForSelection("edited X offset on", *edit.Selected);
		else
			gDocument.basis.setMessageForSelection("edited Y offset on", *edit.Selected);

		for (sel_iter_c it(edit.Selected); !it.done(); it.next())
		{
			const LineDef *L = gDocument.linedefs[*it];

			int sd = box->is_front ? L->right : L->left;

			if (is_sidedef(sd))
			{
				if (w == box->x_ofs)
					gDocument.basis.changeSidedef(sd, SideDef::F_X_OFFSET, new_x_ofs);
				else
					gDocument.basis.changeSidedef(sd, SideDef::F_Y_OFFSET, new_y_ofs);
			}
		}

		gDocument.basis.end();
	}
}


void UI_SideBox::sector_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	int new_sec = atoi(box->sec->value());

	new_sec = CLAMP(0, new_sec, NumSectors-1);

	// iterate over selected linedefs
	if (! edit.Selected->empty())
	{
		gDocument.basis.begin();
		gDocument.basis.setMessageForSelection("edited sector-ref on", *edit.Selected);

		for (sel_iter_c it(edit.Selected); !it.done(); it.next())
		{
			const LineDef *L = gDocument.linedefs[*it];

			int sd = box->is_front ? L->right : L->left;

			if (is_sidedef(sd))
				gDocument.basis.changeSidedef(sd, SideDef::F_SECTOR, new_sec);
		}

		gDocument.basis.end();
	}
}


//------------------------------------------------------------------------

void UI_SideBox::SetObj(int index, int solid_mask, bool two_sided)
{
	if (obj == index && what_is_solid == solid_mask && on_2S_line == two_sided)
		return;

	bool hide_change = !(obj == index && on_2S_line == two_sided);

	obj = index;

	what_is_solid = solid_mask;
	on_2S_line    = two_sided;

	if (hide_change)
		UpdateHiding();

	UpdateLabel();
	UpdateAddDel();
	UpdateField();

	if (obj < 0)
		UnselectPics();

	redraw();
}


void UI_SideBox::UpdateField()
{
	if (is_sidedef(obj))
	{
		const SideDef *sd = gDocument.sidedefs[obj];

		x_ofs->value(SString(sd->x_offset).c_str());
		y_ofs->value(SString(sd->y_offset).c_str());
		  sec->value(SString(sd->sector).c_str());

		SString lower = sd->LowerTex();
		SString rail  = sd->MidTex();
		SString upper = sd->UpperTex();

		l_tex->value(lower.c_str());
		u_tex->value(upper.c_str());
		r_tex->value(rail.c_str());

		l_pic->GetTex(lower);
		u_pic->GetTex(upper);
		r_pic->GetTex(rail);

		if ((what_is_solid & SOLID_LOWER) && is_null_tex(lower))
			l_pic->MarkMissing();

		if ((what_is_solid & SOLID_UPPER) && is_null_tex(upper))
			u_pic->MarkMissing();

		if (is_special_tex(lower)) l_pic->MarkSpecial();
		if (is_special_tex(upper)) u_pic->MarkSpecial();
		if (is_special_tex(rail))  r_pic->MarkSpecial();

		l_pic->AllowHighlight(true);
		u_pic->AllowHighlight(true);
		r_pic->AllowHighlight(true);
	}
	else
	{
		x_ofs->value("");
		y_ofs->value("");
		  sec->value("");

		l_tex->value("");
		u_tex->value("");
		r_tex->value("");

		l_pic->Clear();
		u_pic->Clear();
		r_pic->Clear();

		l_pic->AllowHighlight(false);
		u_pic->AllowHighlight(false);
		r_pic->AllowHighlight(false);
	}
}


void UI_SideBox::UpdateLabel()
{
	if (! is_sidedef(obj))
	{
		label(is_front ? "   No Front Sidedef" : "   No Back Sidedef");
		return;
	}

	char buffer[200];

	snprintf(buffer, sizeof(buffer), "   %s Sidedef: #%d\n",
			is_front ? "Front" : "Back", obj);

	copy_label(buffer);
}


void UI_SideBox::UpdateAddDel()
{
	if (obj == SETOBJ_NO_LINE || ! config::sidedef_add_del_buttons)
	{
		add_button->hide();
		del_button->hide();
	}
	else if (! is_sidedef(obj))
	{
		add_button->show();
		del_button->hide();
	}
	else
	{
		add_button->hide();
		del_button->show();
	}
}


void UI_SideBox::UpdateHiding()
{
	if (obj < 0)
	{
		x_ofs->hide();
		y_ofs->hide();
		  sec->hide();

		l_tex->hide();
		u_tex->hide();
		r_tex->hide();

		l_pic->hide();
		u_pic->hide();
		r_pic->hide();
	}
	else
	{
		x_ofs->show();
		y_ofs->show();
		  sec->show();

		r_tex->show();
		r_pic->show();

		if (on_2S_line || config::show_full_one_sided)
		{
			r_pic->position(m_x_tile_positions[1], r_pic->y());
			r_tex->Fl_Widget::position(m_x_tile_positions[1] - TEXTURE_TILE_OUTSET, r_tex->y());

			l_tex->show();
			u_tex->show();

			l_pic->show();
			u_pic->show();
		}
		else
		{
			r_pic->position(m_x_tile_positions[0], r_pic->y());
			r_tex->Fl_Widget::position(m_x_tile_positions[0] - TEXTURE_TILE_OUTSET, r_tex->y());

			l_tex->hide();
			u_tex->hide();

			l_pic->hide();
			u_pic->hide();

			l_pic->Unhighlight();
			u_pic->Unhighlight();

			l_pic->Selected(false);
			u_pic->Selected(false);
		}
	}
}


int UI_SideBox::GetSelectedPics() const
{
	if (obj < 0) return 0;

	return	(l_pic->Selected() ? PART_RT_LOWER : 0) |
			(u_pic->Selected() ? PART_RT_UPPER : 0) |
			(r_pic->Selected() ? PART_RT_RAIL  : 0);
}

int UI_SideBox::GetHighlightedPics() const
{
	if (obj < 0) return 0;

	return	(l_pic->Highlighted() ? PART_RT_LOWER : 0) |
			(u_pic->Highlighted() ? PART_RT_UPPER : 0) |
			(r_pic->Highlighted() ? PART_RT_RAIL  : 0);
}


void UI_SideBox::UnselectPics()
{
	l_pic->Unhighlight();
	u_pic->Unhighlight();
	r_pic->Unhighlight();

	l_pic->Selected(false);
	u_pic->Selected(false);
	r_pic->Selected(false);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
