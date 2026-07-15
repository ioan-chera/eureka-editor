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

#include "ui_args.h"

#include "LineDef.h"
#include "m_game.h"
#include "sys_macro.h"
#include "Thing.h"
#include "ui_misc.h"
#include "ui_panelinput.h"

#include "FL/fl_draw.H"
#include "FL/Fl_Menu_Button.H"

enum
{
	ARG_DEFAULT_LABEL_SIZE = 12,
	ARG_SHRUNKEN_LABEL_SIZE = 10,
};

UI_ArgField::UI_ArgField(int X, int Y, int W, int H) : Fl_Group(X, Y, W, H)
{
	input = new UI_DynIntInput(X, Y, W, H);
	input->callback([](Fl_Widget *widget, void *userData)
	{
		auto field = static_cast<UI_ArgField *>(userData);
		field->do_callback(FL_REASON_CHANGED);
	}, this);
	input->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	const int width = 3 * TYPE_INPUT_HEIGHT / 4;
	button = new Fl_Menu_Button(X + W - width, Y, width, H);
}

UI_ArgsBox::UI_ArgsBox(int X, int Y) : Fl_Flex(X, Y, PANEL_WIDTH - 2 * NOMBRE_INSET -
											   2 * PANEL_INSET,
												(fl_font(FL_HELVETICA, ARG_DEFAULT_LABEL_SIZE),
												 TYPE_INPUT_HEIGHT + fl_height()),
											   Fl_Flex::HORIZONTAL)
{
	margin(0, 0, 0, h() - TYPE_INPUT_HEIGHT);
	gap(INPUT_SPACING);
	for(size_t i = 0; i < lengthof(args); ++i)
	{
		args[i] = new UI_DynIntInput(0, 0, 0, 0);
		args[i]->callback(argsCallback, this);
		args[i]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		args[i]->align(FL_ALIGN_BOTTOM);
		args[i]->labelsize(ARG_DEFAULT_LABEL_SIZE);
	}
	end();
}

UI_ArgsBox::~UI_ArgsBox()
{
	for(UI_DynIntInput *input : args)
		if(!input->parent())
			delete input;
}

void UI_ArgsBox::loadFields(PanelFieldFixUp &fixUp) const
{
	for(UI_DynIntInput *input : args)
		fixUp.loadField(input);
}

void UI_ArgsBox::trackLabels()
{
	for(size_t i = 0; i < lengthof(args); ++i)
		argLabels[i] = args[i]->label();
}

void UI_ArgsBox::clear(PanelFieldFixUp &fixUp)
{
	for(UI_DynIntInput *input : args)
	{
		fixUp.setInputValue(input, "");
		input->label("");
		input->textcolor(FL_BLACK);
	}
}

void UI_ArgsBox::populate(PanelFieldFixUp &fixUp, const ConfigData &config, const LineDef &linedef)
{
	const linetype_t &info = config.getLineType(linedef.type);
	for(size_t i = 0; i < lengthof(args); ++i)
	{
		int argVal = linedef.Arg(static_cast<int>(i + 1));
		if(argVal || linedef.type)
			fixUp.setInputValue(args[i], SString(argVal).c_str());
		if(info.args[i].name.empty())
		{
			args[i]->label("");
			args[i]->textcolor(fl_rgb_color(160, 160, 160));
		}
		else
			setLabel((int)i, info.args[i].name);
	}
}

void UI_ArgsBox::populate(PanelFieldFixUp &fixUp, const ConfigData &config, const Thing &thing)
{
	const thingtype_t &info = config.getThingType(thing.type);
	const linetype_t &spec = config.getLineType(thing.special);
	for(size_t i = 0; i < lengthof(args); ++i)
	{
		int argVal = thing.Arg(1 + static_cast<int>(i));
		if(thing.special)
		{
			fixUp.setInputValue(args[i], SString(argVal).c_str());
			if(spec.args[i].name.empty())
			{
				args[i]->label("");
				args[i]->textcolor(fl_rgb_color(160, 160, 160));
			}
			else
				setLabel((int)i, spec.args[i].name);
		}
		else
		{
			// spawn args
			if(argVal || !info.args[i].empty())
				fixUp.setInputValue(args[i], SString(argVal).c_str());
			if(info.args[i].empty())
			{
				args[i]->label("");
				args[i]->textcolor(fl_rgb_color(160, 160, 160));
			}
			else
				setLabel((int)i, info.args[i]);
		}
	}
}

bool UI_ArgsBox::labelsChanged() const
{
	for(size_t i = 0; i < lengthof(args); ++i)
		if(argLabels[i] != args[i]->label())
			return true;
	return false;
}

void UI_ArgsBox::argsCallback(Fl_Widget *widget, void *context)
{
	auto self = static_cast<UI_ArgsBox *>(context);
	if(!self->callbackFunction)
		return;
	for(size_t i = 0; i < lengthof(args); ++i)
	{
		if(self->args[i] != widget)
			continue;
		self->callbackFunction(static_cast<int>(i), atoi(self->args[i]->value()));
		break;
	}
}

void UI_ArgsBox::setLabel(int index, const SString &text)
{
	SString argName = text;
	for(char &c : argName)
		if(c == '_')
			c = ' ';
	UI_DynIntInput *input = args[index];
	fl_font(FL_HELVETICA, ARG_DEFAULT_LABEL_SIZE);
	if(fl_width(argName.c_str()) > input->w())
		input->labelsize(ARG_SHRUNKEN_LABEL_SIZE);
	else
		input->labelsize(ARG_DEFAULT_LABEL_SIZE);
	input->copy_label(argName.c_str());
}
