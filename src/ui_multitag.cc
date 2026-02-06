//------------------------------------------------------------------------
//  Multi tag ("more IDs") UDMF widgets
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
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

#include "ui_multitag.h"

#include "Instance.h"
#include "lib_util.h"

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Int_Input.H>
#include <FL/fl_draw.H>

#include <string>

// Constants shared with ui_linedef
static constexpr int TYPE_INPUT_HEIGHT = 24;
static constexpr int INPUT_SPACING = 2;
static constexpr int FLAG_LABELSIZE = 12;

//------------------------------------------------------------------------
//  IDTag Implementation
//------------------------------------------------------------------------

IDTag::IDTag(int X, int Y, int W, int H, const char *label)
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

void IDTag::callback(Fl_Callback *cb, void *data)
{
	mCallback = cb;
	mCallbackData = data;
}

int IDTag::calcRequiredWidth(const char *labelText, int labelSize)
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

int IDTag::handle(int event)
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

void IDTag::draw()
{
	// Background color
	Fl_Color bgColor = FL_BACKGROUND_COLOR;
	fl_color(bgColor);

	// Draw rounded rectangle
	fl_push_clip(x(), y(), w(), h());

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

bool IDTag::isInXButton(int mx, int my) const
{
	int xBtnX = x() + w() - X_BUTTON_SIZE - X_BUTTON_MARGIN;
	int xBtnY = y() + (h() - X_BUTTON_SIZE) / 2;

	return mx >= xBtnX && mx <= xBtnX + X_BUTTON_SIZE &&
		   my >= xBtnY && my <= xBtnY + X_BUTTON_SIZE;
}

void IDTag::drawRoundedRectOutline(int X, int Y, int W, int H, int radius)
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

//------------------------------------------------------------------------
//  MultiTagView Implementation
//------------------------------------------------------------------------

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
