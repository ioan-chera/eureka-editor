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

#ifndef __EUREKA_UI_ACTIVATION_BUTTON_H__
#define __EUREKA_UI_ACTIVATION_BUTTON_H__

#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Grid.H>
#include <functional>
#include <memory>
#include <vector>
#include "m_strings.h"

struct lineflag_t;

//
// UI_ActivationPopup
//
// A popup menu window containing a grid of activation flag checkboxes
//
class UI_ActivationPopup : public Fl_Menu_Window
{
public:
	UI_ActivationPopup(int W, int H);

	int handle(int event) override;

	void setFlags(const std::vector<const lineflag_t *> &flags);
	void setCallback(const std::function<void(const lineflag_t *, int)> &cb);

	void updateFromFlags(int flags, int flags2);

	// Get summary string of currently checked flags
	SString getSummary() const;

private:
	struct FlagCheckbox
	{
		Fl_Check_Button *button;
		const lineflag_t *info;
	};

	std::unique_ptr<Fl_Grid> mGrid;
	std::vector<FlagCheckbox> mCheckboxes;
	std::function<void(const lineflag_t *, int)> mCallback;

	static void checkboxCallback(Fl_Widget *w, void *data);
};

//
// UI_ActivationButton
//
// A button that shows a popup menu window with activation flags when clicked
//
class UI_ActivationButton : public Fl_Button
{
public:
	UI_ActivationButton(int X, int Y, int W, int H, const char *label = nullptr);

	int handle(int event) override;

	void setFlags(const std::vector<const lineflag_t *> &flags);
	void setCallback(const std::function<void(const lineflag_t *, int)> &cb);

	void updateFromFlags(int flags, int flags2);

	// Compute and update the summary label based on current checkbox states
	void updateSummaryLabel();

private:
	void showPopup();
	void hidePopup();

	std::unique_ptr<UI_ActivationPopup> mPopup;
	std::vector<const lineflag_t *> mFlags;
	std::function<void(const lineflag_t *, int)> mCallback;
	SString mSummary;
};

#endif  /* __EUREKA_UI_ACTIVATION_BUTTON_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
