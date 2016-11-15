//------------------------------------------------------------------------
//  DEFAULT PROPERTIES
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

#ifndef __EUREKA_UI_DEFAULT_H__
#define __EUREKA_UI_DEFAULT_H__


class UI_DefaultProps : public Fl_Group
{
private:
	UI_Pic     *w_pic;
	UI_PicName *w_tex;

	Fl_Int_Input *ceil_h;
	Fl_Int_Input *light;
	Fl_Int_Input *floor_h;

	Fl_Button *ce_down, *ce_up;
	Fl_Button *fl_down, *fl_up;

	UI_Pic     *c_pic;
	UI_PicName *c_tex;

	UI_Pic     *f_pic;
	UI_PicName *f_tex;

	Fl_Int_Input *thing;
	Fl_Output    *th_desc;
	UI_Pic		 *th_sprite;

public:
	UI_DefaultProps(int X, int Y, int W, int H);
	virtual ~UI_DefaultProps();

	void BrowsedItem(char kind, int number, const char *name, int e_state);

	void UnselectPics();

	void LoadValues();

private:
	void SetIntVal(Fl_Int_Input *w, int value);
	void UpdateThingDesc();

	void SetTexture(const char *name, int e_state);
	void SetFlat(const char *name, int e_state);
	void SetThing(int number);
	void UnselectPicSet(char what /* 'f' or 't' */);

	static const char * Normalize_and_Dup(UI_PicName *w);

	static void   hide_callback(Fl_Widget *w, void *data);
	static void    tex_callback(Fl_Widget *w, void *data);
	static void dyntex_callback(Fl_Widget *w, void *data);
	static void   flat_callback(Fl_Widget *w, void *data);
	static void button_callback(Fl_Widget *w, void *data);
	static void height_callback(Fl_Widget *w, void *data);
	static void  thing_callback(Fl_Widget *w, void *data);
};


bool Props_ParseUser(const char ** tokens, int num_tok);
void Props_WriteUser(FILE *fp);
void Props_LoadValues();

#endif  /* __EUREKA_UI_DEFAULT_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
