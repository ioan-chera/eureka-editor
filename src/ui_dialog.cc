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

#include "main.h"
#include "ui_window.h"


#define BUT_W  100
#define BUT_H  26

#define ICON_W  40
#define ICON_H  40

#define FONT_SIZE  16


static int dialog_result;

static void dialog_close_callback(Fl_Widget *w, void *data)
{
	dialog_result = 0;
}

static void dialog_button_callback(Fl_Widget *w, void *data)
{
	dialog_result = (long)data;
}


static int DialogShowAndRun(char icon_type, const char *message, const char *title,
		const char *link_title = NULL, const char *link_url = NULL,
		std::vector<std::string> *labels = NULL)
{
	dialog_result = -1;

	// determine required size
	int mesg_W = 480;  // NOTE: fl_measure will wrap to this!
	int mesg_H = 0;

	fl_font(FL_HELVETICA, FONT_SIZE);
	fl_measure(message, mesg_W, mesg_H);

	if (mesg_W < 200)
		mesg_W = 200;

	if (mesg_H < 60)
		mesg_H = 60;

	// add a little wiggle room
	mesg_W += 16;
	mesg_H += 8;

	int total_W = 10 + ICON_W + 10 + mesg_W + 10;
	int total_H = 10 + mesg_H + 10;

	if (link_title)
		total_H += FONT_SIZE + 8;

	total_H += 12 + BUT_H + 12;


	// create window...
	Fl_Window *dialog = new Fl_Window(0, 0, total_W, total_H, title);

	dialog->size_range(total_W, total_H, total_W, total_H);
	dialog->callback((Fl_Callback *) dialog_close_callback);


	// create the error icon...
	Fl_Box *icon = new Fl_Box(10, 10 + (10 + mesg_H - ICON_H) / 2, ICON_W, ICON_H, "");

	icon->box(FL_OVAL_BOX);
	icon->align(FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
	icon->labelfont(FL_HELVETICA_BOLD);
	icon->labelsize(26);

	if (icon_type == '!')
	{
		icon->label("!");
		icon->color(FL_RED, FL_RED);
		icon->labelcolor(FL_WHITE);
	}
	else if (icon_type == '?')
	{
		icon->label("?");
		icon->color(FL_GREEN, FL_GREEN);
		icon->labelcolor(FL_BLACK);
	}
	else
	{
		icon->label("i");
		icon->color(FL_BLUE, FL_BLUE);
		icon->labelcolor(FL_WHITE);
	}


	// create the message area...
	Fl_Box *box = new Fl_Box(ICON_W + 20, 10, mesg_W, mesg_H, message);

	box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
	box->labelfont(FL_HELVETICA);
	box->labelsize(FONT_SIZE);


	// create the hyperlink...
	if (link_title)
	{
		SYS_ASSERT(link_url);

		UI_HyperLink *link = new UI_HyperLink(ICON_W + 20, 10 + mesg_H, mesg_W, 24,
				link_title, link_url);
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

	int but_x = total_W - 20;
	int but_y = b_group->y() + 12;

	for (int b = but_count - 1 ; b >= 0 ; b--)
	{
		const char *text = labels ? (*labels)[b].c_str() :
		                   (icon_type == '?') ? "OK" : "Close";

		int b_width = fl_width(text) + 20;

		Fl_Button *button = new Fl_Button(but_x - b_width, but_y, b_width, BUT_H, text);

		button->align(FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
		button->callback((Fl_Callback *) dialog_button_callback, (void *)(long)b);

		b_group->insert(*button, 0);

		but_x = but_x - b_width - 20;

		// left-most button should get the focus
		focus_button = button;
	}

	dialog->end();


	// show time!
	dialog->set_modal();
	dialog->show();

	if (icon_type == '!')
		fl_beep();

	if (focus_button)
		Fl::focus(focus_button);

	// run the GUI and let user make their choice
	while (dialog_result < 0)
	{
		Fl::wait();
	}

	// delete window (automatically deletes child widgets)
	delete dialog;

	return dialog_result;
}


static void ParseHyperLink(char *buffer, unsigned int buf_len,
		const char ** link_title, const char ** link_url)
{
	// the syntax for a hyperlink is similar to HTML :-
	//    <a http://blah.blah.org/foobie.html>Title</a>

	char *pos = strstr(buffer, "<a ");

	if (! pos)
		return;

	// terminate the rest of the message here
	pos[0] = '\n';
	pos[1] = 0;

	pos += 3;

	*link_url = pos;

	pos = strstr(pos, ">");

	if (! pos)  // malformed : oh well
		return;

	// terminate the URL here
	pos[0] = 0;

	pos++;

	*link_title = pos;

	pos = strstr(pos, "<");

	if (pos)
		pos[0] = 0;
}


static void ParseButtons(const char *buttons,
                         std::vector<std::string>& labels)
{
	for (;;)
	{
		const char *p = strchr(buttons, '|');

		if (! p)
		{
			labels.push_back(buttons);
			return;
		}

		int len = (int)(p - buttons);
		SYS_ASSERT(len > 0);

		labels.push_back(std::string(buttons, len));

		buttons = p + 1;
	}
}


//------------------------------------------------------------------------

static char dialog_buffer[MSG_BUF_LEN];


void DLG_ShowError(const char *msg, ...)
{
	va_list arg_pt;

	va_start (arg_pt, msg);
	vsnprintf (dialog_buffer, MSG_BUF_LEN, msg, arg_pt);
	va_end (arg_pt);

	dialog_buffer[MSG_BUF_LEN-1] = 0;

	const char *link_title = NULL;
	const char *link_url   = NULL;

	// handle error messages with a hyperlink at the end
	ParseHyperLink(dialog_buffer, sizeof(dialog_buffer), &link_title, &link_url);

	DialogShowAndRun('!', dialog_buffer, "Eureka - Fatal Error", link_title, link_url);
}


void DLG_Notify(const char *msg, ...)
{
	va_list arg_pt;

	va_start (arg_pt, msg);
	vsnprintf (dialog_buffer, MSG_BUF_LEN, msg, arg_pt);
	va_end (arg_pt);

	dialog_buffer[MSG_BUF_LEN-1] = 0;

	DialogShowAndRun('i', dialog_buffer, "Eureka - Notification");
}


int DLG_Confirm(const char *buttons, const char *msg, ...)
{
	va_list arg_pt;

	va_start (arg_pt, msg);
	vsnprintf (dialog_buffer, MSG_BUF_LEN, msg, arg_pt);
	va_end (arg_pt);

	dialog_buffer[MSG_BUF_LEN-1] = 0;

	std::vector<std::string> labels;

	ParseButtons(buttons, labels);

	return DialogShowAndRun('?', dialog_buffer, "Eureka - Confirmation",
							NULL, NULL, &labels);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
