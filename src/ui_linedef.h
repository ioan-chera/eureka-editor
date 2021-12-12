//------------------------------------------------------------------------
//  LINEDEF PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2018 Andrew Apted
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

#ifndef __EUREKA_UI_LINEDEF_H__
#define __EUREKA_UI_LINEDEF_H__

#include "e_cutpaste.h"
#include "ui_panelinput.h"

class UI_DynIntInput;

class UI_LineBox : public Fl_Group
{
private:
	int obj = -1;
	int count = 0;

	UI_Nombre *which;

	UI_DynInput  *type;
	Fl_Button    *choose;
	Fl_Button    *gen;

	Fl_Output    *desc;
	Fl_Choice    *actkind;

	UI_DynIntInput *length;
	UI_DynIntInput *tag;
	UI_DynIntInput *args[5];

	UI_SideBox *front;
	UI_SideBox *back;

	// Flags
	Fl_Choice *f_automap;

	Fl_Check_Button *f_upper;
	Fl_Check_Button *f_lower;
	Fl_Check_Button *f_passthru;
	Fl_Check_Button *f_3dmidtex;  // Eternity

	Fl_Check_Button *f_jumpover;  //
	Fl_Check_Button *f_trans1;    // Strife
	Fl_Check_Button *f_trans2;    //

	Fl_Check_Button *f_walk;
	Fl_Check_Button *f_mons;
	Fl_Check_Button *f_sound;
	Fl_Check_Button *f_flyers;    // Strife

	Instance &inst;
	PanelFieldFixUp mFixUp;

public:
	UI_LineBox(Instance &inst, int X, int Y, int W, int H, const char *label = nullptr);

	void SetObj(int _index, int _count);

	int GetObj() const { return obj; }

	// call this if the linedef was externally changed.
	// -1 means "all fields"
	void UpdateField(int field = -1);

	// call this is the linedef's sides were externally modified.
	void UpdateSides();

	void UpdateTotal();

	// see ui_window.h for description of these two methods
	bool ClipboardOp(EditCommand op);
	void BrowsedItem(char kind, int number, const char *name, int e_state);

	void UnselectPics();

	void UpdateGameInfo();

	void checkDirtyFields()
	{
		mFixUp.checkDirtyFields();
	}

private:
	void CalcLength();

	int  CalcFlags() const;
	void FlagsFromInt(int flags);

	void CB_Copy(int parts);
	void CB_Paste(int parts, int new_tex);

	void SetTexture(const char *tex_name, int e_state, int parts);
	void SetTexOnLine(EditOperation &op, int ld, int new_tex, int e_state, int parts);
	void SetLineType(int new_type);

	int SolidMask(const LineDef *L, Side side) const;

	void checkSidesDirtyFields();

	const char *GeneralizedDesc(int type_num);

	static void    type_callback(Fl_Widget *, void *);
	static void dyntype_callback(Fl_Widget *, void *);

	static void    tag_callback(Fl_Widget *, void *);
	static void  flags_callback(Fl_Widget *, void *);
	static void   args_callback(Fl_Widget *, void *);
	static void length_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
};

#endif  /* __EUREKA_UI_LINEDEF_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
