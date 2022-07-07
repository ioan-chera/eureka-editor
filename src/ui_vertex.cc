//------------------------------------------------------------------------
//  VERTEX PANEL
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
#include "m_game.h"
#include "Vertex.h"
#include "w_rawdef.h"


// config items  [ TODO ]
int vertex_bump_small  = 1;
int vertex_bump_medium = 4;
int vertex_bump_large  = 16;


extern const char *const *arrow_pixmaps[8];


//
// UI_VertexBox Constructor
//
UI_VertexBox::UI_VertexBox(Instance &inst, int X, int Y, int W, int H, const char *label) :
    Fl_Group(X, Y, W, H, label), inst(inst)
{
	box(FL_FLAT_BOX);


	X += 6;
	Y += 6;

	W -= 12;
	H -= 10;


	which = new UI_Nombre(X+6, Y, W-12, 28, "Vertex");

	Y += which->h() + 8;


	pos_x = new UI_DynIntInput(X + 64, Y,      75, 24, "x: ");
	pos_y = new UI_DynIntInput(X + 64, Y + 28, 75, 24, "y: ");

	pos_x->align(FL_ALIGN_LEFT);
	pos_y->align(FL_ALIGN_LEFT);

	pos_x->callback(x_callback, this);
	pos_y->callback(y_callback, this);


	int MX = X + W*2/3;

	move_left = new Fl_Button(MX - 30, Y + 14, 24, 24);
	move_left->image(new Fl_Pixmap(arrow_pixmaps[4]));
	move_left->align(FL_ALIGN_CENTER);
	move_left->callback(button_callback, this);

	move_up = new Fl_Button(MX, Y, 24, 24);
	move_up->image(new Fl_Pixmap(arrow_pixmaps[2]));
	move_up->align(FL_ALIGN_CENTER);
	move_up->callback(button_callback, this);

	move_down = new Fl_Button(MX, Y + 28, 24, 24);
	move_down->image(new Fl_Pixmap(arrow_pixmaps[6]));
	move_down->align(FL_ALIGN_CENTER);
	move_down->callback(button_callback, this);

	move_right = new Fl_Button(MX + 30, Y + 14, 24, 24);
	move_right->image(new Fl_Pixmap(arrow_pixmaps[0]));
	move_right->align(FL_ALIGN_CENTER);
	move_right->callback(button_callback, this);

	mFixUp.loadFields({pos_x, pos_y});

	end();

	resizable(NULL);
}


int UI_VertexBox::handle(int event)
{
	return Fl_Group::handle(event);
}


void UI_VertexBox::x_callback(Fl_Widget *w, void *data)
{
	UI_VertexBox *box = (UI_VertexBox *)data;

	int new_x = atoi(box->pos_x->value());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessage("edited X of"/*, inst.edit.Selected*/);

		for (sel_iter_c it(box->inst.edit.Selected); !it.done(); it.next())
		{
			op.changeVertex(*it, Vertex::F_X, MakeValidCoord(box->inst.loaded.levelFormat, new_x));
		}
	}
}

void UI_VertexBox::y_callback(Fl_Widget *w, void *data)
{
	UI_VertexBox *box = (UI_VertexBox *)data;

	int new_y = atoi(box->pos_y->value());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessage("edited Y of"/*, inst.edit.Selected*/);

		for (sel_iter_c it(box->inst.edit.Selected); !it.done(); it.next())
		{
			op.changeVertex(*it, Vertex::F_Y, MakeValidCoord(box->inst.loaded.levelFormat, new_y));
		}
	}
}


void UI_VertexBox::button_callback(Fl_Widget *w, void *data)
{
	UI_VertexBox *box = (UI_VertexBox *)data;

	int dx = 0;
	int dy = 0;

	if (w == box->move_left)  dx = -1;
	if (w == box->move_right) dx = +1;
	if (w == box->move_up)    dy = +1;
	if (w == box->move_down)  dy = -1;

	keycode_t mod = Fl::event_state() & EMOD_ALL_MASK;

	int step = vertex_bump_medium;

	if (mod & EMOD_SHIFT)
		step = vertex_bump_small;
	else if (mod & EMOD_COMMAND)
		step = vertex_bump_large;

	if (!box->inst.edit.Selected->empty())
	{
		FFixedPoint fdx = MakeValidCoord(box->inst.loaded.levelFormat, dx * step);
		FFixedPoint fdy = MakeValidCoord(box->inst.loaded.levelFormat, dy * step);

		box->mFixUp.checkDirtyFields();

		EditOperation op(box->inst.level.basis);
		op.setMessage("adjusted"/*, inst.edit.Selected*/);

		for (sel_iter_c it(box->inst.edit.Selected); !it.done(); it.next())
		{
			const Vertex *V = box->inst.level.vertices[*it];

			op.changeVertex(*it, Vertex::F_X, V->raw_x + fdx);
			op.changeVertex(*it, Vertex::F_Y, V->raw_y + fdy);
		}
	}
}


//------------------------------------------------------------------------

void UI_VertexBox::SetObj(int _index, int _count)
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

void UI_VertexBox::UpdateField()
{
	if (inst.level.isVertex(obj))
	{
		// @@ FIXME show decimals in UDMF
		mFixUp.setInputValue(pos_x, SString(static_cast<int>(inst.level.vertices[obj]->x())).c_str());
		mFixUp.setInputValue(pos_y, SString(static_cast<int>(inst.level.vertices[obj]->y())).c_str());
	}
	else
	{
		mFixUp.setInputValue(pos_x, "");
		mFixUp.setInputValue(pos_y, "");
	}
}


void UI_VertexBox::UpdateTotal()
{
	which->SetTotal(inst.level.numVertices());
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
