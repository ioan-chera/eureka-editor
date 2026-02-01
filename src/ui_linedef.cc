//------------------------------------------------------------------------
//  LINEDEF PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025-2026 Ioan Chera
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

#include "Instance.h"

#include "ui_category_button.h"
#include "ui_misc.h"

#include "m_udmf.h"
#include "w_rawdef.h"

#include "FL/Fl_Flex.H"
#include "FL/Fl_Grid.H"
#include "FL/Fl_Hor_Value_Slider.H"

#define MLF_ALL_AUTOMAP  \
	MLF_Secret | MLF_Mapped | MLF_DontDraw | MLF_XDoom_Translucent

#define UDMF_ACTIVATION_BUTTON_LABEL "Trigger?"

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
	ALPHA_NUMBER_WIDTH = 60,
	ARG_PADDING = 4,
	ARG_LABELSIZE = 10,
	FLAG_LABELSIZE = 12,

	FIELD_HEIGHT = 22,

	FLAG_ROW_HEIGHT = 19,
};


struct FeatureFlagMapping
{
	UDMF_LineFeature feature;
	const char *label;
	const char *tooltip;
	unsigned value;
	int flagSet;
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
		labelsize(FLAG_LABELSIZE);
	}

	void callback(Fl_Callback *cb, void *data = nullptr)
	{
		mCallback = cb;
		mCallbackData = data;
	}

	// Calculate the required width for a given label
	static int calcRequiredWidth(const char *labelText, int labelSize = FLAG_LABELSIZE)
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
	MultiTagView(Instance &inst, const std::function<void()> &redrawCallback,
				 const std::function<void()> &dataCallback, int x, int y, int width, int height,
				 const char *label = nullptr);

	void setTags(std::set<int> &&tags);
	void clearTags();
	const std::set<int> &getTags() const
	{
		return mTags;
	}
private:
	static void addCallback(Fl_Widget *widget, void *data);
	static void tagRemoveCallback(Fl_Widget *widget, void *data);

	void updateTagButtons();
	void calcNextTagPosition(int tagWidth, int position, int *tagX, int *tagY, int *thisH) const;

	Instance &inst;
	const std::function<void()> mRedrawCallback;
	const std::function<void()> mDataCallback;

	Fl_Int_Input *mInput;
	Fl_Button *mAdd;
	std::set<int> mTags;
	std::vector<IDTag *> mTagButtons;
};


MultiTagView::MultiTagView(Instance &inst, const std::function<void()> &redrawCallback,
						   const std::function<void()> &dataCallback, int x, int y,
						   int width, int height, const char *label) :
	Fl_Group(x, y, width, height, label), inst(inst), mRedrawCallback(redrawCallback),
	mDataCallback(dataCallback)
{
	static const char inputLabel[] = "More IDs:";
	fl_font(FL_HELVETICA, FLAG_LABELSIZE);	// prepare
	mInput = new Fl_Int_Input(x + fl_width(inputLabel) + 8, y, 50, TYPE_INPUT_HEIGHT, inputLabel);
	mInput->align(FL_ALIGN_LEFT);
	mInput->callback(addCallback, this);
	mInput->when(FL_WHEN_ENTER_KEY);

	mAdd = new Fl_Button(mInput->x() + mInput->w() + INPUT_SPACING, y, 40, TYPE_INPUT_HEIGHT,
						 "Add");
	mAdd->callback(addCallback, this);
	resizable(nullptr);
	end();
}


void MultiTagView::setTags(std::set<int> &&tags)
{
	mTags = std::move(tags);
	updateTagButtons();
}


void MultiTagView::clearTags()
{
	mTags.clear();
	updateTagButtons();
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
	long valueNumber = strtol(value.c_str(), nullptr, 10);
	if(self->mTags.count((int)valueNumber))
	{
		self->inst.Beep("Tag %ld already added", valueNumber);
		return;
	}

	self->mInput->value("");
	self->mTags.insert((int)valueNumber);

	self->updateTagButtons();
	if(self->mDataCallback)
		self->mDataCallback();
}


void MultiTagView::tagRemoveCallback(Fl_Widget *widget, void *data)
{
	auto tagButton = static_cast<const IDTag *>(widget);
	auto self = static_cast<MultiTagView *>(data);
	SString value = tagButton->label();
	int valueNumber = atoi(value.c_str());

	self->mTags.erase(valueNumber);

	// Clear it from here in a deferred manner, because updateTagButtons will just delete.
	Fl::delete_widget(widget);
	for(auto it = self->mTagButtons.begin(); it != self->mTagButtons.end(); ++it)
		if(*it == tagButton)
		{
			self->mTagButtons.erase(it);
			break;
		}

	self->updateTagButtons();
	if(self->mDataCallback)
		self->mDataCallback();
}


void MultiTagView::updateTagButtons()
{
	for(IDTag *tagButton : mTagButtons)
		remove(tagButton);
	mTagButtons.clear();
	mTagButtons.reserve(mTags.size());

	int thisH = TYPE_INPUT_HEIGHT;
	begin();
	for(const int tag : mTags)
	{
		int tagX, tagY;
		std::string tagString = std::to_string(tag);
		int tagWidth = IDTag::calcRequiredWidth(tagString.c_str());
		calcNextTagPosition(tagWidth, -1, &tagX, &tagY, &thisH);
		IDTag *tagButton = new IDTag(tagX, tagY, tagWidth, TYPE_INPUT_HEIGHT, tagString.c_str());
		tagButton->callback(tagRemoveCallback, this);
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

	W -= INSET_LEFT + INSET_RIGHT;
	H -= INSET_TOP + INSET_BOTTOM;

	which = new UI_Nombre(X + NOMBRE_INSET, 0, W - 2 * NOMBRE_INSET, NOMBRE_HEIGHT, "Linedef");
	panel->afterSpacing(which, SPACING_BELOW_NOMBRE - INPUT_SPACING);

	// Prepare before calling fl_width
	fl_font(FL_HELVETICA, FLAG_LABELSIZE);

	{
		Fl_Flex *type_flex = new Fl_Flex(X + TYPE_INPUT_X, 0, W - TYPE_INPUT_X - NOMBRE_INSET,
										 TYPE_INPUT_HEIGHT, Fl_Flex::HORIZONTAL);
		type_flex->spacing(BUTTON_SPACING);
		type_flex->label("Type:");
		type_flex->align(FL_ALIGN_LEFT);

		type = new UI_DynInput(0, 0, 0, 0);
		type->align(FL_ALIGN_LEFT);
		type->callback(type_callback, this);
		type->callback2(dyntype_callback, this);
		type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		type->type(FL_INT_INPUT);
		type_flex->fixed(type, TYPE_INPUT_WIDTH);

		choose = new Fl_Button(0, 0, 0, 0, "Choose");
		choose->callback(button_callback, this);
		type_flex->fixed(choose, fl_width(choose->label()) + 16);

		gen = new Fl_Button(0, 0, 0, 0, "Gen");
		gen->callback(button_callback, this);
		type_flex->fixed(gen, fl_width(gen->label()) + 16);

		type_flex->end();
	}

	{
		descFlex = new Fl_Flex(X + TYPE_INPUT_X, 0, W - TYPE_INPUT_X - NOMBRE_INSET,
							   TYPE_INPUT_HEIGHT, Fl_Flex::HORIZONTAL);
		descFlex->label("Desc:");
		descFlex->spacing(BUTTON_SPACING);
		descFlex->align(FL_ALIGN_LEFT);

		actkind = new Fl_Choice(0, 0, 0, 0);
		// this order must match the SPAC_XXX constants
		actkind->add(getActivationMenuString());
		actkind->value(getActivationCount());
		actkind->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Activation | MLF_Repeatable));
		actkind->deactivate();
		actkind->hide();

		udmfActivationButton = new Fl_Menu_Button(0, 0, 0, 0, UDMF_ACTIVATION_BUTTON_LABEL);
		udmfActivationButton->hide();

		descFlex->fixed(actkind, TYPE_INPUT_WIDTH);
		descFlex->fixed(udmfActivationButton, TYPE_INPUT_WIDTH);

		desc = new Fl_Output(0, 0, 0, 0);
		desc->align(FL_ALIGN_LEFT);

		descFlex->end();
	}

	{
		argsFlex = new Fl_Flex(which->x(), 0, which->w(), TYPE_INPUT_HEIGHT, Fl_Flex::HORIZONTAL);
		argsFlex->gap(INPUT_SPACING);

		args0str = new UI_DynInput(0, 0, 0, 0);
		args0str->callback(args_callback, new line_flag_CB_data_c(this, 5));
		args0str->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		args0str->hide();
		args0str->align(FL_ALIGN_BOTTOM);
		args0str->labelsize(ARG_LABELSIZE);

		for (int a = 0 ; a < 5 ; a++)
		{
			args[a] = new UI_DynIntInput(0, 0, 0, 0);
			args[a]->callback(args_callback, new line_flag_CB_data_c(this, a));
			args[a]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
			args[a]->align(FL_ALIGN_BOTTOM);
			args[a]->labelsize(ARG_LABELSIZE);
		}

		argsFlex->end();
		argsFlex->hide();
	}
	panel->afterSpacing(argsFlex, 16 - INPUT_SPACING);

	{
		Fl_Group *tag_pack = new Fl_Group(X + TYPE_INPUT_X, 0, W - TYPE_INPUT_X - NOMBRE_INSET,
										  TYPE_INPUT_HEIGHT);
		static const char lengthLabel[] = "Length:";
//		tag_pack->gap(fl_width(lengthLabel) + 16);

		tag = new UI_DynIntInput(tag_pack->x(), tag_pack->y(), TAG_WIDTH, tag_pack->h(), "Tag:");
		tag->align(FL_ALIGN_LEFT);
		tag->callback(tag_callback, this);
		tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

		const int labelWidth = fl_width(lengthLabel) + 8;
		length = new UI_DynIntInput(tag_pack->x() + tag_pack->w() / 2 + labelWidth, tag_pack->y(),
									tag_pack->w() / 2 - labelWidth, tag_pack->h(), lengthLabel);
		length->align(FL_ALIGN_LEFT);
		length->callback(length_callback, this);
		length->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

		tag_pack->end();
	}

	multiTagView = new MultiTagView(inst, [this](){
		redraw();
	}, [this](){
		if (this->inst.edit.Selected->empty())
			return;
		EditOperation op(this->inst.level.basis);
		op.setMessageForSelection("changed other tags", *this->inst.edit.Selected);
		for (sel_iter_c it(*this->inst.edit.Selected) ; !it.done() ; it.next())
		{
			std::set<int> tags = multiTagView->getTags();
			op.changeLinedef(*it, &LineDef::moreIDs, std::move(tags));
		}
	}, X, Y, W, TYPE_INPUT_HEIGHT);
	multiTagView->hide();

	Y += tag->h() + 16;

	{
		Fl_Group *flags_pack = new Fl_Group(X, Y, W, 22);
		Fl_Box *flags = new Fl_Box(FL_FLAT_BOX, X+10, Y, 64, 24, "Flags: ");
		flags->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);


		f_automap = new Fl_Choice(X+W-118, Y, 104, FIELD_HEIGHT, "Map: ");
		f_automap->add(kHardcodedAutoMapMenu);
		f_automap->value(0);
		f_automap_cb_data = std::make_unique<line_flag_CB_data_c>(this, MLF_ALL_AUTOMAP);
		f_automap->callback(flags_callback, f_automap_cb_data.get());

		flags_pack->end();
		panel->beforeSpacing(flags_pack, 8);
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

	panel->beforeSpacing(front, 16);

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

void UI_LineBox::populateUDMFFlagCheckBoxes(const FeatureFlagMapping *mapping, size_t count,
											const LoadingData &loaded, const ConfigData &config,
											const char *title)
{
	if(loaded.levelFormat != MapFormat::udmf)
		return;

	// Check if features exist at all before proceeding
	bool found = false;
	for(size_t i = 0; i < count; ++i)
	{
		const FeatureFlagMapping &entry = mapping[i];
		if(UDMF_HasLineFeature(config, entry.feature))
		{
			found = true;
			break;
		}
	}
	if(!found)
		return;

	CategoryHeader header{};
	const int xPos = which->x();
	const int wSize = which->w();
	if(title)
	{
		header.button = new UI_CategoryButton(xPos, 0, wSize, FIELD_HEIGHT, title);
		header.button->callback(category_callback, this);
	}
	const int numRows = (int)(count + 1) / 2;
	header.grid = new Fl_Grid(xPos + 8, 0, wSize - 8, FLAG_ROW_HEIGHT * numRows);
	header.grid->layout(numRows, 2);

	int index = 0;
	for(size_t i = 0; i < count; ++i)
	{
		const FeatureFlagMapping &entry = mapping[i];
		LineFlagButton button{};
		button.button = new Fl_Check_Button(0, 0, 0, 0, entry.label);
		button.button->labelsize(FLAG_LABELSIZE);
		button.button->tooltip(entry.tooltip);
		button.data = std::make_unique<line_flag_CB_data_c>(
			this, entry.flagSet == 1 ? entry.value : 0, entry.flagSet == 2 ? entry.value : 0
		);
		button.button->callback(flags_callback, button.data.get());
		flagButtons.push_back(std::move(button));

		header.grid->add(button.button);
		header.grid->widget(button.button, index % numRows, index / numRows);
		header.lineFlagButtonIndices.push_back(static_cast<int>(flagButtons.size()) - 1);
		++index;
	}

	if(title)
		panel->insert(*header.button, front);
	panel->insert(*header.grid, front);

	categoryHeaders.push_back(std::move(header));
}

void UI_LineBox::updateUDMFBaseFlags(const LoadingData &loaded, const ConfigData &config)
{
	static const FeatureFlagMapping baseFlags[] =
	{
		{UDMF_LineFeature::twosided, "two sided", "Line behaves properly as two-sided",
		MLF_TwoSided, 1},
		{UDMF_LineFeature::dontpegtop, "upper unpeg", "Upper texture tiles from the top",
		MLF_UpperUnpegged, 1},
		{UDMF_LineFeature::dontpegbottom, "lower unpeg",
		"Lower texture tiles starting from ceiling", MLF_LowerUnpegged, 1},
		{UDMF_LineFeature::blocking, "impassable", "Classic Doom wall or railing", MLF_Blocking, 1},
		{UDMF_LineFeature::blockmonsters, "block monsters", "Blocks only enemy monsters",
		MLF_BlockMonsters, 1},
		{UDMF_LineFeature::blocksound, "sound block", "Half-blocks alert propagation",
		MLF_SoundBlock, 1},
		{UDMF_LineFeature::passuse, "pass thru", "Allows the 'use' activation to pass through",
		MLF_Boom_PassThru, 1},
	};

	populateUDMFFlagCheckBoxes(baseFlags, lengthof(baseFlags), loaded, config, nullptr);
}

void UI_LineBox::updateUDMFActivationMenu(const LoadingData &loaded, const ConfigData &config)
{
	udmfActivationButton->clear();
	udmfActivationMenuItems.clear();
	switch(loaded.levelFormat)
	{
		case MapFormat::udmf:
			if(config.features.udmf_lineparameters)
			{
				udmfActivationButton->show();
				break;
			}
			// else fall through
		case MapFormat::doom:
		case MapFormat::hexen:
			udmfActivationButton->hide();
			return;
		default:
			return;
	}

	// We have it, so let's do it.


	// Description source: https://github.com/ZDoom/gzdoom/blob/g4.14.2/specs/udmf_zdoom.txt
	static const FeatureFlagMapping activatorAndMode[] =
	{
		{ UDMF_LineFeature::playercross, "Player crossing",
		"Player crossing will trigger this line", MLF_UDMF_PlayerCross, 1 },
		{ UDMF_LineFeature::playeruse, "Player using", "Player operating will trigger this line",
	    MLF_UDMF_PlayerUse, 1 },
		{ UDMF_LineFeature::playerpush, "Player bumping", "Player bumping will trigger this line",
		MLF_UDMF_PlayerPush, 1 },
		{ UDMF_LineFeature::monstercross, "Monster crossing",
		"Monster crossing will trigger this line", MLF_UDMF_MonsterCross, 1 },
		{ UDMF_LineFeature::monsteruse, "Monster using", "Monster operating will trigger this line",
		MLF_UDMF_MonsterUse, 1 },
		{ UDMF_LineFeature::monsterpush, "Monster bumping",
		"Monster pushing will trigger this line", MLF_UDMF_MonsterPush, 1 },
		{ UDMF_LineFeature::impact, "On gunshot",
		"Shooting bullets will trigger this line", MLF_UDMF_Impact, 1 },
		{ UDMF_LineFeature::missilecross, "Projectile crossing",
		"Projectile crossing will trigger this line", MLF_UDMF_MissileCross, 1 },
		{ UDMF_LineFeature::anycross, "Anything crossing",
		"Any non-projectile crossing will trigger this line", MLF2_UDMF_AnyCross, 2 },
	};

	static const FeatureFlagMapping mainFlags[] =
	{
		{ UDMF_LineFeature::repeatspecial, "Repeatable activation", "Allow retriggering",
		MLF_UDMF_RepeatSpecial, 1 },
		{ UDMF_LineFeature::checkswitchrange, "Check switch range",
		"Switch can only be activated when vertically reachable", MLF2_UDMF_CheckSwitchRange, 2 },
		{ UDMF_LineFeature::firstsideonly, "First side only",
		"Line can only be triggered from the front side", MLF2_UDMF_FirstSideOnly, 2 },
		{ UDMF_LineFeature::playeruseback, "Player can use from behind",
		"Player can use from back (left) side", MLF2_UDMF_PlayerUseBack, 2 },
		{ UDMF_LineFeature::monsteractivate, "Monster can activate",
		"Monsters can trigger this line (for compatibility only)", MLF2_UDMF_MonsterActivate, 2 },
	};

	static const FeatureFlagMapping destructibleFlags[] =
	{
		{ UDMF_LineFeature::damagespecial, "Trigger on damage",
		"This line will call special if having health > 0 and receiving damage",
		MLF2_UDMF_DamageSpecial, 2 },
		{ UDMF_LineFeature::deathspecial, "Trigger on death",
		"This line will call special if health was reduced to 0", MLF2_UDMF_DeathSpecial, 2 },
	};

	auto addItem = [this, &config](const FeatureFlagMapping &entry, bool &addSeparator)
	{
		if(!UDMF_HasLineFeature(config, entry.feature))
			return;

		if(addSeparator && !udmfActivationMenuItems.empty())
		{
			udmfActivationMenuItems.back().flags |= FL_MENU_DIVIDER;
			addSeparator =false;
		}

		Fl_Menu_Item item = {};
		item.flags = FL_MENU_TOGGLE;
		item.text = entry.label;
		// FIXME: tooltip

		LineFlagButton button{};
		button.udmfActivationMenuIndex = static_cast<int>(udmfActivationMenuItems.size());
		button.data = std::make_unique<line_flag_CB_data_c>(
			this, entry.flagSet == 1 ? entry.value : 0, entry.flagSet == 2 ? entry.value : 0
		);
		// TODO: info
		flagButtons.push_back(std::move(button));
		item.callback(flags_callback, flagButtons.back().data.get());
		udmfActivationMenuItems.push_back(std::move(item));
	};

	bool addSeparator = false;
	for(const FeatureFlagMapping &entry : activatorAndMode)
		addItem(entry, addSeparator);
	addSeparator = true;
	for(const FeatureFlagMapping &entry : mainFlags)
		addItem(entry, addSeparator);
	addSeparator = true;
	for(const FeatureFlagMapping &entry : destructibleFlags)
		addItem(entry, addSeparator);
	udmfActivationMenuItems.emplace_back();
	udmfActivationButton->menu(udmfActivationMenuItems.data());
}

void UI_LineBox::updateUDMFBlockingFlags(const LoadingData &loaded, const ConfigData &config)
{
	static const FeatureFlagMapping blockingFlags[] =
	{
		{UDMF_LineFeature::blockeverything, "block everything", "Acts as a solid wall", MLF2_UDMF_BlockEverything, 2},
		{UDMF_LineFeature::midtex3d, "3DMidTex",
		"Eternity 3D middle texture: solid railing which can be walked over and under",
		MLF_Eternity_3DMidTex, 1},
		{UDMF_LineFeature::midtex3dimpassible, "3DMidTex+missiles",
		"Coupled with 3DMidTex, this also allows projectiles to pass", MLF2_UDMF_MidTex3DImpassible,
		2},
		{UDMF_LineFeature::jumpover, "Strife railing", "Strife-style skippable railing",
		MLF_UDMF_JumpOver, 1},
		{UDMF_LineFeature::blockfloaters, "block floaters", "Block flying enemies",
		MLF_UDMF_BlockFloaters, 1},
		{UDMF_LineFeature::blocklandmonsters, "block land monsters", "Block walking enemies",
		MLF_UDMF_BlockLandMonsters, 1},
		{UDMF_LineFeature::blockplayers, "block players", "Block only players", MLF_UDMF_BlockPlayers, 1},
		{UDMF_LineFeature::blockhitscan, "block gunshots",
		"Blocks hitscans (infinite speed gunshots)", MLF_UDMF_BlockHitScan, 1},
		{UDMF_LineFeature::blockprojectiles, "block projectiles", "Blocks finite-speed projectiles",
		MLF_UDMF_BlockProjectiles, 1},
		{UDMF_LineFeature::blocksight, "block sight", "Blocks monster sight", MLF_UDMF_BlockSight,
		1},
		{UDMF_LineFeature::blockuse, "block use", "Blocks 'use' operation", MLF_UDMF_BlockUse, 1},
	};

	populateUDMFFlagCheckBoxes(blockingFlags, lengthof(blockingFlags), loaded, config, "Advanced blocking");
}

void UI_LineBox::updateUDMFRenderingControls(const LoadingData &loaded, const ConfigData &config)
{
	if(loaded.levelFormat != MapFormat::udmf)
		return;

	static const FeatureFlagMapping renderingFlags[] =
	{
		{UDMF_LineFeature::translucent, "translucent", "Render line as translucent (Strife style)",
		MLF_UDMF_Translucent, 1},
		{UDMF_LineFeature::transparent, "more translucent",
		"Render line as even more translucent (Strife style)", MLF_UDMF_Transparent, 1},
		{UDMF_LineFeature::clipmidtex, "clip railing texture",
		"Always clip two-sided middle texture to floor and ceiling, even if sector properties don't change",
		MLF2_UDMF_ClipMidTex, 2},
		{UDMF_LineFeature::clipmidtex, "tile railing texture", "Repeat two-sided middle texture vertically",
		MLF2_UDMF_WrapMidTex, 2},
	};

	populateUDMFFlagCheckBoxes(renderingFlags, lengthof(renderingFlags), loaded, config, "Advanced rendering");

	Fl_Grid *grid = categoryHeaders.back().grid;

	if(UDMF_HasLineFeature(config, UDMF_LineFeature::alpha))
	{
		int count = 0;
		for(int index : categoryHeaders.back().lineFlagButtonIndices)
		{
			LineFlagButton &button = flagButtons[index];
			if(!button.data || !(button.data->mask & (MLF_UDMF_Translucent | MLF_UDMF_Transparent)))
				continue;
			udmfTranslucencyCheckBoxes[count++] = button.button;
			if(count >= (int)lengthof(udmfTranslucencyCheckBoxes))
				break;
		}
		grid->layout(grid->rows() + 1, grid->cols());
		grid->size(grid->w(), grid->h() + FLAG_ROW_HEIGHT);

		static const char labelText[] = "Custom alpha:";
		fl_font(FL_HELVETICA, FLAG_LABELSIZE);
		const int margin = fl_width(labelText);

		Fl_Flex *spacerFlex = new Fl_Flex(0, 0, 0, 0);
		spacerFlex->margin(margin, 0, 0, 0);

		// Provide a nonzero placeholder width to prevent value_width below from getting truncated
		Fl_Hor_Value_Slider *slider = new Fl_Hor_Value_Slider(0, 0, 2 * ALPHA_NUMBER_WIDTH, 0,
															  labelText);
		alphaWidget = slider;
		slider->align(FL_ALIGN_LEFT);
		slider->labelsize(FLAG_LABELSIZE);
		slider->step(0.015625);
		slider->minimum(0);
		slider->maximum(1);
		slider->value_width(ALPHA_NUMBER_WIDTH);
		slider->callback(field_callback, this);
		slider->tooltip("Custom translucency");
		slider->when(FL_WHEN_RELEASE);
		spacerFlex->end();

		grid->add(spacerFlex);
		grid->widget(spacerFlex, grid->rows() - 1, 0, 1, 2);
	}
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

			const char *b_name = (b == 0) ? SD->l_panel->getTex()->value() :
								 (b == 1) ? SD->u_panel->getTex()->value() :
											SD->r_panel->getTex()->value();
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

	if(w == box->udmfActivationButton)
	{
		box->udmfActivationButton->popup();
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
	struct IntFieldMapping
	{
		const char *name;
		byte fieldID;
	};

	static const IntFieldMapping intFieldMapping[] =
	{
		{ "locknumber", LineDef::F_LOCKNUMBER },
		{ "automapstyle", LineDef::F_AUTOMAPSTYLE },
		{ "health", LineDef::F_HEALTH },
		{ "healthgroup", LineDef::F_HEALTHGROUP },
	};

	UI_LineBox *box = (UI_LineBox *)data;
	if(w == box->alphaWidget)
	{
		box->checkDirtyFields();
		box->checkSidesDirtyFields();

		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited alpha of", *box->inst.edit.Selected);
		for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			op.changeLinedef(*it, &LineDef::alpha, box->alphaWidget->value());
		return;
	}

	if(!box->inst.edit.Selected->empty())
	{
		// Find which field widget this is and whether it's widget or widget2
		const linefield_t *info = nullptr;
		bool isSecondWidget = false;
		for(const LineField &field : box->fields)
		{
			if(field.widget == w)
			{
				info = field.info;
				break;
			}
			if(field.widget2 == w)
			{
				info = field.info;
				isSecondWidget = true;
				break;
			}
		}

		if(!info)
			return;

		// Determine which identifier to use based on which widget was triggered
		const SString &identifier = isSecondWidget ? info->identifier2 : info->identifier;
		SString message = SString("edited ") + identifier + " of";

		switch(info->type)
		{
		case linefield_t::Type::choice:
		{
			auto choice = static_cast<Fl_Choice *>(w);
			int index = choice->value();
			if(index < 0 || index >= static_cast<int>(info->options.size()))
				return;

			box->checkDirtyFields();
			box->checkSidesDirtyFields();

			int new_value = info->options[index].value;

			// Apply to all selected linedefs
			EditOperation op(box->inst.level.basis);
			op.setMessageForSelection(message.c_str(), *box->inst.edit.Selected);

			for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			{
				for(const IntFieldMapping &cm : intFieldMapping)
				{
					if(identifier.noCaseEqual(cm.name))
					{
						op.changeLinedef(*it, cm.fieldID, new_value);
						break;
					}
				}
			}

			break;
		}
		case linefield_t::Type::slider:
		{
			box->checkDirtyFields();
			box->checkSidesDirtyFields();

			auto valuator = static_cast<Fl_Valuator *>(w);

			EditOperation op(box->inst.level.basis);
			op.setMessageForSelection(message.c_str(), *box->inst.edit.Selected);

			for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			{
				// Hardcode the field mapping for now
				if(identifier.noCaseEqual("alpha"))
					op.changeLinedef(*it, &LineDef::alpha, valuator->value());
			}
			break;
		}
		case linefield_t::Type::intpair:
		{
			auto input = static_cast<Fl_Int_Input *>(w);
			int new_value = atoi(input->value());

			EditOperation op(box->inst.level.basis);
			op.setMessageForSelection(message.c_str(), *box->inst.edit.Selected);

			for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			{
				for(const IntFieldMapping &cm : intFieldMapping)
				{
					if(identifier.noCaseEqual(cm.name))
					{
						op.changeLinedef(*it, cm.fieldID, new_value);
						break;
					}
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
		if(!categoryBtn || cat.button == categoryBtn)
		{
			if(cat.button->isExpanded())
				cat.grid->show();
			else
				cat.grid->hide();

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
			if(flagBtn.button && flagBtn.info && flagBtn.button->value())
			{
				summaryText += flagBtn.info->inCategoryAcronym;
			}
		}
		header.button->details(summaryText);
	}

}

void UI_LineBox::UpdateField(std::optional<Basis::EditField> efield)
{
	if(inst.level.isLinedef(obj))
		CalcLength();
	else
		mFixUp.setInputValue(length, "");

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
			inst.loaded.levelFormat == MapFormat::udmf)
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

		inst.main_win->browser->UpdateGenType(0);
	}

	bool changed = false;
	if (inst.level.isLinedef(obj))
	{
		actkind->activate();
		udmfActivationButton->activate();

		changed |= FlagsFromInt(inst.level.linedefs[obj]->flags);
	}
	else
	{
		changed |= FlagsFromInt(0);

		actkind->value(getActivationCount());  // show as "??"
		actkind->deactivate();
		udmfActivationButton->deactivate();
	}

	if(inst.level.isLinedef(obj))
		changed |= Flags2FromInt(inst.level.linedefs[obj]->flags2);
	else
		changed |= Flags2FromInt(0);

	if(changed)
		updateCategoryDetails();

	struct IntFieldMapping
	{
		const char *const name;
		int LineDef::* field;
	};

	static const IntFieldMapping intFieldMappings[] =
	{
		{ "locknumber", &LineDef::locknumber },
		{ "automapstyle", &LineDef::automapstyle },
		{ "health", &LineDef::health },
		{ "healthgroup", &LineDef::healthgroup },
	};

	// Update choice and intpair widgets for UDMF properties
	if(inst.level.isLinedef(obj))
	{
		const auto L = inst.level.linedefs[obj];

		for(const LineField &field : fields)
		{
			if(field.info->type == linefield_t::Type::choice)
			{
				for(const IntFieldMapping &cm : intFieldMappings)
				{
					if(field.info->identifier.noCaseEqual(cm.name))
					{
						int value = L.get()->*cm.field;

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
						static_cast<Fl_Choice*>(field.widget)->value(index);
						break;
					}
				}
			}
			else if(field.info->type == linefield_t::Type::intpair)
			{
				// Update first widget
				for(const IntFieldMapping &cm : intFieldMappings)
				{
					if(field.info->identifier.noCaseEqual(cm.name))
					{
						int value = L.get()->*cm.field;
						mFixUp.setInputValue(static_cast<UI_DynIntInput*>(field.widget),
												SString(value).c_str());
						break;
					}
				}
				// Update second widget
				for(const IntFieldMapping &cm : intFieldMappings)
				{
					if(field.info->identifier2.noCaseEqual(cm.name))
					{
						int value = L.get()->*cm.field;
						mFixUp.setInputValue(static_cast<UI_DynIntInput*>(field.widget2),
												SString(value).c_str());
						break;
					}
				}
			}
		}
	}
	else
	{
		// No linedef selected, reset widgets
		for(const LineField &field : fields)
		{
			if(field.info->type == linefield_t::Type::choice)
			{
				static_cast<Fl_Choice*>(field.widget)->value(0);
			}
			else if(field.info->type == linefield_t::Type::intpair)
			{
				mFixUp.setInputValue(static_cast<UI_DynIntInput*>(field.widget), "");
				mFixUp.setInputValue(static_cast<UI_DynIntInput*>(field.widget2), "");
			}
		}
	}

	if(alphaWidget)
	{
		if(inst.level.isLinedef(obj))
		{
			double alpha = inst.level.linedefs[obj]->alpha;
			alphaWidget->value(alpha);
			for(Fl_Check_Button *button : udmfTranslucencyCheckBoxes)
				if(button)
					button->labelcolor(alpha >= 1 ? FL_FOREGROUND_COLOR : FL_INACTIVE_COLOR);
		}
		else
		{
			alphaWidget->value(0);
			for(Fl_Check_Button *button : udmfTranslucencyCheckBoxes)
				if(button)
					button->labelcolor(FL_FOREGROUND_COLOR);
		}
	}

	if(inst.conf.features.udmf_multipletags)
	{
		if(inst.level.isLinedef(obj))
		{
			multiTagView->setTags(std::set(inst.level.linedefs[obj]->moreIDs));
		}
		else
		{
			multiTagView->clearTags();
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

bool UI_LineBox::updateDynamicFlagControls(int lineFlags, int flagSet)
{
	// Set dynamic line flag buttons
	bool changed = false;
	for(const auto &btn : flagButtons)
	{
		if(!btn.data || (flagSet == 1 && !btn.data->mask) || (flagSet == 2 && !btn.data->mask2))
			continue;
		const int newValue = ((flagSet == 1 && btn.data->mask & lineFlags) ||
							  (flagSet == 2 && btn.data->mask2 & lineFlags)) ? 1 : 0;
		if(btn.button)
		{
			changed = changed || (btn.button->value() != newValue);
			btn.button->value(newValue);
		}
		else if(btn.udmfActivationMenuIndex >= 0)
		{
			Fl_Menu_Item &item = udmfActivationMenuItems[btn.udmfActivationMenuIndex];
			changed = changed || (item.value() != newValue);
			item.value(newValue);
		}
	}
	return changed;
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

	// TODO: set the UDMF activation button label

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

	return updateDynamicFlagControls(lineflags, 1);
}

bool UI_LineBox::Flags2FromInt(int lineflags)
{
	return updateDynamicFlagControls(lineflags, 2);
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
		if(!btn.data || (btn.button && !btn.button->value()) ||
		   (btn.udmfActivationMenuIndex >= 0 &&
			!udmfActivationMenuItems[btn.udmfActivationMenuIndex].value()))
		{
			continue;
		}
		outFlags |= btn.data->mask;
		outFlags2 |= btn.data->mask2;
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
	if (loaded.levelFormat == MapFormat::hexen)
	{
		tag->hide();
		gen->hide();

		actkind->clear();
		actkind->add(getActivationMenuString());

		actkind->show();
	}
	else if(loaded.levelFormat == MapFormat::udmf)
	{
		tag->show();
		tag->label("Line ID:");
		tag->tooltip("Tag of the linedef");

		actkind->hide();	// UDMF uses the separate line flags for activation

		if (config.features.gen_types)
			gen->show();
		else
			gen->hide();
	}
	else
	{
		tag->show();
		tag->label("Tag:");
		tag->tooltip("Tag of targeted sector(s)");

		actkind->hide();

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
			panel->remove(cat.button);
		delete cat.button;
		panel->remove(cat.grid);
		delete cat.grid;
	}
	categoryHeaders.clear();
	flagButtons.clear();
	alphaWidget = nullptr;
	memset(udmfTranslucencyCheckBoxes, 0, sizeof(udmfTranslucencyCheckBoxes));
	updateUDMFBaseFlags(loaded, config);
	updateUDMFActivationMenu(loaded, config);
	updateUDMFBlockingFlags(loaded, config);
	updateUDMFRenderingControls(loaded, config);

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

		static const int FW = 110;
		const int leftX = x() + flagsStartX + 28;
		const int rightX = x() + flagsStartX + flagsAreaW - 120;

		//begin();

		// Process each category
		for(auto &catPair : categorized)
		{
			const SString &catName = catPair.first;
			const std::vector<const lineflag_t *> &flagsInCat = catPair.second;

			CategoryHeader catHeader = {};
			if(!catName.empty())
			{
				catHeader.button = new UI_CategoryButton(x() + flagsStartX, Y,
					flagsAreaW, FIELD_HEIGHT);
				catHeader.button->copy_label(catName.c_str());
				catHeader.button->callback(category_callback, this);

				Y += FIELD_HEIGHT;
			}
			const int numRows = (int(flagsInCat.size()) + 1) / 2;
			catHeader.grid = new Fl_Grid(leftX, 0, FW + rightX - leftX,
													   FLAG_ROW_HEIGHT * numRows);
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
						fb.button->labelsize(FLAG_LABELSIZE);
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
					Fl_Group *pairGroup = new Fl_Group(baseX, curY, FW, FLAG_ROW_HEIGHT);
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
				curY += FLAG_ROW_HEIGHT;
			}

			Y = (yLeft > yRight ? yLeft : yRight);

			if(catHeader.button)
				panel->insert(*catHeader.button, front);
			panel->insert(*catHeader.grid, front);
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
	{
		// Unload intpair widgets from mFixUp before removing them
		if(field.info->type == linefield_t::Type::intpair && field.widget2)
		{
			mFixUp.unloadFields({static_cast<UI_DynIntInput *>(field.widget),
								 static_cast<UI_DynIntInput *>(field.widget2)});
			panel->remove(field.container);
			delete field.container;
		}
		else
		{
			panel->remove(field.widget);
			delete field.widget;
		}
	}
	fields.clear();

	if(loaded.levelFormat == MapFormat::udmf && !config.udmf_line_fields.empty())
	{
		// Match the args span: starts at type->x() and spans 5 args

		//begin();

		for(const linefield_t &lf : config.udmf_line_fields)
		{
			LineField field = {};
			const int labelWidth = fl_width(lf.label.c_str()) + 16;
			const int fieldX = x() + std::max((int)TYPE_INPUT_X, labelWidth);
			const int fieldW = which->x() + which->w() - fieldX;

			if(lf.type == linefield_t::Type::choice)
			{
				if(lf.options.empty())
					continue;

				auto choice = new Fl_Choice(fieldX, Y, fieldW, FIELD_HEIGHT);
				field.widget = choice;

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
				field.widget = slider;

				slider->step(lf.step);
				// slider->lstep(lf.step * 8);
				slider->minimum(lf.minValue);
				slider->maximum(lf.maxValue);
				slider->value_width(ALPHA_NUMBER_WIDTH);
			}
			else if(lf.type == linefield_t::Type::intpair)
			{
				// Create horizontal Fl_Flex container
				auto flex = new Fl_Flex(fieldX, Y, fieldW, FIELD_HEIGHT, Fl_Flex::HORIZONTAL);
				flex->gap(16 + fl_width(lf.label2.c_str()));
				field.container = flex;

				// First input (left)
				auto input1 = new UI_DynIntInput(0, 0, 0, FIELD_HEIGHT);
				field.widget = input1;
				input1->copy_label((lf.label + ":").c_str());
				input1->align(FL_ALIGN_LEFT);
				input1->callback(field_callback, this);

				// Second input (right) - set fixed width for the label spacing
				auto input2 = new UI_DynIntInput(0, 0, 0, FIELD_HEIGHT);
				field.widget2 = input2;
				input2->copy_label((lf.label2 + ":").c_str());
				input2->align(FL_ALIGN_LEFT);
				input2->callback(field_callback, this);
				//flex->fixed(input2, label2Width + 64);  // label width + input width

				flex->end();

				field.info = &lf;
				fields.push_back(std::move(field));
				Y += FIELD_HEIGHT + 4;

				panel->insert(*fields.back().container, front);

				// Register both inputs with mFixUp
				mFixUp.loadFields({static_cast<UI_DynIntInput *>(fields.back().widget),
								   static_cast<UI_DynIntInput *>(fields.back().widget2)});
				continue;
			}
			else
			{
				assert(false);
			}
			field.widget->copy_label((lf.label + ":").c_str());
			field.widget->align(FL_ALIGN_LEFT);

			// Pass the linefield_t pointer via callback data
			field.widget->callback(field_callback, this);
			field.info = &lf;

			fields.push_back(std::move(field));
			Y += FIELD_HEIGHT + 4;

			panel->insert(*fields.back().widget, front);
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

	if (loaded.levelFormat == MapFormat::hexen || loaded.levelFormat == MapFormat::udmf)
	{
		argsFlex->show();
		args0str->hide();
		for(int a = 0; a < 5; ++a)
		{
			args[a]->label("");
		}
	}
	else
		argsFlex->hide();

	if(config.features.udmf_multipletags)
	{
		multiTagView->show();
		multiTagView->clearTags();
	}
	else
		multiTagView->hide();

	// Update UDMF sidepart widgets for sidedefs
	front->UpdateGameInfo(loaded, config);
	back->UpdateGameInfo(loaded, config);

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
