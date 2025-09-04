//------------------------------------------------------------------------
//  SORTABLE TABLE CONTROL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025 Ioan Chera
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

#ifndef __EUREKA_UI_SORTTABLE_H__
#define __EUREKA_UI_SORTTABLE_H__
#endif

#include "FL/Fl_Table_Row.H"

#include "m_strings.h"

#include <array>
#include <string>
#include <vector>

class UI_KeyBindingsTable : public Fl_Table_Row
{
public:
    UI_KeyBindingsTable(int X, int Y, int W, int H, void (*sortCallback)(int column, bool reverse, void*), void *sortContext);
    int getSelectedIndex() const;
    void selectRowAtIndex(int index)
    {
        select_row(index, 1);
    }
    void setChallenge(int index);
    int getChallenged() const
    {
        return challengedIndex;
    }
    void clearChallenge();
    void reload();

    void (*sortCallback)(int column, bool reverse, void*) = nullptr;
    void *sortContext = nullptr;

protected:
    void draw_cell(TableContext context, int R = 0, int C = 0, int X = 0, int Y = 0, int W = 0,
        int H = 0) override;
private:
    struct CacheRow
    {
        SString cols[3];
        bool is_duplicate;
    };

    static void eventCallback(Fl_Widget *w, void *data);
    void eventCallback2();
    void autoWidth(int pad);
    void drawSortArrow(int X, int Y, int W, int H);
    void sortColumn(int column, bool reverse);
    void rebuildCache();

    bool challengedIndex = -1;
    bool sortReverse = false;
    int lastSortedColumn = -1;
    std::vector<CacheRow> cache;
};
