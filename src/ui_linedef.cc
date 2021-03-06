//------------------------------------------------------------------------
//  LINEDEF PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2018 Andrew Apted
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
#include "ui_window.h"

#include "e_checks.h"
#include "e_cutpaste.h"
#include "e_linedef.h"
#include "e_main.h"
#include "e_things.h"
#include "m_game.h"
#include "w_rawdef.h"
#include "w_rawdef.h"


#define MLF_ALL_AUTOMAP  \
	MLF_Secret | MLF_Mapped | MLF_DontDraw | MLF_XDoom_Translucent


class line_flag_CB_data_c
{
public:
	UI_LineBox *parent;

	int mask;

public:
	line_flag_CB_data_c(UI_LineBox *_parent, int _mask) :
		parent(_parent), mask(_mask)
	{ }

	~line_flag_CB_data_c()
	{ }
};


//
// UI_LineBox Constructor
//
UI_LineBox::UI_LineBox(Instance &inst, int X, int Y, int W, int H, const char *label) :
    Fl_Group(X, Y, W, H, label),
    obj(-1), count(0), inst(inst)
{
	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);


	X += 6;
	Y += 6;

	W -= 12;
	H -= 10;


	which = new UI_Nombre(X+6, Y, W-12, 28, "Linedef");

	Y += which->h() + 4;


	type = new UI_DynInput(X+58, Y, 75, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);
	type->callback2(dyntype_callback, this);
	type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
	type->type(FL_INT_INPUT);

	choose = new Fl_Button(X+W/2-5, Y, 80, 24, "Choose");
	choose->callback(button_callback, this);

	gen = new Fl_Button(X+W-60, Y, 50, 24, "Gen");
	gen->callback(button_callback, this);

	Y += type->h() + 2;


	new Fl_Box(FL_NO_BOX, X+10, Y, 48, 24, "Desc: ");

	desc = new Fl_Output(type->x(), Y, W-66, 24);
	desc->align(FL_ALIGN_LEFT);


	actkind = new Fl_Choice(X+58, Y, 57, 24);
	// this order must match the SPAC_XXX constants
	actkind->add("W1|WR|S1|SR|M1|MR|G1|GR|P1|PR|X1|XR|??");
	actkind->value(12);
	actkind->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Activation | MLF_Repeatable));
	actkind->deactivate();
	actkind->hide();

	Y += desc->h() + 2;


	tag = new Fl_Int_Input(X+58, Y, 64, 24, "Tag: ");
	tag->align(FL_ALIGN_LEFT);
	tag->callback(tag_callback, this);
	tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	length = new Fl_Int_Input(X+W/2+58, Y, 64, 24, "Length: ");
	length->align(FL_ALIGN_LEFT);
	length->callback(length_callback, this);
	length->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	for (int a = 0 ; a < 5 ; a++)
	{
		args[a] = new Fl_Int_Input(X+58+47*a, Y, 42, 24);
		args[a]->callback(args_callback, new line_flag_CB_data_c(this, a));
		args[a]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		args[a]->hide();
	}

	args[0]->label("Args: ");


	Y += tag->h() + 10;


	Fl_Box *flags = new Fl_Box(FL_FLAT_BOX, X+10, Y, 64, 24, "Flags: ");
	flags->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);


	f_automap = new Fl_Choice(X+W-118, Y, 104, 22, "Vis: ");
	f_automap->add("Normal|Invisible|Mapped|Secret");
	f_automap->value(0);
	f_automap->callback(flags_callback, new line_flag_CB_data_c(this, MLF_ALL_AUTOMAP));


	Y += flags->h() - 1;


	int FW = 110;

	f_upper = new Fl_Check_Button(X+28, Y+2, FW, 20, "upper unpeg");
	f_upper->labelsize(12);
	f_upper->callback(flags_callback, new line_flag_CB_data_c(this, MLF_UpperUnpegged));

	f_walk = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "impassible");
	f_walk->labelsize(12);
	f_walk->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Blocking));

	Y += 19;


	f_lower = new Fl_Check_Button(X+28, Y+2, FW, 20, "lower unpeg");
	f_lower->labelsize(12);
	f_lower->callback(flags_callback, new line_flag_CB_data_c(this, MLF_LowerUnpegged));

	f_mons = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "block mons");
	f_mons->labelsize(12);
	f_mons->callback(flags_callback, new line_flag_CB_data_c(this, MLF_BlockMonsters));

	Y += 19;


	f_passthru = new Fl_Check_Button(X+28, Y+2, FW, 20, "pass thru");
	f_passthru->labelsize(12);
	f_passthru->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Boom_PassThru));
	f_passthru->hide();

	f_jumpover = new Fl_Check_Button(X+28, Y+2, FW, 20, "jump over");
	f_jumpover->labelsize(12);
	f_jumpover->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Strife_JumpOver));
	f_jumpover->hide();

	f_sound = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "sound block");
	f_sound->labelsize(12);
	f_sound->callback(flags_callback, new line_flag_CB_data_c(this, MLF_SoundBlock));

	Y += 19;


	f_3dmidtex = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "3D MidTex");
	f_3dmidtex->labelsize(12);
	f_3dmidtex->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Eternity_3DMidTex));
	f_3dmidtex->hide();


	f_trans1 = new Fl_Check_Button(X+28, Y+2, FW, 20, "");
	f_trans1->labelsize(12);
	f_trans1->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Strife_Translucent1));
	f_trans1->hide();

	f_trans2 = new Fl_Check_Button(X+44, Y+2, FW, 20, "translucency");
	f_trans2->labelsize(12);
	f_trans2->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Strife_Translucent2));
	f_trans2->hide();


	f_flyers = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "block flyers");
	f_flyers->labelsize(12);
	f_flyers->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Strife_BlockFloaters));
	f_flyers->hide();

	Y += 29;


	front = new UI_SideBox(inst, x(), Y, w(), 140, 0);

	Y += front->h() + 14;


	back = new UI_SideBox(inst, x(), Y, w(), 140, 1);

	Y += back->h();


	end();

	resizable(NULL);
}


//
// UI_LineBox Destructor
//
UI_LineBox::~UI_LineBox()
{ }


void UI_LineBox::type_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	// support hexadecimal
	int new_type = (int)strtol(box->type->value(), NULL, 0);

	if (! box->inst.edit.Selected->empty())
	{
		box->inst.level.basis.begin();
		box->inst.level.basis.setMessageForSelection("edited type of", *box->inst.edit.Selected);

		for (sel_iter_c it(box->inst.edit.Selected) ; !it.done() ; it.next())
		{
			box->inst.level.basis.changeLinedef(*it, LineDef::F_TYPE, new_type);
		}

		box->inst.level.basis.end();
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
		const linetype_t &info = box->inst.M_GetLineType(new_type);
		box->desc->value(info.desc.c_str());
	}

	box->inst.main_win->browser->UpdateGenType(new_type);
}


void UI_LineBox::SetTexOnLine(int ld, int new_tex, int e_state, int parts)
{
	bool opposite = (e_state & FL_SHIFT);

	LineDef *L = inst.level.linedefs[ld];

	// handle the selected texture boxes
	if (parts != 0)
	{
		if (L->OneSided())
		{
			if (parts & PART_RT_LOWER) inst.level.basis.changeSidedef(L->right, SideDef::F_MID_TEX,   new_tex);
			if (parts & PART_RT_UPPER) inst.level.basis.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_tex);

			return;
		}

		if (L->Right(inst.level))
		{
			if (parts & PART_RT_LOWER) inst.level.basis.changeSidedef(L->right, SideDef::F_LOWER_TEX, new_tex);
			if (parts & PART_RT_UPPER) inst.level.basis.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_tex);
			if (parts & PART_RT_RAIL)  inst.level.basis.changeSidedef(L->right, SideDef::F_MID_TEX,   new_tex);
		}

		if (L->Left(inst.level))
		{
			if (parts & PART_LF_LOWER) inst.level.basis.changeSidedef(L->left, SideDef::F_LOWER_TEX, new_tex);
			if (parts & PART_LF_UPPER) inst.level.basis.changeSidedef(L->left, SideDef::F_UPPER_TEX, new_tex);
			if (parts & PART_LF_RAIL)  inst.level.basis.changeSidedef(L->left, SideDef::F_MID_TEX,   new_tex);
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
		    is_null_tex(L->Right(inst.level)->MidTex()) &&
		    is_null_tex(L-> Left(inst.level)->MidTex()) )
		{
			inst.level.basis.changeLinedef(ld, LineDef::F_FLAGS, L->flags | MLF_LowerUnpegged);
		}

		inst.level.basis.changeSidedef(L->left,  SideDef::F_MID_TEX, new_tex);
		inst.level.basis.changeSidedef(L->right, SideDef::F_MID_TEX, new_tex);
		return;
	}

	// one-sided case: set the middle texture only
	if (! L->TwoSided())
	{
		int sd = (L->right >= 0) ? L->right : L->left;

		if (sd < 0)
			return;

		inst.level.basis.changeSidedef(sd, SideDef::F_MID_TEX, new_tex);
		return;
	}

	// modify an upper texture
	int sd1 = L->right;
	int sd2 = L->left;

	if (e_state & FL_BUTTON3)
	{
		// back ceiling is higher?
		if (L->Left(inst.level)->SecRef(inst.level)->ceilh > L->Right(inst.level)->SecRef(inst.level)->ceilh)
			std::swap(sd1, sd2);

		if (opposite)
			std::swap(sd1, sd2);

		inst.level.basis.changeSidedef(sd1, SideDef::F_UPPER_TEX, new_tex);
	}
	// modify a lower texture
	else
	{
		// back floor is lower?
		if (L->Left(inst.level)->SecRef(inst.level)->floorh < L->Right(inst.level)->SecRef(inst.level)->floorh)
			std::swap(sd1, sd2);

		if (opposite)
			std::swap(sd1, sd2);

		SideDef *S = inst.level.sidedefs[sd1];

		// change BOTH upper and lower when they are the same
		// (which is great for windows).
		//
		// Note: we only do this for LOWERS (otherwise it'd be
		// impossible to set them to different textures).

		if (S->lower_tex == S->upper_tex)
			inst.level.basis.changeSidedef(sd1, SideDef::F_UPPER_TEX, new_tex);

		inst.level.basis.changeSidedef(sd1, SideDef::F_LOWER_TEX, new_tex);
	}
}


void UI_LineBox::SetTexture(const char *tex_name, int e_state, int parts)
{
	int new_tex = BA_InternaliseString(tex_name);

	// this works a bit differently than other ways, we don't modify a
	// widget and let it update the map, instead we update the map and
	// let the widget(s) get updated.  That's because we do different
	// things depending on whether the line is one-sided or two-sided.

	if (! inst.edit.Selected->empty())
	{
		inst.level.basis.begin();
		inst.level.basis.setMessageForSelection("edited texture on", *inst.edit.Selected);

		for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
		{
			int p2 = inst.edit.Selected->get_ext(*it);

			// only use parts explicitly selected in 3D view when no
			// parts in the linedef panel are selected.
			if (! (parts == 0 && p2 > 1))
				p2 = parts;

			SetTexOnLine(*it, new_tex, e_state, p2);
		}

		inst.level.basis.end();
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

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%d", new_type);

	type->value(buffer);
	type->do_callback();
}


void UI_LineBox::CB_Copy(int parts)
{
	// determine which sidedef texture to grab from
	const char *name = NULL;

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		UI_SideBox *SD = (pass == 0) ? front : back;

		for (int b = 0 ; b < 3 ; b++)
		{
			int try_part = PART_RT_LOWER << (b + pass * 4);

			if ((parts & try_part) == 0)
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

	inst.Texboard_SetTex(name);

	inst.Status_Set("copied %s", name);
}


void UI_LineBox::CB_Paste(int parts, int new_tex)
{
	// iterate over selected linedefs
	if (inst.edit.Selected->empty())
		return;

	inst.level.basis.begin();
	inst.level.basis.setMessage("pasted %s", BA_GetString(new_tex).c_str());

	for (sel_iter_c it(inst.edit.Selected) ; !it.done() ; it.next())
	{
		const LineDef *L = inst.level.linedefs[*it];

		for (int pass = 0 ; pass < 2 ; pass++)
		{
			int sd = (pass == 0) ? L->right : L->left;
			if (sd < 0)
				continue;

			int parts2 = pass ? (parts >> 4) : parts;

			if (L->TwoSided())
			{
				if (parts2 & PART_RT_LOWER)
					inst.level.basis.changeSidedef(sd, SideDef::F_LOWER_TEX, new_tex);

				if (parts2 & PART_RT_UPPER)
					inst.level.basis.changeSidedef(sd, SideDef::F_UPPER_TEX, new_tex);

				if (parts2 & PART_RT_RAIL)
					inst.level.basis.changeSidedef(sd, SideDef::F_MID_TEX, new_tex);
			}
			else  // one-sided line
			{
				if (parts2 & PART_RT_LOWER)
					inst.level.basis.changeSidedef(sd, SideDef::F_MID_TEX, new_tex);
			}
		}
	}

	inst.level.basis.end();

	UpdateField();
	UpdateSides();

	redraw();
}


bool UI_LineBox::ClipboardOp(EditCommand op)
{
	if (obj < 0)
		return false;

	int parts = front->GetSelectedPics() | (back->GetSelectedPics() << 4);

	if (parts == 0)
		parts = front->GetHighlightedPics() | (back->GetHighlightedPics() << 4);

	if (parts == 0)
		return false;

	switch (op)
	{
		case EditCommand::copy:
			CB_Copy(parts);
			break;

		case EditCommand::paste:
			CB_Paste(parts, inst.Texboard_GetTexNum());
			break;

		case EditCommand::cut:	// Cut
			CB_Paste(parts, BA_InternaliseString(inst.default_wall_tex));
			break;

		case EditCommand::del: // Delete
			CB_Paste(parts, BA_InternaliseString("-"));
			break;
	}

	return true;
}


void UI_LineBox::BrowsedItem(char kind, int number, const char *name, int e_state)
{
	if (kind == 'T' || kind == 'F')
	{
		int front_pics = front->GetSelectedPics();
		int  back_pics =  back->GetSelectedPics();

		// this can be zero, invoking special behavior (based on mouse button)
		int parts = front_pics | (back_pics << 4);

		SetTexture(name, e_state, parts);
	}
	else if (kind == 'L')
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

	int mask = l_f_c->mask;
	int new_flags = box->CalcFlags();

	if (! box->inst.edit.Selected->empty())
	{
		box->inst.level.basis.begin();
		box->inst.level.basis.setMessageForSelection("edited flags of", *box->inst.edit.Selected);

		for (sel_iter_c it(box->inst.edit.Selected); !it.done(); it.next())
		{
			const LineDef *L = box->inst.level.linedefs[*it];

			// only change the bits specified in 'mask'.
			// this is important when multiple linedefs are selected.
			box->inst.level.basis.changeLinedef(*it, LineDef::F_FLAGS, (L->flags & ~mask) | (new_flags & mask));
		}

		box->inst.level.basis.end();
	}
}


void UI_LineBox::args_callback(Fl_Widget *w, void *data)
{
	line_flag_CB_data_c *l_f_c = (line_flag_CB_data_c *)data;

	UI_LineBox *box = l_f_c->parent;

	int arg_idx = l_f_c->mask;
	int new_value = atoi(box->args[arg_idx]->value());

	new_value = CLAMP(0, new_value, 255);

	if (! box->inst.edit.Selected->empty())
	{
		box->inst.level.basis.begin();
		box->inst.level.basis.setMessageForSelection("edited args of", *box->inst.edit.Selected);

		for (sel_iter_c it(box->inst.edit.Selected); !it.done(); it.next())
		{
			box->inst.level.basis.changeLinedef(*it, static_cast<byte>(LineDef::F_TAG + arg_idx),
                                                new_value);
		}

		box->inst.level.basis.end();
	}
}


void UI_LineBox::length_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	int new_len = atoi(box->length->value());

	// negative values are allowed, it moves the start vertex
	new_len = CLAMP(-32768, new_len, 32768);

	box->inst.level.linemod.setLinedefsLength(new_len);
}


void UI_LineBox::button_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	if (w == box->choose)
	{
		box->inst.main_win->BrowserMode('L');
		return;
	}

	if (w == box->gen)
	{
		box->inst.main_win->BrowserMode('G');
		return;
	}
}


//------------------------------------------------------------------------

void UI_LineBox::SetObj(int _index, int _count)
{
	if (obj == _index && count == _count)
		return;

	obj   = _index;
	count = _count;

	which->SetIndex(obj);
	which->SetSelected(count);

	UpdateField();

	if (obj < 0)
		UnselectPics();

	redraw();
}


void UI_LineBox::UpdateField(int field)
{
	if (field < 0 || field == LineDef::F_START || field == LineDef::F_END)
	{
		if (inst.level.isLinedef(obj))
			CalcLength();
		else
			length->value("");
	}

	if (field < 0 || (field >= LineDef::F_TAG && field <= LineDef::F_ARG5))
	{
		for (int a = 0 ; a < 5 ; a++)
		{
			args[a]->value("");
			args[a]->tooltip(NULL);
			args[a]->textcolor(FL_BLACK);
		}

		if (inst.level.isLinedef(obj))
		{
			const LineDef *L = inst.level.linedefs[obj];

			tag->value(SString(inst.level.linedefs[obj]->tag).c_str());

			const linetype_t &info = inst.M_GetLineType(L->type);

			if (inst.Level_format != MapFormat::doom)
			{
				for (int a = 0 ; a < 5 ; a++)
				{
					int arg_val = L->Arg(1 + a);

					if (arg_val || L->type)
						args[a]->value(SString(arg_val).c_str());

					// set the tooltip
					if (!info.args[a].empty())
						args[a]->copy_tooltip(info.args[a].c_str());
					else
						args[a]->textcolor(fl_rgb_color(160,160,160));
				}
			}
		}
		else
		{
			length->value("");
			tag->value("");
		}
	}

	if (field < 0 || field == LineDef::F_RIGHT || field == LineDef::F_LEFT)
	{
		if (inst.level.isLinedef(obj))
		{
			const LineDef *L = inst.level.linedefs[obj];

			int right_mask = SolidMask(L, Side::right);
			int  left_mask = SolidMask(L, Side::left);

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

			type->value(SString(type_num).c_str());

			const char *gen_desc = GeneralizedDesc(type_num);

			if (gen_desc)
			{
				desc->value(gen_desc);
			}
			else
			{
				const linetype_t &info = inst.M_GetLineType(type_num);
				desc->value(info.desc.c_str());
			}

			inst.main_win->browser->UpdateGenType(type_num);
		}
		else
		{
			type->value("");
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

			actkind->value(12);  // show as "??"
			actkind->deactivate();
		}
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

	float len_f = static_cast<float>(inst.level.linedefs[n]->CalcLength(inst.level));

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "%1.0f", len_f);

	length->value(buffer);
}


void UI_LineBox::FlagsFromInt(int lineflags)
{
	// compute activation
	if (inst.Level_format != MapFormat::doom)
	{
		int new_act = (lineflags & MLF_Activation) >> 9;

		new_act |= (lineflags & MLF_Repeatable) ? 1 : 0;

		// show "??" for unknown values
		if (new_act > 12) new_act = 12;

		actkind->value(new_act);
	}

	if (lineflags & MLF_Secret)
		f_automap->value(3);
	else if (lineflags & MLF_DontDraw)
		f_automap->value(1);
	else if (lineflags & MLF_Mapped)
		f_automap->value(2);
	else
		f_automap->value(0);

	f_upper   ->value((lineflags & MLF_UpperUnpegged) ? 1 : 0);
	f_lower   ->value((lineflags & MLF_LowerUnpegged) ? 1 : 0);
	f_passthru->value((lineflags & MLF_Boom_PassThru) ? 1 : 0);
	f_3dmidtex->value((lineflags & MLF_Eternity_3DMidTex) ? 1 : 0);

	f_jumpover->value((lineflags & MLF_Strife_JumpOver)   ? 1 : 0);
	f_trans1  ->value((lineflags & MLF_Strife_Translucent1) ? 1 : 0);
	f_trans2  ->value((lineflags & MLF_Strife_Translucent2) ? 1 : 0);

	f_walk  ->value((lineflags & MLF_Blocking)      ? 1 : 0);
	f_mons  ->value((lineflags & MLF_BlockMonsters) ? 1 : 0);
	f_sound ->value((lineflags & MLF_SoundBlock)    ? 1 : 0);
	f_flyers->value((lineflags & MLF_Strife_BlockFloaters) ? 1 : 0);
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
	}

	if (f_upper->value())    lineflags |= MLF_UpperUnpegged;
	if (f_lower->value())    lineflags |= MLF_LowerUnpegged;

	if (f_walk->value())  lineflags |= MLF_Blocking;
	if (f_mons->value())  lineflags |= MLF_BlockMonsters;
	if (f_sound->value()) lineflags |= MLF_SoundBlock;

	if (inst.Level_format != MapFormat::doom)
	{
		int actval = actkind->value();
		if (actval >= 12) actval = 0;

		lineflags |= (actval << 9);
	}
	else
	{
		if (inst.Features.pass_through && f_passthru->value())
			lineflags |= MLF_Boom_PassThru;

		if (inst.Features.midtex_3d && f_3dmidtex->value())
			lineflags |= MLF_Eternity_3DMidTex;

		if (inst.Features.strife_flags)
		{
			if (f_jumpover->value())
				lineflags |= MLF_Strife_JumpOver;

			if (f_flyers->value())
				lineflags |= MLF_Strife_BlockFloaters;

			if (f_trans1->value())
				lineflags |= MLF_Strife_Translucent1;

			if (f_trans2->value())
				lineflags |= MLF_Strife_Translucent2;
		}
	}

	return lineflags;
}


void UI_LineBox::UpdateTotal()
{
	which->SetTotal(inst.level.numLinedefs());
}


int UI_LineBox::SolidMask(const LineDef *L, Side side) const
{
	SYS_ASSERT(L);

	if (L->left < 0 && L->right < 0)
		return 0;

	if (L->left < 0 || L->right < 0)
		return SOLID_MID;

	Sector *right = L->Right(inst.level)->SecRef(inst.level);
	Sector * left = L->Left (inst.level)->SecRef(inst.level);

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


void UI_LineBox::UpdateGameInfo()
{
	choose->label("Choose");

	if (inst.Level_format != MapFormat::doom)
	{
		tag->hide();
		length->hide();
		gen->hide();

		actkind->show();
		desc->resize(type->x() + 65, desc->y(), w()-78-65, desc->h());

		f_passthru->hide();
		f_3dmidtex->hide();
	}
	else
	{
		tag->show();
		length->show();

		actkind->hide();
		desc->resize(type->x(), desc->y(), w()-78, desc->h());

		if (inst.Features.pass_through)
			f_passthru->show();
		else
			f_passthru->hide();

		if (inst.Features.midtex_3d)
			f_3dmidtex->show();
		else
			f_3dmidtex->hide();

		if (inst.Features.strife_flags)
		{
			f_jumpover->show();
			f_flyers->show();
			f_trans1->show();
			f_trans2->show();
		}
		else
		{
			f_jumpover->hide();
			f_flyers->hide();
			f_trans1->hide();
			f_trans2->hide();
		}

		if (inst.Features.gen_types)
			gen->show();
		else
			gen->hide();
	}

	for (int a = 0 ; a < 5 ; a++)
	{
		if (inst.Level_format != MapFormat::doom)
			args[a]->show();
		else
			args[a]->hide();
	}

	redraw();
}


const char * UI_LineBox::GeneralizedDesc(int type_num)
{
	if (! inst.Features.gen_types)
		return NULL;

	static char desc_buffer[256];

	for (int i = 0 ; i < inst.num_gen_linetypes ; i++)
	{
		const generalized_linetype_t *info = &inst.gen_linetypes[i];

		if (type_num >= info->base && type_num < (info->base + info->length))
		{
			// grab trigger name (we assume it is first field)
			if (info->num_fields < 1 || info->fields[0].num_keywords < 8)
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
