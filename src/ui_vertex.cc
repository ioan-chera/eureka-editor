//------------------------------------------------------------------------
//  Vertex Panel + Default Props
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


class UI_DefaultProps : public Fl_Group
{
private:
	UI_Pic   *l_pic;
	UI_Pic   *m_pic;
	UI_Pic   *u_pic;

	Fl_Input *l_tex;
	Fl_Input *m_tex;
	Fl_Input *u_tex;

	static void tex_callback(Fl_Widget *w, void *data)
	{ /* TODO */ }

public:
	UI_DefaultProps(int X, int Y, int W, int H) :
		Fl_Group(X, Y, W, H, NULL)
	{
		box(FL_FLAT_BOX);


		Fl_Box *title = new Fl_Box(X + 10, Y + 10, W - 20, 30, "Default Properties");
		title->labelsize(18+KF*4);

		Y += 50;
		X += 6;
		W -= 12;

		int MX = X + W/2;


		Fl_Box *line_tit = new Fl_Box(X, Y, W, 30, "Linedef textures:");
		line_tit->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

		Y += line_tit->h() + 2;


		l_pic = new UI_Pic(X+4,      Y, 64, 64);
		m_pic = new UI_Pic(MX-32,    Y, 64, 64);
		u_pic = new UI_Pic(X+W-64-4, Y, 64, 64);

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

		Y += l_tex->h() + 8;


		Fl_Box *sec_tit = new Fl_Box(X, Y, W, 30, "Sector props:");
		sec_tit->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

		Y += sec_tit->h() + 2;


		Fl_Box *rs_box = new Fl_Box(FL_NO_BOX, X + 10, Y + H - 16, W - 20, 12, NULL);

		resizable(rs_box);

		end();
	}

	virtual ~UI_DefaultProps()
	{ }
};



//------------------------------------------------------------------------

//
// UI_VertexBox Constructor
//
UI_VertexBox::UI_VertexBox(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    obj(-1), count(0)
{
	box(FL_FLAT_BOX);

	color(WINDOW_BG, WINDOW_BG);


	int top_h = 112;

	idefs = new UI_DefaultProps(X, Y + top_h + 4, W, H - top_h - 4);

	resizable(idefs);


	Fl_Group *top_bit = new Fl_Group(X, Y, W, top_h);

	top_bit->box(FL_FLAT_BOX);


	X += 6;
	Y += 5;

	W -= 12;
	H -= 10;

	int MX = X + W/2;


	which = new UI_Nombre(X, Y, W-10, 28, "Vertex");

	Y += which->h() + 4;


	pos_x = new Fl_Int_Input(X +28, Y, 80, 24, "x: ");
	pos_y = new Fl_Int_Input(MX +28, Y, 80, 24, "y: ");

	pos_x->align(FL_ALIGN_LEFT);
	pos_y->align(FL_ALIGN_LEFT);

	pos_x->callback(x_callback, this);
	pos_y->callback(y_callback, this);

	Y += pos_y->h() + 4;

	top_bit->end();

	end();
}


//
// UI_VertexBox Destructor
//
UI_VertexBox::~UI_VertexBox()
{
}


int UI_VertexBox::handle(int event)
{
	return Fl_Group::handle(event);
}


void UI_VertexBox::x_callback(Fl_Widget *w, void *data)
{
	UI_VertexBox *box = (UI_VertexBox *)data;

	int new_x = atoi(box->pos_x->value());

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
		
		for (list.begin(&it); !it.at_end(); ++it)
			BA_ChangeVT(*it, Vertex::F_X, new_x);

		BA_End();
		MarkChanges();
	}
}

void UI_VertexBox::y_callback(Fl_Widget *w, void *data)
{
	UI_VertexBox *box = (UI_VertexBox *)data;

	int new_y = atoi(box->pos_y->value());

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
		
		for (list.begin(&it); !it.at_end(); ++it)
			BA_ChangeVT(*it, Vertex::F_Y, new_y);

		BA_End();
		MarkChanges();
	}
}


//------------------------------------------------------------------------

void UI_VertexBox::SetObj(int _index, int _count)
{
	if (_count == 0)
		_index = -1;

	if (obj == _index && count == _count)
		return;

	obj   = _index;
	count = _count;

	which->SetIndex(obj);
	which->SetSelected(count);

	UpdateField();

	redraw();
}

void UI_VertexBox::UpdateField()
{
	if (is_vertex(obj))
	{
		pos_x->value(Int_TmpStr(Vertices[obj]->x));
		pos_y->value(Int_TmpStr(Vertices[obj]->y));
	}
	else
	{
		pos_x->value("");
		pos_y->value("");
	}
}


void UI_VertexBox::UpdateTotal()
{
	which->SetTotal(NumVertices);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
