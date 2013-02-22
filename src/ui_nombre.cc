//------------------------------------------------------------------------
//  Information Bar (bottom of window)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2009 Andrew Apted
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
#include "m_config.h"

#include "ui_window.h"
#include "ui_nombre.h"


#define NOMBRBACK_COL  (gui_scheme == 2 ? FL_GRAY0+1 : FL_GRAY0+3)


//
// UI_Nombre Constructor
//
UI_Nombre::UI_Nombre(int X, int Y, int W, int H, const char *what) : 
    Fl_Box(FL_FLAT_BOX, X, Y, W, H, ""),
    index(-1), total(0), selected(0)
{
	type_name = strdup(what);  // FIXME: consistent string handling

	align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	color(NOMBRBACK_COL);

	labelfont(FL_COURIER_BOLD);
	labelsize(16 + KF*3);

	Update();
}

//
// UI_Nombre Destructor
//
UI_Nombre::~UI_Nombre()
{
}


void UI_Nombre::Update()
{
	char buffer[256];

///	if (index < 0 && total <= 0)
///		sprintf(buffer, "%s: NONE\n", type_name);
/// else
	if (index < 0)
		sprintf(buffer, "No %s    / %d\n", type_name, total);
	else if (selected > 1)
		sprintf(buffer, "%s #%-4d + %d more\n", type_name, index, selected-1);
	else
		sprintf(buffer, "%s #%-4d / %d\n", type_name, index, total);

	if (index < 0 || total == 0)
		labelcolor(FL_DARK1);
	else
		labelcolor(FL_YELLOW);

	if (index < 0 || total == 0)
		color(NOMBRBACK_COL);
	else if (selected == 0)
		color(NOMBRBACK_COL);   // same as above
	else if (selected == 1)
		color(FL_BLUE);
	else
		color(FL_RED);

	copy_label(buffer);
}


void UI_Nombre::SetIndex(int _idx)
{
	if (index != _idx)
	{
		index = _idx;
		Update();
	}
}

void UI_Nombre::SetTotal(int _tot)
{
	if (total != _tot)
	{
		total = _tot;
		Update();
	}
}

void UI_Nombre::SetSelected(int _sel)
{
	if (selected != _sel)
	{
		selected = _sel;
		Update();
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
