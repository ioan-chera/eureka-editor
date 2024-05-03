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

class Fl_Sys_Menu_Bar;

namespace menu
{
Fl_Sys_Menu_Bar *create(int x, int y, int w, int h, void *userData);
void updateBindings();
}

#endif  /* __EUREKA_UI_MENU_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
