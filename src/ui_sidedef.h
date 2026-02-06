//------------------------------------------------------------------------
//  SIDEDEF INFORMATION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
//  Copyright (C) 2026 Ioan Chera
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

#include "m_game.h"
#include "m_udmf.h"
#include "ui_panelinput.h"
#include "ui_stackpanel.h"
#include <memory>
#include <vector>

#define SETOBJ_NO_LINE  -2

class Fl_Check_Button;
class Fl_Grid;
class UI_CategoryButton;
class UI_DynFloatInput;
class UI_DynIntInput;
struct ConfigData;
struct LoadingData;

// solid_mask bits : when set, that part requires a texture
enum
{
	SOLID_LOWER = (1 << 0),
	SOLID_MID   = (1 << 1),
	SOLID_UPPER = (1 << 2)

};

// Stores widgets for a single UDMF sidepart field (one row of inputs per part)
struct SidepartFieldWidgets
{
	sidefield_t info = {};
	std::unique_ptr<Fl_Group> container;
	std::vector<std::unique_ptr<Fl_Widget>> widgets; // one per dimension
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

	const std::vector<SidepartFieldWidgets> &getUDMFFields() const
	{
		return mUDMFFields;
	}

	// Creates UDMF sidepart widgets based on config
	void updateUDMFFields(const LoadingData &loaded, const ConfigData &config,
						  Fl_Callback *callback, void *callbackData,
						  PanelFieldFixUp &fixUp);

private:
	UI_Pic *pic;
	UI_DynInput *tex;

	std::vector<SidepartFieldWidgets> mUDMFFields;
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

	// UDMF sidedef-level flag widgets
	struct SideFlagButton
	{
		Fl_Check_Button *button = nullptr;
		UDMF_SideFeature feature;
		int mask = 0;
	};
	std::vector<SideFlagButton> mFlagButtons;
	UI_CategoryButton *mFlagsHeader = nullptr;
	Fl_Grid *mFlagsGrid = nullptr;

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

	// Updates UDMF sidepart widgets based on config
	void UpdateGameInfo(const LoadingData &loaded, const ConfigData &config);

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
	static void udmf_field_callback(Fl_Widget *, void *);
	static void side_flag_callback(Fl_Widget *, void *);
	static void side_category_callback(Fl_Widget *, void *);

	void loadSideFlags(const LoadingData &loaded, const ConfigData &config);
	void cleanupSideFlags();
	void updateSideFlagValues();
	void updateSideFlagSummary();
};

#endif  /* __EUREKA_UI_SIDEDEF_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
