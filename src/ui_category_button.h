//------------------------------------------------------------------------
//  COLLAPSIBLE CATEGORY BUTTON
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025 Ioan Chera
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

#ifndef __EUREKA_UI_CATEGORY_BUTTON_H__
#define __EUREKA_UI_CATEGORY_BUTTON_H__

#include <FL/Fl_Button.H>

//
// UI_CategoryButton
//
// A clickable header that shows/hides a group of controls with an arrow indicator
//
class UI_CategoryButton : public Fl_Button
{
public:
	UI_CategoryButton(int X, int Y, int W, int H, const char *label = nullptr);

	bool isExpanded() const { return mExpanded; }
	void setExpanded(bool expanded);

	void draw() override;
	int handle(int event) override;

private:
	bool mExpanded = true;
};

#endif  /* __EUREKA_UI_CATEGORY_BUTTON_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
