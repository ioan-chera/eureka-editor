//------------------------------------------------------------------------
//  Vertical stack panel widget
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

#ifndef __EUREKA_UI_STACKPANEL_H__
#define __EUREKA_UI_STACKPANEL_H__

#include <map>
#include <FL/Fl_Group.H>

//
// A vertical stack panel widget, similar to Android LinearLayout or Windows StackPanel.
// Children are laid out vertically from top to bottom with configurable margins and spacing.
//
class UI_StackPanel : public Fl_Group
{
public:
	UI_StackPanel(int X, int Y, int W, int H, const char *label = nullptr) 	: Fl_Group(X, Y, W, H, label)
	{
		resizable(nullptr);
	}

	void draw() override;

	// Margin accessors (left, top, right, bottom)
	void margin(int *left, int *top, int *right, int *bottom) const;
	void margin(int left, int top, int right, int bottom);

	// General vertical spacing between widgets
	int spacing() const
	{
		return mSpacing;
	}
	void spacing(int value)
	{
		mSpacing = value;
	}

	// Per-widget extra vertical spacing
	int beforeSpacing(const Fl_Widget *widget) const;
	void beforeSpacing(const Fl_Widget *widget, int value);
	int afterSpacing(const Fl_Widget *widget) const;
	void afterSpacing(const Fl_Widget *widget, int value);

	// Trigger a relayout of children
	void relayout();

protected:
	// Called when a child widget is about to be removed
	void on_remove(int index) override;

private:
	int mMarginLeft = 0;
	int mMarginTop = 0;
	int mMarginRight = 0;
	int mMarginBottom = 0;
	int mSpacing = 0;
	std::map<const Fl_Widget *, int> mBeforeSpacing;
	std::map<const Fl_Widget *, int> mAfterSpacing;
};

#endif
