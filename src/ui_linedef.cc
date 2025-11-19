//------------------------------------------------------------------------
//  LINEDEF PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2018 Andrew Apted
//  Copyright (C) 2025      Ioan Chera
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

#include "Errors.h"
#include "Instance.h"

#include "main.h"
#include "ui_category_button.h"
#include "ui_misc.h"
#include "ui_window.h"

#include "e_checks.h"
#include "e_cutpaste.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_things.h"
#include "LineDef.h"
#include "m_game.h"
#include "Sector.h"
#include "SideDef.h"
#include "w_rawdef.h"
#include "w_rawdef.h"

#include <algorithm>


#define MLF_ALL_AUTOMAP  \
	MLF_Secret | MLF_Mapped | MLF_DontDraw | MLF_XDoom_Translucent

enum
{
	INSET_LEFT = 6,
	INSET_TOP = 6,
	INSET_RIGHT = 6,
	INSET_BOTTOM = 5,

	SPACING_BELOW_NOMBRE = 4,

	TYPE_INPUT_X = 58,
	TYPE_INPUT_WIDTH = 75,
	TYPE_INPUT_HEIGHT = 24,

	BUTTON_SPACING = 10,
	CHOOSE_BUTTON_WIDTH = 80,

	GEN_BUTTON_WIDTH = 55,
	GEN_BUTTON_RIGHT_PADDING = 10,

	INPUT_SPACING = 2,

	DESC_INSET = 10,
	DESC_WIDTH = 48,

	SPAC_WIDTH = 57,

	TAG_WIDTH = 64,
	ARG_WIDTH = 42,
	ARG_PADDING = 5,
};


class line_flag_CB_data_c
{
public:
	UI_LineBox *parent;

	int flagIndex;
	int mask;

	line_flag_CB_data_c(UI_LineBox *_parent, int _flagIndex, int _mask) :
		parent(_parent), flagIndex(_flagIndex), mask(_mask)
	{ }
};


//
// UI_LineBox Constructor
//
UI_LineBox::UI_LineBox(Instance &inst, int X, int Y, int W, int H, const char *label) :
    MapItemBox(inst, X, Y, W, H, label)
{
	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);

	const int Y0 = Y;
	const int X0 = X;

	X += INSET_LEFT;
	Y += INSET_TOP;

	W -= INSET_LEFT + INSET_RIGHT;
	H -= INSET_TOP + INSET_BOTTOM;


	which = new UI_Nombre(X + NOMBRE_INSET, Y, W - 2 * NOMBRE_INSET, NOMBRE_HEIGHT, "Linedef");

	Y += which->h() + SPACING_BELOW_NOMBRE;


	type = new UI_DynInput(X + TYPE_INPUT_X, Y, TYPE_INPUT_WIDTH, TYPE_INPUT_HEIGHT, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);
	type->callback2(dyntype_callback, this);
	type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	type->type(FL_INT_INPUT);

	choose = new Fl_Button(type->x() + type->w() + BUTTON_SPACING, Y, CHOOSE_BUTTON_WIDTH, TYPE_INPUT_HEIGHT, "Choose");
	choose->callback(button_callback, this);

	gen = new Fl_Button(choose->x() + choose->w() + BUTTON_SPACING, Y, GEN_BUTTON_WIDTH, TYPE_INPUT_HEIGHT, "Gen");
	gen->callback(button_callback, this);

	Y += type->h() + INPUT_SPACING;


	descBox = new Fl_Box(FL_NO_BOX, X + DESC_INSET, Y, DESC_WIDTH, TYPE_INPUT_HEIGHT, "Desc: ");

	desc = new Fl_Output(type->x(), Y, W - type->x(), TYPE_INPUT_HEIGHT);
	desc->align(FL_ALIGN_LEFT);


	actkind = new Fl_Choice(type->x(), Y, SPAC_WIDTH, TYPE_INPUT_HEIGHT);
	// this order must match the SPAC_XXX constants
	actkind->add(getActivationMenuString());
	actkind->value(getActivationCount());
	actkind->callback(flags_callback, new line_flag_CB_data_c(this, 1, MLF_Activation | MLF_Repeatable));
	actkind->deactivate();
	actkind->hide();

	Y += desc->h() + INPUT_SPACING;


	tag = new UI_DynIntInput(type->x(), Y, TAG_WIDTH, TYPE_INPUT_HEIGHT, "Tag: ");
	tag->align(FL_ALIGN_LEFT);
	tag->callback(tag_callback, this);
	tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	length = new UI_DynIntInput(type->x() + W/2, Y, TAG_WIDTH, TYPE_INPUT_HEIGHT, "Length: ");
	length->align(FL_ALIGN_LEFT);
	length->callback(length_callback, this);
	length->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	for (int a = 0 ; a < 5 ; a++)
	{
		args[a] = new UI_DynIntInput(type->x() + (ARG_WIDTH + ARG_PADDING) * a, Y, ARG_WIDTH, TYPE_INPUT_HEIGHT);
		args[a]->callback(args_callback, new line_flag_CB_data_c(this, 1, a));
		args[a]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		args[a]->hide();
	}

	args[0]->label("Args: ");


	Y += tag->h() + 10;


	Fl_Box *flags = new Fl_Box(FL_FLAT_BOX, X+10, Y, 64, 24, "Flags: ");
	flags->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);


	f_automap = new Fl_Choice(X+W-118, Y, 104, 22, "Vis: ");
	f_automap->add("Normal|Invisible|Mapped|Secret|MapSecret");
	f_automap->value(0);
	f_automap->callback(flags_callback, new line_flag_CB_data_c(this, 1, MLF_ALL_AUTOMAP));


	Y += flags->h() - 1;

	// Remember where to place dynamic linedef flags
	flagsStartX = X - X0;
	flagsStartY = Y - Y0;
	flagsAreaW = W;

	// Leave space; dynamic flags will be created in UpdateGameInfo and side boxes moved accordingly
	Y += 29;


	front = new UI_SideBox(inst, x(), Y, w(), 140, 0);

	Y += front->h() + 14;


	back = new UI_SideBox(inst, x(), Y, w(), 140, 1);

	Y += back->h();

	mFixUp.loadFields({type, length, tag, args[0], args[1], args[2], args[3], args[4]});

	end();

	resizable(nullptr);
}


void UI_LineBox::type_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	// support hexadecimal
	int new_type = (int)strtol(box->type->value(), NULL, 0);

	if (! box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited type of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected) ; !it.done() ; it.next())
		{
			op.changeLinedef(*it, LineDef::F_TYPE, new_type);
		}
	}

	// update description
	box->UpdateField(LineDef::F_TYPE);
}


void UI_LineBox::dyntype_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	if (box->obj < 0)
		return;

	// support hexadecimal
	int new_type = (int)strtol(box->type->value(), NULL, 0);

	const char *gen_desc = box->GeneralizedDesc(new_type);

	if (gen_desc)
	{
		box->desc->value(gen_desc);
	}
	else
	{
		const linetype_t &info = box->inst.conf.getLineType(new_type);
		box->desc->value(info.desc.c_str());
	}

	box->inst.main_win->browser->UpdateGenType(new_type);
}


void UI_LineBox::SetTexOnLine(EditOperation &op, int ld, StringID new_tex, int e_state, int parts)
{
	bool opposite = (e_state & FL_SHIFT);

	const auto L = inst.level.linedefs[ld];

	// handle the selected texture boxes
	if (parts != 0)
	{
		if (L->OneSided())
		{
			if (parts & PART_RT_LOWER)
				op.changeSidedef(L->right, SideDef::F_MID_TEX,   new_tex);
			if (parts & PART_RT_UPPER)
				op.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_tex);
            if (parts & PART_RT_RAIL)
                op.changeSidedef(L->right, SideDef::F_LOWER_TEX, new_tex);

			return;
		}

		if (inst.level.getRight(*L))
		{
			if (parts & PART_RT_LOWER)
				op.changeSidedef(L->right, SideDef::F_LOWER_TEX, new_tex);
			if (parts & PART_RT_UPPER)
				op.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_tex);
			if (parts & PART_RT_RAIL)
				op.changeSidedef(L->right, SideDef::F_MID_TEX,   new_tex);
		}

		if (inst.level.getLeft(*L))
		{
			if (parts & PART_LF_LOWER)
				op.changeSidedef(L->left, SideDef::F_LOWER_TEX, new_tex);
			if (parts & PART_LF_UPPER)
				op.changeSidedef(L->left, SideDef::F_UPPER_TEX, new_tex);
			if (parts & PART_LF_RAIL)
				op.changeSidedef(L->left, SideDef::F_MID_TEX,   new_tex);
		}
		return;
	}

	// middle click : set mid-masked texture on both sides
	if (e_state & FL_BUTTON2)
	{
		if (! L->TwoSided())
			return;

		// convenience: set lower unpeg on first change
		if (! (L->flags & MLF_LowerUnpegged)  &&
		    is_null_tex(inst.level.getRight(*L)->MidTex()) &&
		    is_null_tex(inst.level.getLeft(*L)->MidTex()) )
		{
			op.changeLinedef(ld, LineDef::F_FLAGS, L->flags | MLF_LowerUnpegged);
		}

		op.changeSidedef(L->left,  SideDef::F_MID_TEX, new_tex);
		op.changeSidedef(L->right, SideDef::F_MID_TEX, new_tex);
		return;
	}

	// one-sided case: set the middle texture only
	if (! L->TwoSided())
	{
		int sd = (L->right >= 0) ? L->right : L->left;

		if (sd < 0)
			return;

		op.changeSidedef(sd, SideDef::F_MID_TEX, new_tex);
		return;
	}

	// modify an upper texture
	int sd1 = L->right;
	int sd2 = L->left;

	if (e_state & FL_BUTTON3)
	{
		// back ceiling is higher?
		if (inst.level.getSector(*inst.level.getLeft(*L)).ceilh > inst.level.getSector(*inst.level.getRight(*L)).ceilh)
			std::swap(sd1, sd2);

		if (opposite)
			std::swap(sd1, sd2);

		op.changeSidedef(sd1, SideDef::F_UPPER_TEX, new_tex);
	}
	// modify a lower texture
	else
	{
		// back floor is lower?
		if (inst.level.getSector(*inst.level.getLeft(*L)).floorh < inst.level.getSector(*inst.level.getRight(*L)).floorh)
			std::swap(sd1, sd2);

		if (opposite)
			std::swap(sd1, sd2);

		const auto S = inst.level.sidedefs[sd1];

		// change BOTH upper and lower when they are the same
		// (which is great for windows).
		//
		// Note: we only do this for LOWERS (otherwise it'd be
		// impossible to set them to different textures).

		if (S->lower_tex == S->upper_tex)
			op.changeSidedef(sd1, SideDef::F_UPPER_TEX, new_tex);

		op.changeSidedef(sd1, SideDef::F_LOWER_TEX, new_tex);
	}
}

//
// Check sidedefs dirty fields
//
void UI_LineBox::checkSidesDirtyFields()
{
	front->checkDirtyFields();
	back->checkDirtyFields();
}

int UI_LineBox::getActivationCount() const
{
	return inst.conf.features.player_use_passthru_activation ? 14 : 12;
}
const char *UI_LineBox::getActivationMenuString() const
{
	return inst.conf.features.player_use_passthru_activation ?
		"W1|WR|S1|SR|M1|MR|G1|GR|P1|PR|X1|XR|S1+|SR+|??" :
		"W1|WR|S1|SR|M1|MR|G1|GR|P1|PR|X1|XR|??";
}

void UI_LineBox::SetTexture(const char *tex_name, int e_state, int uiparts)
{
	StringID new_tex = BA_InternaliseString(tex_name);

	// this works a bit differently than other ways, we don't modify a
	// widget and let it update the map, instead we update the map and
	// let the widget(s) get updated.  That's because we do different
	// things depending on whether the line is one-sided or two-sided.

	if (! inst.edit.Selected->empty())
	{
        // WARNING: translate uiparts to be valid for a one-sided line
        if(inst.level.isLinedef(obj) && inst.level.linedefs[obj]->OneSided())
        {
            uiparts = (uiparts & PART_RT_UPPER) |
                      (uiparts & PART_RT_LOWER ? PART_RT_RAIL : 0) |
                      (uiparts & PART_RT_RAIL ? PART_RT_LOWER : 0);
        }

        mFixUp.checkDirtyFields();
		checkSidesDirtyFields();

		EditOperation op(inst.level.basis);
		op.setMessageForSelection("edited texture on", *inst.edit.Selected);

		for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
		{
			int p2 = inst.edit.Selected->get_ext(*it);

			// only use parts explicitly selected in 3D view when no
			// parts in the linedef panel are selected.
			if (! (uiparts == 0 && p2 > 1))
				p2 = uiparts;

			SetTexOnLine(op, *it, new_tex, e_state, p2);
		}
	}

	UpdateField();
	UpdateSides();

	redraw();
}


void UI_LineBox::SetLineType(int new_type)
{
	if (obj < 0)
	{
///		Beep("No lines selected");
		return;
	}

	auto buffer = SString(new_type);

	mFixUp.checkDirtyFields();
	checkSidesDirtyFields();

	mFixUp.setInputValue(type, buffer.c_str());
	type->do_callback();
}


void UI_LineBox::CB_Copy(int uiparts)
{
	// determine which sidedef texture to grab from
	const char *name = NULL;

	mFixUp.checkDirtyFields();
	checkSidesDirtyFields();

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		UI_SideBox *SD = (pass == 0) ? front : back;

		for (int b = 0 ; b < 3 ; b++)
		{
			int try_part = PART_RT_LOWER << (b + pass * 4);

			if ((uiparts & try_part) == 0)
				continue;

			const char *b_name = (b == 0) ? SD->l_tex->value() :
								 (b == 1) ? SD->u_tex->value() :
											SD->r_tex->value();
			SYS_ASSERT(b_name);

			if (name && y_stricmp(name, b_name) != 0)
			{
				inst.Beep("multiple textures");
				return;
			}

			name = b_name;
		}
	}

	Texboard_SetTex(name, inst.conf);

	inst.Status_Set("copied %s", name);
}


void UI_LineBox::CB_Paste(int uiparts, StringID new_tex)
{
	// iterate over selected linedefs
	if (inst.edit.Selected->empty())
		return;

	mFixUp.checkDirtyFields();
	checkSidesDirtyFields();

	{
		EditOperation op(inst.level.basis);
		op.setMessage("pasted %s", BA_GetString(new_tex).c_str());

		for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
		{
			const auto L = inst.level.linedefs[*it];

			for (int pass = 0 ; pass < 2 ; pass++)
			{
				int sd = (pass == 0) ? L->right : L->left;
				if (sd < 0)
					continue;

				int uiparts2 = pass ? (uiparts >> 4) : uiparts;

                // WARNING: different meaning of lower/railing between
                // UI panel and elsewhere. Here we know it's UI
                if (uiparts2 & PART_RT_LOWER)
                    op.changeSidedef(sd, SideDef::F_LOWER_TEX, new_tex);

                if (uiparts2 & PART_RT_UPPER)
                    op.changeSidedef(sd, SideDef::F_UPPER_TEX, new_tex);

                if (uiparts2 & PART_RT_RAIL)
                    op.changeSidedef(sd, SideDef::F_MID_TEX, new_tex);
			}
		}
	}

	UpdateField();
	UpdateSides();

	redraw();
}


bool UI_LineBox::ClipboardOp(EditCommand op)
{
	if (obj < 0)
		return false;

	int uiparts = front->GetSelectedPics() | (back->GetSelectedPics() << 4);

	if (uiparts == 0)
		uiparts = front->GetHighlightedPics() | (back->GetHighlightedPics() << 4);

	if (uiparts == 0)
		return false;

	switch (op)
	{
		case EditCommand::copy:
			CB_Copy(uiparts);
			break;

		case EditCommand::paste:
			CB_Paste(uiparts, Texboard_GetTexNum(inst.conf));
			break;

		case EditCommand::cut:	// Cut
			CB_Paste(uiparts, BA_InternaliseString(inst.conf.default_wall_tex));
			break;

		case EditCommand::del: // Delete
			CB_Paste(uiparts, BA_InternaliseString("-"));
			break;
	}

	return true;
}


void UI_LineBox::BrowsedItem(BrowserMode kind, int number, const char *name, int e_state)
{
	if (kind == BrowserMode::textures || kind == BrowserMode::flats)
	{
		int front_pics = front->GetSelectedPics();
		int  back_pics =  back->GetSelectedPics();

		// this can be zero, invoking special behavior (based on mouse button)
		int uiparts = front_pics | (back_pics << 4);

		SetTexture(name, e_state, uiparts);
	}
	else if (kind == BrowserMode::lineTypes)
	{
		SetLineType(number);
	}
}


void UI_LineBox::tag_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	int new_tag = atoi(box->tag->value());

	if (! box->inst.edit.Selected->empty())
		box->inst.level.checks.tagsApplyNewValue(new_tag);
}


void UI_LineBox::flags_callback(Fl_Widget *w, void *data)
{
	line_flag_CB_data_c *l_f_c = (line_flag_CB_data_c *)data;

	UI_LineBox *box = l_f_c->parent;

	int flagSet = l_f_c->flagIndex;
	int mask = l_f_c->mask;
	int new_flags = box->CalcFlags();

	if (! box->inst.edit.Selected->empty())
	{
		box->mFixUp.checkDirtyFields();
		box->checkSidesDirtyFields();

		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited flags of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			const auto L = box->inst.level.linedefs[*it];

			// only change the bits specified in 'mask'.
			// this is important when multiple linedefs are selected.
			op.changeLinedef(*it, flagSet == 1 ? LineDef::F_FLAGS : LineDef::F_FLAGS2, ((flagSet == 1 ? L->flags : L->flags2) & ~mask) | (new_flags & mask));
		}
	}
}


void UI_LineBox::args_callback(Fl_Widget *w, void *data)
{
	line_flag_CB_data_c *l_f_c = (line_flag_CB_data_c *)data;

	UI_LineBox *box = l_f_c->parent;

	int arg_idx = l_f_c->mask;
	int new_value = atoi(box->args[arg_idx]->value());

	new_value = clamp(0, new_value, 255);

	if (! box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited args of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			op.changeLinedef(*it, static_cast<byte>(LineDef::F_ARG1 + arg_idx), new_value);
			if(!arg_idx && box->inst.loaded.levelFormat != MapFormat::udmf)
				op.changeLinedef(*it, LineDef::F_TAG, new_value);
		}
	}
}


void UI_LineBox::length_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	int new_len = atoi(box->length->value());

	// negative values are allowed, it moves the start vertex
	new_len = clamp(-32768, new_len, 32768);

	box->inst.level.linemod.setLinedefsLength(new_len);
}


void UI_LineBox::button_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	if (w == box->choose)
	{
		box->inst.main_win->BrowserMode(BrowserMode::lineTypes);
		return;
	}

	if (w == box->gen)
	{
		box->inst.main_win->BrowserMode(BrowserMode::generalized);
		return;
	}
}

void UI_LineBox::category_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;
	UI_CategoryButton *categoryBtn = (UI_CategoryButton *)w;
	box->categoryToggled(categoryBtn);
}

void UI_LineBox::categoryToggled(UI_CategoryButton *categoryBtn)
{
	// Find which category was toggled
	for(auto &cat : categoryHeaders)
	{
		if(cat.button.get() == categoryBtn)
		{
			cat.expanded = categoryBtn->isExpanded();

			// Show or hide all flags in this category
			for(Fl_Check_Button *flag : cat.flags)
			{
				if(cat.expanded)
					flag->show();
				else
					flag->hide();
			}

			// Reposition all controls below the flags area
			repositionAfterCategoryToggle();
			redraw();
			break;
		}
	}
}

void UI_LineBox::repositionAfterCategoryToggle()
{
	int Y = y() + flagsStartY;
	const int rowH = 19;
	const int categoryH = 22;

	// First, count uncategorized flags (not in any category)
	// These appear before categorized ones and we need to skip over them
	int uncategorizedCount = 0;
	for(const auto &fb : flagButtons)
	{
		bool inCategory = false;
		for(const auto &cat : categoryHeaders)
		{
			for(const auto *catFlag : cat.flags)
			{
				if(catFlag == fb.button.get())
				{
					inCategory = true;
					break;
				}
			}
			if(inCategory)
				break;
		}
		if(!inCategory)
			uncategorizedCount++;
	}

	// If there are uncategorized flags, skip over their space
	if(uncategorizedCount > 0)
	{
		const int leftCount = (uncategorizedCount + 1) / 2;
		const int rightCount = uncategorizedCount - leftCount;
		const int maxRows = (leftCount > rightCount) ? leftCount : rightCount;
		Y += maxRows * rowH;
	}

	// Calculate positions for all categories and their flags
	for(auto &cat : categoryHeaders)
	{
		if(cat.button)
		{
			cat.button->position(cat.button->x(), Y);
			Y += categoryH;

			if(cat.expanded)
			{
				// Position all flags in this category using two-column layout
				const int total = (int)cat.flags.size();
				const int leftCount = (total + 1) / 2;

				int yLeft = Y;
				int yRight = Y;

				for(int idx = 0; idx < total; ++idx)
				{
					Fl_Check_Button *flag = cat.flags[idx];
					const bool onLeft = idx < leftCount;
					int &curY = onLeft ? yLeft : yRight;

					flag->position(flag->x(), curY + 2);
					curY += rowH;
				}

				Y = (yLeft > yRight ? yLeft : yRight);
			}
		}
	}

	// Add spacing after flags
	Y += 29;

	// Reposition side boxes
	front->Fl_Widget::position(front->x(), Y);
	Y += front->h() + 14;
	back->Fl_Widget::position(back->x(), Y);
}


//------------------------------------------------------------------------

void UI_LineBox::UpdateField(int field)
{
	if (field < 0 || field == LineDef::F_START || field == LineDef::F_END)
	{
		if(inst.level.isLinedef(obj))
			CalcLength();
		else
			mFixUp.setInputValue(length, "");
	}

	if (field < 0 || (field >= LineDef::F_TAG && field <= LineDef::F_ARG5))
	{
		for (int a = 0 ; a < 5 ; a++)
		{
			mFixUp.setInputValue(args[a], "");
			args[a]->tooltip(NULL);
			args[a]->textcolor(FL_BLACK);
		}

		if (inst.level.isLinedef(obj))
		{
			const auto L = inst.level.linedefs[obj];

			mFixUp.setInputValue(tag, SString(inst.level.linedefs[obj]->tag).c_str());

			const linetype_t &info = inst.conf.getLineType(L->type);

			if (inst.loaded.levelFormat != MapFormat::doom)
			{
				for (int a = 0 ; a < 5 ; a++)
				{
					int arg_val = L->Arg(1 + a);

					if(arg_val || L->type)
						mFixUp.setInputValue(args[a], SString(arg_val).c_str());

					// set the tooltip
					if (!info.args[a].name.empty())
						args[a]->copy_tooltip(info.args[a].name.c_str());
					else
						args[a]->textcolor(fl_rgb_color(160,160,160));
				}
			}
		}
		else
		{
			mFixUp.setInputValue(length, "");
			mFixUp.setInputValue(tag, "");
		}
	}

	if (field < 0 || field == LineDef::F_RIGHT || field == LineDef::F_LEFT)
	{
		if (inst.level.isLinedef(obj))
		{
			const auto L = inst.level.linedefs[obj];

			int right_mask = SolidMask(L.get(), Side::right);
			int  left_mask = SolidMask(L.get(), Side::left);

			front->SetObj(L->right, right_mask, L->TwoSided());
			 back->SetObj(L->left,   left_mask, L->TwoSided());
		}
		else
		{
			front->SetObj(SETOBJ_NO_LINE, 0, false);
			 back->SetObj(SETOBJ_NO_LINE, 0, false);
		}
	}

	if (field < 0 || field == LineDef::F_TYPE)
	{
		if (inst.level.isLinedef(obj))
		{
			int type_num = inst.level.linedefs[obj]->type;

			mFixUp.setInputValue(type, SString(type_num).c_str());

			const char *gen_desc = GeneralizedDesc(type_num);

			if (gen_desc)
			{
				desc->value(gen_desc);
			}
			else
			{
				const linetype_t &info = inst.conf.getLineType(type_num);
				desc->value(info.desc.c_str());
			}

			inst.main_win->browser->UpdateGenType(type_num);
		}
		else
		{
			mFixUp.setInputValue(type, "");
			desc->value("");
			choose->label("Choose");

			inst.main_win->browser->UpdateGenType(0);
		}
	}

	if (field < 0 || field == LineDef::F_FLAGS)
	{
		if (inst.level.isLinedef(obj))
		{
			actkind->activate();

			FlagsFromInt(inst.level.linedefs[obj]->flags);
		}
		else
		{
			FlagsFromInt(0);

			actkind->value(getActivationCount());  // show as "??"
			actkind->deactivate();
		}
	}

	if(field < 0 || field == LineDef::F_FLAGS2)
	{
		if(inst.level.isLinedef(obj))
			Flags2FromInt(inst.level.linedefs[obj]->flags2);
		else
			Flags2FromInt(0);
	}
}


void UI_LineBox::UpdateSides()
{
	front->UpdateField();
	 back->UpdateField();
}


void UI_LineBox::UnselectPics()
{
	front->UnselectPics();
	 back->UnselectPics();
}


void UI_LineBox::CalcLength()
{
	// ASSERT(obj >= 0);

	int n = obj;

	float len_f = static_cast<float>(inst.level.calcLength(*inst.level.linedefs[n]));

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "%1.0f", len_f);

	mFixUp.setInputValue(length, buffer);
}


void UI_LineBox::FlagsFromInt(int lineflags)
{
	// compute activation
	if (inst.loaded.levelFormat == MapFormat::hexen)
	{
		int new_act = (lineflags & MLF_Activation) >> 9;

		new_act |= (lineflags & MLF_Repeatable) ? 1 : 0;

		// show "??" for unknown values
		int count = getActivationCount();
		if (new_act > count) new_act = count;

		actkind->value(new_act);
	}

	if (lineflags & MLF_DontDraw)
		f_automap->value(1);
	else if(lineflags & MLF_Mapped && lineflags & MLF_Secret)
		f_automap->value(4);
	else if (lineflags & MLF_Secret)
		f_automap->value(3);
	else if (lineflags & MLF_Mapped)
		f_automap->value(2);
	else
		f_automap->value(0);

	// Set dynamic line flag buttons
	for(const auto &btn : flagButtons)
	{
		if(btn.button && btn.data->flagIndex == 1)
			btn.button->value((lineflags & btn.info->value) ? 1 : 0);
	}
}

void UI_LineBox::Flags2FromInt(int lineflags)
{
	for(const auto &btn : flagButtons)
	{
		if(btn.button && btn.data->flagIndex == 2)
			btn.button->value((lineflags & btn.info->value) ? 1 : 0);
	}
}

int UI_LineBox::CalcFlags() const
{
	int lineflags = 0;

	switch (f_automap->value())
	{
		case 0: /* Normal    */; break;
		case 1: /* Invisible */ lineflags |= MLF_DontDraw; break;
		case 2: /* Mapped    */ lineflags |= MLF_Mapped; break;
		case 3: /* Secret    */ lineflags |= MLF_Secret; break;
		case 4: /* MapSecret */ lineflags |= MLF_Mapped | MLF_Secret; break;
	}

	// Activation for non-DOOM formats
	if (inst.loaded.levelFormat == MapFormat::hexen)
	{
		int actval = actkind->value();
		if (actval >= getActivationCount())
			actval = 0;
		lineflags |= (actval << 9);
	}

	// Apply dynamic flags
	for(const auto &btn : flagButtons)
	{
		if(btn.button && btn.button->value())
			lineflags |= btn.info->value;
	}

	return lineflags;
}


void UI_LineBox::UpdateTotal(const Document &doc) noexcept
{
	which->SetTotal(doc.numLinedefs());
}


int UI_LineBox::SolidMask(const LineDef *L, Side side) const
{
	SYS_ASSERT(L);

	if (L->left < 0 && L->right < 0)
		return 0;

	if (L->left < 0 || L->right < 0)
		return SOLID_MID;

	const Sector *right = &inst.level.getSector(*inst.level.getRight(*L));
	const Sector * left = &inst.level.getSector(*inst.level.getLeft(*L));

	if (side == Side::left)
		std::swap(left, right);

	int mask = 0;

	if (right->floorh < left->floorh)
		mask |= SOLID_LOWER;

	// upper texture of "-" is OK between two skies
	bool two_skies = inst.is_sky(right->CeilTex()) && inst.is_sky(left->CeilTex());

	if (right-> ceilh > left-> ceilh && !two_skies)
		mask |= SOLID_UPPER;

	return mask;
}


void UI_LineBox::UpdateGameInfo(const LoadingData &loaded, const ConfigData &config)
{
	choose->label("Choose");

	if (loaded.levelFormat == MapFormat::hexen)
	{
		tag->hide();
		descBox->show();
		length->hide();
		gen->hide();

		actkind->clear();
		actkind->add(getActivationMenuString());

		actkind->show();
		desc->resize(type->x() + 65, desc->y(), w()-78-65, desc->h());
	}
	else if(loaded.levelFormat == MapFormat::udmf)
	{
		tag->show();
		tag->resize(actkind->x(), actkind->y(), actkind->w(), actkind->h());
		tag->label("LineID:");
		descBox->hide();

		if(config.features.udmf_lineparameters)
			length->hide();
		else
			length->show();

		actkind->hide();	// UDMF uses the separate line flags for activation

		desc->resize(type->x() + 65, desc->y(), w()-78-65, desc->h());

		if (config.features.gen_types)
			gen->show();
		else
			gen->hide();
	}
	else
	{
		tag->show();
		tag->resize(type->x(), length->y(), TAG_WIDTH, TYPE_INPUT_HEIGHT);
		tag->label("Tag: ");
		descBox->show();

		length->show();

		actkind->hide();
		desc->resize(type->x(), desc->y(), w()-78, desc->h());

		if (config.features.gen_types)
			gen->show();
		else
			gen->hide();
	}

	// Recreate dynamic linedef flags from configuration
	for(const auto &btn : flagButtons)
		this->remove(btn.button.get());
	flagButtons.clear();
	for(const auto &cat : categoryHeaders)
		this->remove(cat.button.get());
	categoryHeaders.clear();

	int Y = y() + flagsStartY;

	const std::vector<lineflag_t> &flaglist = inst.loaded.levelFormat == MapFormat::udmf ?
			inst.conf.udmf_line_flags : inst.conf.line_flags;

	if(!flaglist.empty())
	{
		// Group flags by category (case-insensitive)
		struct CaseInsensitiveCompare
		{
			bool operator()(const SString &a, const SString &b) const
			{
				return a.noCaseCompare(b) < 0;
			}
		};
		std::map<SString, std::vector<const lineflag_t *>, CaseInsensitiveCompare> categorized;
		for(const lineflag_t &f : flaglist)
		{
			SString catName = f.category.empty() ? SString("") : f.category;
			categorized[catName].push_back(&f);
		}

		const int FW = 110;
		const int leftX = x() + flagsStartX + 28;
		const int rightX = x() + flagsStartX + flagsAreaW - 120;
		const int rowH = 19;
		const int categoryH = 22;

		begin();

		// Process each category
		for(auto &catPair : categorized)
		{
			const SString &catName = catPair.first;
			std::vector<const lineflag_t *> &flagsInCat = catPair.second;

			CategoryHeader catHeader = {};
			if(!catName.empty())
			{
				catHeader.button = std::make_unique<UI_CategoryButton>(x() + flagsStartX, Y, flagsAreaW, categoryH);
				catHeader.button->copy_label(catName.c_str());
				catHeader.button->callback(category_callback, this);
				catHeader.expanded = true;
				Y += categoryH;
			}

			struct Slot
			{
				const lineflag_t* a = nullptr;
				const lineflag_t* b = nullptr;
			};

			std::vector<Slot> slots;
			slots.reserve(flagsInCat.size());
			for(size_t i = 0; i < flagsInCat.size(); ++i)
			{
				const lineflag_t* f = flagsInCat[i];
				if(f->pairIndex == 0 && i + 1 < flagsInCat.size() && flagsInCat[i + 1]->pairIndex == 1)
				{
					Slot s;
					s.a = f;
					s.b = flagsInCat[i + 1];
					slots.push_back(s);
					++i; // consume the pair
				}
				else
				{
					Slot s;
					if(f->pairIndex == 1)
						s.b = f;
					else
						s.a = f;
					slots.push_back(s);
				}
			}

			const int total = (int)slots.size();
			const int leftCount = (total + 1) / 2;

			int yLeft = Y;
			int yRight = Y;

			for(int idx = 0; idx < total; ++idx)
			{
				const Slot& s = slots[idx];
				const bool onLeft = idx < leftCount;
				const int baseX = onLeft ? leftX : rightX;
				int& curY = onLeft ? yLeft : yRight;

				if(s.a)
				{
					LineFlagButton fb;
					fb.button = std::make_unique<Fl_Check_Button>(baseX, curY + 2, FW, 20, s.a->label.c_str());
					fb.button->labelsize(12);
					fb.data = std::make_unique<line_flag_CB_data_c>(this, s.a->flagSet, s.a->value);
					fb.button->callback(flags_callback, fb.data.get());
					fb.info = s.a;
					if(catHeader.button)
						catHeader.flags.push_back(fb.button.get());
					flagButtons.push_back(std::move(fb));
				}
				if(s.b)
				{
					LineFlagButton fb2;
					fb2.button = std::make_unique<Fl_Check_Button>(baseX + 16, curY + 2, FW, 20, s.b->label.c_str());
					fb2.button->labelsize(12);
					fb2.data = std::make_unique<line_flag_CB_data_c>(this, s.b->flagSet, s.b->value);
					fb2.button->callback(flags_callback, fb2.data.get());
					fb2.info = s.b;
					if(catHeader.button)
						catHeader.flags.push_back(fb2.button.get());
					flagButtons.push_back(std::move(fb2));
				}
				curY += rowH;
			}

			Y = (yLeft > yRight ? yLeft : yRight);
			if(catHeader.button)
				categoryHeaders.push_back(std::move(catHeader));
		}

		end();

		Y += 29;
	}
	else
	{
		// keep some spacing if no flags defined
		Y += 29;
	}


	// Reposition side boxes under the generated flags
	front->Fl_Widget::position(front->x(), Y);
	Y += front->h() + 14;
	back->Fl_Widget::position(back->x(), Y);
	front->redraw();
	back->redraw();

	// Show Hexen/UDMF args when needed
	for (int a = 0 ; a < 5 ; a++)
	{
		if (loaded.levelFormat == MapFormat::hexen ||
			(loaded.levelFormat == MapFormat::udmf && (config.features.udmf_lineparameters || !a)))
		{
			args[a]->show();
			if(loaded.levelFormat == MapFormat::hexen || config.features.udmf_lineparameters)
			{
				args[0]->resize(args[0]->x(), args[0]->y(), ARG_WIDTH, TYPE_INPUT_HEIGHT);
				args[0]->label("Args: ");
			}
			else
			{
				args[0]->resize(args[0]->x(), args[0]->y(), TAG_WIDTH, TYPE_INPUT_HEIGHT);
				args[0]->label("Target:");
			}
		}
		else
			args[a]->hide();
	}

	redraw();
}


const char * UI_LineBox::GeneralizedDesc(int type_num)
{
	if (! inst.conf.features.gen_types)
		return NULL;

	static char desc_buffer[256];

	for (int i = 0 ; i < inst.conf.num_gen_linetypes ; i++)
	{
		const generalized_linetype_t *info = &inst.conf.gen_linetypes[i];

		if (type_num >= info->base && type_num < (info->base + info->length))
		{
			// grab trigger name (we assume it is first field)
			if (info->fields.size() < 1 || info->fields[0].keywords.size() < 8)
				return NULL;

			const char *trigger = info->fields[0].keywords[type_num & 7].c_str();

			snprintf(desc_buffer, sizeof(desc_buffer), "%s GENTYPE: %s", trigger, info->name.c_str());
			return desc_buffer;
		}
	}

	return NULL;  // not a generalized linetype
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
