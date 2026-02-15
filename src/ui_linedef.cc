//------------------------------------------------------------------------
//  LINEDEF PANEL
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

#include "Instance.h"

#include "ui_category_button.h"
#include "ui_multitag.h"
#include "ui_misc.h"

#include "m_udmf.h"
#include "w_rawdef.h"

#include "FL/Fl_Flex.H"
#include "FL/Fl_Grid.H"
#include "FL/Fl_Hor_Value_Slider.H"
#include "FL/Fl_Input_Choice.H"

#define MLF_ALL_AUTOMAP  \
	MLF_Secret | MLF_Mapped | MLF_DontDraw | MLF_XDoom_Translucent

#define UDMF_ACTIVATION_BUTTON_LABEL "Trigger?"

static constexpr const char *kHardcodedAutoMapMenu = "Normal|Invisible|Mapped|Secret|MapSecret|InvSecret";
static constexpr const char *kHardcodedAutoMapMenuWithRevealed = "Normal|Invisible|Mapped|Secret|MapSecret|InvSecret|Revealed|RevSecret";

enum
{
	INSET_LEFT = 6,
	INSET_TOP = 6,
	INSET_RIGHT = 6,
	INSET_BOTTOM = 5,

	SPACING_BELOW_NOMBRE = 4,

	TYPE_INPUT_X = 58,
	TYPE_INPUT_WIDTH = 75,
	TYPE_INPUT_HEIGHT = 24,

	BUTTON_SPACING = 10,
	CHOOSE_BUTTON_WIDTH = 80,

	GEN_BUTTON_WIDTH = 55,
	GEN_BUTTON_RIGHT_PADDING = 10,

	INPUT_SPACING = 2,

	DESC_INSET = 10,
	DESC_WIDTH = 48,

	SPAC_WIDTH = 57,

	TAG_WIDTH = 64,
	ARG_WIDTH = 53,
	ALPHA_NUMBER_WIDTH = 60,
	ARG_PADDING = 4,
	ARG_LABELSIZE = 10,
	FLAG_LABELSIZE = 12,

	FIELD_HEIGHT = 22,

	FLAG_ROW_HEIGHT = 19,
};


struct FeatureFlagMapping
{
	UDMF_LineFeature feature;
	const char *label;
	const char *tooltip;
	unsigned value;
	int flagSet;
	const char *shortDisplay;  // Short form for collapsed category display (emoji or acronym)
};

// Structure defining flag dependencies for UI greying
struct FlagDependency
{
	UDMF_LineFeature masterFlag;  // The controlling flag
	bool mustBeOn;                // true = grey dependents when master is ON, false = when OFF
	std::vector<UDMF_LineFeature> dependentFlags;  // Flags to grey out
};

// Table of flag dependencies
static const FlagDependency kFlagDependencies[] =
{
	// impassable ON -> grey block monsters, strife railing, block floaters, block land monsters, block players
	{UDMF_LineFeature::blocking, true, {
		UDMF_LineFeature::blockmonsters,
		UDMF_LineFeature::jumpover,
		UDMF_LineFeature::blockfloaters,
		UDMF_LineFeature::blocklandmonsters,
		UDMF_LineFeature::blockplayers
	}},

	// two sided OFF -> grey block gunshots, block sight, clip railing texture, wrap railing texture
	{UDMF_LineFeature::twosided, false, {
		UDMF_LineFeature::blockhitscan,
		UDMF_LineFeature::blocksight,
		UDMF_LineFeature::blockuse,
		UDMF_LineFeature::clipmidtex,
		UDMF_LineFeature::wrapmidtex
	}},

	// block monsters ON -> grey block floaters, block land monsters
	{UDMF_LineFeature::blockmonsters, true, {
		UDMF_LineFeature::blockfloaters,
		UDMF_LineFeature::blocklandmonsters
	}},

	// block everything ON -> grey many flags
	{UDMF_LineFeature::blockeverything, true, {
		UDMF_LineFeature::blocking,
		UDMF_LineFeature::blockmonsters,
		UDMF_LineFeature::passuse,
		UDMF_LineFeature::midtex3d,
		UDMF_LineFeature::midtex3dimpassible,
		UDMF_LineFeature::jumpover,
		UDMF_LineFeature::blockfloaters,
		UDMF_LineFeature::blocklandmonsters,
		UDMF_LineFeature::blockplayers,
		UDMF_LineFeature::blockhitscan,
		UDMF_LineFeature::blockprojectiles,
		UDMF_LineFeature::blocksight,
		UDMF_LineFeature::blockuse
	}},

	// 3DMidTex OFF -> grey 3DMidTex+missiles
	{UDMF_LineFeature::midtex3d, false, {
		UDMF_LineFeature::midtex3dimpassible
	}},
};

const UI_LineBox::UDMFIntChoiceField UI_LineBox::udmfIntChoiceFields[] =
{
	{ &UI_LineBox::automapStyle, UDMF_LineFeature::automapstyle, LineDef::F_AUTOMAPSTYLE,
	"automapstyle", "Automap style:", &LineDef::automapstyle },
	{ &UI_LineBox::lockNumber, UDMF_LineFeature::locknumber, LineDef::F_LOCKNUMBER, "locknumber",
	"Lock:", &LineDef::locknumber },
};


class line_flag_CB_data_c
{
public:
	UI_LineBox *parent;

	int mask;
	int mask2;	// mask for flags2 set (0 if only flags set is used)

	line_flag_CB_data_c(UI_LineBox *_parent, int _mask, int _mask2 = 0) :
		parent(_parent), mask(_mask), mask2(_mask2)
	{ }
};


//
// UI_LineBox Constructor
//
UI_LineBox::UI_LineBox(Instance &inst, int X, int Y, int W, int H, const char *label) :
    MapItemBox(inst, X, Y, W, H, label)
{
	//Fl_Scroll *scroll = new Fl_Scroll(X, Y, W, H);
	box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);

	panel = new UI_StackPanel(X, Y, W, H);
	panel->margin(INSET_LEFT, INSET_TOP, INSET_RIGHT, INSET_BOTTOM);
	panel->spacing(INPUT_SPACING);

	// Put this spacer here so the scroller places the UI elements correctly when content collapses.
	//Fl_Box *spacer = new Fl_Box(X, Y, W, INSET_TOP);
	//spacer->box(FL_FLAT_BOX);

	X += INSET_LEFT;

	W -= INSET_LEFT + INSET_RIGHT;
	H -= INSET_TOP + INSET_BOTTOM;

	which = new UI_Nombre(X + NOMBRE_INSET, 0, W - 2 * NOMBRE_INSET, NOMBRE_HEIGHT, "Linedef");
	panel->afterSpacing(which, SPACING_BELOW_NOMBRE - INPUT_SPACING);

	// Prepare before calling fl_width
	fl_font(FL_HELVETICA, FLAG_LABELSIZE);

	{
		Fl_Flex *type_flex = new Fl_Flex(X + TYPE_INPUT_X, 0, W - TYPE_INPUT_X - NOMBRE_INSET,
										 TYPE_INPUT_HEIGHT, Fl_Flex::HORIZONTAL);
		type_flex->spacing(BUTTON_SPACING);
		type_flex->label("Type:");
		type_flex->align(FL_ALIGN_LEFT);

		type = new UI_DynInput(0, 0, 0, 0);
		type->align(FL_ALIGN_LEFT);
		type->callback(type_callback, this);
		type->callback2(dyntype_callback, this);
		type->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		type->type(FL_INT_INPUT);
		type_flex->fixed(type, TYPE_INPUT_WIDTH);

		choose = new Fl_Button(0, 0, 0, 0, "Choose");
		choose->callback(button_callback, this);
		type_flex->fixed(choose, fl_width(choose->label()) + 16);

		gen = new Fl_Button(0, 0, 0, 0, "Gen");
		gen->callback(button_callback, this);
		type_flex->fixed(gen, fl_width(gen->label()) + 16);

		type_flex->end();
	}

	{
		descFlex = new Fl_Flex(X + TYPE_INPUT_X, 0, W - TYPE_INPUT_X - NOMBRE_INSET,
							   TYPE_INPUT_HEIGHT, Fl_Flex::HORIZONTAL);
		descFlex->label("Desc:");
		descFlex->spacing(BUTTON_SPACING);
		descFlex->align(FL_ALIGN_LEFT);

		actkind = new Fl_Choice(0, 0, 0, 0);
		// this order must match the SPAC_XXX constants
		actkind->add(getActivationMenuString());
		actkind->value(getActivationCount());
		actkind->callback(flags_callback, new line_flag_CB_data_c(this, MLF_Activation | MLF_Repeatable));
		actkind->deactivate();
		actkind->hide();

		udmfActivationButton = new Fl_Menu_Button(0, 0, 0, 0, UDMF_ACTIVATION_BUTTON_LABEL);
		udmfActivationButton->labelsize(FLAG_LABELSIZE);
		udmfActivationButton->hide();

		descFlex->fixed(actkind, TYPE_INPUT_WIDTH);
		descFlex->fixed(udmfActivationButton, TYPE_INPUT_WIDTH);

		desc = new Fl_Output(0, 0, 0, 0);
		desc->align(FL_ALIGN_LEFT);

		descFlex->end();
	}

	{
		healthFlex = new Fl_Flex(X + TYPE_INPUT_X, 0, W - TYPE_INPUT_X - NOMBRE_INSET, TYPE_INPUT_HEIGHT, Fl_Flex::HORIZONTAL);
		healthFlex->gap(16 + fl_width("Group:"));

		healthInput = new UI_DynIntInput(0, 0, 0, 0, "Health:");
		healthInput->align(FL_ALIGN_LEFT);
		healthInput->callback(field_callback, this);

		healthGroupInput = new UI_DynIntInput(0, 0, 0, 0, "Group:");
		healthGroupInput->align(FL_ALIGN_LEFT);
		healthGroupInput->callback(field_callback, this);

		healthFlex->end();
	}
	healthFlex->hide();

	{
		argsFlex = new Fl_Flex(which->x(), 0, which->w(), TYPE_INPUT_HEIGHT, Fl_Flex::HORIZONTAL);
		argsFlex->gap(INPUT_SPACING);

		args0str = new UI_DynInput(0, 0, 0, 0);
		args0str->callback(args_callback, new line_flag_CB_data_c(this, 5));
		args0str->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
		args0str->hide();
		args0str->align(FL_ALIGN_BOTTOM);
		args0str->labelsize(ARG_LABELSIZE);

		for (int a = 0 ; a < 5 ; a++)
		{
			args[a] = new UI_DynIntInput(0, 0, 0, 0);
			args[a]->callback(args_callback, new line_flag_CB_data_c(this, a));
			args[a]->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
			args[a]->align(FL_ALIGN_BOTTOM);
			args[a]->labelsize(ARG_LABELSIZE);
		}

		argsFlex->end();
		argsFlex->hide();
	}
	panel->afterSpacing(argsFlex, 16 - INPUT_SPACING);

	{
		Fl_Group *tag_pack = new Fl_Group(X + TYPE_INPUT_X, 0, W - TYPE_INPUT_X - NOMBRE_INSET,
										  TYPE_INPUT_HEIGHT);
		static const char lengthLabel[] = "Length:";
//		tag_pack->gap(fl_width(lengthLabel) + 16);

		tag = new UI_DynIntInput(tag_pack->x(), tag_pack->y(), TAG_WIDTH, tag_pack->h(), "Tag:");
		tag->align(FL_ALIGN_LEFT);
		tag->callback(tag_callback, this);
		tag->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

		const int labelWidth = fl_width(lengthLabel) + 8;
		length = new UI_DynIntInput(tag_pack->x() + tag_pack->w() / 2 + labelWidth, tag_pack->y(),
									tag_pack->w() / 2 - labelWidth, tag_pack->h(), lengthLabel);
		length->align(FL_ALIGN_LEFT);
		length->callback(length_callback, this);
		length->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

		tag_pack->end();
	}

	multiTagView = new MultiTagView(inst, [this](){
		redraw();
	}, [this](){
		if (this->inst.edit.Selected->empty())
			return;
		EditOperation op(this->inst.level.basis);
		op.setMessageForSelection("changed other tags", *this->inst.edit.Selected);
		for (sel_iter_c it(*this->inst.edit.Selected) ; !it.done() ; it.next())
		{
			std::set<int> tags = multiTagView->getTags();
			op.changeLinedef(*it, &LineDef::moreIDs, std::move(tags));
		}
	}, X, Y, W, TYPE_INPUT_HEIGHT);
	multiTagView->hide();

	Y += tag->h() + 16;

	{
		Fl_Group *flags_pack = new Fl_Group(X, Y, W, 22);
		Fl_Box *flags = new Fl_Box(FL_FLAT_BOX, X+10, Y, 64, 24, "Flags: ");
		flags->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);


		f_automap = new Fl_Choice(X+W-118, Y, 104, FIELD_HEIGHT, "Map: ");
		f_automap->add(kHardcodedAutoMapMenu);
		f_automap->value(0);
		f_automap_cb_data = std::make_unique<line_flag_CB_data_c>(this, MLF_ALL_AUTOMAP);
		f_automap->callback(flags_callback, f_automap_cb_data.get());

		flags_pack->end();
		panel->beforeSpacing(flags_pack, 8);
	}


	Y += f_automap->h() - 1;

	// Remember where to place dynamic linedef flags
	flagsAreaW = W;

	overrideHeader = new UI_CategoryButton(which->x(), 0, which->w(), FIELD_HEIGHT, "Override settings");
	overrideHeader->setExpanded(false);
	overrideHeader->callback(overrideClicked, this);

	fl_font(FL_HELVETICA, FLAG_LABELSIZE + 2);

	int labelWidth = 0;
	for(const UDMFIntChoiceField &field : udmfIntChoiceFields)
	{
		const int width = fl_width(field.label) + 16;
		if(width > labelWidth)
			labelWidth = width;
	}
	if(labelWidth < TYPE_INPUT_X)
		labelWidth = TYPE_INPUT_X;
	for(const UDMFIntChoiceField &field : udmfIntChoiceFields)
	{
		this->*field.choice = new Fl_Input_Choice(x() + labelWidth, 0, which->w() - labelWidth,
												  FIELD_HEIGHT, field.label);
		(this->*field.choice)->callback(field_callback, this);
		(this->*field.choice)->value("");
		(this->*field.choice)->align(FL_ALIGN_LEFT);
		(this->*field.choice)->hide();
	}

	widgetAfterFlags = overrideHeader;

	// Leave space; dynamic flags will be created in UpdateGameInfo and side boxes moved accordingly
	Y += 29;

	front = new UI_SideBox(inst, x(), Y, w(), 140, 0);

	Y += front->h() + 14;

	panel->beforeSpacing(front, 16);

	back = new UI_SideBox(inst, x(), Y, w(), 140, 1);

	Y += back->h();

	mFixUp.loadFields({healthInput, healthGroupInput, type, length, tag, args[0], args[1], args[2],
		args[3], args[4], args0str});

	//scroll->end();
	panel->end();
	end();

	resizable(nullptr);
}


void UI_LineBox::type_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	// support hexadecimal
	int new_type = (int)strtol(box->type->value(), NULL, 0);

	if (! box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited type of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected) ; !it.done() ; it.next())
		{
			op.changeLinedef(*it, LineDef::F_TYPE, new_type);
		}
	}
}


void UI_LineBox::dyntype_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	if (box->obj < 0)
		return;

	// support hexadecimal
	int new_type = (int)strtol(box->type->value(), NULL, 0);

	const char *gen_desc = box->GeneralizedDesc(new_type);

	if (gen_desc)
	{
		box->desc->value(gen_desc);
	}
	else
	{
		const linetype_t &info = box->inst.conf.getLineType(new_type);
		box->desc->value(info.desc.c_str());
	}

	box->inst.main_win->browser->UpdateGenType(new_type);
}


void UI_LineBox::SetTexOnLine(EditOperation &op, int ld, StringID new_tex, int e_state, int parts)
{
	bool opposite = (e_state & FL_SHIFT);

	const auto L = inst.level.linedefs[ld];

	// handle the selected texture boxes
	if (parts != 0)
	{
		if (L->OneSided())
		{
			if (parts & PART_RT_LOWER)
				op.changeSidedef(L->right, SideDef::F_MID_TEX,   new_tex);
			if (parts & PART_RT_UPPER)
				op.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_tex);
            if (parts & PART_RT_RAIL)
                op.changeSidedef(L->right, SideDef::F_LOWER_TEX, new_tex);

			return;
		}

		if (inst.level.getRight(*L))
		{
			if (parts & PART_RT_LOWER)
				op.changeSidedef(L->right, SideDef::F_LOWER_TEX, new_tex);
			if (parts & PART_RT_UPPER)
				op.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_tex);
			if (parts & PART_RT_RAIL)
				op.changeSidedef(L->right, SideDef::F_MID_TEX,   new_tex);
		}

		if (inst.level.getLeft(*L))
		{
			if (parts & PART_LF_LOWER)
				op.changeSidedef(L->left, SideDef::F_LOWER_TEX, new_tex);
			if (parts & PART_LF_UPPER)
				op.changeSidedef(L->left, SideDef::F_UPPER_TEX, new_tex);
			if (parts & PART_LF_RAIL)
				op.changeSidedef(L->left, SideDef::F_MID_TEX,   new_tex);
		}
		return;
	}

	// middle click : set mid-masked texture on both sides
	if (e_state & FL_BUTTON2)
	{
		if (! L->TwoSided())
			return;

		// convenience: set lower unpeg on first change
		if (! (L->flags & MLF_LowerUnpegged)  &&
		    is_null_tex(inst.level.getRight(*L)->MidTex()) &&
		    is_null_tex(inst.level.getLeft(*L)->MidTex()) )
		{
			op.changeLinedef(ld, LineDef::F_FLAGS, L->flags | MLF_LowerUnpegged);
		}

		op.changeSidedef(L->left,  SideDef::F_MID_TEX, new_tex);
		op.changeSidedef(L->right, SideDef::F_MID_TEX, new_tex);
		return;
	}

	// one-sided case: set the middle texture only
	if (! L->TwoSided())
	{
		int sd = (L->right >= 0) ? L->right : L->left;

		if (sd < 0)
			return;

		op.changeSidedef(sd, SideDef::F_MID_TEX, new_tex);
		return;
	}

	// modify an upper texture
	int sd1 = L->right;
	int sd2 = L->left;

	if (e_state & FL_BUTTON3)
	{
		// back ceiling is higher?
		if (inst.level.getSector(*inst.level.getLeft(*L)).ceilh > inst.level.getSector(*inst.level.getRight(*L)).ceilh)
			std::swap(sd1, sd2);

		if (opposite)
			std::swap(sd1, sd2);

		op.changeSidedef(sd1, SideDef::F_UPPER_TEX, new_tex);
	}
	// modify a lower texture
	else
	{
		// back floor is lower?
		if (inst.level.getSector(*inst.level.getLeft(*L)).floorh < inst.level.getSector(*inst.level.getRight(*L)).floorh)
			std::swap(sd1, sd2);

		if (opposite)
			std::swap(sd1, sd2);

		const auto S = inst.level.sidedefs[sd1];

		// change BOTH upper and lower when they are the same
		// (which is great for windows).
		//
		// Note: we only do this for LOWERS (otherwise it'd be
		// impossible to set them to different textures).

		if (S->lower_tex == S->upper_tex)
			op.changeSidedef(sd1, SideDef::F_UPPER_TEX, new_tex);

		op.changeSidedef(sd1, SideDef::F_LOWER_TEX, new_tex);
	}
}

//
// Check sidedefs dirty fields
//
void UI_LineBox::checkSidesDirtyFields()
{
	front->checkDirtyFields();
	back->checkDirtyFields();
}

void UI_LineBox::overrideClicked(Fl_Widget *widget, void *context)
{
	auto box = static_cast<UI_LineBox *>(context);

	if(box->overrideHeader->isExpanded())
	{
		for(const UDMFIntChoiceField &entry : udmfIntChoiceFields)
		{
			if(UDMF_HasLineFeature(box->inst.conf, entry.feature))
				(box->*entry.choice)->show();
			else
				(box->*entry.choice)->hide();
		}
	}
	else for(const UDMFIntChoiceField &entry : udmfIntChoiceFields)
		(box->*entry.choice)->hide();
	box->redraw();
}

void UI_LineBox::clearArgs()
{
	for (int a = 0 ; a < 5 ; a++)
	{
		mFixUp.setInputValue(args[a], "");
		args[a]->label("");
		args[a]->textcolor(FL_BLACK);
		args0str->textcolor(FL_BLACK);
	}
	if(inst.loaded.levelFormat == MapFormat::hexen || inst.loaded.levelFormat == MapFormat::udmf)
	{
		args0str->hide();
		args[0]->show();
	}
}

void UI_LineBox::clearFields()
{
	mFixUp.setInputValue(type, "");
	desc->value("");
	actkind->value(getActivationCount());  // show as "??"
	actkind->deactivate();
	udmfActivationButton->label(UDMF_ACTIVATION_BUTTON_LABEL);
	udmfActivationButton->deactivate();
	for(Fl_Menu_Item &item : udmfActivationMenuItems)
		item.value(0);

	healthFlex->hide();

	mFixUp.setInputValue(length, "");
	mFixUp.setInputValue(tag, "");
	clearArgs();

	if(inst.loaded.levelFormat == MapFormat::udmf && inst.conf.features.udmf_multipletags)
	{
		multiTagView->clearTags();
		multiTagView->deactivate();
	}

	if(alphaWidget)
	{
		alphaWidget->value(1.0);
		for(Fl_Check_Button *button : udmfTranslucencyCheckBoxes)
			if(button)
				button->labelcolor(FL_FOREGROUND_COLOR);
	}


	front->SetObj(SETOBJ_NO_LINE, 0, false);
	back->SetObj(SETOBJ_NO_LINE, 0, false);

	inst.main_win->browser->UpdateGenType(0);

	f_automap->value(0);
	for(LineFlagButton &button : flagButtons)
	{
		if(!button.button)
			continue;
		button.button->value(0);
	}
	updateFlagDependencyGreying();

	for(const UDMFIntChoiceField &entry : udmfIntChoiceFields)
	{
		(this->*entry.choice)->deactivate();
		(this->*entry.choice)->value("");
	}

	for(const CategoryHeader &header : categoryHeaders)
		if(header.button)
			updateCategorySummary(header);
	updateOverrideSummary();
}

//
// Update the summary details displayed on a collapsed category header
//
void UI_LineBox::updateCategorySummary(const CategoryHeader &header)
{
	if(!header.button)
		return;

	if(!inst.level.isLinedef(obj))
	{
		header.button->details("");
		header.button->redraw();
		return;
	}
	const int flags = inst.level.linedefs[obj]->flags;
	const int flags2 = inst.level.linedefs[obj]->flags2;

	double alpha = inst.level.linedefs[obj]->alpha;
	bool hasAlpha = header.hasAlpha && UDMF_HasLineFeature(inst.conf, UDMF_LineFeature::alpha) &&
		alpha < 1.0f;

	SString summary;
	if(hasAlpha)
		summary = SString::printf("A=%.2g", alpha);
	for(size_t i = 0; i < header.mappingCount; ++i)
	{
		const FeatureFlagMapping &entry = header.mapping[i];
		if(!UDMF_HasLineFeature(inst.conf, entry.feature) ||
		   (hasAlpha && (entry.feature == UDMF_LineFeature::translucent ||
						 entry.feature == UDMF_LineFeature::transparent)))
		{
			continue;
		}
		const int checkFlags = entry.flagSet == 1 ? flags : flags2;
		if(checkFlags & entry.value)
			summary += entry.shortDisplay;
	}

	header.button->details(summary);
	header.button->redraw();
}

//
// Update the summary for the override settings category
//
void UI_LineBox::updateOverrideSummary()
{
	if(!inst.level.isLinedef(obj))
	{
		overrideHeader->details("");
		overrideHeader->redraw();
		return;
	}

	const auto& L = inst.level.linedefs[obj];
	std::vector<SString> activeOverrides;

	// Check each override field
	SString summary;
	for(const UDMFIntChoiceField &entry : udmfIntChoiceFields)
	{
		if(!UDMF_HasLineFeature(inst.conf, entry.feature))
			continue;

		int value = L.get()->*entry.linedefField;
		if(!value)
			continue;

		if(!summary.empty())
			summary.push_back(' ');

		if(*entry.label)
			summary.push_back(*entry.label);
		summary.push_back('=');
		summary += SString(value);
	}

	overrideHeader->details(summary);
	overrideHeader->redraw();
}

bool UI_LineBox::populateUDMFFlagCheckBoxes(const FeatureFlagMapping *mapping, size_t count,
											const LoadingData &loaded, const ConfigData &config,
											const char *title)
{
	if(loaded.levelFormat != MapFormat::udmf)
		return false;

	// Check if features exist at all before proceeding
	int countFound = 0;
	for(size_t i = 0; i < count; ++i)
	{
		const FeatureFlagMapping &entry = mapping[i];
		if(UDMF_HasLineFeature(config, entry.feature))
			++countFound;
	}
	if(!countFound)
		return false;

	CategoryHeader header{};
	const int xPos = which->x();
	const int wSize = which->w();
	if(title)
	{
		header.button = new UI_CategoryButton(xPos, 0, wSize, FIELD_HEIGHT, title);
		header.button->callback(category_callback, this);
	}
	const int numRows = (int)(countFound + 1) / 2;
	header.grid = new Fl_Grid(xPos + 8, 0, wSize - 8, FLAG_ROW_HEIGHT * numRows);
	header.grid->layout(numRows, 2);

	int index = 0;
	for(size_t i = 0; i < count; ++i)
	{
		const FeatureFlagMapping &entry = mapping[i];
		if(!UDMF_HasLineFeature(config, entry.feature))
			continue;
		LineFlagButton button{};
		button.button = new Fl_Check_Button(0, 0, 0, 0, entry.label);
		button.button->labelsize(FLAG_LABELSIZE);
		button.button->tooltip(entry.tooltip);
		button.data = std::make_unique<line_flag_CB_data_c>(
			this, entry.flagSet == 1 ? entry.value : 0, entry.flagSet == 2 ? entry.value : 0
		);
		button.button->callback(flags_callback, button.data.get());

		// Populate feature-to-button lookup array for fast access
		const size_t featureIndex = static_cast<size_t>(entry.feature);
		mFeatureToButton[featureIndex] = button.button;

		flagButtons.push_back(std::move(button));

		header.grid->add(button.button);
		header.grid->widget(button.button, index % numRows, index / numRows);
		header.lineFlagButtonIndices.push_back(static_cast<int>(flagButtons.size()) - 1);
		++index;
	}

	if(title)
		panel->insert(*header.button, widgetAfterFlags);
	panel->insert(*header.grid, widgetAfterFlags);

	header.mapping = mapping;
	header.mappingCount = count;
	categoryHeaders.push_back(std::move(header));
	return true;
}

void UI_LineBox::updateFlagDependencyGreying()
{
	if(inst.loaded.levelFormat != MapFormat::udmf)
		return;
	if(!inst.level.isLinedef(obj))
	{
		for(size_t i = 0; i < (size_t)UDMF_LineFeature::COUNT; ++i)
			if(mFeatureToButton[i])
			{
				mFeatureToButton[i]->labelcolor(FL_FOREGROUND_COLOR);
				mFeatureToButton[i]->redraw_label();
			}
		return;
	}

	// Helper lambda to check if a flag is currently set
	auto isFlagSet = [&](const UDMF_LineFeature feature) -> bool
	{
		const size_t index = static_cast<size_t>(feature);
		const Fl_Check_Button *button = mFeatureToButton[index];
		return button && !!button->value();
	};

	// First pass: collect all flags that should be greyed (any master condition met)
	bool shouldGreyFlag[static_cast<size_t>(UDMF_LineFeature::COUNT)] = {};

	for(const FlagDependency &dep : kFlagDependencies)
	{
		// Check if the master flag meets the condition
		const bool masterSet = isFlagSet(dep.masterFlag);
		const bool masterConditionMet = dep.mustBeOn == masterSet;

		if(!masterConditionMet)
			continue;

		// Mark all dependent flags for greying
		for(const UDMF_LineFeature dependentFeature : dep.dependentFlags)
		{
			const size_t index = static_cast<size_t>(dependentFeature);
			shouldGreyFlag[index] = true;
		}
	}

	// Second pass: apply greying based on collected flags
	for(size_t i = 0; i < static_cast<size_t>(UDMF_LineFeature::COUNT); i++)
	{
		Fl_Check_Button *const button = mFeatureToButton[i];
		if(!button)
			continue;

		button->labelcolor(shouldGreyFlag[i] ? FL_INACTIVE_COLOR : FL_FOREGROUND_COLOR);
		button->redraw_label();
	}
}

void UI_LineBox::loadUDMFBaseFlags(const LoadingData &loaded, const ConfigData &config)
{
	static const FeatureFlagMapping baseFlags[] =
	{
		{UDMF_LineFeature::twosided, "two sided", "Line behaves properly as two-sided",
		MLF_TwoSided, 1},
		{UDMF_LineFeature::dontpegtop, "upper unpeg", "Upper texture tiles from the top",
		MLF_UpperUnpegged, 1},
		{UDMF_LineFeature::dontpegbottom, "lower unpeg",
		"Lower texture tiles starting from ceiling", MLF_LowerUnpegged, 1},
		{UDMF_LineFeature::blocking, "impassable", "Classic Doom wall or railing", MLF_Blocking, 1},
		{UDMF_LineFeature::blockmonsters, "block monsters", "Blocks only enemy monsters",
		MLF_BlockMonsters, 1},
		{UDMF_LineFeature::blocksound, "sound block", "Half-blocks alert propagation",
		MLF_SoundBlock, 1},
		{UDMF_LineFeature::passuse, "pass thru", "Allows the 'use' activation to pass through",
		MLF_Boom_PassThru, 1},
	};

	populateUDMFFlagCheckBoxes(baseFlags, lengthof(baseFlags), loaded, config, nullptr);
}

void UI_LineBox::loadUDMFActivationMenu(const LoadingData &loaded, const ConfigData &config)
{
	udmfActivationButton->clear();
	udmfActivationMenuItems.clear();
	switch(loaded.levelFormat)
	{
		case MapFormat::udmf:
			break;
		case MapFormat::doom:
		case MapFormat::hexen:
			udmfActivationButton->hide();
			return;
		default:
			return;
	}

	// We have it, so let's do it.


	// Description source: https://github.com/ZDoom/gzdoom/blob/g4.14.2/specs/udmf_zdoom.txt
	static const FeatureFlagMapping activatorAndMode[] =
	{
		{ UDMF_LineFeature::playercross, "Player crossing",
		"Player crossing will trigger this line", MLF_UDMF_PlayerCross, 1},
		{ UDMF_LineFeature::playeruse, "Player using", "Player operating will trigger this line",
	    MLF_UDMF_PlayerUse, 1},
		{ UDMF_LineFeature::playerpush, "Player bumping", "Player bumping will trigger this line",
		MLF_UDMF_PlayerPush, 1},
		{ UDMF_LineFeature::monstercross, "Monster crossing",
		"Monster crossing will trigger this line", MLF_UDMF_MonsterCross, 1},
		{ UDMF_LineFeature::monsteruse, "Monster using", "Monster operating will trigger this line",
		MLF_UDMF_MonsterUse, 1},
		{ UDMF_LineFeature::monsterpush, "Monster bumping",
		"Monster pushing will trigger this line", MLF_UDMF_MonsterPush, 1},
		{ UDMF_LineFeature::impact, "On gunshot",
		"Shooting bullets will trigger this line", MLF_UDMF_Impact, 1},
		{ UDMF_LineFeature::missilecross, "Projectile crossing",
		"Projectile crossing will trigger this line", MLF_UDMF_MissileCross, 1},
		{ UDMF_LineFeature::anycross, "Anything crossing",
		"Any non-projectile crossing will trigger this line", MLF2_UDMF_AnyCross, 2},
	};

	static const FeatureFlagMapping mainFlags[] =
	{
		{ UDMF_LineFeature::repeatspecial, "Repeatable activation", "Allow retriggering",
		MLF_UDMF_RepeatSpecial, 1},
		{ UDMF_LineFeature::checkswitchrange, "Check switch range",
		"Switch can only be activated when vertically reachable", MLF2_UDMF_CheckSwitchRange, 2},
		{ UDMF_LineFeature::firstsideonly, "First side only",
		"Line can only be triggered from the front side", MLF2_UDMF_FirstSideOnly, 2},
		{ UDMF_LineFeature::playeruseback, "Player can use from behind",
		"Player can use from back (left) side", MLF2_UDMF_PlayerUseBack, 2},
		{ UDMF_LineFeature::monsteractivate, "Monster can activate",
		"Monsters can trigger this line (for compatibility only)", MLF2_UDMF_MonsterActivate, 2},
	};

	static const FeatureFlagMapping destructibleFlags[] =
	{
		{ UDMF_LineFeature::damagespecial, "Trigger on damage",
		"This line will call special if having health > 0 and receiving damage",
		MLF2_UDMF_DamageSpecial, 2},
		{ UDMF_LineFeature::deathspecial, "Trigger on death",
		"This line will call special if health was reduced to 0", MLF2_UDMF_DeathSpecial, 2},
	};

	auto addItem = [this, &config](const FeatureFlagMapping &entry, bool &addSeparator)
	{
		if(!UDMF_HasLineFeature(config, entry.feature))
			return;

		if(addSeparator && !udmfActivationMenuItems.empty())
		{
			udmfActivationMenuItems.back().flags |= FL_MENU_DIVIDER;
			addSeparator =false;
		}

		Fl_Menu_Item item = {};
		item.flags = FL_MENU_TOGGLE;
		item.text = entry.label;
		// FIXME: tooltip

		LineFlagButton button{};
		button.udmfActivationMenuIndex = static_cast<int>(udmfActivationMenuItems.size());
		button.data = std::make_unique<line_flag_CB_data_c>(
			this, entry.flagSet == 1 ? entry.value : 0, entry.flagSet == 2 ? entry.value : 0
		);
		flagButtons.push_back(std::move(button));
		item.callback(flags_callback, flagButtons.back().data.get());
		udmfActivationMenuItems.push_back(std::move(item));
	};

	bool addSeparator = false;
	for(const FeatureFlagMapping &entry : activatorAndMode)
		addItem(entry, addSeparator);
	addSeparator = true;
	for(const FeatureFlagMapping &entry : mainFlags)
		addItem(entry, addSeparator);
	addSeparator = true;
	for(const FeatureFlagMapping &entry : destructibleFlags)
		addItem(entry, addSeparator);
	udmfActivationMenuItems.emplace_back();
	if(udmfActivationMenuItems.size() <= 1)
		udmfActivationButton->hide();
	else
	{
		udmfActivationButton->show();
		udmfActivationButton->menu(udmfActivationMenuItems.data());
	}
}

void UI_LineBox::loadUDMFBlockingFlags(const LoadingData &loaded, const ConfigData &config)
{
	static const FeatureFlagMapping blockingFlags[] =
	{
		{UDMF_LineFeature::blockeverything, "block everything", "Acts as a solid wall",
			MLF2_UDMF_BlockEverything, 2, "🚫"},
		{UDMF_LineFeature::midtex3d, "3DMidTex",
		"Eternity 3D middle texture: solid railing which can be walked over and under",
		MLF_Eternity_3DMidTex, 1, "🪜"},
		{UDMF_LineFeature::midtex3dimpassible, "3DMidTex+missiles",
		"Coupled with 3DMidTex, this also allows projectiles to pass", MLF2_UDMF_MidTex3DImpassible,
		2, "+"},
		{UDMF_LineFeature::jumpover, "Strife railing", "Strife-style skippable railing",
		MLF_UDMF_JumpOver, 1, "🚧"},
		{UDMF_LineFeature::blockfloaters, "block floaters", "Block flying enemies",
		MLF_UDMF_BlockFloaters, 1, "🦇"},
		{UDMF_LineFeature::blocklandmonsters, "block land monsters", "Block walking enemies",
		MLF_UDMF_BlockLandMonsters, 1, "🧌"},
		{UDMF_LineFeature::blockplayers, "block players", "Block only players",
			MLF_UDMF_BlockPlayers, 1, "👤"},
		{UDMF_LineFeature::blockhitscan, "block gunshots",
		"Blocks hitscans (infinite speed gunshots)", MLF_UDMF_BlockHitScan, 1, "🔫"},
		{UDMF_LineFeature::blockprojectiles, "block projectiles", "Blocks finite-speed projectiles",
		MLF_UDMF_BlockProjectiles, 1, "🚀"},
		{UDMF_LineFeature::blocksight, "block sight", "Blocks monster sight", MLF_UDMF_BlockSight,
		1, "👁️"},
		{UDMF_LineFeature::blockuse, "block use", "Blocks 'use' operation", MLF_UDMF_BlockUse, 1,
			"🚪"},
	};

	populateUDMFFlagCheckBoxes(blockingFlags, lengthof(blockingFlags), loaded, config,
	"Advanced blocking");
}

void UI_LineBox::loadUDMFRenderingControls(const LoadingData &loaded, const ConfigData &config)
{
	if(loaded.levelFormat != MapFormat::udmf)
		return;

	static const FeatureFlagMapping renderingFlags[] =
	{
		{UDMF_LineFeature::translucent, "translucent", "Render line as translucent (Strife style)",
		MLF_UDMF_Translucent, 1, "🪟"},
		{UDMF_LineFeature::transparent, "more translucent",
		"Render line as even more translucent (Strife style)", MLF_UDMF_Transparent, 1, "🌫️"},
		{UDMF_LineFeature::clipmidtex, "clip middle texture",
		"Always clip two-sided middle texture to floor and ceiling, even if sector properties don't change",
		MLF2_UDMF_ClipMidTex, 2, "✂️"},
		{UDMF_LineFeature::wrapmidtex, "tile middle texture", "Repeat two-sided middle texture vertically",
		MLF2_UDMF_WrapMidTex, 2, "🔲"},
	};

	if(!populateUDMFFlagCheckBoxes(renderingFlags, lengthof(renderingFlags), loaded, config,
								   "Advanced rendering"))
	{
		return;
	}

	Fl_Grid *grid = categoryHeaders.back().grid;

	if(UDMF_HasLineFeature(config, UDMF_LineFeature::alpha))
	{
		categoryHeaders.back().hasAlpha = true;
		int count = 0;
		for(int index : categoryHeaders.back().lineFlagButtonIndices)
		{
			LineFlagButton &button = flagButtons[index];
			if(!button.data || !(button.data->mask & (MLF_UDMF_Translucent | MLF_UDMF_Transparent)))
				continue;
			udmfTranslucencyCheckBoxes[count++] = button.button;
			if(count >= (int)lengthof(udmfTranslucencyCheckBoxes))
				break;
		}
		grid->layout(grid->rows() + 1, grid->cols());
		grid->size(grid->w(), grid->h() + FLAG_ROW_HEIGHT);

		static const char labelText[] = "Custom alpha:";
		fl_font(FL_HELVETICA, FLAG_LABELSIZE);
		const int margin = fl_width(labelText);

		Fl_Flex *spacerFlex = new Fl_Flex(0, 0, 0, 0);
		spacerFlex->margin(margin, 0, 0, 0);

		// Provide a nonzero placeholder width to prevent value_width below from getting truncated
		Fl_Hor_Value_Slider *slider = new Fl_Hor_Value_Slider(0, 0, 2 * ALPHA_NUMBER_WIDTH, 0,
															  labelText);
		alphaWidget = slider;
		slider->align(FL_ALIGN_LEFT);
		slider->labelsize(FLAG_LABELSIZE);
		slider->step(0.015625);
		slider->minimum(0);
		slider->maximum(1);
		slider->value_width(ALPHA_NUMBER_WIDTH);
		slider->callback(field_callback, this);
		slider->tooltip("Custom translucency");
		slider->when(FL_WHEN_RELEASE);
		spacerFlex->end();

		grid->add(spacerFlex);
		grid->widget(spacerFlex, grid->rows() - 1, 0, 1, 2);
	}
}

int UI_LineBox::getActivationCount() const
{
	return inst.conf.features.player_use_passthru_activation ? 14 : 12;
}
const char *UI_LineBox::getActivationMenuString() const
{
	return inst.conf.features.player_use_passthru_activation ?
		"W1|WR|S1|SR|M1|MR|G1|GR|P1|PR|X1|XR|S1+|SR+|??" :
		"W1|WR|S1|SR|M1|MR|G1|GR|P1|PR|X1|XR|??";
}

void UI_LineBox::SetTexture(const char *tex_name, int e_state, int uiparts)
{
	StringID new_tex = BA_InternaliseString(tex_name);

	// this works a bit differently than other ways, we don't modify a
	// widget and let it update the map, instead we update the map and
	// let the widget(s) get updated.  That's because we do different
	// things depending on whether the line is one-sided or two-sided.

	if (! inst.edit.Selected->empty())
	{
        // WARNING: translate uiparts to be valid for a one-sided line
        if(inst.level.isLinedef(obj) && inst.level.linedefs[obj]->OneSided())
        {
            uiparts = (uiparts & PART_RT_UPPER) |
                      (uiparts & PART_RT_LOWER ? PART_RT_RAIL : 0) |
                      (uiparts & PART_RT_RAIL ? PART_RT_LOWER : 0);
        }

        mFixUp.checkDirtyFields();
		checkSidesDirtyFields();

		EditOperation op(inst.level.basis);
		op.setMessageForSelection("edited texture on", *inst.edit.Selected);

		for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
		{
			int p2 = inst.edit.Selected->get_ext(*it);

			// only use parts explicitly selected in 3D view when no
			// parts in the linedef panel are selected.
			if (! (uiparts == 0 && p2 > 1))
				p2 = uiparts;

			SetTexOnLine(op, *it, new_tex, e_state, p2);
		}
	}

	redraw();
}


void UI_LineBox::SetLineType(int new_type)
{
	if (obj < 0)
	{
///		Beep("No lines selected");
		return;
	}

	auto buffer = SString(new_type);

	mFixUp.checkDirtyFields();
	checkSidesDirtyFields();

	mFixUp.setInputValue(type, buffer.c_str());
	type->do_callback();
}


void UI_LineBox::CB_Copy(int uiparts)
{
	// determine which sidedef texture to grab from
	const char *name = NULL;

	mFixUp.checkDirtyFields();
	checkSidesDirtyFields();

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		UI_SideBox *SD = (pass == 0) ? front : back;

		for (int b = 0 ; b < 3 ; b++)
		{
			int try_part = PART_RT_LOWER << (b + pass * 4);

			if ((uiparts & try_part) == 0)
				continue;

			const char *b_name = (b == 0) ? SD->l_panel->getTex()->value() :
								 (b == 1) ? SD->u_panel->getTex()->value() :
											SD->r_panel->getTex()->value();
			SYS_ASSERT(b_name);

			if (name && y_stricmp(name, b_name) != 0)
			{
				inst.Beep("multiple textures");
				return;
			}

			name = b_name;
		}
	}

	Texboard_SetTex(name, inst.conf);

	inst.Status_Set("copied %s", name);
}


void UI_LineBox::CB_Paste(int uiparts, StringID new_tex)
{
	// iterate over selected linedefs
	if (inst.edit.Selected->empty())
		return;

	mFixUp.checkDirtyFields();
	checkSidesDirtyFields();

	{
		EditOperation op(inst.level.basis);
		op.setMessage("pasted %s", BA_GetString(new_tex).c_str());

		for (sel_iter_c it(*inst.edit.Selected) ; !it.done() ; it.next())
		{
			const auto L = inst.level.linedefs[*it];

			for (int pass = 0 ; pass < 2 ; pass++)
			{
				int sd = (pass == 0) ? L->right : L->left;
				if (sd < 0)
					continue;

				int uiparts2 = pass ? (uiparts >> 4) : uiparts;

                // WARNING: different meaning of lower/railing between
                // UI panel and elsewhere. Here we know it's UI
                if (uiparts2 & PART_RT_LOWER)
                    op.changeSidedef(sd, SideDef::F_LOWER_TEX, new_tex);

                if (uiparts2 & PART_RT_UPPER)
                    op.changeSidedef(sd, SideDef::F_UPPER_TEX, new_tex);

                if (uiparts2 & PART_RT_RAIL)
                    op.changeSidedef(sd, SideDef::F_MID_TEX, new_tex);
			}
		}
	}

	redraw();
}


bool UI_LineBox::ClipboardOp(EditCommand op)
{
	if (obj < 0)
		return false;

	int uiparts = front->GetSelectedPics() | (back->GetSelectedPics() << 4);

	if (uiparts == 0)
		uiparts = front->GetHighlightedPics() | (back->GetHighlightedPics() << 4);

	if (uiparts == 0)
		return false;

	switch (op)
	{
		case EditCommand::copy:
			CB_Copy(uiparts);
			break;

		case EditCommand::paste:
			CB_Paste(uiparts, Texboard_GetTexNum(inst.conf));
			break;

		case EditCommand::cut:	// Cut
			CB_Paste(uiparts, BA_InternaliseString(inst.conf.default_wall_tex));
			break;

		case EditCommand::del: // Delete
			CB_Paste(uiparts, BA_InternaliseString("-"));
			break;
	}

	return true;
}


void UI_LineBox::BrowsedItem(BrowserMode kind, int number, const char *name, int e_state)
{
	if (kind == BrowserMode::textures || kind == BrowserMode::flats)
	{
		int front_pics = front->GetSelectedPics();
		int  back_pics =  back->GetSelectedPics();

		// this can be zero, invoking special behavior (based on mouse button)
		int uiparts = front_pics | (back_pics << 4);

		SetTexture(name, e_state, uiparts);
	}
	else if (kind == BrowserMode::lineTypes)
	{
		SetLineType(number);
	}
}


void UI_LineBox::tag_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	int new_tag = atoi(box->tag->value());

	if (! box->inst.edit.Selected->empty())
		box->inst.level.checks.tagsApplyNewValue(new_tag);
}


void UI_LineBox::flags_callback(Fl_Widget *w, void *data)
{
	line_flag_CB_data_c *l_f_c = (line_flag_CB_data_c *)data;

	UI_LineBox *box = l_f_c->parent;

	int mask = l_f_c->mask;
	int mask2 = l_f_c->mask2;
	int new_flags, new_flags2;
	box->CalcFlags(new_flags, new_flags2);

	if (! box->inst.edit.Selected->empty())
	{
		box->mFixUp.checkDirtyFields();
		box->checkSidesDirtyFields();

		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited flags of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			const auto L = box->inst.level.linedefs[*it];

			// only change the bits specified in 'mask'.
			// this is important when multiple linedefs are selected.
			if(mask != 0)
			{
				op.changeLinedef(*it, LineDef::F_FLAGS, (L->flags & ~mask) | (new_flags & mask));
			}
			if(mask2 != 0)
			{
				op.changeLinedef(*it, LineDef::F_FLAGS2, (L->flags2 & ~mask2) | (new_flags2 & mask2));
			}
		}
	}

	// Respawn the menu if user clicks an option, to make it look more like a popover panel
	if(w == box->udmfActivationButton)
		box->udmfActivationButton->popup();
}


void UI_LineBox::args_callback(Fl_Widget *w, void *data)
{
	line_flag_CB_data_c *l_f_c = (line_flag_CB_data_c *)data;

	UI_LineBox *box = l_f_c->parent;

	int arg_idx = l_f_c->mask;
	int new_value;
	if(arg_idx == 5)
		new_value = BA_InternaliseString(box->args0str->value()).get();
	else
	{
		new_value = atoi(box->args[arg_idx]->value());
		if(box->inst.loaded.levelFormat != MapFormat::udmf)
			new_value = clamp(0, new_value, 255);
	}

	if (! box->inst.edit.Selected->empty())
	{
		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited args of", *box->inst.edit.Selected);

		for (sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
		{
			int paramIndex = arg_idx == 5 ? LineDef::F_ARG1STR : LineDef::F_ARG1 + arg_idx;
			op.changeLinedef(*it, static_cast<byte>(paramIndex), new_value);

			// Also change the tag when outside of UDMF
			if(!arg_idx && box->inst.loaded.levelFormat != MapFormat::udmf)
				op.changeLinedef(*it, LineDef::F_TAG, new_value);
		}
	}
}


void UI_LineBox::length_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	int new_len = atoi(box->length->value());

	// negative values are allowed, it moves the start vertex
	new_len = clamp(-32768, new_len, 32768);

	box->inst.level.linemod.setLinedefsLength(new_len);
}


void UI_LineBox::button_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;

	if (w == box->choose)
	{
		box->inst.main_win->BrowserMode(BrowserMode::lineTypes);
		return;
	}

	if (w == box->gen)
	{
		box->inst.main_win->BrowserMode(BrowserMode::generalized);
		return;
	}
}

void UI_LineBox::category_callback(Fl_Widget *w, void *data)
{
	UI_LineBox *box = (UI_LineBox *)data;
	UI_CategoryButton *categoryBtn = (UI_CategoryButton *)w;
	box->categoryToggled(categoryBtn);
}

void UI_LineBox::field_callback(Fl_Widget *w, void *data)
{
	struct IntInputMapping
	{
		const UI_DynIntInput *input;
		const char *name;
		byte fieldID;
	};

	UI_LineBox *box = (UI_LineBox *)data;

	if(box->inst.edit.Selected->empty())
		return;

	if(w == box->alphaWidget)
	{
		box->checkDirtyFields();
		box->checkSidesDirtyFields();

		EditOperation op(box->inst.level.basis);
		op.setMessageForSelection("edited alpha of", *box->inst.edit.Selected);
		for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			op.changeLinedef(*it, &LineDef::alpha, box->alphaWidget->value());
		return;
	}

	const IntInputMapping intInputMapping[] =
	{
		{ box->healthInput, "health", LineDef::F_HEALTH },
		{ box->healthGroupInput, "health group", LineDef::F_HEALTHGROUP },
	};

	for(const IntInputMapping &mapping : intInputMapping)
	{
		if(w != mapping.input)
			continue;
		EditOperation op(box->inst.level.basis);
		SString msg = SString::printf("edited %s of", mapping.name);
		op.setMessageForSelection(msg.c_str(), *box->inst.edit.Selected);
		for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
			op.changeLinedef(*it, mapping.fieldID, atoi(mapping.input->value()));
		return;
	}

	for(const UDMFIntChoiceField &entry : udmfIntChoiceFields)
	{
		if(w != box->*entry.choice)
			continue;
		box->checkDirtyFields();
		box->checkSidesDirtyFields();

		int new_value = atoi((box->*entry.choice)->value());
		box->inCallbackChoice = w;
		{
			EditOperation op(box->inst.level.basis);
			SString msg = SString::printf("edited %s of", entry.identifier);
			op.setMessageForSelection(msg.c_str(), *box->inst.edit.Selected);
			for(sel_iter_c it(*box->inst.edit.Selected); !it.done(); it.next())
				op.changeLinedef(*it, entry.fieldID, new_value);
		}
		box->inCallbackChoice = nullptr;
		return;
	}
}

void UI_LineBox::categoryToggled(UI_CategoryButton *categoryBtn)
{
	// Find which category was toggled
	for(auto &cat : categoryHeaders)
	{
		if(!cat.button)
			continue;
		if(!categoryBtn || cat.button == categoryBtn)
		{
			if(cat.button->isExpanded())
				cat.grid->show();
			else
				cat.grid->hide();

			redraw();
			break;
		}
	}
}
//------------------------------------------------------------------------

void UI_LineBox::UpdateField(std::optional<Basis::EditField> efield)
{
	if(!inst.level.isLinedef(obj))
	{
		clearFields();
		return;
	}

	CalcLength();

	clearArgs();

	const auto& L = inst.level.linedefs[obj];
	const int type_num = L->type;
	const linetype_t &info = inst.conf.getLineType(type_num);

	mFixUp.setInputValue(tag, SString(inst.level.linedefs[obj]->tag).c_str());

	if (inst.loaded.levelFormat == MapFormat::hexen || inst.loaded.levelFormat == MapFormat::udmf)
	{
		for (int a = 0 ; a < 5 ; a++)
		{
			int arg_val = L->Arg(1 + a);

			if(arg_val || type_num)
				mFixUp.setInputValue(args[a], SString(arg_val).c_str());

			if(L->arg1str.get())
				mFixUp.setInputValue(args0str, BA_GetString(L->arg1str).c_str());

			// set the label
			if (!info.args[a].name.empty())
			{
				args[a]->copy_label(info.args[a].name.replacing('_', ' ').c_str());
				args[a]->parent()->redraw();
				if(a == 0)
				{
					if(info.args[0].type == SpecialArgType::str)
					{
						args0str->copy_label(args[0]->label());
						args0str->show();
						args[0]->hide();
					}
					else
					{
						args0str->hide();
						args[0]->show();
					}
				}
			}
			else
			{
				args[a]->label("");
				args[a]->textcolor(fl_rgb_color(160,160,160));
				args[a]->parent()->redraw();
				if(a == 0)
				{
					args0str->hide();
					args[0]->show();
				}
			}
		}
	}

	int right_mask = SolidMask(L.get(), Side::right);
	int  left_mask = SolidMask(L.get(), Side::left);

	front->SetObj(L->right, right_mask, L->TwoSided());
	back->SetObj(L->left,   left_mask, L->TwoSided());

	mFixUp.setInputValue(type, SString(type_num).c_str());

	const char *gen_desc = GeneralizedDesc(type_num);

	if (gen_desc)
		desc->value(gen_desc);
	else
		desc->value(info.desc.c_str());

	inst.main_win->browser->UpdateGenType(type_num);

	actkind->activate();
	udmfActivationButton->activate();
	multiTagView->activate();

	FlagsFromInt(L->flags);
	Flags2FromInt(L->flags2);
	setUDMFActivationLabel(L->flags, L->flags2);
	updateFlagDependencyGreying();
	if(inst.loaded.levelFormat == MapFormat::udmf && L->flags2 & (MLF2_UDMF_DamageSpecial |
																  MLF2_UDMF_DeathSpecial))
	{
		healthFlex->show();
		mFixUp.setInputValue(healthInput, SString(L->health).c_str());
		mFixUp.setInputValue(healthGroupInput, SString(L->healthgroup).c_str());
	}
	else
		healthFlex->hide();

	std::unordered_map<SString, const linefield_t *> linefieldMap;
	for(const linefield_t &field : inst.conf.udmf_line_fields)
	{
		for(const UDMFIntChoiceField &entry : udmfIntChoiceFields)
		{
			if(field.identifier.noCaseEqual(entry.identifier))
			{
				linefieldMap[entry.identifier] = &field;
				break;
			}
		}
	}

	if(inst.loaded.levelFormat == MapFormat::udmf)
	{
		for(const UDMFIntChoiceField &entry : udmfIntChoiceFields)
		{
			// If we're in the middle of handling user interaction, do not modify the value at the
			// same time. Otherwise, we autocomplete the text prematurely and impede the user from
			// writing custom numbers.
			// Also, avoid spending computation time if the given feature doesn't exist
			if(inCallbackChoice == this->*entry.choice || !UDMF_HasLineFeature(inst.conf, entry.feature) ||
			   linefieldMap.find(entry.identifier) == linefieldMap.end())
			{
				continue;
			}

			(this->*entry.choice)->activate();

			bool found = false;
			for(const linefield_t::option_t &option : linefieldMap[entry.identifier]->options)
			{
				if(option.value != L.get()->*entry.linedefField)
					continue;
				SString fullEntry = SString(L.get()->*entry.linedefField) + ": " + option.label;
				(this->*entry.choice)->value(fullEntry.c_str());
				found = true;
				break;
			}
			if(!found)
				(this->*entry.choice)->value(SString(L.get()->*entry.linedefField).c_str());
		}
	}

	if(alphaWidget)
	{
		double alpha = L->alpha;
		alphaWidget->value(alpha);
		for(Fl_Check_Button *button : udmfTranslucencyCheckBoxes)
			if(button)
				button->labelcolor(alpha >= 1 ? FL_FOREGROUND_COLOR : FL_INACTIVE_COLOR);
	}

	if(inst.conf.features.udmf_multipletags)
		multiTagView->setTags(L->moreIDs);

	// Update category summaries for collapsed headers
	for(const CategoryHeader &header : categoryHeaders)
		if(header.button)
			updateCategorySummary(header);

	// Update override settings summary
	updateOverrideSummary();
}


void UI_LineBox::UpdateSides()
{
	front->UpdateField();
	 back->UpdateField();
}


void UI_LineBox::UnselectPics()
{
	front->UnselectPics();
	 back->UnselectPics();
}


void UI_LineBox::CalcLength()
{
	// ASSERT(obj >= 0);

	int n = obj;

	float len_f = static_cast<float>(inst.level.calcLength(*inst.level.linedefs[n]));

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "%1.0f", len_f);

	mFixUp.setInputValue(length, buffer);
}

bool UI_LineBox::updateDynamicFlagControls(int lineFlags, int flagSet)
{
	// Set dynamic line flag buttons
	bool changed = false;
	for(const auto &btn : flagButtons)
	{
		if(!btn.data || (flagSet == 1 && !btn.data->mask) || (flagSet == 2 && !btn.data->mask2))
			continue;
		const int newValue = ((flagSet == 1 && btn.data->mask & lineFlags) ||
							  (flagSet == 2 && btn.data->mask2 & lineFlags)) ? 1 : 0;
		if(btn.button)
		{
			changed = changed || (btn.button->value() != newValue);
			btn.button->value(newValue);
		}
		else if(btn.udmfActivationMenuIndex >= 0)
		{
			Fl_Menu_Item &item = udmfActivationMenuItems[btn.udmfActivationMenuIndex];
			changed = changed || (item.value() != newValue);
			item.value(newValue);
		}
	}
	return changed;
}

bool UI_LineBox::FlagsFromInt(int lineflags)
{
	// compute activation
	if (inst.loaded.levelFormat == MapFormat::hexen)
	{
		int new_act = (lineflags & MLF_Activation) >> 9;

		new_act |= (lineflags & MLF_Repeatable) ? 1 : 0;

		// show "??" for unknown values
		int count = getActivationCount();
		if (new_act > count)
			new_act = count;

		actkind->value(new_act);
	}

	bool gotValue = false;
	if(inst.level.isLinedef(obj) && inst.loaded.levelFormat == MapFormat::udmf &&
	   UDMF_HasLineFeature(inst.conf, UDMF_LineFeature::revealed))
	{
		const auto L = inst.level.linedefs[obj];
		if((L->flags2 & MLF2_UDMF_Revealed) && (L->flags & MLF_Secret))
			f_automap->value(7); // RevSecret
		else if(L->flags2 & MLF2_UDMF_Revealed)
			f_automap->value(6); // Revealed
		gotValue = true;
	}
	if(!gotValue)
	{
		// Standard automap flags
		if(lineflags & MLF_DontDraw && lineflags & MLF_Secret)
			f_automap->value(5);
		else if (lineflags & MLF_DontDraw)
			f_automap->value(1);
		else if(lineflags & MLF_Mapped && lineflags & MLF_Secret)
			f_automap->value(4);
		else if (lineflags & MLF_Secret)
			f_automap->value(3);
		else if (lineflags & MLF_Mapped)
			f_automap->value(2);
		else
			f_automap->value(0);
	}

	return updateDynamicFlagControls(lineflags, 1);
}

bool UI_LineBox::Flags2FromInt(int lineflags)
{
	return updateDynamicFlagControls(lineflags, 2);
}

void UI_LineBox::setUDMFActivationLabel(const int flags, const int flags2)
{
	if(!udmfActivationButton->visible())
		return;

	fl_font(FLAG_LABELSIZE, udmfActivationButton->labelsize());
	const int maxWidth = udmfActivationButton->w() - 32;

	SString left, right;

	auto add = [&left, &right, maxWidth](int whichFlags, int flag, const char *what,
										 SString &toWhich)
	{
		if(!(whichFlags & flag))
			return;
		SString berth = left + right + what;
		if(fl_width(berth.c_str()) > maxWidth)
			throw true;
		toWhich += what;
	};

	try
	{
		add(flags, MLF_UDMF_PlayerCross, "W", left);
		add(flags, MLF_UDMF_PlayerUse, "S", left);
		add(flags, MLF_UDMF_Impact, "G", left);
		add(flags, MLF_UDMF_RepeatSpecial, "R", right);
		add(flags, MLF_UDMF_PlayerPush, "P", left);
		add(flags, MLF_UDMF_MonsterCross, "M", left);
		add(flags, MLF_UDMF_MonsterUse, "N", left);
		add(flags, MLF_UDMF_MonsterPush, "Q", left);
		add(flags, MLF_UDMF_MissileCross, "X", left);
		add(flags2, MLF2_UDMF_AnyCross, "A", left);
		add(flags2, MLF2_UDMF_CheckSwitchRange, "r", right);
		add(flags2, MLF2_UDMF_FirstSideOnly, "f", right);
		add(flags2, MLF2_UDMF_PlayerUseBack, "b", right);
		add(flags2, MLF2_UDMF_MonsterActivate, "m", right);
		add(flags2, MLF2_UDMF_DamageSpecial, "🪓", right);
		add(flags2, MLF2_UDMF_DeathSpecial, "💥", right);
	}
	catch(bool)
	{
		right += "…";
	}

	if(left.empty() && right.empty())
		udmfActivationButton->label(UDMF_ACTIVATION_BUTTON_LABEL);
	else
		udmfActivationButton->copy_label((left + right).c_str());
}

void UI_LineBox::CalcFlags(int &outFlags, int &outFlags2) const
{
	outFlags = 0;
	outFlags2 = 0;

	int automapVal = f_automap->value();
	// Hardcoded visibility flags
	switch (automapVal)
	{
		case 0: /* Normal    */; break;
		case 1: /* Invisible */ outFlags |= MLF_DontDraw; break;
		case 2: /* Mapped    */ outFlags |= MLF_Mapped; break;
		case 3: /* Secret    */ outFlags |= MLF_Secret; break;
		case 4: /* MapSecret */ outFlags |= MLF_Mapped | MLF_Secret; break;
		case 5: /* InvSecret */ outFlags |= MLF_DontDraw | MLF_Secret; break;
		case 6: /* Revealed  */
			if(UDMF_HasLineFeature(inst.conf, UDMF_LineFeature::revealed))
				outFlags2 |= MLF2_UDMF_Revealed;
			break;
		case 7: /* RevSecret */
			if(UDMF_HasLineFeature(inst.conf, UDMF_LineFeature::revealed))
				outFlags2 |= MLF2_UDMF_Revealed;
			outFlags |= MLF_Secret;
			break;
	}

	// Activation for non-DOOM formats
	if (inst.loaded.levelFormat == MapFormat::hexen)
	{
		int actval = actkind->value();
		if (actval >= getActivationCount())
			actval = 0;
		outFlags |= (actval << 9);
	}

	// Apply dynamic flags
	for(const auto &btn : flagButtons)
	{
		if(!btn.data || (btn.button && !btn.button->value()) ||
		   (btn.udmfActivationMenuIndex >= 0 &&
			!udmfActivationMenuItems[btn.udmfActivationMenuIndex].value()))
		{
			continue;
		}
		outFlags |= btn.data->mask;
		outFlags2 |= btn.data->mask2;
	}
}


void UI_LineBox::UpdateTotal(const Document &doc) noexcept
{
	which->SetTotal(doc.numLinedefs());
}


int UI_LineBox::SolidMask(const LineDef *L, Side side) const
{
	SYS_ASSERT(L);

	if (L->left < 0 && L->right < 0)
		return 0;

	if (L->left < 0 || L->right < 0)
		return SOLID_MID;

	const Sector *right = &inst.level.getSector(*inst.level.getRight(*L));
	const Sector * left = &inst.level.getSector(*inst.level.getLeft(*L));

	if (side == Side::left)
		std::swap(left, right);

	int mask = 0;

	if (right->floorh < left->floorh)
		mask |= SOLID_LOWER;

	// upper texture of "-" is OK between two skies
	bool two_skies = inst.is_sky(right->CeilTex()) && inst.is_sky(left->CeilTex());

	if (right-> ceilh > left-> ceilh && !two_skies)
		mask |= SOLID_UPPER;

	return mask;
}


void UI_LineBox::UpdateGameInfo(const LoadingData &loaded, const ConfigData &config)
{
	if (loaded.levelFormat == MapFormat::hexen)
	{
		tag->hide();
		gen->hide();

		actkind->clear();
		actkind->add(getActivationMenuString());

		actkind->show();
	}
	else if(loaded.levelFormat == MapFormat::udmf)
	{
		tag->show();
		tag->label("Line ID:");
		tag->tooltip("Tag of the linedef");

		actkind->hide();	// UDMF uses the separate line flags for activation

		if (config.features.gen_types)
			gen->show();
		else
			gen->hide();
	}
	else
	{
		tag->show();
		tag->label("Tag:");
		tag->tooltip("Tag of targeted sector(s)");

		actkind->hide();

		if (config.features.gen_types)
			gen->show();
		else
			gen->hide();
	}

	// Populate f_automap based on UDMF features
	int automapMask = MLF_ALL_AUTOMAP;
	int automapMask2 = 0;
	f_automap->clear();
	if(loaded.levelFormat == MapFormat::udmf && UDMF_HasLineFeature(config, UDMF_LineFeature::revealed))
	{
		// Use extended menu with Revealed entries
		f_automap->add(kHardcodedAutoMapMenuWithRevealed);
		automapMask2 |= MLF2_UDMF_Revealed;
	}
	else
	{
		// Use default hardcoded entries
		f_automap->add(kHardcodedAutoMapMenu);
	}
	f_automap->value(0);

	// Update callback data with new masks
	f_automap_cb_data = std::make_unique<line_flag_CB_data_c>(this, automapMask, automapMask2);
	f_automap->callback(flags_callback, f_automap_cb_data.get());

	// Recreate dynamic linedef flags from configuration
	//for(const auto &btn : flagButtons)
	//	this->remove(btn.button.get());
	for(const auto &cat : categoryHeaders)
	{
		if(cat.button)
			panel->remove(cat.button);
		delete cat.button;
		panel->remove(cat.grid);
		delete cat.grid;
	}
	categoryHeaders.clear();
	flagButtons.clear();
	alphaWidget = nullptr;
	memset(udmfTranslucencyCheckBoxes, 0, sizeof(udmfTranslucencyCheckBoxes));
	memset(mFeatureToButton, 0, sizeof(mFeatureToButton));
	loadUDMFBaseFlags(loaded, config);
	loadUDMFActivationMenu(loaded, config);
	loadUDMFBlockingFlags(loaded, config);
	loadUDMFRenderingControls(loaded, config);

	const std::vector<lineflag_t> &flaglist = config.line_flags;

	if(loaded.levelFormat != MapFormat::udmf && !flaglist.empty())
	{
		// Group flags by category (case-insensitive)
		struct CaseInsensitiveCompare
		{
			bool operator()(const SString &a, const SString &b) const
			{
				return a.noCaseCompare(b) < 0;
			}
		};
		std::map<SString, std::vector<const lineflag_t *>, CaseInsensitiveCompare> categorized;
		for(const lineflag_t &f : flaglist)
		{
			SString catName = f.category.empty() ? SString("") : f.category;
			categorized[catName].push_back(&f);
		}

		static const int FW = 110;
		const int leftX = x() + INSET_LEFT + 28;
		const int rightX = x() + INSET_LEFT + flagsAreaW - 120;

		// Process each category
		for(auto &catPair : categorized)
		{
			const SString &catName = catPair.first;
			const std::vector<const lineflag_t *> &flagsInCat = catPair.second;

			CategoryHeader catHeader = {};
			if(!catName.empty())
			{
				catHeader.button = new UI_CategoryButton(x() + INSET_LEFT, 0,
					flagsAreaW, FIELD_HEIGHT);
				catHeader.button->copy_label(catName.c_str());
				catHeader.button->callback(category_callback, this);
			}
			const int numRows = (int(flagsInCat.size()) + 1) / 2;
			catHeader.grid = new Fl_Grid(leftX, 0, FW + rightX - leftX,
													   FLAG_ROW_HEIGHT * numRows);
			catHeader.grid->layout(numRows, 2);

			struct Slot
			{
				const lineflag_t* a = nullptr;
				const lineflag_t* b = nullptr;
			};

			std::vector<Slot> slots;
			slots.reserve(flagsInCat.size());
			for(size_t i = 0; i < flagsInCat.size(); ++i)
			{
				const lineflag_t* f = flagsInCat[i];
				if(f->pairIndex == 0 && i + 1 < flagsInCat.size() && flagsInCat[i + 1]->pairIndex == 1)
				{
					Slot s;
					s.a = f;
					s.b = flagsInCat[i + 1];
					slots.push_back(s);
					++i; // consume the pair
				}
				else
				{
					Slot s;
					if(f->pairIndex == 1)
						s.b = f;
					else
						s.a = f;
					slots.push_back(s);
				}
			}

			const int total = (int)slots.size();
			const int leftCount = (total + 1) / 2;

			for(int idx = 0; idx < total; ++idx)
			{
				const Slot& s = slots[idx];
				const bool onLeft = idx < leftCount;
				const int baseX = onLeft ? leftX : rightX;

				auto addButton = [baseX, this, &catHeader](int offset, const lineflag_t *flag)
					{
						LineFlagButton fb;
						fb.button = new Fl_Check_Button(baseX + offset, 0, FW, 20,
							flag->label.c_str());
						fb.button->labelsize(FLAG_LABELSIZE);
						fb.data = std::make_unique<line_flag_CB_data_c>(this, flag->flagSet == 1 ?
							flag->value : 0, flag->flagSet == 2 ? flag->value : 0);
						fb.button->callback(flags_callback, fb.data.get());
						if(catHeader.button)
							catHeader.lineFlagButtonIndices.push_back((int)flagButtons.size());
						flagButtons.push_back(std::move(fb));
					};

				if(s.a)
					addButton(0, s.a);
				if(s.b)
					addButton(16, s.b);

				if(s.b)
				{
					// If we have a B, we deal with a pair, so make a group to assign it into the grid
					Fl_Group *pairGroup = new Fl_Group(baseX, 0, FW, FLAG_ROW_HEIGHT);
					pairGroup->end();

					if(s.a)
						pairGroup->add(flagButtons[flagButtons.size() - 2].button);
					pairGroup->add(flagButtons.back().button);
					catHeader.grid->add(pairGroup);
					catHeader.grid->widget(pairGroup, idx % leftCount, onLeft ? 0 : 1);
				}
				else if(s.a)
				{
					Fl_Widget *widget = flagButtons.back().button;
					catHeader.grid->add(widget);
					catHeader.grid->widget(widget, idx % leftCount, onLeft ? 0 : 1);
				}
			}

			if(catHeader.button)
				panel->insert(*catHeader.button, widgetAfterFlags);
			panel->insert(*catHeader.grid, widgetAfterFlags);
			categoryHeaders.push_back(std::move(catHeader));
		}
	}

	overrideHeader->hide();
	for(const UDMFIntChoiceField &entry : udmfIntChoiceFields)
	{
		(this->*entry.choice)->hide();
		(this->*entry.choice)->clear();
	}
	if(loaded.levelFormat == MapFormat::udmf)
	{
		bool hasOne = false;
		for(const UDMFIntChoiceField &entry : udmfIntChoiceFields)
		{
			if(!UDMF_HasLineFeature(config, entry.feature))
				continue;
			if(overrideHeader->isExpanded())
				(this->*entry.choice)->show();
			hasOne = true;
			for(const linefield_t &field : config.udmf_line_fields)
			{
				if(!field.identifier.noCaseEqual(entry.identifier))
				{
					continue;
				}
				for(const linefield_t::option_t &option : field.options)
					(this->*entry.choice)->add((SString(option.value) + ": " + option.label).c_str());
			}
		}
		if(hasOne)
			overrideHeader->show();
	}

	// Reposition side boxes under the generated flags and choice widgets
	//front->Fl_Widget::position(front->x(), Y);
	//Y += front->h() + 14;
	//back->Fl_Widget::position(back->x(), Y);
	//front->redraw();
	//back->redraw();

	// Show Hexen/UDMF args when needed

	if (loaded.levelFormat == MapFormat::hexen || loaded.levelFormat == MapFormat::udmf)
	{
		argsFlex->show();
		args0str->hide();
		for(int a = 0; a < 5; ++a)
		{
			args[a]->label("");
		}
	}
	else
		argsFlex->hide();

	if(loaded.levelFormat == MapFormat::udmf && config.features.udmf_multipletags)
	{
		multiTagView->show();
		multiTagView->clearTags();
	}
	else
		multiTagView->hide();

	// Update UDMF sidepart widgets for sidedefs
	front->UpdateGameInfo(loaded, config);
	back->UpdateGameInfo(loaded, config);

	redraw();

	for(CategoryHeader& header : categoryHeaders)
	{
		if(header.button)
			header.button->setExpanded(false);
	}
	categoryToggled(nullptr);  // Make sure all shows correctly with collapsed categories
}


const char * UI_LineBox::GeneralizedDesc(int type_num)
{
	if (! inst.conf.features.gen_types)
		return NULL;

	static char desc_buffer[256];

	for (int i = 0 ; i < inst.conf.num_gen_linetypes ; i++)
	{
		const generalized_linetype_t *info = &inst.conf.gen_linetypes[i];

		if (type_num >= info->base && type_num < (info->base + info->length))
		{
			// grab trigger name (we assume it is first field)
			if (info->fields.size() < 1 || info->fields[0].keywords.size() < 8)
				return NULL;

			const char *trigger = info->fields[0].keywords[type_num & 7].c_str();

			snprintf(desc_buffer, sizeof(desc_buffer), "%s GENTYPE: %s", trigger, info->name.c_str());
			return desc_buffer;
		}
	}

	return NULL;  // not a generalized linetype
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
