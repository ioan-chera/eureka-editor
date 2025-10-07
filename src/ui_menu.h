//------------------------------------------------------------------------
//  MENUS
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

#ifndef __EUREKA_UI_MENU_H__
#define __EUREKA_UI_MENU_H__

#include "FL/Fl_Sys_Menu_Bar.H"

class Fl_Menu_Bar;
class SString;

namespace menu
{
Fl_Sys_Menu_Bar *create(int x, int y, int w, int h, void *userData);
void updateBindings(Fl_Sys_Menu_Bar *bar);
void setTestMapDetail(Fl_Sys_Menu_Bar *bar, const SString &text);
void setUndoDetail(Fl_Sys_Menu_Bar *bar, const SString &verb);
void setRedoDetail(Fl_Sys_Menu_Bar *bar, const SString &verb);
}

#endif  /* __EUREKA_UI_MENU_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
