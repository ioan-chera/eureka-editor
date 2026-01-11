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
#include "ui_nombre.h"

//
// Given a list of fields, assigns secondary callbacks
//
void PanelFieldFixUp::loadFields(std::initializer_list<ICallback2 *> fields)
{
	for(ICallback2 *field : fields)
	{
		if(field->getMainCallback())
			mOriginalCallbacks[field] = { field->getMainCallback(), field->getMainUserData() };
		field->setMainCallback(clearDirtyCallback, this);
		if(field->callback2())
			mOriginalCallbacks2[field] = { field->callback2(), field->user_data2() };
		field->callback2(setDirtyCallback, this);

		mAsCallback2[field->asWidget()] = field;
		mAsWidget[field] = field->asWidget();
	}
}

//
// Removes fields from tracking. Call this before deleting dynamically created widgets.
//
void PanelFieldFixUp::unloadFields(std::initializer_list<ICallback2 *> fields)
{
	for(ICallback2 *field : fields)
	{
		mOriginalCallbacks.erase(field);
		mOriginalCallbacks2.erase(field);
		mAsCallback2.erase(field->asWidget());
		mAsWidget.erase(field);
		mDirtyFields.erase(field);
	}
}

//
// Check all dirty flags and calls the callbacks. Meant to be called before
// any non-edit action!
//
void PanelFieldFixUp::checkDirtyFields()
{
	while(!mDirtyFields.empty())
	{
		ICallback2 *input = *mDirtyFields.begin();
		input->getMainCallback()(mAsWidget[input], this);
	}
}

//
// Convenience that also sets the dirty field. Only use it for targeted fields
//
void PanelFieldFixUp::setInputValue(ICallback2 *input, const char *value) noexcept
{
	input->setValue(value);
	mDirtyFields.erase(input);
}

//
// Set-callback for this
//
void PanelFieldFixUp::setDirtyCallback(Fl_Widget *widget, void *data)
{
	auto fixup = static_cast<PanelFieldFixUp *>(data);
	auto control = fixup->mAsCallback2[widget];
	fixup->mDirtyFields.insert(control);

	// Now also call its original one
	auto it = fixup->mOriginalCallbacks2.find(control);
	if(it != fixup->mOriginalCallbacks2.end())
		it->second.callback(widget, it->second.data);
}

//
// Clear-callback for this
//
void PanelFieldFixUp::clearDirtyCallback(Fl_Widget *widget, void *data)
{
	auto fixup = static_cast<PanelFieldFixUp *>(data);
	auto control = fixup->mAsCallback2[widget];
	fixup->mDirtyFields.erase(control);

	// Now also call its original one
	auto it = fixup->mOriginalCallbacks.find(control);
	if(it != fixup->mOriginalCallbacks.end())
		it->second.callback(widget, it->second.data);
}

MapItemBox::MapItemBox(Instance &inst, int X, int Y, int W, int H, const char *label) : Fl_Scroll(X, Y, W, H, label), inst(inst)
{
	type(Fl_Scroll::VERTICAL);
	scrollbar_size(Fl::scrollbar_size() * 3 / 4);
}

void MapItemBox::SetObj(int _index, int _count)
{
	if (obj == _index && count == _count)
		return;

	obj   = _index;
	count = _count;

	which->SetIndex(obj);
	which->SetSelected(count);

	UpdateField();

	if (obj < 0)
		UnselectPics();

	redraw();
}
