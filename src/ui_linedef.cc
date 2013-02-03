//------------------------------------------------------------------------
//  LineDef Information Panel
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2013 Andrew Apted
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
#include "e_things.h"
#include "m_game.h"
#include "w_rawdef.h"
#include "w_rawdef.h"


#define MLF_ALL_AUTOMAP  \
	MLF_Secret | MLF_Mapped | MLF_DontDraw | MLF_Translucent


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
	Y += 5;

	W -= 12;
	H -= 10;


	which = new UI_Nombre(X, Y, W-10, 28, "Linedef");

	Y += which->h() + 4;


	type = new Fl_Int_Input(X+58, Y, 64, 24, "Type: ");
	type->align(FL_ALIGN_LEFT);
	type->callback(type_callback, this);
	type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	choose = new Fl_Button(X+W/2+24, Y, 80, 24, "Choose");
	choose->callback(button_callback, this);

	Y += type->h() + 2;


	desc = new Fl_Output(X+58, Y, W-66, 24, "Desc: ");
	desc->align(FL_ALIGN_LEFT);

	Y += desc->h() + 2;


	tag = new Fl_Int_Input(X+W/2+52, Y, 64, 24, "Tag: ");
	tag->align(FL_ALIGN_LEFT);
	tag->callback(tag_callback, this);
	tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);


	length = new Fl_Output(X+58, Y, 64, 24, "Length: ");
	length->align(FL_ALIGN_LEFT);


	for (int a = 0 ; a < 5 ; a++)
	{
		args[a] = new Fl_Int_Input(X+58+43*a, Y, 39, 24);
		args[a]->hide();
	}

	args[0]->label("Args:");


	Y += tag->h() + 2;

	Y += 3;


	Fl_Box *flags = new Fl_Box(FL_FLAT_BOX, X, Y, 64, 24, "Flags: ");
	flags->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);


	f_automap = new Fl_Choice(X+W-104, Y, 104, 22);
	f_automap->add("Normal|Invisible|Mapped|Secret");
	f_automap->value(0);
	f_automap->callback(flags_callback, new line_flag_CB_data_c(this, MLF_ALL_AUTOMAP));


	Y += flags->h() - 1;


	int FW = 110;

	f_upper = new Fl_Check_Button(X+12, Y+2, FW, 20, "upper unpeg");
	f_upper->labelsize(12);
	f_upper->callback(flags_callback, new line_flag_CB_data_c(this, MLF_UpperUnpegged));


	f_walk = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "block walk");
	f_walk->labelsize(12);
	f_walk->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Blocking));


	Y += 19;


	f_lower = new Fl_Check_Button(X+12, Y+2, FW, 20, "lower unpeg");
	f_lower->labelsize(12);
	f_lower->callback(flags_callback, new line_flag_CB_data_c(this, MLF_LowerUnpegged));


	f_mons = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "block mons");
	f_mons->labelsize(12);
	f_mons->callback(flags_callback, new line_flag_CB_data_c(this, MLF_BlockMonsters));


	Y += 19;


	f_passthru = new Fl_Check_Button(X+12, Y+2, FW, 20, "pass thru");
	f_passthru->labelsize(12);
	f_passthru->callback(flags_callback, new line_flag_CB_data_c(this, MLF_PassThru));


	f_sound = new Fl_Check_Button(X+W-120, Y+2, FW, 20, "block sound");
	f_sound->labelsize(12);
	f_sound->callback(flags_callback, new line_flag_CB_data_c(this, MLF_SoundBlock));


	Y += 36;


	front = new UI_SideBox(x(), Y, w(), 140, 0);

	Y += front->h() + 10;


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

	const linetype_t *info = M_GetLineType(new_type);
	box->desc->value(info->desc);

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
		MarkChanges();
	}
}


void UI_LineBox::SetTexOnLine(int ld, int new_tex, int e_state,
                              int front_pics, int back_pics)
{
	bool opposite = (e_state & FL_SHIFT);

	LineDef *L = LineDefs[ld];

	// handle the selected texture boxes
	if (front_pics || back_pics)
	{
		if (L->Right())
		{
			if (front_pics & 1) BA_ChangeSD(L->right, SideDef::F_LOWER_TEX, new_tex);
			if (front_pics & 2) BA_ChangeSD(L->right, SideDef::F_MID_TEX,   new_tex);
			if (front_pics & 4) BA_ChangeSD(L->right, SideDef::F_UPPER_TEX, new_tex);
		}

		if (L->Left())
		{
			if (back_pics & 1) BA_ChangeSD(L->left, SideDef::F_LOWER_TEX, new_tex);
			if (back_pics & 2) BA_ChangeSD(L->left, SideDef::F_MID_TEX,   new_tex);
			if (back_pics & 4) BA_ChangeSD(L->left, SideDef::F_UPPER_TEX, new_tex);
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
		MarkChanges();
	}

	UpdateField();
	UpdateSides();

	redraw();
}


void UI_LineBox::SetLineType(int new_type)
{
	if (obj < 0)
		return;

	char buffer[64];

	sprintf(buffer, "%d", new_type);

	type->value(buffer);
	type->do_callback();
}


void UI_LineBox::tag_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	int new_tag = atoi(box->tag->value());

	selection_c list;
	selection_iterator_c it;

	if (GetCurrentObjects(&list))
	{
		BA_Begin();

		for (list.begin(&it); !it.at_end(); ++it)
			BA_ChangeLD(*it, LineDef::F_TAG, new_tag);

		BA_End();
		MarkChanges();
	}
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
		MarkChanges();
	}
}


void UI_LineBox::button_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	if (w == box->choose)
	{
		main_win->ShowBrowser('L');
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
	if (field < 0 || field == LineDef::F_START || field == LineDef::F_END ||
	    field == LineDef::F_TAG)
	{
		if (is_linedef(obj))
		{
			CalcLength();
			tag->value(Int_TmpStr(LineDefs[obj]->tag));
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
			front->SetObj(LineDefs[obj]->right);
			 back->SetObj(LineDefs[obj]->left);
		}
		else
		{
			front->SetObj(SETOBJ_NO_LINE);
			 back->SetObj(SETOBJ_NO_LINE);
		}
	}

	if (field < 0 || field == LineDef::F_TYPE)
	{
		if (is_linedef(obj))
		{
			const linetype_t *info = M_GetLineType(LineDefs[obj]->type);
			desc->value(info->desc);
			type->value(Int_TmpStr(LineDefs[obj]->type));
		}
		else
		{
			type->value("");
			desc->value("");
		}
	}

	if (field < 0 || field == LineDef::F_FLAGS)
	{
		if (is_linedef(obj))
		{
			FlagsFromInt(LineDefs[obj]->flags);
		}
		else
		{
			FlagsFromInt(0);
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
///--	if (lineflags & MLF_Translucent)
///--		f_automap->value(4);
///--	else
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
	f_passthru->value((lineflags & MLF_PassThru)      ? 1 : 0);

	f_walk ->value((lineflags & MLF_Blocking)      ? 1 : 0);
	f_mons ->value((lineflags & MLF_BlockMonsters) ? 1 : 0);
	f_sound->value((lineflags & MLF_SoundBlock)    ? 1 : 0);
}


int UI_LineBox::CalcFlags() const
{
	// ASSERT(is_linedef(obj))

	int lineflags = 0;

	switch (f_automap->value())
	{
		case 0: /* Normal    */; break;
		case 1: /* Invisible */ lineflags |= MLF_DontDraw; break;
		case 2: /* Mapped    */ lineflags |= MLF_Mapped; break;
		case 3: /* Secret    */ lineflags |= MLF_Secret; break;
///--		case 4: /* Transluce */ lineflags |= MLF_Translucent; break;
	}

	if (f_upper->value())    lineflags |= MLF_UpperUnpegged;
	if (f_lower->value())    lineflags |= MLF_LowerUnpegged;
	if (f_passthru->value()) lineflags |= MLF_PassThru;

	if (f_walk->value())  lineflags |= MLF_Blocking;
	if (f_mons->value())  lineflags |= MLF_BlockMonsters;
	if (f_sound->value()) lineflags |= MLF_SoundBlock;

	return lineflags;
}


void UI_LineBox::UpdateTotal()
{
	which->SetTotal(NumLineDefs);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
