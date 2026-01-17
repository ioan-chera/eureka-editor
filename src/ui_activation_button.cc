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

static constexpr const char *kDefaultLabel = "Activation...";

//------------------------------------------------------------------------
// UI_ActivationButton implementation
//------------------------------------------------------------------------

UI_ActivationButton::UI_ActivationButton(int X, int Y, int W, int H, const char *label) :
	Fl_Menu_Button(X, Y, W, H, label)
{
	copy_label(kDefaultLabel);
	align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
}

void UI_ActivationButton::setFlags(const std::vector<const lineflag_t *> &flags)
{
	mFlags = flags;

	// Clear and rebuild the menu
	clear();

	for(size_t i = 0; i < flags.size(); ++i)
	{
		const lineflag_t *flag = flags[i];
		// Add checkable menu item
		add(flag->label.c_str(), 0, menuCallback, this, FL_MENU_TOGGLE);
	}
}

void UI_ActivationButton::setCallback(const std::function<void(const lineflag_t *, int)> &cb)
{
	mCallback = cb;
}

void UI_ActivationButton::updateFromFlags(int flags, int flags2)
{
	// Update the check state of each menu item based on the flag values
	for(size_t i = 0; i < mFlags.size(); ++i)
	{
		const lineflag_t *flag = mFlags[i];
		int flagVal = (flag->flagSet == 1) ? flags : flags2;
		bool isSet = (flagVal & flag->value) != 0;

		Fl_Menu_Item *item = const_cast<Fl_Menu_Item *>(&menu()[i]);
		if(isSet)
			item->set();
		else
			item->clear();
	}

	updateLabel();
}

void UI_ActivationButton::updateLabel()
{
	mLabelText.clear();

	// Build summary from checked menu items
	for(size_t i = 0; i < mFlags.size(); ++i)
	{
		const Fl_Menu_Item &item = menu()[i];
		if(item.value())
			mLabelText += mFlags[i]->inCategoryAcronym;
	}

	// Update button label
	if(mLabelText.empty())
		copy_label(kDefaultLabel);
	else
		copy_label(mLabelText.c_str());

	redraw();
}

void UI_ActivationButton::menuCallback(Fl_Widget *w, void *data)
{
	auto button = static_cast<UI_ActivationButton *>(data);

	// Find which menu item was toggled
	int index = button->value();
	if(index < 0 || index >= static_cast<int>(button->mFlags.size()))
		return;

	const lineflag_t *flag = button->mFlags[index];
	const Fl_Menu_Item &item = button->menu()[index];
	int isChecked = item.value() ? 1 : 0;

	if(button->mCallback)
		button->mCallback(flag, isChecked);

	button->updateLabel();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
