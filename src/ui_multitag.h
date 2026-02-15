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

#ifndef __EUREKA_UI_IDTAG_H__
#define __EUREKA_UI_IDTAG_H__

#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <functional>
#include <set>
#include <vector>

class Fl_Button;
class Fl_Int_Input;
class Instance;

//
// Individual ID for moreIDs, with a "remove" button
//
class IDTag : public Fl_Widget
{
public:
	static constexpr int RADIUS = 4;
	static constexpr int PADDING = 6;
	static constexpr int X_BUTTON_SIZE = 14;
	static constexpr int X_BUTTON_MARGIN = 4;
	static constexpr int X_OFFSET = 3;
	static constexpr int MIN_WIDTH = 40;

	IDTag(int X, int Y, int W, int H, const char *label = nullptr);

	void callback(Fl_Callback *cb, void *data = nullptr);

	// Calculate the required width for a given label
	static int calcRequiredWidth(const char *labelText, int labelSize = 12);

	int handle(int event) override;
	void draw() override;

private:
	Fl_Callback *mCallback;
	void *mCallbackData;
	bool mXPressed = false;
	bool mXHovered = false;

	bool isInXButton(int mx, int my) const;
	void drawRoundedRectOutline(int X, int Y, int W, int H, int radius);
};

//
// Widget for managing multiple tag IDs with add/remove functionality
//
class MultiTagView : public Fl_Group
{
public:
	MultiTagView(Instance &inst, std::function<void()> redrawCallback,
				 std::function<void()> dataCallback, int x, int y, int width, int height,
				 const char *label = nullptr);

	void setTags(std::set<int> tags);
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

#endif  // __EUREKA_UI_IDTAG_H__
