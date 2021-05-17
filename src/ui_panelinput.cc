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

#include "ui_panelinput.h"

//
// Check all dirty flags and calls the callbacks. Meant to be called before
// any non-edit action!
//
void PanelFieldFixUp::checkDirtyFields()
{
	while(!mDirtyFields.empty())
	{
		Fl_Input *input = *mDirtyFields.begin();
		input->callback()(input, mOwner);
	}
}

//
// Convenience that also sets the dirty field. Only use it for targeted fields
//
void PanelFieldFixUp::setInputValue(Fl_Input *input, const char *value)
{
	input->value(value);
	mDirtyFields.erase(input);
}

//
// Just to mark a flag dirty. Data is a boolean field
//
void PanelFieldFixUp::dirtify_callback(Fl_Widget *w, void *data)
{
	auto fixup = static_cast<PanelFieldFixUp *>(data);
	fixup->mDirtyFields.insert(static_cast<Fl_Input *>(w));
}
