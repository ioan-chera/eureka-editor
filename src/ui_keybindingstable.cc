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

#include "ui_keybindingstable.h"
#include "m_keys.h"

#include "FL/fl_draw.H"

static const Fl_Font HEADER_FONTFACE = FL_HELVETICA_BOLD;
static const Fl_Fontsize HEADER_FONTSIZE = 16;
static const Fl_Font ROW_FONTFACE = FL_HELVETICA;
static const Fl_Fontsize ROW_FONTSIZE = 16;
static const int AUTO_FIT_PADDING = 20;
static const char *HEADINGS[3] = { "Key", "Mode", "Function" };

#ifdef __APPLE__
static SString formatKeyForMac(const SString &key)
{
    SString result = key;
    
    // Replace CMD- with ⌘
    size_t pos = result.find("CMD-");
    while (pos != SString::npos) 
    {
        result.erase(pos, 4);
        result.insert(pos, "⌘");
        pos = result.find("CMD-", pos + 1);
    }
    
    // Replace META- with ⌃
    pos = result.find("META-");
    while (pos != SString::npos) 
    {
        result.erase(pos, 5);
        result.insert(pos, "⌃");
        pos = result.find("META-", pos + 1);
    }
    
    // Replace ALT- with ⌥
    pos = result.find("ALT-");
    while (pos != SString::npos) 
    {
        result.erase(pos, 4);
        result.insert(pos, "⌥");
        pos = result.find("ALT-", pos + 1);
    }
    
    // Replace SHIFT- with ⇧
    pos = result.find("SHIFT-");
    while (pos != SString::npos) 
    {
        result.erase(pos, 6);
        result.insert(pos, "⇧");
        pos = result.find("SHIFT-", pos + 1);
    }
    
    // Replace PGUP with ⇞
    pos = result.find("PGUP");
    while (pos != SString::npos) 
    {
        result.erase(pos, 4);
        result.insert(pos, "⇞");
        pos = result.find("PGUP", pos + 1);
    }
    
    // Replace PGDN with ⇟
    pos = result.find("PGDN");
    while (pos != SString::npos) 
    {
        result.erase(pos, 4);
        result.insert(pos, "⇟");
        pos = result.find("PGDN", pos + 1);
    }
    
    // Replace SPACE with ␣
    pos = result.find("SPACE");
    while (pos != SString::npos) 
    {
        result.erase(pos, 5);
        result.insert(pos, "␣");
        pos = result.find("SPACE", pos + 1);
    }
    
    // Replace TAB with ⇥
    pos = result.find("TAB");
    while (pos != SString::npos) 
    {
        result.erase(pos, 3);
        result.insert(pos, "⇥");
        pos = result.find("TAB", pos + 1);
    }
    
    // Replace LEFT with ←
    pos = result.find("LEFT");
    while (pos != SString::npos) 
    {
        result.erase(pos, 4);
        result.insert(pos, "←");
        pos = result.find("LEFT", pos + 1);
    }
    
    // Replace RIGHT with →
    pos = result.find("RIGHT");
    while (pos != SString::npos) 
    {
        result.erase(pos, 5);
        result.insert(pos, "→");
        pos = result.find("RIGHT", pos + 1);
    }
    
    // Replace UP with ↑ (after PGUP replacement, avoid WHEEL_UP)
    pos = result.find("UP");
    while (pos != SString::npos) 
    {
        // Check if this is part of WHEEL_UP
        if (pos >= 6 && result.substr(pos - 6, 8) == "WHEEL_UP")
        {
            pos = result.find("UP", pos + 2);
            continue;
        }
        result.erase(pos, 2);
        result.insert(pos, "↑");
        pos = result.find("UP", pos + 1);
    }
    
    // Replace DOWN with ↓ (avoid WHEEL_DOWN)
    pos = result.find("DOWN");
    while (pos != SString::npos) 
    {
        // Check if this is part of WHEEL_DOWN
        if (pos >= 6 && result.substr(pos - 6, 10) == "WHEEL_DOWN")
        {
            pos = result.find("DOWN", pos + 4);
            continue;
        }
        result.erase(pos, 4);
        result.insert(pos, "↓");
        pos = result.find("DOWN", pos + 1);
    }
    
    // Replace DEL with ⌫
    pos = result.find("DEL");
    while (pos != SString::npos) 
    {
        result.erase(pos, 3);
        result.insert(pos, "⌦");
        pos = result.find("DEL", pos + 1);
    }
    
    // Replace BS with ⌦
    pos = result.find("BS");
    while (pos != SString::npos) 
    {
        result.erase(pos, 2);
        result.insert(pos, "⌫");
        pos = result.find("BS", pos + 1);
    }
    
    return result;
}
#endif

UI_KeyBindingsTable::UI_KeyBindingsTable(int X, int Y, int W, int H, void (*sortCallback)(int col, bool rev, void*), void *sortContext) : Fl_Table_Row(X, Y, W, H, nullptr), sortCallback(sortCallback), sortContext(sortContext)
{
    cols(3);
    col_header(1);
    col_resize(1);
    row_header(0);
	row_resize(0);
    when(FL_WHEN_RELEASE);
    selection_color(FL_SELECTION_COLOR);
    row_height_all(18);
    color(FL_WHITE);
    type(SELECT_SINGLE);
    rows((int)global::pref_binds.size());
    rebuildCache();
    autoWidth(AUTO_FIT_PADDING);
    end();

    callback(eventCallback, this);
}

int UI_KeyBindingsTable::getSelectedIndex() const
{
    for(int i = 0; i < const_cast<UI_KeyBindingsTable *>(this)->rows(); i++)
        if(const_cast<UI_KeyBindingsTable *>(this)->row_selected(i))
            return i;
    return -1;
}

void UI_KeyBindingsTable::setChallenge(int index)
{
    challengedIndex = index;
    selection_color(FL_YELLOW);
    redraw();
}

void UI_KeyBindingsTable::clearChallenge()
{
    challengedIndex = -1;
    selection_color(FL_SELECTION_COLOR);
    redraw();
}

void UI_KeyBindingsTable::reload()
{
    rebuildCache();
    rows((int)cache.size());
    autoWidth(AUTO_FIT_PADDING);
    redraw();
}

void UI_KeyBindingsTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H)
{
    SString s;
    Fl_Color bgcolor;
    if(R < 0 || R >= rows() || C < 0 || C >= cols())
        return;
    switch(context)
    {
    case CONTEXT_COL_HEADER:
        fl_push_clip(X, Y, W, H);
        fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, FL_BACKGROUND_COLOR);
        fl_font(HEADER_FONTFACE, HEADER_FONTSIZE);
        fl_color(FL_BLACK);
        fl_draw(HEADINGS[C], X + 2, Y, W, H, FL_ALIGN_LEFT, nullptr, 0);
        if(C == lastSortedColumn)
            drawSortArrow(X, Y, W, H);
        fl_pop_clip();
        break;
    case CONTEXT_CELL:
        fl_push_clip(X, Y, W, H);
        bgcolor = row_selected(R) ? selection_color() : FL_WHITE;
        fl_color(bgcolor);
        fl_rectf(X, Y, W, H);
        fl_font(ROW_FONTFACE, ROW_FONTSIZE);
        fl_color(cache[R].is_duplicate ? FL_RED : row_selected(R) && selection_color() == FL_SELECTION_COLOR ? FL_WHITE : FL_BLACK);
        if(C == 0 && challengedIndex == R)
            fl_draw("<?>", X + 2, Y, W, H, FL_ALIGN_LEFT);
        else
            fl_draw(cache[R].cols[C].c_str(), X + 2, Y, W, H, FL_ALIGN_LEFT);
        fl_color(FL_LIGHT2);
        fl_rect(X, Y, W, H);
        fl_pop_clip();
        break;
    default:
        break;
    }
}

void UI_KeyBindingsTable::eventCallback(Fl_Widget *w, void *data)
{
    auto table = static_cast<UI_KeyBindingsTable *>(data);
    table->eventCallback2();
}

void UI_KeyBindingsTable::eventCallback2()
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

void UI_KeyBindingsTable::autoWidth(int pad)
{
    int w, h;
    fl_font(HEADER_FONTFACE, HEADER_FONTSIZE);
    for(int c = 0; c < 3; c++)
    {
        w = 0;
        fl_measure(HEADINGS[c], w, h, 0);
        col_width(c, w + pad);
    }

    fl_font(ROW_FONTFACE, ROW_FONTSIZE);
    for(const auto &row : cache)
        for(int c = 0; c < 3; c++)
        {
            w = 0;
            fl_measure(row.cols[c].c_str(), w, h, 0);
            if(w + pad > col_width(c))
                col_width(c, w + pad);
        }
    table_resized();
    redraw();
}

void UI_KeyBindingsTable::drawSortArrow(int X, int Y, int W, int H)
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

void UI_KeyBindingsTable::sortColumn(int column, bool reverse)
{
    if(sortCallback)
        sortCallback(column, reverse, sortContext);
    redraw();
}

void UI_KeyBindingsTable::rebuildCache()
{
    cache.clear();
    cache.reserve(global::pref_binds.size());
    for(size_t i = 0; i < global::pref_binds.size(); ++i)
    {
        const key_binding_t &binding = global::pref_binds[i];
        CacheRow cacheRow = {};
#ifdef __APPLE__
        cacheRow.cols[0] = formatKeyForMac(keys::toString(binding.key));
#else
        cacheRow.cols[0] = keys::toString(binding.key);
#endif
        cacheRow.cols[1] = M_KeyContextString(binding.context);
        if(cacheRow.cols[1].noCaseEqual("render"))
            cacheRow.cols[1] = "3D view";
        cacheRow.cols[2] = keys::stringForFunc(binding);
        cacheRow.is_duplicate = binding.is_duplicate;
        cache.push_back(std::move(cacheRow));
    }
}
