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

#include <array>
#include <string>
#include <vector>

enum
{
    NUM_COLS = 3,   // currently hardcoded
};

class UI_SortTable : public Fl_Table_Row
{
public:
    UI_SortTable(int X, int Y, int W, int H, const std::array<const char *, NUM_COLS> &headings);
    void setData(const std::vector<std::array<std::string, NUM_COLS>> &newData);
    void setData(std::vector<std::array<std::string, NUM_COLS>> &&newData);
    int getSelectedIndex() const;
    void selectRowAtIndex(int index)
    {
        select_row(index, 1);
    }
    void setRowText(int row, const std::array<std::string, NUM_COLS> &data);

protected:
    void draw_cell(TableContext context, int R = 0, int C = 0, int X = 0, int Y = 0, int W = 0,
        int H = 0) override;
private:
    static void eventCallback(Fl_Widget *w, void *data);
    void eventCallback2();
    void autoWidth(int pad);
    void drawSortArrow(int X, int Y, int W, int H);
    void sortColumn(int column, bool reverse);

    const std::array<const char *, NUM_COLS> headings;
    std::vector<std::array<std::string, NUM_COLS>> data;
    bool sortReverse = false;
    int lastSortedColumn = -1;
};
