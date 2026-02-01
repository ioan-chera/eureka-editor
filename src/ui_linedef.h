//------------------------------------------------------------------------
//  LINEDEF PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025      Ioan Chera
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

#include "e_basis.h"
#include "e_cutpaste.h"
#include "ui_panelinput.h"

class ActivationPopup;
class Fl_Flex;
class Fl_Grid;
class Fl_Simple_Counter;
class line_flag_CB_data_c;
class MultiTagView;
class UI_CategoryButton;
class UI_DynIntInput;
class UI_StackPanel;
struct FeatureFlagMapping;
struct LoadingData;
struct lineflag_t; // forward declaration

class UI_LineBox : public MapItemBox
{
private:
	UI_StackPanel *panel;

	UI_DynInput  *type;
	Fl_Button    *choose;
	Fl_Button    *gen;

	Fl_Flex      *descFlex;

	Fl_Output    *desc;
	Fl_Choice    *actkind;

	Fl_Menu_Button *udmfActivationButton;
	std::vector<Fl_Menu_Item> udmfActivationMenuItems;

	UI_DynIntInput *length;
	UI_DynIntInput *tag;
	UI_DynIntInput *args[5];
	UI_DynInput *args0str;

	Fl_Flex     *argsFlex;

	MultiTagView *multiTagView;

	Fl_Valuator *alphaWidget;
	Fl_Check_Button *udmfTranslucencyCheckBoxes[2];

	UI_SideBox *front;
	UI_SideBox *back;

	// Flags
	Fl_Choice *f_automap;
	std::unique_ptr<line_flag_CB_data_c> f_automap_cb_data;

	// Dynamic linedef flags (configured via .ugh)
	struct LineFlagButton
	{
		Fl_Check_Button *button = nullptr;
		int udmfActivationMenuIndex = -1;
		std::unique_ptr<line_flag_CB_data_c> data;
	};
	std::vector<LineFlagButton> flagButtons;

	// Category headers for collapsible flag sections
	struct CategoryHeader
	{
		UI_CategoryButton *button;
		Fl_Grid *grid;
		std::vector<int> lineFlagButtonIndices;  // indices in flagButtons
	};
	std::vector<CategoryHeader> categoryHeaders;

	// Dynamic UDMF line field widgets (configured via .ugh)
	struct LineField
	{
		Fl_Widget *widget;
		Fl_Widget *widget2;    // second widget for intpair type
		Fl_Group *container;   // container for intpair (Fl_Flex holding both widgets)
		const struct linefield_t *info;
	};
	std::vector<LineField> fields;

	// struct LineSliderWidget
	// {
	// 	std::unique_ptr<Fl_Simple_Counter> counter;
	// 	const struct lineslider_t *info;
	// };
	// std::vector<LineSliderWidget> sliderWidgets;

	int flagsStartX = 0;
	int flagsStartY = 0;
	int flagsAreaW = 0;

public:
	UI_LineBox(Instance &inst, int X, int Y, int W, int H, const char *label = nullptr);

	// call this if the linedef was externally changed.
	// -1 means "all fields"
	void UpdateField(std::optional<Basis::EditField> efield = std::nullopt) override;

	// call this is the linedef's sides were externally modified.
	void UpdateSides();

	void UpdateTotal(const Document &doc) noexcept override;

	// see ui_window.h for description of these two methods
	bool ClipboardOp(EditCommand op);
	void BrowsedItem(BrowserMode kind, int number, const char *name, int e_state);

	void UnselectPics() override;

	void UpdateGameInfo(const LoadingData &loaded, const ConfigData &config) override;

	void checkDirtyFields()
	{
		mFixUp.checkDirtyFields();
	}

private:
	void clearArgs();
	void clearFields();

	void populateUDMFFlagCheckBoxes(const FeatureFlagMapping *mapping, size_t count,
									const LoadingData &loaded, const ConfigData &config,
									const char *title);

	void updateUDMFBaseFlags(const LoadingData &loaded, const ConfigData &config);
	void updateUDMFActivationMenu(const LoadingData &loaded, const ConfigData &config);
	void updateUDMFBlockingFlags(const LoadingData &loaded, const ConfigData &config);
	void updateUDMFRenderingControls(const LoadingData &loaded, const ConfigData &config);

	int getActivationCount() const;
	const char *getActivationMenuString() const;

	void CalcLength();

	void CalcFlags(int &outFlags, int &outFlags2) const;
	bool updateDynamicFlagControls(int lineFlags, int flagSet);
	bool FlagsFromInt(int flags);
	bool Flags2FromInt(int flags2);

	void CB_Copy(int uiparts);
	void CB_Paste(int uiparts, StringID new_tex);

	void SetTexture(const char *tex_name, int e_state, int uiparts);
	void SetTexOnLine(EditOperation &op, int ld, StringID new_tex, int e_state, int parts);
	void SetLineType(int new_type);

	int SolidMask(const LineDef *L, Side side) const;

	void checkSidesDirtyFields();

	const char *GeneralizedDesc(int type_num);

	void categoryToggled(class UI_CategoryButton *categoryBtn);

	static void    type_callback(Fl_Widget *, void *);
	static void dyntype_callback(Fl_Widget *, void *);

	static void    tag_callback(Fl_Widget *, void *);
	static void  flags_callback(Fl_Widget *, void *);
	static void   args_callback(Fl_Widget *, void *);
	static void length_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
	static void category_callback(Fl_Widget *, void *);
	static void field_callback(Fl_Widget *, void *);
};

#endif  /* __EUREKA_UI_LINEDEF_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
