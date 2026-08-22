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

#include "Document.h"
#include "lib_util.h"
#include "LineDef.h"
#include "m_game.h"
#include "sys_macro.h"
#include "Thing.h"
#include "ui_misc.h"
#include "ui_panelinput.h"

#include "FL/Fl.H"
#include "FL/fl_draw.H"
#include "FL/Fl_Light_Button.H"
#include "FL/Fl_Menu_Button.H"

#include <set>

enum
{
	ARG_DEFAULT_LABEL_SIZE = 12,
	ARG_SHRUNKEN_LABEL_SIZE = 10,

	DROP_DOWN_BUTTON_WIDTH = 20,
};

class ArgMenuButton : public Fl_Menu_Button
{
public:
	int handle(int event) override;
	ArgMenuButton(const Fl_Group &parent, int X, int Y, int W, int H, const char *l = 0) :
	Fl_Menu_Button(X, Y, W, H, l), parent(parent)
	{
	}
	const Fl_Menu_Item* popup();	// replace non-virtual method

private:
	const Fl_Group &parent;
};

// Same as Fl_Menu_Button's handle, but with adjusted popup position
int ArgMenuButton::handle(int event)
{
	if(!menu() || !menu()->text)
		return 0;
	switch(event)
	{
		// These events need to use custom popup stuff
		case FL_PUSH:
			if (!box()) {
			  if (Fl::event_button() != 3) return 0;
			} else if (type()) {
			  if (!(type() & (1 << (Fl::event_button()-1)))) return 0;
			}
			if (Fl::visible_focus()) Fl::focus(this);
			popup();
			return 1;
		case FL_KEYBOARD:
		  if (!box()) return 0;
		  if (Fl::event_key() == ' ' &&
			  !(Fl::event_state() & (FL_SHIFT | FL_CTRL | FL_ALT | FL_META))) {
			  popup();
			return 1;
		  } else return 0;
		case FL_SHORTCUT:
		  if (Fl_Widget::test_shortcut()) {popup(); return 1;}
		  return test_shortcut() != 0;
		default:
			return Fl_Menu_Button::handle(event);
	}
}

const Fl_Menu_Item* ArgMenuButton::popup() {
  menu_end();
  const Fl_Menu_Item* m;
  pressed_menu_button_ = this;
  redraw();
  Fl_Widget_Tracker mb(this);
  if (!box() || type()) {
	m = menu()->popup(Fl::event_x(), Fl::event_y(), label(), mvalue(), this);
  } else {
//	Fl_Window_Driver::current_menu_button = this;
	m = menu()->pulldown(parent.x(), parent.y(), parent.w(), parent.h(), 0, this);
//	Fl_Window_Driver::current_menu_button = NULL;
  }
  picked(m);
  pressed_menu_button_ = 0;
  if (mb.exists()) redraw();
  return m;
}


UI_ArgField::UI_ArgField(int X, int Y, int W, int H, PanelFieldFixUp &fixUp) : Fl_Group(X, Y, W, H), fixUp(fixUp)
{
	input = new Input(X, Y, W, H);
	input->callback([](Fl_Widget *widget, void *userData)
	{
		auto field = static_cast<UI_ArgField *>(userData);
		field->do_callback(FL_REASON_CHANGED);
	}, this);
	input->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

	button = new ArgMenuButton(*this, X + W - DROP_DOWN_BUTTON_WIDTH, Y, DROP_DOWN_BUTTON_WIDTH, H);
	button->hide();

	toggleButton = new Fl_Light_Button(X + W - DROP_DOWN_BUTTON_WIDTH, Y, DROP_DOWN_BUTTON_WIDTH, H);
	toggleButton->selection_color(FL_GREEN);
	toggleButton->hide();
	toggleButton->callback([](Fl_Widget *widget, void *userData)
	{
		auto field = static_cast<UI_ArgField *>(userData);
		field->fixUp.setInputValue(field->input, SString((int)field->toggleButton->value()).c_str());
		field->input->do_callback();
	}, this);

	end();

	resizable(nullptr);
	input->setButton(button);
}

void UI_ArgField::resize(int X, int Y, int W, int H)
{
	Fl_Group::resize(X, Y, W, H);
	if(!argType.empty())
	{
		input->resize(X, Y, W - DROP_DOWN_BUTTON_WIDTH, H);
		button->resize(X + W - DROP_DOWN_BUTTON_WIDTH, Y, DROP_DOWN_BUTTON_WIDTH, H);
		toggleButton->resize(X + W - DROP_DOWN_BUTTON_WIDTH, Y, DROP_DOWN_BUTTON_WIDTH, H);
	}
	else
		input->resize(X, Y, W, H);
}

void UI_ArgField::updateOptions()
{
	toggleButton->hide();
	if(argType.empty())
	{
		button->hide();
		input->resize(x(), y(), w(), h());
		return;
	}
	button->show();
	input->resize(x(), y(), w() - DROP_DOWN_BUTTON_WIDTH, h());
	button->resize(x() + w() - DROP_DOWN_BUTTON_WIDTH, y(), DROP_DOWN_BUTTON_WIDTH, h());
	button->clear();
	buttonItems.clear();
	int curval = atoi(input->value());
	int valwithoutflag = curval;
	for(const ArgType::Entry &entry : argType.flags)
	{
		valwithoutflag &= ~entry.number;
	}
	for(size_t i = 0; i < argType.options.size(); ++i)
	{
		int flags = FL_MENU_RADIO;
		if(i + 1 == argType.options.size() && !argType.flags.empty())
			flags |= FL_MENU_DIVIDER;
		if(valwithoutflag == argType.options[i].number)
			flags |= FL_MENU_VALUE;

		buttonItems.push_back({ .text = argType.options[i].name.c_str(), .flags = flags });
		buttonItems.back().callback(optionCallback, this);
	}
	for(const ArgType::Entry &entry : argType.flags)
	{
		int flags = FL_MENU_TOGGLE;

		if(curval & entry.number)
			flags |= FL_MENU_VALUE;
		buttonItems.push_back({ .text = entry.name.c_str(), .flags = flags });
		buttonItems.back().callback(optionCallback, this);
	}
	buttonItems.push_back({});
	button->menu(buttonItems.data());
}

void UI_ArgField::makeTagList(const std::set<int> &tags)
{
	argType.options.clear();
	argType.flags.clear();
	argType.options.reserve(tags.size());
	for(int tag : tags)
		argType.options.push_back({.number = tag, .name = SString(tag)});
	updateOptions();
}

void UI_ArgField::loadToFixUp()
{
	fixUp.loadField(input);
}

void UI_ArgField::textcolor(Fl_Color n)
{
	input->textcolor(n);
}

void UI_ArgField::setInputValue(const char *value)
{
	fixUp.setInputValue(input, value);
}

void UI_ArgField::setAsTag(const Document &doc)
{
	std::set<int> tags;
	for(const auto &sector : doc.sectors)
		tags.insert(sector->tag);
	makeTagList(tags);
}

void UI_ArgField::setAsLineID(const Document &doc, const ConfigData &config)
{
	std::set<int> lineids;
	for(int i = 0; i < doc.numLinedefs(); ++i)
	{
		const LineDef &line = *doc.linedefs[i];
		SpecialTagInfo linfo;
		int lineid = 0;
		if(getSpecialTagInfo(ObjType::linedefs, i, line.type, &line, config, linfo) &&
		   linfo.selflineid >= 1)
		{
			lineid = linfo.selflineid;
		}
		else if(line.lineid >= 1)
			lineid = line.lineid;
		lineids.insert(lineid);
	}
	makeTagList(lineids);
}

void UI_ArgField::setAsPolyobject(const Document &doc, const ConfigData &config)
{
	std::set<int> polyobjects;
	for(const auto &thing : doc.things)
	{
		const thingtype_t *type = get(config.thing_types, thing->type);
		if(!type || !(type->flags & THINGDEF_POLYSPOT))
			continue;
		polyobjects.insert(thing->angle);
	}
	makeTagList(polyobjects);
}

void UI_ArgField::setAsTID(const Document &doc)
{
	std::set<int> tids;
	for(const auto &thing : doc.things)
		tids.insert(thing->tid);
	makeTagList(tids);
}

void UI_ArgField::setAsBoolean()
{
	button->hide();
	input->resize(x(), y(), w() - DROP_DOWN_BUTTON_WIDTH, h());
	toggleButton->show();
	toggleButton->resize(x() + w() - DROP_DOWN_BUTTON_WIDTH, y(), DROP_DOWN_BUTTON_WIDTH, h());
}

void UI_ArgField::setAsGeneric()
{
	argType.options.clear();
	argType.flags.clear();
	updateOptions();
}

void UI_ArgField::setAsCustom(const ArgType &type)
{
	argType = type;
	updateOptions();
}

int UI_ArgField::value() const
{
	return atoi(input->value());
}

void UI_ArgField::updateFlags()
{
	int val = value();
	toggleButton->value(val == 1);
	if(argType.flags.empty() && argType.options.empty())
		return;
	int valwithoutflag = val;
	for(const ArgType::Entry &entry : argType.flags)
	{
		valwithoutflag &= ~entry.number;
	}
	for(size_t i = 0; i < argType.options.size(); ++i)
	{
		if(valwithoutflag == argType.options[i].number)
		{
			buttonItems[i].flags |= FL_MENU_VALUE;
			break;
		}
	}
	for(size_t i = 0; i < argType.flags.size(); ++i)
	{
		if(!(val & argType.flags[i].number))
			buttonItems[i + argType.options.size()].flags &= ~FL_MENU_VALUE;
		else
			buttonItems[i + argType.options.size()].flags |= FL_MENU_VALUE;
	}
}

int UI_ArgField::Input::handle(int event)
{
	if(event == FL_KEYDOWN && Fl::event_key() == FL_Down && button && button->visible() && button->size() >= 1)
	{
		button->popup();
		return 1;
	}
	return UI_DynIntInput::handle(event);
}

void UI_ArgField::optionCallback(Fl_Widget *widget, void *context)
{
	auto field = static_cast<UI_ArgField *>(context);
	auto menu = static_cast<Fl_Menu_ *>(widget);
	int index = menu->value();
	const ArgType &argType = field->argType;
	int flagmask = 0;
	if(!argType.flags.empty())
	{
		for(const ArgType::Entry &entry : argType.flags)
			flagmask |= entry.number;
	}
	bool doPopup = false;
	if(index >= 0 && index < (int)argType.options.size())
	{
		int curval = atoi(field->input->value());
		field->fixUp.setInputValue(field->input, SString((curval & flagmask) | argType.options[index].number).c_str());
	}
	else if(index >= 0)
	{
		int flagIndex = index - (int)argType.options.size();
		if(flagIndex < (int)argType.flags.size())
		{
			int curval = atoi(field->input->value());
			doPopup = true;
			if(field->buttonItems[index].flags & FL_MENU_VALUE)
				field->fixUp.setInputValue(field->input, SString(curval | argType.flags[flagIndex].number).c_str());
			else
				field->fixUp.setInputValue(field->input, SString(curval & ~argType.flags[flagIndex].number).c_str());
		}
	}
	field->do_callback();
	if(doPopup)
		field->button->popup();
}

UI_ArgsBox::UI_ArgsBox(int X, int Y, PanelFieldFixUp &fixUp, const Document &doc) :
		Fl_Flex(X, Y, PANEL_WIDTH - 2 * NOMBRE_INSET - 2 * PANEL_INSET,
				(fl_font(FL_HELVETICA, ARG_DEFAULT_LABEL_SIZE), TYPE_INPUT_HEIGHT + fl_height()),
				Fl_Flex::HORIZONTAL), doc(doc)
{
	margin(0, 0, 0, h() - TYPE_INPUT_HEIGHT);
	gap(INPUT_SPACING);
	for(size_t i = 0; i < lengthof(args); ++i)
	{
		args[i] = new UI_ArgField(0, 0, 0, 0, fixUp);
		args[i]->callback(argsCallback, this);
		args[i]->align(FL_ALIGN_BOTTOM);
		args[i]->labelsize(ARG_DEFAULT_LABEL_SIZE);
	}
	end();
}

UI_ArgsBox::~UI_ArgsBox()
{
	for(UI_ArgField *input : args)
		if(!input->parent())
			delete input;
}

void UI_ArgsBox::loadToFixUp() const
{
	for(UI_ArgField *input : args)
		input->loadToFixUp();
}

void UI_ArgsBox::trackLabels()
{
	for(size_t i = 0; i < lengthof(args); ++i)
		argLabels[i] = args[i]->label();
}

void UI_ArgsBox::clear(PanelFieldFixUp &fixUp)
{
	for(UI_ArgField *input : args)
	{
		input->setInputValue("");
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
			args[i]->setInputValue(SString(argVal).c_str());
		if(info.args[i].name.empty())
		{
			args[i]->label("");
			args[i]->textcolor(fl_rgb_color(160, 160, 160));
			args[i]->setAsGeneric();
		}
		else
		{
			setLabel((int)i, info.args[i].name, info.args[i].type, info.args[i].customTypeName,
					config);
		}
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
			args[i]->setInputValue(SString(argVal).c_str());
			if(spec.args[i].name.empty())
			{
				args[i]->label("");
				args[i]->textcolor(fl_rgb_color(160, 160, 160));
				args[i]->setAsGeneric();
			}
			else
			{
				setLabel((int)i, spec.args[i].name, spec.args[i].type, spec.args[i].customTypeName,
						config);
			}
		}
		else
		{
			// spawn args
			if(argVal || !info.args[i].empty())
				args[i]->setInputValue(SString(argVal).c_str());
			if(info.args[i].empty())
			{
				args[i]->label("");
				args[i]->textcolor(fl_rgb_color(160, 160, 160));
				args[i]->setAsGeneric();
			}
			else
				setLabel((int)i, info.args[i], SpecialArgType::generic, "", config);
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
		self->args[i]->updateFlags();
		self->callbackFunction(static_cast<int>(i), self->args[i]->value());
		break;
	}
}

void UI_ArgsBox::setLabel(int index, const SString &text, SpecialArgType type,
	const SString &customName, const ConfigData &config)
{
	SString argName = text;
	for(char &c : argName)
		if(c == '_')
			c = ' ';
	UI_ArgField *input = args[index];
	fl_font(FL_HELVETICA, ARG_DEFAULT_LABEL_SIZE);
	if(fl_width(argName.c_str()) > input->w())
		input->labelsize(ARG_SHRUNKEN_LABEL_SIZE);
	else
		input->labelsize(ARG_DEFAULT_LABEL_SIZE);
	input->copy_label(argName.c_str());

	switch(type)
	{
		case SpecialArgType::tag:
			input->setAsTag(doc);
			break;
		case SpecialArgType::line_id:
			input->setAsLineID(doc, config);
			break;
		case SpecialArgType::tid:
			input->setAsTID(doc);
			break;
		case SpecialArgType::po:
			input->setAsPolyobject(doc, config);
			break;
		case SpecialArgType::boolean:
			input->setAsBoolean();
			break;
		case SpecialArgType::custom:
		{
			auto it = config.argument_types.find(customName);
			if(it == config.argument_types.end())
				input->setAsGeneric();
			else
				input->setAsCustom(it->second);
			break;
		}
		default:
			input->setAsGeneric();
			break;
	}
}
