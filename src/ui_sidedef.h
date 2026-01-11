//------------------------------------------------------------------------
//  SIDEDEF INFORMATION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
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

#ifndef __EUREKA_UI_SIDEDEF_H__
#define __EUREKA_UI_SIDEDEF_H__

#include "ui_panelinput.h"
#include "ui_stackpanel.h"

#define SETOBJ_NO_LINE  -2

class UI_DynIntInput;

// solid_mask bits : when set, that part requires a texture
enum
{
	SOLID_LOWER = (1 << 0),
	SOLID_MID   = (1 << 1),
	SOLID_UPPER = (1 << 2)

};

class UI_SideSectionPanel : public UI_StackPanel
{
public:
	UI_SideSectionPanel(Instance &inst, int X, int Y, int W, int H, const char *label = nullptr);

	UI_Pic* getPic() const
	{
		return pic;
	}

	UI_DynInput* getTex() const
	{
		return tex;
	}

private:

	UI_Pic *pic;
	UI_DynInput *tex;
};


class UI_SideBox : public Fl_Group
{
private:
	int  obj = SETOBJ_NO_LINE;
	const bool is_front;

	int what_is_solid;
	bool on_2S_line = false;

public:
	UI_DynIntInput *x_ofs;
	UI_DynIntInput *y_ofs;
	UI_DynIntInput *sec;

	UI_SideSectionPanel *l_panel;
	UI_SideSectionPanel *u_panel;
	UI_SideSectionPanel *r_panel;

	Fl_Button *add_button;
	Fl_Button *del_button;

	Instance &inst;

private:
	PanelFieldFixUp mFixUp;

public:
	UI_SideBox(Instance &inst, int X, int Y, int W, int H, int _side);

public:
	// this can be a sidedef number or -1 for none, or the special
	// value SETOBJ_NO_LINE when there is no linedef at all.
	// solid_mask is a bit field of parts which require a texture.
	// two_sided is from the linedef, will show all parts if true.
	void SetObj(int index, int solid_mask, bool two_sided);

	void UpdateField();

	// returns a bitmask of PART_RT_XXX values.
	int GetSelectedPics() const;
	int GetHighlightedPics() const;

	void UnselectPics();

	//
	// Forward to the fixup
	//
	void checkDirtyFields()
	{
		mFixUp.checkDirtyFields();
	}

private:
	void UpdateLabel();
	void UpdateHiding();
	void UpdateAddDel();

	int getMidTexX(int position) const;

	static void    tex_callback(Fl_Widget *, void *);
	static void dyntex_callback(Fl_Widget *, void *);
	static void offset_callback(Fl_Widget *, void *);
	static void sector_callback(Fl_Widget *, void *);
	static void    add_callback(Fl_Widget *, void *);
	static void delete_callback(Fl_Widget *, void *);
};

#endif  /* __EUREKA_UI_SIDEDEF_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
