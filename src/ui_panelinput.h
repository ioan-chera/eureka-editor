//------------------------------------------------------------------------
//  Panel input fixing of problems
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025      Ioan Chera
//  Copyright (C) 2001-2019 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __EUREKA_UI_PANELINPUT_H__
#define __EUREKA_UI_PANELINPUT_H__

#include "e_basis.h"
#include "FL/Fl_Input.H"
#include "FL/Fl_Scroll.H"
#include <optional>
#include <unordered_map>
#include <unordered_set>

class Instance;
class UI_Nombre;
struct ConfigData;
struct Document;
struct LoadingData;

enum
{
	NOMBRE_INSET = 6,
	NOMBRE_HEIGHT = 28
};

//
// Interface for providing "callback2" to controls
//
class ICallback2
{
public:
	virtual void callback2(Fl_Callback *callback, void *data) = 0;
	virtual Fl_Callback *callback2() const = 0;
	virtual void *user_data2() const = 0;

	// Necessary indicators for our task. Oh, boilerplate :(
	virtual Fl_Callback *getMainCallback() const = 0;
	virtual void *getMainUserData() const = 0;
	virtual void setMainCallback(Fl_Callback *callback, void *data) = 0;
	virtual void setValue(const char *value) = 0;
	virtual Fl_Widget *asWidget() = 0;
};

#define ICALLBACK2_BOILERPLATE() \
Fl_Callback *getMainCallback() const override { return callback(); } \
void *getMainUserData() const override { return user_data(); } \
void setMainCallback(Fl_Callback *cb, void *data) override { callback(cb, data); } \
void setValue(const char *v) override { value(v); } \
Fl_Widget *asWidget() override { return static_cast<Fl_Widget *>(this); }

//
// Fix-up class for panel fields to make sure they work as expected when clicking buttons
//
class PanelFieldFixUp
{
public:
	void loadFields(std::initializer_list<ICallback2 *> fields);

	// Call it before starting basis
	void checkDirtyFields();
	void setInputValue(ICallback2 *input, const char *value) noexcept;

private:
	//
	// Callback info, for storage
	//
	struct CallbackInfo
	{
		Fl_Callback *callback;
		void *data;
	};

	static void setDirtyCallback(Fl_Widget *widget, void *data);
	static void clearDirtyCallback(Fl_Widget *widget, void *data);

	std::unordered_map<ICallback2 *, CallbackInfo> mOriginalCallbacks;
	std::unordered_map<ICallback2 *, CallbackInfo> mOriginalCallbacks2;
	std::unordered_map<Fl_Widget *, ICallback2 *> mAsCallback2;
	std::unordered_map<ICallback2 *, Fl_Widget *> mAsWidget;

	// Set of "dirty" edit fields. The rule is as such:
	// - Add them to list when user writes text without exiting or pressing ENTER
	//   (that's why they need to be UI_DynInput or UI_DynIntInput)
	// - Remove them from list when their callback is invoked (caution: only for them)
	// - Remove them from list when their value() is changed externally.
	// - Before any basis.begin() or external function which does that, call checkDirtyFields()
	std::unordered_set<ICallback2 *> mDirtyFields;
};


class MapItemBox : public Fl_Scroll
{
public:
	MapItemBox(Instance &inst, int X, int Y, int W, int H, const char *label = nullptr);

	void SetObj(int _index, int _count);
	int GetObj() const { return obj; }

	virtual void UpdateField(std::optional<Basis::EditField> efield = std::nullopt) = 0;
	virtual void UnselectPics() = 0;
	virtual void UpdateTotal(const Document &doc) noexcept = 0;
	virtual void UpdateGameInfo(const LoadingData &loaded, const ConfigData &config) = 0;

protected:
	Instance &inst;
	int obj = -1;
	int count = 0;

	UI_Nombre *which = nullptr;

	PanelFieldFixUp	mFixUp;
};

#endif
