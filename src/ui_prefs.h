//------------------------------------------------------------------------
//  PREFERENCES DIALOG
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

#ifndef __EUREKA_UI_PREFS_H__
#define __EUREKA_UI_PREFS_H__

class UI_Preferences;

namespace prefsdlg
{
	// Attempt to handle Undo/Redo while Preferences dialog is active.
	// Returns true if handled by the dialog (i.e., dialog is active
	// and an undo/redo step was executed), false otherwise.
	bool TryUndo(UI_Preferences *preferences);
	bool TryRedo(UI_Preferences *preferences);
}

#endif  /* __EUREKA_UI_PREFS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
