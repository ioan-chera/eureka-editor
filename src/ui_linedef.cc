//------------------------------------------------------------------------
//  LINEDEF PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025      Ioan Chera
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
#include "ui_stackpanel.h"

#include "FL/Fl_Grid.H"
#include "FL/Fl_Hor_Value_Slider.H"
#include "FL/Fl_Pack.H"

#include <algorithm>


#define MLF_ALL_AUTOMAP  \
	MLF_Secret | MLF_Mapped | MLF_DontDraw | MLF_XDoom_Translucent

static constexpr const char *kHardcodedAutoMapMenu = "Normal|Invisible|Mapped|Secret|MapSecret|InvSecret";
static constexpr int kHardcodedAutoMapCount = 6;

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
	ARG_WIDTH = 53,
	ARG_PADDING = 4,
	ARG_LABELSIZE = 10,

	FIELD_HEIGHT = 22,
};





class IDTag : public Fl_Widget
{
public:
	// Layout constants
	static constexpr int RADIUS = 4;
	static constexpr int PADDING = 6;
	static constexpr int X_BUTTON_SIZE = 14;
	static constexpr int X_BUTTON_MARGIN = 4;
	static constexpr int X_OFFSET = 3;
	static constexpr int MIN_WIDTH = 40;
	
	IDTag(int X, int Y, int W, int H, const char *label = nullptr)
		: Fl_Widget(X, Y, W, H, nullptr)
		, mCallback(nullptr)
		, mCallbackData(nullptr)
	{
		if (label)
			copy_label(label);
		
		box(FL_NO_BOX);
		align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
		labelsize(12);
	}
	
	void callback(Fl_Callback *cb, void *data = nullptr)
	{
		mCallback = cb;
		mCallbackData = data;
	}
	
	// Calculate the required width for a given label
	static int calcRequiredWidth(const char *labelText, int labelSize = 12)
	{
		if (!labelText || !*labelText)
			return 0;
		
		// Measure text width
		fl_font(FL_HELVETICA, labelSize);
		int textWidth = static_cast<int>(fl_width(labelText));
		
		// Total width = left padding + text + right padding + X button + margin
		int totalWidth = PADDING + textWidth + PADDING + X_BUTTON_SIZE + X_BUTTON_MARGIN;
		
		return totalWidth > MIN_WIDTH ? totalWidth : MIN_WIDTH;
	}
	
	int handle(int event) override
	{
		switch (event)
		{
			case FL_PUSH:
			{
				int mx = Fl::event_x();
				int my = Fl::event_y();
				
				// Check if click is within the X button area
				if (isInXButton(mx, my))
				{
					mXPressed = true;
					redraw();
					return 1;
				}
				return 1;
			}
			
			case FL_RELEASE:
			{
				if (mXPressed)
				{
					int mx = Fl::event_x();
					int my = Fl::event_y();
					
					// Trigger callback if released within X button
					if (isInXButton(mx, my) && mCallback)
					{
						mCallback(this, mCallbackData);
					}
					
					mXPressed = false;
					redraw();
					return 1;
				}
				return 1;
			}
			
			case FL_ENTER:
				redraw();
				return 1;
			
			case FL_LEAVE:
				mXPressed = false;
				redraw();
				return 1;
			
			case FL_MOVE:
			{
				int mx = Fl::event_x();
				int my = Fl::event_y();
				bool wasInX = mXHovered;
				mXHovered = isInXButton(mx, my);
				if (wasInX != mXHovered)
					redraw();
				return 1;
			}
		}
		
		return Fl_Widget::handle(event);
	}
	
	void draw() override
	{
		// Background color
		Fl_Color bgColor = FL_BACKGROUND_COLOR;
		fl_color(bgColor);
		
		// Draw rounded rectangle
		fl_push_clip(x(), y(), w(), h());
//		drawRoundedRect(x(), y(), w(), h(), RADIUS, bgColor);
		
		// Draw border
		fl_color(fl_darker(FL_BACKGROUND_COLOR));
		drawRoundedRectOutline(x(), y(), w(), h(), RADIUS);
		
		// Draw label text
		if (label())
		{
			fl_color(labelcolor());
			fl_font(labelfont(), labelsize());
			
			int labelW = w() - PADDING * 2 - X_BUTTON_SIZE - X_BUTTON_MARGIN;
			fl_draw(label(), x() + PADDING, y(), labelW, h(), 
				FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
		}
		
		// Draw X button
		int xBtnX = x() + w() - X_BUTTON_SIZE - X_BUTTON_MARGIN;
		int xBtnY = y() + (h() - X_BUTTON_SIZE) / 2;
		
		// X button background when hovered
		if (mXHovered)
		{
			fl_color(mXPressed ? fl_darker(FL_BACKGROUND_COLOR) : fl_lighter(bgColor));
			fl_pie(xBtnX, xBtnY, X_BUTTON_SIZE, X_BUTTON_SIZE, 0, 360);
		}
		
		// Draw X symbol
		fl_color(mXHovered ? FL_FOREGROUND_COLOR : fl_darker(FL_BACKGROUND_COLOR));
		fl_line_style(FL_SOLID, 2);
		
		fl_line(xBtnX + X_OFFSET, xBtnY + X_OFFSET, 
			xBtnX + X_BUTTON_SIZE - X_OFFSET, xBtnY + X_BUTTON_SIZE - X_OFFSET);
		fl_line(xBtnX + X_BUTTON_SIZE - X_OFFSET, xBtnY + X_OFFSET, 
			xBtnX + X_OFFSET, xBtnY + X_BUTTON_SIZE - X_OFFSET);
		
		fl_line_style(0);
		fl_pop_clip();
	}

	

private:
	Fl_Callback *mCallback;
	void *mCallbackData;
	bool mXPressed = false;
	bool mXHovered = false;
	
	bool isInXButton(int mx, int my) const
	{
		int xBtnX = x() + w() - X_BUTTON_SIZE - X_BUTTON_MARGIN;
		int xBtnY = y() + (h() - X_BUTTON_SIZE) / 2;
		
		return mx >= xBtnX && mx <= xBtnX + X_BUTTON_SIZE &&
			   my >= xBtnY && my <= xBtnY + X_BUTTON_SIZE;
	}
	
//	void drawRoundedRect(int X, int Y, int W, int H, int radius, Fl_Color color)
//	{
//		fl_color(color);
//		
//		// Main rectangles
//		fl_rectf(X + radius, Y, W - 2 * radius, H);
//		fl_rectf(X, Y + radius, radius, H - 2 * radius);
//		fl_rectf(X + W - radius, Y + radius, radius, H - 2 * radius);
//		
//		// Corners
//		fl_pie(X, Y, radius * 2, radius * 2, 90, 180);
//		fl_pie(X + W - radius * 2, Y, radius * 2, radius * 2, 0, 90);
//		fl_pie(X, Y + H - radius * 2, radius * 2, radius * 2, 180, 270);
//		fl_pie(X + W - radius * 2, Y + H - radius * 2, radius * 2, radius * 2, 270, 360);
//	}
	
	void drawRoundedRectOutline(int X, int Y, int W, int H, int radius)
	{
		fl_line_style(FL_SOLID, 1);
		
		// Top and bottom lines
		fl_line(X + radius, Y, X + W - radius, Y);
		fl_line(X + radius, Y + H - 1, X + W - radius, Y + H - 1);
		
		// Left and right lines
		fl_line(X, Y + radius, X, Y + H - radius);
		fl_line(X + W - 1, Y + radius, X + W - 1, Y + H - radius);
		
		// Corner arcs
		fl_arc(X, Y, radius * 2, radius * 2, 90, 180);
		fl_arc(X + W - radius * 2, Y, radius * 2, radius * 2, 0, 90);
		fl_arc(X, Y + H - radius * 2, radius * 2, radius * 2, 180, 270);
		fl_arc(X + W - radius * 2, Y + H - radius * 2, radius * 2, radius * 2, 270, 360);
		
		fl_line_style(0);
	}
};


class MultiTagView : public Fl_Group
{
public:
	MultiTagView(Instance &inst, const std::function<void()> &redrawCallback, int x, int y,
				 int width, int height, const char *label = nullptr);
private:
	static void addCallback(Fl_Widget *widget, void *data);
	static void tagCallback(Fl_Widget *widget, void *data);

	void updateTagButtons();
	void calcNextTagPosition(int tagWidth, int position, int *tagX, int *tagY, int *thisH) const;

	Instance &inst;
	const std::function<void()> mRedrawCallback;

	Fl_Input *mInput;
	Fl_Button *mAdd;
	std::set<SString, StringAlphanumericCompare> mTags;
	std::vector<IDTag *> mTagButtons;
};


MultiTagView::MultiTagView(Instance &inst, const std::function<void()> &redrawCallback, int x, int y,
						   int width, int height, const char *label) :
	Fl_Group(x, y, width, height, label), inst(inst), mRedrawCallback(redrawCallback)
{
	static const char inputLabel[] = "More tags:";
	mInput = new Fl_Input(x + fl_width(inputLabel), y, 50, TYPE_INPUT_HEIGHT, inputLabel);
	mInput->align(FL_ALIGN_LEFT);
	mInput->callback(addCallback, this);
	mInput->when(FL_WHEN_ENTER_KEY);

	mAdd = new Fl_Button(mInput->x() + mInput->w() + INPUT_SPACING, y, 40, TYPE_INPUT_HEIGHT,
						 "Add");
	mAdd->callback(addCallback, this);
	resizable(nullptr);
	end();
}


void MultiTagView::addCallback(Fl_Widget *widget, void *data)
{
	auto self = static_cast<MultiTagView *>(data);
	SString value = self->mInput->value();
	value.trimLeadingSpaces();
	value.trimTrailingSpaces();
	if(value.empty())
	{
		self->inst.Beep("Cannot add empty tag");
		return;
	}
	if(self->mTags.count(value))
	{
		self->inst.Beep("Tag %s already added", value.c_str());
		return;
	}

	self->mInput->value("");
	self->mTags.insert(value);

	self->updateTagButtons();
}


void MultiTagView::tagCallback(Fl_Widget *widget, void *data)
{
	auto tagButton = static_cast<const IDTag *>(widget);
	auto self = static_cast<MultiTagView *>(data);
	SString value = tagButton->label();

	self->mTags.erase(value);
	Fl::delete_widget(widget);
	for(auto it = self->mTagButtons.begin(); it != self->mTagButtons.end(); ++it)
		if(*it == tagButton)
		{
			self->mTagButtons.erase(it);
			break;
		}

	self->updateTagButtons();
}


void MultiTagView::updateTagButtons()
{
	for(IDTag *tagButton : mTagButtons)
		remove(tagButton);
	mTagButtons.clear();
	mTagButtons.reserve(mTags.size());

	int thisH = TYPE_INPUT_HEIGHT;
	begin();
	for(const SString &tag : mTags)
	{
		int tagX, tagY;
		int tagWidth = IDTag::calcRequiredWidth(tag.c_str());
		calcNextTagPosition(tagWidth, -1, &tagX, &tagY, &thisH);
		IDTag *tagButton = new IDTag(tagX, tagY, tagWidth, TYPE_INPUT_HEIGHT, tag.c_str());
		tagButton->callback(tagCallback, this);
		mTagButtons.push_back(tagButton);
	}
	end();

	h(thisH);
	redraw();
	if(mRedrawCallback)
		mRedrawCallback();
}


void MultiTagView::calcNextTagPosition(int tagWidth, int position, int *tagX, int *tagY,
									   int *thisH) const
{
	const IDTag *lastTag = position == -1 ? mTagButtons.empty() ? nullptr : mTagButtons.back() :
			position >= 1 ? mTagButtons[position - 1] : nullptr;
	if(!lastTag)
	{
		*tagX = mAdd->x() + mAdd->w() + INPUT_SPACING;
		*tagY = y();
		*thisH = TYPE_INPUT_HEIGHT;
		if(*tagX + tagWidth > x() + w())
		{
			*tagX = x();
			*tagY = mInput->y() + mInput->h() + INPUT_SPACING;
			*thisH = 2 * TYPE_INPUT_HEIGHT + INPUT_SPACING;
		}
	}
	else
	{
		*tagX = lastTag->x() + lastTag->w() + INPUT_SPACING;
		*tagY = lastTag->y();
		*thisH = lastTag->y() + lastTag->h() - mInput->y();
		if(*tagX + tagWidth > x() + w())
		{
			*tagX = x();
			*tagY = lastTag->y() + lastTag->h() + INPUT_SPACING;
			*thisH = *tagY + TYPE_INPUT_HEIGHT - mInput->y();
		}
	}
}


class line_flag_CB_data_c
{
public:
	UI_LineBox *parent;

	int mask;
	int mask2;	// mask for flags2 set (0 if only flags set is used)

	line_flag_CB_data_c(UI_LineBox *_parent, int _mask, int _mask2 = 0) :
		parent(_parent), mask(_mask), mask2(_mask2)
	{ }
};


//
// UI_LineBox Constructor
//
UI_LineBox::UI_LineBox(Instance &inst, int X, int Y, int W, int H, const char *label) :
    MapItemBox(inst, X, Y, W, H, label)
{
	//Fl_Scroll *scroll = new Fl_Scroll(X, Y, W, H);
	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);

	const int Y0 = Y;
	const int X0 = X;

	panel = new UI_StackPanel(X, Y, W, H);
	panel->margin(INSET_LEFT, INSET_TOP, INSET_RIGHT, INSET_BOTTOM);
	panel->spacing(INPUT_SPACING);

	// Put this spacer here so the scroller places the UI elements correctly when content collapses.
	//Fl_Box *spacer = new Fl_Box(X, Y, W, INSET_TOP);
	//spacer->box(FL_FLAT_BOX);

	X += INSET_LEFT;
	Y += INSET_TOP;

	W -= INSET_LEFT + INSET_RIGHT;
	H -= INSET_TOP + INSET_BOTTOM;

	which = new UI_Nombre(X + NOMBRE_INSET, Y, W - 2 * NOMBRE_INSET, NOMBRE_HEIGHT, "Linedef");

	Y += which->h() + SPACING_BELOW_NOMBRE;

	{
		Fl_Pack *type_pack = new Fl_Pack(X + TYPE_INPUT_X, Y, W - TYPE_INPUT_X, TYPE_INPUT_HEIGHT);
		type_pack->spacing(BUTTON_SPACING);
		type_pack->type(Fl_Pack::HORIZONTAL);

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

		type_pack->end();

		panel->extraSpacing(type_pack, SPACING_BELOW_NOMBRE - INPUT_SPACING);
	}

	Y += type->h() + INPUT_SPACING;
	{
		Fl_Group *desc_pack = new Fl_Group(X, Y, W, TYPE_INPUT_HEIGHT);
		descBox = new Fl_Box(FL_NO_BOX, X + DESC_INSET, Y, DESC_WIDTH, TYPE_INPUT_HEIGHT, "Desc: ");

		desc = new Fl_Output(type->x(), Y, W - type->x(), TYPE_INPUT_HEIGHT);
		desc->align(FL_ALIGN_LEFT);

		actkind = new Fl_Choice(type->x(), Y, SPAC_WIDTH, TYPE_INPUT_HEIGHT);
		// this order must match the SPAC_XXX constants
		actkind->add(getActivationMenuString());
		actkind->value(getActivationCount());
		actkind->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Activation | MLF_Repeatable));
		actkind->deactivate();
		actkind->hide();

		desc_pack->end();
	}

	Y += desc->h() + INPUT_SPACING;

	{
		Fl_Group *tag_pack = new Fl_Group(X, Y, W, TYPE_INPUT_HEIGHT);
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
			args[a] = new UI_DynIntInput(X + 7 + (ARG_WIDTH + ARG_PADDING) * a, Y, ARG_WIDTH, TYPE_INPUT_HEIGHT);
			args[a]->callback(args_callback, new line_flag_CB_data_c(this, a));
			args[a]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
			args[a]->hide();
			args[a]->align(FL_ALIGN_BOTTOM);
			args[a]->labelsize(ARG_LABELSIZE);
		}
		args0str = new UI_DynInput(args[0]->x(), Y, ARG_WIDTH, TYPE_INPUT_HEIGHT);
		args0str->callback(args_callback, new line_flag_CB_data_c(this, 5));
		args0str->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		args0str->hide();
		args0str->align(FL_ALIGN_BOTTOM);
		args0str->labelsize(ARG_LABELSIZE);

		tag_pack->end();
	}

	MultiTagView *multitag = new MultiTagView(inst, [this](){
		redraw();
	}, X, Y, W, TYPE_INPUT_HEIGHT);

	Y += tag->h() + 16;

	{
		Fl_Group *flags_pack = new Fl_Group(X, Y, W, 22);
		Fl_Box *flags = new Fl_Box(FL_FLAT_BOX, X+10, Y, 64, 24, "Flags: ");
		flags->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);


		f_automap = new Fl_Choice(X+W-118, Y, 104, FIELD_HEIGHT, "Vis: ");
		f_automap->add(kHardcodedAutoMapMenu);
		f_automap->value(0);
		f_automap_cb_data = std::make_unique<line_flag_CB_data_c>(this, MLF_ALL_AUTOMAP);
		f_automap->callback(flags_callback, f_automap_cb_data.get());

		flags_pack->end();

		panel->extraSpacing(flags_pack, 16 - INPUT_SPACING);
	}


	Y += f_automap->h() - 1;

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

	mFixUp.loadFields({type, length, tag, args[0], args[1], args[2], args[3], args[4], args0str});

	//scroll->end();
	panel->end();
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
	box->UpdateField(Basis::EditField(LineDef::F_TYPE));
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

	int mask = l_f_c->mask;
	int mask2 = l_f_c->mask2;
	int new_flags, new_flags2;
	box->CalcFlags(new_flags, new_flags2);

	bool changed = false;
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
			if(mask != 0)
			{
				op.changeLinedef(*it, LineDef::F_FLAGS, (L->flags & ~mask) | (new_flags & mask));
				changed = true;
			}
			if(mask2 != 0)
			{
				op.changeLinedef(*it, LineDef::F_FLAGS2, (L->flags2 & ~mask2) | (new_flags2 & mask2));
				changed = true;
			}
		}
	}
	if(changed)
	{
		box->updateCategoryDetails();
		box->redraw();
	}
}


void UI_LineBox::args_callback(Fl_Widget *w, void *data)
{
	line_flag_CB_data_c *l_f_c = (line_flag_CB_data_c *)data;

	UI_LineBox *box = l_f_c->parent;

	int arg_idx = l_f_c->mask;
	int new_value;
	if(arg_idx == 5)
		new_value = BA_InternaliseString(box->args0str->value()).get();
	else
	{
		new_value = atoi(box->args[arg_idx]->value());
		if(box->inst.loaded.levelFormat != MapFormat::udmf)
			new_value = clamp(0, new_value, 255);
	}

	if (! box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited args of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			int paramIndex = arg_idx == 5 ? LineDef::F_ARG1STR : LineDef::F_ARG1 + arg_idx;
			op.changeLinedef(*it, static_cast<byte>(paramIndex), new_value);

			// Also change the tag when outside of UDMF
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

void UI_LineBox::field_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	if(!box->inst.edit.Selected->empty())
	{
		// Find which choice widget this is
		const linefield_t *info = nullptr;
		for(const LineField &field : box->fields)
		{
			if(field.widget.get() == w)
			{
				info = field.info;
				break;
			}
		}

		if(!info)
			return;

		SString message = SString("edited ") + info->identifier + " of";

		switch(info->type)
		{
		case linefield_t::Type::choice:
		{
			auto choice = static_cast<Fl_Choice *>(w);
			int index = choice->value();
			if(index < 0 || index >= static_cast<int>(info->options.size()))
				return;

			int new_value = info->options[index].value;

			// Apply to all selected linedefs
			EditOperation op(box->inst.level.basis);
			op.setMessageForSelection(message.c_str(), *box->inst.edit.Selected);

			for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			{
				// Hardcode the field mapping for now
				if(info->identifier.noCaseEqual("locknumber"))
				{
					op.changeLinedef(*it, LineDef::F_LOCKNUMBER, new_value);
					// Update the display
					box->UpdateField(Basis::EditField(LineDef::F_LOCKNUMBER));
				}
			}

			break;
		}
		case linefield_t::Type::slider:
		{
			auto valuator = static_cast<Fl_Valuator *>(w);

			EditOperation op(box->inst.level.basis);
			op.setMessageForSelection(message.c_str(), *box->inst.edit.Selected);

			for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			{
				// Hardcode the field mapping for now
				if(info->identifier.noCaseEqual("alpha"))
				{
					op.changeLinedef(*it, &LineDef::alpha, valuator->value());
					box->UpdateField(Basis::EditField(&LineDef::alpha));
				}
			}
			break;
		}
		default:
			break;
		}

	}


}

void UI_LineBox::categoryToggled(UI_CategoryButton *categoryBtn)
{
	// Find which category was toggled
	for(auto &cat : categoryHeaders)
	{
		if(!cat.button)
			continue;
		if(!categoryBtn || cat.button.get() == categoryBtn)
		{
			cat.expanded = cat.button->isExpanded();
			if(cat.expanded)
				cat.grid->show();
			else
				cat.grid->hide();

			// Reposition all controls below the flags area
			//repositionAfterCategoryToggle();
			redraw();
			break;
		}
	}
}
//------------------------------------------------------------------------

void UI_LineBox::updateCategoryDetails()
{
	for(const CategoryHeader& header : categoryHeaders)
	{
		if(!header.button)
			continue;
		SString summaryText;
		for(int flagButtonIndex : header.lineFlagButtonIndices)
		{
			const LineFlagButton &flagBtn = flagButtons[flagButtonIndex];
			if(flagBtn.button->value())
			{
				summaryText += flagBtn.info->inCategoryAcronym;
			}
		}
		header.button->details(summaryText);
	}
}

void UI_LineBox::UpdateField(std::optional<Basis::EditField> efield)
{
	if (!efield || efield->isRaw(LineDef::F_START) || efield->isRaw(LineDef::F_END))
	{
		if(inst.level.isLinedef(obj))
			CalcLength();
		else
			mFixUp.setInputValue(length, "");
	}

	if (!efield || efield->isRaw(LineDef::F_TAG) || efield->isRaw(LineDef::F_ARG1STR))
	{
		for (int a = 0 ; a < 5 ; a++)
		{
			mFixUp.setInputValue(args[a], "");
			if (inst.loaded.levelFormat == MapFormat::hexen ||
				(inst.loaded.levelFormat == MapFormat::udmf && inst.conf.features.udmf_lineparameters))
			{
				args[a]->label("");
			}
			args[a]->textcolor(FL_BLACK);
			args0str->textcolor(FL_BLACK);
		}

		if (inst.level.isLinedef(obj))
		{
			const auto L = inst.level.linedefs[obj];

			mFixUp.setInputValue(tag, SString(inst.level.linedefs[obj]->tag).c_str());

			const linetype_t &info = inst.conf.getLineType(L->type);

			if (inst.loaded.levelFormat == MapFormat::hexen ||
				(inst.loaded.levelFormat == MapFormat::udmf && inst.conf.features.udmf_lineparameters))
			{
				for (int a = 0 ; a < 5 ; a++)
				{
					int arg_val = L->Arg(1 + a);

					if(arg_val || L->type)
						mFixUp.setInputValue(args[a], SString(arg_val).c_str());

					if(L->arg1str.get())
						mFixUp.setInputValue(args0str, BA_GetString(L->arg1str).c_str());

					// set the tooltip
					if (!info.args[a].name.empty())
					{
						args[a]->copy_label(info.args[a].name.replacing('_', ' ').c_str());
						args[a]->parent()->redraw();
						if(a == 0)
						{
							if(info.args[0].type == SpecialArgType::str)
							{
								args0str->copy_label(args[0]->label());
								args0str->show();
								args[0]->hide();
							}
							else
							{
								args0str->hide();
								args[0]->show();
							}
						}
					}
					else
					{
						args[a]->label("");
						args[a]->textcolor(fl_rgb_color(160,160,160));
						args[a]->parent()->redraw();
						if(a == 0)
						{
							args0str->hide();
							args[0]->show();
						}
					}
				}
			}
		}
		else
		{
			mFixUp.setInputValue(length, "");
			mFixUp.setInputValue(tag, "");
		}
	}

	if (!efield || efield->isRaw(LineDef::F_RIGHT) || efield->isRaw(LineDef::F_LEFT))
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

	if (!efield || efield->isRaw(LineDef::F_TYPE))
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

	bool changed = false;
	if (!efield || efield->isRaw(LineDef::F_FLAGS))
	{
		if (inst.level.isLinedef(obj))
		{
			actkind->activate();

			changed |= FlagsFromInt(inst.level.linedefs[obj]->flags);
		}
		else
		{
			changed |= FlagsFromInt(0);

			actkind->value(getActivationCount());  // show as "??"
			actkind->deactivate();
		}
	}

	if(!efield || efield->isRaw(LineDef::F_FLAGS2))
	{
		if(inst.level.isLinedef(obj))
			changed |= Flags2FromInt(inst.level.linedefs[obj]->flags2);
		else
			changed |= Flags2FromInt(0);
	}

	if(changed)
		updateCategoryDetails();


	if(!efield || efield->isRaw(LineDef::F_LOCKNUMBER))
	{
		// Update choice widgets for UDMF properties
		if(inst.level.isLinedef(obj))
		{
			const auto L = inst.level.linedefs[obj];

			for(const LineField &field : fields)
			{
				if(field.info->type != linefield_t::Type::choice)
					continue;
				if(field.info->identifier.noCaseEqual("locknumber"))
				{
					int value = L->locknumber;

					// Find matching option
					int index = 0;
					for(size_t i = 0; i < field.info->options.size(); ++i)
					{
						if(field.info->options[i].value == value)
						{
							index = static_cast<int>(i);
							break;
						}
					}
					static_cast<Fl_Choice*>(field.widget.get())->value(index);
				}
			}
		}
		else
		{
			// No linedef selected, reset to first option
			for(const LineField &field : fields)
			{
				if(field.info->type == linefield_t::Type::choice)
				{
					static_cast<Fl_Choice*>(field.widget.get())->value(0);
				}
			}
		}
	}

	if(!efield || (efield->format == Basis::EditFormat::linedefDouble && efield->doubleLineField == &LineDef::alpha))
	{
		if(inst.level.isLinedef(obj))
		{
			for(const LineField &field : fields)
			{
				if(field.info->type != linefield_t::Type::slider)
					continue;
				if(field.info->identifier.noCaseEqual("alpha"))
				{
					double alpha = inst.level.linedefs[obj]->alpha;
					static_cast<Fl_Valuator*>(field.widget.get())->value(alpha);
				}
			}
		}
		else
		{
			for(const LineField &field : fields)
			{
				if(field.info->type == linefield_t::Type::slider)
				{
					static_cast<Fl_Valuator*>(field.widget.get())->value(0);
				}
			}
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

	float len_f = static_cast<float>(inst.level.calcLength(*inst.level.linedefs[n]));

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "%1.0f", len_f);

	mFixUp.setInputValue(length, buffer);
}


bool UI_LineBox::FlagsFromInt(int lineflags)
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

	// Check UDMF visibility flags first if in UDMF mode
	bool foundUDMFMatch = false;
	if(inst.level.isLinedef(obj) && inst.loaded.levelFormat == MapFormat::udmf && !inst.conf.udmf_line_vis_flags.empty())
	{
		const auto L = inst.level.linedefs[obj];

		// Build list of matching flags with their indices
		std::vector<std::pair<size_t, const linevisflag_t*>> matches;
		for(size_t i = 0; i < inst.conf.udmf_line_vis_flags.size(); i++)
		{
			const linevisflag_t &vf = inst.conf.udmf_line_vis_flags[i];
			// Check if all required flags match exactly
			if((L->flags & vf.flags) == vf.flags && (L->flags2 & vf.flags2) == vf.flags2)
				matches.push_back({i, &vf});
		}

		// Sort matches so more specific combinations come first
		if(!matches.empty())
		{
			std::sort(matches.begin(), matches.end(),
				[](const std::pair<size_t, const linevisflag_t*> &a, const std::pair<size_t, const linevisflag_t*> &b) {
					// b is subset of a if all of b's flags are in a
					bool bSubsetOfA = ((a.second->flags & b.second->flags) == b.second->flags) &&
									  ((a.second->flags2 & b.second->flags2) == b.second->flags2);
					// a is subset of b if all of a's flags are in b
					bool aSubsetOfB = ((b.second->flags & a.second->flags) == a.second->flags) &&
									  ((b.second->flags2 & a.second->flags2) == a.second->flags2);

					// If b is subset of a but a is not subset of b, then a comes first
					if(bSubsetOfA && !aSubsetOfB)
						return true;
					return false;
				});

			f_automap->value(kHardcodedAutoMapCount + (int)matches[0].first);
			foundUDMFMatch = true;
		}
	}

	// Fall back to hardcoded values if no UDMF match
	if(!foundUDMFMatch)
	{
		if(lineflags & MLF_DontDraw && lineflags & MLF_Secret)
			f_automap->value(5);
		else if (lineflags & MLF_DontDraw)
			f_automap->value(1);
		else if(lineflags & MLF_Mapped && lineflags & MLF_Secret)
			f_automap->value(4);
		else if (lineflags & MLF_Secret)
			f_automap->value(3);
		else if (lineflags & MLF_Mapped)
			f_automap->value(2);
		else
			f_automap->value(0);
	}

	// Set dynamic line flag buttons
	bool changed = false;
	for(const auto &btn : flagButtons)
	{
		if(btn.button && btn.info->flagSet == 1)
		{
			int newValue = (lineflags & btn.info->value) ? 1 : 0;
			changed = changed || (btn.button->value() != newValue);
			btn.button->value(newValue);
		}
	}
	return changed;
}

bool UI_LineBox::Flags2FromInt(int lineflags)
{
	bool changed = false;
	for(const auto &btn : flagButtons)
	{
		if(btn.button && btn.info->flagSet == 2)
		{
			int newValue = (lineflags & btn.info->value) ? 1 : 0;
			changed = changed || (btn.button->value() != newValue);
			btn.button->value(newValue);
		}
	}
	return changed;
}

void UI_LineBox::CalcFlags(int &outFlags, int &outFlags2) const
{
	outFlags = 0;
	outFlags2 = 0;

	int automapVal = f_automap->value();
	if(automapVal >= kHardcodedAutoMapCount && inst.loaded.levelFormat == MapFormat::udmf)
	{
		// UDMF visibility flag selected
		int udmfIndex = automapVal - kHardcodedAutoMapCount;
		if(udmfIndex < (int)inst.conf.udmf_line_vis_flags.size())
		{
			const linevisflag_t &vf = inst.conf.udmf_line_vis_flags[udmfIndex];
			outFlags |= vf.flags;
			outFlags2 |= vf.flags2;
		}
	}
	else
	{
		// Hardcoded visibility flags
		switch (automapVal)
		{
			case 0: /* Normal    */; break;
			case 1: /* Invisible */ outFlags |= MLF_DontDraw; break;
			case 2: /* Mapped    */ outFlags |= MLF_Mapped; break;
			case 3: /* Secret    */ outFlags |= MLF_Secret; break;
			case 4: /* MapSecret */ outFlags |= MLF_Mapped | MLF_Secret; break;
			case 5: /* InvSecret */ outFlags |= MLF_DontDraw | MLF_Secret; break;
		}
	}

	// Activation for non-DOOM formats
	if (inst.loaded.levelFormat == MapFormat::hexen)
	{
		int actval = actkind->value();
		if (actval >= getActivationCount())
			actval = 0;
		outFlags |= (actval << 9);
	}

	// Apply dynamic flags
	for(const auto &btn : flagButtons)
	{
		if(btn.button && btn.button->value())
		{
			if(btn.info->flagSet == 1)
				outFlags |= btn.info->value;
			else if(btn.info->flagSet == 2)
				outFlags2 |= btn.info->value;
		}
	}
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
		tag->tooltip("Tag of the linedef");
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
		tag->tooltip("Tag of targeted sector(s)");
		descBox->show();

		length->show();

		actkind->hide();
		desc->resize(type->x(), desc->y(), w()-78, desc->h());

		if (config.features.gen_types)
			gen->show();
		else
			gen->hide();
	}

	// Populate f_automap with UDMF visibility flags if in UDMF mode
	int automapMask = MLF_ALL_AUTOMAP;
	int automapMask2 = 0;
	if(loaded.levelFormat == MapFormat::udmf && !config.udmf_line_vis_flags.empty())
	{
		// Build the menu string with hardcoded entries plus UDMF entries
		SString menuStr = kHardcodedAutoMapMenu;
		for(const linevisflag_t &vf : config.udmf_line_vis_flags)
		{
			menuStr += "|";
			menuStr += vf.label;
			automapMask |= vf.flags;
			automapMask2 |= vf.flags2;
		}
		f_automap->clear();
		f_automap->add(menuStr.c_str());
		f_automap->value(0);
	}
	else
	{
		// Reset to default hardcoded entries
		f_automap->clear();
		f_automap->add(kHardcodedAutoMapMenu);
		f_automap->value(0);
	}

	// Update callback data with new masks
	f_automap_cb_data = std::make_unique<line_flag_CB_data_c>(this, automapMask, automapMask2);
	f_automap->callback(flags_callback, f_automap_cb_data.get());

	// Recreate dynamic linedef flags from configuration
	//for(const auto &btn : flagButtons)
	//	this->remove(btn.button.get());
	for(const auto &cat : categoryHeaders)
	{
		if(cat.button)
			panel->remove(cat.button.get());
		panel->remove(cat.grid.get());
	}
	categoryHeaders.clear();
	flagButtons.clear();
	
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

		//begin();

		// Process each category
		for(auto &catPair : categorized)
		{
			const SString &catName = catPair.first;
			const std::vector<const lineflag_t *> &flagsInCat = catPair.second;

			CategoryHeader catHeader = {};
			if(!catName.empty())
			{
				catHeader.button = std::make_unique<UI_CategoryButton>(x() + flagsStartX, Y,
					flagsAreaW, FIELD_HEIGHT);
				catHeader.button->copy_label(catName.c_str());
				catHeader.button->callback(category_callback, this);

				catHeader.expanded = false;
				Y += FIELD_HEIGHT;
			}
			const int numRows = (int(flagsInCat.size()) + 1) / 2;
			catHeader.grid = std::make_unique<Fl_Grid>(leftX, 0, FW + rightX - leftX, rowH * numRows);
			catHeader.grid->layout(numRows, 2);

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

				auto addButton = [baseX, curY, this, &catHeader](int offset, const lineflag_t *flag)
					{
						LineFlagButton fb;
						fb.button = new Fl_Check_Button(baseX + offset, curY + 2, FW, 20,
							flag->label.c_str());
						fb.button->labelsize(12);
						fb.data = std::make_unique<line_flag_CB_data_c>(this, flag->flagSet == 1 ?
							flag->value : 0, flag->flagSet == 2 ? flag->value : 0);
						fb.button->callback(flags_callback, fb.data.get());
						fb.info = flag;
						if(catHeader.button)
							catHeader.lineFlagButtonIndices.push_back((int)flagButtons.size());
						flagButtons.push_back(std::move(fb));
					};

				if(s.a)
					addButton(0, s.a);
				if(s.b)
					addButton(16, s.b);

				if(s.b)
				{
					// If we have a B, we deal with a pair, so make a group to assign it into the grid
					Fl_Group *pairGroup = new Fl_Group(baseX, curY, FW, rowH);
					pairGroup->end();

					if(s.a)
						pairGroup->add(flagButtons[flagButtons.size() - 2].button);
					pairGroup->add(flagButtons.back().button);
					catHeader.grid->add(pairGroup);
					catHeader.grid->widget(pairGroup, idx % leftCount, onLeft ? 0 : 1);
				}
				else if(s.a)
				{
					Fl_Widget *widget = flagButtons.back().button;
					catHeader.grid->add(widget);
					catHeader.grid->widget(widget, idx % leftCount, onLeft ? 0 : 1);
				}
				curY += rowH;
			}

			Y = (yLeft > yRight ? yLeft : yRight);
			
			if(catHeader.button)
				panel->insert(*catHeader.button.get(), front);
			panel->insert(*catHeader.grid.get(), front);
			categoryHeaders.push_back(std::move(catHeader));
		}

		//end();

		Y += 29;
	}
	else
	{
		// keep some spacing if no flags defined
		Y += 29;
	}

	for(const LineField &field : fields)
		panel->remove(field.widget.get());
	fields.clear();

	if(loaded.levelFormat == MapFormat::udmf && !config.udmf_line_fields.empty())
	{
		// Match the args span: starts at type->x() and spans 5 args
		const int fieldX = x() + TYPE_INPUT_X;
		const int fieldW = desc->x() + desc->w() - x() - TYPE_INPUT_X;

		//begin();

		for(const linefield_t &lf : config.udmf_line_fields)
		{
			LineField field = {};
			if(lf.type == linefield_t::Type::choice)
			{
				if(lf.options.empty())
					continue;
				auto choice = new Fl_Choice(fieldX, Y, fieldW, FIELD_HEIGHT);
				field.widget = std::unique_ptr<Fl_Widget>(choice);
				choice->copy_label((lf.label + ": ").c_str());
				choice->align(FL_ALIGN_LEFT);
				//field.choice->labelsize(12);

				// Build menu string from options
				SString menuStr;
				for(size_t i = 0; i < lf.options.size(); ++i)
				{
					if(i > 0)
						menuStr += "|";
					menuStr += lf.options[i].label;
				}
				choice->add(menuStr.c_str());
				choice->value(0);
			}
			else if(lf.type == linefield_t::Type::slider)
			{
				auto slider = new Fl_Hor_Value_Slider(fieldX, Y, fieldW, FIELD_HEIGHT);
				field.widget = std::unique_ptr<Fl_Widget>(slider);
				slider->copy_label((lf.label + ": ").c_str());
				slider->align(FL_ALIGN_LEFT);
				//field.counter->labelsize(12);

				slider->step(lf.step);
				// slider->lstep(lf.step * 8);
				slider->minimum(lf.minValue);
				slider->maximum(lf.maxValue);
				slider->value_width(60);
			}

			// Pass the linefield_t pointer via callback data
			field.widget->callback(field_callback, this);
			field.info = &lf;

			fields.push_back(std::move(field));
			Y += FIELD_HEIGHT + 4;

			panel->insert(*fields.back().widget.get(), front);
		}

		//end();
		Y += 10;
	}

	// Reposition side boxes under the generated flags and choice widgets
	//front->Fl_Widget::position(front->x(), Y);
	//Y += front->h() + 14;
	//back->Fl_Widget::position(back->x(), Y);
	//front->redraw();
	//back->redraw();

	// Show Hexen/UDMF args when needed
	args0str->hide();
	for (int a = 0 ; a < 5 ; a++)
	{
		if (loaded.levelFormat == MapFormat::hexen ||
			(loaded.levelFormat == MapFormat::udmf && (config.features.udmf_lineparameters || !a)))
		{
			args[a]->show();
			if(loaded.levelFormat == MapFormat::hexen || config.features.udmf_lineparameters)
			{
				args[0]->resize(x() + INSET_LEFT + 7, args[0]->y(), ARG_WIDTH, TYPE_INPUT_HEIGHT);
				args[0]->label(nullptr);
				args[0]->labelsize(ARG_LABELSIZE);
				args[0]->tooltip(nullptr);
				args[0]->align(FL_ALIGN_BOTTOM);
			}
			else
			{
				args[0]->resize(type->x(), args[0]->y(), TAG_WIDTH, TYPE_INPUT_HEIGHT);
				args[0]->label("Target:");
				args[0]->labelsize(14);
				args[0]->tooltip("Tag of targeted sector(s)");
				args[0]->align(FL_ALIGN_LEFT);
			}
		}
		else
			args[a]->hide();
	}

	redraw();

	for(CategoryHeader& header : categoryHeaders)
	{
		if(header.button)
			header.button->setExpanded(false);
	}
	categoryToggled(nullptr);  // Make sure all shows correctly with collapsed categories
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
