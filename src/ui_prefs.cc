//------------------------------------------------------------------------
//  Preferences Dialog
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012 Andrew Apted
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
#include "ui_prefs.h"


//------------------------------------------------------------------------


void UI_Preferences::Run()
{
	set_modal();

	show();

	while (! want_quit)
	{
		Fl::wait(0.2);
	}
}


void UI_Preferences::Load()
{
}


void UI_Preferences::Save()
{
}


void CMD_Preferences()
{
	UI_Preferences * dialog = new UI_Preferences();

	dialog->Load();
	dialog->Run();
	dialog->Save();

	delete dialog;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
