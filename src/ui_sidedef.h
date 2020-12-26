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


#define SETOBJ_NO_LINE  -2

// solid_mask bits : when set, that part requires a texture
enum
{
	SOLID_LOWER = (1 << 0),
	SOLID_MID   = (1 << 1),
	SOLID_UPPER = (1 << 2)

};


class UI_SideBox : public Fl_Group
{
private:
	int  obj;
	bool is_front;

	int what_is_solid;
	bool on_2S_line;

	int m_x_tile_positions[2];	// positions of first two texture tiles, depending on user setting

public:
	Fl_Int_Input *x_ofs;
	Fl_Int_Input *y_ofs;
	Fl_Int_Input *sec;

	UI_Pic *l_pic;
	UI_Pic *u_pic;
	UI_Pic *r_pic;

	UI_DynInput *l_tex;
	UI_DynInput *u_tex;
	UI_DynInput *r_tex;

	Fl_Button *add_button;
	Fl_Button *del_button;

	Instance &inst;

public:
	UI_SideBox(Instance &inst, int X, int Y, int W, int H, int _side);
	virtual ~UI_SideBox();

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

private:
	void UpdateLabel();
	void UpdateHiding();
	void UpdateAddDel();

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
