//------------------------------------------------------------------------
//  DIALOG when all fucked up
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2013 Andrew Apted
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

#include "Errors.h"

#include "main.h"
#include "ui_window.h"


#define BUT_W  100
#define BUT_H  26

#define ICON_W  40
#define ICON_H  40

#define FONT_SIZE  16

//
// Dialog box callback context
//
struct DialogContext
{
	int result;
	std::vector<Fl_Widget *> buttons;
};

static void dialog_close_callback(Fl_Widget *w, void *data)
{
	auto context = static_cast<DialogContext *>(data);
	context->result = 0;
}

static void dialog_button_callback(Fl_Widget *w, void *data)
{
	auto context = static_cast<DialogContext *>(data);
	auto it = std::find(context->buttons.begin(), context->buttons.end(), w);
	SYS_ASSERT(it != context->buttons.end());
	context->result = static_cast<int>(it - context->buttons.begin());
}

//
// Message box icon
//
enum class MessageBoxIcon
{
	information,
	exclamation,
	question
};

static int DialogShowAndRun(
	MessageBoxIcon icon_type, 
	const SString &message, 
	const SString &title, 
	const SString &link_title = NULL, 
	const SString &link_url = NULL,
	const std::vector<SString> *labels = NULL)
{
	DialogContext context = {};
	context.result = -1;

	// determine required size
	int mesg_W = 480;  // NOTE: fl_measure will wrap to this!
	int mesg_H = 0;

	fl_font(FL_HELVETICA, FONT_SIZE);
	fl_measure(message.c_str(), mesg_W, mesg_H);

	if (mesg_W < 200)
		mesg_W = 200;

	if (mesg_H < 60)
		mesg_H = 60;

	// add a little wiggle room
	mesg_W += 16;
	mesg_H += 8;

	int total_W = 10 + ICON_W + 10 + mesg_W + 10;
	int total_H = 10 + mesg_H + 10;

	if (link_title.good())
		total_H += FONT_SIZE + 8;

	total_H += 12 + BUT_H + 12;


	// create window...
	UI_Escapable_Window *dialog = new UI_Escapable_Window(total_W, total_H, title.c_str());

	dialog->size_range(total_W, total_H, total_W, total_H);
	dialog->callback((Fl_Callback *) dialog_close_callback, &context);

	// create the error icon...
	Fl_Box *icon = new Fl_Box(10, 10 + (10 + mesg_H - ICON_H) / 2, ICON_W, ICON_H, "");

	icon->box(FL_OVAL_BOX);
	icon->align(FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
	icon->labelfont(FL_HELVETICA_BOLD);
	icon->labelsize(26);

	switch (icon_type)
	{
	case MessageBoxIcon::exclamation:
		icon->label("!");
		icon->color(FL_RED, FL_RED);
		icon->labelcolor(FL_WHITE);
		break;
	case MessageBoxIcon::question:
		icon->label("?");
		icon->color(FL_GREEN, FL_GREEN);
		icon->labelcolor(FL_BLACK);
		break;
	case MessageBoxIcon::information:
	default:
		icon->label("i");
		icon->color(FL_BLUE, FL_BLUE);
		icon->labelcolor(FL_WHITE);
		break;
	}

	// create the message area...
	Fl_Box *box = new Fl_Box(ICON_W + 20, 10, mesg_W, mesg_H, message.c_str());

	box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
	box->labelfont(FL_HELVETICA);
	box->labelsize(FONT_SIZE);


	// create the hyperlink...
	if (link_title.good())
	{
		SYS_ASSERT(link_url.good());

		UI_HyperLink *link = new UI_HyperLink(ICON_W + 20, 10 + mesg_H, mesg_W, 24,
				link_title.c_str(), link_url.c_str());
		link->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
		link->labelfont(FL_HELVETICA);
		link->labelsize(FONT_SIZE);
	}

	// create buttons...
	int GROUP_H = BUT_H + 12 * 2;

	Fl_Button *focus_button = NULL;

	Fl_Group *b_group = new Fl_Group(0, total_H - GROUP_H, total_W, GROUP_H);
	b_group->box(FL_FLAT_BOX);
	b_group->color(WINDOW_BG, WINDOW_BG);
	b_group->end();

	int but_count = labels ? (int)labels->size() : 1;
	context.buttons.resize(but_count);	// we'll fill it end to start

	int but_x = total_W - 40;
	int but_y = b_group->y() + 12;

	for (int b = but_count - 1 ; b >= 0 ; b--)
	{
		const char *text = labels ? (*labels)[b].c_str() :
		                   (icon_type == MessageBoxIcon::question) ? "OK" : "Close";

		int b_width = static_cast<int>(fl_width(text) + 20);

		Fl_Button *button = new Fl_Button(but_x - b_width, but_y, b_width, BUT_H, text);

		context.buttons[b] = button;

		button->align(FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
		button->callback((Fl_Callback *) dialog_button_callback, &context);

		b_group->insert(*button, 0);

		but_x = but_x - b_width - 40;

		// left-most button should get the focus
		focus_button = button;
	}

	dialog->end();


	// show time!
	if (focus_button)
		dialog->hotspot(focus_button);

	dialog->set_modal();
	dialog->show();

	if (icon_type == MessageBoxIcon::exclamation)
		fl_beep();

	if (focus_button)
		Fl::focus(focus_button);


	// run the GUI and let user make their choice
	while (context.result < 0)
	{
		Fl::wait();
	}

	// delete window (automatically deletes child widgets)
	delete dialog;

	return context.result;
}

static void ParseHyperLink(SString &message, SString &url, SString &linkTitle)
{
	// the syntax for a hyperlink is similar to HTML :-
	//    <a http://blah.blah.org/foobie.html>Title</a>

	SString text = message;

	size_t pos = text.find("< a");
	if(pos == SString::npos)
		return;

	// terminate the rest of the message here
	message = text;
	message.erase(pos, SString::npos);
	message.push_back('\n');

	url = text;
	url.erase(0, pos + 3);

	pos = url.find('>');
	if(pos == SString::npos)	// malformed : oh well
		return;

	linkTitle = url;
	linkTitle.erase(0, pos + 1);

	// terminate the URL here
	url.erase(pos, SString::npos);
	
	pos = linkTitle.find('<');
	if(pos != SString::npos)
		linkTitle.erase(pos, SString::npos);
}

//------------------------------------------------------------------------

void DLG_ShowError(bool fatal, EUR_FORMAT_STRING(const char *msg), ...)
{
	va_list arg_pt;

	va_start (arg_pt, msg);
	SString dialog_buffer = SString::vprintf(msg, arg_pt);
	va_end (arg_pt);

	// handle error messages with a hyperlink at the end
	SString linkTitle;
	SString linkURL;
	ParseHyperLink(dialog_buffer, linkTitle, linkURL);

	DialogShowAndRun(MessageBoxIcon::exclamation, dialog_buffer, fatal ?
					 "Eureka - Fatal Error" : "Eureka - Error", linkTitle,
					 linkURL);
}

std::function<void(const char *msg, va_list ap)> DLG_Notify_Override;

void DLG_Notify(EUR_FORMAT_STRING(const char *msg), ...)
{
	va_list arg_pt;

	va_start (arg_pt, msg);
	if(DLG_Notify_Override)
	{
		DLG_Notify_Override(msg, arg_pt);
		va_end(arg_pt);
		return;
	}
	SString dialog_buffer = SString::vprintf(msg, arg_pt);
	va_end (arg_pt);

	DialogShowAndRun(MessageBoxIcon::information, dialog_buffer, "Eureka - Notification");
}

std::function<int(const std::vector<SString> &buttons, const char *msg,
				  va_list ap)> DLG_Confirm_Override;

int DLG_Confirm(const std::vector<SString>& buttons, EUR_FORMAT_STRING(const char *msg), ...)
{
	va_list arg_pt;

	va_start (arg_pt, msg);
	if(DLG_Confirm_Override)
	{
		int result = DLG_Confirm_Override(buttons, msg, arg_pt);
		va_end(arg_pt);
		return result;
	}
	SString dialog_buffer = SString::vprintf(msg, arg_pt);
	va_end (arg_pt);

	return DialogShowAndRun(MessageBoxIcon::question, dialog_buffer, "Eureka - Confirmation",
							NULL, NULL, &buttons);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
