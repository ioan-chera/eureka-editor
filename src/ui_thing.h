//------------------------------------------------------------------------
//  THING PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2015 Andrew Apted
//  Copyright (C)      2015 Ioan Chera
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

#ifndef __EUREKA_UI_THING_H__
#define __EUREKA_UI_THING_H__


class Sticker;


class UI_ThingBox : public Fl_Group
{
private:
	int obj;
	int count;

	UI_Nombre *which;

	Fl_Int_Input *type;
	Fl_Output    *desc;
	Fl_Button    *choose;

	Fl_Int_Input *angle;
	Fl_Button    *ang_buts[8];

	Fl_Int_Input *tid;

	Fl_Int_Input *exfloor;
	Fl_Button    *efl_down;
	Fl_Button    *efl_up;

	Fl_Int_Input *pos_x;
	Fl_Int_Input *pos_y;
	Fl_Int_Input *pos_z;

	// Options
	Fl_Check_Button *o_easy;
	Fl_Check_Button *o_medium;
	Fl_Check_Button *o_hard;

	Fl_Check_Button *o_sp;
	Fl_Check_Button *o_coop;
	Fl_Check_Button *o_dm;
	Fl_Check_Button *o_vanilla_dm;

	Fl_Check_Button *o_fight;   //
	Fl_Check_Button *o_cleric;  // Hexen
	Fl_Check_Button *o_mage;    //

	Fl_Check_Button *o_ambush;
	Fl_Check_Button *o_friend;   // Boom / MBF
	Fl_Check_Button *o_dormant;  // Hexen

	UI_Pic *sprite;

	// more Hexen stuff
	Fl_Int_Input *spec_type;
	Fl_Button    *spec_choose;
	Fl_Output    *spec_desc;
	Fl_Int_Input *args[5];

public:
	UI_ThingBox(int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_ThingBox();

public:
	void SetObj(int _index, int _count);

	int GetObj() const { return obj; }

	// call this if the thing was externally changed.
	// -1 means "all fields"
	void UpdateField(int field = -1);

	void UpdateTotal();

	void SetThingType(int new_type);

	void UpdateGameInfo();
	void UpdateMapFormatInfo();

private:
	static void      x_callback(Fl_Widget *w, void *data);
	static void      y_callback(Fl_Widget *w, void *data);
	static void      z_callback(Fl_Widget *w, void *data);
	static void   type_callback(Fl_Widget *w, void *data);
	static void  angle_callback(Fl_Widget *w, void *data);
	static void    tid_callback(Fl_Widget *w, void *data);
	static void option_callback(Fl_Widget *w, void *data);
	static void button_callback(Fl_Widget *w, void *data);
	static void   spec_callback(Fl_Widget *w, void *data);
	static void   args_callback(Fl_Widget *w, void *data);

	void AdjustExtraFloor(int dir);

	int  CalcOptions() const;
	void OptionsFromInt(int options);

};

#endif  /* __EUREKA_UI_THING_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
