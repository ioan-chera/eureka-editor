//------------------------------------------------------------------------
//  THING PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2018 Andrew Apted
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

#include "e_cutpaste.h"
#include "ui_panelinput.h"

class Sticker;
class UI_DynIntInput;
class UI_ThingBox;
struct thingflag_t;

class thing_opt_CB_data_c
{
public:
	UI_ThingBox *parent;

	int mask;

public:
	thing_opt_CB_data_c(UI_ThingBox *_parent, int _mask) :
		parent(_parent), mask(_mask)
	{ }

};

struct FlagButton
{
	std::unique_ptr<Fl_Check_Button> button;
	std::unique_ptr<thing_opt_CB_data_c> data;
	const thingflag_t *info;
};

class UI_ThingBox : public MapItemBox
{
private:
	UI_DynInput  *type;
	Fl_Output    *desc;
	Fl_Button    *choose;

	UI_DynIntInput *angle;
	Fl_Button    *ang_buts[8];

	UI_DynIntInput *tid;

	UI_DynIntInput *exfloor;
	Fl_Button    *efl_down;
	Fl_Button    *efl_up;

	UI_DynIntInput *pos_x;
	UI_DynIntInput *pos_y;
	UI_DynIntInput *pos_z;

	std::vector<FlagButton> flagButtons;
	int optionStartX, optionStartY;

	UI_Pic *sprite;

	// more Hexen stuff
	UI_DynInput  *spec_type;
	Fl_Button    *spec_choose;
	Fl_Output    *spec_desc;
	UI_DynIntInput *args[5];

public:
	UI_ThingBox(Instance &inst, int X, int Y, int W, int H, const char *label = NULL);

public:
	// call this if the thing was externally changed.
	// -1 means "all fields"
	void UpdateField(int field = -1) override;

	void UpdateTotal(const Document &doc) noexcept override;

	// see ui_window.h for description of these two methods
	bool ClipboardOp(EditCommand op);
	void BrowsedItem(BrowserMode kind, int number, const char *name, int e_state);

	void UpdateGameInfo(const LoadingData &loaded, const ConfigData &config) override;
	void UnselectPics() override;

private:
	void SetThingType(int new_type);
	void SetSpecialType(int new_type);

	void AdjustExtraFloor(int dir);

	int  CalcOptions() const;
	void OptionsFromInt(int options);

private:
	static void       x_callback(Fl_Widget *w, void *data);
	static void       y_callback(Fl_Widget *w, void *data);
	static void       z_callback(Fl_Widget *w, void *data);
	static void    type_callback(Fl_Widget *w, void *data);
	static void dyntype_callback(Fl_Widget *, void *);

	static void  angle_callback(Fl_Widget *w, void *data);
	static void    tid_callback(Fl_Widget *w, void *data);
	static void option_callback(Fl_Widget *w, void *data);
	static void button_callback(Fl_Widget *w, void *data);

	static void    spec_callback(Fl_Widget *w, void *data);
	static void dynspec_callback(Fl_Widget *w, void *data);
	static void    args_callback(Fl_Widget *w, void *data);
};

#endif  /* __EUREKA_UI_THING_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
