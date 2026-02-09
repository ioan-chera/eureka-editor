//------------------------------------------------------------------------
//  SIDEDEF INFORMATION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
//  Copyright (C) 2026 Ioan Chera
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

#include "ui_category_button.h"

#include "FL/Fl_Check_Button.H"
#include "FL/Fl_Grid.H"
#include "FL/Fl_Light_Button.H"
#include "FL/Fl_Flex.H"

enum
{
	TEXTURE_TILE_OUTSET = 8,
	FLAG_LABELSIZE = 12,
	FLAG_ROW_HEIGHT = 19,
	FIELD_HEIGHT = 22,

	UDMF_PANEL_WIDTH = 80,
};

struct SideFeatureFlagMapping
{
	UDMF_SideFeature feature;
	const char *label;
	const char *tooltip;
	unsigned value;
	const char *shortDisplay;
};


// config item
bool config::swap_sidedefs = false;
bool config::show_full_one_sided = false;
bool config::sidedef_add_del_buttons = false;

static const SideFeatureFlagMapping sidedefFlags[] =
{
	{UDMF_SideFeature::clipmidtex, "clip railing texture",
	"Clip two-sided middle texture to floor and ceiling on this sidedef",
	SideDef::FLAG_CLIPMIDTEX, "✂️"},
	{UDMF_SideFeature::nofakecontrast, "no fake contrast",
	"Disable fake contrast light effect on this sidedef",
	SideDef::FLAG_NOFAKECONTRAST, "◑"},
};

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

	l_panel = new UI_SideSectionPanel(inst, LX, Y, UDMF_PANEL_WIDTH, 85, "Lower");
	u_panel = new UI_SideSectionPanel(inst, UX, Y, UDMF_PANEL_WIDTH, 85, "Upper");
	r_panel = new UI_SideSectionPanel(inst, MX, Y, UDMF_PANEL_WIDTH, 85, "Rail");

	l_panel->getPic()->callback(tex_callback, this);
	u_panel->getPic()->callback(tex_callback, this);
	r_panel->getPic()->callback(tex_callback, this);

	Y += 85;

	l_panel->getTex()->callback(tex_callback, this);
	u_panel->getTex()->callback(tex_callback, this);
	r_panel->getTex()->callback(tex_callback, this);

	l_panel->getTex()->callback2(dyntex_callback, this);
	u_panel->getTex()->callback2(dyntex_callback, this);
	r_panel->getTex()->callback2(dyntex_callback, this);

	mFixUp.loadFields({ x_ofs, y_ofs, sec, l_panel->getTex(), u_panel->getTex(), r_panel->getTex() });

	// Master stack panel for collapsible sections below texture panels
	mMasterStack = new UI_StackPanel(X, Y, W, 0);

	// 1. Category header for UDMF per-part fields
	mPanelsHeader = new UI_CategoryButton(X, 0, W, FIELD_HEIGHT, "Per part details");
	mPanelsHeader->setExpanded(false);
	mPanelsHeader->callback(panels_category_callback, this);

	// 2. Horizontal container for the three UDMF panels
	mUdmfContainer = new Fl_Group(X, 0, W, 0);
	mUdmfContainer->resizable(nullptr);
	l_udmf_panel = new UI_StackPanel(LX, 0, UDMF_PANEL_WIDTH, 0);
	l_udmf_panel->end();
	u_udmf_panel = new UI_StackPanel(UX, 0, UDMF_PANEL_WIDTH, 0);
	u_udmf_panel->end();
	r_udmf_panel = new UI_StackPanel(MX, 0, UDMF_PANEL_WIDTH, 0);
	r_udmf_panel->end();
	mUdmfContainer->end();

	// 3. Flags category header
	mFlagsHeader = new UI_CategoryButton(X, 0, W, FIELD_HEIGHT, "Side flags");
	mFlagsHeader->setExpanded(false);
	mFlagsHeader->callback(flag_category_callback, this);

	// 4. Flags grid (populated in UpdateGameInfo)
	mFlagsGrid = new Fl_Grid(X + 8, 0, W - 8, 0);
	mFlagsGrid->layout(0, 2);

	// Create all possible flag buttons in constructor
	for(const SideFeatureFlagMapping &entry : sidedefFlags)
	{
		SideFlagButton sfb{};
		sfb.button = new Fl_Check_Button(0, 0, 0, 0, entry.label);
		sfb.button->labelsize(FLAG_LABELSIZE);
		sfb.button->tooltip(entry.tooltip);
		sfb.button->callback(side_flag_callback, this);
		sfb.feature = entry.feature;
		sfb.mask = entry.value;
		mFlagButtons.push_back(std::move(sfb));
	}

	mFlagsGrid->end();

	mMasterStack->end();

	// Initially hide collapsible sections
	mPanelsHeader->hide();
	mUdmfContainer->hide();
	mFlagsHeader->hide();
	mFlagsGrid->hide();

	end();

	resizable(nullptr);


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

		// Populate UDMF sidepart field widgets
		populateUDMFFieldValues();

		updateSideFlagValues();
		updateSideFlagSummary();
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

		// Clear UDMF sidepart field widgets
		auto clearFields = [this](const std::vector<SidepartFieldWidgets> &fieldList)
		{
			for(const auto &field : fieldList)
			{
				for(size_t dimIdx = 0; dimIdx < field.widgets.size(); ++dimIdx)
				{
					if(field.info.type == sidefield_t::Type::floatType)
					{
						auto input = static_cast<UI_DynFloatInput *>(field.widgets[dimIdx].get());
						mFixUp.setInputValue(input, "");
					}
					else if(field.info.type == sidefield_t::Type::intType)
					{
						auto input = static_cast<UI_DynIntInput *>(field.widgets[dimIdx].get());
						mFixUp.setInputValue(input, "");
					}
					else if(field.info.type == sidefield_t::Type::boolType)
					{
						auto button = static_cast<Fl_Light_Button *>(field.widgets[dimIdx].get());
						button->value(0);
					}
				}
			}
		};

		clearFields(l_udmf_fields);
		clearFields(u_udmf_fields);
		clearFields(r_udmf_fields);

		updateSideFlagValues();
		updateSideFlagSummary();
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

//
// Recompute UI_SideBox height from visible content
//
void UI_SideBox::adjustHeight()
{
	mMasterStack->relayout();

	// Find the bottom of all visible content
	int bottom = y() + 24; // minimum for label area

	if(x_ofs->visible())
	{
		// Texture panels are visible
		bottom = std::max(bottom, l_panel->y() + l_panel->h());
		if(u_panel->visible())
			bottom = std::max(bottom, u_panel->y() + u_panel->h());
		if(r_panel->visible())
			bottom = std::max(bottom, r_panel->y() + r_panel->h());
	}

	if(mMasterStack->visible() && mMasterStack->h() > 0)
		bottom = std::max(bottom, mMasterStack->y() + mMasterStack->h());

	resize(x(), y(), w(), bottom - y() + 3);
}

void UI_SideBox::UpdateHiding()
{
	if(obj < 0)
	{
		x_ofs->hide();
		y_ofs->hide();
		  sec->hide();

		l_panel->hide();
		u_panel->hide();
		r_panel->hide();

		mMasterStack->hide();
	}
	else
	{
		x_ofs->show();
		y_ofs->show();
		  sec->show();

		// Texture panels
		r_panel->show();
		if(on_2S_line || config::show_full_one_sided)
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

		mMasterStack->show();

		// UDMF panels visibility depends on having fields and expansion state
		bool hasPanels = !l_udmf_fields.empty() || !u_udmf_fields.empty() || !r_udmf_fields.empty();
		if(hasPanels)
		{
			mPanelsHeader->show();
			if(mPanelsHeader->isExpanded())
				mUdmfContainer->show();
			else
				mUdmfContainer->hide();
		}
		else
		{
			mPanelsHeader->hide();
			mUdmfContainer->hide();
		}

		// Flags visibility
		bool anyFlagVisible = false;
		for(const auto &sfb : mFlagButtons)
		{
			if(sfb.button->visible())
			{
				anyFlagVisible = true;
				break;
			}
		}

		if(anyFlagVisible)
		{
			mFlagsHeader->show();
			if(mFlagsHeader->isExpanded())
				mFlagsGrid->show();
			else
				mFlagsGrid->hide();
		}
		else
		{
			mFlagsHeader->hide();
			mFlagsGrid->hide();
		}
	}

	adjustHeight();
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

void UI_SideBox::cleanupSideFlags()
{
	// No longer needed - flags are created in constructor and reused
}

void UI_SideBox::loadSideFlags(const LoadingData &loaded, const ConfigData &config)
{
	if(loaded.levelFormat != MapFormat::udmf)
	{
		mFlagsHeader->hide();
		mFlagsGrid->hide();
		return;
	}

	// Count and show/hide individual flag buttons based on game support
	int countFound = 0;
	for(auto &sfb : mFlagButtons)
	{
		if(UDMF_HasSideFeature(config, sfb.feature))
		{
			sfb.button->show();
			++countFound;
		}
		else
		{
			sfb.button->hide();
		}
	}

	if(!countFound)
	{
		mFlagsHeader->hide();
		mFlagsGrid->hide();
		return;
	}

	// Update grid layout based on visible buttons
	int numRows = (countFound + 1) / 2;
	mFlagsGrid->size(mFlagsGrid->w(), FLAG_ROW_HEIGHT * numRows);
	mFlagsGrid->layout(numRows, 2);

	// Add visible buttons to grid
	int index = 0;
	for(auto &sfb : mFlagButtons)
	{
		if(UDMF_HasSideFeature(config, sfb.feature))
		{
			mFlagsGrid->add(sfb.button);
			mFlagsGrid->widget(sfb.button, index % numRows, index / numRows);
			++index;
		}
	}

	mFlagsHeader->show();
}

void UI_SideBox::updateSideFlagValues()
{
	if(mFlagButtons.empty())
		return;

	if(!inst.level.isSidedef(obj))
	{
		for(auto &sfb : mFlagButtons)
			sfb.button->value(0);
		return;
	}

	const auto sd = inst.level.sidedefs[obj];
	for(auto &sfb : mFlagButtons)
		sfb.button->value(!!(sd->flags & sfb.mask));
}

void UI_SideBox::updateSideFlagSummary()
{
	if(!inst.level.isSidedef(obj))
	{
		mFlagsHeader->details("");
		mFlagsHeader->redraw();
		return;
	}

	const int flags = inst.level.sidedefs[obj]->flags;
	SString summary;
	for(size_t i = 0; i < lengthof(sidedefFlags); ++i)
	{
		if(!UDMF_HasSideFeature(inst.conf, sidedefFlags[i].feature))
			continue;
		if(flags & sidedefFlags[i].value)
			summary += sidedefFlags[i].shortDisplay;
	}
	mFlagsHeader->details(summary);
	mFlagsHeader->redraw();
}

void UI_SideBox::cleanupUDMFPanels()
{
	// Unload existing UDMF field widgets from fixUp and remove them
	struct FieldPanelPair
	{
		std::vector<SidepartFieldWidgets> *fields;
		UI_StackPanel *panel;
	};

	const FieldPanelPair pairs[] =
	{
		{&l_udmf_fields, l_udmf_panel},
		{&u_udmf_fields, u_udmf_panel},
		{&r_udmf_fields, r_udmf_panel}
	};

	for(const FieldPanelPair &pair : pairs)
	{
		for(const SidepartFieldWidgets &field : *pair.fields)
		{
			for(const std::unique_ptr<Fl_Widget> &widget : field.widgets)
			{
				if(field.info.type == sidefield_t::Type::intType)
					mFixUp.unloadFields({static_cast<UI_DynIntInput *>(widget.get())});
				else if(field.info.type == sidefield_t::Type::floatType)
					mFixUp.unloadFields({static_cast<UI_DynFloatInput *>(widget.get())});
			}
			if(field.container)
				pair.panel->remove(field.container.get());
		}
		pair.fields->clear();
	}
}

void UI_SideBox::updateUDMFPanels(const LoadingData &loaded, const ConfigData &config)
{
	cleanupUDMFPanels();

	// Only create widgets for UDMF format
	if(loaded.levelFormat != MapFormat::udmf || config.udmf_sidepart_fields.empty())
	{
		l_udmf_panel->relayout();
		u_udmf_panel->relayout();
		r_udmf_panel->relayout();
		return;
	}

	UI_StackPanel *panels[] = { l_udmf_panel, u_udmf_panel, r_udmf_panel };
	std::vector<SidepartFieldWidgets> *fields[] = { &l_udmf_fields, &u_udmf_fields, &r_udmf_fields};

	for(int panelIdx = 0; panelIdx < 3; ++panelIdx)
	{
		auto panel = panels[panelIdx];
		auto &fieldList = *fields[panelIdx];

		panel->begin();

		for(const sidefield_t &sf : config.udmf_sidepart_fields)
		{
			SidepartFieldWidgets fieldWidgets;
			fieldWidgets.info = sf;

			// Create a horizontal flex container for this field's widgets
			auto flex = new Fl_Flex(panel->x(), 0, UDMF_PANEL_WIDTH, 20, Fl_Flex::HORIZONTAL);
			flex->gap(2);
			fieldWidgets.container = std::unique_ptr<Fl_Group>(flex);

			// Create one widget per dimension
			for(size_t i = 0; i < sf.prefixes.size(); ++i)
			{
				Fl_Widget *widget = nullptr;

				switch(sf.type)
				{
				case sidefield_t::Type::boolType:
				{
					auto lightBtn = new Fl_Light_Button(0, 0, 0, 20);
					lightBtn->callback(udmf_field_callback, this);
					if(sf.prefixes.size() == 1)
						lightBtn->copy_label(sf.label.c_str());
					widget = lightBtn;
					break;
				}
				case sidefield_t::Type::intType:
				{
					auto input = new UI_DynIntInput(0, 0, 0, 20);
					input->callback(udmf_field_callback, this);
					input->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
					mFixUp.loadFields({input});
					widget = input;
					break;
				}
				case sidefield_t::Type::floatType:
				{
					auto input = new UI_DynFloatInput(0, 0, 0, 20);
					input->callback(udmf_field_callback, this);
					input->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
					mFixUp.loadFields({input});
					widget = input;
					break;
				}
				}

				if(widget)
					fieldWidgets.widgets.push_back(std::unique_ptr<Fl_Widget>(widget));
			}

			flex->end();

			if(sf.type != sidefield_t::Type::boolType || sf.prefixes.size() > 1)
			{
				// Set label above the flex, centered
				flex->copy_label(sf.label.c_str());
				flex->align(FL_ALIGN_TOP | FL_ALIGN_CENTER);
				panel->beforeSpacing(flex, 14); // Leave gap for the label above
			}

			fieldList.push_back(std::move(fieldWidgets));
		}

		panel->end();
		panel->relayout();
	}
}

void UI_SideBox::populateUDMFFieldValues()
{
	if(!inst.level.isSidedef(obj))
		return;

	const auto sd = inst.level.sidedefs[obj];

	// Populate UDMF sidepart field widgets
	auto populateFields = [this, &sd](const std::vector<SidepartFieldWidgets> &fieldList,
									  const char *partSuffix)
	{
		for(const auto &field : fieldList)
		{
			for(size_t dimIdx = 0; dimIdx < field.widgets.size(); ++dimIdx)
			{
				SString fieldName = field.info.prefixes[dimIdx] + partSuffix;

				double value = 0.0;
				int flag = 0;
				if(fieldName.noCaseEqual("offsetx_top"))
					value = sd->offsetx_top;
				else if(fieldName.noCaseEqual("offsety_top"))
					value = sd->offsety_top;
				else if(fieldName.noCaseEqual("offsetx_mid"))
					value = sd->offsetx_mid;
				else if(fieldName.noCaseEqual("offsety_mid"))
					value = sd->offsety_mid;
				else if(fieldName.noCaseEqual("offsetx_bottom"))
					value = sd->offsetx_bottom;
				else if(fieldName.noCaseEqual("offsety_bottom"))
					value = sd->offsety_bottom;
				else if(fieldName.noCaseEqual("scalex_top"))
					value = sd->scalex_top;
				else if(fieldName.noCaseEqual("scaley_top"))
					value = sd->scaley_top;
				else if(fieldName.noCaseEqual("scalex_mid"))
					value = sd->scalex_mid;
				else if(fieldName.noCaseEqual("scaley_mid"))
					value = sd->scaley_mid;
				else if(fieldName.noCaseEqual("scalex_bottom"))
					value = sd->scalex_bottom;
				else if(fieldName.noCaseEqual("scaley_bottom"))
					value = sd->scaley_bottom;
				else if(fieldName.noCaseEqual("light_top"))
					value = sd->light_top;
				else if(fieldName.noCaseEqual("light_mid"))
					value = sd->light_mid;
				else if(fieldName.noCaseEqual("light_bottom"))
					value = sd->light_bottom;
				else if(fieldName.noCaseEqual("lightabsolute_top"))
					flag = SideDef::FLAG_LIGHT_ABSOLUTE_TOP;
				else if(fieldName.noCaseEqual("lightabsolute_mid"))
					flag = SideDef::FLAG_LIGHT_ABSOLUTE_MID;
				else if(fieldName.noCaseEqual("lightabsolute_bottom"))
					flag = SideDef::FLAG_LIGHT_ABSOLUTE_BOTTOM;

				if(field.info.type == sidefield_t::Type::floatType)
				{
					auto input = static_cast<UI_DynFloatInput *>(field.widgets[dimIdx].get());
					mFixUp.setInputValue(input, SString::printf("%g", value).c_str());
				}
				else if(field.info.type == sidefield_t::Type::intType)
				{
					auto input = static_cast<UI_DynIntInput *>(field.widgets[dimIdx].get());
					mFixUp.setInputValue(input, SString(static_cast<int>(value)).c_str());
				}
				else if(field.info.type == sidefield_t::Type::boolType)
				{
					auto button = static_cast<Fl_Light_Button *>(field.widgets[dimIdx].get());
					button->value(!!(sd->flags & flag));
				}
			}
		}
	};

	populateFields(l_udmf_fields, "bottom");
	populateFields(u_udmf_fields, "top");
	populateFields(r_udmf_fields, "mid");
}

void UI_SideBox::side_flag_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = static_cast<UI_SideBox *>(data);

	if(box->obj < 0)
		return;

	if(box->inst.edit.Selected->empty())
		return;

	// Find which flag button was clicked
	for(const auto &sfb : box->mFlagButtons)
	{
		if(sfb.button != w)
			continue;

		bool newValue = sfb.button->value() != 0;

		box->mFixUp.checkDirtyFields();
		checkLinedefDirtyFields(box->inst);

		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited sidedef flags on", *box->inst.edit.Selected);

		for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			const auto L = box->inst.level.linedefs[*it];
			int sd = box->is_front ? L->right : L->left;

			if(!box->inst.level.isSidedef(sd))
				continue;

			int oldFlags = box->inst.level.sidedefs[sd]->flags;
			int newFlags = newValue ? (oldFlags | sfb.mask) : (oldFlags & ~sfb.mask);
			op.changeSidedef(sd, SideDef::F_FLAGS, newFlags);
		}

		return;
	}
}

void UI_SideBox::flag_category_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = static_cast<UI_SideBox *>(data);

	if(box->mFlagsHeader->isExpanded())
		box->mFlagsGrid->show();
	else
		box->mFlagsGrid->hide();

	box->adjustHeight();
	box->inst.main_win->line_box->redraw();
}

void UI_SideBox::panels_category_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = static_cast<UI_SideBox *>(data);

	if(box->mPanelsHeader->isExpanded())
	{
		box->mUdmfContainer->show();
		printf("%d %d %d %d\n", box->mUdmfContainer->x(), box->mUdmfContainer->y(), box->mUdmfContainer->w(), box->mUdmfContainer->h());
		printf("(%d %d %d %d)\n", box->l_udmf_panel->x(), box->l_udmf_panel->y(), box->l_udmf_panel->w(), box->l_udmf_panel->h());
	}
	else
		box->mUdmfContainer->hide();

	box->adjustHeight();
	box->inst.main_win->line_box->redraw();
}

void UI_SideBox::UpdateGameInfo(const LoadingData &loaded, const ConfigData &config)
{
	// Update UDMF sidepart widgets
	updateUDMFPanels(loaded, config);

	// Update UDMF container height to max panel height
	bool hasSidepartFields = loaded.levelFormat == MapFormat::udmf && !config.udmf_sidepart_fields.empty();
	if(hasSidepartFields)
	{
		int maxH = std::max({l_udmf_panel->h(), u_udmf_panel->h(), r_udmf_panel->h()});
		mUdmfContainer->size(mUdmfContainer->w(), maxH);
		if(obj >= 0)
			mPanelsHeader->show();
	}
	else
	{
		mPanelsHeader->hide();
		mUdmfContainer->hide();
	}

	// Load sidedef-level flags
	loadSideFlags(loaded, config);

	adjustHeight();
	redraw();
}


void UI_SideBox::udmf_field_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = static_cast<UI_SideBox *>(data);

	if(box->obj < 0)
		return;

	if(box->inst.edit.Selected->empty())
		return;

	// Find which widget was triggered and which panel it belongs to
	std::vector<SidepartFieldWidgets> *fieldLists[] = { &box->l_udmf_fields, &box->u_udmf_fields, &box->r_udmf_fields };
	static const char *partSuffixes[] = { "bottom", "top", "mid" };

	for(int panelIdx = 0; panelIdx < 3; ++panelIdx)
	{
		const auto &fields = *fieldLists[panelIdx];
		for(const auto &field : fields)
		{
			for(size_t dimIdx = 0; dimIdx < field.widgets.size(); ++dimIdx)
			{
				if(field.widgets[dimIdx].get() != w)
					continue;

				// Found the widget - build the UDMF field name
				SString fieldName = field.info.prefixes[dimIdx] + partSuffixes[panelIdx];

				// Get the new value and apply it
				EditOperation op(box->inst.level.basis);
				SString message = SString("edited ") + fieldName + " on";
				op.setMessageForSelection(message.c_str(), *box->inst.edit.Selected);

				for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
				{
					const auto L = box->inst.level.linedefs[*it];
					int sd = box->is_front ? L->right : L->left;

					if(!box->inst.level.isSidedef(sd))
						continue;

					// Apply based on field type and field name
					if(field.info.type == sidefield_t::Type::floatType)
					{
						auto input = static_cast<UI_DynFloatInput *>(w);
						double newValue = atof(input->value());

						// Match field name to SideDef member
						if(fieldName.noCaseEqual("offsetx_top"))
							op.changeSidedef(sd, &SideDef::offsetx_top, newValue);
						else if(fieldName.noCaseEqual("offsety_top"))
							op.changeSidedef(sd, &SideDef::offsety_top, newValue);
						else if(fieldName.noCaseEqual("offsetx_mid"))
							op.changeSidedef(sd, &SideDef::offsetx_mid, newValue);
						else if(fieldName.noCaseEqual("offsety_mid"))
							op.changeSidedef(sd, &SideDef::offsety_mid, newValue);
						else if(fieldName.noCaseEqual("offsetx_bottom"))
							op.changeSidedef(sd, &SideDef::offsetx_bottom, newValue);
						else if(fieldName.noCaseEqual("offsety_bottom"))
							op.changeSidedef(sd, &SideDef::offsety_bottom, newValue);
						else if(fieldName.noCaseEqual("scalex_top"))
							op.changeSidedef(sd, &SideDef::scalex_top, newValue);
						else if(fieldName.noCaseEqual("scaley_top"))
							op.changeSidedef(sd, &SideDef::scaley_top, newValue);
						else if(fieldName.noCaseEqual("scalex_mid"))
							op.changeSidedef(sd, &SideDef::scalex_mid, newValue);
						else if(fieldName.noCaseEqual("scaley_mid"))
							op.changeSidedef(sd, &SideDef::scaley_mid, newValue);
						else if(fieldName.noCaseEqual("scalex_bottom"))
							op.changeSidedef(sd, &SideDef::scalex_bottom, newValue);
						else if(fieldName.noCaseEqual("scaley_bottom"))
							op.changeSidedef(sd, &SideDef::scaley_bottom, newValue);
					}
					else if(field.info.type == sidefield_t::Type::intType)
					{
						auto input = static_cast<UI_DynIntInput *>(w);
						int newValue = atoi(input->value());
						if(fieldName.noCaseEqual("light_top"))
							op.changeSidedef(sd, SideDef::F_LIGHT_TOP, newValue);
						if(fieldName.noCaseEqual("light_mid"))
							op.changeSidedef(sd, SideDef::F_LIGHT_MID, newValue);
						if(fieldName.noCaseEqual("light_bottom"))
							op.changeSidedef(sd, SideDef::F_LIGHT_BOTTOM, newValue);
					}
					else if(field.info.type == sidefield_t::Type::boolType)
					{
						auto lightBtn = static_cast<Fl_Light_Button *>(w);
						bool newValue = lightBtn->value() != 0;
						int flag = 0;
						if(fieldName.noCaseEqual("lightabsolute_top"))
							flag = SideDef::FLAG_LIGHT_ABSOLUTE_TOP;
						else if(fieldName.noCaseEqual("lightabsolute_mid"))
							flag = SideDef::FLAG_LIGHT_ABSOLUTE_MID;
						else if(fieldName.noCaseEqual("lightabsolute_BOTTOM"))
							flag = SideDef::FLAG_LIGHT_ABSOLUTE_BOTTOM;
						if(flag)
						{
							auto &sidedef = box->inst.level.sidedefs[sd];
							if(newValue)
								op.changeSidedef(sd, SideDef::F_FLAGS, sidedef->flags | flag);
							else
								op.changeSidedef(sd, SideDef::F_FLAGS, sidedef->flags & ~flag);
						}
					}
				}

				return;
			}
		}
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
