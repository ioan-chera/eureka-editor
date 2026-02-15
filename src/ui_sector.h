//------------------------------------------------------------------------
//  SECTOR PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025-2026 Ioan Chera
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

#ifndef __EUREKA_UI_SECTOR_H__
#define __EUREKA_UI_SECTOR_H__

#include "e_cutpaste.h"
#include "ui_panelinput.h"

class Fl_Grid;
class Fl_Light_Button;
class Fl_Menu_Button;
class MultiTagView;
class UI_CategoryButton;
class UI_DynFloatInput;
class UI_DynIntInput;
class UI_StackPanel;
struct gensector_t;

struct SectorFlagButton
{
	Fl_Check_Button *button = nullptr;
	Fl_Choice *choice = nullptr;
	const gensector_t *info = nullptr;
};

class UI_SectorBox : public MapItemBox
{
public:
	UI_DynInput   *type;
	Fl_Output    *desc;
	Fl_Button    *choose;

	UI_DynIntInput *light;
	UI_DynIntInput *tag;

	Fl_Button *fresh_tag;
	Fl_Button *lt_down, *lt_up;

	UI_DynIntInput *ceil_h;
	UI_DynIntInput *floor_h;

	Fl_Button *ce_down, *ce_up;
	Fl_Button *fl_down, *fl_up;

	UI_DynInput *c_tex;
	UI_Pic      *c_pic;

	UI_DynInput *f_tex;
	UI_Pic      *f_pic;

	UI_DynIntInput *headroom;

	enum
	{
		HEADROOM_BUTTONS = 6
	};

	Fl_Button * hd_buttons[HEADROOM_BUTTONS];

	// Boom generalized sectors

	Fl_Box    * bm_title;

	std::vector<SectorFlagButton> bm_buttons;
	int genStartX = 0, genStartY = 0;
	int basicSectorMask = 65535;

	UI_CategoryButton *udmfHeader;
	UI_StackPanel *udmfPanel;
	MultiTagView *multiTagView;

	// Floor/ceiling UDMF part grid
	Fl_Grid *mPartGrid = nullptr;

	enum
	{
		PART_IDX_FLOOR,
		PART_IDX_CEIL,
		kNumSectorParts = 2,
	};

	Fl_Box *mPartHeader[kNumSectorParts] = {};

	Fl_Box *mPanLabel = nullptr;
	UI_DynFloatInput *mPanX[kNumSectorParts] = {};
	UI_DynFloatInput *mPanY[kNumSectorParts] = {};

	Fl_Box *mPartScaleLabel = nullptr;
	UI_DynFloatInput *mPartScaleX[kNumSectorParts] = {};
	UI_DynFloatInput *mPartScaleY[kNumSectorParts] = {};

	Fl_Box *mRotLabel = nullptr;
	UI_DynFloatInput *mRotation[kNumSectorParts] = {};

	Fl_Box *mPartLightLabel = nullptr;
	UI_DynIntInput *mPartLight[kNumSectorParts] = {};
	Fl_Light_Button *mPartLightAbsolute[kNumSectorParts] = {};

	Fl_Box *mPartScrollLabel = nullptr;
	UI_DynFloatInput *mPartScrollX[kNumSectorParts] = {};
	UI_DynFloatInput *mPartScrollY[kNumSectorParts] = {};

	Fl_Box *mScrollModeLabel = nullptr;
	Fl_Menu_Button *mScrollMode[kNumSectorParts] = {};
	Fl_Menu_Item mScrollModeItems[kNumSectorParts][5] = {};

	Fl_Box *mSkyLabel = nullptr;
	UI_DynInput *mSky[kNumSectorParts] = {};

public:
	UI_SectorBox(Instance &inst, int X, int Y, int W, int H, const char *label = NULL);

public:
	// call this if the thing was externally changed.
	// -1 means "all fields"
	void UpdateField(std::optional<Basis::EditField> efield = std::nullopt) override;

	void UpdateTotal(const Document &doc) noexcept override;

	void UpdateGameInfo(const LoadingData &loaded, const ConfigData &config) override;

	// see ui_window.h for description of these two methods
	bool ClipboardOp(EditCommand op);
	void BrowsedItem(BrowserMode kind, int number, const char *name, int e_state);

	void UnselectPics() override;

private:
	void CB_Copy(int parts);
	void CB_Paste(int parts, StringID new_tex);
	void CB_Cut(int parts);

	// returns either zero or a combination of PART_FLOOR and PART_CEIL
	int GetSelectedPics() const;
	int GetHighlightedPics() const;

	void SetFlat(const SString &name, int parts);
	void SetSectorType(int new_type);

	void InstallFlat(const SString &name, int parts);
	void InstallSectorType(int mask, int value);

	void FreshTag();

	int findYForUDMF();
	void updateUDMFGameInfo(const ConfigData &config);
	void updatePartGridLayout(const ConfigData &config);
	void updateUDMFFields();
	static void udmfCategoryCallback(Fl_Widget *widget, void *data);
	static void udmfFieldCallback(Fl_Widget *widget, void *data);
	static void scrollModeCallback(Fl_Widget *widget, void *data);
	void updateScrollModeLabel(int partIdx);
	void multiTagViewCallback();

	static void   height_callback(Fl_Widget *, void *);
	static void headroom_callback(Fl_Widget *, void *);

	static void     tex_callback(Fl_Widget *, void *);
	static void  dyntex_callback(Fl_Widget *, void *);
	static void    type_callback(Fl_Widget *, void *);
	static void dyntype_callback(Fl_Widget *, void *);

	static void   light_callback(Fl_Widget *, void *);
	static void     tag_callback(Fl_Widget *, void *);
	static void  button_callback(Fl_Widget *, void *);
};

#endif  /* __EUREKA_UI_SECTOR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
