//------------------------------------------------------------------------
//  VERTEX PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2026      Ioan Chera
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
#include "m_config.h"
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
    MapItemBox(inst,X, Y, W, H, label)
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

	slopeControls = new Fl_Group(X, Y + 84, W, 100);

	ceil_h = new UI_DynInput(X + 70, Y + 112, 64, 24, "Ceiling Slope Z");
	ceil_h->align(FL_ALIGN_TOP);
	ceil_h->callback(slope_height_callback, this);
	ceil_h->vertical_label_margin(8);
	ce_down = new Fl_Button(ceil_h->x() - 42, ceil_h->y() + 1, 30, 22, "-");
	ce_up   = new Fl_Button(ceil_h->x() + ceil_h->w() + 12, ce_down->y(), 30, 22, "+");
	ce_down->labelfont(FL_HELVETICA_BOLD);
	ce_up  ->labelfont(FL_HELVETICA_BOLD);
	ce_down->labelsize(16);
	ce_up  ->labelsize(16);
	ce_down->callback(slope_adjust_button_callback, this);
	ce_up->callback(slope_adjust_button_callback, this);
	clear_ceiling = new Fl_Button(ce_up->x() + ce_up->w() + 12, ce_up->y(), 60, 22, "Clear");
	clear_ceiling->callback(slope_clear_button_callback, this);

	floor_h = new UI_DynInput(X + 70, ceil_h->y() + ceil_h->h() + 10, 64, 24, "Floor Slope Z");
	floor_h->align(FL_ALIGN_BOTTOM);
	ceil_h->callback(slope_height_callback, this);
	floor_h->vertical_label_margin(8);
	fl_down = new Fl_Button(floor_h->x() - 42,  floor_h->y() + 1, 30, 22, "-");
	fl_up   = new Fl_Button(floor_h->x() + floor_h->w() + 12, fl_down->y(), 30, 22, "+");
	fl_down->labelfont(FL_HELVETICA_BOLD);
	fl_up  ->labelfont(FL_HELVETICA_BOLD);
	fl_down->labelsize(16);
	fl_up  ->labelsize(16);
	fl_down->callback(slope_adjust_button_callback, this);
	fl_up->callback(slope_adjust_button_callback, this);
	clear_floor = new Fl_Button(fl_up->x() + fl_up->w() + 12, fl_up->y(), 60, 22, "Clear");
	clear_floor->callback(slope_clear_button_callback, this);

	slopeControls->end();
	slopeControls->resizable(nullptr);
	slopeControls->size(slopeControls->w(), fl_down->y() + fl_down->h() + 28 - slopeControls->y());
	slopeControls->hide();

	mFixUp.loadFields({pos_x, pos_y, ceil_h, floor_h});

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

	double new_x = atof(box->pos_x->value());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessage("edited X of"/*, inst.edit.Selected*/);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			op.changeVertex(*it, &Vertex::xf, MakeValidCoordF(box->inst.loaded.levelFormat, new_x));
		}
	}
}

void UI_VertexBox::y_callback(Fl_Widget *w, void *data)
{
	UI_VertexBox *box = (UI_VertexBox *)data;

	double new_y = atof(box->pos_y->value());

	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessage("edited Y of"/*, inst.edit.Selected*/);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			op.changeVertex(*it, &Vertex::yf, MakeValidCoordF(box->inst.loaded.levelFormat, new_y));
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
		double fdx = MakeValidCoordF(box->inst.loaded.levelFormat, dx * step);
		double fdy = MakeValidCoordF(box->inst.loaded.levelFormat, dy * step);

		box->mFixUp.checkDirtyFields();

		EditOperation op(box->inst.level.basis);
		op.setMessage("adjusted"/*, inst.edit.Selected*/);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			const auto V = box->inst.level.vertices[*it];

			op.changeVertex(*it, &Vertex::xf, V->xf + fdx);
			op.changeVertex(*it, &Vertex::yf, V->yf + fdy);
		}
	}
}


void UI_VertexBox::slope_adjust_button_callback(Fl_Widget *w, void *data)
{
	auto box = static_cast<const UI_VertexBox *>(data);

	if (box->inst.edit.Selected->empty())
		return;

	std::optional<double> Vertex::*field;
	double sign;
	SectorSurface surface;
	if(w == box->ce_down || w == box->ce_up)
	{
		field = &Vertex::zceiling;
		surface = SectorSurface::ceiling;
	}
	else
	{
		field = &Vertex::zfloor;
		surface = SectorSurface::floor;
	}
	if(w == box->ce_down || w == box->fl_down)
		sign = -1;
	else
		sign = +1;

	keycode_t mod = Fl::event_state() & EMOD_ALL_MASK;
	double step;
	if(mod & EMOD_SHIFT)
		step = config::floor_bump_small;
	else if(mod & EMOD_COMMAND)
		step = config::floor_bump_large;
	else
		step = config::floor_bump_medium;
	step *= sign;

	EditOperation op(box->inst.level.basis);
	op.setMessageForSelection("changed Z height of", *box->inst.edit.Selected);
	for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
	{
		const auto &V = box->inst.level.vertices[*it];

		double targetValue;
		if(((*V).*field).has_value())
			targetValue = *((*V).*field) + step;
		else
		{
			std::optional<int> suggestion =
					box->inst.level.vertmod.getTriangleSectorTipHeight(*it, surface);
			if(suggestion.has_value())
				targetValue = *suggestion;
			else
				continue;
		}
		op.changeVertex(*it, field, targetValue);
	}

}


void UI_VertexBox::slope_height_callback(Fl_Widget *w, void *data)
{
	auto box = static_cast<UI_VertexBox *>(data);
	char *endptr = nullptr;
	std::optional<double> f_h = strtod(box->floor_h->value(), &endptr);
	if((endptr && *endptr) || endptr == box->floor_h->value())
		f_h.reset();
	endptr = nullptr;
	std::optional<double> c_h = strtod(box->ceil_h->value(), &endptr);
	if((endptr && *endptr) || endptr == box->ceil_h->value())
		c_h.reset();

	if(box->inst.edit.Selected->empty())
		return;

	EditOperation op(box->inst.level.basis);
	if(w == box->floor_h)
		op.setMessageForSelection("edited floor Z of", *box->inst.edit.Selected);
	else
		op.setMessageForSelection("edited ceiling Z of", *box->inst.edit.Selected);

	for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
	{
		if(w == box->floor_h)
		{
			if(box->inst.level.vertices[*it]->zfloor != f_h)
				op.changeVertex(*it, &Vertex::zfloor, f_h);
		}
		else
		{
			if(box->inst.level.vertices[*it]->zceiling != c_h)
				op.changeVertex(*it, &Vertex::zceiling, c_h);
		}
	}
}


void UI_VertexBox::slope_clear_button_callback(Fl_Widget *w, void *data)
{
	auto box = static_cast<UI_VertexBox *>(data);
	if(box->inst.edit.Selected->empty())
		return;

	std::optional<double> Vertex::*field = w == box->clear_floor ? &Vertex::zfloor :
	                                                               &Vertex::zceiling;
	EditOperation op(box->inst.level.basis);
	op.setMessageForSelection("cleared slope Z of", *box->inst.edit.Selected);
	for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		op.changeVertex(*it, field, std::nullopt);
}


//------------------------------------------------------------------------

void UI_VertexBox::UpdateField()
{
	if (inst.level.isVertex(obj))
	{
		char text[128];
		const auto &V = inst.level.vertices[obj];
		snprintf(text, sizeof(text), "%.16g", V->x());
		mFixUp.setInputValue(pos_x, text);
		snprintf(text, sizeof(text), "%.16g", V->y());
		mFixUp.setInputValue(pos_y, text);

		if(V->zfloor.has_value())
		{
			snprintf(text, sizeof(text), "%.16g", *V->zfloor);
			mFixUp.setInputValue(floor_h, text);
			clear_floor->activate();
		}
		else
		{
			mFixUp.setInputValue(floor_h, "");
			clear_floor->deactivate();
		}
		if(V->zceiling.has_value())
		{
			snprintf(text, sizeof(text), "%.16g", *V->zceiling);
			mFixUp.setInputValue(ceil_h, text);
			clear_ceiling->activate();
		}
		else
		{
			mFixUp.setInputValue(ceil_h, "");
			clear_ceiling->deactivate();
		}
		if(inst.level.vertmod.hasTriangleSectors(obj))
		{
			slopeControls->activate();
		}
		else
		{
			slopeControls->deactivate();
		}
	}
	else
	{
		mFixUp.setInputValue(pos_x, "");
		mFixUp.setInputValue(pos_y, "");
		slopeControls->deactivate();
	}
}

void UI_VertexBox::UpdateGameInfo(const LoadingData &loading, const ConfigData &config)
{
	if(loading.levelFormat == MapFormat::udmf)
	{
		pos_x->type(FL_FLOAT_INPUT);
		pos_y->type(FL_FLOAT_INPUT);
		if(config.features.udmf_vertex_slopes)
			slopeControls->show();
		else
			slopeControls->hide();
	}
	else
	{
		pos_x->type(FL_INT_INPUT);
		pos_y->type(FL_INT_INPUT);
		slopeControls->hide();
	}
}

void UI_VertexBox::UpdateTotal(const Document &doc) noexcept
{
	which->SetTotal(doc.numVertices());
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
