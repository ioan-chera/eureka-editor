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

#include "ui_sorttable.h"

#include "FL/fl_draw.H"

static const Fl_Font HEADER_FONTFACE = FL_HELVETICA_BOLD;
static const Fl_Fontsize HEADER_FONTSIZE = 16;
static const Fl_Font ROW_FONTFACE = FL_HELVETICA;
static const Fl_Fontsize ROW_FONTSIZE = 16;
static const int AUTO_FIT_PADDING = 20;

UI_SortTable::UI_SortTable(int X, int Y, int W, int H,
    const std::array<const char *, NUM_COLS> &headings) : Fl_Table_Row(X, Y, W, H, nullptr),
    headings(headings)
{
    cols(NUM_COLS);
    col_header(1);
    col_resize(1);
    row_header(0);
	row_resize(0);
    when(FL_WHEN_RELEASE);
    selection_color(FL_SELECTION_COLOR);
    row_height_all(18);
    color(FL_WHITE);
    type(SELECT_SINGLE);
    end();

    callback(eventCallback, this);
}

void UI_SortTable::setData(const std::vector<std::array<std::string, NUM_COLS>> &newData)
{
    data = newData;
    rows((int)data.size());
    autoWidth(AUTO_FIT_PADDING);
}

void UI_SortTable::setData(std::vector<std::array<std::string, NUM_COLS>> &&newData)
{
    data = std::move(newData);
    rows((int)data.size());
    autoWidth(AUTO_FIT_PADDING);
}

int UI_SortTable::getSelectedIndex() const
{
    for(int i = 0; i < const_cast<UI_SortTable *>(this)->rows(); i++)
        if(const_cast<UI_SortTable *>(this)->row_selected(i))
            return i;
    return -1;
}

void UI_SortTable::setRowText(int row, const std::array<std::string, NUM_COLS> &data)
{
    if(row < 0 || row >= (int)this->data.size())
        return;
    this->data[row] = data;
    redraw();
}

void UI_SortTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H)
{
    const std::string *s = nullptr;
    Fl_Color bgcolor;
    if(R < (int)data.size() && C < NUM_COLS)
        s = &data[R][C];
    switch(context)
    {
    case CONTEXT_COL_HEADER:
        fl_push_clip(X, Y, W, H);
        fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, FL_BACKGROUND_COLOR);
        if(C < NUM_COLS)
        {
            fl_font(HEADER_FONTFACE, HEADER_FONTSIZE);
            fl_color(FL_BLACK);
            fl_draw(headings[C], X + 2, Y, W, H, FL_ALIGN_LEFT, nullptr, 0);
            if(C == lastSortedColumn)
                drawSortArrow(X, Y, W, H);
        }
        fl_pop_clip();
        break;
    case CONTEXT_CELL:
        fl_push_clip(X, Y, W, H);
        bgcolor = row_selected(R) ? selection_color() : FL_WHITE;
        fl_color(bgcolor);
        fl_rectf(X, Y, W, H);
        fl_font(ROW_FONTFACE, ROW_FONTSIZE);
        fl_color(FL_BLACK);
        if(s)
            fl_draw(s->c_str(), X + 2, Y, W, H, FL_ALIGN_LEFT);
        fl_color(FL_LIGHT2);
        fl_rect(X, Y, W, H);
        fl_pop_clip();
        break;
    default:
        break;
    }
}

void UI_SortTable::eventCallback(Fl_Widget *w, void *data)
{
    auto table = static_cast<UI_SortTable *>(data);
    table->eventCallback2();
}

void UI_SortTable::eventCallback2()
{
    int column = callback_col();
    TableContext context = callback_context();
    switch(context)
    {
    case CONTEXT_COL_HEADER:
        if(Fl::event() != FL_RELEASE || Fl::event_button() != 1)
            break;
        if(lastSortedColumn == column)
            sortReverse = !sortReverse;
        else
            sortReverse = false;
        sortColumn(column, sortReverse);
        lastSortedColumn = column;
        break;
    default:
        break;
    }
}

void UI_SortTable::autoWidth(int pad)
{
    int w, h;
    fl_font(HEADER_FONTFACE, HEADER_FONTSIZE);
    for(int c = 0; c < NUM_COLS; c++)
    {
        w = 0;
        fl_measure(headings[c], w, h, 0);
        col_width(c, w + pad);
    }

    fl_font(ROW_FONTFACE, ROW_FONTSIZE);
    for(const auto &row : data)
        for(int c = 0; c < NUM_COLS; c++)
        {
            w = 0;
            fl_measure(row[c].c_str(), w, h, 0);
            if(w + pad > col_width(c))
                col_width(c, w + pad);
        }
    table_resized();
    redraw();
}

void UI_SortTable::drawSortArrow(int X, int Y, int W, int H)
{
    int xLeft = X + W - 6 - 8;
    int xCenter = X + W - 6 - 4;
    int xRight = X + W - 6;
    int yTop = Y + H / 2 - 4;
    int yBottom = Y + H / 2 + 4;
    if(sortReverse)
    {
        fl_color(FL_WHITE);
        fl_line(xRight, yTop, xCenter, yBottom);
        fl_color(41);
        fl_line(xLeft, yTop, xRight, yTop);
        fl_line(xLeft, yTop, xCenter, yBottom);
    }
    else
    {
        fl_color(FL_WHITE);
        fl_line(xRight, yBottom, xCenter, yTop);
        fl_line(xRight, yBottom, xLeft, yBottom);
        fl_color(41);
        fl_line(xLeft, yBottom, xCenter, yTop);
    }
}

void UI_SortTable::sortColumn(int column, bool reverse)
{
    // TODO: sort the data source
    redraw();
}
