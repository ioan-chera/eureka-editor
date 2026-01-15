//------------------------------------------------------------------------
//  ACTIVATION POPUP BUTTON
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

#include "ui_activation_button.h"
#include "m_game.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>

static constexpr int kCheckboxWidth = 110;
static constexpr int kCheckboxHeight = 20;
static constexpr int kGridPadding = 8;
static constexpr int kRowHeight = 22;

//------------------------------------------------------------------------
// UI_ActivationPopup implementation
//------------------------------------------------------------------------

UI_ActivationPopup::UI_ActivationPopup(int W, int H) :
	Fl_Menu_Window(W, H)
{
	box(FL_UP_BOX);
	end();
}

int UI_ActivationPopup::handle(int event)
{
	switch(event)
	{
	case FL_PUSH:
		// If click is outside our bounds, hide the popup
		if(Fl::event_x() < 0 || Fl::event_x() >= w() ||
		   Fl::event_y() < 0 || Fl::event_y() >= h())
		{
			hide();
			return 1;
		}
		break;

	case FL_UNFOCUS:
		// Hide when losing focus
		hide();
		return 1;
	}

	return Fl_Menu_Window::handle(event);
}

void UI_ActivationPopup::setFlags(const std::vector<const lineflag_t *> &flags)
{
	// Clear existing checkboxes
	if(mGrid)
	{
		remove(mGrid.get());
		mGrid.reset();
	}
	mCheckboxes.clear();

	if(flags.empty())
		return;

	// Calculate grid dimensions
	const int numFlags = static_cast<int>(flags.size());
	const int numRows = (numFlags + 1) / 2;
	const int gridW = kCheckboxWidth * 2 + kGridPadding * 3;
	const int gridH = kRowHeight * numRows + kGridPadding * 2;

	// Resize window to fit
	size(gridW, gridH);

	// Create grid
	begin();
	mGrid = std::make_unique<Fl_Grid>(kGridPadding, kGridPadding,
									  gridW - kGridPadding * 2, gridH - kGridPadding * 2);
	mGrid->layout(numRows, 2);
	mGrid->box(FL_NO_BOX);

	// Create checkboxes
	mCheckboxes.reserve(numFlags);
	for(int i = 0; i < numFlags; ++i)
	{
		const lineflag_t *flag = flags[i];
		const int row = i / 2;
		const int col = i % 2;

		auto checkbox = new Fl_Check_Button(0, 0, kCheckboxWidth, kCheckboxHeight,
											flag->label.c_str());
		checkbox->labelsize(12);
		checkbox->callback(checkboxCallback, this);

		mGrid->widget(checkbox, row, col);

		FlagCheckbox fc;
		fc.button = checkbox;
		fc.info = flag;
		mCheckboxes.push_back(fc);
	}

	mGrid->end();
	end();
}

void UI_ActivationPopup::setCallback(const std::function<void(const lineflag_t *, int)> &cb)
{
	mCallback = cb;
}

void UI_ActivationPopup::updateFromFlags(int flags, int flags2)
{
	for(const FlagCheckbox &fc : mCheckboxes)
	{
		int flagVal = (fc.info->flagSet == 1) ? flags : flags2;
		fc.button->value((flagVal & fc.info->value) ? 1 : 0);
	}
}

void UI_ActivationPopup::checkboxCallback(Fl_Widget *w, void *data)
{
	auto popup = static_cast<UI_ActivationPopup *>(data);
	auto checkbox = static_cast<Fl_Check_Button *>(w);

	// Find which flag was toggled
	for(const FlagCheckbox &fc : popup->mCheckboxes)
	{
		if(fc.button == checkbox)
		{
			if(popup->mCallback)
				popup->mCallback(fc.info, checkbox->value());
			break;
		}
	}
}

SString UI_ActivationPopup::getSummary() const
{
	SString summary;
	for(const FlagCheckbox &fc : mCheckboxes)
	{
		if(fc.button->value())
			summary += fc.info->inCategoryAcronym;
	}
	return summary;
}

//------------------------------------------------------------------------
// UI_ActivationButton implementation
//------------------------------------------------------------------------

UI_ActivationButton::UI_ActivationButton(int X, int Y, int W, int H, const char *label) :
	Fl_Button(X, Y, W, H, label)
{
	box(FL_UP_BOX);
	down_box(FL_DOWN_BOX);
	align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
}

int UI_ActivationButton::handle(int event)
{
	if(event == FL_PUSH)
	{
		if(mPopup && mPopup->visible())
			hidePopup();
		else
			showPopup();
		return 1;
	}

	return Fl_Button::handle(event);
}

void UI_ActivationButton::setFlags(const std::vector<const lineflag_t *> &flags)
{
	mFlags = flags;

	// Create or recreate popup with these flags
	mPopup = std::make_unique<UI_ActivationPopup>(200, 200);
	mPopup->setFlags(flags);
	mPopup->setCallback([this](const lineflag_t *flag, int value) {
		if(mCallback)
			mCallback(flag, value);
		updateSummaryLabel();
	});
}

void UI_ActivationButton::setCallback(const std::function<void(const lineflag_t *, int)> &cb)
{
	mCallback = cb;
	if(mPopup)
	{
		mPopup->setCallback([this, cb](const lineflag_t *flag, int value) {
			if(cb)
				cb(flag, value);
			updateSummaryLabel();
		});
	}
}

void UI_ActivationButton::updateFromFlags(int flags, int flags2)
{
	if(mPopup)
		mPopup->updateFromFlags(flags, flags2);
	updateSummaryLabel();
}

void UI_ActivationButton::updateSummaryLabel()
{
	if(mPopup)
		mSummary = mPopup->getSummary();
	else
		mSummary.clear();

	// Update button label
	if(mSummary.empty())
		copy_label("Activation");
	else
		copy_label(("Activation: " + mSummary).c_str());

	redraw();
}

void UI_ActivationButton::showPopup()
{
	if(!mPopup)
		return;

	// Position popup below the button
	int popupX = window()->x() + x();
	int popupY = window()->y() + y() + h();

	// Ensure popup stays on screen
	int screenW = Fl::w();
	int screenH = Fl::h();

	if(popupX + mPopup->w() > screenW)
		popupX = screenW - mPopup->w();
	if(popupY + mPopup->h() > screenH)
		popupY = window()->y() + y() - mPopup->h();

	mPopup->position(popupX, popupY);
	mPopup->show();
	mPopup->take_focus();
}

void UI_ActivationButton::hidePopup()
{
	if(mPopup)
		mPopup->hide();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
