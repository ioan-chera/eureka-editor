//------------------------------------------------------------------------
//  Miscellaneous UI Dialogs
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2016 Andrew Apted
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

#include "ui_misc.h"

#include "Errors.h"
#include "Instance.h"
#include "e_main.h"
#include "m_vector.h"

#include "main.h"

#include <assert.h>

UI_MoveDialog::UI_MoveDialog(Instance &inst, bool want_dz) :
	UI_Escapable_Window(360, 205, "Move Objects"), inst(inst)
{
    Fl_Box *title = new Fl_Box(10, 11, w() - 20, 32, "Enter the offset to move objects:");
	title->labelsize(16);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	delta_x = new Fl_Int_Input(95, 55, 65, 25,  "delta x:");
	delta_y = new Fl_Int_Input(95, 85, 65, 25,  "delta y:");
	delta_z = new Fl_Int_Input(240, 85, 65, 25, "delta z:");

	delta_x->value("0");
	delta_y->value("0");
	delta_z->value("0");

	if (! want_dz)
		delta_z->hide();

	Fl_Group * grp = new Fl_Group(0, h() - 70, w(), 70);
	grp->box(FL_FLAT_BOX);
	grp->color(WINDOW_BG, WINDOW_BG);
	{
		cancel_but = new Fl_Button(30, grp->y() + 20, 95, 30, "Cancel");
		cancel_but->callback(close_callback, this);

		ok_but = new Fl_Button(245, grp->y() + 20, 95, 30, "Move");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		grp->end();
	}

	end();

	resizable(NULL);

	callback(close_callback, this);
}



void UI_MoveDialog::Run()
{
	set_modal();

	show();

	while (! want_close)
	{
		Fl::wait(0.2);
	}
}


void UI_MoveDialog::close_callback(Fl_Widget *w, void *data)
{
	UI_MoveDialog * that = (UI_MoveDialog *)data;

	that->want_close = true;
}


void UI_MoveDialog::ok_callback(Fl_Widget *w, void *data)
{
	UI_MoveDialog * that = (UI_MoveDialog *)data;

	int delta_x = atoi(that->delta_x->value());
	int delta_y = atoi(that->delta_y->value());
	int delta_z = atoi(that->delta_z->value());

	that->inst.level.objects.move(*that->inst.edit.Selected, { delta_x, delta_y, delta_z });

	that->want_close = true;
}


//------------------------------------------------------------------------


UI_ScaleDialog::UI_ScaleDialog(Instance &inst) :
	UI_Escapable_Window(360, 270, "Scale Objects"),
	inst(inst)
{
    Fl_Box *title = new Fl_Box(10, 11, w() - 20, 32, "Enter the scale amount:");
	title->labelsize(16);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	scale_x = new Fl_Input( 95,  55, 85, 25, "scale x:");
	scale_y = new Fl_Input( 95,  85, 85, 25, "scale y:");
	scale_z = new Fl_Input( 95, 115, 85, 25, "scale z:");

	scale_x->value("1");
	scale_y->value("1");
	scale_z->value("1");

	origin_x = new Fl_Choice(234,  55, 100, 25, "from:");
	origin_y = new Fl_Choice(234,  85, 100, 25, "from:");
	origin_z = new Fl_Choice(234, 115, 100, 25, "from:");

	origin_x->add("Left|Centre|Right");
	origin_x->value(1);

	origin_y->add("Bottom|Centre|Top");
	origin_y->value(1);

	origin_z->add("Bottom|Middle|Top");
	origin_z->value(0);

	Fl_Group * grp = new Fl_Group(0, 160, w(), h() - 160);
	grp->box(FL_FLAT_BOX);
	grp->color(WINDOW_BG, WINDOW_BG);
	{
		Fl_Box * help = new Fl_Box(10, 175, w() - 20, 40);
		help->label("Scale Values:\n"
		            "    can be real: 0.25 or 3.7\n"
					"    or percentages: 25%\n"
					"    or fractions: 3 / 4");
		help->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);

		cancel_but = new Fl_Button(245, 180, 95, 30, "Cancel");
		cancel_but->callback(close_callback, this);

		ok_but = new Fl_Button(245, 225, 95, 30, "Scale");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		grp->end();
	}

	end();

	resizable(NULL);

	callback(close_callback, this);
}


void UI_ScaleDialog::Run()
{
	if (inst.edit.mode != ObjType::sectors)
	{
		 scale_z->hide();
		origin_z->hide();
	}

	set_modal();

	show();

	while (! want_close)
	{
		Fl::wait(0.2);
	}
}


static double ParseScaleStr(const char * s)
{
	// handle percentages
	if (strchr(s, '%'))
	{
		double val;

		if (sscanf(s, " %lf %% ", &val) != 1)
			return -1;

		return val / 100.0;
	}

	// handle fractions
	if (strchr(s, '/'))
	{
		double num, denom;

		if (sscanf(s, " %lf / %lf ", &num, &denom) != 2)
			return -1;

		if (denom <= 0)
			return -1;

		return num / denom;
	}

	return atof(s);
}


void UI_ScaleDialog::close_callback(Fl_Widget *w, void *data)
{
	UI_ScaleDialog * that = (UI_ScaleDialog *)data;

	that->want_close = true;
}


void UI_ScaleDialog::ok_callback(Fl_Widget *w, void *data)
{
	UI_ScaleDialog * that = (UI_ScaleDialog *)data;

	double scale_x = ParseScaleStr(that->scale_x->value());
	double scale_y = ParseScaleStr(that->scale_y->value());
	double scale_z = ParseScaleStr(that->scale_z->value());

	if (scale_x <= 0 || scale_y <= 0 || scale_z <= 0)
	{
		// WISH: deactivate OK button instead (don't get here)
		that->inst.Beep("bad scaling value");
		that->want_close = true;
		return;
	}

	int pos_x = that->origin_x->value() - 1;
	int pos_y = that->origin_y->value() - 1;
	int pos_z = that->origin_z->value() - 1;

	if (that->inst.edit.mode == ObjType::sectors)
		that->inst.level.objects.scale4(scale_x, scale_y, scale_z, pos_x, pos_y, pos_z);
	else
		that->inst.level.objects.scale3(scale_x, scale_y, pos_x, pos_y);

	that->want_close = true;
}


//------------------------------------------------------------------------


UI_RotateDialog::UI_RotateDialog(Instance &inst) :
	UI_Escapable_Window(360, 200, "Rotate Objects"),
	want_close(false), inst(inst)
{
    Fl_Box *title = new Fl_Box(10, 11, w() - 20, 32, "Enter # of degrees to rotate objects:");
	title->labelsize(16);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	angle = new Fl_Float_Input(95, 55, 65, 25,  "angle:");
	angle->value("0");

	dir = new Fl_Choice(170, 55, 140, 25);
	dir->add("Clockwise|Anti-clockwise");
	dir->value(0);

	origin = new Fl_Choice(95, 90, 140, 25, "origin:");
	origin->add("Bottom Left|Bottom|Bottom Right|"
	            "Left|Centre|Right|"
	            "Top Left|Top|Top Right");
	origin->value(4);

	Fl_Group * grp = new Fl_Group(0, h() - 70, w(), 70);
	grp->box(FL_FLAT_BOX);
	grp->color(WINDOW_BG, WINDOW_BG);
	{
		cancel_but = new Fl_Button(30, grp->y() + 20, 95, 30, "Cancel");
		cancel_but->callback(close_callback, this);

		ok_but = new Fl_Button(245, grp->y() + 20, 95, 30, "Rotate");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		grp->end();
	}

	end();

	resizable(NULL);

	callback(close_callback, this);
}


UI_RotateDialog::~UI_RotateDialog()
{
}


void UI_RotateDialog::Run()
{
	set_modal();

	show();

	while (! want_close)
	{
		Fl::wait(0.2);
	}
}


void UI_RotateDialog::close_callback(Fl_Widget *w, void *data)
{
	UI_RotateDialog * that = (UI_RotateDialog *)data;

	that->want_close = true;
}


void UI_RotateDialog::ok_callback(Fl_Widget *w, void *data)
{
	UI_RotateDialog * that = (UI_RotateDialog *)data;

	float angle = static_cast<float>(atof(that->angle->value()));

	if (that->dir->value() == 0)
		angle = -angle;

	int pos_x = (that->origin->value() % 3) - 1;
	int pos_y = (that->origin->value() / 3) - 1;

	that->inst.level.objects.rotate3(angle, pos_x, pos_y);

	that->want_close = true;
}


//------------------------------------------------------------------------


UI_JumpToDialog::UI_JumpToDialog(const char *_objname, int _limit) :
	UI_Escapable_Window(380, 175, "Jump to Objects"),
	limit(_limit)
{
	SYS_ASSERT(limit >= 0);

	char descript[300];
	snprintf(descript, sizeof(descript), "Enter one or more %s numbers (each 0 - %d)\n", _objname, limit);

    Fl_Box *title = new Fl_Box(10, 11, w() - 20, 32, NULL);
	title->copy_label(descript);
	title->labelsize(16);
	title->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

	input = new Fl_Input(145, 55, 90, 25,  "index: ");
	input->when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);
	input->callback(input_callback, this);

	Fl_Group * grp = new Fl_Group(0, h() - 70, w(), 70);
	grp->box(FL_FLAT_BOX);
	grp->color(WINDOW_BG, WINDOW_BG);
	{
		cancel_but = new Fl_Button(30, grp->y() + 20, 95, 30, "Cancel");
		cancel_but->callback(close_callback, this);

		ok_but = new Fl_Button(245, grp->y() + 20, 95, 30, "Jump");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);
		ok_but->deactivate();

		grp->end();
	}

	end();

	resizable(NULL);

	callback(close_callback, this);
}


std::vector<int> UI_JumpToDialog::Run()
{
	set_modal();
	show();

	Fl::focus(input);

	while (! want_close)
		Fl::wait(0.2);

	return result;
}


void UI_JumpToDialog::close_callback(Fl_Widget *w, void *data)
{
	UI_JumpToDialog * that = (UI_JumpToDialog *)data;

	that->want_close = true;
}

//
// Parse a value into a list of integers
//
static std::vector<int> parseValueList(const char *text)
{
    assert(text);
    // Parse strictly numbers
    std::vector<int> result;

    const char *end = text + strlen(text);
    while(text < end)
    {
        char *endptr;
        long value = strtol(text, &endptr, 10);
        if(endptr == text)
        {
            ++text;
            continue;
        }
        result.push_back((int)value);
        text = endptr;
    }

    return result;
}

void UI_JumpToDialog::ok_callback(Fl_Widget *w, void *data)
{
	UI_JumpToDialog * that = (UI_JumpToDialog *)data;

	that->result = parseValueList(that->input->value());
	that->want_close = true;
}


void UI_JumpToDialog::input_callback(Fl_Widget *w, void *data)
{
	UI_JumpToDialog * that = (UI_JumpToDialog *)data;
    auto disable = [that]()
    {
        that->input->textcolor(FL_RED);
        that->input->redraw();
        that->ok_but->deactivate();
    };

	// this is slightly hacky
	bool was_enter = (Fl::event_key() == FL_Enter ||
					  Fl::event_key() == FL_KP_Enter);

    std::vector<int> values = parseValueList(that->input->value());

    if(values.empty())
    {
        disable();
        return;
    }

    for(int value : values)
    {
        if (value < 0 || value > that->limit)
        {
            disable();
            return;
        }
    }

	that->input->textcolor(FL_FOREGROUND_COLOR);
	that->input->redraw();
	that->ok_but->activate();

	if (was_enter)
		that->ok_callback(w, data);
}

//
// Handler for the dynamic int input
//
int UI_DynIntInput::handle(int event)
{
	int res = Fl_Int_Input::handle(event);

	if((event == FL_KEYBOARD || event == FL_PASTE) && mCallback2)
	{
		mCallback2(this, mData2);
	}

	return res;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
