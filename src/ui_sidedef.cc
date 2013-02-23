//------------------------------------------------------------------------
//  SideDef Information Panel
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
#include "w_rawdef.h"
#include "x_loop.h"


// config item
bool swap_sidedefs;

//
// Constructor
//
UI_SideBox::UI_SideBox(int X, int Y, int W, int H, int _side) : 
    Fl_Group(X, Y, W, H),
    obj(SETOBJ_NO_LINE), is_front(_side == 0)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX); // FL_UP_BOX

	align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_LEFT);

	if (is_front)
		labelcolor(FL_BLUE);
	else
		labelcolor(fl_rgb_color(224,64,0));

	
	add_button = new Fl_Button(X + W - 120, Y, 50, 20, "ADD");
	add_button->labelcolor(labelcolor());
	add_button->callback(add_callback, this);

	add(add_button);


	del_button = new Fl_Button(X + W - 60, Y, 50, 20, "DEL");
	del_button->labelcolor(labelcolor());
	del_button->callback(delete_callback, this);

	add(del_button);


	X += 6;
	Y += 6 + 16;  // space for label

	W -= 12;
	H -= 12;

	int MX = X + W/2;


	x_ofs = new Fl_Int_Input(X+20,   Y, 52, 24, "x:");
	y_ofs = new Fl_Int_Input(MX-20,  Y, 52, 24, "y:");
	sec = new Fl_Int_Input(X+W-54, Y, 52, 24, "sec:");

	x_ofs->align(FL_ALIGN_LEFT);
	y_ofs->align(FL_ALIGN_LEFT);
	sec  ->align(FL_ALIGN_LEFT);

	x_ofs->callback(offset_callback, this);
	y_ofs->callback(offset_callback, this);
	sec  ->callback(sector_callback, this);

	x_ofs->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	y_ofs->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	sec  ->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	add(x_ofs); add(y_ofs); add(sec);


	Y += x_ofs->h() + 4;

	int LX = X+8;
	int UX = X+W-64-8;
	    MX = MX-32;
	
	if (swap_sidedefs)
		std::swap(LX, UX);


	l_pic = new UI_Pic(LX, Y, 64, 64);
	m_pic = new UI_Pic(MX, Y, 64, 64);
	u_pic = new UI_Pic(UX, Y, 64, 64);

	l_pic->callback(tex_callback, this);
	m_pic->callback(tex_callback, this);
	u_pic->callback(tex_callback, this);

	Y += 65;


	l_tex = new Fl_Input(LX-8, Y, 80, 20);
	m_tex = new Fl_Input(MX-8, Y, 80, 20);
	u_tex = new Fl_Input(UX-8, Y, 80, 20);

	l_tex->textsize(12);
	m_tex->textsize(12);
	u_tex->textsize(12);

	l_tex->callback(tex_callback, this);
	m_tex->callback(tex_callback, this);
	u_tex->callback(tex_callback, this);

	l_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	m_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	u_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	add(l_pic); add(m_pic); add(u_pic);
	add(l_tex); add(m_tex); add(u_tex);


	Y += 24;


	UpdateHiding(true);
	UpdateLabel();
	UpdateAddDel();
}


//
// Destructor
//
UI_SideBox::~UI_SideBox()
{
}


void UI_SideBox::tex_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	if (box->obj < 0)
		return;

	if (w == box->l_pic ||
	    w == box->m_pic ||
		w == box->u_pic)
	{
		UI_Pic * pic = (UI_Pic *)w;

		pic->Selected(! pic->Selected());
		pic->redraw();

		if (pic->Selected())
			main_win->ShowBrowser('T');
		return;
	}

	int new_tex;

	if (w == box->l_tex)
		new_tex = box->TexFromWidget(box->l_tex);
	else if (w == box->m_tex)
		new_tex = box->TexFromWidget(box->m_tex);
	else
		new_tex = box->TexFromWidget(box->u_tex);

	// iterate over selected linedefs
	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it) ; !it.at_end() ; ++it)
		{
			const LineDef *L = LineDefs[*it];

			int sd = box->is_front ? L->right : L->left;

			if (is_sidedef(sd))
			{
				if (w == box->l_tex)
					BA_ChangeSD(sd, SideDef::F_LOWER_TEX, new_tex);
				else if (w == box->m_tex)
					BA_ChangeSD(sd, SideDef::F_MID_TEX, new_tex);
				else
					BA_ChangeSD(sd, SideDef::F_UPPER_TEX, new_tex);
			}
		}

		BA_End();
		MarkChanges();

		box->UpdateField();
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
	selection_iterator_c it;

	int field = box->is_front ? LineDef::F_RIGHT : LineDef::F_LEFT;

	BA_Begin();

	// iterate through all lines, if this sidedef exists on one of them
	// then use the same sector.
	// DISABLED -- NOT INTUITIVE
#if 0
	int new_sec = -1;

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		const LineDef *L = LineDefs[*it];

		int sd = box->is_front ? L->right : L->left;

		if (sd >= 0)
		{
			new_sec = SideDefs[sd]->sector;
			break;
		}
	}
#endif

	// make sure we have a fallback sector to use
	if (NumSectors == 0)
	{
		int new_sec = BA_New(OBJ_SECTORS);

		Sectors[new_sec]->SetDefaults();
	}


	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		const LineDef *L = LineDefs[*it];

		int sd    = box->is_front ? L->right : L->left;
		int other = box->is_front ? L->left : L->right;

		if (is_sidedef(sd))
			continue;

		// determine what sector to use
		int new_sec = GetOppositeSector(*it, box->is_front);

		if (new_sec < 0)
			new_sec = NumSectors - 1;

		// create the new sidedef
		sd = BA_New(OBJ_SIDEDEFS);

		SideDefs[sd]->SetDefaults(other >= 0);
		SideDefs[sd]->sector = new_sec;

		BA_ChangeLD(*it, field, sd);

		if (other >= 0)
			LD_AddSecondSideDef(*it, sd, other);
	}

	BA_End();

	MarkChanges();

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
	selection_iterator_c it;

	BA_Begin();

	for (edit.Selected->begin(&it) ; !it.at_end() ; ++it)
	{
		const LineDef *L = LineDefs[*it];

		int sd = box->is_front ? L->right : L->left;

		if (sd < 0)
			continue;

		// !!! Note: sidedef itself is not deleted

		LD_RemoveSideDef(*it, box->is_front ? SIDE_RIGHT : SIDE_LEFT);
	}

	BA_End();

	MarkChanges();

	main_win->line_box->UpdateField();
	main_win->line_box->UpdateSides();
}


void UI_SideBox::SetTexture(const char *name, char where)
{
	switch (where)
	{
		case 'l':
			l_tex->value(name);
			l_tex->do_callback();
			break;

		case 'm':
			m_tex->value(name);
			m_tex->do_callback();
			break;

		case 'u':
			u_tex->value(name);
			u_tex->do_callback();
			break;

		default:
			break;
	}
}


void UI_SideBox::offset_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	int new_x_ofs = atoi(box->x_ofs->value());
	int new_y_ofs = atoi(box->y_ofs->value());

	// iterate over selected linedefs
	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it); !it.at_end(); ++it)
		{
			const LineDef *L = LineDefs[*it];

			int sd = box->is_front ? L->right : L->left;

			if (is_sidedef(sd))
			{
				if (w == box->x_ofs)
					BA_ChangeSD(sd, SideDef::F_X_OFFSET, new_x_ofs);
				else
					BA_ChangeSD(sd, SideDef::F_Y_OFFSET, new_y_ofs);
			}
		}

		BA_End();
		MarkChanges();
	}
}


void UI_SideBox::sector_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	int new_sec = atoi(box->sec->value());

	new_sec = CLAMP(0, new_sec, NumSectors-1);

	// iterate over selected linedefs
	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it); !it.at_end(); ++it)
		{
			const LineDef *L = LineDefs[*it];

			int sd = box->is_front ? L->right : L->left;

			if (is_sidedef(sd))
				BA_ChangeSD(sd, SideDef::F_SECTOR, new_sec);
		}

		BA_End();
		MarkChanges();
	}
}


//------------------------------------------------------------------------

void UI_SideBox::SetObj(int index)
{
	if (obj == index)
		return;

	if (obj < 0 && index >= 0)
		UpdateHiding(false);
	else if (obj >= 0 && index < 0)
		UpdateHiding(true);

	obj = index;

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
		x_ofs->value(Int_TmpStr(SideDefs[obj]->x_offset));
		y_ofs->value(Int_TmpStr(SideDefs[obj]->y_offset));
		sec->value(Int_TmpStr(SideDefs[obj]->sector));

		l_tex->value(SideDefs[obj]->LowerTex());
		m_tex->value(SideDefs[obj]->MidTex());
		u_tex->value(SideDefs[obj]->UpperTex());

		l_pic->GetTex(SideDefs[obj]->LowerTex());
		m_pic->GetTex(SideDefs[obj]->MidTex());
		u_pic->GetTex(SideDefs[obj]->UpperTex());
	}
	else
	{
		x_ofs->value("");
		y_ofs->value("");
		sec->value("");

		l_tex->value("");
		m_tex->value("");
		u_tex->value("");

		l_pic->Clear();
		m_pic->Clear();
		u_pic->Clear();
	}
}


void UI_SideBox::UpdateLabel()
{
	if (! is_sidedef(obj))
	{
		label(is_front ? " No Front Sidedef" : " No Back Sidedef");
		return;
	}

	char buffer[200];

	sprintf(buffer, " %s Sidedef: #%d\n",
			is_front ? "Front" : "Back", obj);

	copy_label(buffer);
}


void UI_SideBox::UpdateAddDel()
{
	if (obj == SETOBJ_NO_LINE)
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


void UI_SideBox::UpdateHiding(bool hide)
{
	if (hide)
	{
		x_ofs->hide();
		y_ofs->hide();
		sec->hide();

		l_tex->hide();
		m_tex->hide();
		u_tex->hide();

		l_pic->hide();
		m_pic->hide();
		u_pic->hide();
	}
	else
	{
		x_ofs->show();
		y_ofs->show();
		sec->show();

		l_tex->show();
		m_tex->show();
		u_tex->show();

		l_pic->show();
		m_pic->show();
		u_pic->show();
	}
}


int UI_SideBox::GetSelectedPics() const
{
	return	(l_pic->Selected() ? 1 : 0) |
			(m_pic->Selected() ? 2 : 0) |
			(u_pic->Selected() ? 4 : 0);
}


void UI_SideBox::UnselectPics()
{
	l_pic->Selected(false);
	m_pic->Selected(false);
	u_pic->Selected(false);
}


int UI_SideBox::TexFromWidget(Fl_Input *w)
{
	char name[WAD_TEX_NAME+1];

	memset(name, 0, sizeof(name));

	strncpy(name, w->value(), WAD_TEX_NAME);

	for (int i = 0 ; i < WAD_TEX_NAME ; i++)
		name[i] = toupper(name[i]);

//??	if (name[0] == 0)
//??		strcpy(name, "-");

	return BA_InternaliseString(name);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
