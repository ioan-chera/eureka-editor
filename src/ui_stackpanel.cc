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

#include "ui_stackpanel.h"

#include <algorithm>

//
// Draw override - triggers relayout before drawing
//
void UI_StackPanel::draw()
{
	relayout();
	Fl_Group::draw();
}

//
// Get margins
//
void UI_StackPanel::margin(int *left, int *top, int *right, int *bottom) const
{
	if (left)
		*left = mMarginLeft;
	if (top)
		*top = mMarginTop;
	if (right)
		*right = mMarginRight;
	if (bottom)
		*bottom = mMarginBottom;
}

//
// Set margins
//
void UI_StackPanel::margin(int left, int top, int right, int bottom)
{
	mMarginLeft = left;
	mMarginTop = top;
	mMarginRight = right;
	mMarginBottom = bottom;
}


//
// Get extra vertical spacing for a specific widget
//
int UI_StackPanel::beforeSpacing(const Fl_Widget *widget) const
{
	auto it = mBeforeSpacing.find(widget);
	if (it != mBeforeSpacing.end())
		return it->second;
	return 0;
}

//
// Set extra vertical spacing for a specific widget
//
void UI_StackPanel::beforeSpacing(const Fl_Widget *widget, int value)
{
	if (value == 0)
		mBeforeSpacing.erase(widget);
	else
		mBeforeSpacing[widget] = value;
}

//
// Get extra vertical spacing for a specific widget
//
int UI_StackPanel::afterSpacing(const Fl_Widget *widget) const
{
	auto it = mAfterSpacing.find(widget);
	if (it != mAfterSpacing.end())
		return it->second;
	return 0;
}

//
// Set extra vertical spacing for a specific widget
//
void UI_StackPanel::afterSpacing(const Fl_Widget *widget, int value)
{
	if (value == 0)
		mAfterSpacing.erase(widget);
	else
		mAfterSpacing[widget] = value;
}

//
// Called when a child widget is about to be removed
//
void UI_StackPanel::on_remove(int index)
{
	Fl_Widget *widget = child(index);
	if (widget)
	{
		mBeforeSpacing.erase(widget);
		mAfterSpacing.erase(widget);
	}
}

//
// Relayout all children vertically
//
void UI_StackPanel::relayout()
{
	int currentY = y() + mMarginTop;
	int maxRight = x() + mMarginLeft;

	for (int i = 0; i < children(); ++i)
	{
		Fl_Widget *widget = child(i);
		if(!widget->visible())
			continue;

		// Apply extra spacing for this widget (above it)
		currentY += beforeSpacing(widget);

		// Position the widget
		widget->position(widget->x(), currentY);

		// Track the rightmost edge
		int widgetRight = widget->x() + widget->w();
		if (widgetRight > maxRight)
			maxRight = widgetRight;

		// Move Y down for the next widget
		currentY += widget->h() + afterSpacing(widget);

		// Add general spacing if there's a next widget
		if (i < children() - 1)
			currentY += mSpacing;
	}

	// Update group dimensions to fit all children plus margins
	int newHeight = currentY - y() + mMarginBottom;
	int newWidth = maxRight - x() + mMarginRight;

	size(newWidth, newHeight);
}
