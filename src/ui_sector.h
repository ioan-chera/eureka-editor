//------------------------------------------------------------------------
//  Sector Information Panel
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

#ifndef __EUREKA_UI_SECTOR_H__
#define __EUREKA_UI_SECTOR_H__


class UI_SectorBox : public Fl_Group
{
private:
	int obj;
	int count;

public:
	UI_Nombre *which;

	Fl_Int_Input *ceil_h;
	Fl_Int_Input *headroom;
	Fl_Int_Input *floor_h;

	Fl_Button *ce_down, *ce_up;
	Fl_Button *fl_down, *fl_up;

	Fl_Input *c_tex;
	UI_Pic   *c_pic;

	Fl_Input *f_tex;
	UI_Pic   *f_pic;

	Fl_Int_Input *type;
	Fl_Output    *desc;
	Fl_Button    *choose;

	Fl_Int_Input *light;
	Fl_Int_Input *tag;

	Fl_Button *lt_down, *lt_up;

public:
	UI_SectorBox(int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_SectorBox();

public:
	void SetObj(int _index, int _count);

	int GetObj() const { return obj; }

	// call this if the thing was externally changed.
	// -1 means "all fields"
	void UpdateField(int field = -1);

	void UpdateTotal();

	void SetFlat(const char *name, int e_state);
	void SetSectorType(int new_type);

	// returns a bitmask: 1 for floor, 2 for ceiling
	int GetSelectedPics() const;

	void UnselectPics();

private:
	void AdjustHeight(s16_t *h, int delta);
	void AdjustLight (s16_t *L, int delta);

	// this truncates the name and makes it uppercase, then returns
	// the internalised string.
	int FlatFromWidget(Fl_Input *w);

	static void height_callback(Fl_Widget *, void *);
	static void   room_callback(Fl_Widget *, void *);
	static void    tex_callback(Fl_Widget *, void *);
	static void   type_callback(Fl_Widget *, void *);
	static void  light_callback(Fl_Widget *, void *);
	static void    tag_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
};

#endif  /* __EUREKA_UI_SECTOR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
