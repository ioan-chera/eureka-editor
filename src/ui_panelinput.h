//------------------------------------------------------------------------
//  Panel input fixing of problems
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __EUREKA_UI_PANELINPUT_H__
#define __EUREKA_UI_PANELINPUT_H__

#include "FL/Fl_Input.H"
#include <unordered_set>

//
// Fix-up class for panel fields to make sure they work as expected when clicking buttons
//
class PanelFieldFixUp
{
public:
	explicit PanelFieldFixUp(void *ownerContext) : mOwner(ownerContext)
	{
	}

	// Call it before starting basis
	void checkDirtyFields();
	void setInputValue(Fl_Input *input, const char *value);

	void clear(Fl_Input *input)
	{
		mDirtyFields.erase(input);
	}

	static void dirtify_callback(Fl_Widget *, void *);

private:
	// Set of "dirty" edit fields. The rule is as such:
	// - Add them to list when user writes text without exiting or pressing ENTER
	//   (that's why they need to be UI_DynInput or UI_DynIntInput)
	// - Remove them from list when their callback is invoked (caution: only for them)
	// - Remove them from list when their value() is changed externally.
	// - Before any basis.begin() or external function which does that, call checkDirtyFields()
	std::unordered_set<Fl_Input *> mDirtyFields;
	void *const mOwner;	// who's the user data for the callbacks
};

#endif
