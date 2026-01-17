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

#include <FL/Fl_Menu_Button.H>
#include <functional>
#include <vector>
#include "m_strings.h"

struct lineflag_t;

//
// UI_ActivationButton
//
// A menu button that shows checkable activation flags when clicked
//
class UI_ActivationButton : public Fl_Menu_Button
{
public:
	UI_ActivationButton(int X, int Y, int W, int H, const char *label = nullptr);

	void setFlags(const std::vector<const lineflag_t *> &flags);
	void setCallback(const std::function<void(const lineflag_t *, int)> &cb);

	void updateFromFlags(int flags, int flags2);

private:
	void updateLabel();

	static void menuCallback(Fl_Widget *w, void *data);

	std::vector<const lineflag_t *> mFlags;
	std::function<void(const lineflag_t *, int)> mCallback;
	SString mLabelText;
};

#endif  /* __EUREKA_UI_ACTIVATION_BUTTON_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
