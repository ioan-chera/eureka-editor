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

#include "Instance.h"
#include "main.h"
#include "ui_misc.h"
#include "ui_window.h"

#include "e_hover.h"	// OppositeSector
#include "e_linedef.h"
#include "e_main.h"
#include "LineDef.h"
#include "m_config.h"
#include "m_game.h"
#include "r_render.h"
#include "Sector.h"
#include "SideDef.h"
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
// Check dirty fields for the linedef panel
//
inline static void checkLinedefDirtyFields(const Instance &inst)
{
	inst.main_win->line_box->checkDirtyFields();
}

UI_SideSectionPanel::UI_SideSectionPanel(Instance &inst, int X, int Y, int W, int H,
	const char *label) :
	UI_StackPanel(X, Y, W, H)
{
	spacing(1);
	const int picSize = W - 2 * TEXTURE_TILE_OUTSET;
	pic = new UI_Pic(inst, X + TEXTURE_TILE_OUTSET, 0, picSize, picSize, label);
	tex = new UI_DynInput(X, 0, W, 20);
	tex->textsize(12);
	tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	end();

	relayout();
}

//
// Constructor
//
UI_SideBox::UI_SideBox(Instance &inst, int X, int Y, int W, int H, int _side) :
	Fl_Group(X, Y, W, H),
	is_front(_side == 0),
	inst(inst)
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


	x_ofs = new UI_DynIntInput(X+28,   Y, 52, 24, "x:");
	y_ofs = new UI_DynIntInput(MX-20,  Y, 52, 24, "y:");
	sec = new UI_DynIntInput(X+W-59, Y, 52, 24, "sec:");

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

	l_panel = new UI_SideSectionPanel(inst, LX, Y, 80, 85, "Lower");
	u_panel = new UI_SideSectionPanel(inst, UX, Y, 80, 85, "Upper");
	r_panel = new UI_SideSectionPanel(inst, MX, Y, 80, 85, "Rail");

	l_panel->getPic()->callback(tex_callback, this);
	u_panel->getPic()->callback(tex_callback, this);
	r_panel->getPic()->callback(tex_callback, this);

	Y += 65;

	l_panel->getTex()->callback(tex_callback, this);
	u_panel->getTex()->callback(tex_callback, this);
	r_panel->getTex()->callback(tex_callback, this);

	l_panel->getTex()->callback2(dyntex_callback, this);
	u_panel->getTex()->callback2(dyntex_callback, this);
	r_panel->getTex()->callback2(dyntex_callback, this);

	mFixUp.loadFields({ x_ofs, y_ofs, sec, l_panel->getTex(), u_panel->getTex(), r_panel->getTex() });

	end();


	UpdateHiding();
	UpdateLabel();
	UpdateAddDel();
}


void UI_SideBox::tex_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	if (box->obj < 0)
		return;

	if (Fl::event_button() != FL_RIGHT_MOUSE &&
		(w == box->l_panel->getPic() || w == box->u_panel->getPic() || w == box->r_panel->getPic()))
	{
		UI_Pic * pic = (UI_Pic *)w;

		pic->Selected(! pic->Selected());

		if (pic->Selected())
			box->inst.main_win->BrowserMode(BrowserMode::textures);
		return;
	}

	StringID new_tex;
	box->mFixUp.checkDirtyFields();	// fine to do it here
	checkLinedefDirtyFields(box->inst);

	// right click sets to default value, "-" for rail
	if (Fl::event_button() == FL_RIGHT_MOUSE)
	{
		if (w == box->r_panel->getPic())
			new_tex = BA_InternaliseString("-");
		else
			new_tex = BA_InternaliseString(box->inst.conf.default_wall_tex);
	}
	else
	{
		if (w == box->l_panel->getTex())
			new_tex = BA_InternaliseString(NormalizeTex(box->l_panel->getTex()->value()));
		else if (w == box->u_panel->getTex())
			new_tex = BA_InternaliseString(NormalizeTex(box->u_panel->getTex()->value()));
		else
			new_tex = BA_InternaliseString(NormalizeTex(box->r_panel->getTex()->value()));
	}

	// iterate over selected linedefs
	if (!box->inst.edit.Selected->empty())
	{
		{
			EditOperation op(box->inst.level.basis);
			op.setMessageForSelection("edited texture on", *box->inst.edit.Selected);

			for (sel_iter_c it(*box->inst.edit.Selected) ; !it.done() ; it.next())
			{
				const auto L = box->inst.level.linedefs[*it];

				int sd = box->is_front ? L->right : L->left;

				if (box->inst.level.isSidedef(sd))
				{
					bool lower = (w == box->l_panel->getTex() || w == box->l_panel->getPic());
					bool upper = (w == box->u_panel->getTex() || w == box->u_panel->getPic());
					bool rail  = (w == box->r_panel->getTex() || w == box->r_panel->getPic());


					if (lower)
					{
						op.changeSidedef(sd, SideDef::F_LOWER_TEX, new_tex);
					}
					else if (upper)
					{
						op.changeSidedef(sd, SideDef::F_UPPER_TEX, new_tex);
					}
					else if (rail)
					{
						op.changeSidedef(sd, SideDef::F_MID_TEX,   new_tex);
					}
				}
			}
		}

		box->UpdateField();
	}
}


void UI_SideBox::dyntex_callback(Fl_Widget *w, void *data)
{
	// change picture to match the input, BUT does not change the map

	UI_SideBox *box = (UI_SideBox *)data;

	if (box->obj < 0)
		return;

	if (w == box->l_panel->getTex())
	{
		box->l_panel->getPic()->GetTex(box->l_panel->getTex()->value());
	}
	else if (w == box->u_panel->getTex())
	{
		box->u_panel->getPic()->GetTex(box->u_panel->getTex()->value());
	}
	else if (w == box->r_panel->getTex())
	{
		box->r_panel->getPic()->GetTex(box->r_panel->getTex()->value());
	}
}


void UI_SideBox::add_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	box->mFixUp.checkDirtyFields();
	checkLinedefDirtyFields(box->inst);

	if (box->obj >= 0)
		return;

	if (box->inst.edit.Selected->empty())
		return;

	// iterate over selected linedefs

	int field = box->is_front ? LineDef::F_RIGHT : LineDef::F_LEFT;

	{
		EditOperation op(box->inst.level.basis);

		// make sure we have a fallback sector to use
		if (box->inst.level.numSectors() == 0)
		{
			int new_sec = op.addNew(ObjType::sectors);

			box->inst.level.sectors[new_sec]->SetDefaults(box->inst.conf);
		}

		for (sel_iter_c it(*box->inst.edit.Selected) ; !it.done() ; it.next())
		{
			const auto L = box->inst.level.linedefs[*it];

			int sd    = box->is_front ? L->right : L->left;
			int other = box->is_front ? L->left : L->right;

			// skip lines which already have this sidedef
			if (box->inst.level.isSidedef(sd))
				continue;

			// determine what sector to use
			int new_sec = box->inst.level.hover.getOppositeSector(*it, box->is_front ? Side::right : Side::left, nullptr);

			if (new_sec < 0)
				new_sec = box->inst.level.numSectors() - 1;

			// create the new sidedef
			sd = op.addNew(ObjType::sidedefs);

			box->inst.level.sidedefs[sd]->SetDefaults(box->inst.conf, other >= 0);
			box->inst.level.sidedefs[sd]->sector = new_sec;

			op.changeLinedef(*it, static_cast<byte>(field), sd);

			if (other >= 0)
				box->inst.level.linemod.addSecondSidedef(op, *it, sd, other);
		}

		op.setMessageForSelection("added sidedef to", *box->inst.edit.Selected);
	}

	box->inst.main_win->line_box->UpdateField();
	box->inst.main_win->line_box->UpdateSides();
}


void UI_SideBox::delete_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	box->mFixUp.checkDirtyFields();
	checkLinedefDirtyFields(box->inst);

	if (box->obj < 0)
		return;

	if (box->inst.edit.Selected->empty())
		return;

	// iterate over selected linedefs
	{
		EditOperation op(box->inst.level.basis);

		for (sel_iter_c it(*box->inst.edit.Selected) ; !it.done() ; it.next())
		{
			const auto L = box->inst.level.linedefs[*it];

			int sd = box->is_front ? L->right : L->left;

			if (sd < 0)
				continue;

			// NOTE WELL: the actual sidedef is not deleted (it might be shared)

			box->inst.level.linemod.removeSidedef(op, *it, box->is_front ? Side::right : Side::left);
		}

		op.setMessageForSelection("deleted sidedef from", *box->inst.edit.Selected);
	}

	box->inst.main_win->line_box->UpdateField();
	box->inst.main_win->line_box->UpdateSides();
}


void UI_SideBox::offset_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	int new_x_ofs = atoi(box->x_ofs->value());
	int new_y_ofs = atoi(box->y_ofs->value());

	// iterate over selected linedefs
	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);

		if (w == box->x_ofs)
			op.setMessageForSelection("edited X offset on", *box->inst.edit.Selected);
		else
			op.setMessageForSelection("edited Y offset on", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			const auto L = box->inst.level.linedefs[*it];

			int sd = box->is_front ? L->right : L->left;

			if (box->inst.level.isSidedef(sd))
			{
				if (w == box->x_ofs)
					op.changeSidedef(sd, SideDef::F_X_OFFSET, new_x_ofs);
				else
					op.changeSidedef(sd, SideDef::F_Y_OFFSET, new_y_ofs);
			}
		}
	}
}


void UI_SideBox::sector_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = (UI_SideBox *)data;

	int new_sec = atoi(box->sec->value());

	new_sec = clamp(0, new_sec, box->inst.level.numSectors() -1);

	// iterate over selected linedefs
	if (!box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited sector-ref on", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			const auto L = box->inst.level.linedefs[*it];

			int sd = box->is_front ? L->right : L->left;

			if (box->inst.level.isSidedef(sd))
				op.changeSidedef(sd, SideDef::F_SECTOR, new_sec);
		}
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
	if (inst.level.isSidedef(obj))
	{
		const auto sd = inst.level.sidedefs[obj];

		mFixUp.setInputValue(x_ofs, SString(sd->x_offset).c_str());
		mFixUp.setInputValue(y_ofs, SString(sd->y_offset).c_str());
		mFixUp.setInputValue(sec, SString(sd->sector).c_str());

		SString lower = sd->LowerTex();
		SString rail  = sd->MidTex();
		SString upper = sd->UpperTex();

		mFixUp.setInputValue(l_panel->getTex(), lower.c_str());
		mFixUp.setInputValue(u_panel->getTex(), upper.c_str());
		mFixUp.setInputValue(r_panel->getTex(), rail.c_str());

		l_panel->getPic()->GetTex(lower);
		u_panel->getPic()->GetTex(upper);
		r_panel->getPic()->GetTex(rail);

		if ((what_is_solid & SOLID_LOWER) && is_null_tex(lower))
			l_panel->getPic()->MarkMissing();

		if ((what_is_solid & SOLID_UPPER) && is_null_tex(upper))
			u_panel->getPic()->MarkMissing();

		if (is_special_tex(lower)) l_panel->getPic()->MarkSpecial();
		if (is_special_tex(upper)) u_panel->getPic()->MarkSpecial();
		if (is_special_tex(rail))  r_panel->getPic()->MarkSpecial();

		l_panel->getPic()->AllowHighlight(true);
		u_panel->getPic()->AllowHighlight(true);
		r_panel->getPic()->AllowHighlight(true);
	}
	else
	{
		mFixUp.setInputValue(x_ofs, "");
		mFixUp.setInputValue(y_ofs, "");
		mFixUp.setInputValue(sec, "");

		mFixUp.setInputValue(l_panel->getTex(), "");
		mFixUp.setInputValue(u_panel->getTex(), "");
		mFixUp.setInputValue(r_panel->getTex(), "");

		l_panel->getPic()->Clear();
		u_panel->getPic()->Clear();
		r_panel->getPic()->Clear();

		l_panel->getPic()->AllowHighlight(false);
		u_panel->getPic()->AllowHighlight(false);
		r_panel->getPic()->AllowHighlight(false);
	}
}


void UI_SideBox::UpdateLabel()
{
	if (!inst.level.isSidedef(obj))
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
	else if (!inst.level.isSidedef(obj))
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

//
// Get X position of midtex tile
//
int UI_SideBox::getMidTexX(int position) const
{
	if(position == 0)
		return config::swap_sidedefs ? u_panel->x() : l_panel->x();
	return (l_panel->x() + u_panel->x()) / 2;
}

void UI_SideBox::UpdateHiding()
{
	if (obj < 0)
	{
		x_ofs->hide();
		y_ofs->hide();
		  sec->hide();

		l_panel->hide();
		u_panel->hide();
		r_panel->hide();
	}
	else
	{
		x_ofs->show();
		y_ofs->show();
		  sec->show();

		r_panel->show();

		if (on_2S_line || config::show_full_one_sided)
		{
			r_panel->position(getMidTexX(1), r_panel->y());

			l_panel->show();
			u_panel->show();
		}
		else
		{
			r_panel->position(getMidTexX(0), r_panel->y());	

			l_panel->hide();
			u_panel->hide();

			l_panel->getPic()->Unhighlight();
			u_panel->getPic()->Unhighlight();

			l_panel->getPic()->Selected(false);
			u_panel->getPic()->Selected(false);
		}
	}
}


int UI_SideBox::GetSelectedPics() const
{
	if (obj < 0) return 0;

	return	(l_panel->getPic()->Selected() ? PART_RT_LOWER : 0) |
			(u_panel->getPic()->Selected() ? PART_RT_UPPER : 0) |
			(r_panel->getPic()->Selected() ? PART_RT_RAIL  : 0);
}

int UI_SideBox::GetHighlightedPics() const
{
	if (obj < 0) return 0;

	return	(l_panel->getPic()->Highlighted() ? PART_RT_LOWER : 0) |
			(u_panel->getPic()->Highlighted() ? PART_RT_UPPER : 0) |
			(r_panel->getPic()->Highlighted() ? PART_RT_RAIL  : 0);
}


void UI_SideBox::UnselectPics()
{
	l_panel->getPic()->Unhighlight();
	u_panel->getPic()->Unhighlight();
	r_panel->getPic()->Unhighlight();

	l_panel->getPic()->Selected(false);
	u_panel->getPic()->Selected(false);
	r_panel->getPic()->Selected(false);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
