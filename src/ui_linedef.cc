//------------------------------------------------------------------------
//  LINEDEF PANEL
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

#include "main.h"
#include "ui_window.h"

#include "levels.h"
#include "e_checks.h"
#include "e_linedef.h"
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
UI_LineBox::UI_LineBox(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    obj(-1), count(0)
{
	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);


	X += 6;
	Y += 6;

	W -= 12;
	H -= 10;


	which = new UI_Nombre(X+6, Y, W-12, 28, "Linedef");

	Y += which->h() + 4;


	type = new Fl_Int_Input(X+58, Y, 65, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);
	type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	choose = new Fl_Button(X+W/2+15, Y, 80, 24, "Choose");
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


	f_walk = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "block walk");
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


	f_sound = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "sound block");
	f_sound->labelsize(12);
	f_sound->callback(flags_callback, new line_flag_CB_data_c(this, MLF_SoundBlock));


	Y += 19;


	f_3dmidtex = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "3D MidTex");
	f_3dmidtex->labelsize(12);
	f_3dmidtex->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Eternity_3DMidTex));
	f_3dmidtex->hide();


	Y += 24;


	front = new UI_SideBox(x(), Y, w(), 140, 0);

	Y += front->h() + 16;


	back = new UI_SideBox(x(), Y, w(), 140, 1);

	Y += back->h();


	end();

	resizable(NULL);
}


//
// UI_LineBox Destructor
//
UI_LineBox::~UI_LineBox()
{
}


void UI_LineBox::type_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	int new_type = atoi(box->type->value());

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it) ; !it.at_end() ; ++it)
		{
			BA_ChangeLD(*it, LineDef::F_TYPE, new_type);
		}

		BA_End();
	}

	// update description
	box->UpdateField(LineDef::F_TYPE);
}


void UI_LineBox::SetTexOnLine(int ld, int new_tex, int e_state,
                              int front_pics, int back_pics)
{
	bool opposite = (e_state & FL_SHIFT);

	LineDef *L = LineDefs[ld];

	// handle the selected texture boxes
	if (front_pics || back_pics)
	{
		if (L->OneSided())
		{
			if (front_pics & 1) BA_ChangeSD(L->right, SideDef::F_MID_TEX,   new_tex);
			if (front_pics & 2) BA_ChangeSD(L->right, SideDef::F_UPPER_TEX, new_tex);

			return;
		}

		if (L->Right())
		{
			if (front_pics & 1) BA_ChangeSD(L->right, SideDef::F_LOWER_TEX, new_tex);
			if (front_pics & 2) BA_ChangeSD(L->right, SideDef::F_UPPER_TEX, new_tex);
			if (front_pics & 4) BA_ChangeSD(L->right, SideDef::F_MID_TEX,   new_tex);
		}

		if (L->Left())
		{
			if (back_pics & 1) BA_ChangeSD(L->left, SideDef::F_LOWER_TEX, new_tex);
			if (back_pics & 2) BA_ChangeSD(L->left, SideDef::F_UPPER_TEX, new_tex);
			if (back_pics & 4) BA_ChangeSD(L->left, SideDef::F_MID_TEX,   new_tex);
		}
		return;
	}

	// middle click : set mid-masked texture on both sides
	if (e_state & FL_BUTTON2)
	{
		if (! L->TwoSided())
			return;

		// convenience: set lower unpeg on first change
		if (! (L->flags & MLF_LowerUnpegged) &&
		    L->Right()->MidTex()[0] == '-' &&
		    L-> Left()->MidTex()[0] == '-')
		{
			BA_ChangeLD(ld, LineDef::F_FLAGS, L->flags | MLF_LowerUnpegged);
		}

		BA_ChangeSD(L->left,  SideDef::F_MID_TEX, new_tex);
		BA_ChangeSD(L->right, SideDef::F_MID_TEX, new_tex);
		return;
	}

	// one-sided case: set the middle texture only
	if (! L->TwoSided())
	{
		int sd = (L->right >= 0) ? L->right : L->left;

		if (sd < 0)
			return;

		BA_ChangeSD(sd, SideDef::F_MID_TEX, new_tex);
		return;
	}

	// modify an upper texture
	int sd1 = L->right;
	int sd2 = L->left;

	if (e_state & FL_BUTTON3)
	{
		// back ceiling is higher?
		if (L->Left()->SecRef()->ceilh > L->Right()->SecRef()->ceilh)
			std::swap(sd1, sd2);

		if (opposite)
			std::swap(sd1, sd2);

		BA_ChangeSD(sd1, SideDef::F_UPPER_TEX, new_tex);
	}
	// modify a lower texture
	else
	{
		// back floor is lower?
		if (L->Left()->SecRef()->floorh < L->Right()->SecRef()->floorh)
			std::swap(sd1, sd2);

		if (opposite)
			std::swap(sd1, sd2);

		SideDef *S = SideDefs[sd1];

		// change BOTH upper and lower when they are the same
		// (which is great for windows).
		//
		// Note: we only do this for LOWERS (otherwise it'd be
		// impossible to set them to different textures).

		if (S->lower_tex == S->upper_tex)
			BA_ChangeSD(sd1, SideDef::F_UPPER_TEX, new_tex);

		BA_ChangeSD(sd1, SideDef::F_LOWER_TEX, new_tex);
	}
}


void UI_LineBox::SetTexture(const char *tex_name, int e_state)
{
	int new_tex = BA_InternaliseString(tex_name);

	int front_pics = front->GetSelectedPics();
	int  back_pics =  back->GetSelectedPics();

	// this works a bit differently than other ways, we don't modify a
	// widget and let it update the map, instead we update the map and
	// let the widget(s) get updated.  That's because we do different
	// things depending on whether the line is one-sided or two-sided.

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it) ; !it.at_end() ; ++it)
		{
			SetTexOnLine(*it, new_tex, e_state, front_pics, back_pics);
		}

		BA_End();
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

	sprintf(buffer, "%d", new_type);

	type->value(buffer);
	type->do_callback();
}


void UI_LineBox::tag_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	int new_tag = atoi(box->tag->value());

	Tags_ApplyNewValue(new_tag);
}


void UI_LineBox::flags_callback(Fl_Widget *w, void *data)
{
	line_flag_CB_data_c *l_f_c = (line_flag_CB_data_c *)data;

	UI_LineBox *box = l_f_c->parent;

	int mask = l_f_c->mask;
	int new_flags = box->CalcFlags();

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
	
		for (list.begin(&it); !it.at_end(); ++it)
		{
			const LineDef *L = LineDefs[*it];

			// only change the bits specified in 'mask'.
			// this is important when multiple linedefs are selected.
			BA_ChangeLD(*it, LineDef::F_FLAGS, (L->flags & ~mask) | (new_flags & mask));
		}

		BA_End();
	}
}


void UI_LineBox::args_callback(Fl_Widget *w, void *data)
{
	line_flag_CB_data_c *l_f_c = (line_flag_CB_data_c *)data;

	UI_LineBox *box = l_f_c->parent;

	int arg_idx = l_f_c->mask;
	int new_value = atoi(box->args[arg_idx]->value());

	new_value = CLAMP(0, new_value, 255);

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();
	
		for (list.begin(&it); !it.at_end(); ++it)
		{
			BA_ChangeLD(*it, LineDef::F_TAG + arg_idx, new_value);
		}

		BA_End();
	}
}


void UI_LineBox::length_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	int new_len = atoi(box->length->value());

	new_len = CLAMP(1, new_len, 32768);

	LineDefs_SetLength(new_len);
}


void UI_LineBox::button_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	if (w == box->choose)
	{
		int cur_type = atoi(box->type->value());

		if ((game_info.gen_types && cur_type >= 0x2f80 && cur_type <= 0x7fff) ||
			Fl::event_button() == 3)
		{
			main_win->ShowBrowser('G');
			return;
		}

		main_win->ShowBrowser('L');
		return;
	}

	if (w == box->gen)
	{
		main_win->ShowBrowser('G');
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
		if (is_linedef(obj))
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
		}

		if (is_linedef(obj))
		{
			const LineDef *L = LineDefs[obj];

			tag->value(Int_TmpStr(LineDefs[obj]->tag));

			const linetype_t *info = M_GetLineType(L->type);

			if (Level_format == MAPF_Hexen)
			{
				if (L->tag  || info->args[0]) args[0]->value(Int_TmpStr(L->tag));
				if (L->arg2 || info->args[1]) args[1]->value(Int_TmpStr(L->arg2));
				if (L->arg3 || info->args[2]) args[2]->value(Int_TmpStr(L->arg3));
				if (L->arg4 || info->args[3]) args[3]->value(Int_TmpStr(L->arg4));
				if (L->arg5 || info->args[4]) args[4]->value(Int_TmpStr(L->arg5));

				// set tooltips
				for (int a = 0 ; a < 5 ; a++)
					if (info->args[a])
						args[a]->copy_tooltip(info->args[a]);
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
		if (is_linedef(obj))
		{
			const LineDef *L = LineDefs[obj];

			front->SetObj(L->right, SolidMask(SIDE_RIGHT), L->TwoSided());
			 back->SetObj(L->left,  SolidMask(SIDE_LEFT),  L->TwoSided());
		}
		else
		{
			front->SetObj(SETOBJ_NO_LINE, 0, false);
			 back->SetObj(SETOBJ_NO_LINE, 0, false);
		}
	}

	if (field < 0 || field == LineDef::F_TYPE)
	{
		if (is_linedef(obj))
		{
			int type_num = LineDefs[obj]->type;

			type->value(Int_TmpStr(type_num));

			const char *gen_desc = GeneralizedDesc(type_num);

			if (gen_desc)
			{
				desc->value(gen_desc);
			}
			else
			{
				const linetype_t *info = M_GetLineType(type_num);
				desc->value(info->desc);
			}

			main_win->browser->UpdateGenType(type_num);
		}
		else
		{
			type->value("");
			desc->value("");

			//??  main_win->browser->UpdateGenType(0);
		}
	}

	if (field < 0 || field == LineDef::F_FLAGS)
	{
		if (is_linedef(obj))
		{
			actkind->activate();

			FlagsFromInt(LineDefs[obj]->flags);
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

	float len_f = LineDefs[n]->CalcLength();

	char buffer[300];

	if (int(len_f) >= 10000)
		sprintf(buffer, "%1.0f", len_f);
	else if (int(len_f) >= 100)
		sprintf(buffer, "%1.1f", len_f);
	else
		sprintf(buffer, "%1.2f", len_f);

	length->value(buffer);
}


void UI_LineBox::FlagsFromInt(int lineflags)
{
	// compute activation
	if (Level_format == MAPF_Hexen)
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

	f_walk ->value((lineflags & MLF_Blocking)      ? 1 : 0);
	f_mons ->value((lineflags & MLF_BlockMonsters) ? 1 : 0);
	f_sound->value((lineflags & MLF_SoundBlock)    ? 1 : 0);
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

	if (Level_format == MAPF_Hexen)
	{
		int actval = actkind->value();
		if (actval >= 12) actval = 0;

		lineflags |= (actval << 9);
	}
	else
	{
		if (game_info.pass_through && f_passthru->value())
			lineflags |= MLF_Boom_PassThru;

		if (game_info.midtex_3d && f_3dmidtex->value())
			lineflags |= MLF_Eternity_3DMidTex;
	}

	return lineflags;
}


void UI_LineBox::UpdateTotal()
{
	which->SetTotal(NumLineDefs);
}


int UI_LineBox::SolidMask(int side)
{
	SYS_ASSERT(is_linedef(obj));

	const LineDef *L = LineDefs[obj];

	if (L->left < 0 && L->right < 0)
		return 0;
	
	if (L->left < 0 || L->right < 0)
		return SOLID_MID;

	Sector *right = L->Right()->SecRef();
	Sector * left = L->Left ()->SecRef();

	if (side == SIDE_LEFT)
		std::swap(left, right);

	int mask = 0;

	if (right->floorh < left->floorh)
		mask |= SOLID_LOWER;

	// upper texture of '-' is OK between two skies
	bool two_skies = is_sky(right->CeilTex()) && is_sky(left->CeilTex());

	if (right-> ceilh > left-> ceilh && ! two_skies)
		mask |= SOLID_UPPER;

	return mask;
}


void UI_LineBox::UpdateGameInfo()
{
	if (Level_format == MAPF_Hexen)
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

		if (game_info.pass_through)
			f_passthru->show();
		else
			f_passthru->hide();

		if (game_info.midtex_3d)
			f_3dmidtex->show();
		else
			f_3dmidtex->hide();
#if 0
		if (game_info.gen_types)
			gen->show();
		else
#endif
			gen->hide();
	}

	for (int a = 0 ; a < 5 ; a++)
	{
		if (Level_format == MAPF_Hexen)
			args[a]->show();
		else
			args[a]->hide();
	}

	redraw();
}


const char * UI_LineBox::GeneralizedDesc(int type_num)
{
	if (! game_info.gen_types)
		return NULL;

	static char desc_buffer[256];

	for (int i = 0 ; i < num_gen_linetypes ; i++)
	{
		const generalized_linetype_t *info = &gen_linetypes[i];

		if (type_num >= info->base && type_num < (info->base + info->length))
		{
			// grab trigger name (we assume it is first field)
			if (info->num_fields < 1 || info->fields[0].num_keywords < 8)
				return NULL;

			const char *trigger = info->fields[0].keywords[type_num & 7];

			sprintf(desc_buffer, "%s GENTYPE: %s", trigger, info->name);
			return desc_buffer;
		}
	}

	return NULL;  // not a generalized linetype
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
