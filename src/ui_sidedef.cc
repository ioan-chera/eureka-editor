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
	PART_FIELD_HEIGHT = 20,

	UDMF_PANEL_WIDTH = 80,
};

enum
{
	BOTTOM,
	RAILING,
	TOP,
};

// config item
bool config::swap_sidedefs = false;
bool config::show_full_one_sided = false;
bool config::sidedef_add_del_buttons = false;

//------------------------------------------------------------------------
//  CONSTANTS AND LOOKUP TABLES
//------------------------------------------------------------------------

// Number of sidedef texture parts (bottom, mid, top)
static const int kNumSidedefParts = 3;

// Lookup table entry for per-part double fields
struct DoubleFieldEntry
{
	const char *name;
	double SideDef::*fields[kNumSidedefParts];  // bottom, mid, top
};

// Field types for general sidedef properties
enum GeneralFieldType
{
	GENERAL_LIGHT_INT,
	GENERAL_XSCROLL_DOUBLE,
	GENERAL_YSCROLL_DOUBLE,
	GENERAL_FLAG_TOGGLE
};

// UDMF feature information for each sidedef part
struct PartFeatures
{
	UDMF_SideFeature offset[2];  // x, y
	UDMF_SideFeature scale[2];   // x, y
	UDMF_SideFeature light;
	UDMF_SideFeature lightAbsolute;
	UDMF_SideFeature scroll[2];  // x, y
};

//------------------------------------------------------------------------

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
	tex = new UI_DynInput(X, 0, W, PART_FIELD_HEIGHT);
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


	add_button = new Fl_Button(X + W - 120, Y, 50, PART_FIELD_HEIGHT, "ADD");
	add_button->labelcolor(labelcolor());
	add_button->callback(add_callback, this);

	del_button = new Fl_Button(X + W - 65, Y, 50, PART_FIELD_HEIGHT, "DEL");
	del_button->labelcolor(labelcolor());
	del_button->callback(delete_callback, this);


	X += 6;
	Y += 6 + 16;  // space for label

	W -= 16;
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

	mUDMFHeader = new UI_CategoryButton(X, 0, W, FIELD_HEIGHT, "Advanced properties");
	mUDMFHeader->setExpanded(false);
	mUDMFHeader->callback(udmf_category_callback, this);

	mUDMFGrid = new Fl_Grid(X, 0, W, FIELD_HEIGHT);
	mUDMFGrid->layout(4, 7);

	mHeader[BOTTOM] = new Fl_Box(FL_NO_BOX,0, 0, 0, 0, "Lower");
	mHeader[RAILING] = new Fl_Box(FL_NO_BOX, 0, 0, 0, 0, "Middle");
	mHeader[TOP] = new Fl_Box(FL_NO_BOX, 0, 0, 0, 0, "Upper");
	// header labelsize set in loop

	struct BoxLabelMapping
	{
		Fl_Box **box;
		const char *text;
	};
	const BoxLabelMapping mapping[] =
	{
		{ &mOffsetLabel, "Offset:" },
		{ &mScaleLabel, "Scale:" },
		{ &mGlobalLightLabel, "Light:" },
		{ &mLightLabel, "Lights:" },
		{ &mGlobalScrollLabel, "Scroll:" },
		{ &mScrollLabel, "Scrolls:" },
	};
	for(const BoxLabelMapping &entry : mapping)
	{
		*entry.box = new Fl_Box(FL_NO_BOX, 0, 0, 0, 0, entry.text);
		(*entry.box)->labelsize(FLAG_LABELSIZE);
		(*entry.box)->align(FL_ALIGN_INSIDE);
	}
	mHeader[BOTTOM]->labelsize(FLAG_LABELSIZE);
	mHeader[RAILING]->labelsize(FLAG_LABELSIZE);
	mHeader[TOP]->labelsize(FLAG_LABELSIZE);

	for(int i = 0; i < kNumSidedefParts; ++i)
	{
		mOffsetX[i] = new UI_DynFloatInput(0, 0, 0, 0);
		mOffsetY[i] = new UI_DynFloatInput(0, 0, 0, 0);
		mOffsetX[i]->callback(udmf_field_callback, this);
		mOffsetY[i]->callback(udmf_field_callback, this);
		mOffsetX[i]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		mOffsetY[i]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		mFixUp.loadFields({mOffsetX[i], mOffsetY[i]});
	}

	for(int i = 0; i < kNumSidedefParts; ++i)
	{
		mScaleX[i] = new UI_DynFloatInput(0, 0, 0, 0);
		mScaleY[i] = new UI_DynFloatInput(0, 0, 0, 0);
		mScaleX[i]->callback(udmf_field_callback, this);
		mScaleY[i]->callback(udmf_field_callback, this);
		mScaleX[i]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		mScaleY[i]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		mFixUp.loadFields({mScaleX[i], mScaleY[i]});
	}
	mGlobalLight = new UI_DynIntInput(0, 0, 0, 0);
	mGlobalLight->callback(udmf_field_callback, this);
	mGlobalLight->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	mFixUp.loadFields({mGlobalLight});

	mGlobalLightAbsolute = new Fl_Light_Button(0, 0, 0, 0, "Absolute");
	mGlobalLightAbsolute->callback(udmf_field_callback, this);

	for(int i = 0; i < kNumSidedefParts; ++i)
	{
		mLight[i] = new UI_DynIntInput(0, 0, 0, 0);
		mLight[i]->callback(udmf_field_callback, this);
		mLight[i]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		mFixUp.loadFields({ mLight[i] });

		mLightAbsolute[i] = new Fl_Light_Button(0, 0, 0, 0, "Abs");
		mLightAbsolute[i]->callback(udmf_field_callback, this);
	}

	mGlobalXScroll = new UI_DynFloatInput(0, 0, 0, 0);
	mGlobalXScroll->callback(udmf_field_callback, this);
	mGlobalXScroll->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	mFixUp.loadFields({mGlobalXScroll});

	mGlobalYScroll = new UI_DynFloatInput(0, 0, 0, 0);
	mGlobalYScroll->callback(udmf_field_callback, this);
	mGlobalYScroll->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	mFixUp.loadFields({mGlobalYScroll});

	for(int i = 0; i < kNumSidedefParts; ++i)
	{
		mXScroll[i] = new UI_DynFloatInput(0, 0, 0, 0);
		mYScroll[i] = new UI_DynFloatInput(0, 0, 0, 0);
		mXScroll[i]->callback(udmf_field_callback, this);
		mYScroll[i]->callback(udmf_field_callback, this);
		mXScroll[i]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		mYScroll[i]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		mFixUp.loadFields({mXScroll[i], mYScroll[i]});
	}

	mSmoothLighting = new Fl_Check_Button(0, 0, 0, 0, "smooth lighting");
	mSmoothLighting->labelsize(FLAG_LABELSIZE);
	mSmoothLighting->callback(udmf_field_callback, this);

	mNoFakeContrast = new Fl_Check_Button(0, 0, 0, 0, "no fake contrast");
	mNoFakeContrast->labelsize(FLAG_LABELSIZE);
	mNoFakeContrast->callback(udmf_field_callback, this);

	mClipMidTex = new Fl_Check_Button(0, 0, 0, 0, "clip middle texture");
	mClipMidTex->labelsize(FLAG_LABELSIZE);
	mClipMidTex->callback(udmf_field_callback, this);

	mWrapMidTex = new Fl_Check_Button(0, 0, 0, 0, "tile middle texture");
	mWrapMidTex->labelsize(FLAG_LABELSIZE);
	mWrapMidTex->callback(udmf_field_callback, this);

	mUDMFGrid->end();

	mMasterStack->end();

	// Initially hide collapsible sections
	mUDMFHeader->hide();
	mUDMFGrid->hide();

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

	bool hide_change = (obj != index || on_2S_line != two_sided);

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

		// Populate UDMF per-part and general field widgets
		updateUDMFFields();
	}
	else	// clear
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

		// Clear UDMF per-part and general field widgets
		for(int i = 0; i < kNumSidedefParts; ++i)
		{
			mFixUp.setInputValue(mOffsetX[i], "");
			mFixUp.setInputValue(mOffsetY[i], "");
			mFixUp.setInputValue(mScaleX[i], "");
			mFixUp.setInputValue(mScaleY[i], "");
			mFixUp.setInputValue(mLight[i], "");
			mLightAbsolute[i]->value(0);
			mFixUp.setInputValue(mXScroll[i], "");
			mFixUp.setInputValue(mYScroll[i], "");
		}

		mFixUp.setInputValue(mGlobalLight, "");
		mFixUp.setInputValue(mGlobalXScroll, "");
		mFixUp.setInputValue(mGlobalYScroll, "");
		mGlobalLightAbsolute->value(0);
		mSmoothLighting->value(0);
		mNoFakeContrast->value(0);
		mWrapMidTex->value(0);
		mClipMidTex->value(0);

		mUDMFHeader->details("");
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


void UI_SideBox::udmf_category_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = static_cast<UI_SideBox *>(data);

	if(box->mUDMFHeader->isExpanded())
		box->mUDMFGrid->show();
	else
		box->mUDMFGrid->hide();

	box->adjustHeight();
	box->inst.main_win->line_box->redraw();
}

void UI_SideBox::UpdateGameInfo(const LoadingData &loaded, const ConfigData &config)
{
	if(loaded.levelFormat != MapFormat::udmf)
	{
		mUDMFHeader->hide();
		mUDMFGrid->hide();
		adjustHeight();
		redraw();
		return;
	}
	enum
	{
		ROW_HEADERS       = 0x01,
		ROW_OFFSETS       = 0x02,
		ROW_SCALES        = 0x04,
		ROW_COMMON_LIGHT  = 0x08,
		ROW_LOCAL_LIGHTS  = 0x10,
		ROW_COMMON_SCROLL = 0x20,
		ROW_LOCAL_SCROLLS = 0x40,
		ROWS_FLAGS        = 0x80	// these will be arranged dynamically at the end
	};
	unsigned visibleRows = 0;
	auto setVisibility = [&config, &visibleRows](Fl_Widget *widget, UDMF_SideFeature feature,
												 unsigned row)
	{
		if(UDMF_HasSideFeature(config, feature))
		{
			widget->show();
			visibleRows |= row;
		}
		else
			widget->hide();
	};

	// Reset grid cells (hides all children); setVisibility will re-show the right ones
	mUDMFGrid->clear_layout();

	// Show/hide widgets for each part using a loop (using file-scope lookup table)
	setVisibility(mOffsetX[BOTTOM],  UDMF_SideFeature::offsetx_bottom, ROW_OFFSETS);
	setVisibility(mOffsetY[BOTTOM],  UDMF_SideFeature::offsety_bottom, ROW_OFFSETS);
	setVisibility(mOffsetX[RAILING], UDMF_SideFeature::offsetx_mid,    ROW_OFFSETS);
	setVisibility(mOffsetY[RAILING], UDMF_SideFeature::offsety_mid,    ROW_OFFSETS);
	setVisibility(mOffsetX[TOP],     UDMF_SideFeature::offsetx_top,    ROW_OFFSETS);
	setVisibility(mOffsetY[TOP],     UDMF_SideFeature::offsety_top,    ROW_OFFSETS);
	setVisibility(mScaleX[BOTTOM],   UDMF_SideFeature::scalex_bottom,  ROW_SCALES);
	setVisibility(mScaleY[BOTTOM],   UDMF_SideFeature::scaley_bottom,  ROW_SCALES);
	setVisibility(mScaleX[RAILING],  UDMF_SideFeature::scalex_mid,     ROW_SCALES);
	setVisibility(mScaleY[RAILING],  UDMF_SideFeature::scaley_mid,     ROW_SCALES);
	setVisibility(mScaleX[TOP],      UDMF_SideFeature::scalex_top,     ROW_SCALES);
	setVisibility(mScaleY[TOP],      UDMF_SideFeature::scaley_top,     ROW_SCALES);
	setVisibility(mGlobalLight,      UDMF_SideFeature::light,          ROW_COMMON_LIGHT);
	setVisibility(mGlobalLightAbsolute, UDMF_SideFeature::lightabsolute, ROW_COMMON_LIGHT);
	setVisibility(mLight[BOTTOM],    UDMF_SideFeature::light_bottom,   ROW_LOCAL_LIGHTS);
	setVisibility(mLight[RAILING],   UDMF_SideFeature::light_mid,      ROW_LOCAL_LIGHTS);
	setVisibility(mLight[TOP],       UDMF_SideFeature::light_top,      ROW_LOCAL_LIGHTS);
	setVisibility(mLightAbsolute[BOTTOM], UDMF_SideFeature::lightabsolute_bottom,
				  ROW_LOCAL_LIGHTS);
	setVisibility(mLightAbsolute[RAILING], UDMF_SideFeature::lightabsolute_mid,
				  ROW_LOCAL_LIGHTS);
	setVisibility(mLightAbsolute[TOP], UDMF_SideFeature::lightabsolute_top, ROW_LOCAL_LIGHTS);
	setVisibility(mGlobalXScroll, UDMF_SideFeature::xscroll, ROW_COMMON_SCROLL);
	setVisibility(mGlobalYScroll, UDMF_SideFeature::yscroll, ROW_COMMON_SCROLL);
	setVisibility(mXScroll[BOTTOM], UDMF_SideFeature::xscrollbottom, ROW_LOCAL_SCROLLS);
	setVisibility(mYScroll[BOTTOM], UDMF_SideFeature::yscrollbottom, ROW_LOCAL_SCROLLS);
	setVisibility(mXScroll[RAILING], UDMF_SideFeature::xscrollmid, ROW_LOCAL_SCROLLS);
	setVisibility(mYScroll[RAILING], UDMF_SideFeature::yscrollmid, ROW_LOCAL_SCROLLS);
	setVisibility(mXScroll[TOP], UDMF_SideFeature::xscrolltop, ROW_LOCAL_SCROLLS);
	setVisibility(mYScroll[TOP], UDMF_SideFeature::yscrolltop, ROW_LOCAL_SCROLLS);

	struct FlagMapping
	{
		UDMF_SideFeature feature;
		Fl_Check_Button *button;
	};
	const FlagMapping mapping[] =
	{
		{ UDMF_SideFeature::smoothlighting, mSmoothLighting },
		{ UDMF_SideFeature::nofakecontrast, mNoFakeContrast },
		{ UDMF_SideFeature::clipmidtex, mClipMidTex },
		{ UDMF_SideFeature::wrapmidtex, mWrapMidTex },
	};
	int numflags = 0;
	for(const FlagMapping &entry : mapping)
	{
		if(UDMF_HasSideFeature(config, entry.feature))
		{
			entry.button->show();
			visibleRows |= ROWS_FLAGS;
			++numflags;
		}
		else
			entry.button->hide();
	}

	if(!visibleRows)
	{
		mUDMFHeader->hide();
		mUDMFGrid->hide();
		adjustHeight();
		redraw();
		return;
	}

	int numRows = 0;
	mUDMFHeader->show();
	if(mUDMFHeader->isExpanded())
		mUDMFGrid->show();
	else
		mUDMFGrid->hide();
	if(visibleRows & (ROW_OFFSETS | ROW_SCALES | ROW_LOCAL_LIGHTS | ROW_LOCAL_SCROLLS))
	{
		visibleRows |= ROW_HEADERS;
		numRows++;
	}
	if(visibleRows & ROW_OFFSETS)
		numRows++;
	if(visibleRows & ROW_SCALES)
		numRows++;
	if(visibleRows & ROW_COMMON_LIGHT)
		numRows++;
	if(visibleRows & ROW_LOCAL_LIGHTS)
		numRows++;
	if(visibleRows & ROW_COMMON_SCROLL)
		numRows++;
	if(visibleRows & ROW_LOCAL_SCROLLS)
		numRows++;
	numRows += (numflags + 1) / 2;

	int gridHeight = numRows * FIELD_HEIGHT;
	if(numRows > 1)
		gridHeight += (numRows - 1) * 2;
	mUDMFGrid->resize(mUDMFGrid->x(), mUDMFGrid->y(), mUDMFGrid->w(), gridHeight);
	mUDMFGrid->layout(numRows, 7);
	mUDMFGrid->gap(2, 0);
	mUDMFGrid->col_gap(2, 6);
	mUDMFGrid->col_gap(4, 6);
	int currentRow = 0;
	if(visibleRows & ROW_HEADERS)
	{
		mHeader[BOTTOM]->show();
		mHeader[RAILING]->show();
		mHeader[TOP]->show();
		mUDMFGrid->widget(mHeader[BOTTOM], currentRow, 1, 1, 2);
		mUDMFGrid->widget(mHeader[RAILING], currentRow, 3, 1, 2);
		mUDMFGrid->widget(mHeader[TOP], currentRow, 5, 1, 2);
		++currentRow;
	}
	if(visibleRows & ROW_OFFSETS)
	{
		mOffsetLabel->show();
		mUDMFGrid->widget(mOffsetLabel, currentRow, 0);
		mUDMFGrid->widget(mOffsetX[BOTTOM], currentRow, 1);
		mUDMFGrid->widget(mOffsetY[BOTTOM], currentRow, 2);
		mUDMFGrid->widget(mOffsetX[RAILING], currentRow, 3);
		mUDMFGrid->widget(mOffsetY[RAILING], currentRow, 4);
		mUDMFGrid->widget(mOffsetX[TOP], currentRow, 5);
		mUDMFGrid->widget(mOffsetY[TOP], currentRow, 6);
		currentRow++;
	}
	if(visibleRows & ROW_SCALES)
	{
		mScaleLabel->show();
		mUDMFGrid->widget(mScaleLabel, currentRow, 0);
		mUDMFGrid->widget(mScaleX[BOTTOM], currentRow, 1);
		mUDMFGrid->widget(mScaleY[BOTTOM], currentRow, 2);
		mUDMFGrid->widget(mScaleX[RAILING], currentRow, 3);
		mUDMFGrid->widget(mScaleY[RAILING], currentRow, 4);
		mUDMFGrid->widget(mScaleX[TOP], currentRow, 5);
		mUDMFGrid->widget(mScaleY[TOP], currentRow, 6);
		currentRow++;
	}
	if(visibleRows & ROW_COMMON_LIGHT)
	{
		mGlobalLightLabel->show();
		mUDMFGrid->widget(mGlobalLightLabel, currentRow, 0);
		mUDMFGrid->widget(mGlobalLight, currentRow, 2, 1, 2);
		mUDMFGrid->widget(mGlobalLightAbsolute, currentRow, 4, 1, 2);
		currentRow++;
	}
	if(visibleRows & ROW_LOCAL_LIGHTS)
	{
		mLightLabel->show();
		mUDMFGrid->widget(mLightLabel, currentRow, 0);
		mUDMFGrid->widget(mLight[BOTTOM], currentRow, 1);
		mUDMFGrid->widget(mLightAbsolute[BOTTOM], currentRow, 2);
		mUDMFGrid->widget(mLight[RAILING], currentRow, 3);
		mUDMFGrid->widget(mLightAbsolute[RAILING], currentRow, 4);
		mUDMFGrid->widget(mLight[TOP], currentRow, 5);
		mUDMFGrid->widget(mLightAbsolute[TOP], currentRow, 6);
		++currentRow;
	}
	if(visibleRows & ROW_COMMON_SCROLL)
	{
		mGlobalScrollLabel->show();
		mUDMFGrid->widget(mGlobalScrollLabel, currentRow, 0);
		mUDMFGrid->widget(mGlobalXScroll, currentRow, 2, 1, 2);
		mUDMFGrid->widget(mGlobalYScroll, currentRow, 4, 1, 2);
		++currentRow;
	}
	if(visibleRows & ROW_LOCAL_SCROLLS)
	{
		mScrollLabel->show();
		mUDMFGrid->widget(mScrollLabel, currentRow, 0);
		mUDMFGrid->widget(mXScroll[BOTTOM], currentRow, 1);
		mUDMFGrid->widget(mYScroll[BOTTOM], currentRow, 2);
		mUDMFGrid->widget(mXScroll[RAILING], currentRow, 3);
		mUDMFGrid->widget(mYScroll[RAILING], currentRow, 4);
		mUDMFGrid->widget(mXScroll[TOP], currentRow, 5);
		mUDMFGrid->widget(mYScroll[TOP], currentRow, 6);
		++currentRow;
	}
	int flagnum = 0;
	for(const FlagMapping &entry : mapping)
	{
		if(!entry.button->visible())
			continue;
		mUDMFGrid->widget(entry.button, currentRow + flagnum / 2, 1 + 3 * (flagnum % 2), 1, 3);
		++flagnum;
	}

	adjustHeight();
	redraw();
}


void UI_SideBox::udmf_field_callback(Fl_Widget *w, void *data)
{
	UI_SideBox *box = static_cast<UI_SideBox *>(data);

	if(box->obj < 0 || box->inst.edit.Selected->empty())
		return;

	struct FieldMapping
	{
		Fl_Widget *widget;
		std::variant<int, SideDef::IntAddress, double SideDef::*> field;
		const char *name;
	};
	const FieldMapping mapping[] =
	{
		{ box->mOffsetX[BOTTOM], &SideDef::offsetx_bottom, "bottom offset X" },
		{ box->mOffsetX[RAILING], &SideDef::offsetx_mid, "mid offset X" },
		{ box->mOffsetX[TOP], &SideDef::offsetx_top, "top offset X" },
		{ box->mOffsetY[BOTTOM], &SideDef::offsety_bottom, "bottom offset Y" },
		{ box->mOffsetY[RAILING], &SideDef::offsety_mid, "mid offset Y" },
		{ box->mOffsetY[TOP], &SideDef::offsety_top, "top offset Y" },
		{ box->mScaleX[BOTTOM], &SideDef::scalex_bottom, "bottom scale X" },
		{ box->mScaleX[RAILING], &SideDef::scalex_mid, "mid scale X" },
		{ box->mScaleX[TOP], &SideDef::scalex_top, "top scale X" },
		{ box->mScaleY[BOTTOM], &SideDef::scaley_bottom, "bottom scale Y" },
		{ box->mScaleY[RAILING], &SideDef::scaley_mid, "mid scale Y" },
		{ box->mScaleY[TOP], &SideDef::scaley_top, "top scale Y" },
		{ box->mGlobalLight, SideDef::F_LIGHT, "light" },
		{ box->mGlobalLightAbsolute, SideDef::FLAG_LIGHTABSOLUTE, "light flag" },
		{ box->mLight[BOTTOM], SideDef::F_LIGHT_BOTTOM, "bottom light" },
		{ box->mLight[RAILING], SideDef::F_LIGHT_MID, "mid light" },
		{ box->mLight[TOP], SideDef::F_LIGHT_TOP, "top light" },
		{ box->mLightAbsolute[BOTTOM], SideDef::FLAG_LIGHT_ABSOLUTE_BOTTOM, "bottom light flag" },
		{ box->mLightAbsolute[RAILING], SideDef::FLAG_LIGHT_ABSOLUTE_MID, "mid light flag" },
		{ box->mLightAbsolute[TOP], SideDef::FLAG_LIGHT_ABSOLUTE_TOP, "top light flag" },
		{ box->mGlobalXScroll, &SideDef::xscroll, "scroll X" },
		{ box->mGlobalYScroll, &SideDef::yscroll, "scroll Y" },
		{ box->mXScroll[BOTTOM], &SideDef::xscrollbottom, "bottom scroll X" },
		{ box->mXScroll[RAILING], &SideDef::xscrollmid, "mid scroll X" },
		{ box->mXScroll[TOP], &SideDef::xscrolltop, "top scroll X" },
		{ box->mYScroll[BOTTOM], &SideDef::yscrollbottom, "bottom scroll Y" },
		{ box->mYScroll[RAILING], &SideDef::yscrollmid, "mid scroll Y" },
		{ box->mYScroll[TOP], &SideDef::yscrolltop, "top scroll Y" },
		{ box->mSmoothLighting, SideDef::FLAG_SMOOTHLIGHTING, "flag" },
		{ box->mNoFakeContrast, SideDef::FLAG_NOFAKECONTRAST, "flag" },
		{ box->mClipMidTex, SideDef::FLAG_CLIPMIDTEX, "flag" },
		{ box->mWrapMidTex, SideDef::FLAG_WRAPMIDTEX, "flag" },
	};

	const FieldMapping *found = nullptr;
	for(const FieldMapping &entry : mapping)
	{
		if(entry.widget == w)
		{
			found = &entry;
			break;
		}
	}
	assert(found);
	if(!found)
		return;

	EditOperation op(box->inst.level.basis);
	SString message = SString("edited sidedef ") + found->name + " on";
	op.setMessageForSelection(message.c_str(), *box->inst.edit.Selected);

	for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
	{
		const auto L = box->inst.level.linedefs[*it];
		int sd = box->is_front ? L->right : L->left;

		if(!box->inst.level.isSidedef(sd))
			continue;

		std::visit([&](auto fieldValue)
		{
			using FieldType = decltype(fieldValue);

			if constexpr(std::is_same_v<FieldType, int>)	// flag
			{
				bool value = static_cast<Fl_Button*>(w)->value() != 0;

				auto &sidedef = box->inst.level.sidedefs[sd];
				int newFlags = value ? sidedef->flags | fieldValue : sidedef->flags & ~fieldValue;
				op.changeSidedef(sd, SideDef::F_FLAGS, newFlags);
				return;
			}
			if constexpr(std::is_same_v<FieldType, SideDef::IntAddress>)
			{
				int value = atoi(static_cast<UI_DynIntInput *>(w)->value());
				op.changeSidedef(sd, fieldValue, value);
				return;
			}
			if constexpr(std::is_same_v<FieldType, double SideDef::*>)
			{
				double value = atof(static_cast<UI_DynFloatInput*>(w)->value());
				op.changeSidedef(sd, fieldValue, value);
			}
		}, found->field);
	}
}

void UI_SideBox::updateUDMFFields()
{
	if(!inst.level.isSidedef(obj))
		return;

	const auto &sd = inst.level.sidedefs[obj];

	mFixUp.setInputValue(mOffsetX[BOTTOM], SString::printf("%.16g", sd->offsetx_bottom).c_str());
	mFixUp.setInputValue(mOffsetX[RAILING], SString::printf("%.16g", sd->offsetx_mid).c_str());
	mFixUp.setInputValue(mOffsetX[TOP], SString::printf("%.16g", sd->offsetx_top).c_str());
	mFixUp.setInputValue(mOffsetY[BOTTOM], SString::printf("%.16g", sd->offsety_bottom).c_str());
	mFixUp.setInputValue(mOffsetY[RAILING], SString::printf("%.16g", sd->offsety_mid).c_str());
	mFixUp.setInputValue(mOffsetY[TOP], SString::printf("%.16g", sd->offsety_top).c_str());

	mFixUp.setInputValue(mScaleX[BOTTOM], SString::printf("%.16g", sd->scalex_bottom).c_str());
	mFixUp.setInputValue(mScaleX[RAILING], SString::printf("%.16g", sd->scalex_mid).c_str());
	mFixUp.setInputValue(mScaleX[TOP], SString::printf("%.16g", sd->scalex_top).c_str());
	mFixUp.setInputValue(mScaleY[BOTTOM], SString::printf("%.16g", sd->scaley_bottom).c_str());
	mFixUp.setInputValue(mScaleY[RAILING], SString::printf("%.16g", sd->scaley_mid).c_str());
	mFixUp.setInputValue(mScaleY[TOP], SString::printf("%.16g", sd->scaley_top).c_str());

	mFixUp.setInputValue(mGlobalLight, SString::printf("%d", sd->light).c_str());
	mGlobalLightAbsolute->value(sd->flags & SideDef::FLAG_LIGHTABSOLUTE ? 1 : 0);

	mFixUp.setInputValue(mLight[BOTTOM], SString::printf("%d", sd->light_bottom).c_str());
	mFixUp.setInputValue(mLight[RAILING], SString::printf("%d", sd->light_mid).c_str());
	mFixUp.setInputValue(mLight[TOP], SString::printf("%d", sd->light_top).c_str());
	mLightAbsolute[BOTTOM]->value(sd->flags & SideDef::FLAG_LIGHT_ABSOLUTE_BOTTOM ? 1 : 0);
	mLightAbsolute[RAILING]->value(sd->flags & SideDef::FLAG_LIGHT_ABSOLUTE_MID ? 1 : 0);
	mLightAbsolute[TOP]->value(sd->flags & SideDef::FLAG_LIGHT_ABSOLUTE_TOP ? 1 : 0);

	mFixUp.setInputValue(mGlobalXScroll, SString::printf("%.16g", sd->xscroll).c_str());
	mFixUp.setInputValue(mGlobalYScroll, SString::printf("%.16g", sd->yscroll).c_str());

	mFixUp.setInputValue(mXScroll[BOTTOM], SString::printf("%.16g", sd->xscrollbottom).c_str());
	mFixUp.setInputValue(mXScroll[RAILING], SString::printf("%.16g", sd->xscrollmid).c_str());
	mFixUp.setInputValue(mXScroll[TOP], SString::printf("%.16g", sd->xscrolltop).c_str());
	mFixUp.setInputValue(mYScroll[BOTTOM], SString::printf("%.16g", sd->yscrollbottom).c_str());
	mFixUp.setInputValue(mYScroll[RAILING], SString::printf("%.16g", sd->yscrollmid).c_str());
	mFixUp.setInputValue(mYScroll[TOP], SString::printf("%.16g", sd->yscrolltop).c_str());

	mSmoothLighting->value(sd->flags & SideDef::FLAG_SMOOTHLIGHTING ? 1 : 0);
	mNoFakeContrast->value(sd->flags & SideDef::FLAG_NOFAKECONTRAST ? 1 : 0);
	mClipMidTex->value(sd->flags & SideDef::FLAG_CLIPMIDTEX ? 1 : 0);
	mWrapMidTex->value(sd->flags & SideDef::FLAG_WRAPMIDTEX ? 1 : 0);

	SString summary;
	if(sd->offsetx_mid || sd->offsetx_bottom || sd->offsetx_top)
		summary += "←";
	if(sd->offsety_mid || sd->offsety_bottom || sd->offsety_top)
		summary += "↑";
	if(sd->scalex_mid != 1 || sd->scalex_bottom != 1 || sd->scalex_top != 1)
		summary += "↔";
	if(sd->scaley_mid != 1|| sd->scaley_bottom != 1 || sd->scaley_top != 1)
		summary += "↕";
	if(sd->light || sd->light_mid || sd->light_bottom || sd->light_top)
		summary += "💡";
	if(sd->xscroll || sd->yscroll || sd->xscrollmid || sd->xscrolltop || sd->xscrollbottom ||
	   sd->yscrollmid || sd->yscrolltop || sd->yscrollbottom)
	{
		summary += "🔄";
	}
	if(sd->flags & SideDef::FLAG_SMOOTHLIGHTING)
		summary += "💎";
	if(sd->flags & SideDef::FLAG_NOFAKECONTRAST)
		summary += "◑";
	if(sd->flags & SideDef::FLAG_CLIPMIDTEX)
		summary += "✂️";
	if(sd->flags & SideDef::FLAG_WRAPMIDTEX)
		summary += "🔲";

	mUDMFHeader->details(summary);

	updateSideFlagGreying();
}

void UI_SideBox::updateSideFlagGreying()
{
	bool smooth = !!mSmoothLighting->value();
	bool noFake = !!mNoFakeContrast->value();

	mNoFakeContrast->labelcolor(smooth ? FL_INACTIVE_COLOR : FL_FOREGROUND_COLOR);
	mNoFakeContrast->redraw_label();

	mSmoothLighting->labelcolor(noFake ? FL_INACTIVE_COLOR : FL_FOREGROUND_COLOR);
	mSmoothLighting->redraw_label();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
