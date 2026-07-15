//------------------------------------------------------------------------
//  Linedef and thing argument widget
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

#ifndef __EUREKA_UI_ARGS_H__
#define __EUREKA_UI_ARGS_H__

#include "m_strings.h"

#include "FL/Fl_Flex.H"

#include <functional>

class LineDef;
class PanelFieldFixUp;
class UI_DynIntInput;
struct ConfigData;
struct Thing;

class UI_ArgsBox : public Fl_Flex
{
public:
	using Callback = std::function<void(int index, int value)>;

	UI_ArgsBox(int X, int Y);
	~UI_ArgsBox();

	void loadFields(PanelFieldFixUp &fixUp) const;

	void trackLabels();
	void clear(PanelFieldFixUp &fixUp);
	void populate(PanelFieldFixUp &fixUp, const ConfigData &config, const LineDef &linedef);
	void populate(PanelFieldFixUp &fixUp, const ConfigData &config, const Thing &thing);
	bool labelsChanged() const;

	void setCallbackFunction(Callback callback)
	{
		callbackFunction = std::move(callback);
	}

private:
	static void argsCallback(Fl_Widget *widget, void *context);
	void setLabel(int index, const SString &text);

	UI_DynIntInput *args[5];

	SString argLabels[5];

	Callback callbackFunction;
};

#endif
