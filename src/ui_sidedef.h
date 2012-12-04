//------------------------------------------------------------------------
//  SideDef Information Panel
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2012 Andrew Apted
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


class UI_SideBox : public Fl_Group
{
private:
	int  obj;
	bool is_front;

public:
	Fl_Int_Input *x_ofs;
	Fl_Int_Input *y_ofs;
	Fl_Int_Input *sec;

	UI_Pic   *l_pic;
	UI_Pic   *m_pic;
	UI_Pic   *u_pic;

	Fl_Input *l_tex;
	Fl_Input *m_tex;
	Fl_Input *u_tex;

	Fl_Button *add_button;
	Fl_Button *del_button;

public:
	UI_SideBox(int X, int Y, int W, int H, int _side);
	virtual ~UI_SideBox();

public:
	// this can be a sidedef number or -1 for none, or the special
	// value SETOBJ_NO_LINE when there is no linedef at all.
	void SetObj(int index);

	// where is 'l' for lower / 'm' for middle / 'u' for upper
	void SetTexture(const char *name, char where);

	void UpdateField();

	// returns a bitmask: 1 for lower, 2 for middle, 4 for upper
	int GetSelectedPics() const;

	void UnselectPics();

private:
	void UpdateLabel();
	void UpdateHiding(bool hide);
	void UpdateAddDel();

	int TexFromWidget(Fl_Input *w);

	static void    tex_callback(Fl_Widget *, void *);
	static void offset_callback(Fl_Widget *, void *);
	static void sector_callback(Fl_Widget *, void *);
	static void    add_callback(Fl_Widget *, void *);
	static void delete_callback(Fl_Widget *, void *);
};

#endif  /* __EUREKA_UI_SIDEDEF_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
